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
// Value of node at start and end of cell list
#define END_NODE -1

// Different types of constraints in cell_t constraint array
#define NUM_CELL_CONSTRAINTS 3
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
  SINGLE
} type_t;

typedef struct cellnode {
  int previous;
  int next;
} cellnode_t;

typedef struct celllist {
  int start;
  int size;
  cellnode_t cells[MAX_PROBLEM_SIZE * MAX_PROBLEM_SIZE];
} celllist_t;

typedef struct constraint {
  type_t type;
  long value;
  celllist_t cellList;
} constraint_t;

typedef struct cell {
  int value;
  int numPossibles;
  char possibles[MAX_PROBLEM_SIZE + 1];
  constraint_t* constraints[NUM_CELL_CONSTRAINTS];
} cell_t;

// Algorithm functions
int solve(int step);
inline void updateConstraint(constraint_t* constraint, int oldCellValue,
                            int oldNumCells, int newCellValue, int newNumCells);

// Funtions to initialize line constraints
void initRowConstraint(constraint_t* constraint, int row);
void initColumnConstraint(constraint_t* constraint, int col);

// Functions to initialize cell's possibles in given constraint
void initLineCells(celllist_t* cellList);
void initPlusCells(celllist_t* cellList, long value, int numCells);
void initMinusCells(celllist_t* cellList, long value);
void initMultiplyCells(celllist_t* cellList, long value, int numCells);
void initDivideCells(celllist_t* cellList, long value);
void initSingleCells(celllist_t* cellList, long value);

// Helper functions used when updating cell's possibles
inline void updatePlusCells(celllist_t* cellList, long oldValue,
                            int oldNumCells, long newValue, int newNumCells);
inline void initMinusCellsHelper(celllist_t* cellList, long value,
                                 char markPossible);
inline void initPartialMinusCells(celllist_t* cellList, long value,
                                  int cellValue, char markPossible);
inline void updateMultiplyCells(celllist_t* cellList, long oldValue,
                               int oldNumCells, long newValue, int newNumCells);
inline void initDivideCellsHelper(celllist_t* cellList, long value,
                                  char markPossible);
inline void initPartialDivideCells(celllist_t* cellList, long value,
                                   int cellValue, char markPossible);
inline void notifyCellsOfChange(celllist_t* cellList, int value,
                                char markPossible);
inline void notifyCellsOfChanges(celllist_t* cellList, int start, int end,
                                 char markPossible);

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

int main(int argc, char **argv) {
  FILE* in;
  char type, lineBuf[MAX_LINE_LEN];
  char* ptr;
  int i, x, y;
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
    initList(cellList);
    while ((ptr = strtok(NULL, ", "))) {
      x = atoi(ptr);
      ptr = strtok(NULL, ", ");
      y = atoi(ptr);

      // Add block constraint to cell
      addNode(cellList, GET_CELL(x, y));
      cells[GET_CELL(x, y)].constraints[BLOCK_CONSTRAINT_INDEX] = constraint;
    }

    constraint->value = value;

    // Initialize constraint's type and possibles
    switch (type) {
      case '+':
        constraint->type = PLUS;
        initPlusCells(cellList, value, constraint->cellList.size);
        break;
      case '-':
        constraint->type = MINUS;
        initMinusCells(cellList, value);
        break;
      case 'x':
        constraint->type = MULTIPLY;
        initMultiplyCells(cellList, value, constraint->cellList.size);
        break;
      case '/':
        constraint->type = DIVIDE;
        initDivideCells(cellList, value);
        break;
      case '!':
        constraint->type = SINGLE;
        initSingleCells(cellList, value);
        break;
      default:
        appError("Malformed constraint in input file");
    }
  }

  // Close file
  fclose(in);

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

    numPossibles = cell->numPossibles;

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

    oldNumCells = constraint->cellList.size;
    removeNode(&(constraint->cellList), minIndex);
    newNumCells = constraint->cellList.size;
  }

  // Try all possible values for next cell
  for (i = N; i > 0; i--) {
    if (cell->possibles[i] != NUM_CELL_CONSTRAINTS)
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
    updateConstraint(constraint, oldValue, oldNumCells,
                     UNASSIGNED_VALUE, oldNumCells + 1);

    // Add cell back to cell list after updating constraint so cell's
    // possibles are not changed during the update
    addNode(&(constraint->cellList), minIndex);
  }

  // Unassign value and fail if none of possibilities worked
  cell->value = UNASSIGNED_VALUE;
  return 0;
}

