#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <omp.h>
#include <pthread.h>
#include <assert.h>
#include "tm.h"
//RIC paso timerutils y pongo el de stamp
//#include "timerutils.h"
#include "timer.h"

#define DEF_N        100000000
#define DEF_DUMP     0
#define DEF_CHUNK    1
#define DEF_NTH      1
#define DEF_LOAD     0
#define DEF_SEED     1 
#define DEF_DUMPF    "cholesky.dump"
#define DEF_PROFILE  "seq"
#define DEF_VERBOSE  0
#define DEF_NAME     "none"
#define type_t       float  //RIC double

#define DEBUG_ENABLED_

#ifdef DEBUG_ENABLED
#define DEBUG(...)         printf(__VA_ARGS__); fflush(NULL)            
#else /* ! TRACE */
#define DEBUG(...)
#endif

type_t* X;
type_t* V;

typedef struct {
  int n;
  int dump;
  int chunk;
  int nthreads;
  int load;
  int seed;
  int verbose;
  char dumpfile[100];
  char name[100];
} params_t;

void initParams(params_t* p) {
  p->n = DEF_N;
  p->dump = DEF_DUMP;
  p->chunk = DEF_CHUNK;
  p->nthreads = DEF_NTH;
  p->load = DEF_LOAD;
  p->seed = DEF_SEED;
  p->verbose = DEF_VERBOSE;
  strncpy(p->dumpfile, DEF_DUMPF, 100);
  strncpy(p->name, DEF_NAME, 100);
}

void printParams(params_t p) {
  printf("------------------------\n");
  printf("Cholesky (livermore loop 2)\n");
  printf("Nodes:    %d\n", p.n);
  printf("Dump?:    %d\n", p.dump);
  printf("Dumpfile: %s\n", p.dumpfile);
  printf("Chunk:    %d\n", p.chunk);
  printf("Nthreads: %d\n", p.nthreads);
  printf("Load:     %d\n", p.load);
  printf("Seed:     %d\n", p.seed);
  printf("Verbose:  %d\n", p.verbose);
  printf("Name:     %s\n", p.name);
  printf("------------------------\n");
}

void printUsage(void) {
  printf("usage: ./cholesky [-n <nodes>] [-d dump] [-o <dumpfile>] [-c <chunk>] [-t <nthreads>] \
	        [-l <load>] [-h help] [-v verbose]\n");
}

void initArrays(params_t p) {
  int i;
  type_t tmp; //RIC

  // Allocate arrays
  X = (type_t*) malloc(2 * p.n * sizeof (type_t));
  assert(X != NULL);

  V = (type_t*) malloc(2 * p.n * sizeof (type_t));
  assert(V != NULL);

  // Populate matrices
  srand(p.seed);
  tmp = 1.0 / (type_t) ((rand() % p.n) + 1);
  for (i = 0; i < p.n; i++) {
    X[i] = tmp; /* To avoid inf */
    V[i] = tmp; /* To avoid inf */
  }
}

void exitGraph(params_t p) {

  // Deallocate matrices
  free(X);
  free(V);
}

void dumpGraph(params_t p) {
  int i;
  FILE* fp;

  fp = fopen(p.dumpfile, "w");
  fprintf(fp, "X\n");
  for (i = 0; i < 2 * p.n; i++) {
    fprintf(fp, "%lf\n", X[i]);
  }

  fclose(fp);
}

/* Display some elements for dead-code elimination */
void dco(params_t p) {
  int i, rel;
  unsigned int th_seed = p.seed;

  for (i = 0; i < 10; i++) {
    rel = (int) rand_r(&th_seed) % p.n;
    fprintf(stderr, "DCO: %lf, %lf\n", X[rel], V[rel]);
  }
}

