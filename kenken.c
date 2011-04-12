/**	@file kenken.c
 *
 *	@author Jonathan Park (jjp1) and Ben Parr (bparr)
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

// Maximum problem size supported by program
#define MAX_PROBLEM_SIZE 25
// Maximum line length of input file
#define MAX_LINE_LEN 2048

// Value used to indicate a cell's value is unassigned
#define UNASSIGNED_VALUE 0
// Number of constraints a cell has (1 row constraint, 1 column constraint,
// 1 block constraint)
#define NUM_CELL_CONSTRAINTS 3
// Value of node at start and end of cell list
#define END_NODE -1

// Indexes of different types of constraints in cell_t constraint array
#define ROW_CONSTRAINT_INDEX 0
#define COLUMN_CONSTRAINT_INDEX 1
#define BLOCK_CONSTRAINT_INDEX 2

// Get cell at (x, y)
#define GET_CELL(x, y) (N * (x) + (y))
// Calculate the minimum of two numbers
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
// Calculate the maximum of two numbers
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef enum {
  LINE,
  PLUS,
  MINUS,
  MULTIPLY,
  DIVIDE,
  SINGLE // TODO remove?? SINGLE should be a degenerate case of PLUS
} type_t;

typedef struct possible { // TODO merge into cell, and rename to intersection?
  char flags[MAX_PROBLEM_SIZE + 1];
  int num;
} possible_t;

typedef struct cellnode {
  int previous;
  int next;
} cellnode_t;

typedef struct celllist {
  int start;
  cellnode_t cells[MAX_PROBLEM_SIZE * MAX_PROBLEM_SIZE];
} celllist_t;

typedef struct constraint {
  type_t type;
  long value;
  int numCells; // TODO Move into cellList?
  celllist_t cellList;
} constraint_t;

typedef struct cell {
  int value;
  possible_t possibles;
  constraint_t* constraints[NUM_CELL_CONSTRAINTS];
} cell_t;

// Funtions to initialize line constraints
void initRowConstraint(constraint_t* constraint, int row);
void initColumnConstraint(constraint_t* constraint, int col);

// Functions to initialize possibles
void initLinePossibles(celllist_t* cellList, char markPossible);
void initPlusPossibles(celllist_t* cellList, long value, int numCells, char markPossible);
void initMinusPossibles(celllist_t* cellList, long value, char markPossible);
void initPartialMinusPossibles(celllist_t* cellList, long value,
                               int cellValue, char markPossible);
void initMultiplyPossibles(celllist_t* cellList, long value, int numCells, char markPossible);
void initDividePossibles(celllist_t* cellList, long value, char markPossible);
void initPartialDividePossibles(celllist_t* cellList, long value,
                                int cellValue, char markPossible);
void initSinglePossibles(celllist_t* cellList, long value, int numCells, char markPossible);

// TODO remove "new"?
// TODO rename functions
void updateLinePossibles(celllist_t* cellList, int old, int new);
void updatePlusPossibles(celllist_t* cellList, long oldValue,
                         int oldNumCells, long newValue, int newNumCells);
void updateMinusPossibles(constraint_t* constraint, int value, int oldCellValue, int oldNumCells, int newCellValue, int newNumCells);
void updateMultiplyPossibles(celllist_t* cellList, long oldValue,
                         int oldNumCells, long newValue, int newNumCells);
void updateDividePossibles(constraint_t* constraint, int value, int oldCellValue, int oldNumCells, int newCellValue, int newNumCells);
void updateSinglePossibles(celllist_t* cellList, int value, int numCells);

// Algorithm functions
int solve(int step);
void updateConstraint(constraint_t* constraint, int oldCellValue,
                      int oldNumCells, int newCellValue, int newNumCells);
void notifyCellsOfChange(celllist_t* cellList, int value, char markPossible);
void notifyCellsOfPossible(celllist_t* cellList, int value);
void notifyCellsOfImpossible(celllist_t* cellList, int value);

// Cell list functions
inline void initList(celllist_t* cellList);
inline void addNode(celllist_t* cellList, int node);
inline void removeNode(celllist_t* cellList, int node);

// Miscellaneous functions
void usage(char* program);
void readLine(FILE* in, char* lineBuf);
void appError(const char* str);
void unixError(const char* str);

// Problem size
int N;
// Total number of cells in the problem
int totalNumCells;
// Problem grid
cell_t* cells;
// Number of constraints
int numConstraints;
// Constraints array
constraint_t* constraints;
// Max number by multiplying
long* maxMultiply;

int main(int argc, char **argv)
{
  FILE* in;
  char type, lineBuf[MAX_LINE_LEN];
  char* ptr;
  int i, x, y, numCells;
  long value;
  constraint_t* constraint;
  celllist_t* cellList;

  // Check arguments
  if (argc != 2)
    usage(argv[0]);

  // Read in file
  if (!(in = fopen(argv[1], "r")))
    unixError("Failed to open input file");

  // Read in problem size and number of constraints
  readLine(in, lineBuf);
  N = atoi(lineBuf);
  if (N > MAX_PROBLEM_SIZE)
    appError("Problem size too large");

  readLine(in, lineBuf);
  // N row constraints + N column constraints + number of block constraints
  numConstraints = 2 * N + atoi(lineBuf);

  // Allocate space for cells and constraints
  totalNumCells = N * N;
  cells = (cell_t*)calloc(sizeof(cell_t), totalNumCells);
  if (!cells)
    unixError("Failed to allocate memory for the cells");

  constraints = (constraint_t*)calloc(sizeof(constraint_t), numConstraints);
  if (!constraints)
    unixError("Failed to allocate memory for the constraints");

  maxMultiply = (long*)calloc(sizeof(long), totalNumCells);
  if (!maxMultiply)
    unixError("Failed to allocate memory for the max multiply array");

  // Initialize max multiply array
  for (i = 0, value = 1; i < totalNumCells; i++) {
    maxMultiply[i] = value;

    if (value > LONG_MAX / N)
      break;
    value *= N;
  }
  for (; i < totalNumCells; i++)
    maxMultiply[i] = LONG_MAX;

  // Initialize row and column constraints
  for (i = 0; i < N; i++) {
    initRowConstraint(&constraints[i], i);
    initColumnConstraint(&constraints[i + N], i);
  }

  // Initialize block constraints
  for (i = 2 * N; i < numConstraints; i++) {
    readLine(in, lineBuf);
    constraint = &(constraints[i]);
    cellList = &(constraint->cellList);

    // Read in type
    ptr = strtok(lineBuf, " ");
    type = *ptr;

    // Read in value
    ptr = strtok(NULL, " ");
    value = atol(ptr);

    // Read in cell coordinates
    numCells = 0;
    initList(cellList);
    while ((ptr = strtok(NULL, ", "))) {
      numCells++;

      x = atoi(ptr);
      ptr = strtok(NULL, ", ");
      y = atoi(ptr);

      // Add block constraint to cell
      addNode(cellList, GET_CELL(x, y));
      cells[GET_CELL(x, y)].constraints[BLOCK_CONSTRAINT_INDEX] = constraint;
    }

    constraint->numCells = numCells;
    constraint->value = value;

    // Initialize constraint's type and possibles
    switch (type) {
      case '+':
        constraint->type = PLUS;
        initPlusPossibles(cellList, value, numCells, 1);
        break;
      case '-':
        constraint->type = MINUS;
        initMinusPossibles(cellList, value, 1);
        break;
      case 'x':
        constraint->type = MULTIPLY;
        initMultiplyPossibles(cellList, value, numCells, 1);
        break;
      case '/':
        constraint->type = DIVIDE;
        initDividePossibles(cellList, value, 1);
        break;
      case '!':
        constraint->type = SINGLE;
        initSinglePossibles(cellList, value, numCells, 1);
        break;
      default:
        appError("Malformed constraint in input file");
    }
  }

  // TODO close file

  // Run algorithm
  if (!solve(0))
    appError("No solution found");

  // Print solution if one found
  for (i = 0; i < totalNumCells; i++)
    printf("%d%c", cells[i].value, ((i + 1) % N != 0) ? ' ' : '\n');

  // Free allocated memory
  free(cells);
  free(constraints);
  free(maxMultiply);

  return 0;
}

// Main recursive function used to solve the program
int solve(int step) {
  int i, j, numPossibles, oldValue = UNASSIGNED_VALUE;
  int minIndex = 0, minPossibles = INT_MAX;
  int oldNumCells, newNumCells;
  cell_t* cell;
  constraint_t* constraint;

  // Success if all cells filled in
  if (step == totalNumCells)
    return 1;

  // Find the unassigned cell with the mininum number of possibilities
  for (i = 0; i < totalNumCells; i++) {
    cell = &(cells[i]);

    // Skip assigned cells
    if (cell->value != UNASSIGNED_VALUE)
      continue;

    numPossibles = cell->possibles.num;

    // Fail early if found unassigned cell with no possibilities
    if (numPossibles == 0)
      return 0;

    if (numPossibles < minPossibles) {
      minIndex = i;
      minPossibles = numPossibles;
    }
  }

  // Use the found cell as the next cell to fill
  cell = &(cells[minIndex]);

  // Remove cell from its constraints
  for (i = 0; i < NUM_CELL_CONSTRAINTS; i++) {
    constraint = cell->constraints[i];

    oldNumCells = constraint->numCells;
    newNumCells = oldNumCells - 1;
    constraint->numCells = newNumCells;

    removeNode(&(constraint->cellList), minIndex);
  }

  // Try all possible values for next cell
  for (i = N; i > 0; i--) {
    if (cell->possibles.flags[i] != NUM_CELL_CONSTRAINTS)
      continue;

    cell->value = i;

    for (j = 0; j < NUM_CELL_CONSTRAINTS; j++)
      updateConstraint(cell->constraints[j], oldValue, oldNumCells,
                       i, newNumCells);
    oldValue = i;
    oldNumCells = newNumCells;

    if (solve(step + 1))
      return 1;
  }

  // Add cell back to its constraints
  for (i = 0; i < NUM_CELL_CONSTRAINTS; i++) {
    constraint = cell->constraints[i];
    newNumCells = ++(constraint->numCells);
    updateConstraint(constraint, oldValue, oldNumCells,
                     UNASSIGNED_VALUE, newNumCells);

    // Add cell back to cell list after updating constraint so cell's
    // possibles are not changed during the update
    addNode(&(constraint->cellList), minIndex);
  }

  // Unassign value and fail if none of possibilities worked
  cell->value = UNASSIGNED_VALUE;
  return 0;
}

// TODO make inline?
// TODO make instead notifyCellOfNewPossibles?
// TODO allow updating range of numbers
void notifyCellsOfChange(celllist_t* cellList, int value, char markPossible) {
  if (markPossible)
    notifyCellsOfPossible(cellList, value);
  else
    notifyCellsOfImpossible(cellList, value);
}

void notifyCellsOfPossible(celllist_t* cellList, int value) {
  int i;
  for (i = cellList->start; i != END_NODE; i = (cellList->cells[i]).next) {
    cells[i].possibles.flags[value]++;

    if (cells[i].possibles.flags[value] == NUM_CELL_CONSTRAINTS)
      cells[i].possibles.num++;
  }
}

void notifyCellsOfImpossible(celllist_t* cellList, int value) {
  int i;
  for (i = cellList->start; i != END_NODE; i = (cellList->cells[i]).next) {
    if (cells[i].possibles.flags[value] == NUM_CELL_CONSTRAINTS)
      cells[i].possibles.num--;

    cells[i].possibles.flags[value]--;
  }
}


void updateConstraint(constraint_t* constraint, int oldCellValue,
                      int oldNumCells, int newCellValue, int newNumCells) {
  celllist_t* cellList = &(constraint->cellList);
  long value = constraint->value;
  long oldValue = value;

  switch (constraint->type) {
    case LINE:
      updateLinePossibles(cellList, oldCellValue, newCellValue);
      break;

    case PLUS:
      if (oldCellValue != UNASSIGNED_VALUE)
        value += oldCellValue;
      if (newCellValue != UNASSIGNED_VALUE)
        value -= newCellValue;

      constraint->value = value;
      updatePlusPossibles(cellList, oldValue, oldNumCells, value, newNumCells);
      break;

    case MINUS:
      updateMinusPossibles(constraint, value, oldCellValue, oldNumCells,
                           newCellValue, newNumCells);
      break;

    case MULTIPLY:
      if (oldCellValue != UNASSIGNED_VALUE)
        value *= oldCellValue;
      if (newCellValue != UNASSIGNED_VALUE)
        value /= newCellValue;

      constraint->value = value;
      updateMultiplyPossibles(cellList, oldValue, oldNumCells,
                              value, newNumCells);
      break;

    case DIVIDE:
      updateDividePossibles(constraint, value, oldCellValue, oldNumCells,
                            newCellValue, newNumCells);
      break;

    case SINGLE:
      updateSinglePossibles(cellList, value, newNumCells);
      break;
  }
}

// Initializes a row constraint for the given row
void initRowConstraint(constraint_t* constraint, int row) {
  int i;

  constraint->type = LINE;
  constraint->value = -1;
  constraint->numCells = N;

  // Add constraint to its cells
  initList(&(constraint->cellList));
  for (i = 0; i < N; i++) {
    addNode(&(constraint->cellList), GET_CELL(row, i));
    cells[GET_CELL(row, i)].constraints[ROW_CONSTRAINT_INDEX] = constraint;
  }

  initLinePossibles(&constraint->cellList, 1);
}

// Initializes a column constraint for the given column
void initColumnConstraint(constraint_t* constraint, int col) {
  int i;

  constraint->type = LINE;
  constraint->value = -1;
  constraint->numCells = N;

  // Add constraint to its cells
  initList(&(constraint->cellList));
  for (i = 0; i < N; i++) {
    addNode(&(constraint->cellList), GET_CELL(i, col));
    cells[GET_CELL(i, col)].constraints[COLUMN_CONSTRAINT_INDEX] = constraint;
  }

  initLinePossibles(&constraint->cellList, 1);
}

// Initialize possibles for a line constraint
void initLinePossibles(celllist_t* cellList, char markPossible) {
  int i;
  for (i = 1; i <= N; i++)
    notifyCellsOfChange(cellList, i, markPossible);
}

void updateLinePossibles(celllist_t* cellList, int old, int new) {
  if (old != UNASSIGNED_VALUE)
    notifyCellsOfPossible(cellList, old);

  if (new != UNASSIGNED_VALUE)
    notifyCellsOfImpossible(cellList, new);
}

// Initialize possibles for plus a constraint
void initPlusPossibles(celllist_t* cellList, long value, int numCells, char markPossible) {
  int i;
  int start = MAX(1, value - N * (numCells - 1));
  int end = MIN(N, value - (numCells - 1));

  // Possibles = [start, end], everything else impossible
  for (i = start; i <= end; i++)
    notifyCellsOfChange(cellList, i, markPossible);
}

void updatePlusPossibles(celllist_t* cellList, long oldValue,
                         int oldNumCells, long newValue, int newNumCells) {
  int i;
  // Possibles = [start, end], everything else impossible
  int oldStart = MAX(1, oldValue - N * (oldNumCells - 1));
  int oldEnd = MIN(N, oldValue - (oldNumCells - 1));

  int newStart = MAX(1, newValue - N * (newNumCells - 1));
  int newEnd = MIN(N, newValue - (newNumCells - 1));

  // Notify of new impossibles from start/end changes
  for (i = oldStart; i < newStart ; i++)
   notifyCellsOfImpossible(cellList, i);
  for (i = newEnd + 1; i <= oldEnd; i++)
   notifyCellsOfImpossible(cellList, i);

  // Notify of new possibles from start/end changes
  for (i = newStart; i < oldStart; i++)
   notifyCellsOfPossible(cellList, i);
  for (i = oldEnd + 1; i <= newEnd; i++)
   notifyCellsOfPossible(cellList, i);
}

// Initialize possibles for a minus constraint
void initMinusPossibles(celllist_t* cellList, long value, char markPossible) {
  int i;

  // Impossibles = [N - value + 1, value], everything else possible
  for (i = 1; i <= N - value; i++)
    notifyCellsOfChange(cellList, i, markPossible);
  for (i = MAX(N - value + 1, value + 1); i <= N; i++)
    notifyCellsOfChange(cellList, i, markPossible);
}

// Initialize possibles for a partial minus constraint
void initPartialMinusPossibles(celllist_t* cellList, long value,
                               int cellValue, char markPossible) {
  if (cellValue + value <= N)
    notifyCellsOfChange(cellList, cellValue + value, markPossible);

  if (cellValue - value > 0)
    notifyCellsOfChange(cellList, cellValue - value, markPossible);
}

// Don't update possibles when num cells is 0 so undoing is easier
void updateMinusPossibles(constraint_t* constraint, int value, int oldCellValue, int oldNumCells, int newCellValue, int newNumCells) {
  celllist_t* cellList = &(constraint->cellList);

  if (oldNumCells == 2)
    initMinusPossibles(cellList, value, 0);
  else if (oldCellValue != UNASSIGNED_VALUE && oldNumCells == 1)
    initPartialMinusPossibles(cellList, value, oldCellValue, 0);

  if (newNumCells == 2)
    initMinusPossibles(cellList, value, 1);
  else if (newCellValue != UNASSIGNED_VALUE && newNumCells == 1)
    initPartialMinusPossibles(cellList, value, newCellValue, 1);
}

// Initialize possibles for a multiply constraint
void initMultiplyPossibles(celllist_t* cellList, long value, int numCells, char markPossible) {
  int i;

  if (numCells == 0)
    return;

  long maxLeft = maxMultiply[numCells - 1];
  for (i = MAX(1, value / maxLeft); i <= MIN(value, N); i++) {
    if (value % i == 0)
      notifyCellsOfChange(cellList, i, markPossible);
  }
}

void updateMultiplyPossibles(celllist_t* cellList, long oldValue,
                         int oldNumCells, long newValue, int newNumCells) {
/*  int i;
  long oldStart = N + 1, newStart;
  if (numCells > 0)
    oldStart = MAX(1, value / maxLeft);*/

  initMultiplyPossibles(cellList, oldValue, oldNumCells, 0);
  initMultiplyPossibles(cellList, newValue, newNumCells, 1);
}

