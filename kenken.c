// ============================================================================
// Jonathan Park (jjp1)
// Ben Parr (bparr)
//
// File: kenken.c
// Description: General KenKen code used by both serial and parallel solvers.
//
// CS418 Project
// ============================================================================

#include "kenken.h"

// Maximum line length of input file
#define MAX_LINE_LEN 2048

// Get cell at (x, y)
#define GET_CELL(x, y) (N * (x) + (y))
// Value of node at start and end of cell list
#define END_NODE -1

// Indexes of different types of constraints in cell_t constraint array
#define ROW_CONSTRAINT_INDEX 0
#define COLUMN_CONSTRAINT_INDEX 1
#define BLOCK_CONSTRAINT_INDEX 2

// Calculate the minimum of two numbers
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
// Calculate the maximum of two numbers
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


// Max number by multiplying
long* maxMultiply;


// Update constraint from having a cell with value oldCellValue to having the
// cell assigned newCellValue (valid cell values include UNASSIGNED_VALUE).
inline void updateConstraint(cell_t* cells, constraint_t* constraint,
                             int oldCellValue, int newCellValue);

// Funtions to initialize line constraints
void initRowConstraint(cell_t* cells, constraint_t* constraints,
                       int index, int col);
void initColumnConstraint(cell_t* cells, constraint_t* constraints,
                          int index, int col);

// Functions to initialize cell's possibles in given constraint
void initLineCells(cell_t* cells, celllist_t* cellList);
void initPlusCells(cell_t* cells, celllist_t* cellList, long value,
                   int numCells);
void initMinusCells(cell_t* cells, celllist_t* cellList, long value);
void initMultiplyCells(cell_t* cells, celllist_t* cellList, long value,
                       int numCells);
void initDivideCells(cell_t* cells, celllist_t* cellList, long value);
void initSingleCells(cell_t* cells, celllist_t* cellList, long value);

// Helper functions used when updating cell's possibles
inline void updatePlusCells(cell_t* cells, celllist_t* cellList, long oldValue,
                            int oldNumCells, long newValue, int newNumCells);
inline void initMinusCellsHelper(cell_t* cells, celllist_t* cellList,
                                 long value, char markPossible);
inline void initPartialMinusCells(cell_t* cells, celllist_t* cellList,
                                  long value, int cellValue, char markPossible);
inline void updateMultiplyCells(cell_t* cells, celllist_t* cellList,
                                long oldValue, int oldNumCells, long newValue,
                                int newNumCells);
inline void initDivideCellsHelper(cell_t* cells, celllist_t* cellList,
                                  long value, char markPossible);
inline void initPartialDivideCells(cell_t* cells, celllist_t* cellList,
                                  long value, int cellValue, char markPossible);
inline void notifyCellsOfChange(cell_t* cells, celllist_t* cellList, int value,
                                char markPossible);
inline void notifyCellsOfChanges(cell_t* cells, celllist_t* cellList,
                                 int start, int end, char markPossible);

// Cell list functions
inline void initList(celllist_t* cellList);
inline void addNode(celllist_t* cellList, int node);
inline void removeNode(celllist_t* cellList, int node);

// Miscellaneous functions
void readLine(FILE* in, char* lineBuf);


