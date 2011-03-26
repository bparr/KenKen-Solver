#!/usr/bin/python

import random, sys
from kenken import Puzzle, ConstraintLabel

# Staring probability of unioning two constraint blocks
startingUnionProb = .7
# Probability of using + (instead of - or /) for 2 cells
addTwoCellsProb = .2
# Probability of using x instead of + for more than 2 cells
multProb = .5

def error(msg):
  print msg
  sys.exit(1)

if len(sys.argv) != 3:
  error("Usage: " + sys.argv[0] + " <n> <output file>")

n = int(sys.argv[1])

board = []
for i in range(n):
  board.append([(i*n + x) for x in range(n)])

constraintMap = dict([(x, [(x / n, x % n)]) for x in range(n * n)])

# Cruddy union-find algorithm (good enough though)
def possiblyUnion(x, y):
  if x == y:
    return

  # Determine if should union
  probFactor = max(len(constraintMap[x]), len(constraintMap[y]))
  if random.random() > startingUnionProb / probFactor:
    return

  for i,j in constraintMap[y]:
    board[i][j] = x
  constraintMap[x].extend(constraintMap[y])
  constraintMap[y] = []

# Randomly generate constraint blocks
for i in range(n):
  for j in range(n):
    if i + 1 < n:
      possiblyUnion(board[i][j], board[i + 1][j])
    if j + 1 < n:
      possiblyUnion(board[i][j], board[i][j + 1])


# Generating random latin squares is hard
# Simply permuting row, column and values should be random-enough
rowSwaps = [x for x in range(n)]
columnSwaps = [x for x in range(n)]
valueSwaps = [x for x in range(n)]
random.shuffle(rowSwaps)
random.shuffle(columnSwaps)
random.shuffle(valueSwaps)

answer = []
for i in range(n):
  answer.append([-1 for x in range(n)])

for i in range(n):
  answer.append([])
  for j in range(n):
    answer[rowSwaps[i]][columnSwaps[j]] = valueSwaps[(i + j) % n] + 1


# Generate block constraint output
constraints = []
for k, v in constraintMap.iteritems():
  if len(v) == 0:
    continue

  cells = " ".join(map(lambda x: str(x[0]) + "," + str(x[1]), v))
  v = map(lambda x: answer[x[0]][x[1]], v)
  type = ""
  num = -1

  if len(v) == 1:
    type = "!"
    num = v[0]
  elif len(v) == 2 and random.random() > addTwoCellsProb:
    if max(v[0], v[1]) % min(v[0], v[1]) == 0:
      type = "/"
      num = max(v[0], v[1]) / min(v[0], v[1])
    else:
      type = "-"
      num = abs(v[0] - v[1])
  elif random.random() < multProb:
    type = "x"
    num = reduce(lambda x,y: x * y, v, 1)
  else:
    type = "+"
    num = reduce(lambda x,y: x + y, v, 0)

  constraints.append(type + " " + str(num) + " " + cells)

# Output generated result to file
f = open(sys.argv[2], "w")
f.write(str(n) + "\n" + str(len(constraints)) + "\n")
f.write("\n".join(constraints) + "\n")
for i in range(n):
  f.write(" ".join(map(lambda x: str(x), answer[i])) + "\n")
f.close()

