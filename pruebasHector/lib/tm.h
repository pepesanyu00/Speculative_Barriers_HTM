#ifndef TM_H
#define TM_H 1

#include <stdio.h>
#include <assert.h>
#ifndef X86
#include "transaction.h"
#endif
#include "thread.h"
#include "types.h"
#include "memory.h"

#define MAIN(argc, argv)              int main (int argc, char** argv)
#define MAIN_RETURN(val)              return val

#define GOTO_SIM()                    /* nothing */
#define GOTO_REAL()                   /* nothing */
#define IS_IN_SIM()                   (0)

#define SIM_GET_NUM_CPU(var)          /* nothing */

#define TM_PRINTF                     printf
#define TM_PRINT0                     printf
#define TM_PRINT1                     printf
#define TM_PRINT2                     printf
#define TM_PRINT3                     printf

#define P_MEMORY_STARTUP(numThread)   do { \
                                        bool_t status; \
                                        size_t capacity=(1<<31)-1; \
                                        status = memory_init((numThread), capacity, 2); \
                                        assert(status); \
                                      } while (0) /* enforce comma */

#define P_MEMORY_SHUTDOWN()           memory_destroy() /* nothing */

//RIC la version con locks usa los BEGIN_TRANSACTION que luego en ../common/transaction.h se redefinen como locks
#ifdef PAR
#define TM_ARG                        /* nothing */
#define TM_ARG_ALONE                  /* nothing */
#define TM_ARGDECL                    /* nothing */
#define TM_ARGDECL_ALONE              /* nothing */
#define TM_CALLABLE                   /* nothing */

#define TM_STARTUP(numThread)         /* nothing */
#define TM_SHUTDOWN()                 /* nothing */

#define TM_THREAD_ENTER()             /* nothing */
#define TM_THREAD_EXIT()              /* nothing */

#define P_MALLOC(size)                malloc(size)/* memory_private_get(thread_getId(), size) */
#define P_FREE(ptr)                   free(ptr)/* TODO: thread local free is non-trivial */

#define TM_GET_REGION_PTR()             /* get_mem_region_ptr(thread_getId()) */
#define TM_GET_PRIVATE_REGION_PTR()     /* get_mem_private_region_ptr(thread_getId()) */

#define TM_MALLOC(size)               malloc(size)/* memory_get(thread_getId(), size) */
#define TM_FREE(ptr)                  /* TODO: thread local free is non-trivial */

#define TM_BEGIN(thId, xId)            /* RIC en el paralelo sólo está la barrera */
#define TM_END(thId, xId)              /* */
#define TM_RESTART(id)                 /* ABORT_TRANSACTION(id) */
#define TM_BEGIN_ESCAPE()              /* BEGIN_ESCAPE */
#define TM_END_ESCAPE()                /* END_ESCAPE */
#define TM_EARLY_RELEASE(var)          /* EARLY_RELEASE */

#define TM_BARRIER(thId)               thread_barrier_wait(0 /*non-breaking*/);
#define TM_LAST_BARRIER(thId)          thread_barrier_wait(0 /*non-breaking*/);

#elif defined(UNP)
#define TM_ARG                        /* nothing */
#define TM_ARG_ALONE                  /* nothing */
#define TM_ARGDECL                    /* nothing */
#define TM_ARGDECL_ALONE              /* nothing */
#define TM_CALLABLE                   /* nothing */

#define TM_STARTUP(numThread)         /* nothing */
#define TM_SHUTDOWN()                 /* nothing */

#define TM_THREAD_ENTER()             /* nothing */
#define TM_THREAD_EXIT()              /* nothing */

#define P_MALLOC(size)                malloc(size)/* memory_private_get(thread_getId(), size) */
#define P_FREE(ptr)                   free(ptr)/* TODO: thread local free is non-trivial */

#define TM_MALLOC(size)               malloc(size)/* memory_get(thread_getId(), size) */
#define TM_FREE(ptr)                  /* TODO: thread local free is non-trivial */

#define TM_BEGIN(thId, xId)            /* RIC en el paralelo sólo está la barrera */
#define TM_END(thId, xId)              /* */
#define TM_RESTART(id)                 /* ABORT_TRANSACTION(id) */
#define TM_BEGIN_ESCAPE()              /* BEGIN_ESCAPE */
#define TM_END_ESCAPE()                /* END_ESCAPE */
#define TM_EARLY_RELEASE(var)          /* EARLY_RELEASE */

