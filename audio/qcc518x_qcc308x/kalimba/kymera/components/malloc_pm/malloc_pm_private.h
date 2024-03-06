/****************************************************************************
 * Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file malloc_pm_private.h
 * \ingroup malloc_pm
 *
 * Header for the internals of the PM memory allocation system
 *
 * This file contains bits of the PM memory system that we don't want
 * users to see.
 */

#ifndef MALLOC_PM_PRIVATE_H
#define MALLOC_PM_PRIVATE_H

/****************************************************************************
Include Files
*/

#include "malloc_pm/malloc_pm.h"
#include "platform/pl_trace.h"
#include "platform/pl_intrinsics.h"
#include "panic/panic.h"
#include "fault/fault.h"
#include "patch/patch.h"
#include "io_map.h"

/****************************************************************************
Public Macro Declarations
*/

#ifdef __KCC__
#ifdef KAL_ARCH4
#ifndef UNIT_DATA_SECTION_DEFINED
#define UNIT_DATA_SECTION_DEFINED
/* Keep all configuration user P0 for dual core*/
#pragma unitzeroinitdatasection DM_P0_RW_ZI
#pragma unitdatasection         DM_P0_RW
#endif
#endif /* KAL_ARCH4 */
#endif /* __KCC__ */

/****************************************************************************
Public Function Prototypes
*/

/**
 * \brief Initialise the offset used to calculate the heap start address
 *
 * \param[in] offset number of bytes (see notes for what that means) used for
 * other systems (e.g. patchpoints) from PM space that is shared with P0's heap
 *
 * \note The offset address can only be set before PM heap is initialised
 *
 * \note
 *   The offset is specified in units of the smallest addressable storage unit
 *   in PM on the processor used.
 */
extern void heap_pm_init_start_offset(unsigned offset);

/**
 * \brief Initialise memory heap
 */
extern void init_heap_pm(void);

#if defined(SUPPORTS_MULTI_CORE)
extern void pm_disable_secondary_processor(void);
extern void pm_start_secondary_processor(void);
extern void pm_stop_secondary_processor(void);
#else
#define pm_stop_secondary_processor()
#endif

/**
 * \brief Memory allocation using heap
 *
 * \param[in] size number of addressable units required.
 * \param[in] preference_core core in which we want to allocate the memory
 *
 * \return pointer to the block of memory allocated
 *
 * \note The size is specified in units of the smallest addressable storage
 *       unit in PM on the processor used.
 * \note The memory is not initialised.
 */
extern void_func_ptr heap_alloc_pm(unsigned size_byte,
                                   unsigned preference_core);

/**
 * \brief Free memory allocated from the PM heap
 *
 * \param[in] ptr pointer to the memory to be freed
 */
extern void heap_free_pm(void_func_ptr ptr);

/**
 * \brief Returns whether provided address is within the boundaries
 * of the PM heap
 *
 * \param[in] ptr pointer to the memory
 *
 * \return True if it is, false otherwise.
 *
 */
extern bool is_in_pm_heap(void_func_ptr);

#endif /* MALLOC_PM_PRIVATE_H */

