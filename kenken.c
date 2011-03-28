/**	@file kenken.c
 *
 *	@author Jonathan Park (jjp1) and Ben Parr (bparr)
 */

#define MAX_POSSIBLE_PROBLEM 25
#define POSSIBLE 3 // Value of possible such that it actually is possible
#define UNASSIGNED_VALUE 0
#define CONSTRAINT_NUM 3

#define MAX_LINE_LEN 512

#define ROW 0
#define COLUMN 1
#define BLOCK 2

// Get index into a 2d array initialized as a 1d array
#define GET_INDEX(x, y, size) ((size) * (x) + (y))

typedef enum {
  LINE,
  PLUS,
  MINUS,
  PARTIAL_MINUS,
  MULT,
  DIVID,
  PARTIAL_DIV,
  SINGLE
} type_t;

typedef struct possible {
  char flags[MAX_POSSIBLE_PROBLEM + 1];
  int num;
} possible_t;

// TODO: need head/tail pointers (right now just a head ptr)
// Can switch cellnode_t* cells to point to a struct holding head/tail pointers
typedef struct constraint {
  type_t type;
  long value;
  possible_t possibles;
  char flags[MAX_POSSIBLE_PROBLEM*MAX_POSSIBLE_PROBLEM];
  int numCells;
  cellnode_t* cells;
} constraint_t;

typedef struct cell {
  int value;
  possible_t possibles;
  struct constraint* constraints[CONSTRAINT_NUM];
} cell_t;

typedef struct cellnode {
  cell_t* cell;
  cell_t* prev;
  cell_t* next;
}

/** TODO: incorporate for optimizing finding the next possible?
typedef struct step {
  cell_t* modifiedCell;
  int lastIndex;
} step_t;
*/

// Definitions for possible_t functions
int isPossible(possible_t* possible, int i);
int addPossible(possible_t* possible, int i);
int removePossible(possible_t* possible, int i);
int getNumPossible(possible_t* possible);
int getNextPossible(possible_t* possible);

// Definitions for constraint_t functions
int initializeConstraint(constraint_t* constraint, type_t type, int value);
int addCell(constraint_t* constraint, cell_t* cell);
int removeCell(constraint_t* constraint, cell_t* cell, int value);
int markImpossible(constraint_t* constraint, int value);

void initRowConstraint(constraint_t* constraint, int row);
void initColConstraint(constraint_t* constraint, int col);

// Declarations for cell_t functions
void removePossibles(cell_t* cell, int newvalue);

// Miscellaneous functions
void usage(char* program);
void readLine(FILE* in, char* lineBuf);
void unixError(const char* str);

// Problem size
int problemSize;
// Number of cells in the problem
int numCells;
// Problem grid
cell_t* cells;
// Number of constraints
int numConstraints;
// Steps array: stores indices to the cell that was modified
int* steps;

