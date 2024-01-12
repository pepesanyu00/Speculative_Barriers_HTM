#!/bin/bash

#Para que funciones el proc_bind(close) de #pragma omp parallel
#hay que definir esta variable de entorno
# threads: Each place contains a hardware thread.
# cores: Each place contains a core. If OMP_PLACES is not set, the default setting is cores.
# num_places: Is the number of places.
export OMP_PLACES=cores

#Series cortas
benchs=("./timeseries/audio-MPIII-SVD.txt 200"
        "./timeseries/e0103_n180000.txt 500"
        "./timeseries/human_activity-MPIII-SVC.txt 120"
        "./timeseries/penguin_sample_TutorialMPweb.txt 800"
        "./timeseries/power-MPIII-SVF_n180000.txt 1325"
        "./timeseries/seismology-MPIII-SVE_n180000.txt 50"
        "./timeseries/e0103.txt 500"
        "./timeseries/power-MPIII-SVF.txt 1325"
        "./timeseries/seismology-MPIII-SVE.txt 50")

for i in "${benchs[@]}"; do
  echo "## $i"
  #./scamp serie #th dumpStats
  ./scampUpdateCount $i 1 0
done;

