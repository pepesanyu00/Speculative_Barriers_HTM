#ifndef SB_TM_H
#define SB_TM_H 1

#include "rtmIntel.h"

//RIC

/* High level functions used by HTM */

/**
 * Initialize a transactional thread. This function must be called once
 * from each thread that performs transactional operations, before the
 * thread calls any other functions of the library.
 */
#define TX_DESCRIPTOR_INIT()        tm_tx_t tx;                                 \
                                    /* Initialise tx descriptor */              \
                                    tx.order = 1; /* In p8-ordwsb-nt the order updates in barriers, not in start nor commit */ \
                                    tx.retries = 0;                             \
                                    tx.specMax = MAX_SPEC;                      \
                                    tx.specLevel = tx.specMax;                  \
                                    tx.speculative = 0;                         \
                                    tx.status = 0

#define BARRIER_DESCRIPTOR_INIT(numTh) g_specvars.barrier.nb_threads = numTh;   \
                                       g_specvars.barrier.remain     = numTh






        /**
         * Start a transaction.
         *
         * @param readonly
         *   Specifies if transaction will be read only, allowing some optimizations.
         *
         * @return
         *   None.
         */
#define TM_START(thId, xId)                                                         \
  do{                                                                               \
    assert(xId != SPEC_XACT_ID); /* Me aseguro de que no tiene mismo id que sb*/    \
    if(!tx.speculative) {                                                           \
      __label__ __p_failure;                                                        \
      __p_failure:                                                                  \
      if(tx.retries) profileAbortStatus(tx.status, thId, xId);                      \
      tx.retries++;                                                                 \
      if (tx.retries > MAX_RETRIES) {                                               \
        unsigned int myticket = __sync_add_and_fetch(&(g_fallback_lock.ticket), 1); \
        while (myticket != g_ticketlock.turn)                                       \
          CPU_RELAX();                                                              \
        break;                                                                      \
      }                                                                             \
    }                                                                               \
    while (g_ticketlock.ticket >= g_ticketlock.turn)                                \
      CPU_RELAX(); /* Avoid Lemming effect */                                       \
  }while((__p_status = _xbegin()) != _XBEGIN_STARTED)                               


/**
 * Try to commit a transaction. If successful, the function returns.
 * Otherwise, execution continues at tm_start fallback branch.
 */
#define TM_STOP(thId, xId)                                                      \
      if(!tx.speculative) {                                                     \
        if (tx.retries <= MAX_RETRIES) {                                        \
          _xend();                                                    \
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
          _xend();                                                    \
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
            _xend();                                                  \
            profileCommit(thId, SPEC_XACT_ID, tx.retries-1);                    \
            /* Restore metadata */                                              \
            tx.speculative = 0;                                                 \
            tx.retries = 0;                                                     \
            tx.specLevel = tx.specMax;                                          \
          }                                                                     \
        }                                                                       \
      }



/* El problema es que la sincronizacion se hacía en el commit, asi que si por alguna 
 * razon la iteracion NO EJECUTABA NINGUNA TRANSACCION (cosa que pasa aqui en las 
 * ultimas iteraciones) podia reentrar a la barrera, decrementarla de nuevo y actualizar
 * el orden global mientras el otro hilo estaba ocupado. Con lo cual cuanto el otro
 * hilo llegaba al commit se encontraba con que el orden global ya le habia adelantando */
#define SB_BARRIER(thId)                                                        \
  /* Now, barrier IS SAFE if a thread re-enters the barrier before it have been reset */ \
  if (tx.speculative) {                                                         \
    BEGIN_ESCAPE;                                                               \
    while (tx.order > g_specvars.tx_order);                                     \
    END_ESCAPE;                                                                 \
    /* Llegados a este punto estoy en modo especulativo, en la OUTER tx, que
     * ha llegado a la barrera por segunda vez después de haber terminado de
     * especular con algunas transacciones tras pasar esta misma barrera antes
     * y ponerse en modo especulativo. Lo que se hace aquí es un commit para 
     * intentar salirnos de la OUTER que abrimos la primera vez que pasamos 
     * con la barrera, y desactivar el modo especulativo porque con el while
     * anterior sabemos que el resto de txs atraveso ya la barrera anterior */  \
    _xend();                                                          \
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
__p_failure:                                                                    \
    if(tx.retries) profileAbortStatus(tx.status, thId, SPEC_XACT_ID);      \
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
      while (g_fallback_lock.ticket >= g_fallback_lock.turn);                   \
      if(_xbegin() != _XBEGIN_STARTED) goto __p_failure;                                \
      if (g_fallback_lock.ticket >= g_fallback_lock.turn)                       \
      _xabort(LOCK_TAKEN);/*Early subscription*/                       \
    }                                                                           \
  }

/* RIC hay que poner una última barrera que no especule antes de salir del thread
 */
#define LAST_BARRIER(thId)                                                      \
  /* Now, barrier IS SAFE if a thread re-enters the barrier before it have been reset */ \
  if (tx.speculative) {                                                         \
    BEGIN_ESCAPE;                                                               \
    while (tx.order > g_specvars.tx_order);                                     \
    END_ESCAPE;                                                                 \
    /* Llegados a este punto estoy en modo especulativo, en la OUTER tx, que
     * ha llegado a la barrera por segunda vez después de haber terminado de
     * especular con algunas transacciones tras pasar esta misma barrera antes
     * y ponerse en modo especulativo. Lo que se hace aquí es un commit para 
     * intentar salirnos de la OUTER que abrimos la primera vez que pasamos 
     * con la barrera, y desactivar el modo especulativo porque con el while
     * anterior sabemos que el resto de txs atraveso ya la barrera anterior */  \
    _xend();                                                          \
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

//RIC definido en transaction.c
extern g_spec_vars_t g_specvars;

#endif  /* SB_TM_H */
