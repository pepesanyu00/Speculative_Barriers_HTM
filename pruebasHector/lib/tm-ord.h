#ifndef SB_TM_H
#define SB_TM_H 1

#include "transaction.h"

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
                                    tx.speculative = 0

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
#define TM_START(thId, xId)                                                     \
{                                                                               \
  __label__ __p_failure;                                                        \
  texasru_t __p_abortCause;                                                     \
__p_failure:                                                                    \
  __p_abortCause = __builtin_get_texasru ();                                    \
  if(tx.retries) profileAbortStatus(__p_abortCause, thId, xId);                 \
  tx.retries++;                                                                 \
  while (g_fallback_lock.ticket >= g_fallback_lock.turn);                       \
  if(!__builtin_tbegin(0)) goto __p_failure;                                    \
  if (g_fallback_lock.ticket >= g_fallback_lock.turn)                           \
    __builtin_tabort(LOCK_TAKEN);/*Early subscription*/                         \
}

/**
 * Try to commit a transaction. If successful, the function returns.
 * Otherwise, execution continues at tm_start fallback branch.
 */
#define TM_STOP(thId, xId, ord)                                                 \
{                                                                               \
  if (tx.retries <= MAX_RETRIES) {                                              \
    BEGIN_ESCAPE;                                                               \
    while (ord > g_specvars.tx_order);                                          \
    END_ESCAPE;                                                                 \
    __builtin_tend(0);                                                          \
    __sync_add_and_fetch(&(g_specvars.tx_order), 1);                            \
    profileCommit(thId, xId, tx.retries-1);                                     \
  } else {                                                                      \
    __sync_add_and_fetch(&(g_fallback_lock.turn), 1);                           \
    profileFallback(thId, xId, tx.retries-1);                                   \
  }                                                                             \
}

//RIC definido en transaction.c
extern g_spec_vars_t g_specvars;

#endif  /* SB_TM_H */
