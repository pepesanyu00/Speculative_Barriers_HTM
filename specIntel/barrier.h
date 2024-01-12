#include <pthread.h>
#include <assert.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


#include <immintrin.h>
#include <stdint.h>


//RIC
#define LOCK_TAKEN 0xFF
#define VALIDATION_ERROR 0xFE

#define MAX_THREADS 128
#define MAX_SPEC    4
#define MAX_RETRIES 5

//RIC defino el número máximo de identificadores de transacciones que tendremos
//en los benchmarcs y el identificador de la transacción abierta por la barrera
//especulativa. Se utiliza en las estadísticas
#define MAX_XACT_IDS 4
#define SPEC_XACT_ID MAX_XACT_IDS-1

#define CACHE_BLOCK_SIZE 128

#define BEGIN_ESCAPE _xsusldtrk()
#define END_ESCAPE _xresldtrk()


/* Transaction descriptor. It is aligned (including stats) to CACHELINE_SIZE
 * to avoid aliases with other threads metadata */
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
  uint32_t status;  /* Transaction status.*/
  uint8_t pad2[CACHE_BLOCK_SIZE-sizeof(uint32_t)*3-sizeof(uint8_t)];
} __attribute__ ((aligned (CACHE_BLOCK_SIZE))) tm_tx_t;

/* Transactional barrier descriptor */
typedef struct barrier {
  int nb_threads; /* Number of threads to wait in the barrier */
  volatile uint32_t remain; /* Remaining threads until unblock */
} barrier_t;

//RIC creo una estructura para colocar el global tx order y la barrera
typedef struct global_spec_vars {
  volatile uint32_t tx_order; //Tiene que ser inicializado a 1
  uint8_t pad1[CACHE_BLOCK_SIZE-sizeof(uint32_t)];
  barrier_t barrier;
  uint8_t pad2[CACHE_BLOCK_SIZE-sizeof(barrier_t)];
} __attribute__ ((aligned (CACHE_BLOCK_SIZE))) g_spec_vars_t;

typedef struct fback_lock {
  //RIC Para implementar el spinlock del fallback de Haswell
  volatile uint32_t ticket;
  volatile uint32_t turn;
  uint8_t pad[CACHE_BLOCK_SIZE-sizeof(uint32_t)*2];
} __attribute__ ((aligned (CACHE_BLOCK_SIZE))) fback_lock_t;

extern fback_lock_t g_fallback_lock;
extern pthread_mutex_t global_lock;
extern volatile uint32_t g_lock_var;