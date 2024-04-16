#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define SIZE 1000000 // Tamaño del arreglo

// Función para sumar elementos en un rango del arreglo
int sumArray(int* arr, int start, int end) {
    int sum = 0;
    for (int i = start; i <= end; ++i) {
        sum += arr[i];
    }
    return sum;
}

int main() {
    int arr[SIZE];
    int totalSum = 0;

    // Inicializar el arreglo con valores
    for (int i = 0; i < SIZE; ++i) {
        arr[i] = i + 1;
    }

    // Suma paralela de los elementos del arreglo
    #pragma omp parallel
    {
        int threadID = omp_get_thread_num();
        int numThreads = omp_get_num_threads();
        int chunkSize = SIZE / numThreads;

        int start = threadID * chunkSize;
        int end = (threadID == numThreads - 1) ? SIZE - 1 : start + chunkSize - 1;

        int localSum = sumArray(arr, start, end);

        // Barrera antes de combinar los resultados parciales
        #pragma omp barrier


            totalSum += localSum;
    }

    printf("Suma total de los elementos del arreglo: %d\n", totalSum);

    return 0;
}
