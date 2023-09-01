#!/bin/bash
bench="dgca"

exec=("UNP"
      "PAR"
      "TM"
      "SB"
      "CS")

inputs=("-n 2000 -w 2 -r"
        "-n 2000 -w 4 -r"
        "-n 2000 -w 8 -r"
        "-n 8000 -w 2 -r"
        "-n 8000 -w 4 -r"
        "-n 8000 -w 8 -r")

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

