#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>
#include "tm.h"
//RIC paso timerutils y pongo el de stamp
//#include "timerutils.h"
#include "timer.h"
#include "graphutils.h"
//RIC el rand_r aborta transacciones a mansalva. Pongo el random.h de ../lib
//RIC random.h se pone a generar números aleatorios cuando se sobrepasa 600 y pico y eso hace
//que haya muchos abortos por footprint
//#include "random.h"
#define MAX_THREADS_DGCA  128 /* Used to declare static structures RIC hay otro MAX_THREADS en transaction.h*/

//RIC creamos un PRNG de coña sacado de stackoverflow https://stackoverflow.com/questions/18323738/fast-pseudorandom-number-generator-for-cryptography-in-c
#define RANDENAUER(v)  ((((v * 214013L + 2531011L) >> ((sizeof(v) * 8) - (16))) | ((v * 214013L + 2531011L) << (16))))


/* Aligned array of seed for rand_r */
//seed_t th_seed[MAX_THREADS_DGCA];
//random_t **randomPtr;
__attribute__ ((aligned (CACHE_BLOCK_SIZE))) uint64_t th_seed[MAX_THREADS_DGCA][CACHE_BLOCK_SIZE/sizeof(uint64_t)]; //RIC semillas y padding para RANDENAUER

#define DEF_N        10
#define DEF_DUMP     0
#define DEF_CHUNK    1
#define DEF_NTH      1
#define DEF_COLORS   5
#define DEF_BETA     0.5
#define DEF_TMAX     4000
#define DEF_SEED     1 
#define DEF_DUMPF    "dgca.dump"
#define DEF_PROFILE  "seq"
#define DEF_VERBOSE  0
#define DEF_CONNECTIONS 2
#define DEF_CHECK    0
#define DEF_RANDOM   0
#define DEF_CMAX     0
#define DEF_NAME     "none"
#define type_t       int

#define DEBUG_ENABLED_

#ifdef DEBUG_ENABLED
#define DEBUG(...)         printf(__VA_ARGS__); fflush(NULL)            
#else /* ! TRACE */
#define DEBUG(...)
#endif

typedef struct {
  int n;
  int dump;
  int chunk;
  int nthreads;
  int load;
  int seed;
  int verbose;
  float beta;
  int tmax;
  int cmax;
  int colors;
  int connections;
  int check; /* if enabled, checks whether the graph is solved in each step */
  int random; /* if enabled, asigns random colors according to prob in each step */
  char dumpfile[100];
  char name[100];
} params_t;

/*typedef struct {
  char pad1[CACHELINE_SIZE];
  uint32_t seed;
  char pad2[CACHELINE_SIZE];
} seed_t;*/

/* Graph structure (adjacency list) */
graph_t* graph;
disgraph_t* dg;

/* Color probability array for each node */
float prob[10000][64];

void initParams(params_t* p) {
  p->n = DEF_N;
  p->dump = DEF_DUMP;
  p->chunk = DEF_CHUNK;
  p->nthreads = DEF_NTH;
  p->seed = DEF_SEED;
  p->verbose = DEF_VERBOSE;
  p->beta = DEF_BETA;
  p->tmax = DEF_TMAX;
  p->connections = DEF_CONNECTIONS;
  p->colors = DEF_COLORS;
  p->check = DEF_CHECK;
  p->random = DEF_RANDOM;
  p->cmax = DEF_CMAX;
  strncpy(p->dumpfile, DEF_DUMPF, 100);
  strncpy(p->name, DEF_NAME, 100);
}

void printParams(params_t p) {
  printf("------------------------\n");
  printf("CFL (Graph Coloring)\n");
  printf("Nodes:    %d\n", p.n);
  printf("Colors:   %d\n", p.colors);
  printf("Beta:     %f\n", p.beta);
  printf("Tmax:     %d\n", p.tmax);
  printf("Dump?:    %d\n", p.dump);
  printf("Dumpfile: %s\n", p.dumpfile);
  printf("Chunk:    %d\n", p.chunk);
  printf("Nthreads: %d\n", p.nthreads);
  printf("Seed:     %d\n", p.seed);
  printf("Connect.: %d\n", p.connections);
  printf("Verbose:  %d\n", p.verbose);
  printf("Cmax:     %d\n", p.cmax);
  printf("Check?:   %d\n", p.check);
  printf("Random?:  %d\n", p.random);
  printf("Name:     %s\n", p.name);
  printf("------------------------\n");
}

void printUsage(void) {
  printf("usage: ./dgca [[-n <nodes>] [-m <tmax>] [-k <colors>] [-b <beta>] [-t <nthreads>] [-h help] [-v verbose] [-w connections] [-f load] [-r (random color asignment)] [-v (verbose)] [-e (check)]\n");

}

