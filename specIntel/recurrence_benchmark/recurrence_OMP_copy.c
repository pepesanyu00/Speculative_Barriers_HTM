#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> //RIC en solaris no hace falta
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <omp.h>
#include "tm.h"
//#include "thread.h"
//RIC paso de timerutils y pongo el de stamp
//#include "timerutils.h"
#include "timer.h"

#include <unistd.h>
#include <signal.h>

  //g_spec_vars_t g_specvars = {.tx_order = 1};
#define ABS(a) (((a) > 0) ? (a) : -1 * (a))

#define DEF_N 1000
#define DEF_DUMP 0
#define DEF_CHUNK 1
#define DEF_NTH 4
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

void initParams(params_t *p)
{
  p->n = DEF_N;
  p->dump = DEF_DUMP;
  p->chunk = DEF_CHUNK;
  p->nthreads = DEF_NTH;
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
  int k, t, N, c, chunk, limit, tid, numTh, start, stop, cstart, cstop;
  N = paramPtr->n;
  numTh = paramPtr->nthreads;
  chunk = paramPtr->chunk;

#pragma omp parallel
  {
    tm_tx_t tx;                                 
    /* Initialise tx descriptor */              
    tx.order = 1; /* In p8-ordwsb-nt the order updates in barriers, not in start nor commit */
    tx.retries = 0;                             
    tx.specMax = MAX_SPEC;                      
    tx.specLevel = tx.specMax;                  
    tx.speculative = 0;                         
    tx.status = 0;
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
        }
      }
        /* Now, barrier IS SAFE if a thread re-enters the barrier before it have been reset */                                                         
        if (tx.speculative) {                                                         
          BEGIN_ESCAPE;                                                               
          while (tx.order > g_specvars.tx_order);                                     
          END_ESCAPE;                                                                 
          /* Llegados a este punto estoy en modo especulativo, en la OUTER tx, que
          * ha llegado a la barrera por segunda vez después de haber terminado de
          * especular con algunas transacciones tras pasar esta misma barrera antes
          * y ponerse en modo especulativo. Lo que se hace aquí es un commit para 
          * intentar salirnos de la OUTER que abrimos la primera vez que pasamos 
          * con la barrera, y desactivar el modo especulativo porque con el while
          * anterior sabemos que el resto de txs atraveso ya la barrera anterior */  
          _xend();                                                          
          profileCommit(tid, SPEC_XACT_ID, tx.retries-1);                            
          /* Restore metadata */                                                      
          tx.speculative = 0;                                                         
          tx.retries = 0;                                                             
          tx.specLevel = tx.specMax;                                                  
        }                                                                             
        /* We are now in speculative mode until global order increases */             
        tx.order += 1;                                                                
        /* Determine if thread is last to enter the barrier */                        
        if (__sync_add_and_fetch(&(g_specvars.barrier.remain),-1) == 0) {             
          /* If we are last thread to enter in barrier, reset & update global order.  
          * No switch to speculative necessary */                                    
          g_specvars.barrier.remain = g_specvars.barrier.nb_threads;                  
          /* This instruction involves a full memory fence, so the reset of barrier->remain 
          * should appear ordered to the rest of the threads */                      
          __sync_add_and_fetch(&(g_specvars.tx_order), 1);                            
        } else {                                                                      
          __label__ __p_failure;                                                      
      __p_failure:                                                                    
          if(tx.retries) profileAbortStatus(tx.status, tid, SPEC_XACT_ID);      
          tx.retries++;                                                               
          if (tx.order <= g_specvars.tx_order) {                                      
            tx.speculative = 0;                                                       
            tx.retries = 0;                                                           
            tx.specLevel = tx.specMax;                                                                                  
          } else {                                                                    
            tx.speculative = 1;                                                       
            if (tx.retries > MAX_RETRIES) {                                           
              if(tx.specMax > 1) tx.specMax--;                                        
              tx.specLevel = tx.specMax;                                              
            }                                                                         
            while (g_fallback_lock.ticket >= g_fallback_lock.turn);                                                                                                          
            if(_xbegin() != _XBEGIN_STARTED) goto __p_failure;                                
            if (g_fallback_lock.ticket >= g_fallback_lock.turn)                       
            _xabort(LOCK_TAKEN);/*Early subscription*/                       
          }                                                                           
        }
      //#pragma omp barrier
    }    
  }
}

int main(int argc, char **argv)
{
  //RIC para el tiempo
  TIMER_T start;
  TIMER_T stop;

  params_t params;

  initParams(&params);
  printParams(params);

  //RIC inicio las estadísticas para Power 8
  if (!statsFileInit(params.nthreads,4))
  { //RIC para las estadísticas
    printf("Error abriendo o inicializando el archivo de estadísticas.\n");
    return 0;
  }

  omp_set_num_threads(params.nthreads);
  g_specvars.barrier.nb_threads = params.nthreads;  
  g_specvars.barrier.remain     = params.nthreads;
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
  if (!dumpStats())
    printf("Error volcando las estadísticas.\n");
  
  return 0;
}
