CFLAGS = -O2 -Wall -std=c++11 -march=native
LFLAGS = -lm -fopenmp

all: specScampTilesDiag scampTilesDiag

specScampTilesDiag: specScampTilesDiag.cpp lib/barriers.c lib/stats.c
	g++ $(CFLAGS) specScampTilesDiag.cpp lib/barriers.c lib/stats.c  -o specScampTilesDiag $(LFLAGS) -mrtm

scampTilesDiag: scampTilesDiag.cpp
	g++ $(CFLAGS) scampTilesDiag.cpp -o scampTilesDiag $(LFLAGS)

clean:
	rm -f specScampTilesDiag
	rm -f stats/*