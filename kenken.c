/**	@file kenken.c
 *
 *	@author Jonathan Park (jjp1) and Ben Parr (bparr)
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#define MAX_PROBLEM_SIZE 25
// TODO make POSSIBLE = 0?
#define POSSIBLE 3 // Value of possible such that it actually is possible
#define UNASSIGNED_VALUE 0
#define NUM_CELL_CONSTRAINTS 3

#define MAX_LINE_LEN 2048

// Indexes of different types of constraints in cell_t constraint array
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
  constraint_t* constraints[NUM_CELL_CONSTRAINTS];
} cell_t;

// Funtions to initialize line constraints
void initRowConstraint(constraint_t* constraint, int row);
void initColumnConstraint(constraint_t* constraint, int col);

// Functions to initialize constraint possibles
void initLinePossibles(possible_t* possibles);
void initPlusPossibles(possible_t* possibles, long value, int numCells);
void initMinusPossibles(possible_t* possibles, long value, int numCells);
void initMultiplyPossibles(possible_t* possibles, long value, int numCells);
void initDividePossibles(possible_t* possibles, long value, int numCells);
void initSinglePossibles(possible_t* possibles, long value, int numCells);

// Algorithm functions
int solve(int step);
void removeCell(constraint_t* constraint, int value);
void addCell(constraint_t* constraint, int value);

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
      removeCell(cell->constraints[i], i);

    if (solve(step + 1))
      return 1;

    // Add cell back to its constraints
    // TODO optimization: avoid adding cell back just to remove it?
    for (j = 0; j < NUM_CELL_CONSTRAINTS; j++)
      addCell(cell->constraints[i], i);
  }

  // Unassign value and fail if none of possibilities worked
  cell->value = UNASSIGNED_VALUE;
  return 0;
}

// Remove a cell with certain value from a constraint
void removeCell(constraint_t* constraint, int value) {
  constraint->numCells--;

  // TODO implement
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
      break;
    case SINGLE:
      break;
    default:
      break;
  }
}

// Add a cell with a specific value to a constraint
void addCell(constraint_t* constraint, int value) {
  constraint->numCells++;

  // TODO implement
  switch (constraint->type) {
    case PLUS:
      constraint->value += value;
      break;
    case PARTIAL_MINUS:
      break;
    case MULTIPLY:
      constraint->value *= value;
      break;
    case PARTIAL_DIVIDE:
      break;
    default:
      break;
  }
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

