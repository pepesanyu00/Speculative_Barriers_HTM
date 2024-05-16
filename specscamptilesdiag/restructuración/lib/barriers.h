#ifndef BARRIERS_H
#define BARRIERS_H

#include <pthread.h>
#include <assert.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "htmintrin.h"

#define LOCK_TAKEN 0xFF
#define VALIDATION_ERROR 0xFE
#define CACHE_BLOCK_SIZE 128

#define MAX_THREADS 128
#define MAX_SPEC    2
#define MAX_RETRIES 5
#define MAX_CAPACITY_RETRIES 4

// Esta macro siempre debe coincidir con el número de transacciones(xacts) que se pasen a statsFileInit, sino las estadísticas estarán mal.
#define MAX_XACT_IDS 1

/* Esta macro se define para indicar que transacción es la correspondiente a la barrera,
   para que se puedan definir transacciones junto con la barrera y que esta sea la última
   en el fichero de estadísticas */
#define SPEC_XACT_ID MAX_XACT_IDS-1

/* Macros para hacer una transacción escapada (para poder leer y escribir variables que normalmente darían aborto por conflicto)
   dentro de una transaccion */
#define BEGIN_ESCAPE __builtin_tsuspend()
#define END_ESCAPE __builtin_tresume()

/* inicializa las variables necesarias para implementar una transacción, debe ser llamada una vez por cada thread 
   al principio del bloque paralelo  */
#define TX_DESCRIPTOR_INIT()        tm_tx_t tx;                                 \
                                    /* Initialise tx descriptor */              \
                                    tx.order = 1; /* In p8-ordwsb-nt the order updates in barriers, not in start nor commit */ \
                                    tx.retries = 0;                             \
                                    tx.specMax = MAX_SPEC;                      \
                                    tx.specLevel = tx.specMax;                  \
                                    tx.speculative = 0;                         \
                                    tx.capRetries = 0


// Inicializa las variables globales necesarias para las barreras
#define BARRIER_DESCRIPTOR_INIT(numTh) g_specvars.barrier.nb_threads = numTh;   \
                                       g_specvars.barrier.remain     = numTh


// Empieza una transacción
#define BEGIN_TRANSACTION(thId, xId)                                                     \
  assert(xId != SPEC_XACT_ID); /* Me aseguro de que no tiene mismo id que sb*/  \
  if(!tx.speculative) {                                                         \
    __label__ __p_failure;                                                      \
    texasru_t __p_abortCause;                                                   \
__p_failure:                                                                    \
    __p_abortCause = __builtin_get_texasru ();                                  \
    if(tx.retries) profileAbortStatus(__p_abortCause, thId, xId);               \
    tx.retries++;                                                               \
    if (tx.retries > MAX_RETRIES) {                                             \
      unsigned int myticket = __sync_add_and_fetch(&(g_fallback_lock.ticket), 1); \
      while(myticket != g_fallback_lock.turn) ;                                 \
    } else {                                                                    \
      while (g_fallback_lock.ticket >= g_fallback_lock.turn);                   \
      if(!__builtin_tbegin(0)) goto __p_failure;                                \
      if (g_fallback_lock.ticket >= g_fallback_lock.turn)                       \
      __builtin_tabort(LOCK_TAKEN);/*Early subscription*/                       \
    }                                                                           \
  }

// commita una transacción
#define TM_STOP(thId, xId)                                                      \
      if(!tx.speculative) {                                                     \
        if (tx.retries <= MAX_RETRIES) {                                        \
          __builtin_tend(0);                                                    \
          profileCommit(thId, xId, tx.retries-1);                               \
        } else {                                                                \
          __sync_add_and_fetch(&(g_fallback_lock.turn), 1);                     \
          profileFallback(thId, xId, tx.retries-1);                             \
        }                                                                       \
        tx.retries = 0;                                                         \
        tx.specLevel = tx.specMax;                                              \
      } else { /* RIC ver si podemos hacer commit de la especulativa */         \
        BEGIN_ESCAPE;                                                           \
        if (tx.order <= g_specvars.tx_order) {                                  \
          END_ESCAPE;                                                           \
          __builtin_tend(0);                                                    \
          profileCommit(thId, SPEC_XACT_ID, tx.retries-1); /* ID de la xact especulativa abierta en SB_BARRIER*/ \
          /* Restore metadata */                                                \
          tx.speculative = 0;                                                   \
          tx.retries = 0;                                                       \
          tx.specLevel = tx.specMax;                                            \
        } else {                                                                \
          END_ESCAPE;                                                           \
          /* If we can not, decrement our speclevel */                          \
          tx.specLevel--;                                                       \
          if (tx.specLevel == 0) {                                              \
            BEGIN_ESCAPE;                                                       \
            while (tx.order > g_specvars.tx_order);                             \
            END_ESCAPE;                                                         \
            __builtin_tend(0);                                                  \
            profileCommit(thId, SPEC_XACT_ID, tx.retries-1);                    \
            /* Restore metadata */                                              \
            tx.speculative = 0;                                                 \
            tx.retries = 0;                                                     \
            tx.specLevel = tx.specMax;                                          \
          }                                                                     \
        }                                                                       \
      }


