CFLAGS = -O2 -Wall -std=c++11 -march=native
LFLAGS = -lm

all: recurrence

recurrence: recurrence.c rtmIntel.h rtmIntel.c thread.h thread.c memory.h memory.c
	g++ $(CFLAGS) recurrence.c rtmIntel.c thread.c memory.c -o recurrence $(LFLAGS) -mrtm -DSB


clean:
	rm -f recurrence