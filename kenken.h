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
#define MAX_PROBLEM_SIZE 25
// Value used to indicate a cell's value is unassigned
#define UNASSIGNED_VALUE 0
// TODO move into .c
// Number of constraints a cell has (1 row constraint, 1 column constraint,
// 1 block constraint)
#define NUM_CELL_CONSTRAINTS 3

// Get cell at (x, y)
#define GET_CELL(x, y) (N * (x) + (y))
// Whether or not a specific cell value is possible
#define IS_POSSIBLE(n) (n == NUM_CELL_CONSTRAINTS)

// Calculate the minimum of two numbers
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
// Calculate the maximum of two numbers
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


// TODO just move into .c?
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
// TODO move into .c
// Max number by multiplying
long* maxMultiply;


// Given an input file name, initialize cells and constraints and global
// variables. Note, this should only be called once at beginning of program.
// The constraints and cells results can be memcpy'ed if need be.
void initialize(char* file, cell_t** cellPtr, constraint_t** constraintsPtr);

// TODO
inline void applyValue(cell_t* cells, constraint_t* constraints, int cellIndex,
                       int value);

// TODO
inline int getNextCellToFill(cell_t* cells, constraint_t* constraints);

// TODO
inline int applyNextValue(cell_t* cells, constraint_t* constraints,
                          int cellIndex, int previousValue);


// Print an application error, and exit
void appError(const char* str);
// Print a unix error, and exit
void unixError(const char* str);

#endif

