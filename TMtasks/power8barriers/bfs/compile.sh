#!/bin/bash
make -f Makefile.seq clean
make -f Makefile.seq
make -f Makefile.tm clean
make -f Makefile.tm
make -f Makefile.sb clean
make -f Makefile.sb

