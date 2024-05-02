#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> //RIC en solaris no hace falta
#include <string.h>
#include <stdint.h>
#include <omp.h>
#include "lib/barriers.h"
#include "lib/stats.h"
#include "timer.h"

#include <unistd.h>
#include <signal.h>


#define ABS(a) (((a) > 0) ? (a) : -1 * (a))

#define DEF_N 5000
#define DEF_DUMP 0
#define DEF_CHUNK 1
#define DEF_NTH 64
#define DEF_SEED 1
#define DEF_DUMPF "recurrence.dump"
#define DEF_VERBOSE 0
#define DEF_NAME "none"
#define type_t float //double

#define DEBUG_ENABLED_

#ifdef DEBUG_ENABLED
#define DEBUG(...)     \
  printf(__VA_ARGS__); \
  fflush(NULL)
#else /* ! TRACE */
#define DEBUG(...)
#endif

type_t *W;
type_t **B;

typedef struct
{
  int n;
  int dump;
  int chunk;
  int nthreads;
  int seed;
  int verbose;
  char dumpfile[100];
  char name[100];
} params_t;

void initParams(params_t *p, char **argv)
{
  p->n = atoi(argv[1]);
  p->dump = DEF_DUMP;
  p->chunk = DEF_CHUNK;
  p->nthreads = atoi(argv[2]);
  p->seed = DEF_SEED;
  p->verbose = DEF_VERBOSE;
  strncpy(p->dumpfile, DEF_DUMPF, 100);
  strncpy(p->name, DEF_NAME, 100);
}

void printParams(params_t p)
{
  printf("------------------------\n");
  printf("Recurrence (livermore loop 6)\n");
  printf("Nodes:    %d\n", p.n);
  printf("Dump?:    %d\n", p.dump);
  printf("Dumpfile: %s\n", p.dumpfile);
  printf("Chunk:    %d\n", p.chunk);
  printf("Nthreads: %d\n", p.nthreads);
  printf("Seed:     %d\n", p.seed);
  printf("Verbose:  %d\n", p.verbose);
  printf("Name:     %s\n", p.name);
  printf("------------------------\n");
}



/* Display some elements for dead-code elimination */
void dco(params_t p)
{
  int i, rel;
  unsigned int th_seed = p.seed;

  for (i = 0; i < 10; i++)
  {
    rel = (int)rand_r(&th_seed) % p.n;
    fprintf(stderr, "DCO: %lf, %lf\n", W[rel], B[rel][rel]);
  }
}

void initArrays(params_t p)
{
  int i, j;
  float tmp; //RIC
  // Allocate arrays
  W = (type_t *)malloc(p.n * sizeof(type_t));
  assert(W);

  B = (type_t **)malloc(p.n * sizeof(type_t *));
  assert(B);
  for (i = 0; i < p.n; i++)
  {
    B[i] = (type_t *)malloc(p.n * sizeof(type_t));
    assert(B[i]);
  }

  // Populate matrices
  srand(p.seed);
  tmp = 1.0 / (type_t)((rand() % p.n) + 1); /* To avoid inf */
  for (i = 0; i < p.n; i++)
  {
    W[i] = 0.01; /* Same as original kernel */
    for (j = 0; j < p.n; j++)
    {
      B[i][j] = tmp; //RIC 1.0 / (type_t) ((rand() % p.n) + 1); /* To avoid inf */
    }
  }
}


int checkGraph(params_t p)
{
  int i;
  FILE *fp;
  char tmp_c, fname[200];
  type_t tmp_f;

  sprintf(fname, "SEQ-n%d.txt", p.n);
  fp = fopen(fname, "r");
  if (fp == NULL)
  {
    printf("No se puede comprobar el resultado. No se pudo abrir el fichero de dump para n=%d\n", p.n);
    return 1;
  }
  fscanf(fp, "%c\n", &tmp_c);
  for (i = 0; i < p.n; i++)
  {
    fscanf(fp, "%f\n", &tmp_f);
    if (ABS(W[i] - tmp_f) >= 0.000001f)
    {
      printf("W[%d]=%f tmp_f=%f W-tmp = %f\n", i, W[i], tmp_f, ABS(W[i] - tmp_f));
      fclose(fp);
      return 1;
    }
  }
  fclose(fp);
  return 0;
}

//RIC pongo el parámetro como void*

void kernel_Histogram(void *p)
{
  params_t *paramPtr = (params_t *)p;
  int N,chunk,numTh;
  N = paramPtr->n;
  numTh = paramPtr->nthreads;
  chunk = paramPtr->chunk;

#pragma omp parallel
  {
    int k, t, c, limit, tid, start, stop, cstart, cstop;
    TX_DESCRIPTOR_INIT();
    tid = omp_get_thread_num();
    limit = N / numTh;
    //if (N % numTh) limit++; //RIC dependiendo de los valores de entrada puede que algún thread se quede sin trabajo
    start = tid * limit;
    stop = tid == numTh - 1 ? N : start + limit; //RIC al último se le asigna el final del vector
    /* Limit chunk to prevent stale iterations */
    //if (chunk > limit) chunk = limit;
    //RIC
    if (chunk > stop - start)
      chunk = stop - start;

    cstart = 0;
    cstop = cstart + chunk;
    //printf("tid: %d start: %d stop: %d chunk: %d istart: %d istop: %d ichunk: %d\n", tid, start, stop, limit, cstart, cstop, chunk);
    // The actual algorithm
    for (t = 0; t <= N - 2; t++)
    {
      for (k = start; k < stop;)
      {
        //printf("thread: %d (%d to %d). chunk (%d to %d)\n",tid, start,stop, k, k+chunk);
        for (c = cstart; c < cstop; c++)
        {
          if (k < (N - t - 1))
          {
            W[t + k + 1] += B[k][t + k + 1] * W[t];
          }
          k++;
          if (k >= stop)
            break;
        } //TM_STOP(tid);
      }
      SB_BARRIER(tid);
      //#pragma omp barrier
    }LAST_BARRIER(tid);
  }
}

int main(int argc, char **argv)
{
  //RIC para el tiempo
  TIMER_T start;
  TIMER_T stop;

  params_t params;
  
  if (argc < 2)
  {
    printf("Usage: %s <n> <nthreads>\n", argv[0]);
    exit(1);
  }

  initParams(&params,argv);
  printParams(params);

  //RIC inicio las estadísticas para Power 8
  if (!statsFileInit(argc,argv,params.nthreads,MAX_XACT_IDS))
  { //RIC para las estadísticas
    printf("Error abriendo o inicializando el archivo de estadísticas.\n");
    return 0;
  }

  omp_set_num_threads(params.nthreads);
  BARRIER_DESCRIPTOR_INIT(params.nthreads);
  printf("Initializing arrays... ");
  fflush(stdout);
  initArrays(params);
  printf("Done.\n");


  printf("Executing kernel... ");
  fflush(stdout);
/***********************************************************************************/
  char mypid[1024];
    sprintf(mypid, " %d", getpid()); 
  
  TIMER_READ(start);
  //thread_start(kernel_Histogram, (void *)&params);
  kernel_Histogram((void *)&params);
  TIMER_READ(stop);

/***************************************************************************************/
  printf("Done.\n");
  printf("Time = %lf\n", TIMER_DIFF_SECONDS(start, stop));
  if (!checkGraph(params))
  {
    printf("Check was fine!\n");
  }
  else
    printf("Check was wrong!!!!\n");
  fflush(stdout);
  //RIC
  if (!dumpStats(TIMER_DIFF_SECONDS(start, stop)))
    printf("Error volcando las estadísticas.\n");
  
  return 0;
}
