CFLAGS = -O2 -Wall -std=c++11 -march=native
LFLAGS = -lm -fopenmp

all: specScampTilesDiag scampTilesDiag scamp scampTilesUnprot

specScampTilesDiag: specScampTilesDiag.cpp lib/barriers.c lib/stats.c
	g++ $(CFLAGS) specScampTilesDiag.cpp lib/barriers.c lib/stats.c  -o specScampTilesDiag $(LFLAGS) -mrtm

scampTilesDiag: scampTilesDiag.cpp
	g++ $(CFLAGS) scampTilesDiag.cpp -o scampTilesDiag $(LFLAGS)

scamp: scamp.cpp
	g++ $(CFLAGS) scamp.cpp -o scamp $(LFLAGS)


scampTilesUnprot: scampTilesUnprot.cpp
	g++ $(CFLAGS) scampTilesUnprot.cpp -o scampTilesUnprot $(LFLAGS)


clean:
	rm -f specScampTilesDiag
	rm -f scampTilesDiag
	rm -f scamp
	rm -f scampTilesUnprot
	rm -f stats/*
	rm -f results/*