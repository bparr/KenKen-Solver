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

typedef enum {
  LINE,
  PLUS,
  MINUS,
  PARTIAL_MINUS,
  MULT,
  DIVID,
  PARTIAL_DIV,
  SINGLE
} type_t;

typedef struct possible {
  char flags[MAX_POSSIBLE_PROBLEM + 1];
  int num;
} possible_t;

// TODO: need head/tail pointers (right now just a head ptr)
// Can switch cellnode_t* cells to point to a struct holding head/tail pointers
typedef struct constraint {
  type_t type;
  long value;
  possible_t possibles;
  char flags[MAX_POSSIBLE_PROBLEM*MAX_POSSIBLE_PROBLEM];
  int numCells;
  struct cellnode* cells;
} constraint_t;

typedef struct cell {
  int value;
  possible_t possibles;
  struct constraint* constraints[CONSTRAINT_NUM];
} cell_t;

typedef struct cellnode {
  cell_t* cell;
  struct cellnode* prev;
  struct cellnode* next;
} cellnode_t;

/** TODO: incorporate for optimizing finding the next possible?
typedef struct step {
  cell_t* modifiedCell;
  int lastIndex;
} step_t;
*/

// Definitions for possible_t functions
void initLinePossible(possible_t* possible);
int cellIsPossible(possible_t* possible, int i);
int constraintIsPossible(possible_t* possible, int i);
void addPossible(possible_t* possible, int i);
int removePossible(possible_t* possible, int i);
int getNumPossible(possible_t* possible);
int getNextPossible(possible_t* possible, int last);
void markImpossible(possible_t* possible, int i);

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

// Declarations for cell_t functions
void removePossibles(cell_t* cell, int value);
void restorePossibles(cell_t* cell, int value);
int cellnode_insert(cellnode_t** head, cellnode_t* newnode);

// Algorithm functions
int solve(int step);
int findNextCell();
void assignValue(cell_t* cell, int value);
void unassignValue(cell_t* cell, int value);
void addCell(constraint_t* constraint, cell_t* cell, int value);
void removeCell(constraint_t* constraint, cell_t* cell, int value);

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
  cellnode_t* newnode;

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

      // Add cell to cells list
      newnode = (cellnode_t*)calloc(sizeof(cellnode_t), 1);
      newnode->cell = &(cells[GET_INDEX(x, y, problemSize)]);
      newnode->prev = NULL;
      newnode->next = NULL;
      cellnode_insert(&constraint->cells, newnode);
      
      // Update cells count
      constraint->numCells++;

      // add constraint to cell
      newnode->cell->constraints[BLOCK] = constraint;

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

      assignValue(cell, newvalue);
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

    if ((numPossible = getNumPossible(&(cell->possibles))) < minPossible) {
      minPossible = numPossible;
      minCell = cell;
      index = i;
    }
  }

  // If minPossible == 0, there exists a cell that can't be filled
  return index;
}

void assignValue(cell_t* cell, int value) {
  int i;

  // Assign value
  cell->value = value;

  // Remove cell from constraints
  for (i = 0; i < CONSTRAINT_NUM; i++)
    removeCell(cell->constraints[i], cell, value);
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

// Returns 1 if the value is valid for constraints.
int constraintIsPossible(possible_t* possible, int i) {
  return (possible->flags[i]);
}

// Increments a value in a possible regardless of state
void addPossible(possible_t* possible, int i) {
  if (possible->flags[i] != NOT_POSSIBLE)
    possible->flags[i]++;
}

// Decrements a value in a possible if it's not impossible
int removePossible(possible_t* possible, int i) {
  if (possible->flags[i]) {
    possible->flags[i]--;
    return 1;
  }
  return 0;
}

// Marks a particular value as impossible
void markImpossible(possible_t* possible, int i) {
  possible->flags[i] = NOT_POSSIBLE;
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
  cellnode_t* newnode;

  constraint->type = LINE;
  constraint->value = -1;
  constraint->numCells = 0;

  initLinePossible(&constraint->possibles);
  for (i = 0; i < problemSize; i++) {
    newnode = (cellnode_t*)calloc(sizeof(cellnode_t), 1);
    newnode->cell = &cells[GET_INDEX(row, i, MAX_POSSIBLE_PROBLEM)];
    newnode->prev = NULL;
    newnode->next = NULL;

    cellnode_insert(&constraint->cells, newnode);
    constraint->numCells++;
  }
}

// Initializes a column constraint for the given column
void initColConstraint(constraint_t* constraint, int col) {
  int i;
  cellnode_t* newnode;

  constraint->type = LINE;
  constraint->value = -1;
  constraint->numCells = 0;

  initLinePossible(&constraint->possibles);
  for (i = 0; i < problemSize; i++) {
    newnode = (cellnode_t*)calloc(sizeof(cellnode_t), 1);
    newnode->cell = &cells[GET_INDEX(i, col, MAX_POSSIBLE_PROBLEM)];
    newnode->prev = NULL;
    newnode->next = NULL;

    cellnode_insert(&constraint->cells, newnode);
    constraint->numCells++;
  }
}

// Removes a possible value from the constraint
int removeConstraintsPossible(constraint_t* constraint, int value) {
  return removePossible(&constraint->possibles, value);
}

// Removes possibilities from the cell's constraints and its own possibles
void removePossibles(cell_t* cell, int value) {
  int i;

  for (i = 0; i < CONSTRAINT_NUM; i++) {
    // If constraint has the possibility, remove + decrement from cell
    if (removeConstraintsPossible(cell->constraints[i], value))
      removePossible(&cell->possibles, value);
  }
}

// Restores possibles from the cell's constraints and its own possibles
void restorePossibles(cell_t* cell, int value) {
  // TODO unimplemented
}

// Inserts a node at the front of the cellnode list
int cellnode_insert(cellnode_t** head, cellnode_t* newnode) {
  cellnode_t* oldhead = *head;

  if (oldhead) {
    newnode->next = oldhead;
    oldhead->prev = newnode;
  }

  *head = newnode;
  return 1;
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
      if (getNumPossible(&constraint->possibles) == 2) {
        int val1 = getNextPossible(&constraint->possibles, 0);
        int val2 = getNextPossible(&constraint->possibles, val1);
        constraint->value = (val1 + val2)/2;
      } else {
        // TODO: ?
      }
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

  // TODO: Ben's compute Possibilities
}

// Removes a cell from a constraint
void removeCell(constraint_t* constraint, cell_t* cell, int value) {
  constraint->numCells--;

  switch (constraint->type) {
    case PLUS:
      constraint->value -= value;
      break;
    case MINUS:
      constraint->type = PARTIAL_MINUS;
      int possible1, possible2;

      // Get possibilities
      possible1 = value - constraint->value;
      possible2 = value + constraint->value;

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

  // TODO: Ben updates constraint's possible values
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