// Given an input file name, initialize cells and constraints and global
// variables. Note, this should only be called once at beginning of program.
// The constraints and cells results can be memcpy'ed if need be.
void initialize(char* file, cell_t** cellsPtr, constraint_t** constraintsPtr) {
  FILE* in;
  char type, lineBuf[MAX_LINE_LEN];
  char* ptr;
  int i, x, y;
  long value;
  constraint_t* constraints, *constraint;
  cell_t* cells;
  celllist_t* cellList;

  // Read in file
  if (!(in = fopen(file, "r")))
    unixError("Failed to open input file");

  // Read in problem size and number of constraints
  readLine(in, lineBuf);
  N = atoi(lineBuf);
  if (N > MAX_PROBLEM_SIZE)
    appError("Problem size too large");

  readLine(in, lineBuf);
  // N row constraints + N column constraints + number of block constraints
  numConstraints = 2 * N + atoi(lineBuf);

  // Allocate space for cells and constraints
  totalNumCells = N * N;
  cells = (cell_t*)calloc(sizeof(cell_t), totalNumCells);
  if (!cells)
    unixError("Failed to allocate memory for the cells");

  constraints = (constraint_t*)calloc(sizeof(constraint_t), numConstraints);
  if (!constraints)
    unixError("Failed to allocate memory for the constraints");

  maxMultiply = (long*)calloc(sizeof(long), totalNumCells);
  if (!maxMultiply)
    unixError("Failed to allocate memory for the max multiply array");

  // Initialize max multiply array
  for (i = 0, value = 1; i < totalNumCells; i++) {
    maxMultiply[i] = value;

    if (value > LONG_MAX / N)
      break;
    value *= N;
  }
  for (; i < totalNumCells; i++)
    maxMultiply[i] = LONG_MAX;

  // Initialize row and column constraints
  for (i = 0; i < N; i++) {
    initRowConstraint(cells, constraints, i, i);
    initColumnConstraint(cells, constraints, i + N, i);
  }

  // Initialize block constraints
  for (i = 2 * N; i < numConstraints; i++) {
    readLine(in, lineBuf);
    constraint = &(constraints[i]);
    cellList = &(constraint->cellList);

    // Read in type
    ptr = strtok(lineBuf, " ");
    type = *ptr;

    // Read in value
    ptr = strtok(NULL, " ");
    value = atol(ptr);

    // Read in cell coordinates
    initList(cellList);
    while ((ptr = strtok(NULL, ", "))) {
      x = atoi(ptr);
      ptr = strtok(NULL, ", ");
      y = atoi(ptr);

      // Add block constraint to cell
      addNode(cellList, GET_CELL(x, y));
      cells[GET_CELL(x, y)].constraintIndexes[BLOCK_CONSTRAINT_INDEX] = i;
    }

    constraint->value = value;

    // Initialize constraint's type and possibles
    switch (type) {
      case '+':
        constraint->type = PLUS;
        initPlusCells(cells, cellList, value, constraint->cellList.size);
        break;
      case '-':
        constraint->type = MINUS;
        initMinusCells(cells, cellList, value);
        break;
      case 'x':
        constraint->type = MULTIPLY;
        initMultiplyCells(cells, cellList, value, constraint->cellList.size);
        break;
      case '/':
        constraint->type = DIVIDE;
        initDivideCells(cells, cellList, value);
        break;
      case '!':
        constraint->type = SINGLE;
        initSingleCells(cells, cellList, value);
        break;
      default:
        appError("Malformed constraint in input file");
    }
  }

  // Close file
  fclose(in);

  *constraintsPtr = constraints;
  *cellsPtr = cells;
}

// Get number of possibles for a specific cell
inline int getNumPossibles(cell_t* cells, int cellIndex) {
  return cells[cellIndex].numPossibles;
}

// Apply a value to a specific cell, updating its constraints
inline void applyValue(cell_t* cells, constraint_t* constraints, int cellIndex,
                       int value) {
  int i;
  cell_t* cell = &(cells[cellIndex]);
  constraint_t* constraint;

  // Remove cell from its constraints, and update constraint
  for (i = 0; i < NUM_CELL_CONSTRAINTS; i++) {
    constraint = &(constraints[cell->constraintIndexes[i]]);
    removeNode(&(constraint->cellList), cellIndex);
    updateConstraint(cells, constraint, UNASSIGNED_VALUE, value);
  }

  cell->value = value;
}

// Get the next cell to fill in, remove it from its constraints, and return its
// index
inline int getNextCellToFill(cell_t* cells, constraint_t* constraints) {
  return getNextCellToFillN(cells, constraints, INT_MAX);
}

