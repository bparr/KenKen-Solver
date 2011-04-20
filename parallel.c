/** @file kenken.c
 *
 *  @brief Parallel implementation of a KenKen puzzle solver.
 *
 *  @author Jonathan Park (jjp1) and Ben Parr (bparr)
 */

#include "kenken.h"
#include <sys/time.h>
#include <omp.h>

// Length of processor job queue
#define QUEUE_LENGTH 20 // TODO use better value?
// Increment in the job array
#define INCREMENT(i) (((i) + 1) % (QUEUE_LENGTH))
// Number of available slots in queue
#define AVAILABLE(q) ((QUEUE_LENGTH - 1 - ((QUEUE_LENGTH + (q)->tail - \
                      (q)->head) % QUEUE_LENGTH)))
// Maximum length of a job that can be added to a queue
#define MAX_JOB_LENGTH (5 * N)
// Whether or not a job should be added to a queue
#define ADD_TO_QUEUE(q, j) ((j)->length < MAX_JOB_LENGTH && AVAILABLE(q) >= N)

// Calculate number of milliseconds between two timevals
#define TIME_DIFF(b, a) (((b).tv_sec - (a).tv_sec) * 1000.0 + \
                         ((b).tv_usec - (a).tv_usec) / 1000.0)

typedef struct assignment {
  int cellIndex;
  int value;
} assignment_t;

typedef struct job {
  int length;
  assignment_t assignments[MAX_PROBLEM_SIZE * MAX_PROBLEM_SIZE];
} job_t;

// Job queue implemented as a circular array. Jobs are popped from head, and
// pushed to tail.
typedef struct job_queue {
  job_t queue[QUEUE_LENGTH];
  volatile int head; // TODO remove volatile?
  volatile int tail; // TODO remove volatile?
  omp_lock_t headLock;
} job_queue_t;

// Algorithm functions
void runParallel(unsigned P);
int getNextJob(int pid, job_t* myJob);
void addToQueue(int step, cell_t* myCells, constraint_t* myConstraints,
                job_queue_t* myJobQueue, assignment_t* assignments);
int solve(int step, cell_t* myCells, constraint_t* myConstraints);
void usage(char* program);


// Number of processors
unsigned P;
// Problem grid
cell_t* cells;
// Constraints array
constraint_t* constraints;
// Array of job queues, so each processor owns a queue
job_queue_t* jobQueues;
// Flag to mark if a solution is found by a processor
volatile int found; // TODO remove volatile?
// Program execution timinges (in milliseconds)
double totalTime, compTime;

int main(int argc, char **argv) {
  int i;
  struct timeval startTime, endTime;

  if (argc != 3)
    usage(argv[0]);

  // Record start of total time
  gettimeofday(&startTime, NULL);

  // Initialize global variables and data-structures.
  P = atoi(argv[1]);
  initialize(argv[2], &cells, &constraints);
  found = 0;

  jobQueues = (job_queue_t*)calloc(sizeof(job_queue_t), P);
  if (!jobQueues)
    unixError("Failed to allocated memory for the job queues");

  for (i = 0; i < P; i++)
    omp_init_lock(&(jobQueues[i].headLock));

  // Add initial job (nothing assigned) to root processor
  jobQueues[0].tail = 1;


  runParallel(P);
  if (!found)
    appError("No solution found");

  // Use final time to calculate total time
  gettimeofday(&endTime, NULL);
  totalTime = TIME_DIFF(endTime, startTime);

  // Print out calculated times
  printf("Computation Time = %.3f millisecs\n", compTime);
  printf("      Total Time = %.3f millisecs\n", totalTime);

  return 0;
}

// Sets up and runs the parallel kenken solver
void runParallel(unsigned P) {
  int i, pid;
  job_t* myJob;
  cell_t* myCells;
  constraint_t* myConstraints;
  struct timeval startCompTime, endCompTime;

  // Begin parallel
  omp_set_num_threads(P);

  // Run algorithm
#pragma omp parallel default(shared) private(i,pid,myConstraints,myCells,myJob)
{
  pid = omp_get_thread_num();

  // Initialize our local copies of the data-structures
  myConstraints = (constraint_t*)calloc(sizeof(constraint_t), numConstraints);
  if (!myConstraints)
    unixError("Failed to allocate memory for myConstraints");

  myCells = (cell_t*)calloc(sizeof(cell_t), totalNumCells);
  if (!myCells)
    unixError("Failed to allocate memory for myCells");

  myJob = (job_t*)malloc(sizeof(job_t));
  if (!myJob)
    unixError("Failed to allocate memory for myJob");

  #pragma omp single
    gettimeofday(&startCompTime, NULL);

  // Get and complete new job until none left, or solution found
  while (getNextJob(pid, myJob)) {
    memcpy(myConstraints, constraints, numConstraints * sizeof(constraint_t));
    memcpy(myCells, cells, totalNumCells * sizeof(cell_t));

    for (i = 0; i < myJob->length; i++)
      applyValue(myCells, myConstraints, myJob->assignments[i].cellIndex,
                 myJob->assignments[i].value);

    if (ADD_TO_QUEUE(&(jobQueues[pid]), myJob))
      addToQueue(myJob->length, myCells, myConstraints, &(jobQueues[pid]),
                 myJob->assignments);
    else
      solve(myJob->length, myCells, myConstraints);
  }
}

  // Calculate computation time
  gettimeofday(&endCompTime, NULL);
  compTime = TIME_DIFF(endCompTime, startCompTime);
}

// Retrieves the next job from the job array. Will block until a job is
// available
// TODO don't loop forever when no solution found (important?)
int getNextJob(int pid, job_t* myJob) {
  int i;
  job_t* nextJob;

  for (i = pid; !found; i = (i + 1) % P) {
    if (jobQueues[i].head == jobQueues[i].tail)
      continue;

    omp_set_lock(&(jobQueues[i].headLock));
    if (jobQueues[i].head != jobQueues[i].tail && !found) {
      nextJob = &(jobQueues[i].queue[jobQueues[i].head]);
      myJob->length = nextJob->length;
      memcpy(myJob->assignments, nextJob->assignments,
             sizeof(assignment_t) * nextJob->length);

      jobQueues[i].head = INCREMENT(jobQueues[i].head);
      omp_unset_lock(&(jobQueues[i].headLock));
      return 1;
    }

    omp_unset_lock(&(jobQueues[i].headLock));
  }

  return 0;
}

// Given a job, iterate a step and add each possible value as a new job
void addToQueue(int step, cell_t* myCells, constraint_t* myConstraints,
               job_queue_t* myJobQueue, assignment_t* assignments) {
  int cellIndex;
  int value = UNASSIGNED_VALUE;
  job_t* job;

  if ((cellIndex = getNextCellToFill(myCells, myConstraints)) < 0)
    return;

  assignments[step].cellIndex = cellIndex;
  while (UNASSIGNED_VALUE != (value = applyNextValue(myCells, myConstraints,
                                                     cellIndex, value))) {
    assignments[step].value = value;
    job = &(myJobQueue->queue[myJobQueue->tail]);
    memcpy(&(job->assignments), assignments, (step + 1) * sizeof(assignment_t));
    job->length = step + 1;

    myJobQueue->tail = INCREMENT(myJobQueue->tail);
  }
}

// Main recursive function used to solve the program
int solve(int step, cell_t* myCells, constraint_t* myConstraints) {
  int cellIndex;
  int value = UNASSIGNED_VALUE;

  // TODO remove?
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

