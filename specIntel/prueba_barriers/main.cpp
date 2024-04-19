#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "tm.h"
#include <iostream>

using namespace std;

#define N 100 // Tamaño de las matrices (N x N)
#define DTYPE double        /* DATA TYPE */
#define ITYPE uint64_t /* INDEX TYPE */
#define NTHREADS 2

// Función para inicializar una matriz con valores aleatorios
void initializeMatrix(double* matrix, int size) {
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            matrix[i * size + j] = (double)rand() / RAND_MAX; // valor aleatorio entre 0 y 1
        }
    }
}

// Función para imprimir una matriz
void printMatrix(double* matrix, int size) {
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            printf("%.2f ", matrix[i * size + j]);
        }
        printf("\n");
    }
}

// Función para multiplicar dos matrices (C = A * B)
void multiplyMatrices(double* A, double* B, double* C, int size) {
    double sum = 0.0;
    #pragma omp parallel 
        {
        ITYPE tid = omp_get_thread_num();
        TM_THREAD_ENTER();  
        //cout << "Thread: " << tid << endl;
        #pragma omp for nowait
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                sum = 0.0;
                for (int k = 0; k < size; ++k) {
                    sum += A[i * size + k] * B[k * size + j];
                }
                C[i * size + j] = sum;
            }TM_BARRIER(tid);
        }TM_LAST_BARRIER(tid);
    }
}

int main() {
    double* A = (double*)malloc(N * N * sizeof(double));
    double* B = (double*)malloc(N * N * sizeof(double));
    double* C = (double*)malloc(N * N * sizeof(double));

    if (!statsFileInit(NTHREADS,4))
    { //RIC para las estadísticas
        printf("Error abriendo o inicializando el archivo de estadísticas.\n");
        return 0;
    }

    // Inicializar matrices A y B con valores aleatorios
    initializeMatrix(A, N);
    initializeMatrix(B, N);
    TM_STARTUP(NTHREADS);
    omp_set_num_threads(NTHREADS);

    // Multiplicar matrices A y B para obtener C de forma paralela
    double startTime = omp_get_wtime();
    multiplyMatrices(A, B, C, N);
    double endTime = omp_get_wtime();
    printf("Tiempo de ejecución: %.4f segundos\n", endTime - startTime);

    // Imprimir matrices (descomentar para mostrar)
    // printf("Matriz A:\n");
    // printMatrix(A, N);
    // printf("Matriz B:\n");
    // printMatrix(B, N);
    // printf("Matriz C (resultado de A * B):\n");
    // printMatrix(C, N);
    /*if (!dumpStats())
    printf("Error volcando las estadísticas.\n");*/

    if(!dumpStats()){
      cout << "Error volcando las estadísticas." << endl;
    }

    free(A);
    free(B);
    free(C);

    return 0;
}
