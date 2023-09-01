
/***************************************************************************
 *cr
 *cr            (C) Copyright 2010 The Board of Trustees of the
 *cr                        University of Illinois
 *cr                         All Rights Reserved
 *cr
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parboil.h"
#include "file.h"
#include "common.h"
#include "kernels.h"
#include "tm.h"
#include "thread.h"

static int read_data(float *A0, int nx, int ny, int nz, FILE *fp) {
  int s = 0, res;
  int i, j, k;
  for (i = 0; i < nz; i++) {
    for (j = 0; j < ny; j++) {
      for (k = 0; k < nx; k++) {
        res = fread(A0 + s, sizeof (float), 1, fp);
        assert(res);
        s++;
      }
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  struct pb_TimerSet timers;
  struct pb_Parameters *parameters;

  printf("**************************************\n");
  printf("** CPU-based 7 points stencil codes **\n");
  printf("***************************************************************\n");
  printf("* Original version by Li-Wen Chang <lchang20@illinois.edu>    *\n"
         "*                   and I-Jui Sung <sung10@illinois.edu>      *\n");
  printf("* Former version maintained by Chris Rodrigues                *\n");
  printf("* This version modified by Ricardo Quislant <quislant@uma.es> *\n");
  printf("***************************************************************\n");
  parameters = pb_ReadParameters(&argc, argv);
  pb_InitializeTimerSet(&timers);
  //declaration RIC meto nthreads
  int nx, ny, nz, nthreads;
  int size;
  int iteration;
  float c0 = 1.0f / 6.0f;
  float c1 = 1.0f / 6.0f / 6.0f;

  if (argc < 6) {
    printf("Usage: -i input -o output nx ny nz t th\n"
           "     - nx: the grid size x\n"
           "     - ny: the grid size y\n"
           "     - nz: the grid size z\n"
           "     - t:  the number of iterations\n"
           "     - th: threads\n");
    return -1;
  }
  printf("* Parameters ************************\n");
  printf("Input file: %s\n", parameters->inpFiles[0]);
  printf("Output file: %s\n", parameters->outFile);
  nx = atoi(argv[1]);
  if (nx < 1) return -1;
  printf("nx: %d\n", nx);
  ny = atoi(argv[2]);
  if (ny < 1) return -1;
  printf("ny: %d\n", ny);
  nz = atoi(argv[3]);
  if (nz < 1) return -1;
  printf("nz: %d\n", nz);
  iteration = atoi(argv[4]);
  if (iteration < 1) return -1;
  printf("iterations (t): %d\n", iteration);
  nthreads = atoi(argv[5]);
  if (nthreads < 1) return -1;
  printf("nthreads (th): %d\n", nthreads);
  printf("**************************************\n");
  
  /* RIC meto aquí las cosas de Power8 y GEMS*/
  //RIC para cargar la PLT (Procedure Linkage Table)
  volatile int tmp = (uintptr_t) argv[1] % (uintptr_t) argv[0];
  tmp += (intptr_t) argv[1] % (intptr_t) argv[0];
  tmp += (intptr_t) argv[1] * (intptr_t) argv[0];
  tmp += (intptr_t) argv[1] / (intptr_t) argv[0];
  tmp += (uintptr_t) argv[1] * (uintptr_t) argv[0];
  tmp += (uintptr_t) argv[1] / (uintptr_t) argv[0];
  printf("Done with ops\n");

  //RIC inicio las estadísticas para Power 8
  if(!statsFileInit(argc, argv, nthreads)) { //RIC para las estadísticas
    printf("Error opening stats file.\n");
    return 0;
  }  
  //RIC inicia la barrera para SB
  TM_STARTUP(nthreads);
  //Barrier_init();
  thread_startup(nthreads);

  //host data
  float *h_A0;
  float *h_Anext;

  printf("Allocating memory...\n");
  size = nx * ny * nz;
  h_A0 = (float*) malloc(sizeof (float)*size);
  if(!h_A0) return -1;
  h_Anext = (float*) malloc(sizeof (float)*size);
  if(!h_Anext) return -1;
  printf("Reading input file...\n");
  FILE *fp = fopen(parameters->inpFiles[0], "rb");
  if(!fp) return -1;
  read_data(h_A0, nx, ny, nz, fp);
  fclose(fp);
  memcpy(h_Anext, h_A0, sizeof (float)*size);
  
  params_t params;
  params.c0 = c0;
  params.c1 = c1;
  params.A0 = h_A0;
  params.Anext = h_Anext;
  params.nx = nx;
  params.ny = ny;
  params.nz = nz;
  params.iterations = iteration;
  params.nthreads = nthreads;
  printf("Executing Kernel...\n");
  pb_SwitchToTimer(&timers, pb_TimerID_COMPUTE);
  thread_start(kernel_stencil, (void*) &params);
  pb_SwitchToTimer(&timers, pb_TimerID_NONE);
  /* RIC computation moved to kernel_stencil
  int t;
  for (t = 0; t < iteration; t++) {
    cpu_stencil(c0, c1, h_A0, h_Anext, nx, ny, nz);
    float *temp = h_A0;
    h_A0 = h_Anext;
    h_Anext = temp;
  }*/
  float *temp = h_A0;
  h_A0 = h_Anext;
  h_Anext = temp;
  //RIC 
  thread_shutdown();
  TM_SHUTDOWN();
  if (parameters->outFile) {
    //pb_SwitchToTimer(&timers, pb_TimerID_IO);
    outputData(parameters->outFile, h_Anext, nx, ny, nz);
  }
  //RIC verificate result with tools/compare-output
  //char stmp[1024] = "./tools/compare-output input/default_512x512x64.out ";
  //strcat(stmp,parameters->outFile);
  //RIC verificate result with tools/compare-output
  char stmp[1024] = "./tools/compare-output ";
  strcat(stmp, parameters->inpFiles[0]);
  int begi = strlen(stmp)-3;
  stmp[begi] = 'o';//Cambio .bin por .out
  stmp[begi+1] = 'u';
  stmp[begi+2] = 't';
  stmp[begi+3] = ' ';
  stmp[begi+4] = '\0';
  strcat(stmp, parameters->outFile);
  printf("Verficating results... (%s) ", stmp);
  fflush(stdout);
  int verif = !system(stmp); //compare-output returns -1 on success
  if(!dumpStats(pb_GetElapsedTime(&timers.timers[pb_TimerID_COMPUTE]), verif))
    printf("Error dumping stats.\n");
  //pb_SwitchToTimer(&timers, pb_TimerID_COMPUTE);

  free(h_A0);
  free(h_Anext);
  
  pb_PrintTimerSet(&timers);
  pb_FreeParameters(parameters);

  return 0;
}
