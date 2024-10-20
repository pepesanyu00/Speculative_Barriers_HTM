#ifndef TM_H
#define TM_H 1

#include <stdio.h>
#include <assert.h>
//#ifndef X86
//#include "transaction.h"
#include "rtmIntel.h"
//#endif
//#include "thread.h"
//#include "types.h"

/* =============================================================================
 * RIC: SPECULATIVE BARRIERS
 * =============================================================================
 */

#if defined(SB)
#include "tm-sb.h"



#define TM_STARTUP(numTh)             BARRIER_DESCRIPTOR_INIT(numTh)
#define TM_SHUTDOWN()                 /* nothing */

#define TM_THREAD_ENTER()             TX_DESCRIPTOR_INIT()

#define TM_THREAD_EXIT()              /* nothing */

#define TM_BEGIN(thId, xId)            TM_START(thId, xId)
#define TM_END(thId, xId)              TM_STOP(thId, xId)
#define TM_RESTART(id)                 _xabort(id)
#define TM_BEGIN_ESCAPE()              _xsusldtrk()
#define TM_END_ESCAPE()                _xresldtrk()

#define TM_BARRIER(thId)               SB_BARRIER(thId)
#define TM_LAST_BARRIER(thId)          LAST_BARRIER(thId)

/* Check spec */
#elif defined(CS)
#include "tm-cs.h"


#define TM_STARTUP(numTh)             BARRIER_DESCRIPTOR_INIT(numTh)
#define TM_SHUTDOWN()                 /* nothing */

#define TM_THREAD_ENTER()             TX_DESCRIPTOR_INIT()

#define TM_THREAD_EXIT()              /* nothing */


#define TM_BEGIN(thId, xId)            TM_START(thId, xId)
#define TM_END(thId, xId)              TM_STOP(thId, xId)
#define TM_RESTART(id)                 _xabort(id)
#define TM_BEGIN_ESCAPE()              _xsusldtrk()
#define TM_END_ESCAPE()                _xresldtrk()

#define TM_BARRIER(thId)               SB_BARRIER(thId)
#define TM_LAST_BARRIER(thId)          LAST_BARRIER(thId)
#endif 

#endif /* TM_H */


/* =============================================================================
 *
 * End of tm.h
 *
 * =============================================================================
 */
