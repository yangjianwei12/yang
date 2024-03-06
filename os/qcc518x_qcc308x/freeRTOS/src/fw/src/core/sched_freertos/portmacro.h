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
#ifndef PORTMACRO_H
#define PORTMACRO_H

#include "kal_utils/kal_utils.h"
#include "int/int.h"

/*-----------------------------------------------------------
 * Port specific definitions.
 *
 * The settings in this file configure FreeRTOS correctly for the
 * given hardware and compiler.
 *
 * These settings should not be altered.
 *-----------------------------------------------------------
 */

/* Type definitions. */
#define portCHAR        char
#define portFLOAT       float
#define portDOUBLE      double
#define portLONG        long
#define portSHORT       short
#define portSTACK_TYPE  uint32_t
#define portBASE_TYPE   long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

/**
 * Accesses to the tick are atomic whether they are 16 or 32 bit.
 */
#define portTICK_TYPE_IS_ATOMIC (1)

/**
 * By default we use 32-bit ticks, 16-bit ticks could save some memory.
 */
#if configUSE_16_BIT_TICKS == 1
typedef uint16_t TickType_t;
#define portMAX_DELAY ((TickType_t) 0xffff)
#else /* configUSE_16_BIT_TICKS == 1 */
typedef uint32_t TickType_t;
#define portMAX_DELAY ((TickType_t) 0xffffffffUL)
#endif  /* configUSE_16_BIT_TICKS == 1 */

/**
 * Architecture specifics.
 */

/**
 * The Kalimba stack grows upwards.
 */
#define portSTACK_GROWTH (1)

/**
 * Kalimba has stack overflow protection support in hardware.
 */
#define portHAS_STACK_OVERFLOW_CHECKING (1)

/**
 * The byte alignment is used for checking stack pointer alignment.
 * Our stack type is 32-bits, so this should be set to 4.
 */
#define portBYTE_ALIGNMENT (4)


/**
 * The period between FreeRTOS scheduler ticks in milliseconds.
 */
#define portTICK_PERIOD_MS ((TickType_t) 1000 / configTICK_RATE_HZ)

/**
 * KCC inline keyword, used by some FreeRTOS libraries, e.g. FreeRTOS+FAT.
 */
#define portINLINE inline

/**
 * Scheduler utilities.
 */

/**
 * \brief Kalimba port yield.
 *
 * Switch to another task of the same priority if one is runnable.
 */
void vPortYield(void);

/**
 * \brief Yield definition that the FreeRTOS scheduler uses.
 */
#define portYIELD() vPortYield()

/**
 * \brief Yield from within an API call.
 *
 * This variant of yield needs to cope with yielding whilst interrupts are
 * blocked. The Kalimba implementation defers the context switch until
 * interrupts are unblocked.
 */
void vPortYieldWithinApi(void);

/**
 * \brief Yield from wthin an API function that the FreeRTOS scheduler uses.
 */
#define portYIELD_WITHIN_API() vPortYieldWithinApi()

/**
 * Indicates whether a context switch should be performed on exit from the
 * interrupt. Set by portYIELD_FROM_ISR().
 *
 * Declared in portasm.asm.
 */
extern uint32_t context_switch_on_return_from_interrupt;

/**
 * \brief Perform a context switch at the end of the ISR.
 *
 * \param[in] _switch_required  Set to non-zero if a switch is required.
 */
#define portEND_SWITCHING_ISR(_switch_required) do { \
    if((_switch_required)) \
    { \
        context_switch_on_return_from_interrupt = 1; \
    } \
} while(0)

/**
 * \brief Yield from within an ISR.
 *
 * \param[in] _switch_required  Set to non-zero if a yield is required.
 */
#define portYIELD_FROM_ISR(_switch_required) \
    portEND_SWITCHING_ISR(_switch_required)

/**
 * Critical section management.
 *
 * Compiler memory barriers are required before and after enabling and disabling
 * interrupts to prevent the compiler migrating any code either side of these
 * calls.
 */

/**
 * \brief Disable interrupts, non-nesting version.
 */
#define disable_interrupts() do { \
    portMEMORY_BARRIER(); \
    hal_set_reg_int_block_priority(INT_LEVEL_EXCEPTION); \
    portMEMORY_BARRIER(); \
    } while(0)

/**
 * \brief enable_interrupts, non-nesting version.
 */
#define enable_interrupts() do { \
    portMEMORY_BARRIER(); \
    hal_set_reg_int_block_priority(0); \
    portMEMORY_BARRIER(); \
    } while(0)

/**
 * \brief Disable interrupts function that the FreeRTOS scheduler uses.
 */
#define portDISABLE_INTERRUPTS() disable_interrupts()

/**
 * \brief Enable interrupts function that the FreeRTOS scheduler uses.
 */
#define portENABLE_INTERRUPTS() enable_interrupts()

/**
 * \brief Block interrupts function that the FreeRTOS scheduler uses.
 */