// Initialize possibles for a divide constraint
void initDividePossibles(celllist_t* cellList, long value, char markPossible) {
  int i;

  // Note that value can not equal 1 since 1,1 is the only answer to 1/
  // and a divide has either 2 cells on the same row or same column
  for (i = 1; i <= N / value; i++) {
    // Ensure don't double count a value
    if (i % value != 0)
      notifyCellsOfChange(cellList, i, markPossible);

    notifyCellsOfChange(cellList, i * value, markPossible);
  }
}

// Initialize possibles for a partial divide constraint
void initPartialDividePossibles(celllist_t* cellList, long value,
                                int cellValue, char markPossible) {
  if (cellValue * value <= N)
    notifyCellsOfChange(cellList, cellValue * value, markPossible);

  if ((cellValue % value == 0) && cellValue >= value)
    notifyCellsOfChange(cellList, cellValue / value, markPossible);
}

// Don't update possibles when num cells is 0 so undoing is easier
void updateDividePossibles(constraint_t* constraint, int value, int oldCellValue, int oldNumCells, int newCellValue, int newNumCells) {
  celllist_t* cellList = &(constraint->cellList);

  if (oldNumCells == 2)
    initDividePossibles(cellList, value, 0);
  else if (oldCellValue != UNASSIGNED_VALUE && oldNumCells == 1)
    initPartialDividePossibles(cellList, value, oldCellValue, 0);

  if (newNumCells == 2)
    initDividePossibles(cellList, value, 1);
  else if (newCellValue != UNASSIGNED_VALUE && newNumCells == 1)
    initPartialDividePossibles(cellList, value, newCellValue, 1);
}

