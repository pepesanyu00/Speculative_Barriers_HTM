CFLAGS = -O2 -Wall -std=c++11
LFLAGS = -lm -fopenmp

all: scampTilesDiag

scampTilesDiag: scampTilesDiag.cpp transaction.c
#	g++ $(CFLAGS) scampTilesDiag.cpp transaction.c -o scampTilesDiag $(LFLAGS) -DSB
	g++ $(CFLAGS) scampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS) -DCS

clean:
	rm -f specScampTilesDiag