#define SB_BARRIER(thId)                                                        \
  /* Now, barrier IS SAFE if a thread re-enters the barrier before it have been reset */ \
  if (tx.speculative) {                                                         \
    BEGIN_ESCAPE;                                                               \
    while (tx.order > g_specvars.tx_order);                                     \
    END_ESCAPE;                                                                 \
    __builtin_tend(0);                                                          \
    profileCommit(thId, SPEC_XACT_ID, tx.retries-1);                            \
    /* Restore metadata */                                                      \
    tx.speculative = 0;                                                         \
    tx.retries = 0;                                                             \
    tx.specLevel = tx.specMax;                                                  \
  }                                                                             \
  /* We are now in speculative mode until global order increases */             \
  tx.order += 1;                                                                \
  /* Determine if thread is last to enter the barrier */                        \
  if (__sync_add_and_fetch(&(g_specvars.barrier.remain),-1) == 0) {             \
    /* If we are last thread to enter in barrier, reset & update global order.  
     * No switch to speculative necessary */                                    \
    g_specvars.barrier.remain = g_specvars.barrier.nb_threads;                  \
    /* This instruction involves a full memory fence, so the reset of barrier->remain 
     * should appear ordered to the rest of the threads */                      \
    __sync_add_and_fetch(&(g_specvars.tx_order), 1);                            \
  } else {                                                                      \
    __label__ __p_failure;                                                      \
    texasru_t __p_abortCause;                                                   \
__p_failure:                                                                    \
    __p_abortCause = __builtin_get_texasru ();                                  \
    if(tx.retries) profileAbortStatus(__p_abortCause, thId, SPEC_XACT_ID);      \
    tx.retries++;                                                               \
    if (tx.order <= g_specvars.tx_order) {                                      \
      tx.speculative = 0;                                                       \
      tx.retries = 0;                                                           \
      tx.specLevel = tx.specMax;                                                \
    } else {                                                                    \
      tx.speculative = 1;                                                       \
      if (tx.retries > MAX_RETRIES) {                                           \
        if(tx.specMax > 1) tx.specMax--;                                        \
        tx.specLevel = tx.specMax;                                              \
      }                                                                         \
      if(_TEXASRU_FAILURE_PERSISTENT(__p_abortCause)){                         \
          if (_TEXASRU_FOOTPRINT_OVERFLOW(__p_abortCause) && tx.capRetries < MAX_CAPACITY_RETRIES ){                     \
            tx.capRetries++;                                                    \
          } else{                                                             \
            while (tx.order > g_specvars.tx_order);                               \
            tx.speculative = 0;                                                   \
            tx.retries = 0;                                                       \
            tx.specLevel = tx.specMax;                                            \
          }                                                                     \
      } else {                                                                  \
        if(_TEXASRU_TRANSACTION_CONFLICT(__p_abortCause)){			                \
          /*srand(time(NULL));*/							                                      \
          /*usleep((rand() % 30));*/							                                  \
        }										                                                    \
        if(!__builtin_tbegin(0)) goto __p_failure;                                \
      }                                                                         \
    }                                                                           \
  }


#define LAST_BARRIER(thId)                                                      \
  /* Now, barrier IS SAFE if a thread re-enters the barrier before it have been reset */ \
  if (tx.speculative) {                                                         \
    BEGIN_ESCAPE;                                                               \
    while (tx.order > g_specvars.tx_order);                                     \
    END_ESCAPE;                                                                 \
    __builtin_tend(0);                                                          \
    profileCommit(thId, SPEC_XACT_ID, tx.retries-1);                            \
    /* Restore metadata */                                                      \
    tx.speculative = 0;                                                         \
    tx.retries = 0;                                                             \
    tx.specLevel = tx.specMax;                                                  \
  }                                                                             \
  /* We are now in speculative mode until global order increases */             \
  tx.order += 1;                                                                \
  /* Determine if thread is last to enter the barrier */                        \
  if (__sync_add_and_fetch(&(g_specvars.barrier.remain),-1) == 0) {             \
    /* If we are last thread to enter in barrier, reset & update global order.  
     * No switch to speculative necessary */                                    \
    g_specvars.barrier.remain = g_specvars.barrier.nb_threads;                  \
    /* This instruction involves a full memory fence, so the reset of barrier->remain 
     * should appear ordered to the rest of the threads */                      \
    __sync_add_and_fetch(&(g_specvars.tx_order), 1);                            \
  } else {                                                                      \
    while(tx.order > g_specvars.tx_order) ;                                     \
  }

