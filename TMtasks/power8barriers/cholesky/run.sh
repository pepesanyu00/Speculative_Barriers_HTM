#!/bin/bash
bench="cholesky"

exec=("UNP"
      "PAR"
      "TM"
      "SB"
      "CS")

inputs=("-n 100000000 -c 1"
        "-n 100000000 -c 5"
        "-n 100000000 -c 10"
        "-n 100000000 -c 15")

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

