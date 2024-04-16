#!/bin/bash
make -f Makefile.seq clean
make -f Makefile.seq
make -f Makefile.par clean
make -f Makefile.par
make -f Makefile.unp clean
make -f Makefile.unp
make -f Makefile.tm clean
make -f Makefile.tm
make -f Makefile.sb clean
make -f Makefile.sb
make -f Makefile.ord clean
make -f Makefile.ord
make -f Makefile.cs clean
make -f Makefile.cs
./recurrence_SEQ -d -o SEQ-n10000.txt -n 10000 -c 1 -t 1
./recurrence_SEQ -d -o SEQ-n20000.txt -n 20000 -c 1 -t 1

