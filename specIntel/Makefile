CFLAGS = -O2 -Wall -std=c++11 -march=native
LFLAGS = -lm -fopenmp

all: scamp scampTilesUnprot scampTilesDiag specScampTilesDiag

scamp: scamp.cpp
	g++ $(CFLAGS) scamp.cpp -o scamp $(LFLAGS)


scampTilesUnprot: scampTilesUnprot.cpp
	g++ $(CFLAGS) scampTilesUnprot.cpp -o scampTilesUnprot $(LFLAGS)


scampTilesDiag: scampTilesDiag.cpp
	g++ $(CFLAGS) scampTilesDiag.cpp -o scampTilesDiag $(LFLAGS)

specScampTilesDiag: specScampTilesDiag.cpp rtmIntel.c rtmIntel.h 
#	g++ $(CFLAGS) specScampTilesDiag.cpp rtmIntel.c  -o specScampTilesDiag $(LFLAGS) -mrtm -DSB
	g++ $(CFLAGS) specScampTilesDiag.cpp rtmIntel.c -o specScampTilesDiag $(LFLAGS) -mrtm -DCS

debug: specScampTilesDiag.cpp rtmIntel.c rtmIntel.h
	g++ $(CFLAGS) specScampTilesDiag.cpp rtmIntel.c -o specScampTilesDiag $(LFLAGS) -mrtm -g -DSB


clean:
	rm -f scamp scampTilesUnprot scampTilesDiag specScampTilesDiag
