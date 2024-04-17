CFLAGS = -O2 -Wall -std=c++11 -march=native
LFLAGS = -lm -fopenmp

all: main

main: main.cpp rtmIntel.c rtmIntel.h
	g++ $(CFLAGS) main.cpp rtmIntel.c -o main $(LFLAGS) -mrtm -DSB

clean:
	rm -f main
