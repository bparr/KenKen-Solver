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
  int i;

  // Check arguments
  if (argc != 2)
    usage(argv[0]);

  initialize(argv[1], &cells, &constraints);

  // Run algorithm
  if (!solve(0))
    appError("No solution found");

  // Print solution if one found
  for (i = 0; i < totalNumCells; i++)
    printf("%d%c", cells[i].value, ((i + 1) % N != 0) ? ' ' : '\n');

  return 0;
}

// Main recursive function used to solve the program
int solve(int step) {
  int i, j, nextCellIndex, oldValue = UNASSIGNED_VALUE;
  constraint_t* constraint;
  cell_t* cell;

  // Success if all cells filled in
  if (step == totalNumCells)
    return 1;

  nextCellIndex = findNextCell(cells);
  if (nextCellIndex < 0)
    return 0;

  // Use the found cell as the next cell to fill
  cell = &(cells[nextCellIndex]);

  // Remove cell from its constraints
  for (i = 0; i < NUM_CELL_CONSTRAINTS; i++) {
    constraint = &(constraints[cell->constraintIndexes[i]]);
    removeCellFromConstraint(constraint, nextCellIndex);
  }

  // Try all possible values for next cell
  for (i = N; i > 0; i--) {
    if (!(IS_POSSIBLE(cell->possibles[i])))
      continue;

    cell->value = i;

    for (j = 0; j < NUM_CELL_CONSTRAINTS; j++) {
      constraint = &(constraints[cell->constraintIndexes[j]]);
      updateConstraint(cells, constraint, oldValue, i);
    }
    oldValue = i;

    if (solve(step + 1))
      return 1;
  }

  // Add cell back to its constraints
  for (i = 0; i < NUM_CELL_CONSTRAINTS; i++) {
    constraint = &(constraints[cell->constraintIndexes[i]]);
    updateConstraint(cells, constraint, oldValue, UNASSIGNED_VALUE);

    // Add cell back to cell list after updating constraint so cell's
    // possibles are not changed during the update
    addCellToConstraint(constraint, nextCellIndex);
  }

  // Unassign value and fail if none of possibilities worked
  cell->value = UNASSIGNED_VALUE;
  return 0;
}

// Print usage information and exit
void usage(char* program) {
  printf("Usage: %s filename\n", program);
  exit(0);
}

