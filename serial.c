/**	@file kenken.c
 *
 *	@author Jonathan Park (jjp1) and Ben Parr (bparr)
 */

#include <stdio.h>
#include <stdlib.h>
#include "kenken.h"

int solve(int step);
void usage(char* program);

// Problem grid
cell_t* cells;
// Constraints array
constraint_t* constraints;

int main(int argc, char **argv) {
  if (argc != 2)
    usage(argv[0]);

  initialize(argv[1], &cells, &constraints);

  // Run algorithm
  if (!solve(0))
    appError("No solution found");

  printSolution(cells);
  return 0;
}

// Main recursive function used to solve the program
int solve(int step) {
  int cellIndex;
  int value = UNASSIGNED_VALUE;

  // Success if all cells filled in
  if (step == totalNumCells)
    return 1;

  if ((cellIndex = getNextCellToFill(cells, constraints)) < 0)
    return 0;

  while (UNASSIGNED_VALUE != (value = applyNextValue(cells, constraints,
                                                     cellIndex, value))) {
    if (solve(step + 1))
      return 1;
  }

  return 0;
}

// Print usage information and exit
void usage(char* program) {
  printf("Usage: %s filename\n", program);
  exit(0);
}

