/** @file kenken.h
 *
 *  @author Jonathan Park (jjp1) and Ben Parr (bparr)
 */

#ifndef __KENKEN_H__
#define __KENKEN_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

// Maximum problem size supported by program
#define MAX_PROBLEM_SIZE 15
// Value used to indicate a cell's value is unassigned
#define UNASSIGNED_VALUE 0
// Value used to indicate a board is in an impossible state
#define IMPOSSIBLE_STATE -1
// Value used to indicate the next cell has too many possibles
#define TOO_MANY_POSSIBLES -2
// Number of constraints a cell has (1 row constraint, 1 column constraint,
// 1 block constraint)
#define NUM_CELL_CONSTRAINTS 3


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
  int constraintIndexes[NUM_CELL_CONSTRAINTS];
} cell_t;


// Problem size
int N;
// Total number of cells in the problem
int totalNumCells;
// Number of constraints
int numConstraints;


// Given an input file name, initialize cells and constraints and global
// variables. Note, this should only be called once at beginning of program.
// The constraints and cells results can be memcpy'ed if need be.
void initialize(char* file, cell_t** cellPtr, constraint_t** constraintsPtr);

// Get number of possibles for a specific cell
inline int getNumPossibles(cell_t* cells, int cellIndex);

// Apply a value to a specific cell, updating its constraints
inline void applyValue(cell_t* cells, constraint_t* constraints, int cellIndex,
                       int value);

// Get the next cell to fill in, remove it from its constraints, and return its
// index. The next cell is unassigned cell with the minimum number of
// possibilities. If puzzle is in impossible state return IMPOSSIBLE_STATE.
inline int getNextCellToFill(cell_t* cells, constraint_t* constraints);

// Same as getNextCellToFill, except it imposes a max number of possibles
// allowed for the chosen cell. If the next cell has too many possibles,
// return TOO_MANY_POSSIBLES.
inline int getNextCellToFillN(cell_t* cells, constraint_t* constraints,
                              int maxPossibles);

// Apply and return next value for the cell currently filling in. On first time
// called for a specific cell, previousValue should be UNASSIGNED_VALUE. When
// there are no more values to fill in, unassign the value, add the cell back
// to its constraints and return UNASSIGNED_VALUE.
inline int applyNextValue(cell_t* cells, constraint_t* constraints,
                          int cellIndex, int previousValue);

// Print solution to stdout
void printSolution(cell_t* cells);


// Print an application error, and exit
void appError(const char* str);
// Print a unix error, and exit
void unixError(const char* str);

#endif

