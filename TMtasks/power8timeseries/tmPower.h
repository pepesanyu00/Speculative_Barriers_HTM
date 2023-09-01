#ifndef TRANSACTION_H_
#define TRANSACTION_H_

//RIC mete definiciones de tipos y códigos de error de HTM de power8
#include <htmintrin.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unistd.h> //For getpid(), used to get the pid to generate a unique filename
#include <iostream>

//RIC
#define LOCK_TAKEN 0xFF
#define VALIDATION_ERROR 0xFE

#define MAX_THREADS 128
#define MAX_SPEC 4
#define MAX_RETRIES 3

//RIC defino el número máximo de identificadores de transacciones que tendremos
//en los benchmarks y el identificador de la transacción abierta por la barrera
//especulativa. Se utiliza en las estadísticas
#define MAX_XACT_IDS 4
#define SPEC_XACT_ID MAX_XACT_IDS - 1

#define CACHE_BLOCK_SIZE 128

#define BEGIN_ESCAPE __builtin_tsuspend()
#define END_ESCAPE __builtin_tresume()

#define TAREA(param) function(asdfa)

#define INIT_TRANSACTION() \
  uint32_t __p_retries

//Con ticket lock
#define BEGIN_TRANSACTION(thId, xId)                                                           \
  __p_retries = 0;                                                                             \
  do                                                                                           \
  {                                                                                            \
    if (__p_retries)                                                                           \
      profileAbortStatus(__builtin_get_texasru(), thId, xId);                                  \
    __p_retries++;                                                                             \
    if (__p_retries > MAX_RETRIES)                                                             \
    {                                                                                          \
      /*uint32_t myticket = __sync_add_and_fetch(&(g_fallback_lock.ticket), 1); */             \
      uint32_t myticket = __atomic_add_fetch(&(g_fallback_lock.ticket), 1, __ATOMIC_SEQ_CST); \
      while (myticket != g_fallback_lock.turn)                                                 \
        ;                                                                                      \
      break;                                                                                   \
    }                                                                                          \
    while (g_fallback_lock.ticket >= g_fallback_lock.turn)                                     \
      ;                                                                                        \
  } while (!__builtin_tbegin(0))

#define COMMIT_TRANSACTION(thId, xId)                                 \
  if (__p_retries <= MAX_RETRIES)                                     \
  {                                                                   \
    if (g_fallback_lock.ticket >= g_fallback_lock.turn)               \
      __builtin_tabort(LOCK_TAKEN); /*Early subscription*/            \
    __builtin_tend(0);                                                \
    profileCommit(thId, xId, __p_retries - 1);                        \
  }                                                                   \
  else                                                                \
  {                                                                   \
    /* __sync_add_and_fetch(&(g_fallback_lock.turn), 1); */           \
    __atomic_add_fetch(&(g_fallback_lock.turn), 1, __ATOMIC_SEQ_CST); \
    profileFallback(thId, xId, __p_retries - 1);                      \
  }

using namespace std;

/* Transaction descriptor. It is aligned (including stats) to CACHELINE_SIZE
 * to avoid aliases with other threads metadata */
typedef struct tm_tx
{
  uint32_t order; /* Order of the transaction relative to gClock. This is
                             * updated only in non-xact mode (in tm_barrier) and is
                             * read in non-xact mode (abort) or xact. suspended mode. */
  uint8_t pad1[CACHE_BLOCK_SIZE - sizeof(uint32_t)];
  uint32_t retries;    /* Number of retries remaining until switch to fallback.
                             * This is read & updated only in non-xact mode (abort
                             * and after a successful commit) */
  uint8_t speculative; /* True if transaction is in speculative mode. This is
                        * read in xact. mode (tm_commit) and non-xact mode
                        * (tm_start, abort) and it is updated in non-xact.
                        * mode (outside the xact.) in aborts and tm_commit 
                        * and upon reach a tm_barrier to switch to speculative */
  uint32_t specMax;    /* Max level of speculation before wait for commit. This
                    * is updated only in non-xact mode (after a series of
                    * aborts in speculative mode). */
  uint32_t specLevel;  /* Number of transactions remaining until wait for commit.
                             * This is decremented in xact. mode when a nested xact.
                             * reaches commit but have to remain in speculative mode.
                             * It is also reset after a successful commit. */
  uint8_t pad2[CACHE_BLOCK_SIZE - sizeof(uint32_t) * 3 - sizeof(uint8_t)];
} __attribute__((aligned(CACHE_BLOCK_SIZE))) tm_tx_t;

/* Transactional barrier descriptor */
typedef struct barrier
{
  int nb_threads;           /* Number of threads to wait in the barrier */
  volatile uint32_t remain; /* Remaining threads until unblock */
} barrier_t;

//RIC creo una estructura para colocar el global tx order y la barrera
typedef struct global_spec_vars
{
  volatile uint32_t tx_order; //Tiene que ser inicializado a 1
  uint8_t pad1[CACHE_BLOCK_SIZE - sizeof(uint32_t)];
  barrier_t barrier;
  uint8_t pad2[CACHE_BLOCK_SIZE - sizeof(barrier_t)];
} __attribute__((aligned(CACHE_BLOCK_SIZE))) g_spec_vars_t;

typedef struct fback_lock
{
  //RIC Para implementar el spinlock del fallback de Haswell
  volatile uint32_t ticket;
  volatile uint32_t turn;
  uint8_t pad[CACHE_BLOCK_SIZE - sizeof(uint32_t) * 2];
} __attribute__((aligned(CACHE_BLOCK_SIZE))) fback_lock_t;

extern fback_lock_t g_fallback_lock;

//Funciones para el fichero de estadísticas
int statsFileInit(int argc, char **argv, long thCount);
void profileCommit(long thread, long xid, long retries);
void profileFallback(long thread, long xid, long retries);
unsigned long profileAbortStatus(texasru_t cause, long thread, long xid);
int dumpStats();

#endif
