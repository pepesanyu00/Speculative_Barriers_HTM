#ifndef TRANSACTION_H_
#define TRANSACTION_H_

#include <pthread.h>
#include <assert.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * Copyright (c) 2012,2013 Intel Corporation
 * Author: Andi Kleen
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
This file provide an alternative RTM intrinsics implementation using
the "asm goto" gcc extension. This will only work on gcc 4.7+ or
gcc 4.6 with backport (Fedora). It exposes the jump to the abort
handler to the programmer, which can save a few instructions.
XBEGIN(label)
Start a transaction. When an abort happens jump to the XFAIL* defined
by "label".
When the CPU does not support transactions this will unconditionally
jump to label.
XEND()
End a transaction. Must match a XBEGIN().
XFAIL(label)
Begin a fallback handler that is jumped too from XBEGIN in case of a 
transaction abort. Must be in the same function as the XBEGIN().
XFAIL acts like a label.
Note: caller must ensure to not execute the code when not jumped too, e.g.
by placing an if (0) around it, unless this is intended.
The fail handler should not execute XEND() for the transaction.
XFAIL_STATUS(label, status)
Same as XFAIL(), but supply a status code containing the reason 
for the abort. See below for the valid status bits.
XABORT(x) 
Abort a transaction.
 */

#define XABORT(status) asm volatile(".byte 0xc6,0xf8,%P0" ::"i"(status))
#define XBEGIN(label)                                       \
  asm volatile goto(".byte 0xc7,0xf8 ; .long %l0-1f\n1:" :: \
                        : "eax"                             \
                    : label)
#define XEND() asm volatile(".byte 0x0f,0x01,0xd5")
#define XFAIL(label) \
  label:             \
  asm volatile("" :: \
                   : "eax")
#define XFAIL_STATUS(label, status) \
  label:                            \
  asm volatile(""                   \
               : "=a"(status))
//RIC Meto pausa recomendada por Intel para mejorar la eficiencia en spinlocks
#define CPU_RELAX() asm volatile("pause\n" \
                                 :         \
                                 :         \
                                 : "memory")

//#include "rtm.h"
//#define XTEST() _xtest()

/* Status bits */
#define XBEGIN_STARTED (~0u)
#define XABORT_EXPLICIT (1 << 0)
#define XABORT_RETRY (1 << 1)
#define XABORT_CONFLICT (1 << 2)
#define XABORT_CAPACITY (1 << 3)
#define XABORT_DEBUG (1 << 4)
#define XABORT_NESTED (1 << 5)
#define XABORT_CODE(x) (((x) >> 24) & 0xff)
//RIC
#define LOCK_TAKEN 0xFF

#define MAX_THREADS 128

#define CACHE_BLOCK_SIZE 64

#define GLOBAL_RETRIES 3

#if defined(INTRIN) /* Con intrínsecos */

#include <immintrin.h>

#define BEGIN_TRANSACTION(thId, xId)                                             \
  {                                                                              \
    unsigned long __p_status, __p_retries = 0;                                   \
    long __p_thId = thId, __p_xId = xId;                                         \
    do                                                                           \
    {                                                                            \
      if (__p_retries)                                                           \
        profileAbortStatus(__p_status, __p_thId, __p_xId);                       \
      __p_retries++;                                                             \
      if (__p_retries > GLOBAL_RETRIES)                                          \
      {                                                                          \
        unsigned int myticket = __sync_add_and_fetch(&(g_ticketlock.ticket), 1); \
        while (myticket != g_ticketlock.turn)                                    \
          CPU_RELAX();                                                           \
        break;                                                                   \
      }                                                                          \
      while (g_ticketlock.ticket >= g_ticketlock.turn)                           \
        CPU_RELAX(); /* Avoid Lemming effect */                                  \
    } while ((__p_status = _xbegin()) != XBEGIN_STARTED)

#define COMMIT_TRANSACTION()                             \
  if (__p_retries <= GLOBAL_RETRIES)                     \
  {                                                      \
    if (g_ticketlock.ticket >= g_ticketlock.turn)        \
      _xabort(LOCK_TAKEN); /*Lazy subscription*/         \
    _xend();                                             \
    profileCommit(__p_thId, __p_xId, __p_retries - 1);   \
  }                                                      \
  else                                                   \
  {                                                      \
    __sync_add_and_fetch(&(g_ticketlock.turn), 1);       \
    profileFallback(__p_thId, __p_xId, __p_retries - 1); \
  }                                                      \
  }

#elif defined(INTRINCONF)
#include <immintrin.h>
#define INIT_TRANSACTION() \
  unsigned long __p_status, __p_retries

#define BEGIN_TRANSACTION(thId, xId)                                           \
  __p_retries = 0;                                                             \
  do                                                                           \
  {                                                                            \
    if (__p_retries)                                                           \
      profileAbortStatus(__p_status, thId, xId);                               \
    __p_retries++;                                                             \
    if (__p_retries > GLOBAL_RETRIES)                                          \
    {                                                                          \
      unsigned int myticket = __sync_add_and_fetch(&(g_ticketlock.ticket), 1); \
      while (myticket != g_ticketlock.turn)                                    \
        CPU_RELAX();                                                           \
      break;                                                                   \
    }                                                                          \
    while (g_ticketlock.ticket >= g_ticketlock.turn)                           \
      CPU_RELAX(); /* Avoid Lemming effect */                                  \
  } while ((__p_status = _xbegin()) != XBEGIN_STARTED)

