CFLAGS = -O2 -Wall -std=c++11 -march=native
LFLAGS = -lm -fopenmp

all: recurrence

recurrence: recurrence_OMP.c rtmIntel.h rtmIntel.c types.h
	g++  $(CFLAGS) recurrence_OMP.c rtmIntel.c -o recurrence $(LFLAGS) -mrtm -DSB

copy: recurrence_OMP_copy.c rtmIntel.h rtmIntel.c types.h
	g++ -g $(CFLAGS) recurrence_OMP_copy.c rtmIntel.c -o recurrence $(LFLAGS) -mrtm -DSB

clean:
	rm -f recurrence
	rm -f stats/*