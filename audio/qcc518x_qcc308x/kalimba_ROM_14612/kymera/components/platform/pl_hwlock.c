/****************************************************************************
 * Copyright (c) 2020 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file pl_hwlock.c
 * \ingroup platform
 *
 * Encapsulates handling of lightweight hardware locks.
 */

/****************************************************************************
Include Files
*/
#include "fault/fault.h"
#include "hal/hal_time.h"
#include "hal_macros.h"
#include "io_map.h"
#include "panic/panic.h"
#include "patch/patch.h"
#include "platform/pl_hwlock.h"
#include "platform/pl_interrupt.h"
#include "pl_timers/pl_timers.h"
#include "common/interface/util.h"

/****************************************************************************
Private Macro Definitions
*/

/* uncomment this for debug
#define HW_LOCK_DEBUG */

#ifdef HW_LOCK_DEBUG
#include "platform/pl_assert.h"
#define LOCK_ASSERT(x) PL_ASSERT(x)
#else
#define LOCK_ASSERT(x)
#endif

/****************************************************************************
Private Variable Definitions
*/

/****************************************************************************
Private Function Definitions
*/
#ifdef __KCC__
/**
 * \brief Inline assembly function which reads and writes a lock uninterrupted
 *
 * \param lock_addr   lock address
 * \return TRUE if lock was obtained, FALSE otherwise.
 */
asm bool pl_hwlock_claim_low_lvl(unsigned *lock_addr)
{
    @[    .scratch ret_val
          .restrict ret_val:bank1_with_rmac<R1>
          .restrict lock_addr:bank1_with_rmac<R0>
          .restrict :bank1_with_rmac<R0>
     ]

    @{ret_val} = 1;
    READANDSET @{lock_addr};
    /* Z flag should be set for a successful lock claim;
     * it represents the assert done on the lock content prior to setting it */
    if NZ @{ret_val} = 0;
    @{} = @{ret_val};
}
#else/* __KCC__ */
#define pl_hwlock_claim_low_lvl(lock_addr) TRUE
#endif/* __KCC__ */


/****************************************************************************
Public function definitions
*/


#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 && !defined(UNIT_TEST_BUILD)
/**
 * \brief Claim a HW lock
 *
 * \param lock_addr lock address
 *
 * \returns TRUE if lock was obtained, FALSE otherwise.
 *
 * \note This function will fault if the lock address is NULL.
 */
#ifdef __KCC__
_Pragma("codesection PM_KEEP")
#endif /* __KCC__ */
bool pl_hwlock_get(unsigned *lock_addr)
{
    if (lock_addr == NULL)
    {
        fault_diatribe(FAULT_AUDIO_PL_HWLOCK_INVALID, 0);
        return FALSE;
    }
    interrupt_block();
    if (!pl_hwlock_claim_low_lvl(lock_addr))
    {
        interrupt_unblock();
        return FALSE;
    }
    return TRUE;
}
#else
bool pl_hwlock_get(unsigned *lock_addr)
{
    return FALSE;
}
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 && !defined(UNIT_TEST_BUILD) */

#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 && !defined(UNIT_TEST_BUILD)
/**
 * \brief Claim a hw lock with retry.
 *
 * \param lock_addr   lock address
 * \param retries     try at most 'retries' times if unsuccessful
 *
 * \note This function will fault if the lock address is NULL
 *       and panic if it cannot claim the hw lock after retries.
 */
#ifdef __KCC__
_Pragma("codesection PM_KEEP")
#endif /* __KCC__ */
void pl_hwlock_get_with_retry(unsigned *lock_addr, unsigned retries)
{

    patch_fn_shared(pl_hwlock);

    unsigned i;

    LOCK_ASSERT(retries <= PL_MAX_HWLOCK_RETRIES);

    if (lock_addr == NULL)
    {
        fault_diatribe(FAULT_AUDIO_PL_HWLOCK_INVALID, 1);
        return;
    }

    interrupt_block();
    for (i=0; i<retries; i++)
    {
        if (pl_hwlock_claim_low_lvl(lock_addr))
        {
            return;
        }
        /* wait 1 microsecond before trying again */
        timer_n_us_delay(1);
    }

    interrupt_unblock();
    panic_diatribe(PANIC_AUDIO_PL_HWLOCK_RETRIES, retries);
}
#else
void pl_hwlock_get_with_retry(unsigned *lock_addr, unsigned retries)
{
    NOT_USED(retries);
    pl_hwlock_get(lock_addr);
}
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 && !defined(UNIT_TEST_BUILD) */

#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 && !defined(UNIT_TEST_BUILD)
/**
 * \brief Claim a hw lock with a timeout.
 *
 * \param lock_addr   lock address
 * \param timeout     Maximum time in microseconds to wait before failing
 *
 * \note This function will fault if the lock address is NULL
 *       and panic if it cannot claim the hw lock after timeout microseconds.
 */
#ifdef __KCC__
_Pragma("codesection PM_KEEP")
#endif /* __KCC__ */
void pl_hwlock_get_with_timeout(unsigned *lock_addr, TIME timeout)
{

    patch_fn_shared(pl_hwlock);

    TIME wait_until;

    LOCK_ASSERT(timeout <= 1000);

    if (lock_addr == NULL)
    {
        fault_diatribe(FAULT_AUDIO_PL_HWLOCK_INVALID, 2);
        return;
    }

    wait_until = hal_get_reg_timer_time() + timeout;
    interrupt_block();
    do
    {
        if (pl_hwlock_claim_low_lvl(lock_addr))
        {
            return;
        }
        /* wait 1 microsecond before trying again */
        timer_n_us_delay(1);
    }  while (hal_time_ge(wait_until, hal_get_reg_timer_time()));

    interrupt_unblock();
    panic_diatribe(PANIC_AUDIO_PL_HWLOCK_TIMEOUT, timeout);
}
#else
void pl_hwlock_get_with_timeout(unsigned *lock_addr, TIME timeout)
{
    NOT_USED(timeout);
    pl_hwlock_get(lock_addr);
}
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 && !defined(UNIT_TEST_BUILD) */

#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 && !defined(UNIT_TEST_BUILD)
/**
 * \brief Release a hw lock.
 *
 * \param lock_addr   lock address
 *
 * \note This function will panic if the lock is not being held.
 */
#ifdef __KCC__
_Pragma("codesection PM_KEEP")
#endif /* __KCC__ */
void pl_hwlock_rel(unsigned *lock_addr)
{
    patch_fn_shared(pl_hwlock);

    if (lock_addr == NULL)
    {
        fault_diatribe(FAULT_AUDIO_PL_HWLOCK_INVALID, 3);
        return;
    }
    if (*lock_addr == 0)
    {
        panic(PANIC_AUDIO_PL_HWLOCK_ALREADY_RELEASED);
    }
    *lock_addr = 0;
    interrupt_unblock();
}
#else
void pl_hwlock_rel(unsigned *lock_addr)
{
}
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 && !defined(UNIT_TEST_BUILD) */
