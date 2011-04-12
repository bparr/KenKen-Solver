/** @file kenken.c
 *
 *  @brief Parallel implementation of a KenKen puzzle solver.
 *  
 *  Basic idea: Dedicate one processor for finding jobs and have the other
 *  processors work on the jobs as they come in. Jobs consist of setting
 *  n*n/2 squares (half the squares).
 *
 *  @author Jonathan Park (jjp1) and Ben Parr (bparr)
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <omp.h>

// Maximum problem size supported by program
#define MAX_PROBLEM_SIZE 25
// Maximum line length of input file
#define MAX_LINE_LEN 2048
// Number of processors
#define P 32

// Value of possible flag indicating it is valid
#define POSSIBLE 0
// Value of possible flag to set if invalid. Don't use to check for validity,
// since flag values can be incremented and decremented. Instead, do
// (flag == POSSIBLE) to check for validity.
#define IMPOSSIBLE CHAR_MAX
// Value used to indicate a cell's value is unassigned
#define UNASSIGNED_VALUE 0
// Number of constraints a cell has (1 row constraint, 1 column constraint,
// 1 block constraint)
#define NUM_CELL_CONSTRAINTS 3
// Value of node at start and end of cell list
#define END_NODE -1

// Indexes of different types of constraints in cell_t constraint array
#define ROW_CONSTRAINT_INDEX 0
#define COLUMN_CONSTRAINT_INDEX 1
#define BLOCK_CONSTRAINT_INDEX 2

// Get cell at (x, y)
#define GET_CELL(x, y) (N * (x) + (y))
// Calculate the minimum of two numbers
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
// Calculate the maximum of two numbers
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
// Increment in the job array
#define INCREMENT(i) (((i) + 1) % (maxJobs))
// Index into jobs array
#define GET_JOB(i) ((i)*(jobLength))

typedef enum {
  LINE,
  PLUS,
  MINUS,
  MULTIPLY,
  DIVIDE,
  SINGLE // TODO remove?? SINGLE should be a degenerate case of PLUS
} type_t;

typedef struct possible {
  char flags[MAX_PROBLEM_SIZE + 1];
  int num;
} possible_t;

typedef struct cellnode {
  int previous;
  int next;
} cellnode_t;

typedef struct celllist {
  int start;
  cellnode_t cells[MAX_PROBLEM_SIZE * MAX_PROBLEM_SIZE];
} celllist_t;

typedef struct constraint {
  type_t type;
  long value;
  possible_t possibles;
  int numCells;
  celllist_t cellList;
} constraint_t;

typedef struct cell {
  int value;
  possible_t possibles;
  constraint_t* constraints[NUM_CELL_CONSTRAINTS];
} cell_t;

typedef struct job {
  int cellIndex;
  int value;
} job_t;

// Algorithm functions
int runParallel();
int fillJobs(int step);
int solve(int step);
void removeCellFromConstraints(cell_t* cell, int cellIndex);
void applyValue(cell_t* cell, int* oldValue, int newValue);
void restoreCellToConstraints(cell_t* cell, int cellIndex, int oldValue);

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
// Max number by multiplying
long* maxMultiply;
// Max number of jobs
int maxJobs;
// Number of cells set per job
int jobLength;
// Job queue
job_t* jobs;
// Job queue head
int queueHead;
// Job queue tail
int queueTail;
// 1 if solution is found, 0 otherwise
int found;
// Current job
job_t* myJob;

// Local cells
cell_t* myCells;
// Local constraints
constraint_t* myConstraints;

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
    
  constraints = NULL;
  cells = NULL;

  // Initialize global variables and data-structures.
  initialize(argv[1], &constraints, &cells);

  // Initialize job queue (implemented as a circular array)
  // Always keeps one block in the array empty. This is necessary because of
  // the concurrent access. Readers modify end, writer modifies start.
  // Inherently thread-safe as long as reader's access is controlled.
  jobLength = N*N/2; // Half the cells filled in
  maxJobs = N*N;

  jobs = (job_t*)calloc(sizeof(job_t), jobLength * maxJobs);
  if (!jobs)
    unixError("Failed to allocated memory for the job queue");

  // Initialize indices for the queue
  queueHead = 0;
  queueTail = 0;

  // Run algorithm
  if (!runParallel())
    appError("No solution found");

  // Print solution if one found
  for (i = 0; i < totalNumCells; i++)
    printf("%d%c", cells[i].value, ((i + 1) % N != 0) ? ' ' : '\n');

  // Free allocated memory
  free(cells);
  free(constraints);
  free(maxMultiply);

  return 0;
}

// Sets up and runs the parallel kenken solver
int runParallel() {
  int step;
  job_t* jobStep;
  cell_t* cell;
  int cellIndex, oldValue;

  // Begin parallel
  omp_set_num_threads(P);

  found = 0;

  // Run algorithm
#pragma omp parallel private(myConstraints, myCells, myJob, jobStep, \
                            cellIndex, cell, oldValue, step)
{
  // Initialize our local copies of the data-structures
  myConstraints = (constraint_t*)calloc(sizeof(constraint_t), numConstraints);
  if (!myConstraints)
    unixError("Failed to allocate memory for myConstraints");

  myCells = (cell_t*)calloc(sizeof(cell_t), totalNumCells);
  if (!myCells)
    unixError("Failed to allocate memory for myCells");

  myJob = (job_t*)calloc(sizeof(job_t), jobLength);
  if (!myJob)
    unixError("Failed to allocate memory for myJob");

  // Copy the data-structures into our own local copies
  memcpy(myConstraints, constraints, numConstraints * sizeof(constraint_t));
  memcpy(myCells, cells, totalNumCells * sizeof(cell_t));

  // Begin finding jobs and running algorithm
  #pragma omp single
  {
    fillJobs(0);
    // After filling jobs, thread should go and help compute remaining jobs
  }

  // All other threads loop
  while (!found) {
    bzero(myJob, sizeof(job_t) * jobLength); // Zero out job

    // Get a job from the queue
    #pragma omp critical
    {
      // Will hog critical section but this is ok because no processor
      // should continue if there are no jobs available. All should wait
      while (queueHead == queueTail);

      // Copy over job
      memcpy(myJob, &jobs[GET_JOB(queueHead)], sizeof(job_t) * jobLength);
      queueHead = INCREMENT(queueHead);
    }

    // Apply job: Note that cellIndices align properly with system state
    // of cell/constraint modifications because the method for achieving their
    // values are similar to solving.
    for (step = 0; step < jobLength; step++) {
      oldValue = UNASSIGNED_VALUE;
      jobStep = &myJob[step];

      // Remove cell from Constraint
      cellIndex = jobStep->cellIndex;
      cell = &myCells[cellIndex];
      removeCellFromConstraints(cell, cellIndex);

      // Set value
      applyValue(cell, &oldValue, jobStep->value);
    }

    // Begin computation
    if (solve(step)) {
      // Set found
      found = 1;

      // Copy answer to global cells
      #pragma omp single
      {
        // Locked in critical just in case multiple threads found
        // a solution at the same time. (Note: this is only time data is
        // written to the global cells array)
        memcpy(cells, myCells, totalNumCells * sizeof(cell_t));
      }
      break;
    }

    // Restore cells to original state
    // TODO: optimize this portion using a diff between current job
    //       and new job due to job locality?
    for (step = 0; step < jobLength; step++) {
      jobStep = &myJob[step];
      cellIndex = jobStep->cellIndex;
      cell = &myCells[cellIndex];

      // Restore
      restoreCellToConstraints(cell, cellIndex, jobStep->value);
      cell->value = UNASSIGNED_VALUE;
    }

  } // While closing brace
  
  // Deallocate resources
  free(myJob);
  free(myConstraints);
  free(myCells);
}

  return found;
}

// Fills the job array. Returns 1 to end the filling, 0 to continue
int fillJobs(int step) {
  int i, numPossibles, oldValue = UNASSIGNED_VALUE;
  int minIndex = 0, minPossibles = INT_MAX;
  cell_t* cell;
  
  int nextCellIndex;
  
  if (found)
    return 1;

  if (step == jobLength) {
    // Loop while buffer is full and no solution is found
    while (!found && INCREMENT(queueTail) == queueHead);

    // If we exited while because solution was found, exit
    if (found)
      return 1;

    // Spot in the buffer freed up so set value as current job
    memcpy(&jobs[GET_JOB(queueTail)], myJob, sizeof(job_t) * jobLength);
    queueTail = INCREMENT(queueTail);
    printf("job inserted into the queue");
    return 0;
  }

  // Success if all cells filled in
  if (step == totalNumCells)
    return 1;

  int nextCellIndex = findNextCell(cells);

  if (nextCellIndex < 0)
    return 0;

  // Use the found cell as the next cell to fill
  cell = &(cells[nextCellIndex]);
  
  // Remove cell from its constraints
  removeCellFromConstraints(cell, nextCellIndex);

  // Try all possible values for next cell
  for (i = N; i > 0; i--) {
    if (!IS_POSSIBLE(cell->possibles[i]))
      continue;

    // Store job step values
    myJob[step].cellIndex = nextCellIndex;
    myJob[step].value = i;
    
    // Set job values
    applyValue(cell, &oldValue, i);
    
    if (fillJobs(step + 1)) 
      return 1;
  }
  
  // Restore cell to its constraints
  restoreCellToConstraints(cell, nextCellIndex, oldValue);

  // Reset job step values
  myJob[step].cellIndex = -1;
  myJob[step].value = UNASSIGNED_VALUE;

  // Unassign value and fail if none of possibilities worked
  cell->value = UNASSIGNED_VALUE;
  return 0;
}

// Main recursive function used to solve the program
int solve(int step) {
  int i, j, nextCellIndex, oldValue = UNASSIGNED_VALUE;
  cell_t* cell;
  constraint_t* constraint;

  // Success if all cells filled in. Or if another processor found a solution
  if (step == totalNumCells || found == 1)
    return 1;

  int nextCellIndex = findNextCell(cells);

  if (nextCellIndex < 0)
    return 0;

  // Use the found cell as the next cell to fill
  cell = &(cells[nextCellIndex]);
  
  removeCellFromConstraints(cell, nextCellIndex);

  // Try all possible values for next cell
  for (i = N; i > 0; i--) {
    if (!IS_POSSIBLE(cell->possibles[i]))
      continue;

    applyValue(cell, &oldValue, i);
    
    if (solve(step + 1)) 
      return 1;
  }
  
  restoreCellToConstraints(cell, nextCellIndex, oldValue);

  // Unassign value and fail if none of possibilities worked
  cell->value = UNASSIGNED_VALUE;
  return 0;
}

// Remove a cell from its constraints
void removeCellFromConstraints(cell_t* cell, int cellIndex) {
  int i
  
  // Remove cell from its constraints
  for (i = 0; i < NUM_CELL_CONSTRAINTS; i++)
    removeCellFromConstraint(myConstraints[cell->constraints[i]], cellIndex);
}

// Apply a value to a cell and update constraints
void applyValue(cell_t* cell, int* oldValue, int newValue) {
  int j;
  cell->value = newValue;

  for (j = 0; j < NUM_CELL_CONSTRAINTS; j++)
    updateConstraint(myCells, myConstraints[cell->constraints[j]], *oldValue, newValue);
  *oldValue = newValue;
}

// Restore a cell to its constraints
void restoreCellToConstraints(cell_t* cell, int cellIndex, int oldValue) {
  int i;
  constraint_t* constraint;

  // Add cell back to its constraints
  for (i = 0; i < NUM_CELL_CONSTRAINTS; i++) {
    constraint = myConstraints[cell->constraints[i]];
    updateConstraint(myCells, constraint, oldValue, UNASSIGNED_VALUE);

    // Add cell back to cell list after updating constraint so cell's
    // possibles are not changed during the update
    addCellToConstraint(constraint, cellIndex);
  }
}
