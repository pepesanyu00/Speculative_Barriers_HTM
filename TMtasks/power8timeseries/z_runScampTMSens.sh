#!/bin/bash

#Para que funciones el proc_bind(close) de #pragma omp parallel
#hay que definir esta variable de entorno
# threads: Each place contains a hardware thread.
# cores: Each place contains a core. If OMP_PLACES is not set, the default setting is cores.
# num_places: Is the number of places.
export OMP_PLACES=cores

benchs=("./timeseries/audio-MPIII-SVD.txt 200"
        "./timeseries/e0103_n180000.txt 500"
        "./timeseries/human_activity-MPIII-SVC.txt 120"
        "./timeseries/penguin_sample_TutorialMPweb.txt 800"
        "./timeseries/power-MPIII-SVF_n180000.txt 1325"
        "./timeseries/seismology-MPIII-SVE_n180000.txt 50"
        )

xactSize="1 2 4 8 16 32 64 128 256"
n=4

for j in $(seq 1 $n); do
  echo "### $j ###"
  for i in "${benchs[@]}"; do
    ./scamp $i 1 0
    for t in $xactSize; do
      echo "## $i $t"
      #RIC una tirada con dump stats a 1
      ./scampTM $i 1 $t 0
    done;
  done;
done;
