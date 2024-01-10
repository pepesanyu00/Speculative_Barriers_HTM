#!/usr/bin/python3.8
# -*- coding: utf-8 -*-
import sys
import os #Para quitar directorios y extensiones de un path
import glob #Para obtner una lista de archivos de un path con wildcards
import numpy as np #Package for scientific computing with Python
import matplotlib.pyplot as plt #Librería gráfica
from subprocess import call #Para llamar a un comando del shell

sys.path.append('/home/quislant/Ricardo/Research/myPyLib')
import parseStatsHaswell as psf

titlesDict = {"audio-MPIII-SVD": "Audio",
              "power-MPIII-SVF_n180000": "Power",
              "seismology-MPIII-SVE_n180000": "Seismology",
              "e0103_n180000": "ECG",
              "penguin_sample_TutorialMPweb": "Penguin",
              "human_activity-MPIII-SVC": "Human activity"}

# Esta función lee los archivos en la lista de archivos que se genera a partir
# de filePath (que tendrá wildcards) y devuelve la media del tiempo de ejecución
def readTimeAvg(filePath):
	fileList = glob.glob(filePath)
	timeAcc = 0.0
	n = len(fileList)
	if n == 0:
		print("No hay archivos %s" % filePath)
		return timeAcc
	for file in fileList:
		f = open(file, 'r')
		tmp = f.readline() # Leo el primer comentario
		timeAcc = timeAcc + float(f.readline()) # Leo el tiempo y lo paso a float
		f.close()
	return timeAcc/len(fileList)

#Parseo los argumentos
if len(sys.argv) == 4:
	tseries = sys.argv[1]
	# Le quito el directorio y el .txt si los tiene
	tseries = os.path.basename(tseries)
	tseries = os.path.splitext(tseries)[0]
	w = int(sys.argv[2])
	legend = int(sys.argv[3])
	print("Plotting results for tseries %s with window size %d and %d legend ..." % (tseries, w, legend))
else:
	print("Uso: %s timeseries windowSize legend" % (sys.argv[0]))
	exit(-1)

direc="../results/"
# [legend label, file]
linesv = [["TM"  , "scampTM_%s_w%d_t%d_x%d_*", "scampTM_%s.txt_%d_%d_%d_*"],]
numThreads = (1,)
xactSize = (1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048)
x = range(1,len(xactSize)+1)

lfs = 20 #33 #label font size
ms = 2   #marker size
markers = ('o', '^', 'v', 's', '*', 'p', 'h', '<', '>', '8', 'H', 'D', 'd', None)
mksize = (ms*6,ms*7,ms*7,ms*6,ms*8,ms*7,ms*6,ms*7,ms*6)
#colors = plt.cm.Set1(range(ini,fin,int((fin-ini)/len(linesv)) ))
setx = plt.colormaps['Paired'] #Obtengo el color map
#colors = set1(np.linspace(0.0,1,len(linesv)+7)) #Eligo los colores (cambiar ini y fin para variar los colores)
colors = setx(np.linspace(0.0,1,8))
#colors = plt.cm.Set1(range(ini,fin,int(-(ini-fin)/8.)))
print(colors)
#En blanco y negro (hago la media)
#colors = [[sum(x)/3.]*3 for x in colors]

