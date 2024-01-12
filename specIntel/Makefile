CFLAGS = -O2 -Wall -std=c++11
LFLAGS = -lm -fopenmp

all: scamp scampTilesUnprot scampTilesDiag specScampTilesDiag

scamp: scamp.cpp
	g++ $(CFLAGS) scamp.cpp -o scamp $(LFLAGS)


scampTilesUnprot: scampTilesUnprot.cpp
	g++ $(CFLAGS) scampTilesUnprot.cpp -o scampTilesUnprot $(LFLAGS)


scampTilesDiag: scampTilesDiag.cpp
	g++ $(CFLAGS) scampTilesDiag.cpp -o scampTilesDiag $(LFLAGS)

specScampTilesDiag: specScampTilesDiag.cpp
	g++ $(CFLAGS) specScampTilesDiag.cpp -o specScampTilesDiag $(LFLAGS) -DSB
#	g++ $(CFLAGS) specScampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS) -DCS

clean:
	rm -f scamp scampTilesUnprot scampTilesDiag specScampTilesDiag
