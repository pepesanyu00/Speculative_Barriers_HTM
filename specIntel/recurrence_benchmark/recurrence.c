#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> //RIC en solaris no hace falta
#include <string.h>
#include <stdint.h>
#include "lib/barriers.h"
#include "lib/stats.h"
#include "lib/thread.h"
//RIC paso de timerutils y pongo el de stamp
//#include "timerutils.h"
#include "timer.h"

#define ABS(a) (((a)>0)? (a):-1*(a))

#define DEF_N        1000
#define DEF_DUMP     0
#define DEF_CHUNK    1
#define DEF_NTH      1
#define DEF_SEED     1 
#define DEF_DUMPF    "recurrence.dump"
#define DEF_VERBOSE  0
#define DEF_NAME     "none"
#define type_t       float //double

#define DEBUG_ENABLED_

#ifdef DEBUG_ENABLED
#define DEBUG(...)         printf(__VA_ARGS__); fflush(NULL)            
#else /* ! TRACE */
#define DEBUG(...)
#endif

type_t* W;
type_t** B;

typedef struct {
  int n;
  int dump;
  int chunk;
  int nthreads;
  int seed;
  int verbose;
  char dumpfile[100];
  char name[100];
} params_t;

void initParams(params_t* p,char **argv) {
  p->n = atoi(argv[1]);
  p->dump = DEF_DUMP;
  p->chunk = DEF_CHUNK;
  p->nthreads = atoi(argv[2]);
  p->seed = DEF_SEED;
  p->verbose = DEF_VERBOSE;
  strncpy(p->dumpfile, DEF_DUMPF, 100);
  strncpy(p->name, DEF_NAME, 100);
}