#define portENTER_CRITICAL() block_interrupts()

/**
 * \brief Unblock interrupts function that the FreeRTOS scheduler uses.
 */
#define portEXIT_CRITICAL() unblock_interrupts()

/**
 * \brief Task function prototype
 *
 * Allows for compiler specific extensions to be added to the
 * task function prototype.
 */
#define portTASK_FUNCTION_PROTO(_function, _parameters) \
    void _function(void *_parameters)

/**
 * \brief Task function
 *
 * Allows for compiler specific extensions to be added to the
 * task function.
 */
#define portTASK_FUNCTION(_function, _parameters) \
    void _function(void *_parameters)

/** Desktop and Lint builds cannot understand Kalimba inline assembly. */
#if defined(DESKTOP_TEST_BUILD) || defined(_lint)
#define memory_barrier() do { } while(0)
#else /* defined(DESKTOP_TEST_BUILD) || defined(_lint) */
/**
 * \brief Kalimba inline assembly code to perform a compiler memory barrier.
 *
 * KCC will not reorder loads or stores either side of a call to this function.
 */
asm void memory_barrier(void)
{
    @[.barrier]
}
#endif /* defined(DESKTOP_TEST_BUILD) || defined(_lint) */

/**
 * \brief Compiler memory barrier definition that the FreeRTOS scheduler uses.
 */
#define portMEMORY_BARRIER() memory_barrier()


/**
 * \brief Extern of the interrupt nest count.
 *
 * Used for determining if we're in the ISR context, should only be used through
 * xPortInIsrContext() macro rather than referencing it directly.
 */
extern uint8_t interrupt_nest_count;

/**
 * \brief Determine if the CPU is currently in the ISR.
 *
 * \return TRUE if the CPU is currently in the ISR, FALSE otherwise.
 */
#define xPortInIsrContext() (interrupt_nest_count > 0)

/**
 * \brief Assert that the CPU is not currently in the ISR.
 */
#define portASSERT_IF_IN_ISR() configASSERT(!xPortInIsrContext())

/**
 * \brief Callback from FreeRTOS when a task is deleted.
 *
 * \param[in] tcb  The TCB / task handle that has been deleted.
 */
void vPortCleanUpTCB(void *tcb);

/**
 * \brief Clean up TCB function that the FreeRTOS scheduler uses.
 */
#define portCLEAN_UP_TCB vPortCleanUpTCB

/**
 * Architecture specific optimisations.
 */

/**
 * Enable the optimised task selection code if it hasn't already been defined.
 *
 * The optimised task selection code maintains a bitset of ready priorities
 * avoiding the need to search arrays.
 */
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
#define configUSE_PORT_OPTIMISED_TASK_SELECTION (1)
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1

/* Since the implementation uses a bitset to determine the next task the
   maximum number of priorities is limited to 31. */
#if configMAX_PRIORITIES > 31
#error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when \
    configMAX_PRIORITIES is less than or equal to 31.
#endif

/**
 * \brief Add a priority to the bitset.
 *
 * \param[in]     _priority          The priority to set.
 * \param[in,out] _ready_priorities  The priority bitset.
 */
#define portRECORD_READY_PRIORITY(_priority, _ready_priorities) \
    do { \
        (_ready_priorities) |= (1UL << (_priority)); \
    } while(0)

/**
 * \brief Remove a priority from the bitset.
 *
 * \param[in]     _priority          The priority to clear.
 * \param[in,out] _ready_priorities  The priority bitset.
 */
#define portRESET_READY_PRIORITY(_priority, _ready_priorities) \
    do { \
        (_ready_priorities) &= ~(1UL << (_priority)); \
    } while(0)

/**
 * \brief Get the highest priority set in the priorities bitset.
 *
 * \param[out] _top_priority      The highest priority set in the bitset.
 * \param[in]  _ready_priorities  The priority bitset.
 */
#define portGET_HIGHEST_PRIORITY(_top_priority, _ready_priorities) \
    do { \
        (_top_priority) = MAX_BIT_POS_31((uint32_t) (_ready_priorities)); \
    } while(0)

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

/**
 * Low power tickless idle support.
 */
#ifndef portSUPPRESS_TICKS_AND_SLEEP

/**
 * \brief Disable FreeRTOS ticks and sleep until there's work to do.
 *
 * \param[in] expected_idle_ticks  The number of ticks until FreeRTOS needs to
 * do something.
 */
void vPortSuppressTicksAndSleep(TickType_t expected_idle_ticks);

/**
 * \brief Suppress ticks and sleep function that the FreeRTOS scheduler uses.
 */
#define portSUPPRESS_TICKS_AND_SLEEP(_expected_idle_ticks) \
    vPortSuppressTicksAndSleep(_expected_idle_ticks)

#endif /* !portSUPPRESS_TICKS_AND_SLEEP */

#endif /* PORTMACRO_H */
