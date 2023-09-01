#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>
#include "tm.h"
#include "thread.h"
#include "timer.h" //RIC pongo el time de stamp

#define DEF_N        1000000 /* Default i in IWOMP paper */
#define DEF_DUMP     0
#define DEF_NTH      1
#define DEF_LOAD     10000
#define DEF_SEED     1 
#define DEF_DUMPF    "iwompMB.dump"
#define DEF_PROFILE  "seq"
#define DEF_VERBOSE  0
#define DEF_NAME     "none"
#define type_t       double

typedef struct{
	int n;
	int dump;
	int nthreads;
	int load;
	int seed;
	int verbose;
	char profile[100];
	char dumpfile[100];
	char name[100];
} params_t;

void initParams(params_t* p){
	p->n = DEF_N;
	p->dump = DEF_DUMP;
	p->nthreads = DEF_NTH;
	p->load = DEF_LOAD;
	p->seed = DEF_SEED;
	p->verbose = DEF_VERBOSE;
	strncpy(p->dumpfile, DEF_DUMPF, 100);
	strncpy(p->name, DEF_NAME, 100);
}

void printParams(params_t p){
	printf("------------------------\n");
	printf("IWOMP Microbenchmark\n");
	printf("Nodes:    %d\n", p.n);
	printf("Dump?:    %d\n", p.dump);
	printf("Dumpfile: %s\n", p.dumpfile);
	printf("Nthreads: %d\n", p.nthreads);
	printf("Load:     %d\n", p.load);
	printf("Seed:     %d\n", p.seed);
	printf("Verbose:  %d\n", p.verbose);
	printf("Name:     %s\n", p.name);
	printf("------------------------\n");
}

void printUsage(void){
	printf("usage: ./iwompSEQ [-n <nodes>][-d dump][-o <dumpfile>][-t <nthreads>][-l <load>][-h help][-v verbose]\n");
}

/* Display some elements for dead-code elimination */
void dco(params_t p){

}

void initArrays(params_t p){
	
}

void exitGraph(params_t p){
	
}

void dumpGraph(params_t p){
	
}

void dumpGraph2(params_t p, int iter){
	
}

void kernel(void *p){
  params_t *paramPtr = (params_t *) p;
  int i, j, tid, N, load;
  volatile uint32_t cnt = 0;
  N = paramPtr->n;
  load = paramPtr->load;
	tid = thread_getId();
	
	TM_THREAD_ENTER();
	
	for(i=0;i<N;i++){
		TM_BEGIN(tid,0);
		for(j=0; j!=load*((tid+i) % 2); j++){
			// Carga. La mitad de hilos ejecutaran p.load veces este codigo
			// y la otra mitad, 0
			cnt++;
		}
		TM_END(tid,0);
		TM_BARRIER(tid);
	}
	TM_LAST_BARRIER(tid);
	//printf("Barrier: %lu\n",cnt);
	TM_THREAD_EXIT();
}

int main(int argc, char** argv){
  //RIC para el tiempo
  TIMER_T start;
  TIMER_T stop;
  
	int option;
	params_t params;
	
	initParams(&params);
		
	while((option = getopt(argc, argv, "n:dt:l:ho:vz:")) != -1){
		switch(option) {
			case 'n':
				/* Size of the array */
				params.n = atoi(optarg);
				break;
			case 'd':
				/* Dump array into a file? */
				params.dump = 1;
				break;
			case 't':
				/* Number of threads */
				params.nthreads = atoi(optarg);
				break;
			case 'l':
				/* Additional load */
				params.load = atoi(optarg);
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
  
  printf("Executing kernel... ");
  fflush(stdout);
  TIMER_READ(start);
  thread_start(kernel, (void*) &params);
  TIMER_READ(stop);
  printf("Done.\n");
  printf("Time = %lf\n", TIMER_DIFF_SECONDS(start, stop));
  thread_shutdown();
  TM_SHUTDOWN();
  //RIC
  if(!dumpStats(TIMER_DIFF_SECONDS(start, stop), TRUE))
    printf("Error volcando las estadísticas.\n");
	
	if(params.dump) dumpGraph(params);
	dco(params);
	exitGraph(params);
	
	return 0;
}