// Same as getNextCellToFill, except it imposes a max number of possibles
// allowed for the chosen cell, returning TOO_MANY_POSSIBLES if broken.
inline int getNextCellToFillN(cell_t* cells, constraint_t* constraints,
                              int maxPossibles) {
  int i, numPossibles;
  int minIndex = -1, minPossibles = INT_MAX;
  cell_t* cell;
  constraint_t* constraint;

  // Find the unassigned cell with the mininum number of possibilities
  for (i = 0; i < totalNumCells; i++) {
    cell = &(cells[i]);

    // Skip assigned cells
    if (cell->value != UNASSIGNED_VALUE)
      continue;

    numPossibles = cell->numPossibles;

    // Fail early if found unassigned cell with no possibilities
    if (numPossibles == 0)
      return IMPOSSIBLE_STATE;

    if (numPossibles < minPossibles) {
      minIndex = i;
      minPossibles = numPossibles;
    }
  }

  if (minPossibles > maxPossibles)
    return TOO_MANY_POSSIBLES;

  // Remove cell from its constraints in preparation for updating
  cell = &(cells[minIndex]);
  for (i = 0; i < NUM_CELL_CONSTRAINTS; i++) {
    constraint = &(constraints[cell->constraintIndexes[i]]);
    removeNode(&(constraint->cellList), minIndex);
  }

  return minIndex;
}

// Apply and return next value for the cell currently filling in
inline int applyNextValue(cell_t* cells, constraint_t* constraints,
                          int cellIndex, int previousValue) {
  int i;
  int value = (previousValue != UNASSIGNED_VALUE) ? previousValue - 1 : N;
  cell_t* cell = &(cells[cellIndex]);
  constraint_t* constraint;

  for (; value > 0; value--) {
    if (cell->possibles[value] != NUM_CELL_CONSTRAINTS)
      continue;

    cell->value = value;

    for (i = 0; i < NUM_CELL_CONSTRAINTS; i++) {
      constraint = &(constraints[cell->constraintIndexes[i]]);
      updateConstraint(cells, constraint, previousValue, value);
    }

    return value;
  }

  for (i = 0; i < NUM_CELL_CONSTRAINTS; i++) {
    constraint = &(constraints[cell->constraintIndexes[i]]);
    updateConstraint(cells, constraint, previousValue, UNASSIGNED_VALUE);

    // Add cell back to cell list after updating constraint so cell's
    // possibles are not changed during the update
    addNode(&(constraint->cellList), cellIndex);
  }

  cell->value = UNASSIGNED_VALUE;
  return UNASSIGNED_VALUE;
}

// Print solution to stdout
void printSolution(cell_t* cells) {
  int i;
  for (i = 0; i < totalNumCells; i++)
    printf("%d%c", cells[i].value, ((i + 1) % N != 0) ? ' ' : '\n');
}


