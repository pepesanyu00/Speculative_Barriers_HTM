CFLAGS = -O2 -Wall -std=c++11
LFLAGS = -lm -fopenmp

all: specScampTilesDiag scampTilesDiag

specScampTilesDiag: specScampTilesDiag.cpp transaction.c
	g++ $(CFLAGS) specScampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS) -D DEBUG -DSB
#	g++ $(CFLAGS) specScampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS) -DCS

scampTilesDiag: scampTilesDiag.cpp
	g++ $(CFLAGS) scampTilesDiag.cpp -o scampTilesDiag $(LFLAGS) -D DEBUG

clean:
	rm -f specScampTilesDiag
	rm -f scampTilesDiag
