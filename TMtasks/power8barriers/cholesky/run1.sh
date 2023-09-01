#!/bin/bash
bench="cholesky"

exec=("TM")

inputs=("-n 100000000 -c 1"
        "-n 100000000 -c 5"
        "-n 100000000 -c 10"
        "-n 100000000 -c 15")

threads="1"

for j in {1..1}; do
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
  done;
done;

