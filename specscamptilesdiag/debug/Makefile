CFLAGS = -O2 -Wall -std=c++11
LFLAGS = -lm -fopenmp

all: specScampTilesDiag

specScampTilesDiag: specScampTilesDiag.cpp transaction.c
	g++ $(CFLAGS) specScampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS) -D DEBUG -DSB
#	g++ $(CFLAGS) specScampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS) -DCS

clean:
	rm -f specScampTilesDiag
