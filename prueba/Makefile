all:  pruebaPAD

pruebaPAD: pruebaPAD.c barriers.c
	icpx  pruebaPAD.c barriers.c -o pruebaPAD -mrtm -mtsxldtrk -qopenmp

clean:
	rm  pruebaPAD
