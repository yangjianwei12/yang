/****************************************************************************
 * Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup platform Kalimba Platform
 * \file pl_interrupt.h
 * \ingroup platform
 *
 * Header file for kalimba interrupt functions
 */

#ifndef PL_INTERRUPT_H
#define PL_INTERRUPT_H

/**  Callback function types */
typedef void (*INTERRUPT_HANDLER)(void);

typedef enum
{
    INTERRUPT_PRI_WAKEUP = 0,
    INTERRUPT_PRI_LOW    = 1,
    INTERRUPT_PRI_MID    = 2,
    INTERRUPT_PRI_HIGH   = 3
} INTERRUPT_PRI;

extern void interrupt_initialise(void);

extern void interrupt_block(void);

extern void interrupt_unblock(void);

extern void interrupt_register(int int_source,
                               INTERRUPT_PRI int_priority,
                               INTERRUPT_HANDLER IntFunction);

extern void safe_enable_shallow_sleep(void);

#ifdef DESKTOP_TEST_BUILD
extern void test_run_timers(void);
#endif

#endif /* PL_INTERRUPT_H */
