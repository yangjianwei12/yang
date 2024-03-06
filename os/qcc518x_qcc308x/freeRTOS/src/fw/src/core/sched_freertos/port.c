/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*------------------------------------------------------------------------------
 * Implementation of functions defined in portable.h for the Kalimba port.
 *
 * The primary purpose of this port is to be able to use FreeRTOS as the
 * scheduler for the Developer Kit that runs on Qualcomm Hydra chips. These
 * chips are based on the Kalimba CPU architecture.
 *
 * In addition to implementing the Developer Kit OS this FreeRTOS port can be
 * built to run stand-alone by setting the FREERTOS_STANDALONE_BUILD define.
 * This mode is useful mainly for testing FreeRTOS on the Kalimba architecture,
 * whilst only depending on a minimal set of Hydra software modules. It does
 * not provide any support for communicating with the P0 processor or even
 * dynamic memory allocation.
 *----------------------------------------------------------------------------*/

/* Scheduler includes. */
#include "sched_freertos_hooks.h"
#include "portasm.h"

/* Required IO Defs */
#define IO_DEFS_MODULE_K32_DEBUG_REGFILE /* INT_UM_FLAG_MASK */
#define IO_DEFS_MODULE_K32_TIMERS /* hal_set_reg_timer2* */

/* Lint notices that int/int.h also defines this macro so only define it if it
   isn't already defined. */
#ifndef IO_DEFS_MODULE_K32_INTERRUPT
#define IO_DEFS_MODULE_K32_INTERRUPT /* INT_SOURCE_TIMER2 */
#endif /* !IO_DEFS_MODULE_K32_INTERRUPT */
#include "io/io.h"

#include "hal/hal_macros.h"
#include "hal/haltime.h"
#include "timed_event/rtime.h"
#include "int/int.h"
#include "hydra/hydra_macros.h"

/* Some dependencies aren't required for the standalone build. */
#ifndef FREERTOS_STANDALONE_BUILD
#include "ipc/ipc_task.h"
#include "hydra_log/hydra_log.h"
#include "panic/panic.h"
#endif /* !FREERTOS_STANDALONE_BUILD */

/**
 * Global Variables
 */

/**
 * The interrupt nesting depth.
 *
 * Shared with the interrupt handler in portasm.asm.
 */
uint8_t interrupt_nest_count = 0;

/**
 * All ports must be written to support yield from within a critical section
 * (see comment in tasks.c). The Kalimba port supports this by setting this flag
 * and performing the yield on exit from the critical section.
 *
 * The count of the block interrupts nesting depth, i.e. how many times
 * block_interrupts has been called before a matching unblock_interrupts, is
 * also stored in the same byte. We expect this number to be very low and by
 * using the same byte it allows unblock_interrupts to be faster as it just
 * needs to read this one byte to determine what to do.
 *
 * Shared with block and unblock_interrupts in portasm.asm.
 */
uint8_t yield_and_block_count = 0;

/**
 * The bit representing whether to yield on exit from a critical section in
 * yield_and_block_count.
 */
#define YIELD_BIT (7u)

/**
 * Mask for extracting the yield on exit from a critical section bit from
 * yield_and_block_count.
 */
#define YIELD_MASK ((uint8_t)(1 << YIELD_BIT))

/**
 * Mask for extracting the interrupt block count from yield_and_block_count.
 */
#define INTERRUPT_BLOCK_COUNT_MASK ((uint8_t)(YIELD_MASK - 1))

/**
 * \brief Get the interrupt block count.
 */
#define INTERRUPT_BLOCK_COUNT_GET()                                            \
    (yield_and_block_count & INTERRUPT_BLOCK_COUNT_MASK)

/**
 * \brief Increment the interrupt block count.
 */
#define INTERRUPT_BLOCK_COUNT_INC()                                            \
    do                                                                         \
    {                                                                          \
        ++yield_and_block_count;                                               \
    } while (0)

/**
 * \brief Decrement the interrupt block count.
 */
#define INTERRUPT_BLOCK_COUNT_DEC()                                            \
    do                                                                         \
    {                                                                          \
        --yield_and_block_count;                                               \
    } while (0)

/**
 * \brief Set the yield on exit from critical section flag.
 */
#define YIELD_ON_EXIT_FROM_CRITICAL_SECTION_SET()                              \
    do                                                                         \
    {                                                                          \
        yield_and_block_count |= YIELD_MASK;                                   \
    } while (0)

