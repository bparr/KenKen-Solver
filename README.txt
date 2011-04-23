15-418 Final Project
Parallelized KenKen Solver
by. Jonathan Park and Ben Parr


Solving puzzles
===============

To compile and run the serial and parallel solvers on a system supporting
OpenMP, run:

make
./serial input_file
./parallel number_of_processors input_file

Examples:
./serial puzzle.txt
./parallel 8 puzzle.txt


Python scripts
==============

The scripts directory contains helpful Python scripts. These scripts were
hacked together; don't hate us for style atrocities.


Generating puzzles
------------------

To generate a random N x N puzzle, run:

./generate.py N output_file

Examples:
./generate.py 3 3x3.txt
./generate.py 10 hard.txt


Checking puzzles
----------------

The output of generate.py contains an answer to the puzzle at the end of the
file, which can be easily replaced. To check that a puzzle file contains a
valid answer, run:

./check.py input_file

Example:
./check.py serialResult.txt


Generating puzzle images
------------------------

The puzzle files are difficult for humans to visualize, so we created a script
to generate an image of the puzzle. The resulting image includes the answer in
the puzzle file, if it exists. By removing the answer from the puzzle file,
you can generate a blank puzzle. To generate a puzzle image, run:

./image.py input_file output_file

Example:
./image.py 3x3.txt 3x3.png


Note: The puzzle image creator requires Python Imaging Library (PIL), found at
http://www.pythonware.com/products/pil/.


Input files
===========

The input directory contains our generated input files, including both small
test puzzles, and the five puzzles we used to measure speedups. These five
files were originally named based on their original serial times, which have
become outdated. However, the files in the blacklight/pople and jobs
directories use these outdated filenames, so we decided not to update the
filenames. In terms of the report, the filename mapping is:

Puzzle 1 = 27minutes.txt
Puzzle 2 = 33minutes.txt
Puzzle 3 = 35minutes.txt
Puzzle 4 = 39minutes.txt
Puzzle 5 = 75minutes.txt


Other directories
=================

jobs - job scripts submitted on both blacklight and pople
blacklight - data from blacklight supercomputer
pople - data from pople supercomputer
reports - documents and data for our milestone and final reports

