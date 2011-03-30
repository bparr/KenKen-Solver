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

// Get index into a 2d array initialized as a 1d array
#define GET_INDEX(x, y, size) ((size) * (x) + (y))

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
int problemSize;
// Number of cells in the problem
int numCells;
// Problem grid
cell_t* cells;
// Number of constraints
int numConstraints;
// Constraints array
constraint_t* constraints;

int main(int argc, char **argv)
{
  FILE* in;
  unsigned P = 1;
  int optchar, index, numCages;
  char lineBuf[MAX_LINE_LEN];
  char* ptr;
  int x, y, i;
  constraint_t* constraint;
  cell_t* cell;

  // Check arguments
  if (argc != 2)
    usage(argv[0]);

  // Read in file
  if (!(in = fopen(argv[optind], "r"))) {
    unixError("Failed to open input file");
    exit(1);
  }

  // TODO: Consider using the maximum sizes for all initialization to be
  //       consistent throughout the code

  // Read in problem size
  readLine(in, lineBuf);
  problemSize = atoi(lineBuf);

  numCells = problemSize*problemSize;

  // Initiate problem board
  cells = (cell_t*)calloc(sizeof(cell_t), numCells);
  if (!cells)
    unixError("Failed to allocate memory for the cells");

  numConstraints = 2*problemSize; // problemSize row constraints +
                                  // problemSize col constraints

  // Read in number of constraints
  readLine(in, lineBuf);
  numCages = atoi(lineBuf);
  numConstraints += numCages;

  // TODO: consider order of constraints inserted into the array or
  //       after insertion then sort by size?
  constraints = (constraint_t*)calloc(sizeof(constraint_t), numConstraints);
  if (!constraints)
    unixError("Failed to allocate memory for the constraints");

  // Initialize row constraints
  index = 0;
  for (i = 0; i < problemSize; i++)
    initRowConstraint(&constraints[index++], i);

  // Initialize column constraints
  for (i = 0; i < problemSize; i++)
    initColumnConstraint(&constraints[index++], i);

  // Read in block constraints
  for (i = 0; i < numCages; i++) {
    readLine(in, lineBuf);
    constraint = &constraints[index++];

    ptr = strtok(lineBuf, " ");
    // TODO move down (for Ben's part)
    // Check type
    switch (*ptr) {
      case '+':
        constraint->type = PLUS;
        break;
      case '-':
        constraint->type = MINUS;
        break;
      case 'x':
        constraint->type = MULTIPLY;
        break;
      case '/':
        constraint->type = DIVIDE;
        break;
      case '!':
        constraint->type = SINGLE;
        break;
      default:
        appError("Malformed constraint in input file");
    }

    // Read in value
    ptr = strtok(NULL, " ");
    constraint->value = (long)atoi(ptr);

    ptr = strtok(NULL, ", ");
    // Read in coordinates for cells
    while (ptr) {
      // TODO: error checking
      x = atoi(ptr);
      ptr = strtok(NULL, ", "); // x-value
      y = atoi(ptr);

      // Update cells count
      constraint->numCells++;

      // Add block constraint to cell
      cell = &cells[GET_INDEX(x, y, problemSize)];
      cell->constraints[BLOCK_CONSTRAINT_INDEX] = constraint;

      // Iterate again
      ptr = strtok(NULL, ", ");
    }

    // Adjust possible list for constraint
    // TODO: fix for block constraints
    initLinePossibles(&constraint->possibles);
  }

  // TODO: validation check to see if all cells are accounted for?

  // Run algorithm
  solve(0);

  // TODO check if a solution was actually found (return value of solve)

  for (i = 0; i < numCells; i++)
    printf("%d%c", cells[i].value, ((i + 1) % problemSize != 0) ? ' ' : '\n');

  // Free data
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
    for (newValue = 1; newValue <= problemSize; newValue++) {
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

// Initializes the possible values for a line constraint
void initLinePossibles(possible_t* possibles) {
  int i;
  possibles->num = problemSize;
  for (i = 1; i <= problemSize; i++)
    possibles->flags[i] = POSSIBLE;
}

// Initializes a row constraint for the given row
void initRowConstraint(constraint_t* constraint, int row) {
  int i;
  cell_t* cell;

  constraint->type = LINE;
  constraint->value = -1;
  constraint->numCells = 0;

  initLinePossibles(&constraint->possibles);
  for (i = 0; i < problemSize; i++) {
    // Increment number of cells
    constraint->numCells++;

    // Add row constraint to cell
    cell = &cells[GET_INDEX(row, i, problemSize)];
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
  for (i = 0; i < problemSize; i++) {
    // Increment number of cells
    constraint->numCells++;

    // Add col constraint to cell
    cell = &cells[GET_INDEX(i, col, problemSize)];
    cell->constraints[COLUMN_CONSTRAINT_INDEX] = constraint;
  }
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

