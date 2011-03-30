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

// Value of possible flag indicating it is valid
#define POSSIBLE 0
// Value of possible flag to set if invalid. Don't use to check for validity,
// since flag values can be incremented and decremented. Instead, do
// (flag == POSSIBLE) to check for validity.
#define IMPOSSIBLE CHAR_MAX
// Value used to indicate a cell's value is unassigned
#define UNASSIGNED_VALUE 0
// Number of constraints a cell has (1 row constraint, 1 column constraint,
// 1 block constraint)
#define NUM_CELL_CONSTRAINTS 3

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
  PARTIAL_MINUS,
  MULTIPLY,
  DIVIDE,
  PARTIAL_DIVIDE,
  SINGLE // TODO remove?? SINGLE should be a degenerate case of PLUS
} type_t;

typedef struct possible {
  char flags[MAX_PROBLEM_SIZE + 1];
  int num;
} possible_t;

typedef struct constraint {
  type_t type;
  long value;
  possible_t possibles;
  int numCells;
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
void initCellPossibles(cell_t* cell);
void initLinePossibles(possible_t* possibles);
void initPlusPossibles(possible_t* possibles, long value, int numCells);
void initMinusPossibles(possible_t* possibles, long value, int numCells);
void initPartialMinusPossibles(possible_t* possibles, long value,
                               int cellValue);
void initMultiplyPossibles(possible_t* possibles, long value, int numCells);
void initDividePossibles(possible_t* possibles, long value, int numCells);
void initPartialDividePossibles(possible_t* possibles, long value,
                                int cellValue);
void initMultiplyPossibles(possible_t* possibles, long value, int numCells);
void initSinglePossibles(possible_t* possibles, long value, int numCells);

// Algorithm functions
int solve(int step);
void updateCellsPossibles(constraint_t* constraint, char* originalFlags,
                          char* newFlags);
void removeCell(constraint_t* constraint, int cellValue);
void addCell(constraint_t* constraint, int cellValue);

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

int main(int argc, char **argv)
{
  FILE* in;
  char type, lineBuf[MAX_LINE_LEN];
  char* ptr;
  int i, x, y, numCells;
  long value;
  constraint_t* constraint;

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

  // Initialize row and column constraints
  for (i = 0; i < N; i++) {
    initRowConstraint(&constraints[i], i);
    initColumnConstraint(&constraints[i + N], i);
  }

  // Initialize block constraints
  for (i = 2 * N; i < numConstraints; i++) {
    readLine(in, lineBuf);
    constraint = &(constraints[i]);

    // Read in type
    ptr = strtok(lineBuf, " ");
    type = *ptr;

    // Read in value
    ptr = strtok(NULL, " ");
    value = atol(ptr);

    // Read in cell coordinates
    numCells = 0;
    while ((ptr = strtok(NULL, ", "))) {
      numCells++;

      x = atoi(ptr);
      ptr = strtok(NULL, ", ");
      y = atoi(ptr);

      // Add block constraint to cell
      cells[GET_CELL(x, y)].constraints[BLOCK_CONSTRAINT_INDEX] = constraint;
    }

    constraint->numCells = numCells;
    constraint->value = value;

    // Initialize constraint's type and possibles
    switch (type) {
      case '+':
        constraint->type = PLUS;
        initPlusPossibles(&(constraint->possibles), value, numCells);
        break;
      case '-':
        constraint->type = MINUS;
        initMinusPossibles(&(constraint->possibles), value, numCells);
        break;
      case 'x':
        constraint->type = MULTIPLY;
        initMultiplyPossibles(&(constraint->possibles), value, numCells);
        break;
      case '/':
        constraint->type = DIVIDE;
        initDividePossibles(&(constraint->possibles), value, numCells);
        break;
      case '!':
        constraint->type = SINGLE;
        initSinglePossibles(&(constraint->possibles), value, numCells);
        break;
      default:
        appError("Malformed constraint in input file");
    }
  }

  // Initialize possibles of cells
  for (i = 0; i < totalNumCells; i++)
    initCellPossibles(&(cells[i]));


  // Run algorithm
  if (!solve(0))
    appError("No solution found");

  // Print solution if one found
  for (i = 0; i < totalNumCells; i++)
    printf("%d%c", cells[i].value, ((i + 1) % N != 0) ? ' ' : '\n');

  // Free allocated memory
  free(cells);
  free(constraints);

  return 0;
}

// Main recursive function used to solve the program
int solve(int step) {
  int i, j, numPossibles;
  int minIndex = 0, minPossibles = INT_MAX;
  cell_t* cell;

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

  // Try all possible values for next cell
  for (i = 1; i <= N; i++) {
    if (cell->possibles.flags[i] != POSSIBLE)
      continue;

    cell->value = i;

    // Remove cell from its constraints
    for (j = 0; j < NUM_CELL_CONSTRAINTS; j++)
      removeCell(cell->constraints[j], i);

    if (solve(step + 1))
      return 1;

    // Add cell back to its constraints
    // TODO optimization: avoid adding cell back just to remove it?
    for (j = 0; j < NUM_CELL_CONSTRAINTS; j++)
      addCell(cell->constraints[j], i);
  }

  // Unassign value and fail if none of possibilities worked
  cell->value = UNASSIGNED_VALUE;
  return 0;
}

// TODO make faster
void updateCellsPossibles(constraint_t* constraint, char* originalFlags,
                          char* newFlags) {

  int i, j, k;
  for (i = 1; i <= N; i++) {
    // Only update possibles that changed
    if (originalFlags[i] == newFlags[i])
      continue;

    // Find unassigned cells who are in the constraint
    for (j = 0; j < totalNumCells; j++) {
      if (cells[j].value != UNASSIGNED_VALUE)
        continue;

      for (k = 0; k < NUM_CELL_CONSTRAINTS; k++) {
        if (cells[j].constraints[k] == constraint)
          break;
      }

      if (k == NUM_CELL_CONSTRAINTS)
        continue;

      // Found a cell
      if (originalFlags[i] == POSSIBLE)
        cells[j].possibles.flags[i]++;
      else
        cells[j].possibles.flags[i]--;
    }
  }
}


// Remove a cell with certain value from a constraint
void removeCell(constraint_t* constraint, int cellValue) {
  int numCells = --(constraint->numCells);
  long value = constraint->value;

  possible_t* possibles = &(constraint->possibles);
  possible_t originalPossibles;
  memcpy(&originalPossibles, possibles, sizeof(possible_t));

  switch (constraint->type) {
    case LINE:
      possibles->flags[cellValue] = IMPOSSIBLE;
      possibles->num--;
      break;

    case PLUS:
      value -= cellValue;
      constraint->value = value;
      initPlusPossibles(possibles, value, numCells);
      break;

    case MINUS:
      constraint->type = PARTIAL_MINUS;
      initPartialMinusPossibles(possibles, value, cellValue);
      break;

    case MULTIPLY:
      value /= cellValue;
      constraint->value = value;
      initPlusPossibles(possibles, value, numCells);
      break;

    case DIVIDE:
      constraint->type = PARTIAL_DIVIDE;
      initPartialDividePossibles(possibles, value, cellValue);
      break;

    case SINGLE:
      value -= cellValue;
      constraint->value = value;
      initSinglePossibles(possibles, constraint->value, numCells);
      break;

    default:
      break;
  }

  updateCellsPossibles(constraint, originalPossibles.flags, possibles->flags);
}

// Add a cell with a specific value to a constraint
void addCell(constraint_t* constraint, int cellValue) {
  int numCells = ++(constraint->numCells);
  long value = constraint->value;

  possible_t* possibles = &(constraint->possibles);
  possible_t originalPossibles;
  memcpy(&originalPossibles, possibles, sizeof(possible_t));


  switch (constraint->type) {
    case LINE:
      possibles->flags[cellValue] = POSSIBLE;
      possibles->num++;
      break;

    case PLUS:
      value += cellValue;
      constraint->value = value;
      initPlusPossibles(possibles, value, numCells);
      break;

    case PARTIAL_MINUS:
      if (numCells == 2) { // TODO macroify
        constraint->type = MINUS;
        initMinusPossibles(possibles, value, numCells);
      }
      break;


    case MULTIPLY:
      value *= cellValue;
      constraint->value = value;
      initPlusPossibles(possibles, value, numCells);
      break;

    case PARTIAL_DIVIDE:
      if (numCells == 2) { // TODO macroify
        constraint->type = DIVIDE;
        initDividePossibles(possibles, value, numCells);
      }
      break;

    case SINGLE:
      value += cellValue;
      constraint->value = value;
      initSinglePossibles(possibles, constraint->value, numCells);
      break;

    default:
      break;
  }

  updateCellsPossibles(constraint, originalPossibles.flags, possibles->flags);
}

// Initializes a row constraint for the given row
void initRowConstraint(constraint_t* constraint, int row) {
  int i;

  constraint->type = LINE;
  constraint->value = -1;
  constraint->numCells = N;
  initLinePossibles(&constraint->possibles);

  // Add constraint to its cells
  for (i = 0; i < N; i++)
    cells[GET_CELL(row, i)].constraints[ROW_CONSTRAINT_INDEX] = constraint;
}

// Initializes a column constraint for the given column
void initColumnConstraint(constraint_t* constraint, int col) {
  int i;

  constraint->type = LINE;
  constraint->value = -1;
  constraint->numCells = N;
  initLinePossibles(&constraint->possibles);

  // Add constraint to its cells
  for (i = 0; i < N; i++)
    cells[GET_CELL(i, col)].constraints[COLUMN_CONSTRAINT_INDEX] = constraint;
}

// Initialize possibles for a cell
void initCellPossibles(cell_t* cell) {
  int i, j, num = 0;

  memset(cell->possibles.flags, IMPOSSIBLE, N + 1);

  for (i = 1; i <= N; i++) {
    for (j = 0; j < NUM_CELL_CONSTRAINTS; j++) {
      if (cell->constraints[j]->possibles.flags[i] != POSSIBLE)
        break;
    }

    // Mark possible if possible in all of its constraints
    if (j == NUM_CELL_CONSTRAINTS) {
      num++;
      cell->possibles.flags[i] = POSSIBLE;
    }
  }

  cell->possibles.num = num;
}

// Initialize possibles for a line constraint
void initLinePossibles(possible_t* possibles) {
  possibles->num = N;
  memset(possibles->flags, POSSIBLE, N + 1);
}

// Initialize possibles for plus a constraint
void initPlusPossibles(possible_t* possibles, long value, int numCells) {
  int i;
  int rangeMin = MAX(1, value - N * (numCells - 1));
  int rangeMax = MIN(N, value - (numCells - 1));

  // Possibles = [rangeMin, rangeMax], everything else impossible
  memset(possibles->flags, IMPOSSIBLE, N + 1);
  for (i = rangeMin; i <= rangeMax; i++)
    possibles->flags[i] = POSSIBLE;

  possibles->num = MAX(0, rangeMax - rangeMin + 1);
}

// Initialize possibles for a minus constraint
void initMinusPossibles(possible_t* possibles, long value, int numCells) {
  int i;

  // Impossibles = [N - value + 1, value], everything else possible
  memset(possibles->flags, POSSIBLE, N + 1);

  for (i = N - value + 1; i <= value; i++)
    possibles->flags[i] = IMPOSSIBLE;

  possibles->num = N - MAX(0, 2 * value - N);
}

// Initialize possibles for a partial minus constraint
void initPartialMinusPossibles(possible_t* possibles, long value,
                               int cellValue) {
  int num = 0;

  memset(possibles, IMPOSSIBLE, N + 1);

  if (cellValue + value <= N) {
    possibles->flags[cellValue + value] = POSSIBLE;
    num++;
  }

  if (cellValue - value > 0) {
    possibles->flags[cellValue - value] = POSSIBLE;
    num++;
  }

  possibles->num = num;
}

// Initialize possibles for a multiply constraint
void initMultiplyPossibles(possible_t* possibles, long value, int numCells) {
  int i, num = 0;

  memset(possibles->flags, IMPOSSIBLE, N + 1);
  for (i = 1; i <= MIN(value, N); i++) {
    if (value % i == 0) {
      possibles->flags[i] = POSSIBLE;
      num++;
    }
  }

  possibles->num = num;
}

// Initialize possibles for a divide constraint
void initDividePossibles(possible_t* possibles, long value, int numCells) {
  int i, num = 0;

  memset(possibles->flags, IMPOSSIBLE, N + 1);
  for (i = 1; i <= N / value; i++) {
    if (value % i == 0) {
      if (possibles->flags[i] != POSSIBLE) {
        possibles->flags[i] = POSSIBLE;
        num += 2;
      }
      else {
        // One part of found possible pair was already found
        num++;
      }

      possibles->flags[i * value] = POSSIBLE;
    }
  }
}

// Initialize possibles for a partial divide constraint
void initPartialDividePossibles(possible_t* possibles, long value,
                               int cellValue) {
  int num = 0;

  memset(possibles, IMPOSSIBLE, N + 1);

  if (cellValue * value <= N) {
    possibles->flags[cellValue * value] = POSSIBLE;
    num++;
  }

  if ((cellValue % value == 0) && cellValue >= value) {
    possibles->flags[cellValue / value] = POSSIBLE;
    num++;
  }

  possibles->num = num;
}

// Initialize possibles for a single constraint
void initSinglePossibles(possible_t* possibles, long value, int numCells) {
  initPlusPossibles(possibles, value, numCells);
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

