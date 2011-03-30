/**	@file kenken.c
 *
 *	@author Jonathan Park (jjp1) and Ben Parr (bparr)
 */

// TODO fix naming inconsistencies

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#define MAX_PROBLEM_SIZE 25
// TODO make POSSIBLE = 0?
#define POSSIBLE 3 // Value of possible such that it actually is possible
#define UNASSIGNED_VALUE 0
#define CONSTRAINT_NUM 3

#define MAX_LINE_LEN 2048

// TODO remove
#define ROW_CONSTRAINT_INDEX 0
#define COLUMN_CONSTRAINT_INDEX 1
#define BLOCK_CONSTRAINT_INDEX 2

// Get cell at (x, y)
#define GET_CELL(x, y) (N * (x) + (y))

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
  constraint_t* constraints[CONSTRAINT_NUM];
} cell_t;

// Funtions to create constraints
// TODO make like createBlockConstraint
void initRowConstraint(constraint_t* constraint, int row);
void initColumnConstraint(constraint_t* constraint, int col);
constraint_t* createBlockConstraint(char type, long value, int numCells);

// Functions to initialize constraint possibles
// TODO: fill in from Ben
void initLinePossibles(possible_t* possibles);
void initPlusPossibles(possible_t* possibles, long value, int numCells);
void initMinusPossibles(possible_t* possibles, long value, int numCells);
void initMultiplyPossibles(possible_t* possibles, long value, int numCells);
void initDividePossibles(possible_t* possibles, long value, int numCells);
void initSinglePossibles(possible_t* possibles, long value, int numCells);

// Algorithm functions
// TODO merge some of these functions?
int solve(int step);
int findNextCell();
void assignValue(cell_t* cell, int value);
void unassignValue(cell_t* cell, int value);
void addCell(constraint_t* constraint, cell_t* cell, int value);
void removeCell(constraint_t* constraint, cell_t* cell, int value);

// Miscellaneous functions
void usage(char* program);
void readLine(FILE* in, char* lineBuf);
void appError(const char* str);
void unixError(const char* str);

// Problem size
int N;
// Number of cells in the problem
int numCells; // TODO remove? at least rename?
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
  if (!(in = fopen(argv[1], "r"))) {
    unixError("Failed to open input file");
    exit(1);
  }

  // Read in problem size and number of constraints
  readLine(in, lineBuf);
  N = atoi(lineBuf);
  readLine(in, lineBuf);
  // N row constraints + N column constraints + number of block constraints
  numConstraints = 2 * N + atoi(lineBuf);

  // Allocate space for cells and constraints
  numCells = N * N;
  cells = (cell_t*)calloc(sizeof(cell_t), numCells);
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

  // Run algorithm
  if (!solve(0)) {
    appError("No solution found");
  }

  for (i = 0; i < numCells; i++)
    printf("%d%c", cells[i].value, ((i + 1) % N != 0) ? ' ' : '\n');

  // Free allocated memory
  free(cells);
  free(constraints);

  return 0;
}

// Main recursive function used to solve the program
// Note: dangerous for large problem sizes because of the amount of
//       stack space required.
int solve(int step) {
  // Initialize
  int newValue, nextCell, possibleSet;
  cell_t* cell;

  // Find next cell to fill
  nextCell = findNextCell();
  cell = &(cells[nextCell]);

  // If possible values exist
  if (cell->possibles.num > 0) {
    // Get next possible value
    possible_t oldPossible = cell->possibles;
    for (newValue = 1; newValue <= N; newValue++) {
      if (oldPossible.flags[newValue] == POSSIBLE)
        continue;

      assignValue(cell, newValue);
      // Loop to next step
      if (solve(step + 1))
        return 1;
      unassignValue(cell, newValue);

      // Restore step information
      cell->possibles = oldPossible;
    }

  }

  // No possibilities remain
  return 0;
}

/** @brief Finds the next cell to assign. Returns index of the cell
 *
 *  @return index of next cell
 *  @return -1 if there exists a cell that can't be filled
 */
int findNextCell() {
  int i, minPossible = INT_MAX, numPossible = 0, index;
  cell_t* minCell, *cell;

  // Loop and check all unassigned cells
  for (i = 0; i < numCells; i++) {
    if (minPossible == 0)
      break;

    cell = &(cells[i]);

    // Skip assigned cells
    if (cell->value != UNASSIGNED_VALUE)
      continue;

    if ((numPossible = cell->possibles.num) < minPossible) {
      minPossible = numPossible;
      minCell = cell;
      index = i;
    }
  }

  // If minPossible == 0, there exists a cell that can't be filled
  return index;
}

