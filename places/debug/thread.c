/* =============================================================================
 *
 * thread.c
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Author: Chi Cao Minh
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
#include "thread.h"
#include "types.h"
//RIC Lo cambio por tm.h para el secuencial
//#include "transaction.h"
#include "tm.h"

static THREAD_LOCAL_T global_threadId;
static long global_numThread = 1;
static THREAD_BARRIER_T* global_barrierPtr = NULL;
static long* global_threadIds = NULL;
static THREAD_ATTR_T global_threadAttr;
static THREAD_T* global_threads = NULL;
static void (*global_funcPtr)(void*) = NULL;
static void* global_argPtr = NULL;
static volatile bool_t global_doShutdown = FALSE;

int stick_this_thread_to_core(int core_id) {
  int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
  if (core_id < 0 || core_id >= num_cores)
    return 0;
  int destination;
  //Power8 tiene dos nodos NUMA de 10 cores, cada uno con 8 threads = 160 threads
  //Asigno de 8 en 8 para que no haya multithreading si el número de threads
  //es menor que 20
  //mod(a .* 8,160) + floor((a .* 8)/160)
  destination = ((core_id * 8) % 160) + (core_id * 8) / 160;

  printf("Binding thread %d to core %d out of %d\n", core_id, destination, num_cores);
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(destination, &cpuset);

  pthread_t current_thread = pthread_self();
  return !pthread_setaffinity_np(current_thread, sizeof (cpu_set_t), &cpuset);
}

/* =============================================================================
 * threadWait
 * -- Synchronizes all threads to start/stop parallel section
 * =============================================================================
 */
static void
threadWait(void* argPtr) {
  TM_BEGIN_ESCAPE();
  long threadId = *(long*) argPtr;
  THREAD_LOCAL_SET(global_threadId, (long) threadId);
  TM_END_ESCAPE();

  //RIC pongo aquí el pthread_setaffinity para los otros threads, no el main,
  //para que no se contabilice en la toma de tiempos
  if (threadId != 0) assert(stick_this_thread_to_core(threadId));

  while (1) {
    TM_BEGIN_ESCAPE();
    THREAD_BARRIER(global_barrierPtr, threadId); /* wait for start parallel */
    if (global_doShutdown) {
      TM_END_ESCAPE();
      break;
    }
    TM_END_ESCAPE();
    global_funcPtr(global_argPtr);
    TM_BEGIN_ESCAPE();
    THREAD_BARRIER(global_barrierPtr, threadId); /* wait for end parallel */
    TM_END_ESCAPE();
    if (threadId == 0) {
      break;
    }
  }
}

/* =============================================================================
 * thread_startup
 * -- Create pool of secondary threads
 * -- numThread is total number of threads (primary + secondaries)
 * =============================================================================
 */
void
thread_startup(long numThread) {
  long i;

  global_numThread = numThread;
  global_doShutdown = FALSE;

  /* Set up barrier */
  assert(global_barrierPtr == NULL);
  global_barrierPtr = THREAD_BARRIER_ALLOC(numThread);
  assert(global_barrierPtr);
  THREAD_BARRIER_INIT(global_barrierPtr, numThread);

  /* Set up ids */
  THREAD_LOCAL_INIT(global_threadId);
  assert(global_threadIds == NULL);
  global_threadIds = (long*) malloc(numThread * sizeof (long));
  assert(global_threadIds);
  for (i = 0; i < numThread; i++) {
    global_threadIds[i] = i;
  }

  /* Set up thread list */
  assert(global_threads == NULL);
  global_threads = (THREAD_T*) malloc(numThread * sizeof (THREAD_T));
  assert(global_threads);

  //RIC Ato el main al core 0. Los otros se atarán en threadWait
  assert(stick_this_thread_to_core(0));

  /* Set up pool */
  THREAD_ATTR_INIT(global_threadAttr);
  for (i = 1; i < numThread; i++) {
    THREAD_CREATE(global_threads[i],
            global_threadAttr,
            &threadWait,
            &global_threadIds[i]);
  }

  /*
   * Wait for primary thread to call thread_start
   */
}

/* =============================================================================
 * thread_start
 * -- Make primary and secondary threads execute work
 * -- Should only be called by primary thread
 * -- funcPtr takes one arguments: argPtr
 * =============================================================================
 */
void
thread_start(void (*funcPtr)(void*), void* argPtr) {
  global_funcPtr = funcPtr;
  global_argPtr = argPtr;

  long threadId = 0; /* primary */
  threadWait((void*) &threadId);
}

/* =============================================================================
 * thread_shutdown
 * -- Primary thread kills pool of secondary threads
 * =============================================================================
 */
void
thread_shutdown() {
  /* Make secondary threads exit wait() */
  global_doShutdown = TRUE;
  THREAD_BARRIER(global_barrierPtr, 0);

  long numThread = global_numThread;

  long i;
  for (i = 1; i < numThread; i++) {
    THREAD_JOIN(global_threads[i]);
  }

  THREAD_BARRIER_FREE(global_barrierPtr);
  global_barrierPtr = NULL;

  free(global_threadIds);
  global_threadIds = NULL;

  free(global_threads);
  global_threads = NULL;

  global_numThread = 1;
}

/* =============================================================================
 * thread_barrier_alloc
 * =============================================================================
 */
thread_barrier_t*
thread_barrier_alloc(long numThread) {
  thread_barrier_t* barrierPtr;

  assert(numThread > 0);
  //RIC comento este assert, para que funcione con LOCK
  //#ifndef LOGTM
  // LOGTM doesn't use logarithmic barrier code
  //    assert((numThread & (numThread - 1)) == 0); /* must be power of 2 */
  //#endif
  barrierPtr = (thread_barrier_t*) malloc(numThread * sizeof (thread_barrier_t));
  assert(barrierPtr != NULL);
  if (barrierPtr != NULL) {
    barrierPtr->numThread = numThread;
  }

  return barrierPtr;
}