#Extraigo el tiempo del secuencial (se tomará para el secuencial el scamp no-vect con 1 hilo)
tSeq = readTimeAvg(direc + ("scamp_%s_w%d_t%d_*"%(tseries,w,1)))
for j in range(len(linesv)):
  timePerTh = []
  commitsTh = []
  abortsTh = []
  fallbacksTh = []
  capAbortsTh = []
  confAbortsTh = []
  expAbortsTh = []
  miscAbortsTh = []
  retriesTh = []
  # Obtengo el tiempo para cada número de hilos
  for d in numThreads:
    for xs in xactSize:
      timePerTh.append(readTimeAvg(direc + (linesv[j][1]%(tseries,w,d,xs))))
      hwstats = psf.StatsAvg(direc, linesv[j][2]%(tseries,w,d,xs), False)
      #hwstats.ParseCommitsAvg()
      #hwstats.ParseAbortsAvg()
      hwstats.ParseAbortsAvg()
      hwstats.ParseCommitsAvg()
      hwstats.ParseFallbacksAvg()
      abortsTh.append(hwstats.GetAbortsAvg())
      commitsTh.append(hwstats.GetCommitsAvg())
      fallbacksTh.append(hwstats.GetFallbacksAvg())
      #retriesTh.append(hwstats.GetRetriesAvg())
      capAbortsTh.append(hwstats.GetCapacityAbortsAvg())
      confAbortsTh.append(hwstats.GetConflictAbortsAvg())
      expAbortsTh.append(hwstats.GetExpSubsAbortsAvg()+hwstats.GetExpAddAbortsAvg())
      miscAbortsTh.append(hwstats.GetDebugAbortsAvg()+hwstats.GetNestedAbortsAvg()+hwstats.GetEaxAbortsAvg())

  #Convierto las listas en arrays para hacer cálculos más fácilmente (np.array)
  timePerTh = np.array(timePerTh)
  commitsTh = np.array(commitsTh)
  fallbacksTh = np.array(fallbacksTh)
  abortsTh = np.array(abortsTh)
  #retriesTh = np.array(retriesTh)
  capAbortsTh = np.array(capAbortsTh)
  confAbortsTh = np.array(confAbortsTh)
  expAbortsTh = np.array(expAbortsTh)
  miscAbortsTh = np.array(miscAbortsTh)
  print(tseries)
  print("Commits: "),
  print(commitsTh)
  print("Fallbacks: "),
  print(fallbacksTh)
  print("Aborts: "),
  print(abortsTh)
  print("Capacity: "),
  print(capAbortsTh)
  print("Conflict: "),
  print(confAbortsTh)
  print("Explicit: "),
  print(expAbortsTh)
  print("Miscellaneous: "),
  print(miscAbortsTh)
  #plt.bar(x, (abortsTh+commitsTh)/(abortsTh[0]+commitsTh[0]),0.8, color=colors[j], label="#tx")
  sumaTh = fallbacksTh + capAbortsTh + confAbortsTh + expAbortsTh + miscAbortsTh #+ commitsTh
  print("Suma: ")
  print(sumaTh)
  plt.bar(x, fallbacksTh/sumaTh,0.8, color=colors[1], edgecolor='gray', label="Fallbacks")
  plt.bar(x, capAbortsTh/sumaTh,0.8, bottom=fallbacksTh/sumaTh, color=colors[0], edgecolor='gray', label="Cap. Abort")
  plt.bar(x, confAbortsTh/sumaTh,0.8, bottom=(capAbortsTh+fallbacksTh)/sumaTh, color=colors[5], edgecolor='gray', label="Conf. Abort")
  plt.bar(x, expAbortsTh/sumaTh,0.8, bottom=(capAbortsTh+fallbacksTh+confAbortsTh)/sumaTh, color=colors[4], edgecolor='gray', label="Exp. Abort")
  plt.bar(x, miscAbortsTh/sumaTh,0.8, bottom=(capAbortsTh+fallbacksTh+confAbortsTh+expAbortsTh)/sumaTh, color=colors[6], edgecolor='gray', label="Misc. Abort")
  #plt.bar(x, commitsTh/sumaTh,0.8, bottom=(capAbortsTh+fallbacksTh+confAbortsTh+expAbortsTh+miscAbortsTh)/sumaTh, color=colors[7], edgecolor='gray', label="Commits")


  plt.plot(x, np.zeros(len(x)), color='w', alpha=0, label=' ')
  plt.plot(x, np.zeros(len(x)), color='w', alpha=0, label=' ')


  plt.plot(x, tSeq/timePerTh, color=colors[2], linewidth=ms, marker=markers[j], markeredgecolor=colors[2], markersize=mksize[j], markeredgewidth=1, label="Speedup")
  plt.plot(x, capAbortsTh/(abortsTh+commitsTh), color=colors[3], linewidth=ms, marker=markers[j+1], markeredgecolor=colors[3], markersize=mksize[j+1], markeredgewidth=1, label="CAR")
    
  #print(fallbacksTh+commitsTh)
  plt.grid(axis='y', linewidth=1, linestyle="-")
  #plt.ylabel('Speedup', fontsize=lfs)
  plt.xlabel('Xact Size', fontsize=lfs)
  plt.title(titlesDict[tseries] + ' m=' + str(w), fontsize=lfs)
  plt.xticks(x,xactSize,fontsize=lfs*0.8,rotation=90)
  plt.yticks(fontsize=lfs*0.8)
  if (legend != 0):
    plt.legend(frameon=False, fontsize=lfs*0.9, ncols=2, columnspacing=0.8, loc="upper left", reverse=True)

ax = plt.gca()
ax.set_aspect(9)
plt.savefig("./scamp_xactSens_%s-w%d.pdf" % (tseries,w), format='pdf',bbox_inches='tight')
plt.savefig("./scamp_xactSens_%s-w%d.png" % (tseries,w), format='png',bbox_inches='tight')
#call("cp ../z_figs/SU.pdf ../../paper-TPDS/figs/", shell=True)
#plt.show()

