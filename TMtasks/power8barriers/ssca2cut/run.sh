#!/bin/bash
bench="ssca2cut"

exec=("TM"
      "SB")

inputs=("-s 13 -i 1.0 -u 1.0 -l 3 -p 3"
        "-s 14 -i 1.0 -u 1.0 -l 9 -p 9"
        "-s 20 -i 1.0 -u 1.0 -l 3 -p 3")

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

