#ifndef CSR_HYDRA_RTIME__
#define CSR_HYDRA_RTIME__

/*****************************************************************************
Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #2 $
*****************************************************************************/

#include "csr_hydra_types.h"
#include "csr_synergy.h"


#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HYDRA) && !defined(CAA)

/** Difference between two system times, in microseconds */
typedef int32 INTERVAL;

/**
 * Read the current system time
 *
 * Returns the current value of the system's 32 bit 1MHz clock. The
 * resolution of the returned int32 is one microsecond, so the system's
 * clock wraps after approximately 71 minutes.
 *
 * This clock is the basis of all timed events in the chip's hardware,
 * notably other functions declared in this file.
 *
 * The function can be called from the machine's background or from
 * interrupt routines.
 *
 * This essentially just reads TIMER_TIME. However, this requires two
 * separate 16 bit word reads, and there is a risk of a carry occuring
 * between the two reads. This function encapsulates the complexity of
 * obtaining a consistent value.
 *
 * Returns the current system time.
 */
extern uint32 hal_get_time(void);

#endif /* !HYDRA */

#ifdef __cplusplus
}
#endif

#endif /* CSR_HYDRA_RTIME__ */
