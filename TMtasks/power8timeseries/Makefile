CFLAGS = -O2 -Wall -std=c++11
LFLAGS = -lm -fopenmp -mcpu=power8 -mtune=power8

all: scamp scampLocks scampTM scampTilesUnprot scampTiles scampTilesTM scampTilesDiag

scamp: scamp.cpp
	g++ $(CFLAGS) scamp.cpp -o scamp $(LFLAGS)

scampLocks: scampLocks.cpp
	g++ -DFG $(CFLAGS) scampLocks.cpp -o scampFGL $(LFLAGS)
	g++ -DCG $(CFLAGS) scampLocks.cpp -o scampCGL $(LFLAGS)

scampTM: scampTM.cpp tmPower.cpp tmPower.h
	g++ $(CFLAGS) scampTM.cpp tmPower.cpp -o scampTM $(LFLAGS) -mhtm

scampTilesUnprot: scampTilesUnprot.cpp
	g++ $(CFLAGS) scampTilesUnprot.cpp -o scampTilesUnprot $(LFLAGS)

scampTiles: scampTiles.cpp
	g++ $(CFLAGS) scampTiles.cpp -o scampTiles $(LFLAGS)

scampTilesTM: scampTilesTM.cpp tmPower.cpp tmPower.h
	g++ $(CFLAGS) scampTilesTM.cpp tmPower.cpp -o scampTilesTM $(LFLAGS) -mhtm

scampTilesDiag: scampTilesDiag.cpp
	g++ $(CFLAGS) scampTilesDiag.cpp -o scampTilesDiag $(LFLAGS)

clean:
	rm -f scamp scampFGL scampCGL scampTM scampTilesUnprot scampTiles scampTilesTM scampTilesDiag

