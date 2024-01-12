CFLAGS = -O2 -Wall -std=c++11
LFLAGS = -lm

all: diff

diff: diff.cpp
	g++ $(CFLAGS) diff.cpp -o diff $(LFLAGS)

clean:
	rm -f diff
