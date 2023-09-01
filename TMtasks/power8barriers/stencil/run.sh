#!/bin/bash
bench="stencil"

exec=("UNP"
      "PAR"
      "TM"
      "SB"
      "CS")
#./stencil_PAR -i inputs/default_512x512x64x100.bin -o default_512x512x64x100_PAR.out 512 512 64 100 32
inputs=("100"
        "200"
        "400"
        "800")

threads="1 2 4 8 16 32 64 128"

for j in {1..20}; do
  echo "######################################################"
  echo "###################### Loop $j ######################"
  echo "######################################################"
  for i in "${inputs[@]}"; do
    for th in $threads; do
      for e in "${exec[@]}"; do
        echo "## $j ##./$bench""_$e -i inputs/default_512x512x64x100.bin -o default_512x512x64.out 512 512 64 $i $th"
        ./$bench"_"$e -i inputs/default_512x512x64x100.bin -o default_512x512x64.out 512 512 64 $i $th
      done;
    done;
    echo "## $j ## ./$bench""SEQ -i inputs/default_512x512x64x100.bin -o default_512x512x64.out 512 512 64 $i 1"
    ./$bench"_SEQ" -i inputs/default_512x512x64x100.bin -o default_512x512x64.out 512 512 64 $i 1;
  done;
done;

