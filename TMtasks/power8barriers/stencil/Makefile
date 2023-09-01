# (c) 2007 The Board of Trustees of the University of Illinois.

LANGUAGE=c
SRCDIR_OBJS=main.o kernels.o file.o
APP_CFLAGS=-fopenmp
APP_LDFLAGS=-lgomp

stencil: main.c kernels.c file.c parboil.c
	gcc -Wall main.c kernels.c file.c parboil.c -o stencil -O3 -fopenmp

