/****************************************************************************
 * Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup malloc_pm Management of executable memory.
 * \file malloc_pm.h
 * \ingroup malloc_pm
 *
 * Public definitions of functions related to managing program memory.
 * Unlike data memory, program memory is reachable via the instruction bus
 * so functions can be stored into it.
 */

#ifndef MALLOC_PM_H
#define MALLOC_PM_H

#include "proc/proc.h"

/****************************************************************************
Public Macro Declarations
*/

/* Attribute macros that can be used to put specific items in Kalimba PM
 * Defined to nothing for GCC builds */

#define MALLOC_PM_PREFERENCE_CORE_0 0
#define MALLOC_PM_PREFERENCE_CORE_1 1
#define MALLOC_PM_PREFERENCE_SLOW   2

/****************************************************************************
Public Type Declarations
*/

/** A pointer to a block of memory reachable through the instructions bus.
 */
typedef void (*void_func_ptr)(void);

/****************************************************************************
Public Function Prototypes
*/

/**
 * \brief Initialize the malloc_pm component.
 *
 * \note This function must be called before any other function
 *       from this component are being called.
 */
extern void init_malloc_pm(void);

/**
 * \brief Make some memory at the beginning of the program
 *        memory unavailable for allocation.
 *
 * \param offset Number of octets to set aside.
 *
 * \note This function is used exclusively by the patch component.
 */
extern void malloc_pm_init_start_offset(unsigned offset);

#if defined(SUPPORTS_MULTI_CORE)
/**
 * \brief Function called when the state of the secondary processor changes.
 *
 * \param transition What is happening to the secondary core.
 */
extern void malloc_pm_processor_change(PROC_TRANSITION transition);
#endif

/**
 * \brief Allocate a block of memory accessible via the instruction bus.
 *
 * \param numOctets       Number of octets to allocated.
 * \param preference_core Which heap to use. 
 *
 * \return NULL in case of failure, otherwise a pointer
 *         to the desired amount of memory.
 */
extern void_func_ptr xpmalloc_pm(unsigned int numOctets,
                                 unsigned int preference);

/**
 * \brief Free some memory allocated using xpmalloc_pm.
 *
 * \param pMemory A pointer to previously allocated memory.
 */
extern void free_pm(void_func_ptr pMemory);

/**
 * \brief Get a pointer to a piece of program memory accessible
 *        via the data bus.
 *
 * \param ptr A pointer returned by function xpmalloc_pm.
 *
 * \note The pointer remains valid until function malloc_pm_close_window
 *       is called.
 */
extern uint32 *malloc_pm_open_window(void_func_ptr ptr);

/**
 * \brief Return ownership of a piece of program memory to the
 *        instruction bus.
 *
 * \param ptr A pointer returned by function xpmalloc_pm.
 */
extern void malloc_pm_close_window(void_func_ptr ptr);

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
/**
 * \brief Returns whether provided address is within the boundaries
 * of the slow PM heap.
 *
 * \param[in] ptr pointer to the memory
 *
 * \return True if it is, false otherwise.
 *
 */
extern bool is_in_slow_pm_heap(void_func_ptr ptr);

/**
 * \brief Returns whether provided address is in the patch
 * space.
 *
 * \param[in] address The address
 *
 * \return True if it is, false otherwise.
 *
 */
extern bool is_a_patch_address(unsigned address);
#else
/* This function is only used in the download manager. It is only
 * relevant if CHIP_HAS_NVRAM_ACCESS_TO_DM is defined. */
#define is_a_patch_address(x)      FALSE
#endif

#endif /* MALLOC_PM_H */

