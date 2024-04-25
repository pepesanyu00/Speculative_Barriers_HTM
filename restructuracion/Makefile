CFLAGS = -O2 -Wall -std=c++11 -march=native
LFLAGS = -lm -fopenmp

all: specScampTilesDiag

specScampTilesDiag: specScampTilesDiag.cpp barriers.c barriers.h 
	g++ $(CFLAGS) specScampTilesDiag.cpp rtmIntel.c  -o specScampTilesDiag $(LFLAGS) -mrtm


clean:
	rm -f specScampTilesDiag
	rm -f stats/*