/* Choose a random color from a prob* vector */
int randColor(float* prob, int colours, int tid) {

  int i;
  //float aux = (float) rand_r(&th_seed[tid].seed) / RAND_MAX;
  //float aux = (float) random_generate(randomPtr[tid]) / UINT32_MAX;
  th_seed[tid][0] = RANDENAUER(th_seed[tid][0]);
  float aux = (float) th_seed[tid][0] / UINT64_MAX;
  float dist = 0;
  //printf("%f\n", aux);

  for (i = 0; i < colours; i++) {
    dist += prob[i];
    if (aux < dist) {
      return i;
    }
  }
  return i;
}

/* Display some elements for dead-code elimination */
void dco(params_t p) {

}

void initSeedArray(params_t p) {
  int i;

  // Seed initialization
  for (i = 0; i < p.nthreads; i++) {
    //th_seed[i].seed = p.seed;
    th_seed[i][0] = p.seed;
  }
}

void initProbArray(params_t p) {
  int i, j;
  /* En t=0, el vector prob es 1/C en todos los casos */
  for (i = 0; i < p.n; i++) {
    for (j = 0; j < p.colors; j++) {
      prob[i][j] = 1.0 / p.colors;
    }
    addColorDis(dg, i, randColor(prob[i], p.colors, 0));
  }
}

void exitProbArray(params_t p) {

}

void dumpGraph(params_t p) {

}

void dumpGraph2(params_t p, int iter) {

}

void kernel(void *p) {
  params_t *paramPtr = (params_t *) p;
  int numTh, NN, tid, tmax, cmax, i, j, k, c, t, colors, collision, tstart, tstop, limit, random;
  float beta;
  
  TM_THREAD_ENTER();
  
  numTh = paramPtr->nthreads;
  NN = paramPtr->n;
  tid = thread_getId();
  tmax = paramPtr->tmax;
  cmax = paramPtr->cmax;
  colors = paramPtr->colors;
  beta = paramPtr->beta;
  random = paramPtr->random;
  limit = NN / numTh;
  //if (N % numTh) limit++; //RIC dependiendo de los valores de entrada puede que algún thread se quede sin trabajo
  tstart = tid*limit;
  tstop = tid==numTh-1? NN : tstart + limit; //RIC al último se le asigna el final del vector

  /* Kernel */
  for (t = 0; t < tmax; t++) {
    /* For each node */
    for (i = tstart; i < tstop; i++) {

      TM_BEGIN(tid,0);
      collision = 0;

      /* Check if an adjacent node has same color */
      for (j = dg->row[i]; j < dg->row[i + 1]; j++) {
        if (dg->nodeInfo[i].color == dg->nodeInfo[dg->adjList[j]].color) {
          collision = 1;
        }
      }

      /* If there has been a collision, update prob array for current node */
      if (collision) {
        for (k = 0; k < colors; k++) {
          /* Formula for matching color prob[i] */
          if (k == dg->nodeInfo[i].color) {
            prob[i][k] = (1 - beta) * prob[i][k]; /* write en variable compartida (particionada) */
          }/* Formula for non-matching color prob[i] */
          else {
            prob[i][k] = (1 - beta) * prob[i][k]+(beta / (colors - 1)); /* write en variable compartida (particionada) */
          }
        }
      }        /* If there has not been any collision, Assign 1 prob to matching color */
      else {
        for (k = 0; k < colors; k++) {
          /* Formula for matching color prob[i] */
          if (k == dg->nodeInfo[i].color) {
            prob[i][k] = 1; /* write en variable compartida (particionada) */
          }/* Formula for non-matching color prob[i] */
          else {
            prob[i][k] = 0; /* write en variable compartida (particionada) */
          }
        }
      }

      /* Update current node color according to its prob array */
      if (random) {
        addColorDis(dg, i, randColor(prob[i], colors, tid));
      } else {
        dg->nodeInfo[i].color = 0;
      }

      /* Added: Computational charge */
      for (c = 0; c < cmax; c++) {
        collision += c * c + cmax*c;
      }

      TM_END(tid,0);
    }

    /* Wait all the thread to complete the step */
    TM_BARRIER(tid);
  }
  TM_LAST_BARRIER(tid);

  TM_THREAD_EXIT();
}

