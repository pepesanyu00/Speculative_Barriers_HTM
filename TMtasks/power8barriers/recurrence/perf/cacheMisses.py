#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
# Importo la clase que lee los ficheros de estadísticas de gems
sys.path.append('/home/quislant/Ricardo/Research/myPyLib/')
import numpy as np #Fundamental package for scientific computing with Python
import matplotlib.pyplot as plt #Librería gráfica
#Importo la clase StatsFile de mi librería de parsing
import parseStatsHaswell as psf
import re #Paquete para expresiones regulares
import locale
from subprocess import call #Para llamar a un comando del shell

direc="../results/"
params=["n20000-c1",]
titles = ["\\textbf{Recurrence N=20000, C=1}",]
# [synchronization method, label, file]
files = [["UNP", "Unprotected", "recurrence_%s-%s-t%d-*"],]
numThreads = (1, 2, 4, 8, 16, 32, 64, 128)
perfFile = "recurrence_UNPOMP_n20000_c1_t%d"
N = len(numThreads)
ind = np.arange(N)  # the x locations for the groups

missesRe = re.compile("\s*([\d,]+)\s+cache-misses\s+\#\s+(\d+\.\d+)\s%.+")
LLCmissesRe = re.compile("\s*([\d,]+)\s+LLC-load-misses\s+\#\s+(\d+\.\d+)%.+")
L1DmissesRe = re.compile("\s*([\d,]+)\s+L1-dcache-load-misses\s+\#\s+(\d+\.\d+)%.+")
misses = []
missesPercent = []
L1Dmisses = []
L1DmissesPercent = []
LLCmisses = []
LLCmissesPercent = []

for i in numThreads:
  f = open((perfFile % i), 'r')
  for line in f:
    m = missesRe.match(line)
    if m:
      misses.append(int(m.group(1).replace(',', '')))
      missesPercent.append(float(m.group(2)))

    m = L1DmissesRe.match(line)
    if m:
      L1Dmisses.append(int(m.group(1).replace(',', '')))
      L1DmissesPercent.append(float(m.group(2)))

    m = LLCmissesRe.match(line)
    if m:
      LLCmisses.append(int(m.group(1).replace(',', '')))
      LLCmissesPercent.append(float(m.group(2)))
      break
  f.close()
    
misses = np.array(misses)
missesPercent = np.array(missesPercent)
L1Dmisses = np.array(L1Dmisses)
L1DmissesPercent = np.array(L1DmissesPercent)
LLCmisses = np.array(LLCmisses)
LLCmissesPercent = np.array(LLCmissesPercent)

print("Misses: %s" % misses)
print("Misses %%: %s" % missesPercent)
print("L1Dmisses: %s" % L1Dmisses)
print("L1Dmisses %%: %s" % L1DmissesPercent)
print("LLCmisses: %s" % LLCmisses)
print("LLCmisses %%: %s" % LLCmissesPercent)

width = 0.111       # the width of the bars, para 7 barras, dejo la primera en blanco

