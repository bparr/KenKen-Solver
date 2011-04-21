#!/usr/bin/python

import math, re, sys

files = [27, 33, 35, 39, 75]
nodeRegex = re.compile('Nodes Visited\: ([\d]+)\n')
compRegex = re.compile('Computation Time = ([\d\.]+) millisecs\n')
totalRegex = re.compile('      Total Time = ([\d\.]+) millisecs\n')

def parseFile(filename):
  node = None
  compTime = None
  totalTime = None

  f = open(filename, "r")
  for line in f.readlines():
    m = nodeRegex.match(line)
    if m:
      node = m.group(1)
      continue

    m = compRegex.match(line)
    if m:
      compTime = m.group(1)
      continue

    m = totalRegex.match(line)
    if m:
      totalTime = m.group(1)
      continue


  print "Number nodes,Computational Times,Total Time"
  print node + "," + compTime + "," + totalTime


for file in files:
  fileStr = str(file)
  print "Blacklight Serial " + fileStr
  parseFile("blacklight/serial" + fileStr)
  print "Blacklight Parallel " + fileStr
  parseFile("blacklight/parallel" + fileStr)

for file in files:
  fileStr = str(file)
  print "Pople Serial " + fileStr
  parseFile("pople/serial" + fileStr)
  print "Pople Parallel " + fileStr
  parseFile("pople/parallel" + fileStr)

