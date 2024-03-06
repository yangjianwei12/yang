/****************************************************************************
 * Copyright (c) 2015 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file malloc_pm.c
 * \ingroup malloc_pm
 *
 * PM memory allocation/free functionality
 */

/****************************************************************************
Include Files
*/

#include "malloc_pm/malloc_pm_private.h"

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Initialise memory heap
 */
void init_malloc_pm(void)
{
    init_heap_pm();
}

/**
 * \brief Initialise the offset used to calculate the PM start address
 *
 * \param[in] offset number of bytes (see notes for what that means) used for
 * other systems (e.g. patchpoints) from PM space that is shared with PM malloc
 *
 * \note The offset can only be set before PM malloc is initialised by invoking
 *  init_malloc_pm
 *
 * \note
 *   The offset is specified in units of the smallest addressable storage unit
 *   in PM on the processor used. This is 8 bits on KAL_ARCH4 and KAL_ARCH5 or
 *   32 bits on KAL_ARCH3.
 *
 */
void malloc_pm_init_start_offset(unsigned offset)
{
    heap_pm_init_start_offset(offset);
}

#if defined(SUPPORTS_MULTI_CORE)
/**
 * \brief Function called when the state of the secondary processor changes.
 *
 * \param transition What is happening to the secondary core.
 */
void malloc_pm_processor_change(PROC_TRANSITION transition)
{
    if (transition == PROC_DISABLE)
    {
        pm_disable_secondary_processor();
    }
    else if (transition == PROC_START)
    {
        pm_start_secondary_processor();
    }
    else if (transition == PROC_STOP)
    {
        pm_stop_secondary_processor();
    }
}
#endif /* defined(SUPPORTS_MULTI_CORE) */

/**
 * \brief PM memory allocation, based on heap
 *
 * Allocate a chunk of memory from the heap. Returns a Null pointer and raises
 * a fault if not enough memory is available. The memory is not initialised.
 *
 * \param[in] numOctets number of octets required.
 * \param[in] preference Heap in which we want to allocate the memory
 *
 * \return pointer to the block of memory allocated
 */
void_func_ptr xpmalloc_pm(unsigned int numOctets, unsigned int preference)
{
    void_func_ptr addr;

    /* xpmalloc_pm must return NULL if requested to allocate zero bytes. */
    if (numOctets == 0)
    {
        return NULL;
    }

    addr = heap_alloc_pm(numOctets, preference);
    if (addr == NULL)
    {
        fault_diatribe(FAULT_AUDIO_INSUFFICIENT_PROGRAM_MEMORY, numOctets);
    }
    return addr;
}

/**
 * \brief free memory allocated using versions of malloc
 *
 * Returns previously allocated memory to the PM memory heap.
 *
 * \param[in] pMemory pointer to the memory to be freed
 *
 * \note It requires that the memory just above the top of the
 *       buffer pointed to by pMemory contains the block header information.
 *       This will be the case assuming the memory was allocated using
 *       xpmalloc_pm and there has been not been memory corruption.
 */
void free_pm(void_func_ptr pMemory)
{
    patch_fn_shared(malloc_pm);

    if (pMemory == NULL)
    {
        return;
    }

    PL_PRINT_P1(TR_PL_FREE, "freeing PM memory %p\n", pMemory);
    if (is_in_pm_heap(pMemory))
    {
        PL_PRINT_P0(TR_PL_FREE, "freeing PM memory from heap\n");
        heap_free_pm(pMemory);
        return;
    }
}

