#ifndef BARRIERS_H_
#define BARRIERS_H_


#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <immintrin.h>
#include <string.h>

/* IMPORTANTE: en este archivo se encuentran tanto las macros dedicadas a las barreras como las dedicadas a 
   las transacciones. Estas transacciones son especiales y están adaptadas a las barreras, si se quiere implementar
   un programa que utilice transacciones sin barrera, utilizar el archivo transaction.h*/



// Intrínseco de pausa recomendada por intel para los bucles de espera
#define CPU_RELAX  _mm_pause()

// Código de error para indicar que el lock está cogido por otro hilo
#define LOCK_TAKEN 0xFF

#define CACHE_BLOCK_SIZE 64

#define GLOBAL_RETRIES 3

#define MAX_THREADS 128

#define MAX_SPEC    4

#define MAX_RETRIES 5

// Esta macro siempre debe coincidir con el número de transacciones(xacts) que se pasen a statsFileInit, sino las estadísticas estarán mal.
#define MAX_XACT_IDS 3

/* Esta macro se define para indicar que transacción es la correspondiente a la barrera,
   para que se puedan definir transacciones junto con la barrera y que esta sea la última
   en el fichero de estadísticas */
#define SPEC_XACT_ID MAX_XACT_IDS-1

/* Macros para hacer una transacción escapada (para poder leer y escribir variables que normalmente darían aborto por conflicto)
   dentro de una transaccion */
#define BEGIN_ESCAPE _xsusldtrk()
#define END_ESCAPE _xresldtrk()




/* inicializa las variables necesarias para implementar una transacción, debe ser llamada una vez por cada thread 
   al principio del bloque paralelo  */
#define TX_DESCRIPTOR_INIT()        tm_tx_t tx;                                 \
                                    tx.order = 1;                               \
                                    tx.retries = 0;                             \
                                    tx.speculative = 0;                         \
                                    tx.status = 0

// Inicializa las variables globales necesarias para las barreras
#define BARRIER_DESCRIPTOR_INIT(numTh) g_specvars.barrier.nb_threads = numTh;   \
                                       g_specvars.barrier.remain     = numTh



// Empieza una transacción
#define BEGIN_TRANSACTION(thId, xId)                                                         \
  assert(xId != SPEC_XACT_ID);                                              \
  tx.retries = 0;                                                           \
  do                                                                        \
  {                                                                         \
    if (tx.retries){                                                        \
      profileAbortStatus(tx.status, thId, xId);                             \
    }                                                                       \
    tx.retries++;                                                           \
    if (tx.retries > GLOBAL_RETRIES)                                        \
    {                                                                          \
      unsigned int myticket = __sync_add_and_fetch(&(g_ticketlock.ticket), 1); \
      while (myticket != g_ticketlock.turn)                                    \
        CPU_RELAX;                                                             \
      break;                                                                   \
    }                                                                          \
    while (g_ticketlock.ticket >= g_ticketlock.turn)                           \
      CPU_RELAX; /* Avoid Lemming effect */                                    \
  } while ((tx.status = _xbegin()) != _XBEGIN_STARTED)

#define COMMIT_TRANSACTION(thId, xId)                                     \
  if (tx.retries <= GLOBAL_RETRIES) {                                     \
    if (g_ticketlock.ticket >= g_ticketlock.turn)                         \
      _xabort(LOCK_TAKEN); /*Lazy subscription*/                          \
    _xend();                                                              \
    profileCommit(thId, xId, tx.retries-1);                               \
  } else {                                                                \
    __sync_add_and_fetch(&(g_ticketlock.turn), 1);                        \
    profileFallback(thId, xId, tx.retries-1);                             \
  }

// commita una transacción
//#define COMMIT_TRANSACTION(thId, xId)                                                      \
      if(!tx.speculative) {                                                     \
        if (tx.retries <= MAX_RETRIES) {                                        \
          _xend();                                                    \
          profileCommit(thId, xId, tx.retries-1);                               \
        } else {                                                                \
          __sync_add_and_fetch(&(g_fallback_lock.turn), 1);                     \
          profileFallback(thId, xId, tx.retries-1);                             \
        }                                                                       \
        tx.retries = 0;                                                         \
      } else { /* RIC ver si podemos hacer commit de la especulativa */         \
        BEGIN_ESCAPE;                                                           \
        if (tx.order <= g_specvars.tx_order) {                                  \
          END_ESCAPE;                                                           \
          _xend();                                                    \
          profileCommit(thId, SPEC_XACT_ID, tx.retries-1); /* ID de la xact especulativa abierta en SB_BARRIER*/ \
          /* Restore metadata */                                                \
          tx.speculative = 0;                                                   \
          tx.retries = 0;                                                       \
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
          }                                                                     \
        }                                                                       \
      }