int main(int argc, char** argv) {
  int option, i, j, k;
  params_t params;

  //RIC para el tiempo
  TIMER_T start;
  TIMER_T stop;

  initParams(&params);

  while ((option = getopt(argc, argv, "n:c:t:l:hvz:b:k:m:s:w:edf:r")) != -1) {
    switch (option) {
      case 'n':
        /* Size of the array */
        params.n = atoi(optarg);
        break;
      case 'c':
        /* Chunksize per xact. */
        params.chunk = atoi(optarg);
        break;
      case 't':
        /* Number of threads */
        params.nthreads = atoi(optarg);
        break;
      case 'v':
        /* Verbose mode (not for benchmarking) */
        params.verbose = 1;
        break;
      case 'z':
        /* Customize name (for parsing) */
        strncpy(params.name, optarg, 100);
        break;
      case 'b':
        /* Beta parameter */
        params.beta = atof(optarg);
        assert(params.beta >= 0 && params.beta <= 1);
        break;
      case 'k':
        /* Number of colors */
        params.colors = atoi(optarg);
        assert(params.colors >= 1);
        break;
      case 'm':
        /* Tmax */
        params.tmax = atoi(optarg);
        assert(params.tmax >= 1);
        break;
      case 's':
        /* Initial seed for prng */
        params.seed = atoi(optarg);
        break;
      case 'e':
        /* check results (not for benchmarking) */
        params.check = 1;
        break;
      case 'w':
        /* Number of connections per node */
        params.connections = atoi(optarg);
        break;
      case 'h':
        /* Help */
        printUsage();
        exit(EXIT_SUCCESS);
        break;
      case 'd':
        /* Help */
        params.dump = 1;
        break;
      case 'r':
        /* Randon color assignment (may produce more conflicts) */
        params.random = 1;
        break;
      case 'f':
        /* Help */
        params.cmax = atoi(optarg);
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
  //temp += rand_r(&th_seed[0].seed);
  //printf("Done with rand_r\n");
  //printf("sizeof(uint32_t)=%d, sizeof(uint8_t)=%d, sizeof(char)=%d, sizeof(tm_tx_t)=%d\n",sizeof(uint32_t), sizeof(uint8_t), sizeof(char), sizeof(tm_tx_t));
  
  //RIC inicio las estadísticas para Power 8
  if(!statsFileInit(argc, argv, params.nthreads)) { //RIC para las estadísticas
    printf("Error abriendo o inicializando el archivo de estadísticas.\n");
    return 0;
  }
  //RIC inicia la barrera para SB
  TM_STARTUP(params.nthreads);
  thread_startup(params.nthreads);
  printf("Creating graph... ");
  fflush(stdout);
  srand(params.seed);
  /* 1. Create dense graph and convert it to disperse */
  graph = createGraph(params.n);
  printf("Done.\n");

  printf("Creating connections... ");
  fflush(stdout);
  /* Create connections */
  int conn;
  uint32_t tmp_adj1; 
  uint32_t tmp_adj2;
  /* Garantizamos un máximo de params.connections por nodo */
  for (i = 0; i < params.n; i++) {
    tmp_adj1 = 0;
    for(j = 0; j < params.n; j++)
      if(graph->adjMat[i][j]) tmp_adj1++;
    for(j = tmp_adj1; j < params.connections; j++) {
      conn = rand() % params.n;
      while (conn == i) conn = rand() % params.n;
      tmp_adj2 = 0;
      for(k = 0; k < params.n; k++)
        if(graph->adjMat[conn][k]) tmp_adj2++;
      if(tmp_adj2 < params.connections)
        addEdge(graph, i, conn);
      else j--;
    }
  }
  printf("Done.\n");
  printf("Creating disperse graph... ");
  fflush(stdout);
  /* Convert graph to disperse */
  dg = convertGraph(*graph);
  printf("Connectivity: %f\n", getConnectivityDis(*dg));
  printf("Destroying initial graph... ");
  fflush(stdout);
  /* Destroy initial graph */
  destroyGraph(graph);
  printf("Done.\n");


  /* 2. Init aligned seed array and set initial probability to prob matrix */
  /* randomPtr = (random_t **)malloc(sizeof(random_t *)*params.nthreads);
  assert(randomPtr);
  for(i=0; i<params.nthreads; i++) {
    randomPtr[i] = random_alloc();
    assert(randomPtr[i] != NULL);
    random_seed(randomPtr[i], params.seed);
  }*/
  initSeedArray(params);
  initProbArray(params);

  /* 3. Execute kernel */
  printf("Executing kernel... ");
  fflush(stdout);
  TIMER_READ(start);
  thread_start(kernel, (void*) &params);
  TIMER_READ(stop);
  printf("Done.\n");

  printf("Time = %lf\n", TIMER_DIFF_SECONDS(start, stop));

  bool_t status = FALSE;
  printf("Checking solution... ");
  if(checkColorsDis(*dg)) {
    printf("Solution found.\n");
    status = TRUE;
  } else printf("Solution NOT found.\n");
  fflush(stdout);
  thread_shutdown();
  TM_SHUTDOWN();

  //RIC
  if(!dumpStats(TIMER_DIFF_SECONDS(start, stop), status))
    printf("Error volcando las estadísticas.\n");
  
  if (params.dump) printAdjList2(*dg);

  dco(params);
  /*for(i=0;i<params.nthreads; i++)
    random_free(randomPtr[i]);
  free(randomPtr);*/

  destroyDisgraph(dg);
  fflush(NULL);
  return 0;
}
