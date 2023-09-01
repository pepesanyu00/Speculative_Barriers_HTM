#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
# Importo la clase que lee los ficheros de estadísticas de gems
sys.path.append('/home/quislant/Ricardo/Research/myPyLib/')
import numpy as np #Fundamental package for scientific computing with Python
import matplotlib.pyplot as plt #Librería gráfica
#Importo la clase StatsFile de mi librería de parsing
import parseStatsHaswell as psf
from subprocess import call #Para llamar a un comando del shell

direc="../Power8/"
grids=["-x128-y128-z3-n1024-l8", "-x128-y128-z3-n512-l8",]
titles=["x128 y128 z3 n1024", "x128 y128 z3 n512",]
# [synchronization method, label, protocol]
# FB_LTGL Lazy Ticket Global Lock con lemming justo antes de tbegin
# FB_LTGL2 Lemming justo después de label de retorno
# Con la nueva API sólo hay un FB_LTGL
files = [["FB_LTGL", "Power8 Priv Lee", "labyrintHTM_%s-t%d-irandom%s.txt-*"],
         ["FB", "Power8 Std Lee", "labyrintHTM_%s-t%d-irandom%s.txt-*"]]

numThreads = (1, 2, 4, 8, 16, 32, 64, 128)
N = len(numThreads)
ind = np.arange(N)  # the x locations for the groups
width = 0.111       # the width of the bars, para 7 barras, dejo la primera en blanco

#Lista de texturas (hatch - sombreado) repitiendo el carácter se consigue más densidad
patterns = ('-', '++', 'x', '\\\\', '//', '*', 'o', 'O', '.', None)
markers = ('o', 'v', '^', '*', '*', 'p', 'h', '<', '>', '8', 'H', 'D', 'd', None)
lines = ('--', '-.', ':', '-')
dash = ((15, 15), (10,5,2,5), (5,5)) #points of dash, blank, dash, blank
dgc=(0.3, 0.3, 0.3) #dark gray color
lgc=(0.8, 0.8, 0.8) #light gray
blw = 3 #bar linewidth
lfs = 40#33 #label font size
plt.rc('text', usetex=True) #Le digo que el procesador de textos sea latex (se pueden usar fórmulas)
plt.rc('font', family='sans-serif') #Le digo que la fuente por defecto sea sans-serif
plt.rc('figure', autolayout=True) #Para que no se corten las etiquetas de los ejes (calcula el bouding box automáticamente)
ini=256
fin=10
colors = plt.cm.GnBu(range(ini,fin,-(ini-fin)/len(files)))
#En blanco y negro (hago la media)
#colors = [[sum(x)/3.]*3 for x in colors]
for i in range(len(grids)):
  tmp = psf.StatsAvg(direc, (files[0][2]%("Seq",1,grids[i])), True)
  seq = np.array(tmp.GetTimeAvg())
  ax = plt.subplot(1, 2, i+1) #nrows, ncols, plot_number
  #Los diferentes fbacks con retries=5
  for j in range(len(files)):
    fbackPriv = []
    for d in numThreads:
      tmp = psf.StatsAvg(direc, (files[j][2]%(files[j][0],d,grids[i])), False)
      tmp.ParseTimeAvg()
      fbackPriv.append(tmp.GetTimeAvg())
    #Convierto las listas en arrays para hacer cálculos más fácilmente (np.array)
    fbackPriv = np.array(fbackPriv)
    ax.plot(ind, seq/fbackPriv, color=colors[j], label=files[j][1], linewidth=blw,
            marker=markers[j], markeredgecolor=colors[j], markersize=blw*8,
            markeredgewidth=blw)
  #ax = plt.gca()
  ax.spines["right"].set_visible(False)
  ax.spines["top"].set_visible(False)
  #ax.spines["left"].set_linewidth(1)
  #ax.spines["bottom"].set_linewidth(1)
  ax.get_xaxis().tick_bottom()
  ax.get_yaxis().tick_left()
  ax.yaxis.grid(color=lgc, linewidth=1, linestyle="-")
  ax.set_axisbelow(True) #Para que el grid no se muestre por encima de las barras
  #print ax.yaxis.label.get_fontname()
  
  # add some text for labels, title and axes ticks if i == 0:
  if (i == 0):
    ax.set_ylabel('Speedup over Sequential', fontsize=lfs)
  ax.set_xlabel(r'\# Threads', fontsize=lfs)
  ax.set_title(titles[i], fontsize=lfs)
  ax.set_xlim([-0.3, len(numThreads)-0.7])
  ax.set_xticks(ind)
  ax.set_xticklabels(('1', '2', '4', '8', '16', '32', '64', '128'), fontsize=lfs*0.8)
  ymax = np.ceil(max(ax.get_yticks()))
  ymax = ymax + (ymax == 1) # Si ymax es uno le sumo uno
  ax.set_yticks(range(int(ymax)+1))
  ax.set_yticklabels(range(int(ymax)+1), fontsize=lfs*0.8)
  ax.set_aspect(5.8/ymax)
  if i == len(grids)-1: #Si es el último imprimo la leyenda
    lgd = ax.legend(loc="center right", bbox_to_anchor=(1.9, 1), frameon=False, fontsize=lfs, labelspacing=0.15)

#plt.tight_layout()#w_pad=10)#h_pad=-30
#plt.subplots_adjust(hspace=-0.9)
#Pongo la figura más grande, ya que los patrones no se ven afectados por el tamaño
#de las gráficas, y cuantas más se meten en una figura más pequeñas son las gráficas
#(no se cambia el tamaño del patrón, no sé pq)
fig = plt.gcf()
si = fig.get_size_inches()
fig.set_size_inches(si[0]*2, si[1]*2)

plt.savefig("../z_figs/Power128th.pdf", format='pdf', bbox_extra_artists=(lgd,), bbox_inches="tight")
call("cp ../z_figs/Power128th.pdf ../../../../paper-JSUPER/figs/", shell=True)
#plt.show()

