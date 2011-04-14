/** @file kenken.c
 *
 *  @brief Parallel implementation of a KenKen puzzle solver.
 *
 *  @author Jonathan Park (jjp1) and Ben Parr (bparr)
 */

#include "kenken.h"
#include <omp.h>

// Increment in the job array
#define INCREMENT(i) (((i) + 1) % (maxJobs))
// Index into jobs array
#define GET_JOB(i) ((i)*(jobLength))

typedef struct job {
  int cellIndex;
  int value;
} job_t;

// Algorithm functions
void runParallel(unsigned P);
int getNextJob(job_t* myJob);
int fillJobs(int step, job_t* myJob, cell_t* myCells,
             constraint_t* myConstraints);
int solve(int step, cell_t* myCells, constraint_t* myConstraints);
void usage(char* program);


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
volatile int queueHead;
// Index to the tail of the queue
volatile int queueTail;
// Flag to mark if a solution is found by a processor
volatile int found;

int main(int argc, char **argv)
{
  if (argc != 3)
    usage(argv[0]);

  // Initialize global variables and data-structures.
  initialize(argv[2], &cells, &constraints);

  // Initialize job queue (implemented as a circular array)
  // Always keeps one block in the array empty. This is necessary because of
  // the concurrent access. Readers modify end, writer modifies start.
  // Inherently thread-safe as long as reader's access is controlled.
  jobLength = N;
  maxJobs = N * N;

  jobs = (job_t*)calloc(sizeof(job_t), jobLength * maxJobs);
  if (!jobs)
    unixError("Failed to allocated memory for the job queue");

  queueHead = 0;
  queueTail = 0;
  found = 0;

  runParallel(atoi(argv[1]));
  if (!found)
    appError("No solution found");

  return 0;
}

// Sets up and runs the parallel kenken solver
void runParallel(unsigned P) {
  int i;
  job_t* myJob;
  cell_t* myCells;
  constraint_t* myConstraints;

  // Begin parallel
  omp_set_num_threads(P);

  // Run algorithm
#pragma omp parallel default(shared) private(i, myConstraints, myCells, myJob)
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


  // Begin finding jobs and running algorithm
  // After filling jobs, thread should go and help compute remaining jobs
  #pragma omp master
    fillJobs(0, myJob, myCells, myConstraints);

  // Get and complete new job until none left, or solution found
  while (getNextJob(myJob)) {
    memcpy(myConstraints, constraints, numConstraints * sizeof(constraint_t));
    memcpy(myCells, cells, totalNumCells * sizeof(cell_t));

    for (i = 0; i < jobLength; i++)
      applyValue(myCells, myConstraints, myJob[i].cellIndex, myJob[i].value);

    solve(jobLength, myCells, myConstraints);
  }
}
}

// Retrieves the next job from the job array. Will block until a job is
// available
int getNextJob(job_t* myJob) {
  int gotJob = 0;
  while (!gotJob) {
    // Block until job becomes available
    while (!found && (queueHead == queueTail));

    if (found)
      return 0;

    // Only one processor should actually get the job
    #pragma omp critical
    {
      if (queueHead != queueTail && !found) {
        // Copy over job
        memcpy(myJob, &jobs[GET_JOB(queueHead)], sizeof(job_t) * jobLength);
        queueHead = INCREMENT(queueHead);

        // Successfully retrieved job
        gotJob = 1;
      }
    }
  }

  return 1;
}

// Fills the job array. Returns 1 to end the filling, 0 to continue
int fillJobs(int step, job_t* myJob, cell_t* myCells,
             constraint_t* myConstraints) {
  int cellIndex;
  int value = UNASSIGNED_VALUE;

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
    return 0;
  }

  // Find the next cell to fill and test all possible values
  if ((cellIndex = getNextCellToFill(myCells, myConstraints)) < 0)
    return 0;

  while (UNASSIGNED_VALUE != (value = applyNextValue(myCells, myConstraints,
                                                     cellIndex, value))) {
    myJob[step].cellIndex = cellIndex;
    myJob[step].value = value;

    if (fillJobs(step + 1, myJob, myCells, myConstraints))
      return 1;
  }

  return 0;
}

// Main recursive function used to solve the program
int solve(int step, cell_t* myCells, constraint_t* myConstraints) {
  int cellIndex;
  int value = UNASSIGNED_VALUE;

  if (found)
    return 1;

  if (step == totalNumCells) {
    // Critical section that prints cells if a solution is found
    #pragma omp critical
    {
      if (!found) {
        printSolution(myCells);
        found = 1;
      }
    }

    return 1;
  }

  // Find the next cell to fill and test all possible values
  if ((cellIndex = getNextCellToFill(myCells, myConstraints)) < 0)
    return 0;

  while (UNASSIGNED_VALUE != (value = applyNextValue(myCells, myConstraints,
                                                     cellIndex, value))) {
    if (solve(step + 1, myCells, myConstraints))
      return 1;
  }

  return 0;
}

// Print usage information and exit
void usage(char* program) {
  printf("Usage: %s P filename\n", program);
  exit(0);
}