// Update constraint from having a cell with value oldCellValue to having the
// cell assigned newCellValue (valid cell values include UNASSIGNED_VALUE).
inline void updateConstraint(cell_t* cells, constraint_t* constraint,
                             int oldCellValue, int newCellValue) {
  celllist_t* cellList = &(constraint->cellList);
  long value = constraint->value;
  long oldValue = value;

  int oldNumCells = (constraint->cellList.size);
  int newNumCells = oldNumCells;
  if (oldCellValue == UNASSIGNED_VALUE)
    oldNumCells++;
  if (newCellValue == UNASSIGNED_VALUE)
    newNumCells++;

  switch (constraint->type) {
    case LINE:
      if (oldCellValue != UNASSIGNED_VALUE)
        notifyCellsOfChange(cells, cellList, oldCellValue, 1);

      if (newCellValue != UNASSIGNED_VALUE)
        notifyCellsOfChange(cells, cellList, newCellValue, 0);
      break;

    case PLUS:
      if (oldCellValue != UNASSIGNED_VALUE)
        value += oldCellValue;
      if (newCellValue != UNASSIGNED_VALUE)
        value -= newCellValue;

      constraint->value = value;
      updatePlusCells(cells, cellList, oldValue, oldNumCells,
                      value, newNumCells);
      break;

    case MINUS:
      // Don't update possibles when num cells is 0 so undoing is easier

      if (oldNumCells == 2)
        initMinusCellsHelper(cells, cellList, value, 0);
      else if (oldCellValue != UNASSIGNED_VALUE && oldNumCells == 1)
        initPartialMinusCells(cells, cellList, value, oldCellValue, 0);

      if (newNumCells == 2)
        initMinusCellsHelper(cells, cellList, value, 1);
      else if (newCellValue != UNASSIGNED_VALUE && newNumCells == 1)
        initPartialMinusCells(cells, cellList, value, newCellValue, 1);

      break;

    case MULTIPLY:
      if (oldCellValue != UNASSIGNED_VALUE)
        value *= oldCellValue;
      if (newCellValue != UNASSIGNED_VALUE)
        value /= newCellValue;

      constraint->value = value;
      updateMultiplyCells(cells, cellList, oldValue, oldNumCells,
                          value, newNumCells);
      break;

    case DIVIDE:
      // Don't update possibles when num cells is 0 so undoing is easier

      if (oldNumCells == 2)
        initDivideCellsHelper(cells, cellList, value, 0);
      else if (oldCellValue != UNASSIGNED_VALUE && oldNumCells == 1)
        initPartialDivideCells(cells, cellList, value, oldCellValue, 0);

      if (newNumCells == 2)
        initDivideCellsHelper(cells, cellList, value, 1);
      else if (newCellValue != UNASSIGNED_VALUE && newNumCells == 1)
        initPartialDivideCells(cells, cellList, value, newCellValue, 1);
      break;

    case SINGLE:
      notifyCellsOfChange(cells, cellList, value, (char)(newNumCells == 1));
      break;
  }
}


// Initializes a row constraint for the given row
void initRowConstraint(cell_t* cells, constraint_t* constraints,
                       int index, int row) {
  int i;
  constraint_t* constraint = &(constraints[index]);

  constraint->type = LINE;
  constraint->value = -1;

  // Add constraint to its cells
  initList(&(constraint->cellList));
  for (i = 0; i < N; i++) {
    addNode(&(constraint->cellList), GET_CELL(row, i));
    cells[GET_CELL(row, i)].constraintIndexes[ROW_CONSTRAINT_INDEX] = index;
  }

  initLineCells(cells, &constraint->cellList);
}

// Initializes a column constraint for the given column
void initColumnConstraint(cell_t* cells, constraint_t* constraints,
                          int index, int col) {
  int i;
  constraint_t* constraint = &(constraints[index]);

  constraint->type = LINE;
  constraint->value = -1;

  // Add constraint to its cells
  initList(&(constraint->cellList));
  for (i = 0; i < N; i++) {
    addNode(&(constraint->cellList), GET_CELL(i, col));
    cells[GET_CELL(i, col)].constraintIndexes[COLUMN_CONSTRAINT_INDEX] = index;
  }

  initLineCells(cells, &constraint->cellList);
}


// Initialize cell's possibles for a line constraint
void initLineCells(cell_t* cells, celllist_t* cellList) {
  notifyCellsOfChanges(cells, cellList, 1, N, 1);
}

// Initialize cell's possibles for a plus constraint
void initPlusCells(cell_t* cells, celllist_t* cellList, long value,
                   int numCells) {
  // Possibles = [start, end], everything else impossible
  int start = MAX(1, value - N * (numCells - 1));
  int end = MIN(N, value - (numCells - 1));
  notifyCellsOfChanges(cells, cellList, start, end, 1);
}

