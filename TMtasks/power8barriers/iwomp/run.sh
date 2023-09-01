#!/bin/bash
bench="iwomp"

exec=("PAR"
      "TM"
      "SB"
      "CS")

inputs=("-n 100000 -l 100"
        "-n 100000 -l 1000"
        "-n 100000 -l 10000")

threads="1 2 4 8 16 32 64 128"

for j in {1..20}; do
  echo "######################################################"
  echo "###################### Loop $j ######################"
  echo "######################################################"
  for i in "${inputs[@]}"; do
    for th in $threads; do
      for e in "${exec[@]}"; do
        echo "## $j ##./$bench""_$e $i -t $th"
        ./$bench"_"$e $i -t $th
      done;
    done;
    echo "## $j ## ./$bench""SEQ $i -t 1"
    ./$bench"_SEQ" $i -t 1;
  done;
done;