#Lista de texturas (hatch - sombreado) repitiendo el carácter se consigue más densidad
patterns = ('-', '++', 'x', '\\\\', '//', '*', 'o', 'O', '.', None)
markers = ('o', '^', 'v', 's', '*', 'p', 'h', '<', '>', '8', 'H', 'D', 'd', None)
lines = ('--', '-.', ':', '-')
dash = ((15, 15), (10,5,2,5), (5,5)) #points of dash, blank, dash, blank
dgc=(0.3, 0.3, 0.3) #dark gray color
lgc=(0.8, 0.8, 0.8) #light gray
blw = 2 #bar linewidth
lfs = 20 #30#33 #label font size
plt.rc('text', usetex=True) #Le digo que el procesador de textos sea latex (se pueden usar fórmulas)
plt.rc('font', family='sans-serif') #Le digo que la fuente por defecto sea sans-serif
plt.rc('figure', autolayout=True) #Para que no se corten las etiquetas de los ejes (calcula el bouding box automáticamente)
colors = plt.cm.get_cmap('Set1')
colors = [colors(i) for i in np.linspace(0.0, 1.0, 10)]
print(colors)
#colors[-3] = colors[-2]
mksize = (blw*6,blw*7,blw*7,blw*6,blw*8)
#En blanco y negro (hago la media)
#colors = [[sum(x)/3.]*3 for x in colors]
for i in range(len(params)):
  tmp = psf.StatsAvg(direc, (files[0][2]%("SEQ",params[0],1)), True)
  seq = np.array(tmp.GetTimeAvg())
  ax = plt.subplot(1, 1, i+1) #nrows, ncols, plot_number
  for j in range(len(files)):
    fbackPriv = []
    for d in numThreads:
      tmp = psf.StatsAvg(direc, (files[j][2]%(files[j][0],params[i],d)), False)
      tmp.ParseTimeAvg()
      fbackPriv.append(tmp.GetTimeAvg())
    #Convierto las listas en arrays para hacer cálculos más fácilmente (np.array)
    fbackPriv = np.array(fbackPriv)
    ax.plot(ind, seq/fbackPriv, color=colors[1], label="Speedup", linewidth=blw,
            marker=markers[j], markeredgecolor=colors[1], markersize=mksize[j],
            markeredgewidth=1)
    ax.plot(ind, misses/10000000, color=colors[2], label="Cache Misses $x10^{7}$", linewidth=blw,
            marker=markers[j+1], markeredgecolor=colors[2], markersize=mksize[j+1],
            markeredgewidth=1)
    ax.plot(ind, L1Dmisses/10000000, color=colors[3], label="L1D Load Misses $x10^{7}$", linewidth=blw,
            marker=markers[j+2], markeredgecolor=colors[3], markersize=mksize[j+2],
            markeredgewidth=1)
    ax.plot(ind, LLCmisses/1000000, color=colors[4], label="LLC Load Misses $x10^{6}$", linewidth=blw,
            marker=markers[j+3], markeredgecolor=colors[4], markersize=mksize[j+3],
            markeredgewidth=1)


  
  ax.spines["right"].set_visible(False)
  ax.spines["top"].set_visible(False)
  #ax.spines["left"].set_linewidth(1)
  #ax.spines["bottom"].set_linewidth(1)
  ax.get_xaxis().tick_bottom()
  ax.get_yaxis().tick_left()
  ax.yaxis.grid(color=lgc, linewidth=1, linestyle="-")
  ax.set_axisbelow(True) #Para que el grid no se muestre por encima de las barras
  #print ax.yaxis.label.get_fontname()
  
  # add some text for labels, title and axes ticks
  #if i == 0:
  #ax.set_ylabel('Speedup', fontsize=lfs)
  ax.set_xlabel(r'Threads', fontsize=lfs)
  ax.set_title(titles[i], fontsize=lfs)
  ax.set_xlim([-0.3, len(numThreads)-0.7])
  ax.set_xticks(ind)
  ax.set_xticklabels(('1', '2', '4', '8', '16', '32', '64', '128'), fontsize=lfs*0.8)
  #ymax = np.ceil(max(ax.get_yticks()))
  #ymax = ymax + (ymax == 1) # Si ymax es uno le sumo uno
  ymax = 42.0 #Lo pongo fijo para que todas la gráficas tengan el mismo ymax
  ax.set_ylim([0, int(ymax)])
  ax.set_yticks(range(0,int(ymax)+1,5))
  ax.set_yticklabels(range(0,int(ymax)+1,5), fontsize=lfs*0.8)
  #ax.set_aspect(4.5/ymax)
  #if i == 0: #Si es el último imprimo la leyenda
  #lgd=ax.legend(frameon=True, fontsize=lfs, ncol=5, bbox_to_anchor=(4.5, 1.42), columnspacing=1)
  lgd=ax.legend(frameon=True, fontsize=lfs)

#plt.tight_layout()#w_pad=10)#h_pad=-30
#plt.subplots_adjust(hspace=-0.9)
#Pongo la figura más grande, ya que los patrones no se ven afectados por el tamaño
#de las gráficas, y cuantas más se meten en una figura más pequeñas son las gráficas
#(no se cambia el tamaño del patrón, no sé pq)
#fig = plt.gcf()
#si = fig.get_size_inches()
#fig.set_size_inches(si[0], si[1])

plt.savefig("./SUrecurrenceP8-n20000.pdf", format='pdf',bbox_extra_artists=(lgd,), bbox_inches='tight')
plt.savefig("./SUrecurrenceP8-n20000.png", format='png',bbox_extra_artists=(lgd,), bbox_inches='tight')
#call("cp ../z_figs/SU.pdf ../../paper-TPDS/figs/", shell=True)
#plt.show()

