#ifndef CSR_TIME_H__
#define CSR_TIME_H__
/******************************************************************************
 Copyright (c) 2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/


#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif


#include "platform/csr_hydra_rtime.h"

/*******************************************************************************

    NAME
        CsrTime

    DESCRIPTION
        Type to hold a value describing the current system time, which is a
        measure of time elapsed since some arbitrarily defined fixed time
        reference, usually associated with system startup.

*******************************************************************************/
typedef uint32 CsrTime;


/*******************************************************************************

    NAME
        CsrTimeGet

    DESCRIPTION
        Returns the current value of the system's 32 bit 1MHz clock. The
        resolution of the returned int32 is one microsecond, so the system's
        clock wraps after approximately 71 minutes.

        This clock is the basis of all timed events in the chip's hardware,
        notably other functions declared in this file.

        The function can be called from the machine's background or from
        interrupt routines.

        This essentially just reads TIMER_TIME. However, this requires two
        separate 16 bit word reads, and there is a risk of a carry occuring
        between the two reads. This function encapsulates the complexity of
        obtaining a consistent value.

        Returns the current system time.

    PARAMETERS
        high - Pointer to variable that will receive the high part of the
               current system time. Passing NULL is valid.

    RETURNS
        Low part of current system time in microseconds.

*******************************************************************************/
#define CsrTimeGet(high) (*(CsrTime *) high = 0, (CsrTime) hal_get_time())


/*------------------------------------------------------------------*/
/* CsrTime Macros */
/*------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeAdd
 *
 *  DESCRIPTION
 *      Add two time values. Adding the numbers can overflow the range of a
 *      CsrTime, so the user must be cautious.
 *
 *  RETURNS
 *      CsrTime - the sum of "t1" and "t2".
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeAdd(t1, t2) ((t1) + (t2))

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeSub
 *
 *  DESCRIPTION
 *      Subtract two time values. Subtracting the numbers can provoke an
 *      underflow, so the user must be cautious.
 *
 *  RETURNS
 *      CsrTime - "t1" - "t2".
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeSub(t1, t2)    ((CsrInt32) (t1) - (CsrInt32) (t2))

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeEq
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" equal "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeEq(t1, t2) ((t1) == (t2))

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeGt
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" is greater than "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeGt(t1, t2) (CsrTimeSub((t1), (t2)) > 0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeGe
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" is greater than, or equal to "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeGe(t1, t2) (CsrTimeSub((t1), (t2)) >= 0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeLt
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" is less than "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeLt(t1, t2) (CsrTimeSub((t1), (t2)) < 0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeLe
 *
 *  DESCRIPTION
 *      Compare two time values.
 *
 *  RETURNS
 *      !0 if "t1" is less than, or equal to "t2", else 0.
 *
 *----------------------------------------------------------------------------*/
#define CsrTimeLe(t1, t2) (CsrTimeSub((t1), (t2)) <= 0)

#ifdef __cplusplus
}
#endif

#endif