// Assigns a value to a cell
void assignValue(cell_t* cell, int value) {
  int i, ret = 1;

  // Assign value
  cell->value = value;

  // Remove cell from constraints
  for (i = 0; i < CONSTRAINT_NUM; i++)
    removeCell(cell->constraints[i], cell, value);
}

// Unassigns a value to a cell
// TODO mark as impossible
void unassignValue(cell_t* cell, int value) {
  int i;

  // Set value to unassigned
  cell->value = UNASSIGNED_VALUE;

  // Add cell back to its constraints
  for (i = 0; i < CONSTRAINT_NUM; i++)
    addCell(cell->constraints[i], cell, value);
}

// Adds a cell to a constraint
void addCell(constraint_t* constraint, cell_t* cell, int value) {
  int oldValue = constraint->value;

  // Add cell to constraint
  constraint->numCells++;

  switch (constraint->type) {
    case PLUS:
      constraint->value += value;
      break;
    case PARTIAL_MINUS:
      // TODO: Partial minus converge
      break;
    case MULTIPLY:
      constraint->value *= value;
      break;
    case PARTIAL_DIVIDE:
      // TODO: Partial divide converge
      break;
    default:
      break;
  }

  // TODO: Ben's compute Possibilities
}

// Removes a cell from a constraint
void removeCell(constraint_t* constraint, cell_t* cell, int value) {
  // Remove cell from constraint
  constraint->numCells--;

  switch (constraint->type) {
    case PLUS:
      constraint->value -= value;
      break;
    case MINUS:
      constraint->type = PARTIAL_MINUS;
      break;
    case MULTIPLY:
      constraint->value /= value;
      break;
    case DIVIDE:
      constraint->type = PARTIAL_DIVIDE;
      // TODO: div case
      break;
    case SINGLE:
      break;
    default:
      break;
  }


  // TODO: Ben updates constraint's possible values
}

// Initializes a row constraint for the given row
void initRowConstraint(constraint_t* constraint, int row) {
  int i;
  cell_t* cell;

  constraint->type = LINE;
  constraint->value = -1;
  constraint->numCells = 0;

  initLinePossibles(&constraint->possibles);
  for (i = 0; i < N; i++) {
    // Increment number of cells
    constraint->numCells++;

    // Add row constraint to cell
    cell = &cells[GET_CELL(row, i)];
    cells->constraints[ROW_CONSTRAINT_INDEX] = constraint;
  }
}

// Initializes a column constraint for the given column
void initColumnConstraint(constraint_t* constraint, int col) {
  int i;
  cell_t* cell;

  constraint->type = LINE;
  constraint->value = -1;
  constraint->numCells = 0;

  initLinePossibles(&constraint->possibles);
  for (i = 0; i < N; i++) {
    // Increment number of cells
    constraint->numCells++;

    // Add col constraint to cell
    cell = &cells[GET_CELL(i, col)];
    cell->constraints[COLUMN_CONSTRAINT_INDEX] = constraint;
  }
}

// Initialize possibles for a line constraint
void initLinePossibles(possible_t* possibles) {
  printf("Initialize line possibles\n");
}

// Initialize possibles for plus a constraint
void initPlusPossibles(possible_t* possibles, long value, int numCells) {
  printf("Initialize plus possibles: %ld, %d\n", value, numCells);
}

// Initialize possibles for a minus constraint
void initMinusPossibles(possible_t* possibles, long value, int numCells) {
  printf("Initialize minus possibles: %ld, %d\n", value, numCells);
}

// Initialize possibles for a multiply constraint
void initMultiplyPossibles(possible_t* possibles, long value, int numCells) {
  printf("Initialize multiply possibles: %ld, %d\n", value, numCells);
}

// Initialize possibles for a divide constraint
void initDividePossibles(possible_t* possibles, long value, int numCells) {
  printf("Initialize divide possibles: %ld, %d\n", value, numCells);
}

// Initialize possibles for a single constraint
void initSinglePossibles(possible_t* possibles, long value, int numCells) {
  printf("Initialize single possibles: %ld, %d\n", value, numCells);
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

