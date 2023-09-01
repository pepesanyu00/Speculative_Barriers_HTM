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
make -f Makefile.cs clean
make -f Makefile.cs
./stencil_TM -i inputs/default_512x512x64x100.bin -o default_512x512x64.out 512 512 64 100 1
./stencil_TM -i inputs/default_512x512x64x100.bin -o default_512x512x64.out 512 512 64 800 1

