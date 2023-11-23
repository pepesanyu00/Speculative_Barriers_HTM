CFLAGS = -O2 -Wall -std=c++11
LFLAGS = -lm -fopenmp

all: specScampTilesDiag scampTilesDiag scamp scampTilesUnprot

specScampTilesDiag: specScampTilesDiag.cpp transaction.c
	g++ $(CFLAGS) specScampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS) -DSB
#	g++ $(CFLAGS) specScampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS) -DCS

scampTilesDiag: scampTilesDiag.cpp transaction.c
	g++ $(CFLAGS) scampTilesDiag.cpp transaction.c -o scampTilesDiag $(LFLAGS)

scamp: scamp.cpp transaction.c
	g++ $(CFLAGS) scamp.cpp transaction.c -o scamp $(LFLAGS)

scampTilesUnprot: scampTilesUnprot.cpp transaction.c
	g++ $(CFLAGS) scampTilesUnprot.cpp transaction.c -o scampTilesUnprot $(LFLAGS)

clean:
	rm -f scampTilesDiag specScampTilesDiag scamp scampTilesUnprot
