/*****************************************************************************
* Copyright (c) 2020 - 2020 Qualcomm Technologies International, Ltd.
*
*****************************************************************************/
/**
 * \file  pl_hwlock.h
 * \ingroup platform
 *
 * Hardware lightweight lock is in use. <br>
 *
 */

#ifndef PL_HWLOCK_H
#define PL_HWLOCK_H

#include "types.h"
/****************************************************************************
Public Type Declarations
*/
/****************************************************************************
Public Constant and macros
*/
/* Upper limit for taking a lock with retry (panic if exceeded) */
#define PL_MAX_HWLOCK_RETRIES   128

/****************************************************************************
Public Variable Declarations
*/

/****************************************************************************
Public Function Declarations
*/

/* Acquire, release a HW lock */
/* A lock can be placed by the user anywhere in the DM RAM. */
/* The locks have the width of a word and should be word-aligned. */
/**
 * \brief Claim a HW lock
 *
 * \param lock_addr lock address
 *
 * \returns TRUE if lock was obtained, FALSE otherwise.
 */
extern bool pl_hwlock_get(unsigned *lock_addr);

/**
 * \brief Release a HW lock
 *
 * \param lock_addr lock address
 */
extern void pl_hwlock_rel(unsigned *lock_addr);

/**
 * \brief Claim a HW lock with retries
 *
 * \param lock_addr lock address
 * \param retries   try at most 'retries' times if unsuccessful
 *
 * It faults if lock address is NULL.
 * It panics if failed to claim hw lock after retries.
 */
extern void pl_hwlock_get_with_retry(unsigned *lock_addr, unsigned retries);

/**
 * \brief Claim a HW lock with timeout
 *
 * \param lock_addr lock address
 * \param timeout   Maximum time to wait before failing
 *
 * It faults if lock address is NULL.
 * It panics if failed to claim hw semaphore after timeout.
 */
extern void pl_hwlock_get_with_timeout(unsigned *lock_addr, TIME timeout);

#endif /* PL_HWLOCK_H */
