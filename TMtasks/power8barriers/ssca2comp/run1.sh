#!/bin/bash
bench="ssca2comp"

exec=("TM")

inputs=("-s 13 -i 1.0 -u 1.0 -l 3 -p 3"
        "-s 14 -i 1.0 -u 1.0 -l 9 -p 9"
        "-s 20 -i 1.0 -u 1.0 -l 3 -p 3")

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

