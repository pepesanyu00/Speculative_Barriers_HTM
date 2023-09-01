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
./histo_TM -i inputs/deflt_img.bin -o histo.bmp 1000 1 1
./histo_TM -i inputs/deflt_img.bin -o histo.bmp 1000 5 1

