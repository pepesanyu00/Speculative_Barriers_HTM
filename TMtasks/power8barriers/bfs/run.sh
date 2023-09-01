#!/bin/bash
bench="bfs"

exec=("TM"
      "SB")
inputs=("inputs/1M_graph_input.dat"
        "inputs/NY_graph_input.dat")

threads="1 2 4 8 16 32 64 128"

for j in {1..20}; do
  echo "######################################################"
  echo "###################### Loop $j ######################"
  echo "######################################################"
  for i in "${inputs[@]}"; do
    for th in $threads; do
      for e in "${exec[@]}"; do
        echo "## $j ##./$bench""_$e -i $i -o bfs.out $th"
        ./$bench"_"$e -i $i -o bfs.out $th
      done;
    done;
    echo "## $j ## ./$bench""_SEQ -i $i -o bfs.out 1"
    ./$bench"_SEQ" -i $i -o bfs.out 1;
  done;
done;

