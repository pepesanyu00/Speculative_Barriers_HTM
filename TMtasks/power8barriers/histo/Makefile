# (c) 2007 The Board of Trustees of the University of Illinois.
LANGUAGE=c
SRCDIR_OBJS=main.o util.o
APP_CFLAGS=-fopenmp
APP_LDFLAGS=-lgomp

hist-omp: main-omp.c util.c parboil.c
	gcc -Wall main-omp.c util.c parboil.c -o hist-omp -O3 -lm -fopenmp 

