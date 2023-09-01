/***************************************************************************
 *
 *            (C) Copyright 2010 The Board of Trustees of the
 *                        University of Illinois
 *                         All Rights Reserved
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parboil.h"
#include "util.h"

#include "tm.h"
#include "thread.h"

//#define UINT8_MAX 255

typedef struct {
  int numIterations;
  unsigned int histo_height, histo_width, img_width, img_height;
  unsigned char *histo;
  unsigned int *img;
  unsigned int ichunk;
  int nthreads;
} params_t;

void HISTO(void *p) {
  int iter, nthreads, tid, chunk, start, stop, chunkh, starth, stoph;
  params_t *pp = (params_t *) p;
  unsigned char * histo = pp->histo;
  unsigned int * img = pp->img;
  unsigned int histo_height = pp->histo_height;
  unsigned int histo_width = pp->histo_width;
  unsigned int img_height = pp->img_height;
  unsigned int img_width = pp->img_width;
  unsigned int ichunk = pp->ichunk;
  nthreads = pp->nthreads;
  tid = thread_getId();

  TM_THREAD_ENTER();

  chunk = (img_width * img_height + nthreads - 1) / nthreads;
  start = tid*chunk;
  stop = (start + chunk) > img_width * img_height ? img_width * img_height : start + chunk;

  chunkh = (histo_height * histo_width + nthreads - 1) / nthreads;
  starth = tid*chunkh;
  stoph = (starth + chunkh) > histo_height * histo_width ? histo_height * histo_width : starth + chunkh;
  //printf("tid: %3d, start: %d, stop: %d, chunk: %d\n          starth: %d, stoph: %d, chunkh: %d\n",tid,start,stop,chunk,starth,stoph,chunkh);
  for (iter = 0; iter < pp->numIterations; iter++) {
    //memset(histo,0,histo_height*histo_width*sizeof(unsigned char));
    for (int j = starth; j < stoph; j++) histo[j] = 0;
    TM_BARRIER(tid);
    unsigned int i, j;

    //#pragma omp parallel for 
    for (i = start; i < stop; ) {
      //#pragma omp critical
      TM_BEGIN(tid, 0);
      for (j = 0; (j < ichunk) && (i < stop); j++, i++) {
        const unsigned int value = img[i];
        if (histo[value] < UINT8_MAX) {
          ++histo[value];
        }
      }
      TM_END(tid, 0);
    }
    TM_BARRIER(tid);
  }
  TM_LAST_BARRIER(tid);
  TM_THREAD_EXIT();
}

/******************************************************************************
 * Implementation: Reference
 * Details:
 * This implementations is a scalar, minimally optimized version. The only 
 * optimization, which reduces the number of pointer chasing operations is the 
 * use of a temporary pointer for each row.
 ******************************************************************************/