#define COMMIT_TRANSACTION(thId, xId)              \
  if (__p_retries <= GLOBAL_RETRIES)               \
  {                                                \
    if (g_ticketlock.ticket >= g_ticketlock.turn)  \
      _xabort(LOCK_TAKEN); /*Lazy subscription*/   \
    _xend();                                       \
    profileCommit(thId, xId, __p_retries - 1);     \
  }                                                \
  else                                             \
  {                                                \
    __sync_add_and_fetch(&(g_ticketlock.turn), 1); \
    profileFallback(thId, xId, __p_retries - 1);   \
  }

#else

#define BEGIN_TRANSACTION(thId, xId)                                           \
  {                                                                            \
    __label__ __p_failure_##xId;                                               \
    volatile long __p_retries = 0;                                             \
    unsigned long __p_eax;                                                     \
    long __p_thId = thId, __p_xId = xId;                                       \
    XFAIL_STATUS(__p_failure_##xId, __p_eax);                                  \
    if (__p_retries)                                                           \
      profileAbortStatus(__p_eax, __p_thId, __p_xId);                          \
    __p_retries++;                                                             \
    if (__p_retries > GLOBAL_RETRIES)                                          \
    {                                                                          \
      unsigned int myticket = __sync_add_and_fetch(&(g_ticketlock.ticket), 1); \
      while (myticket != g_ticketlock.turn)                                    \
        CPU_RELAX();                                                           \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      while (g_ticketlock.ticket >= g_ticketlock.turn)                         \
        CPU_RELAX();                                                           \
      XBEGIN(__p_failure_##xId);                                               \
    }

#define COMMIT_TRANSACTION()                             \
  if (__p_retries <= GLOBAL_RETRIES)                     \
  {                                                      \
    if (g_ticketlock.ticket >= g_ticketlock.turn)        \
      XABORT(LOCK_TAKEN); /*Lazy subscription*/          \
    XEND();                                              \
    profileCommit(__p_thId, __p_xId, __p_retries - 1);   \
  }                                                      \
  else                                                   \
  {                                                      \
    __sync_add_and_fetch(&(g_ticketlock.turn), 1);       \
    profileFallback(__p_thId, __p_xId, __p_retries - 1); \
  }                                                      \
  }

#endif

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
  volatile char pad2[CACHE_BLOCK_SIZE];
};

struct TicketLock
{
  volatile char pad1[CACHE_BLOCK_SIZE];
  //RIC Para implementar el spinlock del fallback de Haswell
  volatile unsigned int ticket; // = 0;
  volatile unsigned int turn;   // = 1;
  volatile char pad2[CACHE_BLOCK_SIZE];
};

extern struct TicketLock g_ticketlock;
extern struct Stats **stats;
//extern long GLOBAL_RETRIES;
//extern volatile unsigned int g_ticketlock_ticket;
//extern volatile unsigned int g_ticketlock_turn;

//Funciones para el fichero de estadísticas
int statsFileInit(int argc, char **argv, long thCount, long xCount);
int dumpStats();

inline unsigned long profileAbortStatus(unsigned long eax, long thread, long xid)
{
  stats[thread][xid].xabortCount++;
  if (eax & XABORT_EXPLICIT)
  {
    stats[thread][xid].explicitAborts++;
    if (XABORT_CODE(eax) == LOCK_TAKEN)
      stats[thread][xid].explicitAbortsSubs++;
  }
  else if (eax & XABORT_RETRY)
  {
    stats[thread][xid].retryAborts++;
    if (eax & XABORT_CONFLICT)
      stats[thread][xid].retryConflictAborts++;
    if (eax & XABORT_CAPACITY)
      stats[thread][xid].retryCapacityAborts++;
    if (eax & XABORT_DEBUG)
      assert(0);
    if (eax & XABORT_NESTED)
      assert(0);
  }
  else if (eax & XABORT_CONFLICT)
  {
    stats[thread][xid].conflictAborts++;
  }
  else if (eax & XABORT_CAPACITY)
  {
    stats[thread][xid].capacityAborts++;
  }
  else if (eax & XABORT_DEBUG)
  {
    stats[thread][xid].debugAborts++;
  }
  else if (eax & XABORT_NESTED)
  {
    stats[thread][xid].nestedAborts++;
  }
  else
  {
    //Todos los bits a cero (puede ocurrir por una llamada a CPUID u otro cosa)
    //Véase Section 8.3.5 RTM Abort Status Definition del Intel Architecture
    //Instruction Set Extensions Programming Reference (2012))
    stats[thread][xid].eaxzeroAborts++;
  }
  return 0;
}
inline void profileCommit(long thread, long xid, long retries)
{
  stats[thread][xid].xcommitCount++;
  stats[thread][xid].retryCCount += retries;
}
inline void profileFallback(long thread, long xid, long retries)
{
  stats[thread][xid].fallbackCount++;
  stats[thread][xid].retryFCount += retries;
}

#endif
