/**	@file kenken.c
 *
 *	@author Jonathan Park (jjp1) and Ben Parr (bparr)
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#define MAX_POSSIBLE_PROBLEM 25
#define POSSIBLE 3 // Value of possible such that it actually is possible
#define NOT_POSSIBLE -1
#define UNASSIGNED_VALUE 0
#define CONSTRAINT_NUM 3

#define MAX_LINE_LEN 2048

#define ROW 0
#define COLUMN 1
#define BLOCK 2

// Get index into a 2d array initialized as a 1d array
#define GET_INDEX(x, y, size) ((size) * (x) + (y))

// Get index into a 2d array given addresses
#define GET_ARRAY_INDEX(head, elem, elemSize) (((elem) - (head))/(elemSize))

typedef enum {
  LINE,
  PLUS,
  MINUS,
  PARTIAL_MINUS,
  MULT,
  DIVID,
  PARTIAL_DIV,
  SINGLE // SINGLE should be a degenerate case of PLUS
} type_t;

typedef struct possible {
  char flags[MAX_POSSIBLE_PROBLEM + 1];
  int num;
} possible_t;

// TODO: need head/tail pointers (right now just a head ptr)
typedef struct constraint {
  type_t type;
  long value;
  possible_t possibles;
  char flags[MAX_POSSIBLE_PROBLEM*MAX_POSSIBLE_PROBLEM];
  int numCells;
} constraint_t;

typedef struct cell {
  int value;
  possible_t possibles;
  struct constraint* constraints[CONSTRAINT_NUM];
} cell_t;

// Definitions for possible_t functions
void initLinePossible(possible_t* possible);
int cellIsPossible(possible_t* possible, int i);
int getNumPossible(possible_t* possible);
int getNextPossible(possible_t* possible, int last);

// TODO: fill in from Ben
void initializeAddPossibles();
void initializeMinusPossibles();
void initializeDividePossibles();
void initializeMultPossibles();
void initializeLinePossibles();
void initializeSinglePossibles();

// Definitions for constraint_t functions
int initializeConstraint(constraint_t* constraint, type_t type, int value);

void initRowConstraint(constraint_t* constraint, int row);
void initColConstraint(constraint_t* constraint, int col);

// Algorithm functions
int solve(int step);
int findNextCell();
int assignValue(cell_t* cell, int value);
void unassignValue(cell_t* cell, int value);
void addCell(constraint_t* constraint, cell_t* cell, int value);
int removeCell(constraint_t* constraint, cell_t* cell, int value);

// Miscellaneous functions
void usage(char* program);
void readLine(FILE* in, char* lineBuf);
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
// Steps array: stores indices to the cell that was modified
int* steps;

int main(int argc, char **argv)
{
  FILE* in;
  unsigned P = 1;
  int optchar, index, numCages;
  char lineBuf[MAX_LINE_LEN];
  char* ptr, *coordinate;
  int x, y, i;
  constraint_t* constraint;

  // Check arguments
  if (argc != 0)
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
    initColConstraint(&constraints[index++], i);

  // Read in block constraints
  for (i = 0; i < numCages; i++) {
    readLine(in, lineBuf);
    constraint = &constraints[index++];

    ptr = strtok(lineBuf, " ");
    // Check type
    switch (*ptr) {
      case '+':
        constraint->type = PLUS;
        break;
      case '-':
        constraint->type = MINUS;
        break;
      case 'x':
        constraint->type = MULT;
        break;
      case '/':
        constraint->type = DIVID;
        break;
      case '!':
        constraint->type = SINGLE;
        break;
      default:
        // TODO: error?
        constraint->type = SINGLE;
        break;
    }

    // Read in value
    ptr = strtok(NULL, " ");
    constraint->value = (long)atoi(ptr);

    ptr = strtok(NULL, " ");
    // Read in coordinates for cells
    while (ptr) {
      // TODO: error checking
      coordinate = strtok(ptr, ","); // x-value
      x = atoi(coordinate);
      coordinate = strtok(NULL, ",");
      y = atoi(coordinate);

      // Mark as present
      constraint->flags[GET_INDEX(x, y, MAX_POSSIBLE_PROBLEM)] = 1;

      // Update cells count
      constraint->numCells++;

      // Add block constraint to cell
      cells[GET_INDEX(x, y, problemSize)].constraints[BLOCK] = constraint;

      // Iterate across the line
      ptr = strtok(NULL, " ");
    }

    // Adjust possible list for constraint
    // TODO: fix for block constraints
    initLinePossible(&constraint->possibles);
  }

  // TODO: validation check to see if all cells are accounted for?

  // Run algorithm
  solve(0);

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
  int newvalue, nextCell, lastvalue = 0, possibleSet;
  cell_t* cell;
  
  // Find next cell to fill
  nextCell = findNextCell();
  cell = &(cells[nextCell]);
  
  // If possible values exist
  if (getNumPossible(&cell->possibles) > 0) {
    // Get next possible value
    possible_t oldpossible = cell->possibles;
    while ((newvalue = getNextPossible(&cell->possibles, lastvalue)) > 0) {
      // Store step information
      lastvalue = newvalue;

      if (assignValue(cell, newvalue))
        // Loop to next step
        if (solve(step + 1))
          return 1;
      unassignValue(cell, newvalue);

      // Restore step information
      cell->possibles = oldpossible;
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
    if (minPossible == 0) break;
    
    cell = &(cells[i]);

    // Skip assigned cells
    if (cell->value != UNASSIGNED_VALUE)
      continue;

    if ((numPossible = getNumPossible(&cell->possibles)) < minPossible) {
      minPossible = numPossible;
      minCell = cell;
      index = i;
    }
  }

  // If minPossible == 0, there exists a cell that can't be filled
  return index;
}

int assignValue(cell_t* cell, int value) {
  int i, ret = 1;

  // Assign value
  cell->value = value;

  // Remove cell from constraints
  for (i = 0; i < CONSTRAINT_NUM; i++)
    ret &= removeCell(cell->constraints[i], cell, value);

  return ret;
}

void unassignValue(cell_t* cell, int value) {
  int i;

  // Set value to unassigned
  cell->value = UNASSIGNED_VALUE;

  // Add cell back to its constraints
  for (i = 0; i < CONSTRAINT_NUM; i++)
    addCell(cell->constraints[i], cell, value);
}

void initLinePossible(possible_t* possible) {
  int i;
  possible->num = problemSize;
  for (i = 1; i <= problemSize; i++)
    possible->flags[i] = POSSIBLE;
}

// Returns 1 if the value is valid for cells
int cellIsPossible(possible_t* possible, int i) {
  return (possible->flags[i] == POSSIBLE);
}

// Returns the number of possible values in a possible_t
int getNumPossible(possible_t* possible) {
  return possible->num;
}

// Gets the next possible new value
int getNextPossible(possible_t* possible, int last) {
  int i;

  for (i = last + 1; i < MAX_POSSIBLE_PROBLEM + 1; i++)
    if (cellIsPossible(possible, i))
      return i;

  return -1;
}

// Initializes a row constraint for the given row
void initRowConstraint(constraint_t* constraint, int row) {
  int i;
  
  constraint->type = LINE;
  constraint->value = -1;
  constraint->numCells = 0;

  initLinePossible(&constraint->possibles);
  for (i = 0; i < problemSize; i++) {
    // Mark as present
    constraint->flags[GET_INDEX(row, i, MAX_POSSIBLE_PROBLEM)] = 1;
    constraint->numCells++;

    // Add row constraint to cell
    cells[GET_INDEX(row, i, problemSize)].constraints[ROW] = constraint;
  }
}

// Initializes a column constraint for the given column
void initColConstraint(constraint_t* constraint, int col) {
  int i;

  constraint->type = LINE;
  constraint->value = -1;
  constraint->numCells = 0;

  initLinePossible(&constraint->possibles);
  for (i = 0; i < problemSize; i++) {
    // Mark as present
    constraint->flags[GET_INDEX(i, col, MAX_POSSIBLE_PROBLEM)] = 1;
    constraint->numCells++;

    // Add col constraint to cell
    cells[GET_INDEX(i, col, problemSize)].constraints[COLUMN] = constraint;
  }
}

// Adds a cell to a constraint
void addCell(constraint_t* constraint, cell_t* cell, int value) {
  int oldValue = constraint->value;

  constraint->numCells++;

  switch (constraint->type) {
    case PLUS:
      constraint->value += value;
      break;
    case PARTIAL_MINUS:
      // TODO: Partial minus converge
      break;
    case MULT:
      constraint->value *= value;
      break;
    case PARTIAL_DIV:
      // TODO: Partial divide converge
      break;
    default:
      break;
  }
  
  // Add cell to constraint
  int index = GET_ARRAY_INDEX(cells, cell, sizeof(cell_t));
  constraint->flags[index] = 1;
  constraint->numCells++;

  // TODO: Ben's compute Possibilities
}

// Removes a cell from a constraint
int removeCell(constraint_t* constraint, cell_t* cell, int value) {
  constraint->numCells--;

  switch (constraint->type) {
    case PLUS:
      constraint->value -= value;
      break;
    case MINUS:
      constraint->type = PARTIAL_MINUS;
      break;
    case MULT:
      constraint->value /= value;
      break;
    case DIVID:
      constraint->type = PARTIAL_DIV;
      // TODO: div case
      break;
    case SINGLE:
      break;
    default:
      break;
  }

  // Remove cell from constraint
  int index = GET_ARRAY_INDEX(cells, cell, sizeof(cell_t));
  constraint->flags[index] = 0;
  constraint->numCells--;

  if (constraint->numCells == 0) {
    if (constraint->type != LINE) {
      // Check if value = 0
      if (constraint->value != 0) {
        // Failure
        return 0;
      }
    }
  }

  // TODO: Ben updates constraint's possible values
  return 1;
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

// Display a unix error and exit
void unixError(const char* str) {
  perror(str);
  exit(1);
}