// Initialize possibles for a single constraint
void initSinglePossibles(celllist_t* cellList, long value, int numCells, char markPossible) {
  initPlusPossibles(cellList, value, numCells, markPossible);
}

void updateSinglePossibles(celllist_t* cellList, int value, int numCells) {
  if (numCells == 1)
    notifyCellsOfPossible(cellList, value);
  else
    notifyCellsOfImpossible(cellList, value);
}

// Initialize a cell list
inline void initList(celllist_t* cellList) {
  cellList->start = END_NODE;
}

// Add node to a cell list
inline void addNode(celllist_t* cellList, int node) {
  int start = cellList->start;

  cellList->cells[node].previous = END_NODE;
  cellList->cells[node].next = start;
  cellList->start = node;

  if (start != END_NODE)
    cellList->cells[start].previous = node;
}

// Remove a node from a cell list
inline void removeNode(celllist_t* cellList, int node) {
  int previous = cellList->cells[node].previous;
  int next = cellList->cells[node].next;

  if (next != END_NODE)
    cellList->cells[next].previous = previous;

  if (previous == END_NODE)
    cellList->start = next;
  else
    cellList->cells[previous].next = next;
}

// Print usage information and exit
void usage(char* program) {
  printf("Usage: %s filename\n", program);
  exit(0);
}

void readLine(FILE* in, char* lineBuf) {
  if (!fgets(lineBuf, MAX_LINE_LEN, in))
    unixError("Failed to read line from input file");
}

void appError(const char* str) {
  fprintf(stderr, "%s\n", str);
  exit(1);
}

// Display a unix error and exit
void unixError(const char* str) {
  perror(str);
  exit(1);
}

