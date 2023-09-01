#!/bin/bash

#exec=("UNP" "PAR" "TM" "SB" "CS")
#exec=("ORD")
exec=("UNPOMP")
#inputs=("-n 10000 -c 1"
#        "-n 10000 -c 5"
#        "-n 10000 -c 10"
#        "-n 10000 -c 15"
#        "-n 20000 -c 1"
#        "-n 20000 -c 5"
#        "-n 20000 -c 10"
#        "-n 20000 -c 15")
inputs=("-n 20000 -c 1")

threads="1 2 4 8 16 32 64 128"

for j in {1..1}; do
  echo "######################################################"
  echo "###################### Loop $j ######################"
  echo "######################################################"
  for i in "${inputs[@]}"; do
    for th in $threads; do
      for e in "${exec[@]}"; do
        echo "## $j ##./recurrence_$e $i -t $th"
        ./recurrence_$e $i -t $th 2> ./perf/recurrence_UNPOMP_n20000_c1_t$th
      done;
    done;
    #echo "## $j ## ./recurrence_SEQ $i -t 1"
    #./recurrence_SEQ $i -t 1;
  done;
done;

