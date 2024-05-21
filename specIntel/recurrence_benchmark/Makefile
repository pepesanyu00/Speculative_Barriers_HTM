CFLAGS = -O2 -Wall -std=c++11 -march=native
LFLAGS = -lm -fopenmp

all: recurrence sin_trans

recurrence: recurrence_OMP.c lib/barriers.c lib/stats.c types.h
	g++  $(CFLAGS)  recurrence_OMP.c lib/barriers.c lib/stats.c -o recurrence $(LFLAGS) -mrtm

sin_trans: sin_trans.c types.h
	g++  $(CFLAGS) sin_trans.c -o sin_trans $(LFLAGS) -mrtm


copy: debug_ddd.c lib/barriers.c lib/stats.c
	g++ -g $(CFLAGS) debug_ddd.c lib/barriers.c lib/stats.c -o debug $(LFLAGS) -mrtm

recurrence_thread: recurrence.c lib/barriers.c lib/stats.c lib/thread.c lib/memory.c types.h
	g++  $(CFLAGS)  recurrence.c lib/barriers.c lib/stats.c lib/thread.c lib/memory.c -o rec_thread $(LFLAGS) -mrtm

clean:
	rm -f recurrence
	rm -f stats/*
	rm -f debug