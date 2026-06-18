/*
 * mypt->h
 *
 *  Created on: Sep 12, 2023
 *      Author: lq
 */

#ifndef INC_MYPT_H_
#define INC_MYPT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"

    typedef uint16_t PtNumber;

    typedef struct myPt_t
    {
        PtNumber _ptLine;
        uint8_t _ptYield;
        uint8_t status;
    } myPt_t;

#define PT_RET_RUNNING  1
#define PT_RET_COMPLETE 0

    typedef uint8_t (*Task)();

#define PT_Reset(mypt)     \
    do                     \
    {                      \
        mypt->_ptLine = 0; \
        mypt->status = 0; \
    } while (0)

#define PT_Stop(mypt)                 \
    do                                \
    {                                 \
        mypt->_ptLine = (PtNumber)-1; \
    } while (0)

// Declare start of protothread (use at start of task implementation).
#define PT_BEGIN(mypt)     \
    mypt->_ptYield = 1;    \
    switch (mypt->_ptLine) \
    {                      \
    case 0:

// Stop protothread and end it (use at end of task implementation).
#define PT_END(mypt)   \
    default:;          \
        }              \
        PT_Stop(mypt); \
        return 0;

    // protothread completed
    #define PT_CPLT(mypt)   \
	    do                   \
	    {                    \
	        PT_Reset(mypt);  \
	        return 0;        \
	    } while (0)

    // Restart protothread's execution at its PT_BEGIN.
    #define PT_RESTART(mypt) \
        do                   \
        {                    \
            PT_Reset(mypt);  \
            return 1;        \
        } while (0)

    // Stop and exit from protothread.
    #define PT_EXIT(mypt)  \
        do                 \
        {                  \
            PT_Stop(mypt); \
            return 1;      \
        } while (0)

// Cause protothread to wait until given condition is true.
#define PT_WAIT_UNTIL(mypt, condition) \
    do                                 \
    {                                  \
        mypt->_ptLine = __LINE__;      \
    case __LINE__:                     \
        if (!(condition))              \
            return 1;                  \
    } while (0)

    // Yield protothread
    #define PT_YIELD(mypt)            \
        do                            \
        {                             \
            mypt->_ptYield = 0;       \
            PT_WAIT_UNTIL(mypt, mypt->_ptYield); \
        } while (0)



// Cause protothread to wait while given condition is true.
#define PT_WAIT_TILL(mypt, condition) PT_WAIT_UNTIL(mypt, (!condition))

    // Cause protothread to wait until given child protothread completes.
    #define PT_WAIT_THREAD(mypt, child) PT_WAIT_TILL(mypt, child)

#if 0
// Restart and spawn given child protothread and wait until it completes.
#define PT_SPAWN(child)        \
    do                         \
    {                          \
        (child).Restart();     \
        PT_WAIT_THREAD(child); \
    } while (0)
#endif


#ifdef __cplusplus
}
#endif

#endif /* INC_MYPT_H_ */
