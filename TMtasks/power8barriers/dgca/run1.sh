#!/bin/bash
bench="dgca"

exec=("TM")

inputs=("-n 2000 -w 2 -r"
        "-n 2000 -w 4 -r"
        "-n 2000 -w 8 -r"
        "-n 8000 -w 2 -r"
        "-n 8000 -w 4 -r"
        "-n 8000 -w 8 -r")

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

