#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
using namespace std;
#include "tm.h"

#define SIZE 100 // Tamaño del arreglo
#define NTHREADS 2

int main() {
    int arr[SIZE];
    int totalSum = 0;
    
    if (!statsFileInit(NTHREADS,4))
    { //RIC para las estadísticas
        printf("Error abriendo o inicializando el archivo de estadísticas.\n");
        return 0;
    }

    // Inicializar el arreglo con valores
    for (int i = 0; i < SIZE; ++i) {
        arr[i] = i + 1;
    }
    omp_set_num_threads(NTHREADS);
    TM_STARTUP(NTHREADS);

    // Suma paralela de los elementos del arreglo
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        cout << tid << endl;
        TM_THREAD_ENTER();
        int chunkSize = SIZE / NTHREADS;

        int start = tid * chunkSize;
        int end = (tid == NTHREADS - 1) ? SIZE - 1 : start + chunkSize - 1;

        // Calcular suma local en cada hilo
        int localSum = 0;
        for (int i = start; i <= end; ++i) {
            localSum += arr[i];
        }

        // Reducción paralela usando un árbol binario de sumas
        int offset = 1;
        while (offset < NTHREADS) {
            int neighbor = tid + offset;
            if (neighbor < NTHREADS) {
                localSum += localSum; // sumar con el vecino
                offset *= 2; // duplicar el offset
            }TM_BARRIER(tid);
        }TM_LAST_BARRIER(tid);

        // El hilo 0 tendrá la suma total en localSum
        if (tid == 0) {
            totalSum = localSum;
        }
    }

    printf("Suma total de los elementos del arreglo: %d\n", totalSum);

    if(!dumpStats()){
      cout << "Error volcando las estadísticas." << endl;
    }

    return 0;
}