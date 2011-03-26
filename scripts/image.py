#!/usr/bin/python

import Image, ImageDraw, ImageFont, sys
from kenken import Puzzle

if len(sys.argv) != 3:
  print "Usage: " + sys.argv[0] + " <input> <output>"
  sys.exit(1)

puzzle = Puzzle(sys.argv[1])
n = puzzle.n

# Generate image
cellSize = 100
size = n * cellSize
dotLen = 3

im = Image.new("1", (size, size), "white")
smallFont = ImageFont.truetype("FreeSans.ttf", 16)
largeFont = ImageFont.truetype("FreeSans.ttf", 32)
draw = ImageDraw.Draw(im)


def drawHorizantalDottedLine(x, y, len):
  endX = x + len
  while (x < endX):
    draw.line([x, y, min(x + dotLen, endX), y])
    x += 2 * dotLen
def drawVerticalDottedLine(x, y, len):
  endY = y + len
  while (y < endY):
    draw.line([x, y, x, min(y + dotLen, endY)])
    y += 2 * dotLen


# Draw grid
draw.line([1, 1, size, 1], width=2)
draw.line([1, size - 2, size, size - 2], width=2)
draw.line([1, 1, 1, size], width=2)
draw.line([size - 2, 1, size - 2, size], width=2)
for i in range(1, n):
  drawHorizantalDottedLine(0, i * cellSize, size)
  drawVerticalDottedLine(i * cellSize, 0, size)

# Draw constrained regions
for i in range(n):
  for j in range(n):
    x = j * cellSize
    y = i * cellSize

    if i + 1 < n and not puzzle.inSameConstraint(i, j, i + 1, j):
      draw.line([x, y + cellSize, x + cellSize, y + cellSize], width=2)
    if j + 1 < n and not puzzle.inSameConstraint(i, j, i, j + 1):
      draw.line([x + cellSize, y, x + cellSize, y + cellSize], width=2)

# Draw constraint labels
for constraint in puzzle.constraints:
  cell = constraint.label.cell
  draw.text((cell[1] * cellSize + 5, cell[0] * cellSize + 5), 
            str(constraint.label), font=smallFont)

# Draw answers (if any)
for i in range(n):
  for j in range(n):
    answer = puzzle.getAnswer(i , j)

    if answer == -1:
      continue

    xOffset = 30;
    if answer < 10:
      xOffset += 10;

    draw.text((j * cellSize + xOffset, i * cellSize + 40), str(answer),
              font=largeFont)


del draw

# save created image
im.save(sys.argv[2])

