# (c) 2010 The Board of Trustees of the University of Illinois.

#LANGUAGE=c
#SRCDIR_OBJS=main.o 

# To use slow (but precise) math routines
#ifeq ($(slowmath), 1)
#	APP_CFLAGS += -fp-model precise
#else
#	APP_CFLAGS += -ffast-math
#	APP_CXXFLAGS += -ffast-math
#endif

#APP_CFLAGS += -fopenmp
#APP_CXXFLAGS += -fopenmp

#APP_LDFLAGS = -lm -lstdc++ -lgomp

# To use the MKL library
#ifeq ($(mkl), 1)
#	APP_LDFLAGS += -L/opt/intel/mkl/8.0/lib/em64t \
#		-lmkl -lvml -lguide -lpthread
#	INCLUDES = -I/opt/intel/mkl/9.1.023/include
#	APP_CFLAGS += -DMKL $(INCLUDES)
#endif

bfs-omp: main-omp.c parboil.c ../lib/queue.c ../lib/random.c ../lib/mt19937ar.c
	gcc -Wall -O3 -DX86=1 main-omp.c parboil.c ../lib/queue.c ../lib/random.c ../lib/mt19937ar.c -o bfs-omp -I ../common -I ../lib -fopenmp