/**
 * \brief Clear the yield on exit from critical section flag.
 */
#define YIELD_ON_EXIT_FROM_CRITICAL_SECTION_CLEAR()                            \
    do                                                                         \
    {                                                                          \
        yield_and_block_count &= (uint8_t)~YIELD_MASK;                         \
    } while (0)

/**
 * \brief Get the yield on exit from critical section flag.
 */
#define YIELD_ON_EXIT_FROM_CRITICAL_SECTION_GET()                              \
    (yield_and_block_count >> YIELD_BIT)

/**
 * Static Variables
 */

/**
 * The time the next tick should occur.
 */
static uint32_t next_tick_us = 0;

/**
 * When RECORD_LATENCY is enabled use the less optimal but more complex block
 * interrupts routine that can time how long interrupts have been blocked for.
 */
#ifdef RECORD_LATENCY
typedef struct latency_monitor_times_
{
    TIME start_us;
    INTERVAL max_us;
    INTERVAL panic_us;
} latency_monitor_times_t;

typedef struct latency_monitor_
{
    latency_monitor_times_t block_interrupts;
    latency_monitor_times_t interrupts;
} latency_monitor_t;

static latency_monitor_t latency =
{
    {0, 0, INT_MAX},
    {0, 0, INT_MAX}
};

/**
 * \brief Start the interrupt timer.
 *
 * To be called on interrupt entry if RECORD_LATENCY is enabled.
 */
void xPortInterruptTimingStart(void);

/**
 * \brief Stop the interrupt timer.
 *
 * To be called on interrupt exit if RECORD_LATENCY is enabled.
 */
void xPortInterruptTimingStop(void);
#endif /* RECORD_LATENCY */

/**
 * Defines
 */

/**
 * Pydbg will stop a backtrace when the PC <= 0 or the outermost frame pointer
 * is equal to the current task's stack start address.
 *
 * MDE stops the backtrace when there is no debug information for the frame.
 */
#define TASK_RETURN_ADDRESS (0x0)

/**
 * Tick rate for the system microsecond timer based on configTICK_RATE_HZ.
 */
#define TICK_RATE_US (1000000ul / configTICK_RATE_HZ)

/**
 * The maximum duration we can sleep for and still determine whether a time is
 * in the past or future is 2^31-1 us, which is ~35m47s. We need some time to
 * process the interrupt and any pre/post sleep processing user functions, these
 * should only take a few ms, but to be sure we can round the maximum sleep
 * duration down to 35 minutes, or 2,100,000,000 us.
 */
#define MAX_SLEEP_US (2100000000ul)
#if MAX_SLEEP_US >= (1ul << 31)
#error "MAX_SLEEP_US must be less than 2^31-1"
#endif

/**
 * The maximum sleep duration in ticks.
 * Relies on integer division rounding down.
 */
#define MAX_SLEEP_TICKS (MAX_SLEEP_US / TICK_RATE_US)

/**
 * \brief Convert a short ASCII string to StackType_t.
 *
 * Helps to store different constants as initial register values to aid
 * debugging.
 *
 * \param[in] _str  A string, only the first 4 characters will be used.
 */
#define ASCII_TO_STACK_TYPE(_str) ((StackType_t) \
    ((_str[3] << 24) | \
     (_str[2] << 16) | \
     (_str[1] << 8) | \
     (_str[0] << 0)))

/* Standalone builds should rely on FreeRTOS configASSERT rather than panic. */
#ifdef FREERTOS_STANDALONE_BUILD
#define port_panic_if(_condition, _panic_id, _diatribe) \
    configASSERT(!(_condition))
#else /* FREERTOS_STANDALONE_BUILD */
#define port_panic_if(_condition, _panic_id, _diatribe) \
    do \
    { \
        if(_condition) \
        { \
            panic_diatribe((_panic_id), (_diatribe)); \
        } \
    } \
    while(0)
#endif /* FREERTOS_STANDALONE_BUILD */

/**
 * Static Function Declarations
 */

/**
 * \brief Setup the timer to generate the tick interrupts.
 */
static void systick_init(void);

/**
 * \brief The systick interrupt handler (uses HW timer 2).
 */
static void systick_interrupt_handler(void);