#define SB_BARRIER(thId)                                                        \
  /* Se comprueba si el hilo entra por primera vez a la barrera (si está en modo especulativo o no) */ \
  if (tx.speculative) {                                                         \
    BEGIN_ESCAPE;                                                               \
    while (tx.order > g_specvars.tx_order);                                     \
    END_ESCAPE;                                                                 \
    /* Aquí ya he terminado una barrera así que puedo commitear la transacción para después*/ \
    /* empezar la de la siguiente.*/   \
    _xend();                                                          \
    profileCommit(thId, SPEC_XACT_ID, tx.retries-1);                            \
    /* Restore metadata */                                                      \
    tx.speculative = 0;                                                         \
    tx.retries = 0;                                                             \
  }                                                                             \
  /* incrementamos el orden del hilo */             \
  tx.order += 1;                                                                \
  /* Determina si el thread es el último en entrar a la barrera */                        \
  if (__sync_add_and_fetch(&(g_specvars.barrier.remain),-1) == 0) {             \
    /* Si se es el último en cruzar la barrera, se hace un reset y se incrementa el global order*/                                    \
    g_specvars.barrier.remain = g_specvars.barrier.nb_threads;                  \
    /* This instruction involves a full memory fence, so the reset of barrier->remain 
     * should appear ordered to the rest of the threads */                      \
    __sync_add_and_fetch(&(g_specvars.tx_order), 1);                            \
  } else {                                                                      \
    __label__ __p_failure;                                                      \
__p_failure:                                                                    \
    if(tx.retries){                                                             \
      profileAbortStatus(tx.status, thId, SPEC_XACT_ID);                        \
    }                                                                           \
    tx.retries++;                                                               \
    if (tx.order <= g_specvars.tx_order) {                                      \
      tx.speculative = 0;                                                       \
      tx.retries = 0;                                                           \
    } else {                                                                    \
      tx.speculative = 1;                                                       \
      if ( (tx.status & _XABORT_RETRY) && (tx.status & _XABORT_CONFLICT)){      \
            srand(time(NULL));                                                  \
            usleep((rand() % 10));                                              \
      }                                                                         \
      /*while (g_fallback_lock.ticket >= g_fallback_lock.turn);*/               \
      if((tx.status = _xbegin()) != _XBEGIN_STARTED) {goto __p_failure;}        \
      /*if (g_fallback_lock.ticket >= g_fallback_lock.turn)*/                   \
      /*  _xabort(LOCK_TAKEN); *//*Early subscription*/                         \
    }                                                                           \
  }
                                                                  

/* Última barrera antes de terminar la ejecución, al contrario que la otra macro, 
   esta no abre otra transacción sino que termina. */
#define LAST_BARRIER(thId)                                                      \
  if (tx.speculative) {                                                         \
    BEGIN_ESCAPE;                                                               \
    while (tx.order > g_specvars.tx_order);                                     \
    END_ESCAPE;                                                                 \
    _xend();                                                                    \
    profileCommit(thId, SPEC_XACT_ID, tx.retries-1);                            \
    tx.speculative = 0;                                                         \
    tx.retries = 0;                                                             \
  }                                                                             \
  tx.order += 1;                                                                \
  if (__sync_add_and_fetch(&(g_specvars.barrier.remain),-1) == 0) {             \
    g_specvars.barrier.remain = g_specvars.barrier.nb_threads;                  \
    __sync_add_and_fetch(&(g_specvars.tx_order), 1);                            \
  /* Aquí está la diferencia con SB_BARRIER, no se crea otra transacción sino que espera a los demás y termina */ \
  } else {                                                                      \
    while(tx.order > g_specvars.tx_order) ;                                     \
  }

