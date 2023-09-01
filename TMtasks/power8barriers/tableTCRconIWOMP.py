#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
# Importo la clase que lee los ficheros de estadísticas de gems
sys.path.append('/home/quislant/Ricardo/Research/myPyLib/')
import numpy as np #Fundamental package for scientific computing with Python
import matplotlib.pyplot as plt #Librería gráfica
#Importo la clase StatsFile de mi librería de parsing
import parseStatsPower8 as psp8
import sys #Para imprimir sin que me meta espacios o newlines

dirs = ["./iwomp/results/",
        "./recurrence/results/",
        "./cholesky/results/",
        "./dgca/results/",
        "./ssca2comp/results/",
        "./ssca2cut/results/",]

titles = ["Barr","Recurren", "Cholesky", "DGCA", "SSCA2"]
# [subtitle, dir, file pattern]
rows = [["L1K", "./iwomp/results/", "*_%s-n100000-l1000-t%s-*"],
        ["L10K", "./iwomp/results/", "*_%s-n100000-l10000-t%s-*"],
        ["L1K", "./iwomp/results/", "*_%s-n100000-l1000-t%s-*"],
        ["L10K", "./iwomp/results/", "*_%s-n100000-l10000-t%s-*"],
        ["C1", "./recurrence/results/", "*_%s-n20000-c1-t%s-*"],
        ["C5", "./recurrence/results/", "*_%s-n20000-c5-t%s-*"],
        ["C10", "./recurrence/results/", "*_%s-n20000-c10-t%s-*"],
        ["C15", "./recurrence/results/", "*_%s-n20000-c15-t%s-*"],
        ["C1", "./cholesky/results/", "*_%s-n100000000-c1-t%s-*"],
        ["C5", "./cholesky/results/","*_%s-n100000000-c5-t%s-*"],
        ["C10", "./cholesky/results/", "*_%s-n100000000-c10-t%s-*"],
        ["C15", "./cholesky/results/", "*_%s-n100000000-c15-t%s-*"],
        ["N2K A2", "./dgca/results/", "*_%s-n2000-w2-r-t%s-*"],
        ["N8K A2", "./dgca/results/", "*_%s-n8000-w2-r-t%s-*"],
        ["N2K A8", "./dgca/results/", "*_%s-n2000-w8-r-t%s-*"],
        ["N8K A8", "./dgca/results/", "*_%s-n8000-w8-r-t%s-*"],
        ["k1-s14", "./ssca2comp/results/", "*_%s-s14-i1.0-u1.0-l9-p9-t%s-*"],
        ["k1-s20", "./ssca2comp/results/", "*_%s-s20-i1.0-u1.0-l3-p3-t%s-*"],
        ["k4-s14", "./ssca2cut/results/", "*_%s-s14-i1.0-u1.0-l9-p9-t%s-*"],
        ["k4-s20", "./ssca2cut/results/", "*_%s-s20-i1.0-u1.0-l3-p3-t%s-*"]]
numThs = [16,]
pattern = ["TM", "SB", "CS"]

print("\\hline")
print("\\multicolumn{2}{|c|}{\\multirow{2}{*}{\\textbf{Bench}}} & \\multicolumn{3}{c|}{\\textbf{TCR/SCR}} \\\\")
print("\\hhline{|~~|---|}")
print("\\multicolumn{2}{|c|}{} & TM  &  SB  &  CS  \\\\")
print("\\hline \\hline")
for i in range(len(rows)):
  if (i % 4) == 0:
    print("\\multirow{4}{*}{\\rotatebox[origin=c]{90}{%s}} & %s " % (titles[i/4], rows[i][0]))
  else:
    print(" & %s " % rows[i][0]),
  for j in numThs:
    for k in range(len(pattern)):
      sys.stdout.write(" & ")
      if i > 15 and k == 2: #Si estamos con SSCA2 no hay CS
        sys.stdout.write("-")
        break
      tmp = psp8.StatsAvg(rows[i][1], rows[i][2]%(pattern[k],j), False)
      tmp.ParseAbortsAvg()
      tmp.ParseCommitsAvg()
      commits = tmp.GetCommitsAvg()
      aborts = tmp.GetAbortsAvg()
      commitsSB = tmp.GetCommitsSBAvg()
      abortsSB = tmp.GetAbortsSBAvg()
      #print("commits: %d aborts: %d" % (xacts, aborts))
      #print("\n**%f %f %f***" % (xacts, aborts, specBars))
      if(pattern[k] == "TM"):
        sys.stdout.write("%2.2f/-" % ( 100*commits / (commits + aborts)) )
      else:
        if(pattern[k] == "CS"):
          sys.stdout.write("-/%2.2f" % ( 100*commitsSB / (commitsSB + abortsSB)) )
        else:
          sys.stdout.write("%2.2f/%2.2f" % ( 100*(commits-commitsSB) / ((commits-commitsSB) + (aborts-abortsSB)), 100*commitsSB / (commitsSB + abortsSB)) )
        
  print(" \\\\")
  if (i % 4) == 3:
    print("\\hline") 
  else:
    print("\\hhline{|~|----|}")


