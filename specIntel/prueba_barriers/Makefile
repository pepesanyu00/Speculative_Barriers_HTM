CFLAGS = -O2 -Wall -std=c++11 -march=native
LFLAGS = -lm -fopenmp

all: main

main:
	g++ $(CFLAGS) main.cpp -o main $(LFLAGS)

clean:
	rm -f main
