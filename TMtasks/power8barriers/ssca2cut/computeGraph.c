/* =============================================================================
 *
 * computeGraph.c
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
#include <stdio.h>
#include <stdlib.h>
#include "computeGraph.h"
#include "createPartition.h"
#include "defs.h"
#include "globals.h"
#include "thread.h"
#include "utility.h"
#include "tm.h"

static ULONGINT_T* global_p = NULL;
static ULONGINT_T global_maxNumVertices = 0;
static ULONGINT_T global_outVertexListSize = 0;
static ULONGINT_T* global_impliedEdgeList = NULL;
static ULONGINT_T** global_auxArr = NULL;


/* =============================================================================
 * prefix_sums
 * =============================================================================
 */
//static void
//prefix_sums (ULONGINT_T* result, LONGINT_T* input, ULONGINT_T arraySize)
//{
//long myId = thread_getId();
//long numThread = thread_getNumThread();

/* RIC Pongo lo que había aquí, que tenía barreras, en computeGraph,
 para que no me de el error de que no se encuentra el descriptor
 de la transacción tx, que se define con TM_THREAD_ENTER*/
//}

/* =============================================================================
 * computeGraph
 * =============================================================================
 */
void
computeGraph(void* argPtr) {
  TM_THREAD_ENTER();

  graph* GPtr = ((computeGraph_arg_t*) argPtr)->GPtr;
  graphSDG* SDGdataPtr = ((computeGraph_arg_t*) argPtr)->SDGdataPtr;

  long myId = thread_getId();
  long numThread = thread_getNumThread();

  ULONGINT_T j;
  ULONGINT_T maxNumVertices = 0;
  ULONGINT_T numEdgesPlaced = SDGdataPtr->numEdgesPlaced;

  /*
   * First determine the number of vertices by scanning the tuple
   * startVertex list
   */

  long i;
  long i_start;
  long i_stop;
  createPartition(0, numEdgesPlaced, myId, numThread, &i_start, &i_stop);

  for (i = i_start; i < i_stop; i++) {
    if (SDGdataPtr->startVertex[i] > maxNumVertices) {
      maxNumVertices = SDGdataPtr->startVertex[i];
    }
  }

  pthread_mutex_lock(&global_lock);
  long tmp_maxNumVertices = (long) TM_SHARED_READ(global_maxNumVertices);
  long new_maxNumVertices = MAX(tmp_maxNumVertices, maxNumVertices) + 1;
  TM_SHARED_WRITE(global_maxNumVertices, new_maxNumVertices);
  pthread_mutex_unlock(&global_lock);

  thread_barrier_wait(0 /* non-breaking*/);

  if (myId == 0) {
    //RIC muevo esta asignación dentro del if
    maxNumVertices = global_maxNumVertices;

    GPtr->numVertices = maxNumVertices;
    GPtr->numEdges = numEdgesPlaced;
    GPtr->intWeight = SDGdataPtr->intWeight;
    GPtr->strWeight = SDGdataPtr->strWeight;

    for (i = 0; i < numEdgesPlaced; i++) {
      if (GPtr->intWeight[numEdgesPlaced - i - 1] < 0) {
        GPtr->numStrEdges = -(GPtr->intWeight[numEdgesPlaced - i - 1]) + 1;
        GPtr->numIntEdges = numEdgesPlaced - GPtr->numStrEdges;
        break;
      }
    }
    //LUKE - add additional dummy entries to pad to end of cache line
    int dummy_entries = 64 - sizeof (LONGINT_T);
    GPtr->outDegree = (LONGINT_T*) malloc((GPtr->numVertices + dummy_entries) * sizeof (LONGINT_T));
    assert(GPtr->outDegree);

    dummy_entries = 64 - sizeof (ULONGINT_T);
    GPtr->outVertexIndex = (ULONGINT_T*) malloc((GPtr->numVertices + dummy_entries) * sizeof (ULONGINT_T));
    assert(GPtr->outVertexIndex);
  }

  thread_barrier_wait(0 /* non-breaking*/);

  createPartition(0, GPtr->numVertices, myId, numThread, &i_start, &i_stop);

  for (i = i_start; i < i_stop; i++) {
    GPtr->outDegree[i] = 0;
    GPtr->outVertexIndex[i] = 0;
  }

  ULONGINT_T outVertexListSize = 0;

  thread_barrier_wait(0 /* non-breaking*/);

  ULONGINT_T i0 = -1UL;

  for (i = i_start; i < i_stop; i++) {

    ULONGINT_T k = i;
    if ((outVertexListSize == 0) && (k != 0)) {
      while (i0 == -1UL) {
        for (j = 0; j < numEdgesPlaced; j++) {
          if (k == SDGdataPtr->startVertex[j]) {
            i0 = j;
            break;
          }

        }
        k--;
      }
    }

    if ((outVertexListSize == 0) && (k == 0)) {
      i0 = 0;
    }

    for (j = i0; j < numEdgesPlaced; j++) {
      if (i == GPtr->numVertices - 1) {
        break;
      }
      if ((i != SDGdataPtr->startVertex[j])) {
        if ((j > 0) && (i == SDGdataPtr->startVertex[j - 1])) {
          if (j - i0 >= 1) {
            outVertexListSize++;
            GPtr->outDegree[i]++;
            ULONGINT_T t;
            for (t = i0 + 1; t < j; t++) {
              if (SDGdataPtr->endVertex[t] != SDGdataPtr->endVertex[t - 1]) {
                outVertexListSize++;
                GPtr->outDegree[i] = GPtr->outDegree[i] + 1;
              }
            }
          }
        }
        i0 = j;
        break;
      }
    }

    if (i == GPtr->numVertices - 1) {
      if (numEdgesPlaced - i0 >= 0) {
        outVertexListSize++;
        GPtr->outDegree[i]++;
        ULONGINT_T t;
        for (t = i0 + 1; t < numEdgesPlaced; t++) {
          if (SDGdataPtr->endVertex[t] != SDGdataPtr->endVertex[t - 1]) {
            outVertexListSize++;
            GPtr->outDegree[i]++;
          }
        }
      }
    }

  } /* for i */

  thread_barrier_wait(0 /* non-breaking*/);

  /* RIC cambio prefix_sums por su contenido */
  //prefix_sums(GPtr->outVertexIndex, GPtr->outDegree, GPtr->numVertices);
  //                  result            ,   input,     arraySize
  ULONGINT_T* p = NULL;
  if (myId == 0) {
    p = (ULONGINT_T*) malloc(NOSHARE(numThread) * sizeof (ULONGINT_T));
    assert(p);
    global_p = p;
  }

  thread_barrier_wait(0 /* non-breaking*/);

  p = global_p;

  long start;
  long end;

  long r = GPtr->numVertices / numThread;
  start = myId * r + 1;
  end = (myId + 1) * r;
  if (myId == (numThread - 1)) {
    end = GPtr->numVertices;
  }

  for (j = start; j < end; j++) {
    GPtr->outVertexIndex[j] = GPtr->outDegree[j - 1] + GPtr->outVertexIndex[j - 1];
  }

  p[NOSHARE(myId)] = GPtr->outVertexIndex[end - 1];

  thread_barrier_wait(0 /* non-breaking*/);

  if (myId == 0) {
    for (j = 1; j < numThread; j++) {
      p[NOSHARE(j)] += p[NOSHARE(j - 1)];
    }
  }

  thread_barrier_wait(0 /* non-breaking*/);

  if (myId > 0) {
    ULONGINT_T add_value = p[NOSHARE(myId - 1)];
    for (j = start - 1; j < end; j++) {
      GPtr->outVertexIndex[j] += add_value;
    }
  }

  thread_barrier_wait(0 /* non-breaking*/);

  if (myId == 0) {
    free(p);
  }

  thread_barrier_wait(0 /* non-breaking*/);

  pthread_mutex_lock(&global_lock);
  TM_SHARED_WRITE(global_outVertexListSize, ((long) TM_SHARED_READ(global_outVertexListSize) + outVertexListSize));
  pthread_mutex_unlock(&global_lock);

  thread_barrier_wait(0 /* non-breaking*/);

  if (myId == 0) {
    //RIC muevo la asignación dentro del if
    outVertexListSize = global_outVertexListSize;

    GPtr->numDirectedEdges = outVertexListSize;
    //LUKE - add additional dummy entries to pad to end of cache line
    int dummy_entries = 64 - sizeof (ULONGINT_T);
    GPtr->outVertexList = (ULONGINT_T*) malloc((outVertexListSize + dummy_entries) * sizeof (ULONGINT_T));
    assert(GPtr->outVertexList);

    GPtr->paralEdgeIndex = (ULONGINT_T*) malloc((outVertexListSize + dummy_entries) * sizeof (ULONGINT_T));
    assert(GPtr->paralEdgeIndex);
    GPtr->outVertexList[0] = SDGdataPtr->endVertex[0];
  }

  thread_barrier_wait(0 /* non-breaking*/);

  /*
   * Evaluate outVertexList
   */

  i0 = -1UL;

  for (i = i_start; i < i_stop; i++) {

    ULONGINT_T k = i;
    while ((i0 == -1UL) && (k != 0)) {
      for (j = 0; j < numEdgesPlaced; j++) {
        if (k == SDGdataPtr->startVertex[j]) {
          i0 = j;
          break;
        }
      }
      k--;
    }

    if ((i0 == -1) && (k == 0)) {
      i0 = 0;
    }

    for (j = i0; j < numEdgesPlaced; j++) {
      if (i == GPtr->numVertices - 1) {
        break;
      }
      if (i != SDGdataPtr->startVertex[j]) {
        if ((j > 0) && (i == SDGdataPtr->startVertex[j - 1])) {
          if (j - i0 >= 1) {
            long ii = GPtr->outVertexIndex[i];
            ULONGINT_T r = 0;
            GPtr->paralEdgeIndex[ii] = i0;
            GPtr->outVertexList[ii] = SDGdataPtr->endVertex[i0];
            r++;
            ULONGINT_T t;
            for (t = i0 + 1; t < j; t++) {
              if (SDGdataPtr->endVertex[t] != SDGdataPtr->endVertex[t - 1]) {
                GPtr->paralEdgeIndex[ii + r] = t;
                GPtr->outVertexList[ii + r] = SDGdataPtr->endVertex[t];
                r++;
              }
            }
          }
        }
        i0 = j;
        break;
      }
    } /* for j */

    if (i == GPtr->numVertices - 1) {
      ULONGINT_T r = 0;
      if (numEdgesPlaced - i0 >= 0) {
        long ii = GPtr->outVertexIndex[i];
        GPtr->paralEdgeIndex[ii + r] = i0;
        GPtr->outVertexList[ii + r] = SDGdataPtr->endVertex[i0];
        r++;
        ULONGINT_T t;
        for (t = i0 + 1; t < numEdgesPlaced; t++) {
          if (SDGdataPtr->endVertex[t] != SDGdataPtr->endVertex[t - 1]) {
            GPtr->paralEdgeIndex[ii + r] = t;
            GPtr->outVertexList[ii + r] = SDGdataPtr->endVertex[t];
            r++;
          }
        }
      }
    }

  } /* for i */

  thread_barrier_wait(0 /* non-breaking*/);

  if (myId == 0) {
    free(SDGdataPtr->startVertex);
    free(SDGdataPtr->endVertex);
    //LUKE - add additional dummy entries to pad to end of cache line
    int dummy_entries = 64 - sizeof (LONGINT_T);
    GPtr->inDegree = (LONGINT_T*) malloc((GPtr->numVertices + dummy_entries) * sizeof (LONGINT_T));
    assert(GPtr->inDegree);
    dummy_entries = 64 - sizeof (ULONGINT_T);
    GPtr->inVertexIndex = (ULONGINT_T*) malloc((GPtr->numVertices + dummy_entries) * sizeof (ULONGINT_T));
    assert(GPtr->inVertexIndex);
  }

  thread_barrier_wait(0 /* non-breaking*/);

  for (i = i_start; i < i_stop; i++) {
    GPtr->inDegree[i] = 0;
    GPtr->inVertexIndex[i] = 0;
  }

  /* A temp. array to store the inplied edges */
  ULONGINT_T* impliedEdgeList;
  if (myId == 0) {
    impliedEdgeList = (ULONGINT_T*) malloc(GPtr->numVertices * MAX_CLUSTER_SIZE * sizeof (ULONGINT_T));
    global_impliedEdgeList = impliedEdgeList;
  }

  thread_barrier_wait(0 /* non-breaking*/);

  impliedEdgeList = global_impliedEdgeList;

  createPartition(0, (GPtr->numVertices * MAX_CLUSTER_SIZE), myId, numThread, &i_start, &i_stop);

  for (i = i_start; i < i_stop; i++) {
    impliedEdgeList[i] = 0;
  }

  /*
   * An auxiliary array to store implied edges, in case we overshoot
   * MAX_CLUSTER_SIZE
   */

  ULONGINT_T** auxArr;
  if (myId == 0) {
    auxArr = (ULONGINT_T**) malloc(GPtr->numVertices * sizeof (ULONGINT_T*));
    assert(auxArr);
    global_auxArr = auxArr;
  }

  thread_barrier_wait(0 /* non-breaking*/);

  auxArr = global_auxArr;

  createPartition(0, GPtr->numVertices, myId, numThread, &i_start, &i_stop);

  for (i = i_start; i < i_stop; i++) {
    /* Inspect adjacency list of vertex i */
    for (j = GPtr->outVertexIndex[i]; j < (GPtr->outVertexIndex[i] + GPtr->outDegree[i]); j++) {
      ULONGINT_T v = GPtr->outVertexList[j];
      ULONGINT_T k;
      for (k = GPtr->outVertexIndex[v]; k < (GPtr->outVertexIndex[v] + GPtr->outDegree[v]); k++) {
        if (GPtr->outVertexList[k] == i) {
          break;
        }
      }
      if (k == GPtr->outVertexIndex[v] + GPtr->outDegree[v]) {
        pthread_mutex_lock(&global_lock);
        /* Add i to the impliedEdgeList of v */
        long inDegree = (long) TM_SHARED_READ(GPtr->inDegree[v]);
        TM_SHARED_WRITE(GPtr->inDegree[v], (inDegree + 1));
        if (inDegree < MAX_CLUSTER_SIZE) {
          TM_SHARED_WRITE(impliedEdgeList[v * MAX_CLUSTER_SIZE + inDegree], i);
        } else {
          /* Use auxiliary array to store the implied edge */
          /* Create an array if it's not present already */
          ULONGINT_T* a = NULL;
          if ((inDegree % MAX_CLUSTER_SIZE) == 0) {
            a = (ULONGINT_T*) malloc(MAX_CLUSTER_SIZE * sizeof (ULONGINT_T));
            assert(a);
            TM_SHARED_WRITE_P(auxArr[v], a);
          } else {
            a = auxArr[v];
          }
          TM_SHARED_WRITE(a[inDegree % MAX_CLUSTER_SIZE], i);
        }
        pthread_mutex_unlock(&global_lock);
      }
    }
  } /* for i */

  thread_barrier_wait(0 /* non-breaking*/);

  /* RIC cambio prefix_sums por su contenido */
  //prefix_sums(GPtr->inVertexIndex, GPtr->inDegree, GPtr->numVertices);
  //                  result            ,   input,     arratSize
  p = NULL;
  if (myId == 0) {
    p = (ULONGINT_T*) malloc(NOSHARE(numThread) * sizeof (ULONGINT_T));
    assert(p);
    global_p = p;
  }

  thread_barrier_wait(0 /* non-breaking*/);

  p = global_p;

  r = GPtr->numVertices / numThread;
  start = myId * r + 1;
  end = (myId + 1) * r;
  if (myId == (numThread - 1)) {
    end = GPtr->numVertices;
  }

  for (j = start; j < end; j++) {
    GPtr->inVertexIndex[j] = GPtr->inDegree[j - 1] + GPtr->inVertexIndex[j - 1];
  }

  p[NOSHARE(myId)] = GPtr->inVertexIndex[end - 1];

  thread_barrier_wait(0 /* non-breaking*/);

  if (myId == 0) {
    for (j = 1; j < numThread; j++) {
      p[NOSHARE(j)] += p[NOSHARE(j - 1)];
    }
  }

  thread_barrier_wait(0 /* non-breaking*/);

  if (myId > 0) {
    ULONGINT_T add_value = p[NOSHARE(myId - 1)];
    for (j = start - 1; j < end; j++) {
      GPtr->inVertexIndex[j] += add_value;
    }
  }

  thread_barrier_wait(0 /* non-breaking*/);

  if (myId == 0) {
    free(p);
  }
  /* RIC fin prefix_sum */


  if (myId == 0) {
    GPtr->numUndirectedEdges = GPtr->inVertexIndex[GPtr->numVertices - 1] + GPtr->inDegree[GPtr->numVertices - 1];
    //LUKE - add additional dummy entries to pad to end of cache line
    int dummy_entries = 64 - sizeof (ULONGINT_T);
    GPtr->inVertexList = (ULONGINT_T *) malloc((GPtr->numUndirectedEdges + dummy_entries) * sizeof (ULONGINT_T));
  }

  thread_barrier_wait(0 /* non-breaking*/);

  /*
   * Create the inVertex List
   */

  for (i = i_start; i < i_stop; i++) {
    for (j = GPtr->inVertexIndex[i]; j < (GPtr->inVertexIndex[i] + GPtr->inDegree[i]); j++) {
      if ((j - GPtr->inVertexIndex[i]) < MAX_CLUSTER_SIZE) {
        GPtr->inVertexList[j] = impliedEdgeList[i * MAX_CLUSTER_SIZE + j - GPtr->inVertexIndex[i]];
      } else {
        GPtr->inVertexList[j] = auxArr[i][(j - GPtr->inVertexIndex[i]) % MAX_CLUSTER_SIZE];
      }
    }
  }

  thread_barrier_wait(0 /* non-breaking*/);

  if (myId == 0) free(impliedEdgeList);

  for (i = i_start; i < i_stop; i++) {
    if (GPtr->inDegree[i] > MAX_CLUSTER_SIZE) {
      free(auxArr[i]);
    }
  }

  thread_barrier_wait(0 /* non-breaking*/);

  if (myId == 0) {
    free(auxArr);
  }

  TM_THREAD_EXIT();
}


/* =============================================================================
 *
 * End of computeGraph.c
 *
 * =============================================================================
 */
