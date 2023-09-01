#!/bin/bash
bench="iwomp"

exec=("TM")

inputs=("-n 100000 -l 100"
        "-n 100000 -l 1000"
        "-n 100000 -l 10000")

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

