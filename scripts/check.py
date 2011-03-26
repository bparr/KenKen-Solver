#!/usr/bin/python

import sys
from kenken import Puzzle, ConstraintLabel

def error(msg):
  print msg 
  sys.exit(1)

if len(sys.argv) != 2:
  error("Usage: " + sys.argv[0] + " <input>")

puzzle = Puzzle(sys.argv[1])
n = puzzle.n

# Ensure all answers are in correct range
for i in range(n):
  for j in range(n):
    answer = puzzle.getAnswer(i, j)
    if answer <= 0 or answer > n:
      error("Answer out of correct range (" + str(answer) + ")")

# Ensure row constraints are met
for i in range(n):
  used = [False for j in range(n)]
  for j in range(n):
    answer = puzzle.getAnswer(i, j)
    if used[answer - 1]:
      error("Row constraint violated (" + str(i) + ", " + str(j) + ")")
    used[answer - 1] = True

# Ensure column constraints are met
for j in range(n):
  used = [False for i in range(n)]
  for i in range(n):
    answer = puzzle.getAnswer(i , j)
    if used[answer - 1]:
      error("Column constraint violated (" + str(i) + ", " + str(j) + ")")
    used[answer - 1] = True

# Ensure constraint blocks are met
for c in puzzle.constraints:
  answers = map(lambda x: puzzle.getAnswer(x[0], x[1]), c.cells)
  if not c.isAnswer(answers):
    error("Constraint block violated (" + str(c.label) + ")")


print "Success"

