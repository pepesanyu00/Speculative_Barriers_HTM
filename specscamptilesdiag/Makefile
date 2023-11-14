CFLAGS = -O2 -Wall -std=c++11
LFLAGS = -lm -fopenmp

all: scampTilesDiag

scampTilesDiag: scampTilesDiag.cpp transaction.c
#	g++ $(CFLAGS) specScampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS) -DSB
	g++ $(CFLAGS) specScampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS) -DCS

clean:
	rm -f specScampTilesDiag