void kernel_Cholesky(void *p) {
  params_t *paramPtr = (params_t *) p;
  uint32_t ii, ipntp, ipnt, i, k, end;
  int tid, chunk, c, tx_chunk;
  tid = thread_getId();

  TM_THREAD_ENTER();

  ii = paramPtr->n;
  ipntp = 0;
  tx_chunk = paramPtr->chunk;
  
  do {
    ipnt = ipntp;
    ipntp += ii;
    ii /= 2;
    i = ipntp-1;

    chunk = (ipntp - ipnt) / 2 + (ipntp - ipnt) % 2;
    chunk = chunk / paramPtr->nthreads + ((chunk % paramPtr->nthreads) ? 1 : 0);

    if (chunk < 16) chunk = 16;
    i += tid*chunk;

    end = (chunk * 2 * (tid + 1)) + ipnt + 1;

    k = ipnt + 1 + (tid * 2 * chunk);
    while (k < end && k < ipntp) {
#ifndef SEQ
      TM_BEGIN(tid,0);
      for (c = 0; c < tx_chunk; c++) {
#endif
        i++;
        //printf("chunk=%d, tid = %d, Se accede a X[%d] , X y V[%d], X[%d], X y V[%d]\n", chunk, tid, i, k, k-1, k+1);
        X[i] = X[k] - V[k] * X[k - 1] - V[k + 1] * X[k + 1];
        k += 2;
#ifndef SEQ
        if(k >= end || k >= ipntp) break;
      }
      TM_END(tid,0);
#endif
    }
    TM_BARRIER(tid);
    //~ printf("I'm thread %d: Passing barrier (ii = %10lu) (new order = %lu)\n",tid,ii, tx.order); fflush(NULL);
  } while (ii > 1);
  TM_LAST_BARRIER(tid);

  TM_THREAD_EXIT();
}

int main(int argc, char** argv) {
  //RIC para el tiempo
  TIMER_T start;
  TIMER_T stop;
  int option;
  params_t params;

  initParams(&params);

  while ((option = getopt(argc, argv, "n:dc:t:l:ho:vz:")) != -1) {
    switch (option) {
      case 'n':
        /* Size of the array */
        params.n = atoi(optarg);
        break;
      case 'd':
        /* Dump array into a file? */
        params.dump = 1;
        break;
      case 'c':
        /* Chunksize per xact. */
        params.chunk = atoi(optarg);
        break;
      case 't':
        /* Number of threads */
        params.nthreads = atoi(optarg);
        break;
      case 'l':
        /* Additional load */
        params.load = atoi(optarg);
        if (params.load != 0) {
          printf("ERROR: Load not implemented\n");
          exit(EXIT_FAILURE);
        }
        break;
      case 'o':
        /* Output file (need -d) */
        strncpy(params.dumpfile, optarg, 100);
        break;
      case 'v':
        /* Verbose mode (not for benchmarking) */
        params.verbose = 1;
        break;
      case 'z':
        /* Output file (need -z) */
        strncpy(params.name, optarg, 100);
        break;
      case 'h':
        /* Help */
        printUsage();
        exit(EXIT_SUCCESS);
        break;
      default:
        printUsage();
        exit(EXIT_FAILURE);
        break;
    }
  }

  printParams(params);

  //RIC para cargar la PLT (Procedure Linkage Table)
  volatile int temp = (unsigned int) argv[1] % (unsigned int) argv[0];
  temp += (int) argv[1] % (int) argv[0];
  temp += (int) argv[1] * (int) argv[0];
  temp += (int) argv[1] / (int) argv[0];
  temp += (unsigned int) argv[1] * (unsigned int) argv[0];
  temp += (unsigned int) argv[1] / (unsigned int) argv[0];
  printf("Done with ops\n");

  //RIC inicio las estadísticas para Power 8
  if(!statsFileInit(argc, argv, params.nthreads)) { //RIC para las estadísticas
    printf("Error abriendo o inicializando el archivo de estadísticas.\n");
    return 0;
  }
  //RIC inicia la barrera para SB
  TM_STARTUP(params.nthreads);
  thread_startup(params.nthreads);
  printf("Initializing arrays... ");
  fflush(stdout);
  initArrays(params);
  printf("Done.\n");

  printf("Executing kernel... ");
  fflush(stdout);
  TIMER_READ(start);
  thread_start(kernel_Cholesky, (void*) &params);
  TIMER_READ(stop);
  printf("Done.\n");
  printf("Time = %lf\n", TIMER_DIFF_SECONDS(start, stop));
  thread_shutdown();
  TM_SHUTDOWN();

  //RIC
  if(!dumpStats(TIMER_DIFF_SECONDS(start, stop), TRUE))
    printf("Error volcando las estadísticas.\n");

  if (params.dump) dumpGraph(params);
  dco(params);
  exitGraph(params);

  return 0;
}
