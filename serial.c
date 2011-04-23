// ============================================================================
// Jonathan Park (jjp1)
// Ben Parr (bparr)
//
// File: kenken.c
// Description: Serial implementation of a KenKen puzzle solver.
//
// CS418 Project
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "kenken.h"

// Calculate number of milliseconds between two timevals
#define TIME_DIFF(b, a) (((b).tv_sec - (a).tv_sec) * 1000.0 + \
                         ((b).tv_usec - (a).tv_usec) / 1000.0)

int solve(int step);
void usage(char* program);

// Problem grid
cell_t* cells;
// Constraints array
constraint_t* constraints;
// Number of nodes visited
long long nodeCount;

int main(int argc, char **argv) {
  struct timeval startTime, endTime;
  struct timeval compStartTime;
  double totalTime, compTime;

  if (argc != 2)
    usage(argv[0]);

  // Record start of total time
  gettimeofday(&startTime, NULL);

  initialize(argv[1], &cells, &constraints);
  nodeCount = 0;

  //Record start of Computation time
  gettimeofday(&compStartTime, NULL);
  
  // Run algorithm
  if (!solve(0))
    appError("No solution found");

  gettimeofday(&endTime, NULL);
  printSolution(cells);
  printf("Nodes Visited: %lld\n", nodeCount);

  compTime = TIME_DIFF(endTime, compStartTime);
  totalTime = TIME_DIFF(endTime, startTime);

  // Print out calculated times
  printf("Computation Time = %.3f millisecs\n", compTime);
  printf("      Total Time = %.3f millisecs\n", totalTime);

  return 0;
}

// Main recursive function used to solve the program
int solve(int step) {
  int cellIndex;
  int value = UNASSIGNED_VALUE;

  // Success if all cells filled in
  if (step == totalNumCells)
    return 1;

  nodeCount++;
  // Find the next cell to fill and test all possible values
  if ((cellIndex = getNextCellToFill(cells, constraints)) == IMPOSSIBLE_STATE)
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

