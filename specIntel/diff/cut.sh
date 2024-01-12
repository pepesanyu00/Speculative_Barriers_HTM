#!/bin/bash

file1="scamp_e0103_n180000_w500_t1_d1_105765.csv"
file2="scamp-tiles-nopriv_e0103_n180000_w500_l10000_t1_d1_112629.csv"
len=$( wc -l < $file1)

file1tmp="scamp_e0103_n180000_w500_t1_d1_105765.csv.tmp"
file2tmp="scamp-tiles-nopriv_e0103_n180000_w500_l10000_t1_d1_112629.csv.tmp"

cp $file1 $file1tmp
cp $file2 $file2tmp

for (( i=0; i< $len; i=$i+10000 )); do
  head -n 10000 $file1tmp > ./cut/"$file1tmp"_$i
  tail -n $( expr $len - $i - 10000 ) $file1 > $file1tmp
  
  head -n 10000 $file2tmp > ./cut/"$file2tmp"_$i
  tail -n $( expr $len - $i - 10000 ) $file2 > $file2tmp
done

rm $file1tmp
rm $file2tmp
