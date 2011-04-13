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

#include "kenken.h"
#include <omp.h>

// Number of processors
#define P 32

// Increment in the job array
#define INCREMENT(i) (((i) + 1) % (maxJobs))
// Index into jobs array
#define GET_JOB(i) ((i)*(jobLength))

typedef struct job {
  int cellIndex;
  int value;
} job_t;

// Algorithm functions
int runParallel();
int fillJobs(int step, job_t* myJob, cell_t* myCells, constraint_t* myConstraints);
int solve(int step, cell_t* myCells, constraint_t* myConstraints);
void removeCellFromConstraints(constraint_t* myConstraints, cell_t* cell, int cellIndex);
void applyValue(cell_t* myCells, constraint_t* myConstraints, cell_t* cell, int* oldValue, int newValue);
void restoreCellToConstraints(cell_t* myCells, constraint_t* myConstraints, cell_t* cell, int cellIndex, int oldValue);

// Problem grid
cell_t* cells;
// Constraints array
constraint_t* constraints;
// Max number of jobs
int maxJobs;
// Number of cells set per job
int jobLength;
// Job queue
job_t* jobs;
// Index to the head of the queue
int queueHead;
// Index to the tail of the queue
int queueTail;
// Flag to mark if a solution is found by a processor
int found;

int main(int argc, char **argv)
{
  int i;

  constraints = NULL;
  cells = NULL;

  // Initialize global variables and data-structures.
  initialize(argv[1], &cells, &constraints);

  // Initialize job queue (implemented as a circular array)
  // Always keeps one block in the array empty. This is necessary because of
  // the concurrent access. Readers modify end, writer modifies start.
  // Inherently thread-safe as long as reader's access is controlled.
  jobLength = N; // Half the cells filled in
  maxJobs = N*N;

  jobs = (job_t*)calloc(sizeof(job_t), jobLength * maxJobs);
  if (!jobs)
    unixError("Failed to allocated memory for the job queue");

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
  job_t* myJob, *jobStep;
  cell_t* cell, *myCells;
  constraint_t* myConstraints;
  int cellIndex, oldValue;

  // Begin parallel
  omp_set_num_threads(P);

  // Run algorithm
#pragma omp parallel shared(queueHead, queueTail, found, cells, constraints, \
														maxMultiply, N, totalNumCells, numConstraints) \
                    private(myConstraints, myCells, myJob, jobStep, \
                            cellIndex, cell, oldValue, step)
{
  found = 0;

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
  #pragma omp single nowait
  {
    fillJobs(0, myJob, myCells, myConstraints);
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
      while ((!found) && (queueHead == queueTail)) {
				sleep(0);
			}

      // Copy over job
      memcpy(myJob, &jobs[GET_JOB(queueHead)], sizeof(job_t) * jobLength);
      queueHead = INCREMENT(queueHead);
    }

    // Apply job: Note that cellIndices align properly with system state
    // of cell/constraint modifications because the method for achieving their
    // values are similar to solving.
		// TODO: optimize this portion to "smartly" do it based on previous job
    for (step = 0; step < jobLength; step++) {
      oldValue = UNASSIGNED_VALUE;
      jobStep = &myJob[step];

      // Remove cell from Constraint
      cellIndex = jobStep->cellIndex;
      cell = &myCells[cellIndex];
      removeCellFromConstraints(myConstraints, cell, cellIndex);

      // Set value
      applyValue(myCells, myConstraints, cell, &oldValue, jobStep->value);
    }

    // Begin computation
    if (solve(step, myCells, myConstraints)) {
      // If succeed, Copy answer to global cells
      #pragma omp single nowait
      {
        memcpy(cells, myCells, totalNumCells * sizeof(cell_t));
      }

      // Set found
      found = 1;
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
      restoreCellToConstraints(myCells, myConstraints, cell, cellIndex, jobStep->value);
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
int fillJobs(int step, job_t* myJob, cell_t* myCells, constraint_t* myConstraints) {
  int i, oldValue = UNASSIGNED_VALUE;
  cell_t* cell; 
  int nextCellIndex;
 
  if (found)
    return 1;

  if (step == jobLength) {
    // Loop while buffer is full and no solution is found
    while (!found && INCREMENT(queueTail) == queueHead) {
			sleep(0);
		}

    // If we exited while because solution was found, exit
    if (found) 
      return 1;

    // Spot in the buffer freed up so set value as current job
    memcpy(&jobs[GET_JOB(queueTail)], myJob, sizeof(job_t) * jobLength);
    queueTail = INCREMENT(queueTail);
    return 0;
  }

  nextCellIndex = findNextCell(myCells);

  if (nextCellIndex < 0)
    return 0;

  // Use the found cell as the next cell to fill
  cell = &(myCells[nextCellIndex]);

  // Remove cell from its constraints
  removeCellFromConstraints(myConstraints, cell, nextCellIndex);

  // Try all possible values for next cell
  for (i = N; i > 0; i--) {
    if (!IS_POSSIBLE(cell->possibles[i]))
      continue;

    // Store job step values
    myJob[step].cellIndex = nextCellIndex;
    myJob[step].value = i;
    
    // Set job values
    applyValue(myCells, myConstraints, cell, &oldValue, i);
    
    if (fillJobs(step + 1, myJob, myCells, myConstraints))
      return 1;
  }
 
  // Restore cell to its constraints
  restoreCellToConstraints(myCells, myConstraints, cell, nextCellIndex, oldValue);

  // Unassign value and fail if none of possibilities worked
  cell->value = UNASSIGNED_VALUE;
  return 0;
}

// Main recursive function used to solve the program
int solve(int step, cell_t* myCells, constraint_t* myConstraints) {
  int i, nextCellIndex, oldValue = UNASSIGNED_VALUE;
  cell_t* cell;

  // Success if all cells filled in. Or if another processor found a solution
  if (step == totalNumCells || found == 1)
    return 1;

  nextCellIndex = findNextCell(myCells);

  if (nextCellIndex < 0)
    return 0;

  // Use the found cell as the next cell to fill
  cell = &(myCells[nextCellIndex]);
  
  removeCellFromConstraints(myConstraints, cell, nextCellIndex);

  // Try all possible values for next cell
  for (i = N; i > 0; i--) {
    if (!IS_POSSIBLE(cell->possibles[i]))
      continue;

    applyValue(myCells, myConstraints, cell, &oldValue, i);
    
    if (solve(step + 1, myCells, myConstraints)) 
      return 1;
  }
  
  restoreCellToConstraints(myCells, myConstraints, cell, nextCellIndex, oldValue);

  // Unassign value and fail if none of possibilities worked
  cell->value = UNASSIGNED_VALUE;
  return 0;
}

// Remove a cell from its constraints
void removeCellFromConstraints(constraint_t* myConstraints, cell_t* cell, int cellIndex) {
  int i;
  
  // Remove cell from its constraints
  for (i = 0; i < NUM_CELL_CONSTRAINTS; i++)
    removeCellFromConstraint(&myConstraints[cell->constraintIndexes[i]], cellIndex);
}

// Apply a value to a cell and update constraints
// We can use myCells as a reference because only an individual processor
// will ever call this function
void applyValue(cell_t* myCells, constraint_t* myConstraints, cell_t* cell, int* oldValue, int newValue) {
  int j;
  cell->value = newValue;

  for (j = 0; j < NUM_CELL_CONSTRAINTS; j++)
    updateConstraint(myCells, &myConstraints[cell->constraintIndexes[j]], *oldValue, newValue);
  *oldValue = newValue;
}

// Restore a cell to its constraints
// We may use myCells as a reference because only an individual processor
// will ever call this function
void restoreCellToConstraints(cell_t* myCells, constraint_t* myConstraints, cell_t* cell, int cellIndex, int oldValue) {
  int i;
  constraint_t* constraint;

  // Add cell back to its constraints
  for (i = 0; i < NUM_CELL_CONSTRAINTS; i++) {
    constraint = &myConstraints[cell->constraintIndexes[i]];
    updateConstraint(myCells, constraint, oldValue, UNASSIGNED_VALUE);

    // Add cell back to cell list after updating constraint so cell's
    // possibles are not changed during the update
    addCellToConstraint(constraint, cellIndex);
  }
}