// Update constraint from having a cell with value oldCellValue to having the
// cell assigned newCellValue
inline void updateConstraint(constraint_t* constraint, int oldCellValue,
                           int oldNumCells, int newCellValue, int newNumCells) {
  celllist_t* cellList = &(constraint->cellList);
  long value = constraint->value;
  long oldValue = value;

  switch (constraint->type) {
    case LINE:
      if (oldCellValue != UNASSIGNED_VALUE)
        notifyCellsOfChange(cellList, oldCellValue, 1);

      if (newCellValue != UNASSIGNED_VALUE)
        notifyCellsOfChange(cellList, newCellValue, 0);
      break;

    case PLUS:
      if (oldCellValue != UNASSIGNED_VALUE)
        value += oldCellValue;
      if (newCellValue != UNASSIGNED_VALUE)
        value -= newCellValue;

      constraint->value = value;
      updatePlusCells(cellList, oldValue, oldNumCells, value, newNumCells);
      break;

    case MINUS:
      // Don't update possibles when num cells is 0 so undoing is easier

      if (oldNumCells == 2)
        initMinusCellsHelper(cellList, value, 0);
      else if (oldCellValue != UNASSIGNED_VALUE && oldNumCells == 1)
        initPartialMinusCells(cellList, value, oldCellValue, 0);

      if (newNumCells == 2)
        initMinusCellsHelper(cellList, value, 1);
      else if (newCellValue != UNASSIGNED_VALUE && newNumCells == 1)
        initPartialMinusCells(cellList, value, newCellValue, 1);

      break;

    case MULTIPLY:
      if (oldCellValue != UNASSIGNED_VALUE)
        value *= oldCellValue;
      if (newCellValue != UNASSIGNED_VALUE)
        value /= newCellValue;

      constraint->value = value;
      updateMultiplyCells(cellList, oldValue, oldNumCells,
                              value, newNumCells);
      break;

    case DIVIDE:
      // Don't update possibles when num cells is 0 so undoing is easier

      if (oldNumCells == 2)
        initDivideCellsHelper(cellList, value, 0);
      else if (oldCellValue != UNASSIGNED_VALUE && oldNumCells == 1)
        initPartialDivideCells(cellList, value, oldCellValue, 0);

      if (newNumCells == 2)
        initDivideCellsHelper(cellList, value, 1);
      else if (newCellValue != UNASSIGNED_VALUE && newNumCells == 1)
        initPartialDivideCells(cellList, value, newCellValue, 1);
      break;

    case SINGLE:
      notifyCellsOfChange(cellList, value, (char)(newNumCells == 1));
      break;
  }
}


// Initializes a row constraint for the given row
void initRowConstraint(constraint_t* constraint, int row) {
  int i;

  constraint->type = LINE;
  constraint->value = -1;

  // Add constraint to its cells
  initList(&(constraint->cellList));
  for (i = 0; i < N; i++) {
    addNode(&(constraint->cellList), GET_CELL(row, i));
    cells[GET_CELL(row, i)].constraints[ROW_CONSTRAINT_INDEX] = constraint;
  }

  initLineCells(&constraint->cellList);
}

// Initializes a column constraint for the given column
void initColumnConstraint(constraint_t* constraint, int col) {
  int i;

  constraint->type = LINE;
  constraint->value = -1;

  // Add constraint to its cells
  initList(&(constraint->cellList));
  for (i = 0; i < N; i++) {
    addNode(&(constraint->cellList), GET_CELL(i, col));
    cells[GET_CELL(i, col)].constraints[COLUMN_CONSTRAINT_INDEX] = constraint;
  }

  initLineCells(&constraint->cellList);
}


// Initialize cell's possibles for a line constraint
void initLineCells(celllist_t* cellList) {
  notifyCellsOfChanges(cellList, 1, N, 1);
}

// Initialize cell's possibles for a plus constraint
void initPlusCells(celllist_t* cellList, long value, int numCells) {
  // Possibles = [start, end], everything else impossible
  int start = MAX(1, value - N * (numCells - 1));
  int end = MIN(N, value - (numCells - 1));
  notifyCellsOfChanges(cellList, start, end, 1);
}

// Initialize cell's possibles for a minus constraint
void initMinusCells(celllist_t* cellList, long value) {
  initMinusCellsHelper(cellList, value, 1);
}

// Initialize cell's possibles for a multiply constraint
void initMultiplyCells(celllist_t* cellList, long value, int numCells) {
  int i;

  if (numCells == 0)
    return;

  long maxLeft = maxMultiply[numCells - 1];
  for (i = MAX(1, value / maxLeft); i <= MIN(value, N); i++) {
    if (value % i == 0)
      notifyCellsOfChange(cellList, i, 1);
  }
}

// Initialize cell's possibles for a divide constraint
void initDivideCells(celllist_t* cellList, long value) {
  initDivideCellsHelper(cellList, value, 1);
}

// Initialize cell's possibles for a single constraint
void initSingleCells(celllist_t* cellList, long value) {
  notifyCellsOfChange(cellList, value, 1);
}


