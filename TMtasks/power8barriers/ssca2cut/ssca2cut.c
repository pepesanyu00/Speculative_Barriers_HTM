/* =============================================================================
 *
 * ssca2.c
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of ssca2, please see ssca2/COPYRIGHT
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 * 
 * ------------------------------------------------------------------------
 * 
 * Unless otherwise noted, the following license applies to STAMP files:
 * 
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 * 
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "cutClusters.h"
#include "computeGraph.h"
#include "defs.h"
#include "genScalData.h"
#include "getUserParameters.h"
#include "globals.h"
#include "timer.h"
#include "thread.h"
#include "tm.h"


MAIN(argc, argv) {
  GOTO_REAL();

  /*
   * Tuple for Scalable Data Generation
   * stores startVertex, endVertex, long weight and other info
   */
  graphSDG* SDGdata;

  /*
   * The graph data structure for this benchmark - see defs.h
   */
  graph* G;

  double totalTime = 0.0;

  /* -------------------------------------------------------------------------
   * Preamble
   * -------------------------------------------------------------------------
   */

  /*
   * User Interface: Configurable parameters, and global program control
   */

  printf("\nHPCS SSCA #2 Graph Analysis Executable Specification:");
  printf("\nRunning...\n\n");

  getUserParameters(argc, (char* * const) argv);

  //RIC inicio las estadísticas para Power 8
  if(!statsFileInit(argc, argv, THREADS)) { //RIC para las estadísticas
    printf("Error abriendo o inicializando el archivo de estadísticas.\n");
    return 0;
  }
  SIM_GET_NUM_CPU(THREADS);
  TM_STARTUP(THREADS);
  P_MEMORY_STARTUP(THREADS);
  thread_startup(THREADS);

  //RIC para cargar la PLT (Procedure Linkage Table)
  volatile int temp = (unsigned int) argv[1] % (unsigned int) argv[0];
  temp += (int) argv[1] % (int) argv[0];
  temp += (int) argv[1] * (int) argv[0];
  temp += (int) argv[1] / (int) argv[0];
  temp += (unsigned int) argv[1] * (unsigned int) argv[0];
  temp += (unsigned int) argv[1] / (unsigned int) argv[0];
  printf("Done with ops\n");

  puts("");
  printf("Number of processors:       %ld\n", THREADS);
  printf("Problem Scale:              %ld\n", SCALE);
  printf("Max parallel edges:         %ld\n", MAX_PARAL_EDGES);
  printf("Percent int weights:        %f\n", PERC_INT_WEIGHTS);
  printf("Probability unidirectional: %f\n", PROB_UNIDIRECTIONAL);
  printf("Probability inter-clique:   %f\n", PROB_INTERCL_EDGES);
  printf("Subgraph edge length:       %ld\n", SUBGR_EDGE_LENGTH);
  printf("Kernel 3 data structure:    %ld\n", K3_DS);
  puts("");

  /*
   * Scalable Data Generator
   */

  printf("\nScalable Data Generator - genScalData() beginning execution...\n");

  SDGdata = (graphSDG*) malloc(sizeof (graphSDG));
  assert(SDGdata);

  TIMER_T start;
  TIMER_READ(start);

  genScalData_seq(SDGdata);

  TIMER_T stop;
  TIMER_READ(stop);

  double time = TIMER_DIFF_SECONDS(start, stop);
  totalTime += time;

  printf("\nTime taken for Scalable Data Generation is %9.6f sec.\n\n", time);
  printf("\n\tgenScalData() completed execution.\n");

  /* -------------------------------------------------------------------------
   * Kernel 1 - Graph Construction
   *
   * From the input edges, construct the graph 'G'
   * -------------------------------------------------------------------------
   */

  printf("\nKernel 1 - computeGraph() beginning execution...\n");

  G = (graph*) malloc(sizeof (graph));
  assert(G);

  computeGraph_arg_t computeGraphArgs;
  computeGraphArgs.GPtr = G;
  computeGraphArgs.SDGdataPtr = SDGdata;

  TIMER_READ(start);

  GOTO_SIM();
  thread_start(computeGraph, (void*) &computeGraphArgs);
  GOTO_REAL();

  TIMER_READ(stop);

  time = TIMER_DIFF_SECONDS(start, stop);
  totalTime += time;

  printf("\n\tcomputeGraph() completed execution.\n");
  printf("\nTime taken for kernel 1 is %9.6f sec.\n", time);
  /* -------------------------------------------------------------------------
   * Kernel 4 - Graph Clustering
   * -------------------------------------------------------------------------
   */

  printf("\nKernel 4 - cutClusters() beginning execution...\n");

  TIMER_READ(start);
  GOTO_SIM();
  thread_start(cutClusters, (void*) G);
  GOTO_REAL();
  TIMER_READ(stop);

  time = TIMER_DIFF_SECONDS(start, stop);
  totalTime += time;

  printf("\n\tcutClusters() completed execution.\n");
  printf("\nTime taken for Kernel 4 is %9.6f sec.\n\n", time);

  printf("\nTime taken for all is %9.6f sec.\n\n", totalTime);

 //RIC
  if(!dumpStats(time, TRUE))
    printf("Error volcando las estadísticas.\n");

  /* -------------------------------------------------------------------------
   * Cleanup
   * -------------------------------------------------------------------------
   */

  P_FREE(G->outDegree);
  P_FREE(G->outVertexIndex);
  P_FREE(G->outVertexList);
  P_FREE(G->paralEdgeIndex);
  P_FREE(G->inDegree);
  P_FREE(G->inVertexIndex);
  P_FREE(G->inVertexList);
  P_FREE(G->intWeight);
  P_FREE(G->strWeight);

  P_FREE(SOUGHT_STRING);
  P_FREE(G);
  P_FREE(SDGdata);

  TM_SHUTDOWN();
  P_MEMORY_SHUTDOWN();

  GOTO_SIM();

  thread_shutdown();

  MAIN_RETURN(0);
}


/* =============================================================================
 *
 * End of ssca2.c
 *
 * =============================================================================
 */
