#!/bin/bash
bench="histo"

exec=("UNP"
      "PAR"
      "TM"
      "SB"
      "CS")

inputs=("1000")
#inputs=("500"
#        "1000"
#        "2000")

threads="1 2 4 8 16 32 64 128"

chunk=1

for j in {1..20}; do
  echo "######################################################"
  echo "###################### Loop $j ######################"
  echo "######################################################"
  for i in "${inputs[@]}"; do
    for th in $threads; do
      for e in "${exec[@]}"; do
        echo "## $j ##./$bench""_$e -i inputs/large_img.bin -o histo.bmp $i $chunk $th"
        ./$bench"_"$e -i inputs/large_img.bin -o histo.bmp $i $chunk $th
      done;
    done;
    echo "## $j ## ./$bench""SEQ -i inputs/large_img.bin -o histo.bmp $i $chunk 1"
    ./$bench"_SEQ" -i inputs/large_img.bin -o histo.bmp $i $chunk 1;
  done;
done;