// Helper function for updating a plus constraint
inline void updatePlusCells(celllist_t* cellList, long oldValue,
                            int oldNumCells, long newValue, int newNumCells) {
  // Possibles = [start, end], everything else impossible
  int oldStart = MAX(1, oldValue - N * (oldNumCells - 1));
  int oldEnd = MIN(N, oldValue - (oldNumCells - 1));

  int newStart = MAX(1, newValue - N * (newNumCells - 1));
  int newEnd = MIN(N, newValue - (newNumCells - 1));

  // Notify of new impossibles from start/end changes
  notifyCellsOfChanges(cellList, oldStart, newStart - 1, 0);
  notifyCellsOfChanges(cellList, newEnd + 1, oldEnd, 0);

  // Notify of new possibles from start/end changes
  notifyCellsOfChanges(cellList, newStart, oldStart - 1, 1);
  notifyCellsOfChanges(cellList, oldEnd + 1, newEnd, 1);
}

// Helper function for initializing/updating a minus constraint
inline void initMinusCellsHelper(celllist_t* cellList, long value,
                                 char markPossible) {
  // Impossibles = [N - value + 1, value], everything else possible
  notifyCellsOfChanges(cellList, 1, N - value, markPossible);
  notifyCellsOfChanges(cellList, MAX(N - value, value) + 1, N, markPossible);
}

// Helper function for initializing/updating a partial minus constraint
inline void initPartialMinusCells(celllist_t* cellList, long value,
                                  int cellValue, char markPossible) {
  if (cellValue + value <= N)
    notifyCellsOfChange(cellList, cellValue + value, markPossible);

  if (cellValue - value > 0)
    notifyCellsOfChange(cellList, cellValue - value, markPossible);
}

// Helper function for updating a multiply constraint
inline void updateMultiplyCells(celllist_t* cellList, long oldValue,
                              int oldNumCells, long newValue, int newNumCells) {
  int i;
  int oldStart = N + 1;
  int newStart = N + 1;
  int oldEnd = MIN(oldValue, N);
  int newEnd = MIN(newValue, N);
  char oldIsPossible, newIsPossible;

  if (oldNumCells > 0)
    oldStart = MAX(1, oldValue / maxMultiply[oldNumCells - 1]);
  if (newNumCells > 0)
    newStart = MAX(1, newValue / maxMultiply[newNumCells - 1]);

  for (i = MIN(oldStart, newStart); i <= MAX(oldEnd, newEnd); i++) {
    oldIsPossible = (char)(i >= oldStart && i <= oldEnd && oldValue % i == 0);
    newIsPossible = (char)(i >= newStart && i <= newEnd && newValue % i == 0);
    if (oldIsPossible != newIsPossible)
      notifyCellsOfChange(cellList, i, newIsPossible);
  }
}

// Helper function for initializing/updating a divide constraint
inline void initDivideCellsHelper(celllist_t* cellList, long value,
                                  char markPossible) {
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

// Helper function for initializing/updating a partial divide constraint
inline void initPartialDivideCells(celllist_t* cellList, long value,
                                   int cellValue, char markPossible) {
  if (cellValue * value <= N)
    notifyCellsOfChange(cellList, cellValue * value, markPossible);

  if ((cellValue % value == 0) && cellValue >= value)
    notifyCellsOfChange(cellList, cellValue / value, markPossible);
}

// Notify cells that a possible value has changed state
inline void notifyCellsOfChange(celllist_t* cellList, int value,
                                char markPossible) {
  int i;
  char flag;

  for (i = cellList->start; i != END_NODE; i = (cellList->cells[i]).next) {
    flag = cells[i].possibles[value];
    if (markPossible && flag == NUM_CELL_CONSTRAINTS - 1)
      cells[i].numPossibles++;
    else if (!markPossible && flag == NUM_CELL_CONSTRAINTS)
      cells[i].numPossibles--;

    cells[i].possibles[value] = flag + (char)(markPossible ? 1 : -1);
  }
}

// Notify cells that a range of possible values have changed state
inline void notifyCellsOfChanges(celllist_t* cellList, int start, int end,
                                 char markPossible) {
  int i, j, flag, num;

  if (end < start)
    return;

  for (i = cellList->start; i != END_NODE; i = (cellList->cells[i]).next) {
    num = cells[i].numPossibles;

    for (j = start; j <= end; j++) {
      flag = cells[i].possibles[j];
      if (markPossible && flag == NUM_CELL_CONSTRAINTS - 1)
        num++;
      if (!markPossible && flag == NUM_CELL_CONSTRAINTS)
        num--;

      cells[i].possibles[j] = flag + (char)(markPossible ? 1 : -1);
    }

    cells[i].numPossibles = num;
  }
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

  ++(cellList->size);
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

  --(cellList->size);
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

