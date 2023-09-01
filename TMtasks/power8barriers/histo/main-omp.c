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

#define UINT8_MAX 255

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
    printf("Usage: -i input -o output iter th\n"
           " - iter: number of iterations\n"
           " - th: threads\n");
    return -1;
  }

  if(!parameters->inpFiles[0]){
    printf("Usage: -i input -o output iter th\n"
           " - iter: number of iterations\n"
           " - th: threads\n");
    fputs("Input file expected\n", stderr);
    return -1;
  }

  int numIterations;
  if (argc >= 2){
    numIterations = atoi(argv[1]);
  } else {
    printf("Usage: -i input -o output iter th\n"
           " - iter: number of iterations\n"
           " - th: threads\n");
    fputs("Expected at least one command line argument\n", stderr);
    return -1;
  }

  pb_InitializeTimerSet(&timers);
  
  char *inputStr = "Input";
  char *outputStr = "Output";
  
  pb_AddSubTimer(&timers, inputStr, pb_TimerID_IO);
  pb_AddSubTimer(&timers, outputStr, pb_TimerID_IO);
  
  pb_SwitchToSubTimer(&timers, inputStr, pb_TimerID_IO);  

  unsigned int img_width, img_height;
  unsigned int histo_width, histo_height;

  FILE* f = fopen(parameters->inpFiles[0],"rb");
  int result = 0;

  printf("sizeof(unsigned int): %ld\n", sizeof(unsigned int));
  result += fread(&img_width,    sizeof(unsigned int), 1, f);
  result += fread(&img_height,   sizeof(unsigned int), 1, f);
  result += fread(&histo_width,  sizeof(unsigned int), 1, f);
  result += fread(&histo_height, sizeof(unsigned int), 1, f);

  if (result != 4){
    fputs("Error reading input and output dimensions from file\n", stderr);
    return -1;
  }

  unsigned int* img = (unsigned int*) malloc (img_width*img_height*sizeof(unsigned int));
  unsigned char* histo = (unsigned char*) calloc (histo_width*histo_height, sizeof(unsigned char));
  
  pb_SwitchToSubTimer(&timers, "Input", pb_TimerID_IO);

  result = fread(img, sizeof(unsigned int), img_width*img_height, f);

  fclose(f);

  if (result != img_width*img_height){
    fputs("Error reading input array from file\n", stderr);
    return -1;
  }

  pb_SwitchToTimer(&timers, pb_TimerID_COMPUTE);

  int iter;
  for (iter = 0; iter < numIterations; iter++){
    memset(histo,0,histo_height*histo_width*sizeof(unsigned char));
    unsigned int i;

#pragma omp parallel for 
    for (i = 0; i < img_width*img_height; ++i) {
      const unsigned int value = img[i];

#pragma omp critical
      if (histo[value] < UINT8_MAX) {
        ++histo[value];
      }
    }
  }

//  pb_SwitchToTimer(&timers, pb_TimerID_IO);
  pb_SwitchToSubTimer(&timers, outputStr, pb_TimerID_IO);

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
  printf("Verification %s\n",verif?"Right":"Wrong");
  //if (!dumpStats(pb_GetElapsedTime(&timers.timers[pb_TimerID_COMPUTE]), verif))
    //printf("Error dumping stats.\n");

  //pb_SwitchToTimer(&timers, pb_TimerID_COMPUTE);

  free(img);
  free(histo);

  pb_SwitchToTimer(&timers, pb_TimerID_NONE);

  printf("\n");
  pb_PrintTimerSet(&timers);
  pb_FreeParameters(parameters);

  return 0;
}
