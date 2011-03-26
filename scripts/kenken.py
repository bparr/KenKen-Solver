#!/usr/bin/python

# Class for labels for constrains (e.g. "3+")
class ConstraintLabel:
  _type = ""
  _num = -1
  cell = None

  def __init__(self, type, num, cell):
    self._type = type
    self._num = num
    self.cell = cell # Which cell the label will appear in

  def __str__(self):
    s = str(self._num)
    if self._type != "!":
      s += self._type
    return s

# Class containing information about a single constraint block
class Constraint:
  _type = ""
  _num = -1
  cells = None
  label = None

  def __init__(self, type, num, cells):
    self._type = type
    self._num = num
    self.cells = cells

    labeledCell = cells[0]
    for cell in (cells[1:]):
      if cell[0] < labeledCell[0] or \
         (cell[0] == labeledCell[0] and cell[1] < labeledCell[1]):
        labeledCell = cell

    self.label = ConstraintLabel(type, num, labeledCell)

  # Check if an array of answers meets the constraint
  def isAnswer(self, answers):
    if self._type == "+":
      result = reduce(lambda x,y: x + y, answers, 0)
    elif self._type == "x":
      result = reduce(lambda x,y: x * y, answers, 1)
    elif self._type == "-":
      if len(answers) != 2:
        return False
      result = abs(answers[0] - answers[1])
    elif self._type == "/":
      if len(answers) != 2:
        return False
      result = (1.0 * max(answers[0], answers[1])) / min(answers[0], answers[1])
    elif self._type == "!":
      if len(answers) != 1:
        return False
      result = answers[0]
    else:
      return False

    return result == self._num

# Class containing information about a KenKen puzzle
class Puzzle:
  n = -1 # Size of the board: n x n
  _board = None # Used to check if two cells are in same constraint block
  _answer = None
  constraints = None

  def __init__(self, input):
    self.parseFile(input)

  def inSameConstraint(self, x1, y1, x2, y2):
    return self._board[x1][y1] == self._board[x2][y2]

  def getAnswer(self, x, y):
    return self._answer[x][y]

  def parseFile(self, input):
    f = open(input, "r")
    lines = f.readlines()
    f.close()

    # Initialize class variables
    self.n = int(lines.pop(0))
    self._board = []
    self._answer = []
    for i in range(self.n):
      self._board.append([])
      self._answer.append([])
      for j in range(self.n):
        self._board[i].append(-1)
        self._answer[i].append(-1)

    # Parse constraints
    numConstraints = int(lines.pop(0))
    self.constraints = []
    for (i, line) in zip(range(numConstraints), lines):
      lineParts = line.split(" ")
      cells = map(lambda x: map(lambda y: int(y), x.split(",")), lineParts[2:])

      for cell in cells:
        self._board[cell[0]][cell[1]] = i

      constraint = Constraint(lineParts[0], int(lineParts[1]), cells)
      self.constraints.append(constraint)

    # Parse answer (if exists)
    lines = lines[numConstraints:]
    for (i, line) in zip(range(self.n), lines):
      lineParts = line.split(" ")
      for (j, linePart) in zip(range(self.n), lineParts):
        self._answer[i][j] = int(linePart)