/* =============================================================================
 * thread_barrier_free
 * =============================================================================
 */
void
thread_barrier_free(thread_barrier_t* barrierPtr) {
  free(barrierPtr);
}

/* =============================================================================
 * thread_barrier_init
 * =============================================================================
 */
void
thread_barrier_init(thread_barrier_t* barrierPtr) {
  long i;
  long numThread = barrierPtr->numThread;

  for (i = 0; i < numThread; i++) {
    barrierPtr[i].count = 0;
    //LOGTM - start local sense at 0
    barrierPtr[i].local_sense = 0;
    THREAD_MUTEX_INIT(barrierPtr[i].countLock);
    THREAD_COND_INIT(barrierPtr[i].proceedCond);
    THREAD_COND_INIT(barrierPtr[i].proceedAllCond);
  }
}

/* =============================================================================
 * thread_barrier
 * -- Simple logarithmic barrier
 * =============================================================================
 */
void
thread_barrier(thread_barrier_t* barrierPtr, long threadId) {
  long i = 2;
  long base = 0;
  long index;
  long numThread = barrierPtr->numThread;

  if (numThread < 2) {
    return;
  }

  do {
    index = base + threadId / i;
    if ((threadId % i) == 0) {
      THREAD_MUTEX_LOCK(barrierPtr[index].countLock);
      barrierPtr[index].count++;
      while (barrierPtr[index].count < 2) {
        THREAD_COND_WAIT(barrierPtr[index].proceedCond,
                barrierPtr[index].countLock);
      }
      THREAD_MUTEX_UNLOCK(barrierPtr[index].countLock);
    } else {
      THREAD_MUTEX_LOCK(barrierPtr[index].countLock);
      barrierPtr[index].count++;
      if (barrierPtr[index].count == 2) {
        THREAD_COND_SIGNAL(barrierPtr[index].proceedCond);
      }
      while (THREAD_COND_WAIT(barrierPtr[index].proceedAllCond,
              barrierPtr[index].countLock) != 0) {
        /* wait */
      }
      THREAD_MUTEX_UNLOCK(barrierPtr[index].countLock);
      break;
    }
    base = base + numThread / i;
    i *= 2;
  } while (i <= numThread);

  for (i /= 2; i > 1; i /= 2) {
    base = base - numThread / i;
    index = base + threadId / i;
    THREAD_MUTEX_LOCK(barrierPtr[index].countLock);
    barrierPtr[index].count = 0;
    THREAD_COND_SIGNAL(barrierPtr[index].proceedAllCond);
    THREAD_MUTEX_UNLOCK(barrierPtr[index].countLock);
  }
}

/* =============================================================================
 * thread_barrier_breaking
 * -- Uses LogTM's transaction.c Barrier()
 * =============================================================================
 */
void
thread_barrier_breaking(thread_barrier_t* barrierPtr, long threadId) {
  long numThread = barrierPtr->numThread;

  // use transaction.c's barrier code...
  printf("thread_barrier_breaking %ld of %ld entering barrier\n",threadId, numThread);
  //RIC, en power 8 no hay breaking que valga
  //Barrier(&(barrierPtr[threadId].local_sense), threadId, numThread);
  assert(FALSE);
}

/* =============================================================================
 * thread_getId
 * -- Call after thread_start() to get thread ID inside parallel region
 * =============================================================================
 */
long
thread_getId() {
  TM_BEGIN_ESCAPE();
  long id = (long) THREAD_LOCAL_GET(global_threadId);
  TM_END_ESCAPE();
  return id;
}

/* =============================================================================
 * thread_getNumThread
 * -- Call after thread_start() to get number of threads inside parallel region
 * =============================================================================
 */
long
thread_getNumThread() {
  return global_numThread;
}

/* =============================================================================
 * thread_barrier_wait
 * -- Call after thread_start() to synchronize threads inside parallel region
 * =============================================================================
 */
void
thread_barrier_wait(int breaking) {
  long threadId = thread_getId();

  TM_BEGIN_ESCAPE();
  if (!breaking) {
    // non-breaking barrier
    THREAD_BARRIER(global_barrierPtr, threadId);
  } else {
    THREAD_BARRIER_BREAKING(global_barrierPtr, threadId);
  }
  TM_END_ESCAPE();
}


/* =============================================================================
 * TEST_THREAD
 * =============================================================================
 */
#ifdef TEST_THREAD


#include <stdio.h>
#include <unistd.h>


#define NUM_THREADS    (4)
#define NUM_ITERATIONS (3)

void
printId(void* argPtr) {
  long threadId = thread_getId();
  long numThread = thread_getNumThread();
  long i;

  for (i = 0; i < NUM_ITERATIONS; i++) {
    thread_barrier_wait();
    if (threadId == 0) {
      sleep(1);
    } else if (threadId == numThread - 1) {
      usleep(100);
    }
    printf("i = %li, tid = %li\n", i, threadId);
    if (threadId == 0) {
      puts("");
    }
    fflush(stdout);
  }
}

int
main() {
  puts("Starting...");

  /* Run in parallel */
  thread_startup(NUM_THREADS);
  /* Start timing here */
  thread_start(printId, NULL);
  thread_start(printId, NULL);
  thread_start(printId, NULL);
  /* Stop timing here */
  thread_shutdown();

  puts("Done.");

  return 0;
}


#endif /* TEST_THREAD */


/* =============================================================================
 *
 * End of thread.c
 *
 * =============================================================================
 */
