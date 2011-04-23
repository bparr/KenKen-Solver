#!/usr/bin/python

import math, re, sys

files = [27, 33, 35, 39, 75]
nodeRegex = re.compile('Nodes Visited\: ([\d]+)\n')
compRegex = re.compile('Computation Time = ([\d\.]+) millisecs\n')
totalRegex = re.compile('      Total Time = ([\d\.]+) millisecs\n')

def parseFile(filename, nodes, compTimes, totalTimes):
  f = open(filename, "r")
  for line in f.readlines():
    m = nodeRegex.match(line)
    if m:
      nodes.append(m.group(1))
      continue

    m = compRegex.match(line)
    if m:
      compTimes.append(m.group(1))
      continue

    m = totalRegex.match(line)
    if m:
      totalTimes.append(m.group(1))
      continue

def printData(nodes, compTimes, totalTimes):
  print "Processors,Nodes,Computational Time,Total Time"
  processors = [1,2,4,8,16,24,32]

  for (processor,node,compTime,totalTime) in zip(processors, nodes, compTimes, totalTimes):
    print str(processor) + "," + node + "," + compTime + "," + totalTime
  print ""


for file in files:
  nodes = []
  compTimes = []
  totalTimes = []
  fileStr = str(file)
  parseFile("../blacklight/serial" + fileStr, nodes, compTimes, totalTimes)
  parseFile("../blacklight/parallel" + fileStr, nodes, compTimes, totalTimes)
  print "Blacklight " + fileStr
  printData(nodes, compTimes, totalTimes)

for file in files:
  nodes = []
  compTimes = []
  totalTimes = []
  fileStr = str(file)
  parseFile("../pople/serial" + fileStr, nodes, compTimes, totalTimes)
  parseFile("../pople/parallel" + fileStr, nodes, compTimes, totalTimes)
  print "Pople " + fileStr
  printData(nodes, compTimes, totalTimes)


