#include <stdio.h>
#include <omp.h>
#include <assert.h>
#include <stdlib.h>
#include <cpuid.h>
#include "barriers.h"

int g_numThreads, g_xLength;

int *g_x;

volatile int g_flag;
volatile int g_flag2;

void check_TSXLDTRK();

void prueba(void)
{

#pragma omp parallel // proc_bind(close)
  {
    int tid = omp_get_thread_num();
    TX_DESCRIPTOR_INIT(); // Declara las variable retries
    long int suma = 0;
    int flag = 0;

    for (int j = 0; j < 10; j++)
    {
#pragma omp for //schedule(dynamic) nowait
      for (int i = 0; i < g_xLength; i++)
      {
        // END_ESCAPE;
        // if (!flag)
        //{
        BEGIN_TRANSACTION(tid, 0);
        //}
        BEGIN_ESCAPE;
        if (tid == 1)
          while (!g_flag)
            ;
        END_ESCAPE;
        suma += g_x[i];
        if (tid == 0)
          g_flag = 1;
        // if (!flag)
        //{
        COMMIT_TRANSACTION(tid, 0);
        //}
        // else
        //{
        //  COMMIT_TRANSACTION(tid, 1);
        //  flag = 0;
        //}
        if (tid == 1)
        {
          g_flag2 = 1;
          g_flag = 0;
        }
        if (tid == 0)
        {
          while (!g_flag2)
            ;
          g_flag2 = 0;
        }
      }
      // BEGIN_TRANSACTION(tid, 1);
      // flag = 1;
      // BEGIN_ESCAPE;
    }
    printf("La suma del thread %d es: %ld\n", tid, suma);
  }
}

int main(int argc, char *argv[])
{
  check_TSXLDTRK();
  if (argc != 3)
  {
    printf("prueba #th array_length\n");
    exit(0);
  }
  g_numThreads = atoi(argv[1]);
  g_xLength = atoi(argv[2]);

  printf("Num threads: %d. X length: %d.\n", g_numThreads, g_xLength);

  omp_set_num_threads(g_numThreads);

  g_flag = 0;
  g_flag2 = 0;

  printf("VA de g_flag: %p\n",&g_flag);
  printf("VA de g_flag2: %p\n",&g_flag2);

  // RIC xCount = 1 -> último parámetro de statsFileInit
  if (!statsFileInit(argc, argv, g_numThreads, 3))
  {
    printf("TM statsFileInit() error.\n");
    return 1;
  }
  g_x = (int *)malloc(sizeof(int) * g_xLength);
  assert(g_x);
  for (int i = 0; i < g_xLength; i++)
    g_x[i] = rand();

  /* ------------------------------------------------------------------ */
  prueba();
  /***********************************************/

  if (!dumpStats(0))
  {
    printf("TM dumpStats() error.\n");
    return 1;
  }
  return 0;
}

void check_TSXLDTRK() {
  unsigned int eax, ebx, ecx, edx;
  //unsigned int bit_TSXLDTRK;  //Bit 16: TSXLDTRK. If 1, the processor supports Intel TSX suspend/resume of load address tracking.

  //bit_TSXLDTRK = 1 << 16;

  //Structured Extended Feature Flags Enumeration Main Leaf (Initial EAX Value = 07H, ECX = 0)
  eax = 7;
  ecx = 0;
  __get_cpuid (0, &eax, &ebx, &ecx, &edx);
  if(ebx & bit_RTM)
    printf("La CPU soporta RTM.\n");
  else
    printf("La CPU NO soporta RTM.\n");
  if (edx & bit_TSXLDTRK)
    printf("La CPU soporta TSXLDTRK.\n");
  else
    printf("La CPU NO soporta TSXLDTRK.\n");
}