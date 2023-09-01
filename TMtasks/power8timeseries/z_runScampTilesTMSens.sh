#!/bin/bash

#Para que funciones el proc_bind(close) de #pragma omp parallel
#hay que definir esta variable de entorno
# threads: Each place contains a hardware thread.
# cores: Each place contains a core. If OMP_PLACES is not set, the default setting is cores.
# num_places: Is the number of places.
export OMP_PLACES=cores

benchs=("./timeseries/power-MPIII-SVF_n180000.txt 1325"
        "./timeseries/audio-MPIII-SVD.txt 200"
        "./timeseries/seismology-MPIII-SVE_n180000.txt 50"
        "./timeseries/e0103_n180000.txt 500"
        "./timeseries/penguin_sample_TutorialMPweb.txt 800"
        "./timeseries/human_activity-MPIII-SVC.txt 120")
 
#hilos="1 2 4 8 16 32 64 128"
#Cambio hilos por tamaño de transacción
#hilos="1 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80 85 90 95 100"
hilos="110 120 130 140 150 160 170 180 190 200 210 220 230 240 250 260 270 280 290 300"

for i in "${benchs[@]}"; do
  #./scamp $i 1 0
  for t in $hilos; do
      echo "## $i $t"
      #RIC una tirada con dump stats a 1
      ./scampTilesTM $i 1000 $t 1 0
  done;
done;