/**
 * The interrupt handler does not need to store the entire CPU context before
 * calling the secondary interrupt handler, this can save 60 bytes from the
 * interrupt stack compared so storing everything required for a context switch.
 * This order is optimised so that only the remainder of the registers need to
 * be appended to the current stack if a context switch occurs at the end of the
 * interrupt.
 *
 * Full CPU context size         184 bytes
 * Interrupt handler stack size  124 bytes
 *
 * 0x00 FP                  0x40 L0           0x80 r5
 * 0x04 r0                  0x44 L1           0x84 r6
 * 0x08 r1                  0x48 L4           0x88 r7
 * 0x0c r2                  0x4c L5           0x8c r8
 * 0x10 rFlags              0x50 rMAC2        0x90 r9
 * 0x14 M[$ARITHMETIC_MODE] 0x54 rMAC1        0x94 I0
 * 0x18 M[$MM_RINTLINK]     0x58 rMAC0        0x98 I1
 * 0x1c r3                  0x5c DoLoopStart  0x9c I2
 * 0x20 r10                 0x60 DoLoopEnd    0xa0 I4
 * 0x24 rLink               0x64 DivResult    0xa4 I5
 * 0x28 I3                  0x68 DivRemainder 0xa8 I6
 * 0x2c I7                  0x6c B0           0xac rMACB2
 * 0x30 M0                  0x70 B1           0xb0 rMACB1
 * 0x34 M1                  0x74 B4           0xb4 rMACB0
 * 0x38 M2                  0x78 B5 [1]
 * 0x3c M3                  0x7c r4
 *
 * [1] The interrupt only stacks up to and including register B5 before calling
 *     a registered handler. The remainder of the context is stored only if a
 *     context switch happens.
 */

/**
 * \brief Setup the stack of a new task.
 *
 * Setup the stack of a new task so it is ready to be placed under the scheduler
 * control. The registers have to be placed on the stack in the order that the
 * port expects to find them.
 *
 * \param[in] top_of_stack  A pointer to the top of the stack for this task.
 * \param[in] end_of_stack  A pointer to the highest valid address in the stack
 * for this task.
 * \param[in] code  The entrypoint function pointer for the task.
 * \param[in] parameters  The task's argument.
 *
 * \return The new top of the stack.
 */
StackType_t *pxPortInitialiseStack(StackType_t *top_of_stack,
                                   StackType_t *end_of_stack,
                                   TaskFunction_t code,
                                   void *parameters)
{
    /* The Kalimba stack grows upwards! */

    /* Store the end of the stack at the very start of the stack so we can set
       the boundaries for HW stack overflow protection during context switch.
       Kalimba uses first invalid address so convert from last valid to first
       invalid by adding 1 element. */
    *top_of_stack = (StackType_t) (end_of_stack + 1);
    ++top_of_stack;

    /* FP */
    *top_of_stack = (StackType_t) top_of_stack;
    ++top_of_stack;

    /* parameters is the argument to pass to the task function (code)
       parameters is a single 32-bit pointer argument so passed via R0.
       See Kalimba C Compiler calling conventions for 32-bit targets. */
    *top_of_stack = (StackType_t) parameters;
    ++top_of_stack;

    /* Some registers default values don't matter so I've selected some ASCII defaults.
       This aids debugging as the value will be different to the stack default value
       used for calculating stack usage and we can see which registers are likely to
       have not been used.
    */
    *top_of_stack = ASCII_TO_STACK_TYPE("  R1");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  R2");
    ++top_of_stack;

    /* rFlags - write as if being restored from an interrupt context. */
    *top_of_stack = (StackType_t) INT_UM_FLAG_MASK;
    ++top_of_stack;

    /* M[$ARITHMETIC_MODE] */
    *top_of_stack = 0;
    ++top_of_stack;

    /* M[$MM_RINTLINK] - Context switches always appear as though we're returning from an
       interrupt context. code contains the task start address. */
    *top_of_stack = (StackType_t)code;
    ++top_of_stack;

    *top_of_stack = ASCII_TO_STACK_TYPE("  R3");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE(" R10");
    ++top_of_stack;

    /* rLink - The value that rLink will be when we start executing from code.
       Technically there's nowhere to return to and the task shouldn't return.
       Return to TASK_RETURN_ADDRESS.
     */
    *top_of_stack = (StackType_t) TASK_RETURN_ADDRESS;
    ++top_of_stack;

    *top_of_stack = ASCII_TO_STACK_TYPE("  I3");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  I7");
    ++top_of_stack;

    /* M0 */
    *top_of_stack = (StackType_t)0;
    ++top_of_stack;
    /* M1 */
    *top_of_stack = (StackType_t)4;
    ++top_of_stack;
    /* M2 */
    *top_of_stack = (StackType_t)-4;
    ++top_of_stack;
    /* M3 */
    *top_of_stack = ASCII_TO_STACK_TYPE("  M3");
    ++top_of_stack;

    *top_of_stack = ASCII_TO_STACK_TYPE("  L0");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  L1");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  L4");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  L5");
    ++top_of_stack;

    *top_of_stack = ASCII_TO_STACK_TYPE("MAC2");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("MAC1");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("MAC0");
    ++top_of_stack;

    /* DoLoopStart, DoLoopEnd */
    *top_of_stack = ASCII_TO_STACK_TYPE(" DLS");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE(" DLE");
    ++top_of_stack;

    /*DivResult, DivRemainder */
    *top_of_stack = ASCII_TO_STACK_TYPE("DVRS");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("DVRM");
    ++top_of_stack;

    /* rMACB2, rMACB1, rMACB0 */

    *top_of_stack = ASCII_TO_STACK_TYPE("  B0");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  B1");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  B4");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  B5");
    ++top_of_stack;

    *top_of_stack = ASCII_TO_STACK_TYPE("  R4");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  R5");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  R6");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  R7");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  R8");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  R9");
    ++top_of_stack;

    *top_of_stack = ASCII_TO_STACK_TYPE("  I0");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  I1");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  I2");
    ++top_of_stack;

    *top_of_stack = ASCII_TO_STACK_TYPE("  I4");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  I5");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("  I6");
    ++top_of_stack;

    *top_of_stack = ASCII_TO_STACK_TYPE("MBC2");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("MBC1");
    ++top_of_stack;
    *top_of_stack = ASCII_TO_STACK_TYPE("MBC0");
    ++top_of_stack;

    return top_of_stack;
}

