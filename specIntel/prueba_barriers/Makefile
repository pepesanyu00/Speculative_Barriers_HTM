CFLAGS = -O2 -Wall -std=c++11 -march=native
LFLAGS = -lm -fopenmp -mrtm

all: main parallel_reduction

main: main.cpp rtmIntel.c rtmIntel.h
	g++ -g $(CFLAGS) main.cpp rtmIntel.c -o main $(LFLAGS) -DSB

parallel_reduction: parallel_reduction.cpp rtmIntel.c rtmIntel.h
	g++ -g $(CFLAGS) parallel_reduction.cpp rtmIntel.c -o parallel_reduction $(LFLAGS) -DSB

main.s: main.cpp
	g++ -S $(CFLAGS) main.cpp -o main.s $(LFLAGS) -DSB


clean:
	rm -f main
	rm -f parallel_reduction
	rm -f stats/*
