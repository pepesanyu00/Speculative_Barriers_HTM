CFLAGS = -O2 -Wall -std=c++11
LFLAGS = -lm -fopenmp

all: specScampTilesDiag scampTilesDiag scamp scampTilesUnprot

<<<<<<< HEAD
specScampTilesDiag: specScampTilesDiag.cpp transaction.c
	g++ $(CFLAGS) specScampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS)
=======
specScampTilesDiag: specScampTilesDiag.cpp lib/barriers.c lib/stats.c
	g++ $(CFLAGS) specScampTilesDiag.cpp lib/barriers.c lib/stats.c -o specScampTilesDiag $(LFLAGS)
>>>>>>> c8d388ed91eb0497bc9706df995620f579a16875
#	g++ $(CFLAGS) specScampTilesDiag.cpp transaction.c -o specScampTilesDiag $(LFLAGS) -DCS

scampTilesDiag: scampTilesDiag.cpp
	g++ $(CFLAGS) scampTilesDiag.cpp -o scampTilesDiag $(LFLAGS)

scamp: scamp.cpp
	g++ $(CFLAGS) scamp.cpp -o scamp $(LFLAGS)

scampTilesUnprot: scampTilesUnprot.cpp
	g++ $(CFLAGS) scampTilesUnprot.cpp -o scampTilesUnprot $(LFLAGS)

clean:
	rm -f scampTilesDiag specScampTilesDiag scamp scampTilesUnprot
	rm -f stats/*
	rm -f results/*