/**
 * \brief Setup the hardware ready for the scheduler to take control.
 */
BaseType_t xPortStartScheduler(void)
{
    /* Interrupts are disabled at this point. FreeRTOS disables interrupts and
       assumes starting the scheduler will re-enable them. */

    /* In standalone builds there's no OsInit call to initialise interrupts so
       do it here. */
#ifdef FREERTOS_STANDALONE_BUILD
    hal_set_reg_int_addr((uint32)interrupt_handler);
    hal_set_reg_int_gbl_enable(1);
    hal_set_reg_int_unblock(1);
#endif /* FREERTOS_STANDALONE_BUILD */

    /* Start the timer that generates the tick ISR. */
    systick_init();

    /* Start the first task. */
    vPortStartFirstTaskAsm();

    return 0;
}

/**
 * Undo any hardware/ISR setup that was performed by xPortStartScheduler() so
 * the hardware is left in its original condition after the scheduler stops
 * executing.
 */
void vPortEndScheduler(void)
{
    /* Unimplemented */
}

/**
 * Some functions in this file are marked RUN_FROM_RAM, this is done to improve
 * performance in often executed code, SQIF access are still allowed and should
 * funcion normally.
 */

RUN_FROM_RAM
void vPortYield(void)
{
    disable_interrupts();

    /* vPortYieldAsm will unblock interrupts before context switching. */
    vPortYieldAsm();
}

RUN_FROM_RAM
void vPortYieldWithinApi(void)
{
    /* If interrupts are blocked just set a flag so that we yield as soon as
       interrupts are re-enabled again. */
    if (INTERRUPT_BLOCK_COUNT_GET())
    {
        YIELD_ON_EXIT_FROM_CRITICAL_SECTION_SET();
    }
    else
    {
        vPortYield();
    }
}

/**
 * When RECORD_LATENCY is enabled use the less optimal but more complex block
 * interrupts routine that can time how long interrupts have been blocked for.
 */
#ifdef RECORD_LATENCY
static inline void latency_monitor_start(latency_monitor_times_t *monitor)
{
    monitor->start_us = get_time();
}

