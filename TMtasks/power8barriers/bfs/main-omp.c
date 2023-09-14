/***************************************************************************
 *cr
 *cr            (C) Copyright 2007 The Board of Trustees of the
 *cr                        University of Illinois
 *cr                         All Rights Reserved
 *cr
 ***************************************************************************/
/*
  Implementing Breadth first search on CUDA using algorithm given in DAC'10
  paper "An Effective GPU Implementation of Breadth-First Search"

  Copyright (c) 2010 University of Illinois at Urbana-Champaign. 
  All rights reserved.

  Permission to use, copy, modify and distribute this software and its documentation for 
  educational purpose is hereby granted without fee, provided that the above copyright 
  notice and this permission notice appear in all copies of this software and that you do 
  not sell the software.

  THE SOFTWARE IS PROVIDED "AS IS" AND WITHOUT WARRANTY OF ANY KIND,EXPRESS, IMPLIED OR 
  OTHERWISE.

  Author: Lijiuan Luo (lluo3@uiuc.edu)
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "parboil.h"
//#include <deque>
#include "queue.h" //RIC meto el queue.h de STAMP
#include "thread.h"

#define MAX_THREADS_PER_BLOCK 512
#define NUM_SM 30//the number of Streaming Multiprocessors; may change in the future archs 
#define NUM_SP 16//8//the number of Streaming processors within each SM; may change in the future 
//architectures
#define EXP 4//3// EXP = log(NUM_SP), assuming NUM_SP is still power of 2 in the future architecture
//using EXP and shifting can speed up division operation 
#define MOD_OP 8//7 // This variable is also related with NUM_SP; may change in the future architecture;
//using MOD_OP and "bitwise and" can speed up mod operation
#define INF 2147483647//2^31-1

#define UP_LIMIT 16677216//2^24
#define WHITE 16677217
#define GRAY 16677218
#define GRAY0 16677219
#define GRAY1 16677220
#define BLACK 16677221
int no_of_nodes; //the number of nodes in the graph
int edge_list_size; //the number of edges in the graph
FILE *fp;

//typedef int2 Node;
//typedef int2 Edge;

typedef struct node {
  int x;
  int y;
} Node;

typedef struct edge {
  int x;
  int y;
} Edge;

//void BFS_CPU(Node * h_graph_nodes, Edge * h_graph_edges, int * color, int * h_cost, int source)
typedef struct {
  Node *h_graph_nodes;
  Edge *h_graph_edges;
  int *color;
  int *h_cost;
  int nthreads;
} params_t;

const int h_top = 1;
const int zero = 0;

//RIC pongo un valor alto para que no haga resize dentro de transacción
queue_t* g_wavefront;
int g_index;

////////////////////////////////////////////////////////////////////
//the cpu version of bfs for speed comparison
//the text book version ("Introduction to Algorithms")
////////////////////////////////////////////////////////////////////

//void BFS_CPU(Node * h_graph_nodes, Edge * h_graph_edges, int * color, int * h_cost, int source) {
void BFS_CPU(void *p) {
  params_t *pp = (params_t *)p;
  int *color = pp->color;
  Edge *hge = pp->h_graph_edges;
  Node *hgn = pp->h_graph_nodes;
  int *hc = pp->h_cost;
  int nthreads = pp->nthreads;
  int tid; // = thread_getId();
  int chunk, start, stop;
  
  //TM_THREAD_ENTER();
  //RIC pongo la cola global
  //std::deque<int> wavefront;
  //wavefront.push_back(source);
  // RIC put this before threading
  //queue_push(wavefront,(void *)source);
  //color[source] = GRAY;
  //int index; RIC index global
  //while (!wavefront.empty()) {
  while (!TMQUEUE_ISEMPTY(g_wavefront)) {
    //index = wavefront.front();
    //TM_BARRIER(tid);
    g_index = (intptr_t)TMQUEUE_POP(g_wavefront);
    //TM_BARRIER(tid);
    //wavefront.pop_front();
    
#pragma omp parallel for
    //chunk = ((hgn[g_index].y + hgn[g_index].x) - hgn[g_index].x) / nthreads;
    //start = tid*chunk + hgn[g_index].x;
    //stop = (tid == nthreads - 1)? (hgn[g_index].y + hgn[g_index].x) : start + chunk;
    //printf("tid: %d chunk: %d start: %d stop: %d\n",tid, chunk, start, stop);
    //printf("nthreads: %d, tid: %d, start: %d, stop: %d, index: %d, hgn[g_index].y: %d, hgn[g_index].x: %d\n",nthreads, tid, start, stop, g_index, hgn[g_index].y, hgn[g_index].x);
    for (int i = hgn[g_index].x; i < (hgn[g_index].y + hgn[g_index].x); i++) {
    //for (int i = start; i < stop; i++) {
      int id = hge[i].x;
      if (color[id] == WHITE) {
        hc[id] = hc[g_index] + 1;

#pragma omp critical
        //TM_BEGIN(tid, 0);
        //wavefront.push_back(id);
        //queue_push(wavefront,(void *)id);
        TMQUEUE_PUSH(g_wavefront,(void *)(intptr_t)id);
        //TM_END(tid, 0);

        color[id] = GRAY;
      }
    }
    //TM_BARRIER(tid);
    //if(tid == 0)
    color[g_index] = BLACK;
  }
  //TM_LAST_BARRIER(tid);
  //TM_THREAD_EXIT();
}
////////////////////////////////////////////////////////////////////////////////
// Main Program
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
  no_of_nodes = 0;
  edge_list_size = 0;
  struct pb_Parameters *params;
  struct pb_TimerSet timers;
  int nthreads;
  
  printf("**************************************\n");
  printf("** CPU-based Breadth First Search   **\n");
  printf("***************************************************************\n");
  printf("* Original version by Lijiuan Luo <lluo3@uiuc.edu>            *\n");
  printf("* This version modified by Ricardo Quislant <quislant@uma.es> *\n");
  printf("***************************************************************\n");

  pb_InitializeTimerSet(&timers);
  params = pb_ReadParameters(&argc, argv);
  if ((params->inpFiles[0] == NULL) || (params->inpFiles[1] != NULL) || (argc < 2)) {
    printf("Usage: -i input -o output th\n"
           " - th: threads\n");
    fprintf(stderr, "Expecting one input filename\n");
    exit(-1);
  }
  printf("* Parameters ************************\n");
  printf("Input file: %s\n", params->inpFiles[0]);
  printf("Output file: %s\n", params->outFile);
  nthreads = atoi(argv[1]);
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
  printf("Done with ops.\n");
  
  //RIC inicio las estadísticas para Power 8
  // Vuelvo a meter el archivo de entrada en los argumentos porque pb_ReadParameters() lo quita
  // y lo necesito para el nombre del fichero de salida
  argc++;
  argv[argc-1] = params->inpFiles[0]; 
  /*if(!statsFileInit(argc, argv, nthreads)) { //RIC para las estadísticas
    printf("Error opening stats file.\n");
    return -1;
  } */ 
  //RIC inicia la barrera para SB
  //TM_STARTUP(nthreads);
  //Barrier_init();
  //thread_startup(nthreads);
  g_wavefront = queue_alloc(1024);

  pb_SwitchToTimer(&timers, pb_TimerID_IO);
  printf("Reading input file...\n");
  //Read in Graph from a file
  fp = fopen(params->inpFiles[0], "r");
  if (!fp) {
    printf("Error Reading graph file\n");
    return -1;
  }

  int source, fsout;
  fsout = fscanf(fp, "%d", &no_of_nodes);
  assert(fsout);
  // allocate host memory
  Node* h_graph_nodes = (Node*) malloc(sizeof (Node) * no_of_nodes);
  int *color = (int*) malloc(sizeof (int)*no_of_nodes);
  int start, edgeno;
  // initalize the memory
  for (unsigned int i = 0; i < no_of_nodes; i++) {
    fsout = fscanf(fp, "%d %d", &start, &edgeno);
    assert(fsout);
    h_graph_nodes[i].x = start;
    h_graph_nodes[i].y = edgeno;
    color[i] = WHITE;
  }
  //read the source node from the file
  fsout = fscanf(fp, "%d", &source);
  assert(fsout);
  fsout = fscanf(fp, "%d", &edge_list_size);
  assert(fsout);
  int id, cost;
  Edge* h_graph_edges = (Edge*) malloc(sizeof (Edge) * edge_list_size);
  for (int i = 0; i < edge_list_size; i++) {
    fsout = fscanf(fp, "%d", &id);
    assert(fsout);
    fsout = fscanf(fp, "%d", &cost);
    assert(fsout);
    h_graph_edges[i].x = id;
    h_graph_edges[i].y = cost;
  }
  if (fp) fclose(fp);

  // allocate mem for the result on host side
  int* h_cost = (int*) malloc(sizeof (int)*no_of_nodes);
  for (int i = 0; i < no_of_nodes; i++) {
    h_cost[i] = INF;
  }
  h_cost[source] = 0;

  params_t th_pars;
  th_pars.color = color;
  th_pars.h_cost = h_cost;
  th_pars.h_graph_edges = h_graph_edges;
  th_pars.h_graph_nodes = h_graph_nodes;
  th_pars.nthreads = nthreads;
  printf("Executing Kernel...\n");
  pb_SwitchToTimer(&timers, pb_TimerID_COMPUTE);
  queue_push(g_wavefront,(void *)(intptr_t)source);
  color[source] = GRAY;
  //BFS_CPU(h_graph_nodes, h_graph_edges, color, h_cost, source);
  //thread_start(BFS_CPU, (void*) &th_pars);
  BFS_CPU((void*) &th_pars);
  pb_SwitchToTimer(&timers, pb_TimerID_IO);
  printf("Writing output file...\n");
  if (params->outFile != NULL) {
    FILE *fp = fopen(params->outFile, "w");
    fprintf(fp, "%d\n", no_of_nodes);
    for (int i = 0; i < no_of_nodes; i++)
      fprintf(fp, "%d %d\n", i, h_cost[i]);
    fclose(fp);
  }
  //RIC verificate result with tools/compare-output
  char stmp[1024] = "./tools/compare-output inputs/";
  int begi = params->inpFiles[0][0] == '.'? 9:7; // RIC ./inputs/ or inputs/
  char stmp2[4] = {params->inpFiles[0][begi], params->inpFiles[0][begi+1], params->inpFiles[0][begi+2], '\0'};
  strcat(stmp,stmp2);
  strcat(stmp,"bfs.out ");
  strcat(stmp,params->outFile);
  printf("Verficating results... ");
  fflush(stdout);
  int verif = !system(stmp); //compare-output returns -1 on success
  //printf("Verification %s\n",verif?"Right":"Wrong");
  /*if(!dumpStats(pb_GetElapsedTime(&timers.timers[pb_TimerID_COMPUTE]), verif))
    printf("Error dumping stats.\n");*/
  pb_SwitchToTimer(&timers, pb_TimerID_COMPUTE);
  // cleanup memory
  free(h_graph_nodes);
  free(h_graph_edges);
  free(color);
  free(h_cost);
  queue_free(g_wavefront);
  pb_SwitchToTimer(&timers, pb_TimerID_NONE);
  pb_PrintTimerSet(&timers);
  pb_FreeParameters(params);
}

