CFLAGS = -O2 -Wall -std=c++11
LFLAGS = -lm -fopenmp

all: specScampTilesDiag scampTilesDiag scamp scampTilesUnprot

specScampTilesDiag: specScampTilesDiag.cpp transaction.c
	g++ $(CFLAGS) specScampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS) -DSB
#	g++ $(CFLAGS) specScampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS) -DCS

scampTilesDiag: scampTilesDiag.cpp
	g++ $(CFLAGS) scampTilesDiag.cpp -o scampTilesDiag $(LFLAGS)

scamp: scamp.cpp
	g++ $(CFLAGS) scamp.cpp -o scamp $(LFLAGS)

scampTilesUnprot: scampTilesUnprot.cpp
	g++ $(CFLAGS) scampTilesUnprot.cpp -o scampTilesUnprot $(LFLAGS)

clean:
	rm -f scampTilesDiag specScampTilesDiag scamp scampTilesUnprot