static inline void latency_monitor_stop(latency_monitor_times_t *monitor)
{
    INTERVAL duration_us;

    duration_us = time_sub(get_time(), monitor->start_us);

    if(time_gt(duration_us, monitor->max_us))
    {
        monitor->max_us = duration_us;
    }

    port_panic_if(time_ge(duration_us, monitor->panic_us),
                  PANIC_HYDRA_INTERRUPT_TOO_LONG, (DIATRIBE_TYPE)duration_us);
}

static inline void block_interrupts_monitor_start(void)
{
    if(1 == INTERRUPT_BLOCK_COUNT_GET())
    {
        latency_monitor_start(&latency.block_interrupts);
    }
}

static inline void block_interrupts_monitor_stop(void)
{
    if(0 == INTERRUPT_BLOCK_COUNT_GET())
    {
        latency_monitor_stop(&latency.block_interrupts);
    }
}

void xPortInterruptTimingStart(void)
{
    if(0 == interrupt_nest_count)
    {
        latency_monitor_start(&latency.interrupts);
    }
}

void xPortInterruptTimingStop(void)
{
    /* The INT_SOURCE register will likely say which interrupt was the long one
       if the assertion is hit. Although if nested interrupts are enabled it
       may have been modified.

       The rIntLink register will contain the PC from when the interrupt
       occurred. */
    latency_monitor_stop(&latency.interrupts);
}

static inline void block_interrupts_count(void)
{
    disable_interrupts();
    INTERRUPT_BLOCK_COUNT_INC();
}

static inline void unblock_interrupts_count(void)
{
    port_panic_if(INTERRUPT_BLOCK_COUNT_GET() == 0, PANIC_HYDRA_INTERRUPT_BLOCK,
                  INTERRUPT_BLOCK_COUNT_GET());
    INTERRUPT_BLOCK_COUNT_DEC();
}

static inline void unblock_interrupts_exit(void)
{
    if(0 == INTERRUPT_BLOCK_COUNT_GET())
    {
        if(YIELD_ON_EXIT_FROM_CRITICAL_SECTION_GET())
        {
            YIELD_ON_EXIT_FROM_CRITICAL_SECTION_CLEAR();
            vPortYieldAsm();
        }
        else
        {
            enable_interrupts();
        }
    }
}

RUN_FROM_RAM
void block_interrupts(void)
{
    block_interrupts_count();

    /* Only monitor the durations of interrupts that aren't due to the chip
       going to sleep. Interrupts being blocked for extended periods whilst
       asleep is expected and doesn't impact latency, the interrupts are
       unblocked as soon as an interrupt wakes us from sleep. */
    block_interrupts_monitor_start();
}

RUN_FROM_RAM
void unblock_interrupts(void)
{
    unblock_interrupts_count();
    block_interrupts_monitor_stop();
    unblock_interrupts_exit();
}

RUN_FROM_RAM
void block_interrupts_before_sleep(void)
{
    block_interrupts_count();
}

RUN_FROM_RAM
void unblock_interrupts_after_sleep(void)
{
    unblock_interrupts_count();
    unblock_interrupts_exit();
}
#endif /* RECORD_LATENCY */

static void systick_init(void)
{
    next_tick_us = get_time();
    hal_set_reg_timer2_en(0);
    hal_set_reg_timer2_trigger(next_tick_us);
    configure_interrupt(INT_SOURCE_TIMER2, INT_LEVEL_FG,
                        systick_interrupt_handler);
    hal_set_reg_timer2_en(1);
}

RUN_FROM_RAM
static void systick_interrupt_handler(void)
{
    uint32 missed_ticks = 0;
    uint32 now = get_time();

    next_tick_us += TICK_RATE_US;
    portYIELD_FROM_ISR(xTaskIncrementTick());

    /* During debugging or in cases where interrupts have been paused for an
       extended period of time we may have missed tick interrupts. Ensure
       that next_tick_us is in the future.
    */
    while (time_le(next_tick_us, now))
    {
        ++missed_ticks;
        next_tick_us += TICK_RATE_US;

        /* The Kalimba FreeRTOS port will automatically advance the FreeRTOS
           tick for every tick that has been missed. This will only work for
           delays up to 2^31-1us, or ~35m47s. This could be time consuming
           and we're in an interrupt, if this causes problems consider making
           this behaviour optional.
        */
        portYIELD_FROM_ISR(xTaskIncrementTick());
    }

    if (missed_ticks > 0)
    {
        /* Missed ticks aren't necessarily errors or faults, they should occur
           only during debugging, but they could occur if interrupt processing
           takes too long or interrupts are blocked for too long, so we should
           at least emit a warning.
        */
#ifdef FREERTOS_STANDALONE_BUILD
        UNUSED(missed_ticks);
#else  /* FREERTOS_STANDALONE_BUILD */
        L1_DBG_MSG1("FreeRTOS Tick: Missed %u ticks", missed_ticks);
#endif /* FREERTOS_STANDALONE_BUILD */
    }

    hal_set_reg_timer2_en(0);
    hal_set_reg_timer2_trigger(next_tick_us);
    hal_set_reg_timer2_en(1);
}