void printParams(params_t p) {
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

void printUsage(void) {
  printf("usage: ./recurrence [-n <nodes>][-d dump][-o <dumpfile>][-c <chunk>][-t <nthreads>][-h help][-v verbose]\n");
}

/* Display some elements for dead-code elimination */
void dco(params_t p) {
  int i, rel;
  unsigned int th_seed = p.seed;

  for (i = 0; i < 10; i++) {
    rel = (int) rand_r(&th_seed) % p.n;
    fprintf(stderr, "DCO: %lf, %lf\n", W[rel], B[rel][rel]);
  }
}

void initArrays(params_t p) {
  int i, j;
  float tmp; //RIC
  // Allocate arrays
  W = (type_t*) malloc(p.n * sizeof (type_t));
  assert(W);

  B = (type_t**) malloc(p.n * sizeof (type_t*));
  assert(B);
  for (i = 0; i < p.n; i++) {
    B[i] = (type_t*) malloc(p.n * sizeof (type_t));
    assert(B[i]);
  }

  // Populate matrices
  srand(p.seed);
  tmp = 1.0 / (type_t) ((rand() % p.n) + 1); /* To avoid inf */
  for (i = 0; i < p.n; i++) {
    W[i] = 0.01; /* Same as original kernel */
    for (j = 0; j < p.n; j++) {
      B[i][j] = tmp; //RIC 1.0 / (type_t) ((rand() % p.n) + 1); /* To avoid inf */
    }
  }
}

void exitGraph(params_t p) {
  int i;

  // Deallocate matrices
  for (i = 0; i < p.n; i++) {
    free(B[i]);
  }
  free(B);
  free(W);
}

void dumpGraph(params_t p) {
  int i;
  FILE* fp;

  fp = fopen(p.dumpfile, "w");
  fprintf(fp, "W\n");
  for (i = 0; i < p.n; i++) {
    fprintf(fp, "%.6lf\n", W[i]);
  }

  fclose(fp);
}

int checkGraph(params_t p) {
  int i;
  FILE* fp;
  char tmp_c, fname[200];
  type_t tmp_f;
  
  sprintf(fname, "SEQ-n%d.txt", p.n);
  fp = fopen(fname, "r");
  if(fp == NULL) {
    printf("No se puede comprobar el resultado. No se pudo abrir el fichero de dump para n=%d\n",p.n);
    return 1;
  }
  fscanf(fp, "%c\n", &tmp_c);
  for (i = 0; i < p.n; i++) {
    fscanf(fp, "%f\n", &tmp_f);
    if(ABS(W[i] - tmp_f) >= 0.000001f)
    {
      printf("W[%d]=%f tmp_f=%f W-tmp = %f\n", i, W[i], tmp_f, ABS(W[i] - tmp_f));
      fclose(fp);
      return 1;
    }      
  }
  fclose(fp);
  return 0;
}


void dumpGraph2(params_t p, int iter) {
  int i, j;
  FILE* fp;

  fp = fopen(p.dumpfile, "a");
  fprintf(fp, "Iter: %d, W:\n", iter);
  for (i = 0; i < p.n; i++) {
    fprintf(fp, "%.6lf ", W[i]);
  }
  fprintf(fp, "\n\n");
  for (i = 0; i < p.n; i++) {
    for (j = 0; j < p.n; j++) {
      fprintf(fp, "%.6lf ", B[i][j]);
    }
    fprintf(fp, "\n");
  }
  fprintf(fp, "\n\n");
  fclose(fp);
}

//RIC pongo el parámetro como void*

void kernel_Histogram(void * p) {
  params_t *paramPtr = (params_t *) p;
  int k, t, N, c, chunk, limit, tid, numTh, start, stop, cstart, cstop;
  N = paramPtr->n;
  numTh = paramPtr->nthreads;
  tid = thread_getId();
  chunk = paramPtr->chunk;

  TX_DESCRIPTOR_INIT();
  
  limit = N / numTh;
  //if (N % numTh) limit++; //RIC dependiendo de los valores de entrada puede que algún thread se quede sin trabajo
  start = tid*limit;
  stop = tid==numTh-1? N : start + limit; //RIC al último se le asigna el final del vector
  /* Limit chunk to prevent stale iterations */
  //if (chunk > limit) chunk = limit;
  //RIC 
  if (chunk > stop-start) chunk = stop-start;
    
  cstart = 0;
  cstop = cstart + chunk;

  // The actual algorithm
  for (t = 0; t <= N - 2; t++) {
    for (k = start; k < stop;) {
      //printf("thread: %d (%d to %d). chunk (%d to %d)\n",tid, start,stop, k, k+chunk);
#ifndef SEQ
      //TM_BEGIN(tid, 0);
      for (c = cstart; c < cstop; c++) {
#endif
        if (k < (N - t - 1)) {
          W[t + k + 1] += B[k][t + k + 1] * W[t];
        }
        k++;
#ifndef SEQ
        if(k >= stop) break;
      }
#ifdef ORD
      TM_END(tid, 0, k+1);
#else
      //TM_END(tid, 0);
#endif
#endif
    }
    SB_BARRIER(tid);
  }
  LAST_BARRIER(tid);
}

int main(int argc, char** argv) {
  //RIC para el tiempo
  TIMER_T start;
  TIMER_T stop;

  int option;
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
  //RIC inicia la barrera para SB
  BARRIER_DESCRIPTOR_INIT(params.nthreads);
  //Barrier_init();
  thread_startup(params.nthreads);
  printf("Initializing arrays... ");
  fflush(stdout);
  initArrays(params);
  printf("Done.\n");
  
  printf("Executing kernel... ");
  fflush(stdout);
  TIMER_READ(start);
  thread_start(kernel_Histogram, (void*) &params);
  TIMER_READ(stop);
  printf("Done.\n");
  printf("Time = %lf\n", TIMER_DIFF_SECONDS(start, stop));
  bool_t status = 0;
  if(!checkGraph(params)) {
    status = 1;
    printf("Check was fine!\n");
  } else printf("Check was wrong!!!!\n");
  fflush(stdout);
  thread_shutdown();
  //RIC
  if (!dumpStats(TIMER_DIFF_SECONDS(start, stop)))
    printf("Error volcando las estadísticas.\n");
   
  if (params.dump) dumpGraph(params);
  dco(params);
  exitGraph(params);
  return 0;
}