// Initialize cell's possibles for a minus constraint
void initMinusCells(cell_t* cells, celllist_t* cellList, long value) {
  initMinusCellsHelper(cells, cellList, value, 1);
}

// Initialize cell's possibles for a multiply constraint
void initMultiplyCells(cell_t* cells, celllist_t* cellList, long value,
                       int numCells) {
  int i;

  if (numCells == 0)
    return;

  long maxLeft = maxMultiply[numCells - 1];
  for (i = MAX(1, value / maxLeft); i <= MIN(value, N); i++) {
    if (value % i == 0)
      notifyCellsOfChange(cells, cellList, i, 1);
  }
}

// Initialize cell's possibles for a divide constraint
void initDivideCells(cell_t* cells, celllist_t* cellList, long value) {
  initDivideCellsHelper(cells, cellList, value, 1);
}

// Initialize cell's possibles for a single constraint
void initSingleCells(cell_t* cells, celllist_t* cellList, long value) {
  notifyCellsOfChange(cells, cellList, value, 1);
}


// Helper function for updating a plus constraint
inline void updatePlusCells(cell_t* cells, celllist_t* cellList, long oldValue,
                            int oldNumCells, long newValue, int newNumCells) {
  // Possibles = [start, end], everything else impossible
  int oldStart = MAX(1, oldValue - N * (oldNumCells - 1));
  int oldEnd = MIN(N, oldValue - (oldNumCells - 1));

  int newStart = MAX(1, newValue - N * (newNumCells - 1));
  int newEnd = MIN(N, newValue - (newNumCells - 1));

  // Notify of new impossibles from start/end changes
  notifyCellsOfChanges(cells, cellList, oldStart, newStart - 1, 0);
  notifyCellsOfChanges(cells, cellList, newEnd + 1, oldEnd, 0);

  // Notify of new possibles from start/end changes
  notifyCellsOfChanges(cells, cellList, newStart, oldStart - 1, 1);
  notifyCellsOfChanges(cells, cellList, oldEnd + 1, newEnd, 1);
}

// Helper function for initializing/updating a minus constraint
inline void initMinusCellsHelper(cell_t* cells, celllist_t* cellList,
                                 long value, char markPossible) {
  // Impossibles = [N - value + 1, value], everything else possible
  int secondStart = MAX(N - value + 1, value + 1);
  notifyCellsOfChanges(cells, cellList, 1, N - value, markPossible);
  notifyCellsOfChanges(cells, cellList, secondStart, N, markPossible);
}

// Helper function for initializing/updating a partial minus constraint
inline void initPartialMinusCells(cell_t* cells, celllist_t* cellList,
                                 long value, int cellValue, char markPossible) {
  if (cellValue + value <= N)
    notifyCellsOfChange(cells, cellList, cellValue + value, markPossible);

  if (cellValue - value > 0)
    notifyCellsOfChange(cells, cellList, cellValue - value, markPossible);
}

// Helper function for updating a multiply constraint
inline void updateMultiplyCells(cell_t* cells, celllist_t* cellList,
                                long oldValue, int oldNumCells, long newValue,
                                int newNumCells) {
  int i;
  int oldStart = N + 1;
  int newStart = N + 1;
  int oldEnd = MIN(oldValue, N);
  int newEnd = MIN(newValue, N);
  char oldIsPossible, newIsPossible;

  if (oldNumCells > 0)
    oldStart = MAX(1, oldValue / maxMultiply[oldNumCells - 1]);
  if (newNumCells > 0)
    newStart = MAX(1, newValue / maxMultiply[newNumCells - 1]);

  for (i = MIN(oldStart, newStart); i <= MAX(oldEnd, newEnd); i++) {
    oldIsPossible = (char)(i >= oldStart && i <= oldEnd && oldValue % i == 0);
    newIsPossible = (char)(i >= newStart && i <= newEnd && newValue % i == 0);
    if (oldIsPossible != newIsPossible)
      notifyCellsOfChange(cells, cellList, i, newIsPossible);
  }
}