#define TM_BARRIER(thId)               /*sin barrera*/
#define TM_LAST_BARRIER(thId)          /*sin barrera*/

#elif defined(TM)
//RIC paralelo con transacciones y barrera normal

#define TM_ARG                        /* nothing */
#define TM_ARG_ALONE                  /* nothing */
#define TM_ARGDECL                    /* nothing */
#define TM_ARGDECL_ALONE              /* nothing */
#define TM_CALLABLE                   /* nothing */

#define TM_STARTUP(numThread)         /* nothing */
#define TM_SHUTDOWN()                 printf("Barrier Count: %u\n", barrierCounter) /* nothing */

#define TM_THREAD_ENTER()             /* nothing */
#define TM_THREAD_EXIT()              /* nothing */

#define P_MALLOC(size)                memory_get(thread_getId(), size) /*RIC todo TM, no tengo private pool*/
#define P_FREE(ptr)                   /* TODO: thread local free is non-trivial */

#define TM_MALLOC(size)               memory_get(thread_getId(), size)
#define TM_FREE(ptr)                  /* TODO: thread local free is non-trivial */

#define TM_BEGIN(thId, xId)            BEGIN_TRANSACTION(thId, xId)
#define TM_END(thId, xId)              COMMIT_TRANSACTION()
#define TM_RESTART(id)                 __builtin_tabort(id)
#define TM_BEGIN_ESCAPE()              __builtin_tsuspend()
#define TM_END_ESCAPE()                __builtin_tresume()

//RIC pongo un contador para contar las barreras en cada benchmark
//RIC lo defino como extern y lo meto en transaction.c y lo imprimo en TM_SHUTDOWN()
//RIC usar con la versión TM y con 1 thread
extern unsigned long barrierCounter;

#define TM_BARRIER(thId)               {                                      \
  thread_barrier_wait(0 /*non-breaking*/);                                    \
  barrierCounter++;                                                           \
  }
#define TM_LAST_BARRIER(thId)          {                                      \
  thread_barrier_wait(0 /*non-breaking*/);                                    \
  barrierCounter++;                                                           \
  }

#elif defined(ORD)
#include "tm-ord.h"
//RIC paralelo con transacciones y barrera normal

#define TM_ARG                        /* nothing */
#define TM_ARG_ALONE                  /* nothing */
#define TM_ARGDECL                    /* nothing */
#define TM_ARGDECL_ALONE              /* nothing */
#define TM_CALLABLE                   /* nothing */

#define TM_STARTUP(numTh)             BARRIER_DESCRIPTOR_INIT(numTh) /* nothing */
#define TM_SHUTDOWN()                 /* nothing */

#define TM_THREAD_ENTER()             TX_DESCRIPTOR_INIT() /* nothing */
#define TM_THREAD_EXIT()              /* nothing */

#define P_MALLOC(size)                memory_get(thread_getId(), size) /*RIC todo TM, no tengo private pool*/
#define P_FREE(ptr)                   /* TODO: thread local free is non-trivial */

#define TM_MALLOC(size)               memory_get(thread_getId(), size)
#define TM_FREE(ptr)                  /* TODO: thread local free is non-trivial */

#define TM_BEGIN(thId, xId)            TM_START(thId, xId)
#define TM_END(thId, xId, ord)         TM_STOP(thId, xId, ord)
#define TM_RESTART(id)                 __builtin_tabort(id)
#define TM_BEGIN_ESCAPE()              __builtin_tsuspend()
#define TM_END_ESCAPE()                __builtin_tresume()

#define TM_BARRIER(thId)               /*nothing*/
#define TM_LAST_BARRIER(thId)          /*nothing*/



/* =============================================================================
 * RIC: SPECULATIVE BARRIERS
 * =============================================================================
 */

#elif defined(SB)
#include "tm-sb.h"

#define TM_ARG                        /* nothing */
#define TM_ARG_ALONE                  /* nothing */
#define TM_ARGDECL                    /* nothing */
#define TM_ARGDECL_ALONE              /* nothing */
#define TM_CALLABLE                   /* nothing */

#define TM_STARTUP(numTh)             BARRIER_DESCRIPTOR_INIT(numTh)
#define TM_SHUTDOWN()                 /* nothing */

#define TM_THREAD_ENTER()             TX_DESCRIPTOR_INIT()

#define TM_THREAD_EXIT()              /* nothing */