#define CHECK_SPEC(thId)                                                      \
      if(tx.speculative) {                                                      \
        BEGIN_ESCAPE;                                                           \
        if (tx.order <= g_specvars.tx_order) {                                  \
          END_ESCAPE;                                                           \
          __builtin_tend(0);                                                    \
          profileCommit(thId, SPEC_XACT_ID, tx.retries-1); /* ID de la xact especulativa abierta en SB_BARRIER*/ \
          /* Restore metadata */                                                \
          tx.speculative = 0;                                                   \
          tx.retries = 0;                                                       \
          tx.specLevel = tx.specMax;                                            \
        } else {                                                                \
          END_ESCAPE;                                                           \
          /* If we can not, decrement our speclevel */                          \
            BEGIN_ESCAPE;                                                       \
            while (tx.order > g_specvars.tx_order);                             \
            END_ESCAPE;                                                         \
            __builtin_tend(0);                                                  \
            profileCommit(thId, SPEC_XACT_ID, tx.retries-1);                    \
            /* Restore metadata */                                              \
            tx.speculative = 0;                                                 \
            tx.retries = 0;                                                     \
            tx.specLevel = tx.specMax;                                          \
        }                                                                       \
      }

typedef struct fback_lock {
  //RIC Para implementar el spinlock del fallback de Haswell
  volatile uint32_t ticket;
  volatile uint32_t turn;
  uint8_t pad[CACHE_BLOCK_SIZE-sizeof(uint32_t)*2];
} __attribute__ ((aligned (CACHE_BLOCK_SIZE))) fback_lock_t;

// Declarado en stats.c
extern fback_lock_t g_fallback_lock;

/* Transaction descriptor. It is aligned (including stats) to CACHELINE_SIZE
 * to avoid aliases with other threads metadata */
// Con pad1 y pad2 se añade padding para prevenir el false sharing
typedef struct tm_tx {
  uint32_t order; /* Order of the transaction relative to gClock. This is
                             * updated only in non-xact mode (in tm_barrier) and is
                             * read in non-xact mode (abort) or xact. suspended mode. */
  uint8_t pad1[CACHE_BLOCK_SIZE-sizeof(uint32_t)];
  uint32_t retries; /* Number of retries remaining until switch to fallback.
                             * This is read & updated only in non-xact mode (abort
                             * and after a successful commit) */
  uint8_t speculative; /* True if transaction is in speculative mode. This is
                        * read in xact. mode (tm_commit) and non-xact mode
                        * (tm_start, abort) and it is updated in non-xact.
                        * mode (outside the xact.) in aborts and tm_commit 
                        * and upon reach a tm_barrier to switch to speculative */
  uint32_t specMax; /* Max level of speculation before wait for commit. This
                    * is updated only in non-xact mode (after a series of
                    * aborts in speculative mode). */
  uint32_t specLevel; /* Number of transactions remaining until wait for commit.
                             * This is decremented in xact. mode when a nested xact.
                             * reaches commit but have to remain in speculative mode.
                             * It is also reset after a successful commit. */
  uint32_t capRetries;
  uint8_t pad2[CACHE_BLOCK_SIZE-sizeof(uint32_t)*3-sizeof(uint8_t)];
} __attribute__ ((aligned (CACHE_BLOCK_SIZE))) tm_tx_t;

/* Descriptor de barrera transaccional */
typedef struct barrier {
  int nb_threads; /* Número de threads que esperan en la barrera */
  volatile uint32_t remain; /* Threads restantes hasta desbloquear la barrera */
} barrier_t;

//RIC creo una estructura para colocar el global tx order y la barrera
typedef struct global_spec_vars {
  volatile uint32_t tx_order; //Tiene que ser inicializado a 1
  uint8_t pad1[CACHE_BLOCK_SIZE-sizeof(uint32_t)];
  barrier_t barrier;
  uint8_t pad2[CACHE_BLOCK_SIZE-sizeof(barrier_t)];
} __attribute__ ((aligned (CACHE_BLOCK_SIZE))) g_spec_vars_t;

// Estructura que guarda las variables globales necesarias para las barreras
extern g_spec_vars_t g_specvars;


#endif
