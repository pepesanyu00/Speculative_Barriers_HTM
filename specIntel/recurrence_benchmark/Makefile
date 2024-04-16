CFLAGS = -O2 -Wall -std=c++11 -march=native
LFLAGS = -lm

all: recurrence

recurrence: recurrence.c rtmIntel.h rtmIntel.c
	g++ $(CFLAGS) recurrence.cpp rtmIntel.c -o recurrence $(LFLAGS) -mrtm -DSB


clean:
	rm -f recurrence