// Helper function for initializing/updating a divide constraint
inline void initDivideCellsHelper(cell_t* cells, celllist_t* cellList,
                                  long value, char markPossible) {
  int i;

  // Note that value can not equal 1 since 1,1 is the only answer to 1/
  // and a divide has either 2 cells on the same row or same column
  for (i = 1; i <= N / value; i++) {
    // Ensure don't double count a value
    if (i % value != 0)
      notifyCellsOfChange(cells, cellList, i, markPossible);

    notifyCellsOfChange(cells, cellList, i * value, markPossible);
  }
}

// Helper function for initializing/updating a partial divide constraint
inline void initPartialDivideCells(cell_t* cells, celllist_t* cellList,
                                 long value, int cellValue, char markPossible) {
  if (cellValue * value <= N)
    notifyCellsOfChange(cells, cellList, cellValue * value, markPossible);

  if ((cellValue % value == 0) && cellValue >= value)
    notifyCellsOfChange(cells, cellList, cellValue / value, markPossible);
}

// Notify cells that a possible value has changed state
inline void notifyCellsOfChange(cell_t* cells, celllist_t* cellList, int value,
                                char markPossible) {
  int i;
  char flag;

  for (i = cellList->start; i != END_NODE; i = (cellList->cells[i]).next) {
    flag = cells[i].possibles[value];
    if (markPossible && flag == NUM_CELL_CONSTRAINTS - 1)
      cells[i].numPossibles++;
    if (!markPossible && flag == NUM_CELL_CONSTRAINTS)
      cells[i].numPossibles--;

    cells[i].possibles[value] = (char)(flag + (markPossible ? 1 : -1));
  }
}

// Notify cells that a range of possible values have changed state
inline void notifyCellsOfChanges(cell_t* cells, celllist_t* cellList,
                                 int start, int end, char markPossible) {
  int i, j, flag, num;

  if (end < start)
    return;

  for (i = cellList->start; i != END_NODE; i = (cellList->cells[i]).next) {
    num = cells[i].numPossibles;

    for (j = start; j <= end; j++) {
      flag = cells[i].possibles[j];
      if (markPossible && flag == NUM_CELL_CONSTRAINTS - 1)
        num++;
      if (!markPossible && flag == NUM_CELL_CONSTRAINTS)
        num--;

      cells[i].possibles[j] = (char)(flag + (markPossible ? 1 : -1));
    }

    cells[i].numPossibles = num;
  }
}


// Initialize a cell list
inline void initList(celllist_t* cellList) {
  cellList->start = END_NODE;
}

// Add node to a cell list
inline void addNode(celllist_t* cellList, int node) {
  int start = cellList->start;

  cellList->cells[node].previous = END_NODE;
  cellList->cells[node].next = start;
  cellList->start = node;

  if (start != END_NODE)
    cellList->cells[start].previous = node;

  ++(cellList->size);
}

// Remove a node from a cell list
inline void removeNode(celllist_t* cellList, int node) {
  int previous = cellList->cells[node].previous;
  int next = cellList->cells[node].next;

  if (next != END_NODE)
    cellList->cells[next].previous = previous;

  if (previous == END_NODE)
    cellList->start = next;
  else
    cellList->cells[previous].next = next;

  --(cellList->size);
}

// Read line from file into lineBuf, exiting if the read failed
void readLine(FILE* in, char* lineBuf) {
  if (!fgets(lineBuf, MAX_LINE_LEN, in))
    unixError("Failed to read line from input file");
}

// Print an application error, and exit
void appError(const char* str) {
  fprintf(stderr, "%s\n", str);
  exit(1);
}

// Print a unix error, and exit
void unixError(const char* str) {
  perror(str);
  exit(1);
}