void vPortCleanUpTCB(void *tcb)
{
#ifdef FREERTOS_STANDALONE_BUILD
    /* The standalone FreeRTOS build doesn't include IPC. */
    UNUSED(tcb);
#else  /* FREERTOS_STANDALONE_BUILD */
    /* Tell IPC that this task has been deleted, so it no longer needs to keep
       the IPC state associated with this task. */
    ipc_task_delete_from_handle(tcb);
#endif  /* FREERTOS_STANDALONE_BUILD */
}

/**
 * Low power tickless idle support.
 */
#if configUSE_TICKLESS_IDLE == 1

/**
 * \brief Hysteresis for the no deadline wakeup time to prevent spamming P0.
 *
 * If there's no deadline from a timer FreeRTOS must wake before MAX_SLEEP_TICKS
 * so it can calculate the number of ticks that have passed.
 *
 * If there's no deadline but we're repeatedly woken by an unpredictable
 * interrupt, e.g. a PIO, we do not want to update this deadline every time as
 * we could spam P0 with deep sleep timing messages that aren't hard deadlines.
 *
 * This hysteresis value limits how often we'll update P0 with the no deadline
 * wake time.
 *
 * It's chosen so that it prevents frequent messages to P0 but is still short
 * enough that it is testable.
 */
#define NO_DEADLINE_HYSTERESIS_US (30 * SECOND)

/**
 * \brief Check whether a time is between two times.
 *
 * The given range is not inclusive.
 *
 * \param [in] time  The time to test.
 * \param [in] lower The range lower bound (not inclusive).
 * \param [in] upper The range upper bound (not inclusive).
 *
 * \return TRUE if time is within the range (lower, upper),
 * FALSE otherwise.
 */
RUN_FROM_RAM
static bool time_in_range(TIME time, TIME lower, TIME upper)
{
    return time_gt(time, lower) && time_lt(time, upper);
}

/**
 * \brief Test whether the no deadline wake time should be updated.
 *
 * The no deadline wake time needs updating when it is no longer 'a long time'
 * in the future. 'A long time' is decided based on the maximum sleep duration
 * before we would lose track of scheduler time and some hysteresis that
 * prevents us updating the deep sleep wake time too often.
 *
 * \param [in] no_deadline_wake  The current wake time to use if there's no
 * deadline.
 *
 * \return TRUE if \p no_deadline_wake is too close to the current time, or too
 * far in the future for us to determine how much time has passed, FALSE
 * otherwise.
 */
RUN_FROM_RAM
static bool should_update_no_deadline_wake(TIME no_deadline_wake)
{
    TIME time = (TIME) time_sub(no_deadline_wake, get_time());
    TIME lower = MAX_SLEEP_US - NO_DEADLINE_HYSTERESIS_US;
    return !time_in_range(time, lower, MAX_SLEEP_US);
}

/**
 * \brief Return the time to wake if there is no deadline.
 *
 * Deadlines are only provided if the OS knows it has something to do in the
 * future or a timer will expire. This function returns the time we should
 * wake up if there is no deadline. We can't never wake as our timer might
 * wrap and we need to know how long we were asleep for to keep the scheduler
 * tick count correct.
 *
 * \return The time to wake if there is no deadline.
 */
RUN_FROM_RAM
static TIME get_no_deadline_wake_time(void)
{
    static TIME no_deadline_wake = 0;
    if(should_update_no_deadline_wake(no_deadline_wake))
    {
        no_deadline_wake = next_tick_us + TICK_RATE_US * (MAX_SLEEP_TICKS - 1);
    }
    return no_deadline_wake;
}

