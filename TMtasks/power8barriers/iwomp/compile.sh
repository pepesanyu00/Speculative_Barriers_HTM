#!/bin/bash
make -f Makefile.seq clean
make -f Makefile.seq
make -f Makefile.par clean
make -f Makefile.par
make -f Makefile.tm clean
make -f Makefile.tm
make -f Makefile.sb clean
make -f Makefile.sb
make -f Makefile.cs clean
make -f Makefile.cs

