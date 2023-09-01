#!/bin/bash

#Borro todos los resultados previos
read -p "Atención! Se van a borrar todos los resultados previos y a recompilar los códigos. ¿Quiere continuar? [s\n] " ans

if [ $ans = "s" ]; then

cd ssca2cut
rm ./results/*
./compile.sh
./run.sh
cd ..

cd ssca2comp
rm ./results/*
./compile.sh
./run.sh
cd ..

cd dgca
rm ./results/*
./compile.sh
./run.sh
cd ..

cd cholesky
rm ./results/*
./compile.sh
./run.sh
cd ..

cd recurrence
rm ./results/*
./compile.sh
./run.sh
cd ..

cd iwomp
rm ./results/*
./compile.sh
./run.sh
cd ..

else
  echo "Adiós!"
fi
