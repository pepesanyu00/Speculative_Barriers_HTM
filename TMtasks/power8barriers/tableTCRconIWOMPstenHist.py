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

# [title, rows, subtitle, barriers, xact size, dir, file pattern]
rows = [["Barr"    , 2, "N=100K L=1K", "100K", "5/4.5", "./iwomp/results/", "*_%s-n100000-l1000-t%s-*"],
        [""        , 0, "N=100K L=10K", "100K", "5/4.5", "./iwomp/results/", "*_%s-n100000-l10000-t%s-*"],
        ["Recurren", 4, "N=20K C=1", "20K", "9/6.5", "./recurrence/results/", "*_%s-n20000-c1-t%s-*"],
        [""        , 0, "N=20K C=5", "20K", "16/8.9", "./recurrence/results/", "*_%s-n20000-c5-t%s-*"],
        [""        , 0, "N=20K C=10", "20K", "21/11.8", "./recurrence/results/", "*_%s-n20000-c10-t%s-*"],
        [""        , 0, "N=20K C=15", "20K", "26/14.8", "./recurrence/results/", "*_%s-n20000-c15-t%s-*"],
        ["Cholesky", 4, "N=100M C=1", "27", "8/7.2", "./cholesky/results/", "*_%s-n100000000-c1-t%s-*"],
        [""        , 0, "N=100M C=5", "27", "10/8.5", "./cholesky/results/","*_%s-n100000000-c5-t%s-*"],
        [""        , 0, "N=100M C=10", "27", "12/10", "./cholesky/results/", "*_%s-n100000000-c10-t%s-*"],
        [""        , 0, "N=100M C=15", "27", "12/11.6", "./cholesky/results/", "*_%s-n100000000-c15-t%s-*"],
        ["DGCA"    , 4, "N=2K A=2 M=4K", "4K", "20/18.1", "./dgca/results/", "*_%s-n2000-w2-r-t%s-*"],
        [""        , 0, "N=8K A=2 M=4K", "4K", "20/18.1", "./dgca/results/", "*_%s-n8000-w2-r-t%s-*"],
        [""        , 0, "N=2K A=8 M=4K", "4K", "27/25.1", "./dgca/results/", "*_%s-n2000-w8-r-t%s-*"],
        [""        , 0, "N=8K A=8 M=4K", "4K", "26/24.6", "./dgca/results/", "*_%s-n8000-w8-r-t%s-*"],
        ["Sten"    , 2, "512x512x64 T=100", "101", "12/11.1", "./stencil/results/", "*_%s-512-512-64-100-%s-*"],
        [""        , 0, "512x512x64 T=800", "801", "12/11.1", "./stencil/results/", "*_%s-512-512-64-800-%s-*"],
        ["Hist"    , 2, "Large T=1000 C=1", "2001", "6/5.9", "./histo/results/", "*_%s-1000-1-%s-large_img.bin-*"],
        [""        , 0, "Large T=1000 C=5", "2001", "15/13.2", "./histo/results/", "*_%s-1000-5-%s-large_img.bin-*"],
        ["SSCA2"   , 4, "kernel1 -s14-l9-p9", "10", "8/8", "./ssca2comp/results/", "*_%s-s14-i1.0-u1.0-l9-p9-t%s-*"],
        [""        , 0, "kernel1 -s20-l3-p3", "10", "8/8", "./ssca2comp/results/", "*_%s-s20-i1.0-u1.0-l3-p3-t%s-*"],
        [""        , 0, "kernel4 -s14-l9-p9", "24K", "6/6",  "./ssca2cut/results/", "*_%s-s14-i1.0-u1.0-l9-p9-t%s-*"],
        [""        , 0, "kernel4 -s20-l3-p3", "1572K", "6/6",  "./ssca2cut/results/", "*_%s-s20-i1.0-u1.0-l3-p3-t%s-*"]]
numThs = [16,]
pattern = ["TM", "SB", "CS"]

print("\\hline")
print("\\multicolumn{2}{|c|}{\multirow{2}{*}{\\textbf{Benchmark}}} & \\multirow{2}{*}{\\textbf{Barriers}} & \\textbf{Xact Size} & \\multicolumn{3}{c|}{\\textbf{TCR (\\%)/SCR (\\%)}} \\\\")
print("\\hhline{|~~~~|---|}")
print("\\multicolumn{2}{|c|}{} &  & \\textbf{MAX/AVG} &  TM  &  SB  &  CS  \\\\")
print("\\hline "),
for i in range(len(rows)):
  if (rows[i][1] != 0):
    print("\\hline")
    print("\\multirow{%d}{*}{\\rotatebox[origin=c]{90}{%s}} & %s & %s & %s " % (rows[i][1], rows[i][0], rows[i][2], rows[i][3], rows[i][4]))
  else:
    print("\\hhline{|~|------|}")
    print(" & %s & %s & %s " % (rows[i][2], rows[i][3], rows[i][4])),
  for j in numThs:
    for k in range(len(pattern)):
      sys.stdout.write(" & ")
      if i >= len(rows)-4 and k == 2: #Si estamos con SSCA2 no hay CS
        sys.stdout.write("-")
        break
      tmp = psp8.StatsAvg(rows[i][5], rows[i][6]%(pattern[k],j), False)
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
print("\\hline") 