/**
 * \brief Return the time to wake the scheduler from tickless idle.
 *
 * \param [in] expected_idle_ticks  The number of ticks before the scheduler
 * needs to do something.
 *
 * \return The time to wake, returns the no deadline wake time if
 * \p expected_idle_ticks is set to MAX_SLEEP_TICKS.
 */
RUN_FROM_RAM
static TIME get_wake_time(TickType_t expected_idle_ticks)
{
    /* If the expected_idle_ticks is MAX_SLEEP_TICKS there is no real deadline
       other than to not let the clock wrap. To stop P1 sending repeated
       deadline updates to P0 every time we go through the sleep loop is some
       hysteresis.
    */
    if(expected_idle_ticks == MAX_SLEEP_TICKS)
    {
        return get_no_deadline_wake_time();
    }

    /* We're in between ticks, minus 1 to wake up before expected_idle_ticks
       passes.
    */
    return next_tick_us + TICK_RATE_US * (expected_idle_ticks - 1);
}

RUN_FROM_RAM
void vPortSuppressTicksAndSleep(TickType_t expected_idle_ticks)
{
    /* See https://www.freertos.org/low-power-tickless-rtos.html */

    /* Enter a critical section that will not affect interrupts bringing the
       MCU out of sleep mode. */
    block_interrupts_before_sleep();
    {
        eSleepModeStatus sleep_status;

        /* Stop the tick, the tick is the sole user of timer 2. */
        hal_set_reg_timer2_en(0);

        /* Set the maximum sleep duration so we can still calculate how long
           we've been asleep for */
        if(expected_idle_ticks > MAX_SLEEP_TICKS)
        {
            expected_idle_ticks = MAX_SLEEP_TICKS;
        }

        /* If a context switch is pending or a task is waiting for the scheduler
           to be unsuspended then abandon the low power entry. */
        sleep_status = eTaskConfirmSleepModeStatus();
        if(sleep_status != eAbortSleep)
        {
            TickType_t modifiable_idle_time = expected_idle_ticks;

            hal_set_reg_timer2_trigger(get_wake_time(expected_idle_ticks));
            hal_set_reg_timer2_en(1);

            configPRE_SLEEP_PROCESSING(modifiable_idle_time);
            if(modifiable_idle_time > 0)
            {
                enter_shallow_sleep();
            }
            configPOST_SLEEP_PROCESSING(expected_idle_ticks);
        }

        /* Determine how long we were actually in a low power state for, this
           will be less than expected_idle_ticks if we were brought out of low
           power mode by an interrupt before wake_time_us has been reached. This
           wake interrupt could even be the systick if the timer expired after
           interrupts were blocked and before the timer was disabled.

           Note that the scheduler is suspended before
           portSUPPRESS_TICKS_AND_SLEEP() is called, and resumed when
           portSUPPRESS_TICKS_AND_SLEEP() returns. Therefore no other tasks will
           execute until this function completes, even after we unblock
           interrupts.
        */
        {
            INTERVAL diff_us = time_sub(get_time(), next_tick_us);
            if (diff_us >= (INTERVAL)TICK_RATE_US)
            {
                /* The time the tick was set to before we went to sleep is at
                   least 1 tick in the past, scheduler time needs advancing. */
                uint32 whole_ticks_to_step = (TIME)diff_us / TICK_RATE_US;

                /* Limit to the expected number of whole ticks, the minus one is
                   because expected_idle_ticks includes partial ticks. This
                   prevents FreeRTOS failing an assertion due to waking up late
                   when we could have been waiting on a breakpoint. The systick
                   handler will log the number of unexpected ticks skipped. */
                if (whole_ticks_to_step > expected_idle_ticks - 1)
                {
                    whole_ticks_to_step = expected_idle_ticks - 1;
                }

                /* Advance to the most recent expired tick. The systick handler
                   will catch up with the last tick (or more if the CPU has been
                   paused on a breakpoint). The systick will handle potential
                   context switches, vTaskStepTick doesn't. */
                next_tick_us += whole_ticks_to_step * TICK_RATE_US;
                vTaskStepTick(whole_ticks_to_step);
            }
        }

        /* Resume systick */
        hal_set_reg_timer2_en(0);
        hal_set_reg_timer2_trigger(next_tick_us);
        hal_set_reg_timer2_en(1);
    }
    unblock_interrupts_after_sleep();
}

#endif /* configUSE_TICKLESS_IDLE == 1 */
