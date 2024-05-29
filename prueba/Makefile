all:  pruebaPAD

pruebaPAD: pruebaPAD.c barriers.c
	gcc -O2 pruebaPAD.c barriers.c -o pruebaPAD -mrtm -mtsxldtrk -fopenmp

clean:
	rm  pruebaPAD