int main(int argc, char* argv[]) {
  struct pb_TimerSet timers;
  struct pb_Parameters *parameters;

  printf("****************************************\n");
  printf("* Base implementation of histogramming.*\n");
  printf("***************************************************************\n");
  printf("* Original maintained by Nady Obeid <obeid1@ece.uiuc.edu>     *\n");
  printf("* This version modified by Ricardo Quislant <quislant@uma.es> *\n");
  printf("***************************************************************\n");

  parameters = pb_ReadParameters(&argc, argv);
  if (!parameters) {
    printf("Usage: -i input -o output iter ichunk th\n"
            " - iter: number of iterations\n"
            " - ichunk: inner chunk (increase size of transaction)\n"
            " - th: threads\n");
    return -1;
  }

  if (!parameters->inpFiles[0]) {
    printf("Usage: -i input -o output iter ichunk th\n"
            " - iter: number of iterations\n"
            " - ichunk: inner chunk (increase size of transaction)\n"
            " - th: threads\n");
    fputs("Input file expected\n", stderr);
    return -1;
  }

  int numIterations, nthreads, ichunk;
  if (argc >= 2) {
    numIterations = atoi(argv[1]);
    ichunk = atoi(argv[2]);
    nthreads = atoi(argv[3]);
    if (nthreads < 1) return -1;
  } else {
    printf("Usage: -i input -o output iter ichunk th\n"
            " - iter: number of iterations\n"
            " - ichunk: inner chunk (increase size of transaction)\n"
            " - th: threads\n");
    fputs("Expected at least one command line argument\n", stderr);
    return -1;
  }

  printf("* Parameters ************************\n");
  printf("Input file: %s\n", parameters->inpFiles[0]);
  printf("Output file: %s\n", parameters->outFile);
  printf("iter: %d\n", numIterations);
  printf("ichunk: %d\n", ichunk);
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
  printf("Done with ops.\n");

  //RIC inicio las estadísticas para Power 8
  // Vuelvo a meter el archivo de entrada en los argumentos porque pb_ReadParameters() lo quita
  // y lo necesito para el nombre del fichero de salida
  argc++;
  argv[argc - 1] = parameters->inpFiles[0];
  if (!statsFileInit(argc, argv, nthreads)) { //RIC para las estadísticas
    printf("Error opening stats file.\n");
    return -1;
  }
  //RIC inicia la barrera para SB
  TM_STARTUP(nthreads);
  //Barrier_init();
  thread_startup(nthreads);

  pb_InitializeTimerSet(&timers);

  char *inputStr = "Input";
  char *outputStr = "Output";

  pb_AddSubTimer(&timers, inputStr, pb_TimerID_IO);
  pb_AddSubTimer(&timers, outputStr, pb_TimerID_IO);

  pb_SwitchToSubTimer(&timers, inputStr, pb_TimerID_IO);

  unsigned int img_width, img_height;
  unsigned int histo_width, histo_height;

  FILE* f = fopen(parameters->inpFiles[0], "rb");
  int result = 0;

  result += fread(&img_width, sizeof (unsigned int), 1, f);
  result += fread(&img_height, sizeof (unsigned int), 1, f);
  result += fread(&histo_width, sizeof (unsigned int), 1, f);
  result += fread(&histo_height, sizeof (unsigned int), 1, f);

  if (result != 4) {
    fputs("Error reading input and output dimensions from file\n", stderr);
    return -1;
  }
  printf("Image widthxheight: %dx%d\n", img_width, img_height);
  printf("Histo widthxheight: %dx%d\n", histo_width, histo_height);
  unsigned int* img = (unsigned int*) malloc(img_width * img_height * sizeof (unsigned int));
  unsigned char* histo = (unsigned char*) calloc(histo_width*histo_height, sizeof (unsigned char));

  pb_SwitchToSubTimer(&timers, "Input", pb_TimerID_IO);

  result = fread(img, sizeof (unsigned int), img_width*img_height, f);

  fclose(f);

  if (result != img_width * img_height) {
    fputs("Error reading input array from file\n", stderr);
    return -1;
  }

  params_t th_pars;
  th_pars.histo_width = histo_width;
  th_pars.histo_height = histo_height;
  th_pars.img_width = img_width;
  th_pars.img_height = img_height;
  th_pars.histo = histo;
  th_pars.img = img;
  th_pars.nthreads = nthreads;
  th_pars.numIterations = numIterations;
  th_pars.ichunk = ichunk;
  printf("Executing Kernel...\n");
  pb_SwitchToTimer(&timers, pb_TimerID_COMPUTE);
  thread_start(HISTO, (void*) &th_pars);
  //  pb_SwitchToTimer(&timers, pb_TimerID_IO);
  pb_SwitchToSubTimer(&timers, outputStr, pb_TimerID_IO);
  //RIC 
  thread_shutdown();
  TM_SHUTDOWN();
  printf("Writing output file...\n");
  if (parameters->outFile) {
    dump_histo_img(histo, histo_height, histo_width, parameters->outFile);
  }

  //RIC verificate result with tools/compare-output
  char stmp[1024] = "./tools/compare-output inputs/";
  int begi = parameters->inpFiles[0][0] == '.' ? 9 : 7; // RIC ./inputs/ or inputs/
  //Los archivos de entrada empiezan por una palabra de 5 letras (el que empezaba por default lo he cambiado a deflt)
  char stmp2[6] = {parameters->inpFiles[0][begi], parameters->inpFiles[0][begi + 1], parameters->inpFiles[0][begi + 2], parameters->inpFiles[0][begi + 3], parameters->inpFiles[0][begi + 4], '\0'};
  strcat(stmp, stmp2);
  strcat(stmp, "_output_ref.bmp ");
  strcat(stmp, parameters->outFile);
  printf("Verficating results... ");
  fflush(stdout);
  int verif = !system(stmp); //compare-output returns -1 on success
  //printf("Verification %s\n",verif?"Right":"Wrong");
  if (!dumpStats(pb_GetElapsedTime(&timers.timers[pb_TimerID_COMPUTE]), verif))
    printf("Error dumping stats.\n");

  //pb_SwitchToTimer(&timers, pb_TimerID_COMPUTE);

  free(img);
  free(histo);

  pb_SwitchToTimer(&timers, pb_TimerID_NONE);

  printf("\n");
  pb_PrintTimerSet(&timers);
  pb_FreeParameters(parameters);

  return 0;
}
