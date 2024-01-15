#!/usr/bin/python3
# -*- coding: utf-8 -*-


#./plotScampSpeedup.py audio-MPIII-SVD 200 1
#./plotMem.py e0103_n180000.txt 500 1
#./plotScampSpeedup.py penguin_sample_TutorialMPweb.txt 800 0
#./plotScampSpeedup.py power-MPIII-SVF_n180000 1325 0
#./plotScampSpeedup.py seismology-MPIII-SVE_n180000 50 0
#./plotScampSpeedup.py human_activity-MPIII-SVC.txt 120 0
#### Large series
#./plotScampSpeedupLarge.py e0103 500 1
#./plotMem.py power-MPIII-SVF 1325
#./plotScampSpeedupLarge.py seismology-MPIII-SVE 50 0

import sys
import os #Para quitar directorios y extensiones de un path
import glob #Para obtner una lista de archivos de un path con wildcards
import numpy as np #Package for scientific computing with Python
import matplotlib.pyplot as plt #Librería gráfica
from subprocess import call #Para llamar a un comando del shell

titlesDict = {"e0103": "ECG",
	      			"power-MPIII-SVF": "Power",
	      			"seismology-MPIII-SVE": "Seismology"}

# Esta función lee los archivos en la lista de archivos que se genera a partir
# de filePath (que tendrá wildcards) y devuelve la media del tiempo de ejecución
def readMem(filePath):
	fileList = glob.glob(filePath)
	memBreakdown = []
	n = len(fileList)
	if n == 0:
		print("No hay archivos %s" % filePath)
		return memBreakdown

	for file in fileList:
		f = open(file, 'r')
		tmp = f.readline() # Leo el primer comentario
		tmp = f.readline() # Leo el tiempo
		# Leo el comentario siguiente
		# Mem(KB) tseries,means,norms,df,dg,profile,profileIndex,Total(MB)
		tmp = f.readline()
		tmp = f.readline() # Leo la memoria
		f.close()
		if memBreakdown: # Si ya se instanció compruebo que los demás ficheros contienen lo mismo
			if(memBreakdown != tmp):
				print("El archivo %s contiene valores diferentes a otro experimiento con los mismos parámetros" % file)
				return []
		else:
			memBreakdown = tmp

	return [ float(i) for i in memBreakdown.split(",") ]

tseries = [["e0103", 500],
           ["power-MPIII-SVF", 1325],
	  			 ["seismology-MPIII-SVE", 50]]

direc="../results/"
l=(8192,) # (2048) #1024,) #  16384
#numThreads = (1, 2, 4, 8, 16, 32, 64, 128)

# Mem(KB) tseries,means,norms,df,dg,profile,profileIndex,Total(MB)
print("\\begin{tabular}{|l|c|c|c|c|}")
print("\\hline")
print("  & \\multicolumn{4}{c|}{\\bf Size (MB)} \\\\")
print("\\hhline{~----}")
print("\\bf Time series & \\bf T & \\bf Statistics ($\mu, \sigma, df, dg$) & \\bf P,I & \\bf Total \\\\")
print("\\hline")
print("\\hline")


for i in l:
	# [legend label, file]
	linesv = [["Base"    , "scamp_%s_w%d_t%d_*"],
            ["TM X=128"  , "scampTM_%s_w%d_t%d_x128_*"],
            ["TilesDiag L=" + str(i) , "scampTilesDiag_%s_w%d_l"+str(i)+"_t%d_*"],
            ["TilesCritic L=" + str(i), "scampTiles_%s_w%d_l"+str(i)+"_t%d_*"],
            ["TilesTM L=" + str(i) , "scampTilesTM_%s_w%d_l"+str(i)+"_t%d_x128_*"],]

#            ["TilesUnprot L=" + str(i) , "scampTilesUnprot_%s_w%d_l"+str(i)+"_t%d_*"],

	x = np.array(range(1,len(linesv)+1))
	for k in range(len(tseries)):
		for j in range(len(linesv)):
			memBreak = readMem(direc + linesv[j][1]%(tseries[k][0],tseries[k][1],128))
			print(titlesDict[tseries[k][0]] + " " + linesv[j][0] + " & ", end="")
			print(str(round(memBreak[0]/1024.0,2)) + " & ", end="") #Time series length
			print(str(round((memBreak[1] + memBreak[2] + memBreak[3] + memBreak[4])/1024.0,2)) + " & ", end="") #Statistics length
			print(str(round((memBreak[5] + memBreak[6])/1024.0,2)) + " & ", end="") #Profile length
			print(str(memBreak[7]) + " \\\\ ") #Total length
			print("\hline")
		if k != len(tseries) - 1:
			print("\hline")

print("\\end{tabular}")