int main(int argc, char **argv)
{
  FILE* in;
  unsigned P = 1;
  int optchar, index, numCages;
  char lineBuf[MAX_LINE_LEN];
  char* ptr, *coordinate;
  int x, y;
  constraint_t* constraint;
  cellnode_t* newnode;

  // Check arguments
  if (argc != 0)
    usage(argv[0]);
  
  // Read in file
  if (!(in = fopen(argv[optind], "r"))) {
    unixError("Failed to open input file");
    exit(1);
  }

  // TODO: Consider using the maximum sizes for all initialization to be
  //       consistent throughout the code

  // Read in problem size
  readLine(in, lineBuf);
  problemSize = atoi(lineBuf);

  numCells = problemSize*problemSize;

  // Initiate problem board
  cells = (cell_t*)calloc(sizeof(cell_t), numCells);
  if (!cells)
    unixError("Failed to allocate memory for the cells");

  numConstraints = 2*problemSize; // problemSize row constraints +
                                  // problemSize col constraints

  // Read in number of constraints
  readLine(in, lineBuf);
  numCages = atoi(lineBuf);
  numConstraints += numCages;

  // TODO: consider order of constraints inserted into the array or
  //       after insertion then sort by size?
  constraints = (constraint_t*)calloc(sizeof(constraint_t), numConstraints);
  if (!constraints)
    unixError("Failed to allocate memory for the constraints");

  // Initialize row constraints
  index = 0;
  for (i = 0; i < problemSize; i++)
    initRowConstraint(&constraints[index++], i);

  // Initialize column constraints
  for (i = 0; i < problemSize; i++)
    initColConstraint(&constraints[index++], i);

  // Read in block constraints
  for (i = 0; i < numCages; i++) {
    readLine(in, lineBuf);
    constraint = &constraints[index++];

    ptr = strtok(lineBuf, " ");
    // Check type
    switch (ptr) {
      case "+":
        constraint->type = PLUS;
        break;
      case "-":
        constraint->type = MINUS;
        break;
      case "x":
        constraint->type = MULT;
        break;
      case "/":
        constraint->type = DIV;
        break;
      case "!":
        constraint->type = SINGLE;
        break;
      default:
        // TODO: error?
        constraint->type = SINGLE;
        break;
    }

    // Read in value
    ptr = strtok(NULL, " ");
    constraint->value = (long)atoi(ptr);

    ptr = strtok(NULL, " ");
    // Read in coordinates for cells
    while (ptr) {
      // TODO: error checking
      coordinate = strtok(ptr, ","); // x-value
      x = atoi(coordinate);
      coordinate = strtok(NULL, ",");
      y = atoi(coordinate);

      // Mark as present
      constraint->flags[GET_INDEX(x, y, MAX_PROBLEM_SIZE)] = 1;

      // Add cell to cells list
      newnode = (cellnode_t*)calloc(sizeof(cellnode_t), 1);
      newnode->cell = &(cells[GET_INDEX(x, y, problemSize)]);
      cellnode_insert(&(constraint->cells), newnode);
      
      // Update cells count
      constraint->numCells++;

      // add constraint to cell
      newnode->cell.constraints[BLOCK] = constraint;

      ptr = strtok(NULL, " ");
    }

    // Adjust possible list for constraint
    updateBlockPossibles(constraint);
  }

  // TODO: validation check to see if all cells are accounted for?

  // Run algorithm

  // Free data
  return 0;
}

int solve() {
  // Initialize
  int i, minPossible = INT_MAX, numPossible = 0, index;
  cell_t* minCell, *cell;
  int newvalue;

  int step = 0;

  while (step != DONE_STEP) {

    // Find next cell to fill
    for (i = 0; i < numCells; i++) {
      cell = &(cells[i]);

      // Skip assigned cells
      if (cell->value != UNASSIGNED_VALUE)
        continue;

      if ((numPossible = getNumPossible(&(cell->possibles))) < minPossible) {
        minPossible = numPossible;
        minCell = cell;
        index = i;
      }
    }

    // If possible values exist
    if (minPossible > 0) {  
      // Get next possible value
      newvalue = getNextPossible(&(minCell->possibles))

      // Set cell
      minCell->value = newvalue;

      // Remove possibility from constraints and cell
      removePossibiles(minCell, newvalue);

      // Store step information

      // Loop to next step
    } else {
      // Else
      // Backtrack

      // Set 
    }
  }

  return 1;
}

int isPossible(possible_t* possible, int i) {
  return 1;
}

int addPossible(possible_t* possible, int i) {
  return 1;
}

int removePossible(possible_t* possible, int i) {
  return 1;
}

int getNumPossible(possible_t* possible) {
  return possible->num;
}

// Initializes a row constraint for the given row
void initRowConstraint(constraint_t* constraint, int row) {
  
}

// Initializes a column constraint for the given column
void initColConstraint(constraint_t* constraint, int col) {
}

// Removes possibilities from the cell's constraints and its own possibles
void removePossibles(cell_t* cell, int newvalue) {
  int i;

  for (i = 0; i < CONSTRAINTS_NUM; i++) {
    // If constraint has the possibility, remove + decrement from cell
    if (removeConstraintsPossible(cell->constraints[i], newvalue))
      removePossible(&(cell->possibles), newvalue);
  }
}

// Restores possibles from the cell's constraints and its own possibles
void restorePossibles(cell_t* cell, int newvalue) {
  int i;
}

// Print usage information and exit
void usage(char* program) {
  printf("Usage: %s filename\n", program);
  exit(0);
}

void readLine(FILE* in, char* lineBuf) {
  if (!fgets(lineBuf, MAX_LINE_LEN, in))
    unixError("Failed to read line from input file");
}

// Display a unix error and exit
void unixError(const char* str) {
  perror(str);
  exit(1);
}
