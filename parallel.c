// ============================================================================
// Jonathan Park (jjp1)
// Ben Parr (bparr)
//
// File: kenken.c
// Description: Parallel implementation of a KenKen puzzle solver.
//
// CS418 Project
// ============================================================================

#include "kenken.h"
#include <sys/time.h>
#include <omp.h>

// Length of processor job queue
#define QUEUE_LENGTH 20
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
  volatile int head;
  volatile int tail;
  omp_lock_t headLock;
} job_queue_t;

// Algorithm functions
void runParallel(unsigned P);
int getNextJob(int pid, job_t* myJob);
int addToQueue(int step, cell_t* myCells, constraint_t* myConstraints,
               job_queue_t* myJobQueue, assignment_t* assignments,
               int availableSpots);
int solve(int step, cell_t* myCells, constraint_t* myConstraints,
          long long* myNodeCount);
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
volatile int found;
// Number of nodes visited
long long nodeCount;
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
  nodeCount = 0;
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

  // Print out number of nodes visited and calculated times
  printf("Nodes Visited: %lld\n", nodeCount);
  printf("Computation Time = %.3f millisecs\n", compTime);
  printf("      Total Time = %.3f millisecs\n", totalTime);

  return 0;
}

// Sets up and runs the parallel kenken solver
void runParallel(unsigned P) {
  int i, pid;
  long long myNodeCount;
  job_t* myJob;
  cell_t* myCells;
  constraint_t* myConstraints;
  struct timeval startCompTime, endCompTime;

  // Begin parallel
  omp_set_num_threads(P);

  // Run algorithm
#pragma omp parallel default(shared) private(i, pid, myNodeCount, myJob, \
                                             myCells, myConstraints)
{
  // Initialize local variables and data-structures
  pid = omp_get_thread_num();
  myNodeCount = 0;

  myConstraints = (constraint_t*)calloc(sizeof(constraint_t), numConstraints);
  if (!myConstraints)
    unixError("Failed to allocate memory for myConstraints");

  myCells = (cell_t*)calloc(sizeof(cell_t), totalNumCells);
  if (!myCells)
    unixError("Failed to allocate memory for myCells");

  myJob = (job_t*)malloc(sizeof(job_t));
  if (!myJob)
    unixError("Failed to allocate memory for myJob");

  // Record start of computation time
  #pragma omp single
    gettimeofday(&startCompTime, NULL);

  // Get and complete new job until none left, or solution found
  while (getNextJob(pid, myJob)) {
    memcpy(myConstraints, constraints, numConstraints * sizeof(constraint_t));
    memcpy(myCells, cells, totalNumCells * sizeof(cell_t));

    for (i = 0; i < myJob->length; i++)
      applyValue(myCells, myConstraints, myJob->assignments[i].cellIndex,
                 myJob->assignments[i].value);

    if (ADD_TO_QUEUE(&(jobQueues[pid]), myJob)) {
      myNodeCount++;
      // Guarenteed to succeed given ADD_TO_QUEUE(...) returned true
      addToQueue(myJob->length, myCells, myConstraints, &(jobQueues[pid]),
                 myJob->assignments, AVAILABLE(&jobQueues[pid]));
    }
    else
      solve(myJob->length, myCells, myConstraints, &myNodeCount);
  }

  #pragma omp critical
    nodeCount += myNodeCount;
}

  // Calculate computation time
  gettimeofday(&endCompTime, NULL);
  compTime = TIME_DIFF(endCompTime, startCompTime);
}

// Retrieves the next job from the job array. Will block until a job is
// available. Note, if the puzzle has no solution, then this will block forever.
// Since only care about puzzles with solutions, this is fine (no need to add
// logic for a case that will never happen by assumption).
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

// Split up a job into smaller jobs and add each part to the given queue.
// Returns the number of spots used, or -1 if failed to split up job.
int addToQueue(int step, cell_t* myCells, constraint_t* myConstraints,
               job_queue_t* myJobQueue, assignment_t* assignments,
               int availableSpots) {
  int cellIndex, spotsUsed;
  int value = UNASSIGNED_VALUE;
  int originalAvailableSpots = availableSpots;
  job_t* job;

  if (step > MAX_JOB_LENGTH)
    return -1;

  cellIndex = getNextCellToFillN(myCells, myConstraints, availableSpots);
  if (cellIndex == IMPOSSIBLE_STATE)
    return 0;
  else if (cellIndex == TOO_MANY_POSSIBLES)
    return -1;

  // Preemptively assign each possible a spot in the queue, so there is always
  // at least room in queue to add the job itself
  availableSpots -= getNumPossibles(cells, cellIndex);

  assignments[step].cellIndex = cellIndex;
  while (UNASSIGNED_VALUE != (value = applyNextValue(myCells, myConstraints,
                                                     cellIndex, value))) {
    assignments[step].value = value;
    spotsUsed = addToQueue(step + 1, myCells, myConstraints, myJobQueue,
                           assignments, availableSpots + 1);

    // Able to add split up job to queue, so just update available spot count
    if (spotsUsed >= 0) {
      availableSpots -= (spotsUsed - 1);
      continue;
    }

    // Add job to queue since failed to add split up job to queue
    job = &(myJobQueue->queue[myJobQueue->tail]);
    memcpy(&(job->assignments), assignments, (step + 1) * sizeof(assignment_t));
    job->length = step + 1;

    myJobQueue->tail = INCREMENT(myJobQueue->tail);
  }

  // Return the number of spots in the queue that were used
  // Note: this value is always in range of [0, originalAvailalbeSpots]
  return (originalAvailableSpots - availableSpots);
}

// Main recursive function used to solve the program
int solve(int step, cell_t* myCells, constraint_t* myConstraints,
          long long* myNodeCount) {
  int cellIndex;
  int value = UNASSIGNED_VALUE;

  if (found)
    return 1;

  if (step == totalNumCells) {
    // Print out solution and set found to true
    #pragma omp critical
    {
      if (!found) {
        printSolution(myCells);
        found = 1;
      }
    }

    return 1;
  }

  (*myNodeCount)++;
  // Find the next cell to fill and test all possible values
  if ((cellIndex = getNextCellToFill(myCells, myConstraints)) < 0)
    return 0;

  while (UNASSIGNED_VALUE != (value = applyNextValue(myCells, myConstraints,
                                                     cellIndex, value))) {
    if (solve(step + 1, myCells, myConstraints, myNodeCount))
      return 1;
  }

  return 0;
}


// Print usage information and exit
void usage(char* program) {
  printf("Usage: %s P filename\n", program);
  exit(0);
}

