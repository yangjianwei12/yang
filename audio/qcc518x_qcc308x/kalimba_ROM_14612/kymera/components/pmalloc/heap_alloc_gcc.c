/****************************************************************************
 * Copyright (c) 2012 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file heap_alloc_gcc.c
 * \ingroup pl_malloc
 *
 * Host specific heap memory management.
 */

/****************************************************************************
Include Files
*/

#include "pmalloc/pl_malloc_private.h"

/* local pointer to the processor copy of heap_config */
static heap_config *pheap_info;

void heap_config_static(heap_config *processor_heap_info_list)
{
    size_t size;
    char *ptr;

    size = NUMBER_DM_BANKS * DM_RAM_BANK_SIZE;
    ptr = calloc(size, sizeof(char));
    PL_ASSERT(ptr != NULL);
    processor_heap_info_list[0].heap[HEAP_MAIN].heap_start = ptr;
    processor_heap_info_list[0].heap[HEAP_MAIN].heap_end = ptr + size;
    processor_heap_info_list[0].heap[HEAP_MAIN].heap_size = size;
    processor_heap_info_list[0].heap[HEAP_MAIN].heap_free = 0;

#if defined(CHIP_HAS_SLOW_DM_RAM)
    size = NUMBER_DM_BANKS * DM_RAM_BANK_SIZE;
    ptr = calloc(size, sizeof(char));
    PL_ASSERT(ptr != NULL);
    processor_heap_info_list[0].heap[HEAP_SLOW].heap_start = ptr;
    processor_heap_info_list[0].heap[HEAP_SLOW].heap_end = ptr + size;
    processor_heap_info_list[0].heap[HEAP_SLOW].heap_size = size;
    processor_heap_info_list[0].heap[HEAP_SLOW].heap_free = 0;
#endif /* defined(CHIP_HAS_SLOW_DM_RAM) */

    size = DM_RAM_BANK_SIZE;
    ptr = calloc(DM_RAM_BANK_SIZE, sizeof(char));
    PL_ASSERT(ptr != NULL);
    processor_heap_info_list[0].heap[HEAP_SHARED].heap_start = ptr;
    processor_heap_info_list[0].heap[HEAP_SHARED].heap_end = ptr + size;
    processor_heap_info_list[0].heap[HEAP_SHARED].heap_size = size;
    processor_heap_info_list[0].heap[HEAP_SHARED].heap_free = 0;

#if defined(INSTALL_EXTERNAL_MEM)
    size = EXT_HEAP_SIZE;
    ptr = calloc(size, sizeof(char));
    PL_ASSERT(ptr != NULL);
    processor_heap_info_list[0].heap[HEAP_EXT].heap_start = ptr;
    processor_heap_info_list[0].heap[HEAP_EXT].heap_end = ptr + size;
    processor_heap_info_list[0].heap[HEAP_EXT].heap_size = size;
    processor_heap_info_list[0].heap[HEAP_EXT].heap_free = 0;
#endif /* defined(INSTALL_EXTERNAL_MEM) */
}

void heap_get_allocation_rules(heap_allocation_rules *rules)
{
    memset(rules, 0, sizeof(heap_allocation_rules));
    rules->relaxmallocstrictness = MALLOC_STRICTNESS_DM1_FALLBACK;
}

void heap_config_dynamic(heap_config *processor_heap_info_list)
{
    /* Keep a pointer to the global structures for the
       benefit of "heap_get_freestats". */
    pheap_info = &processor_heap_info_list[0];
#if defined(INSTALL_EXTERNAL_MEM)
    ext_malloc_enable(TRUE);
#endif
}

void heap_config_hardware(heap_config *processor_heap_info_list)
{
    NOT_USED(processor_heap_info_list);
}

/**
 * \brief Test-only function to get total and largest free space.
 */
void heap_get_freestats(unsigned *maxfree,
                        unsigned *totfree,
                        unsigned *tracked_free)
{
    unsigned heap_num, tot_size = 0, max_size = 0, tracked = 0;
    mem_node *curnode;

    for (heap_num = 0; heap_num < HEAP_ARRAY_SIZE; heap_num++)
    {
        tracked += pheap_info->heap[heap_num].heap_free;
        curnode = heap_get_freelist(heap_num);
        while (curnode != NULL)
        {
            if (curnode->length - GUARD_SIZE > max_size)
            {
                max_size = curnode->length - GUARD_SIZE;
            }
            tot_size += curnode->length - GUARD_SIZE;
            curnode = curnode->u.next;
        }
    }

    if (maxfree != NULL)
    {
        *maxfree = max_size;
    }
    if (totfree != NULL)
    {
        *totfree = tot_size;
    }
    if (tracked_free != NULL)
    {
        *tracked_free = tracked;
    }
}

/**
 * \brief Test-only function to get start of heap
 */
char *heap_get_start(heap_names heap_num)
{
    return pheap_info->heap[heap_num].heap_start;
}