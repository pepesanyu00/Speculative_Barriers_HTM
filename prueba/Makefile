all: prueba pruebaPAD

prueba: prueba.c barriers.c
	gcc -O2 prueba.c barriers.c -o prueba -mrtm -mtsxldtrk -fopenmp

pruebaPAD: pruebaPAD.c barriers.c
	icpx  pruebaPAD.c barriers.c -o pruebaPAD -mrtm -mtsxldtrk -qopenmp

clean:
	rm prueba pruebaPAD