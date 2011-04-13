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

// Value used to indicate a cell's value is unassigned
#define UNASSIGNED_VALUE 0

typedef struct cell cell_t;
typedef struct constraint constraint_t;

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

// Apply a value to a specific cell, updating its constraints
inline void applyValue(cell_t* cells, constraint_t* constraints, int cellIndex,
                       int value);

// Get the next cell to fill in, remove it from its constraints, and return its
// index. The next cell is unassigned cell with the minimum number of
// possibilities. If puzzle is in impossible state return -1.
inline int getNextCellToFill(cell_t* cells, constraint_t* constraints);

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