//  Termina la transacción en el punto en el que se indica, para que las transacciones no sean demasiado largas
#define CHECK_SPEC(thId, xId)                                                      \
      if(tx.speculative) {                                                      \
        BEGIN_ESCAPE;                                                           \
        if (tx.order <= g_specvars.tx_order) {                                  \
          END_ESCAPE;                                                           \
          _xend();                                                 \
          profileCommit(thId, SPEC_XACT_ID, tx.retries-1); /* ID de la xact especulativa abierta en SB_BARRIER*/ \
          /* Restore metadata */                                                \
          tx.speculative = 0;                                                   \
          tx.retries = 0;                                                       \
        } else {                                                                \
          /* If we can not, decrement our speclevel */                          \
            while (tx.order > g_specvars.tx_order);                             \
            END_ESCAPE;                                                         \
            _xend();                                                            \
            profileCommit(thId, SPEC_XACT_ID, tx.retries-1);                    \
            /* Restore metadata */                                              \
            tx.speculative = 0;                                                 \
            tx.retries = 0;                                                     \
        }                                                                       \
      }




// Tickets para realizar la ejecución de forma secuencial en caso de que sea necesario
struct TicketLock
{
  volatile char pad1[CACHE_BLOCK_SIZE];
  volatile unsigned int ticket;
  volatile unsigned int turn;
  volatile char pad2[CACHE_BLOCK_SIZE];
};

// Declarado en stats.c 
extern struct TicketLock g_ticketlock;

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
  uint32_t status;  /* Transaction status.*/
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


/* Lock para hacer un fallback*/
/* typedef struct fback_lock {
  volatile uint32_t ticket;
  volatile uint32_t turn;
  uint8_t pad[CACHE_BLOCK_SIZE-sizeof(uint32_t)*2];
} __attribute__ ((aligned (CACHE_BLOCK_SIZE))) fback_lock_t;

extern fback_lock_t g_fallback_lock;
*/
////////////////////////////////////////////////////////////////////////

// Estructura para guardar las estadísticas
struct Stats
{
  //Las estadísticas se actualizan fuera de transacción, pero se reservan con malloc (¿podrían caer cerca de datos compartidos accedidos dentro de transacción y compartir línea de caché?)
  volatile char pad1[CACHE_BLOCK_SIZE];  //Pads para que no haya false sharing (que no coincidan xabortCount de un thread con retryFCount de otro en un bloque cache)
  unsigned long int xabortCount;         //Número total de abortos
  unsigned long int explicitAborts;      //Número de llamadas a XABORT en el código
  unsigned long int explicitAbortsSubs;  //Número de abortos explícitos por subscripción de lock
  unsigned long int retryAborts;         //Abortos para los que el hardware piensa que debemos reintentar
  unsigned long int retryConflictAborts; //Abortos para los que el hardware piensa que debemos reintentar
  unsigned long int retryCapacityAborts; //Abortos para los que el hardware piensa que debemos reintentar
  unsigned long int conflictAborts;      //Abortos por conflicto
  unsigned long int capacityAborts;      //Abortos por capacidad
  unsigned long int debugAborts;         //Abortos por breakpoint de debugger
  unsigned long int nestedAborts;        //Abortos dentro de una transacción anidada
  unsigned long int eaxzeroAborts;       //Abortos con eax = 0
  unsigned long int xcommitCount;        //Número de commits
  unsigned long int fallbackCount;       //Número de fallbacks
  unsigned long int retryCCount;         //Número de retries de las que commitan
  unsigned long int retryFCount;         //Número de retries de las que entran en fallback
  unsigned long int xbeginCount;         //Número de transacciones que se han abierto
  volatile char pad2[CACHE_BLOCK_SIZE];
};

extern struct Stats **stats;


//Funciones para el fichero de estadísticas
int statsFileInit(int argc, char **argv, long thCount, long xCount);
int dumpStats(double time);

// Funciones de profile de estadísticas (hechas inline para mejorar el rendimiento)


// Función para determinar el tipo de aborto
unsigned long profileAbortStatus(unsigned long eax, long thread, long xid);

// Función para indicar un commit
void profileCommit(long thread, long xid, long retries);

// Funcion para indicar un fallback
void profileFallback(long thread, long xid, long retries);

#endif