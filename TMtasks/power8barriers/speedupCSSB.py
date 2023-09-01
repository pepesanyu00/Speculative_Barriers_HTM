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

# [title, subtitle, 2 speedups o 1,dir, file pattern]
rows = [["Recurren", "C1    ", 2,"./recurrence/results/", "*_%s-n20000-c1-t%s-*"],
        ["        ", "C5    ", 2,"./recurrence/results/", "*_%s-n20000-c5-t%s-*"],
        ["        ", "C10   ", 2,"./recurrence/results/", "*_%s-n20000-c10-t%s-*"],
        ["        ", "C15   ", 2,"./recurrence/results/", "*_%s-n20000-c15-t%s-*"],
        ["Cholesky", "C1    ", 2,"./cholesky/results/", "*_%s-n100000000-c1-t%s-*"],
        ["        ", "C5    ", 2,"./cholesky/results/","*_%s-n100000000-c5-t%s-*"],
        ["        ", "C10   ", 2,"./cholesky/results/", "*_%s-n100000000-c10-t%s-*"],
        ["        ", "C15   ", 2,"./cholesky/results/", "*_%s-n100000000-c15-t%s-*"],
        ["DGCA    ", "N2K A2", 2,"./dgca/results/", "*_%s-n2000-w2-r-t%s-*"],
        ["        ", "N8K A2", 2,"./dgca/results/", "*_%s-n8000-w2-r-t%s-*"],
        ["        " ,"N2K A8", 2,"./dgca/results/", "*_%s-n2000-w8-r-t%s-*"],
        ["        " ,"N8K A8", 2,"./dgca/results/", "*_%s-n8000-w8-r-t%s-*"],
        ["Stencil ", "100   ", 2,"./stencil/results/", "*_%s-512-512-64-100-%s-*"],
        ["        ", "800   ", 2,"./stencil/results/", "*_%s-512-512-64-800-%s-*"],
        ["Histo   ", "C1    ", 2,"./histo/results/", "*_%s-1000-1-%s-large_img.bin-*"],
        ["        ", "C5    ", 2,"./histo/results/", "*_%s-1000-5-%s-large_img.bin-*"],
        ["SSCA2   ", "k1-s14", 1,"./ssca2comp/results/", "*_%s-s14-i1.0-u1.0-l9-p9-t%s-*"],
        ["        " ,"k1-s20", 1,"./ssca2comp/results/", "*_%s-s20-i1.0-u1.0-l3-p3-t%s-*"],
        ["        " ,"k4-s14", 1,"./ssca2cut/results/", "*_%s-s14-i1.0-u1.0-l9-p9-t%s-*"],
        ["        " ,"k4-s20", 1,"./ssca2cut/results/", "*_%s-s20-i1.0-u1.0-l3-p3-t%s-*"]]
numThs = [1,2,4,8,16,32,64,128]

print("Power 8 Speedups (CS over Parallel / SB over TM)")
sys.stdout.write("Bench    Params    ")
for j in range(len(numThs)):
  sys.stdout.write("        %d" % numThs[j])
print("")
print("---------------------------------------------------------------------------------------------------")
gmSuCS = [1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0] #Geomean per thread
gmSuSB = [1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0]
for i in range(len(rows)):
  sys.stdout.write("%s %s    " % (rows[i][0], rows[i][1] ))
  for j in range(len(numThs)):
    if rows[i][2] == 2: #Si es 2 mostramos CS/Par y SB/TM
      tmp = psp8.StatsAvg(rows[i][3], rows[i][4]%("CS",numThs[j]), False)
      tmp.ParseTimeAvg()
      suCS = tmp.GetTimeAvg()
      tmp = psp8.StatsAvg(rows[i][3], rows[i][4]%("PAR",numThs[j]), False)
      tmp.ParseTimeAvg()
      suPAR = tmp.GetTimeAvg()
      tmp = psp8.StatsAvg(rows[i][3], rows[i][4]%("TM",numThs[j]), False)
      tmp.ParseTimeAvg()
      suTM = tmp.GetTimeAvg()
      tmp = psp8.StatsAvg(rows[i][3], rows[i][4]%("SB",numThs[j]), False)
      tmp.ParseTimeAvg()
      suSB = tmp.GetTimeAvg()
      sys.stdout.write("%2.2f/%2.2f  " % ( suPAR/suCS, suTM/suSB) )
      gmSuCS[j] *= suPAR/suCS
      gmSuSB[j] *= suTM/suSB
    else: #Para SSCA sólo mostramos SB/TM
      tmp = psp8.StatsAvg(rows[i][3], rows[i][4]%("TM",numThs[j]), False)
      tmp.ParseTimeAvg()
      suTM = tmp.GetTimeAvg()
      tmp = psp8.StatsAvg(rows[i][3], rows[i][4]%("SB",numThs[j]), False)
      tmp.ParseTimeAvg()
      suSB = tmp.GetTimeAvg()
      sys.stdout.write("  -/%2.2f  " % ( suTM/suSB) )
      gmSuSB[j] *= suTM/suSB
  print(" ")

nCS=(len(rows)-4)*len(numThs) # SSCA2 no tiene CS
nSB=len(rows)*len(numThs)
gmCS = 1.0
gmSB = 1.0
print("GeoMean CS speedup per thread:")
for j in range(len(numThs)):
  sys.stdout.write(" %.2f " % gmSuCS[j]**(1.0/(len(rows)-4)))
  gmCS *= gmSuCS[j]
print("\nGeoMean CS speedup: %.2f" % gmCS**(1.0/nCS))
print("GeoMean SB speedup per thread:")
for j in range(len(numThs)):
  sys.stdout.write(" %.2f " % gmSuSB[j]**(1.0/len(rows)))
  gmSB *= gmSuSB[j]
print("\nGeoMean SB speedup: %.2f" % gmSB**(1.0/nSB))