#define P_MALLOC(size)                memory_get(thread_getId(), size) /*RIC todo TM, no tengo private pool*/
#define P_FREE(ptr)                   /* TODO: thread local free is non-trivial */

#define TM_MALLOC(size)               memory_get(thread_getId(), size)
#define TM_FREE(ptr)                  /* TODO: thread local free is non-trivial */

#define TM_BEGIN(thId, xId)            TM_START(thId, xId)
#define TM_END(thId, xId)              TM_STOP(thId, xId)
#define TM_RESTART(id)                 __builtin_tabort(id)
#define TM_BEGIN_ESCAPE()              __builtin_tsuspend()
#define TM_END_ESCAPE()                __builtin_tresume()

#define TM_BARRIER(thId)               SB_BARRIER(thId)
#define TM_LAST_BARRIER(thId)          LAST_BARRIER(thId)

/* Check spec */
#elif defined(CS)
#include "tm-cs.h"

#define TM_ARG                        /* nothing */
#define TM_ARG_ALONE                  /* nothing */
#define TM_ARGDECL                    /* nothing */
#define TM_ARGDECL_ALONE              /* nothing */
#define TM_CALLABLE                   /* nothing */

#define TM_STARTUP(numTh)             BARRIER_DESCRIPTOR_INIT(numTh)
#define TM_SHUTDOWN()                 /* nothing */

#define TM_THREAD_ENTER()             TX_DESCRIPTOR_INIT()

#define TM_THREAD_EXIT()              /* nothing */

#define P_MALLOC(size)                memory_get(thread_getId(), size) /*RIC todo TM, no tengo private pool*/
#define P_FREE(ptr)                   /* TODO: thread local free is non-trivial */

#define TM_MALLOC(size)               memory_get(thread_getId(), size)
#define TM_FREE(ptr)                  /* TODO: thread local free is non-trivial */

#define TM_BEGIN(thId, xId)            TM_START(thId, xId)
#define TM_END(thId, xId)              TM_STOP(thId, xId)
#define TM_RESTART(id)                 __builtin_tabort(id)
#define TM_BEGIN_ESCAPE()              __builtin_tsuspend()
#define TM_END_ESCAPE()                __builtin_tresume()

#define TM_BARRIER(thId)               SB_BARRIER(thId)
#define TM_LAST_BARRIER(thId)          LAST_BARRIER(thId)


/* =============================================================================
 * Sequential execution
 * =============================================================================
 */

#else /* SEQUENTIAL */

#define TM_ARG                        /* nothing */
#define TM_ARG_ALONE                  /* nothing */
#define TM_ARGDECL                    /* nothing */
#define TM_ARGDECL_ALONE              /* nothing */
#define TM_CALLABLE                   /* nothing */

#define TM_STARTUP(numThread)         /* nothing */
#define TM_SHUTDOWN()                 /* nothing */

#define TM_THREAD_ENTER(numTh)             /* nothing */
#define TM_THREAD_EXIT()              /* nothing */

#define P_MALLOC(size)              malloc(size) /*RIC todo TM, no tengo private pool*/
#define P_FREE(ptr)                 /* TODO: thread local free is non-trivial */
#define TM_MALLOC(size)             malloc(size)
#define TM_FREE(ptr)                /* TODO: thread local free is non-trivial */

//RIC Pongo el id para que no de error al compilar el secuencial
#define TM_BEGIN(thId, xId)            /* nothing */
#define TM_END(thId, xId)              /* nothing */
#define TM_RESTART(id)                  assert(0)
#define TM_BEGIN_ESCAPE()              /* nothing */
#define TM_END_ESCAPE()                /* nothing */
#define TM_BARRIER(thId)               /*RIC nothing */
#define TM_LAST_BARRIER(thId)          /*RIC nothing */

#endif /* SEQUENTIAL */

#define TM_SHARED_READ(var)           (var)
#define TM_SHARED_READ_P(var)         (var)
#define TM_SHARED_READ_F(var)         (var)

#define TM_SHARED_WRITE(var, val)     ({var = val; var;})
#define TM_SHARED_WRITE_P(var, val)   ({var = val; var;})
#define TM_SHARED_WRITE_F(var, val)   ({var = val; var;})

#define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#define TM_LOCAL_WRITE_F(var, val)    ({var = val; var;})

#endif /* TM_H */


/* =============================================================================
 *
 * End of tm.h
 *
 * =============================================================================
 */
