CFLAGS = -O2 -Wall -std=c++11
LFLAGS = -lm -fopenmp

all: scamp scampLocks scampTM scampTilesUnprot scampTiles scampTilesTM scampTilesDiag

scamp: scamp.cpp
	g++ $(CFLAGS) scamp.cpp -o scamp $(LFLAGS)

scampLocks: scampLocks.cpp
	g++ -DFG $(CFLAGS) scampLocks.cpp -o scampFGL $(LFLAGS)
	g++ -DCG $(CFLAGS) scampLocks.cpp -o scampCGL $(LFLAGS)

scampTM: scampTM.cpp rtmIntel.c rtmIntel.h
	g++ $(CFLAGS) scampTM.cpp rtmIntel.c -o scampTM $(LFLAGS) -mrtm

scampTilesUnprot: scampTilesUnprot.cpp
	g++ $(CFLAGS) scampTilesUnprot.cpp -o scampTilesUnprot $(LFLAGS)

scampTiles: scampTiles.cpp
	g++ $(CFLAGS) scampTiles.cpp -o scampTiles $(LFLAGS)

scampTilesTM: scampTilesTM.cpp rtmIntel.c rtmIntel.h
	g++ $(CFLAGS) scampTilesTM.cpp rtmIntel.c -o scampTilesTM $(LFLAGS) -mrtm

scampTilesDiag: scampTilesDiag.cpp
	g++ $(CFLAGS) scampTilesDiag.cpp -o scampTilesDiag $(LFLAGS)

clean:
	rm -f scamp scampFGL scampCGL scampTM scampTilesUnprot scampTiles scampTilesTM scampTilesDiag
