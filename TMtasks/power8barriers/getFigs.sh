#!/bin/bash

echo "#########################################################################"
echo "Processing Cholesky..."
echo "#########################################################################"
cd cholesky/results/z_py
./plotSUcholesky-n100M.py
cd ../../../
cp cholesky/results/z_figs/SUcholeskyP8-n100M.pdf ./figs

echo "#########################################################################"
echo "Processing DGCA..."
echo "#########################################################################"
cd dgca/results/z_py
./plotSUdgca-nw.py
cd ../../../
cp dgca/results/z_figs/SUdgcaP8-nw.pdf ./figs

echo "#########################################################################"
echo "Processing Recurrence..."
echo "#########################################################################"
cd recurrence/results/z_py
./plotSUrecurrence-n10000.py
./plotSUrecurrence-n20000.py
cd ../../../
cp recurrence/results/z_figs/SUrecurrenceP8-n10000.pdf ./figs
cp recurrence/results/z_figs/SUrecurrenceP8-n20000.pdf ./figs

echo "#########################################################################"
echo "Processing IWOMP..."
echo "#########################################################################"
cd iwomp/results/z_py
./plotSUiwomp.py
cd ../../../
cp iwomp/results/z_figs/SUiwompP8.pdf ./figs

echo "#########################################################################"
echo "Processing SSCA2comp..."
echo "#########################################################################"
cd ssca2comp/results/z_py
./plotSUssca2comp.py
cd ../../../
cp ssca2comp/results/z_figs/SUssca2compP8.pdf ./figs

echo "#########################################################################"
echo "Processing SSCA2cut..."
echo "#########################################################################"
cd ssca2cut/results/z_py
./plotSUssca2cut.py
cd ../../../
cp ssca2cut/results/z_figs/SUssca2cutP8.pdf ./figs

echo "#########################################################################"
echo "Processing SSCA2..."
echo "#########################################################################"
./plotSUssca2.py

