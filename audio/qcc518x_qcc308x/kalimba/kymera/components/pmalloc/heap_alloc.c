/****************************************************************************
 * Copyright (c) 2012 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file heap_alloc.c
 * \ingroup pl_malloc
 *
 * Common heap memory management.
 *
 * Each chip's Data Memory space is divided into areas. The area at the
 * lowest addressable location is assigned at link time to the static
 * variables. Then come the shared heap, the optional slow heap, the main
 * heap, the optional extra heap and finally the optional nvram area.
 * There are also one optional heap per processor for memory located
 * in a separate chip on the board.
 *
 * Examples of Data Memory space layout:
 *
 * \verbatim
 *        QCC512x                QCC515x                QCC517x
 * +-------------------+  +-------------------+  +-------------------+
 * |    P1 Main heap   |  |    P1 Main heap   |  |   P1 NVRAM heap   |
 * +-------------------+  +-------------------+  +-------------------+
 * |    P0 Main heap   |  |    P0 Main heap   |  |   P0 NVRAM heap   |
 * +-------------------+  +-------------------+  +-------------------+
 * |   P1 Shared heap  |  |    Shared heap    |  |   P1 Extra heap   |
 * +-------------------+  +-------------------+  +-------------------+
 * |   P1 Shared heap  |  |  Static variables |  |   P0 Extra heap   |
 * +-------------------+  +-------------------+  +-------------------+
 * |  Static variables |                         |    P1 Main heap   |
 * +-------------------+                         +-------------------+
 *                                               |    P0 Main heap   |
 *                                               +-------------------+
 *                                               |    Shared heap    |
 *                                               +-------------------+
 *                                               |  Static variables |
 *                                               +-------------------+
 * \endverbatim
 *
 * The area for the static variables is ignored by this component. The
 * optional nvram area is used exclusively by the "malloc_pm" component
 * using a specific API. The heaps are exposed to the entire system
 * including customers capabilities via the preference option of the
 * "pmalloc" API.
 *
 * Each heap is described by the "heap_info" type, the use of which
 * member is described in the following paragraphs:
 *
 * \code{.c}
 * typedef struct heap_size_info
 * {
 *     char *heap_start;
 *     char *heap_end;
 *     unsigned heap_size;
 *     unsigned heap_free;
 *     char *heap_guard;
 * } heap_info;
 * \endcode
 *
 * When a chip boots, it has no access to the other subsystems so it
 * does not have access to the per-product configuration. However heap
 * memory is needed to get said per-product configuration so some static
 * heaps are created by the "init_heap" function. The boundaries of the
 * static heaps are obtained by calling the "heap_config_static" function
 * from file "heap_alloc_kcc.c". In particular, at this stage, the optional
 * heaps will be disabled by setting all members of the structure to NULL
 * or 0. The resulting heaps are used in a limited manner by always
 * allocating memory at the bottom which will allow resizing them later on.
 *
 * Once the per-product configuration has been downloaded into the audio
 * subsystem, the "config_heap" function is called. It, in turn, obtains the
 * boundaries of all the heaps by calling the "heap_config_dynamic" function
 * from file "heap_alloc_kcc.c".
 *
 * Some heaps will be disabled by configuration. The members "heap_start" and
 * "heap_end" will be set to NULL and the member "heap_size" will be set to 0.
 * If the value of member "heap_size" has changed from what was returned during
 * static configuration then the heap will be resized by calling the
 * "update_freelist_with_new_end" function.
 *
 * For some chips that support it, any memory that is not claimed for a
 * heap will be disabled so that the underlying memory for them can be powered
 * down when function "heap_config_hardware" is called. The same function also
 * sets memory access permission to help catch programming errors.
 *
 * Because some low level algorithms benefit from reading ahead possibly
 * outside of their allocated buffers, a bit of memory is set aside at the
 * end of each heap. This is done by the "heap_update_guard" function. The
 * fixed sized allocated chunk of memory is saved into member "heap_guard".
 *
 * If a chip allow powering down memory, then its "heap_processor_change"
 * function will be called just after "config_heap". This will disable the
 * heaps and power off the memory reserved to the secondary processors until
 * they are enabled by calling again the "heap_processor_change" function.
 * A powered off heap can be checking that its member "heap_guard" is set to
 * NULL.
 *
 * The area for each heap is covered by nodes. Each node can be either free
 * in which case it is linked in a list of free nodes or allocated. This
 * information is put in the node's header. The header is followed by the
 * usable space and by a optional trailer used by debug builds to detect
 * buffer overflows when the node is freed.
 *
 * The first node of a heap is created using the "init_heap_node" function
 * that ensures that the value of member "heap_free" is equal to the value
 * of member "heap_size" minus the size of the header. This member is used
 * later to find out quickly if there is not enough memory available for a
 * new allocation without having to traverse the list of free nodes.
 *
 * Nodes can be allocated from the bottom of the heap or from the top of the
 * heap. This is done so to avoid having simultaneous accesses from the DM1
 * and DM2 bus to the same underlying memory block which would cause wait
 * cycles to be inserted and performance to be reduced. The heaps are also
 * separated between cores except for the shared heap which is explicitly
 * meant to be shared between processors.
 *
 * See file "heap_alloc_kcc.c" for more information about how the boundaries
 * of the heaps are calculated.
 */

/****************************************************************************
Include Files
*/

#include "pmalloc/pl_malloc_private.h"
#if defined(COMMON_SHARED_HEAP)
#include "hal/hal_hwsemaphore.h"
#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
#include "hal/hal_dm_sections.h"
#include "platform/pl_hwlock.h"
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */
#endif /* COMMON_SHARED_HEAP */

/****************************************************************************
Private Macro Declarations
*/

#define MIN_SPARE 8

/* Amount of memory reserved at the end of the memory to allow
 * optimizing tight loops. Performing a read access outside
 * of the allocated range of a buffer and discarding its value
 * improves performance of some timing critical functions.
 *
 * If a buffer were to be allocated at the end of physical memory,
 * this would cause an exception so artificially reduce the range
 * of memory that is made available out of the main heap.
 */
#define HEAP_GUARD_AREA_SIZE 64

#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
/* HEAP_ALLOC_SEMAPHORE_RETRIES = HEAP_ALLOC_SEMAPHORE_RETRIES + 5 */
#define HEAP_ALLOC_HW_LOCK_RETRIES      55
#else
#define HEAP_ALLOC_SEMAPHORE_RETRIES    50
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */

/****************************************************************************
Private Variable Definitions
*/

/* Free list and minimum heap arrays
 * Minimum heap array will be initialised at pmalloc_init() by P0
 * pmalloc_config() will extend the heap beyond this array if free
 * memory available as per the memory map.
 * heap boundaries are externally visible only for the minimum heap sizes.
 * For the expanded heap boundaries after configuration needs to be calculated
 * using the configured sizes.
 */

static DM_SHARED_ZI heap_allocation_rules heap_rules;

/* The heap info is shared between P0 and P1 if IPC installed */
static DM_SHARED_ZI heap_config processor_heap_info_list[PROC_PROCESSOR_BUILD];

/* local pointer to the processor copy of heap_config */
static heap_config *pheap_info;

#if defined(COMMON_SHARED_HEAP) && defined(__KCC__)

#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
DM_SHARED_ZI static unsigned heap_alloc_lock;
/* The number of HW lock retries when allocating/freeing memory in the
 * shared heap. */
unsigned heap_alloc_num_retries = HEAP_ALLOC_HW_LOCK_RETRIES;
#else
/* The number of semaphore retries when allocating/freeing memory in the
 * shared heap. */
unsigned heap_alloc_num_retries = HEAP_ALLOC_SEMAPHORE_RETRIES;
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */
#endif /* COMMON_SHARED_HEAP */

/****************************************************************************
Private Function Declarations
*/

static void heap_update_guard(heap_names heap_num);

/****************************************************************************
Private Function Definitions
*/

/**
 * \brief Get the processor id, based on the heap_num.
 */
static inline PROC_ID_NUM get_proc_id(heap_names heap_num)
{
    PROC_ID_NUM proc_id;

    proc_id = proc_get_processor_id();
#if defined(COMMON_SHARED_HEAP)
    if (heap_num == HEAP_SHARED)
    {
        proc_id = PROC_PROCESSOR_0;
    }
#endif /* COMMON_SHARED_HEAP */

    return proc_id;
}

/**
 * \brief Lock the interrupts of get the semaphore. The semaphore function
 *        getter locks the interrupts as well.
 */
static inline void acquire_lock(heap_names heap_num)
{
#if defined(COMMON_SHARED_HEAP) && defined(__KCC__)
    if (PROC_PRIMARY_CONTEXT() || (heap_num == HEAP_SHARED))
    {
#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
        pl_hwlock_get_with_retry(&heap_alloc_lock, heap_alloc_num_retries);
#else
        hal_hwsemaphore_get_with_retry(HWSEMIDX_HEAP_MEM, heap_alloc_num_retries);
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */
    }
    else
#endif /* COMMON_SHARED_HEAP */
    {
        LOCK_INTERRUPTS;
    }
}

/**
 * \brief Unlock the interrupts or release the semaphore (and unlock interrupts).
 */
static inline void release_lock(heap_names heap_num)
{
#if defined(COMMON_SHARED_HEAP) && defined(__KCC__)
    if (PROC_PRIMARY_CONTEXT() || (heap_num == HEAP_SHARED))
    {
#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
        pl_hwlock_rel(&heap_alloc_lock);
#else
        hal_hwsemaphore_rel(HWSEMIDX_HEAP_MEM);
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */
    }
    else
#endif /* COMMON_SHARED_HEAP */
    {
        UNLOCK_INTERRUPTS;
    }
}

/**
 * \brief Initialise the heap mem node.
 */
static mem_node *init_heap_node(char *heap, unsigned heap_size)
{
    unsigned freelength;
    heap_names heap_num;
    mem_node *node = (mem_node *) heap;
    heap_config *config;
    PROC_ID_NUM proc_id;

    PL_ASSERT((heap_size > sizeof(mem_node) + GUARD_SIZE) && (heap != NULL));

    freelength = heap_size - sizeof(mem_node);
    node->u.next = NULL;
    node->length = freelength;

    /* find out the location of the free node */
    heap_num = get_heap_num((void*) heap);
    proc_id = get_proc_id(heap_num);
    config = &processor_heap_info_list[proc_id];

    config->heap[heap_num].heap_free += freelength - GUARD_SIZE;

    config->heap_debug_free += freelength - GUARD_SIZE;

    return node;
}

#if defined(HAVE_EXTERNAL_HEAP)
static bool ext_heap_is_disabled(void)
{
    return (pheap_info->freelist[HEAP_EXT] == NULL);
}
#endif

#if defined(HAVE_EXTERNAL_HEAP) || \
    defined(DM_BANKS_CAN_BE_POWERED_OFF)
static bool heap_is_powered_on(heap_names heap_num)
{
#if defined(HAVE_EXTERNAL_HEAP)
    if (heap_num == HEAP_EXT)
    {
        return ext_malloc_enabled();
    }
#endif

#if defined(DM_BANKS_CAN_BE_POWERED_OFF)
    if (heap_num == HEAP_EXTRA)
    {
        return pheap_info->heap[HEAP_EXTRA].heap_guard != NULL;
    }
#endif

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    if (heap_num == HEAP_NVRAM)
    {
        /* The area set aside to be accessed through the NVRAM
           is not used by the data heap so pretend that it is
           always powered off. */
        return FALSE;
    }
#endif

    return TRUE;
}
#else
#define heap_is_powered_on(x) TRUE
#endif

/**
 * \brief Find the mid point for the given heap
 */
static char *heap_midpoint(const heap_info *pinfo)
{
    return pinfo->heap_start + pinfo->heap_size / 2;
}

/**
 * \brief Get the size of the heap node
 */
static inline unsigned get_heap_size_node(char *heap)
{
   return (heap != NULL) ? (((mem_node *) heap)->length + sizeof(mem_node)) : 0;
}

/**
 * \brief Internal call to allocate memory.
 */
static void *heap_alloc_internal(unsigned size,
                                 heap_names heap_num,
                                 bool top_down)
{
    mem_node **pbest;
    mem_node **pnode;
    heap_info *pheap;
    unsigned bestsize;
    char *addr;
    mem_node *newnode;
    PROC_ID_NUM proc_id;
    heap_config *config;

    patch_fn_shared(heap);

    pbest = NULL;
    /* TODO: The condition should probably be inverted everywhere else. */
    top_down = !top_down;

    /* Round up size to the nearest whole word */
    size = ROUND_UP_TO_WHOLE_WORDS(size) + GUARD_SIZE;

    /* Get the variables needed for the allocation. */
    proc_id = get_proc_id(heap_num);

    /* Lock the interrupts or get the semaphore (getting the semaphore locks
     * the interrupts as well). */
    acquire_lock(heap_num);
    config = &processor_heap_info_list[proc_id];
    pnode = &config->freelist[heap_num];
    pheap = &config->heap[heap_num];
    bestsize = pheap->heap_size;

    /* Traverse the list looking for the best-fit free block
     * Best fit is the smallest one of at least the requested size
     * This will help to minimise wastage
     */

    while (*pnode != NULL)
    {
        unsigned nodesize = (*pnode)->length;
        char *midpoint = heap_midpoint(pheap);

        if (top_down)
        {
            if ((char *)(*pnode) + nodesize < midpoint)
            {
                nodesize = pheap->heap_size - 1;
            }
        }
        else
        {
            if ((char *)(*pnode) >= midpoint)
            {
                nodesize = pheap->heap_size - 1;
            }
        }
        if (((*pnode)->length >= size ) && (nodesize < bestsize))
        {
            pbest = pnode;
            bestsize = nodesize;
        }
        pnode = &(*pnode)->u.next;
    }

    if (pbest == NULL)
    {
        release_lock(heap_num);

        return NULL;
    }

    addr = (char *)(*pbest);
    if ((*pbest)->length >= size + sizeof(mem_node) + MIN_SPARE)
    {
        /* There's enough space to allocate something else
         * so keep the existing free block and allocate the space at the top
         * In this case the allocation size is exactly what was requested
         */
        if (top_down)
        {
            addr += (*pbest)->length - size;
        }
        else
        {
            mem_node prev_node = **pbest;
            *pbest = (mem_node *)(addr + size + sizeof(mem_node));
            **pbest = prev_node;
        }
        (*pbest)->length -= (size + sizeof(mem_node));
    }
    else
    {
        /* Not enough extra space to be useful
         * Replace the free block with an allocated one
         * The allocation size is the whole free block
         */
        size = (*pbest)->length;
        *pbest = (*pbest)->u.next;

        /* This node gets reused. To simplify the logic, temporarily add its
           size to the available space. It gets taken off again below. */
        config->heap_debug_free += sizeof(mem_node) + GUARD_SIZE;
        pheap->heap_free += sizeof(mem_node) + GUARD_SIZE;
    }

    /* Finally populate the header for the newly-allocated block */
    newnode = (mem_node *)addr;
    newnode->length = size - GUARD_SIZE;
    newnode->u.magic = MAGIC_WORD_WITH_OWNER();

    unsigned heap_debug_free;
    unsigned heap_debug_min_free;

    config->heap_debug_free -= (size + sizeof(mem_node));

    heap_debug_free = config->heap_debug_free;
    heap_debug_min_free = config->heap_debug_min_free;

    if (heap_debug_min_free > heap_debug_free)
    {
        config->heap_debug_min_free = heap_debug_free;
    }

    pheap->heap_free -= (size + sizeof(mem_node));

    /* Release the lock. */
    release_lock(heap_num);

    return addr + sizeof(mem_node);
}

/**
 * \brief Allocate a block of heap trying to use the cheapest
 *        internal memory first.
 *
 * \param size     Number of octets requested.
 * \param pref_dm1 Prefer memory from DM1.
 *
 * \return An accessible piece of heap or NULL.
 */
static void *heap_alloc_cheapest(unsigned size, bool pref_dm1)
{
    void *addr = NULL;

#if defined(CHIP_HAS_SLOW_DM_RAM)
    addr = heap_alloc_internal(size, HEAP_SLOW, pref_dm1);
#endif
    if (addr == NULL)
    {
        addr = heap_alloc_internal(size, HEAP_MAIN, pref_dm1);
        if (addr == NULL)
        {
            /* Failed to allocate memory. Use the shared heap as
               the last fallback. */
            addr = heap_alloc_internal(size, HEAP_SHARED, pref_dm1);
        }
    }

    if ((addr != NULL) && (!pref_dm1))
    {
        addr = convert_to_dm2(addr);
    }

    return addr;
}

#if defined(CHIP_HAS_SLOW_DM_RAM)
/**
 * \brief Allocate a block of heap trying to use the fastest
 *        internal memory first.
 *
 * \param size     Number of octets requested.
 * \param pref_dm1 Prefer memory from DM1.
 *
 * \return An accessible piece of heap or NULL.
 */
static void *heap_alloc_fastest(unsigned size, bool pref_dm1)
{
    void *addr = NULL;

    addr = heap_alloc_internal(size, HEAP_MAIN, pref_dm1);
    if (addr == NULL)
    {
        addr = heap_alloc_internal(size, HEAP_SLOW, pref_dm1);
        if (addr == NULL)
        {
            /* Failed to allocate memory. Use the shared heap as
               the last fallback. */
            addr = heap_alloc_internal(size, HEAP_SHARED, pref_dm1);
        }
    }

    if ((addr != NULL) && (!pref_dm1))
    {
        addr = convert_to_dm2(addr);
    }
    return addr;
}
#endif /* defined(CHIP_HAS_SLOW_DM_RAM) */

#if defined(DM_BANKS_CAN_BE_POWERED_OFF)
static void *heap_alloc_extra(unsigned size, bool pref_dm1)
{
    patch_fn_shared(heap);

    if (!heap_is_powered_on(HEAP_EXTRA))
    {
        PROC_ID_NUM proc_id;
        heap_info *heap;

        heap = &pheap_info->heap[HEAP_EXTRA];
        if (heap->heap_size == 0)
        {
            return NULL;
        }

        proc_id = proc_get_processor_id();
        heap_processor_change(proc_id, HEAP_EXTRA, TRUE);

        heap->heap_free = 0;
        memset(heap->heap_start, 0, heap->heap_size);
        pheap_info->freelist[HEAP_EXTRA] = init_heap_node(heap->heap_start,
                                                          heap->heap_size);
        heap_update_guard(HEAP_EXTRA);
    }

    return heap_alloc_internal(size, HEAP_EXTRA, pref_dm1);
}
#endif /* defined(DM_BANKS_CAN_BE_POWERED_OFF) */

/**
 * \brief Claim back the free memory.
 */
static void coalesce_free_mem(heap_names heap_num,
                              mem_node *free_mem,
                              unsigned len)
{
    patch_fn_shared(heap);

    mem_node **pfreelist;
    mem_node *curnode, **pnode;
    heap_info *info;
    PROC_ID_NUM proc_id;
    heap_config *config;

    proc_id = get_proc_id(heap_num);

    /* Lock the interrupts or get the semaphore (getting the semaphore locks
     * the interrupts as well). */
    acquire_lock(heap_num);
    config = &processor_heap_info_list[proc_id];
    /* Get the information of the heap the node belongs to. */
    info = &config->heap[heap_num];
    pfreelist = &config->freelist[heap_num];
    curnode = *pfreelist;

    /* Traverse the free list to see if we can coalesce an
       existing free block with this one. */
    while (curnode != NULL)
    {
        if ((char *) curnode + curnode->length +
            sizeof(mem_node) == (char *)free_mem)
        {
            /* Matching block found. */
            break;
        }
        curnode = curnode->u.next;
    }

    if (curnode != NULL)
    {
        /* The immediately-previous block is free
           add the one now being freed to it. */
        curnode->length += len;
        config->heap_debug_free += len;

        /* update free heap */
        info->heap_free += len;
    }
    else
    {
        /* Previous block wasn't free so add the now-free block to the free
           list. Note the length is unchanged from when it was allocated (unless
           we have guard words, which get added to the free space). */
        curnode = init_heap_node((char *)free_mem, len);
        curnode->u.next = *pfreelist;
        *pfreelist = curnode;
    }

    /* Now check if there is a free block immediately after the found / new one */
    pnode = pfreelist;
    while (*pnode != NULL)
    {
        if ((char*)(*pnode) == (char*)curnode + curnode->length + sizeof(mem_node))
        {
            /* Matching block found. */
            break;
        }
        pnode = &(*pnode)->u.next;
    }
    if (*pnode != NULL)
    {
        /* The immediately-following block is free, add it to the current one
           and remove from the free list. */
        curnode->length += (*pnode)->length + sizeof(mem_node);
        *pnode = (*pnode)->u.next;
        config->heap_debug_free += sizeof(mem_node) + GUARD_SIZE;

        /* update free heap */
        info->heap_free += sizeof(mem_node) + GUARD_SIZE;
    }

#if defined(DM_BANKS_CAN_BE_POWERED_OFF)
    if (heap_num == HEAP_EXTRA)
    {
        unsigned max_size;

        max_size = info->heap_size;
        max_size -= HEAP_GUARD_AREA_SIZE;
        max_size -= sizeof(mem_node);
        if (info->heap_free == max_size)
        {
            heap_processor_change(proc_id, HEAP_EXTRA, FALSE);
            config->heap_debug_free -= max_size;
        }
    }
#endif

    release_lock(heap_num);
}

/**
 * \brief Update the freelist to account for the end pointer moving.
 *
 * In normal applications, the highest pre-existing memory node needs
 * to be resized. However, if the secondary processor was disabled
 * late, this function will fail.
 *
 * \param heap      Name of the heap to be updated.
 * \param curr_end  Pointer to the current end of the heap.
 * \param new_end   Pointer to the new end of the heap.
 *
 * \note In practice, this function is only called on the primary processor.
 */
static void update_freelist_with_new_end(heap_names heap,
                                         char *curr_end,
                                         char *new_end)
{
    mem_node **pfreelist;
    bool found;
    PROC_ID_NUM proc_id;

    patch_fn_shared(heap);

    proc_id = get_proc_id(heap);

    pfreelist = &processor_heap_info_list[proc_id].freelist[heap];
    /* Do all the list-traversal and update with interrupts blocked. */
    LOCK_INTERRUPTS;

    /* Traverse the free list to find existing free block
       with the current stop address. */
    found = FALSE;
    while (*pfreelist != NULL)
    {
        mem_node *curr_node = *pfreelist;
        char *node_end;

        node_end = (char *) curr_node;
        node_end += sizeof(mem_node);
        node_end += curr_node->length;

        if (node_end == curr_end)
        {
            if (curr_end <= new_end)
            {
                /* The secondary core is being disabled via
                   a command from application. */
                curr_node->length += (new_end - curr_end);
            }
            else
            {
                /* The heap is being shrunk to make space for
                   the secondary core. */
                curr_node->length -= (curr_end - new_end);
            }
            found = TRUE;
            break;
        }
        pfreelist = &(curr_node->u.next);
    }

    /* Unlock interrupt and exit. */
    UNLOCK_INTERRUPTS;

    PL_ASSERT(found);
}

/**
 * \brief Allocate a block of memory at the end of a heap.
 *
 * \param heap_num The heap to put a guard in.
 *
 * \note See definition of HEAP_GUARD_AREA_SIZE for more information.
 */
static void heap_update_guard(heap_names heap_num)
{
    mem_node *pbest;
    mem_node *pnode;
    mem_node *newnode;
    char *heap_guard;
    char *addr;
    unsigned *heap_debug_free;
    unsigned *heap_debug_min_free;

    patch_fn_shared(heap);

    if (heap_num == HEAP_SHARED)
    {
        return;
    }

    LOCK_INTERRUPTS;

    /* Find the memory node closer to the end of memory. */
    pbest = NULL;
    pnode = pheap_info->freelist[heap_num];

    while (pnode != NULL)
    {
        if (pnode > pbest)
        {
            pbest = pnode;
        }
        pnode = pnode->u.next;
    }

    /* The guard node should occupy HEAP_GUARD_AREA_SIZE octets
       including overhead so shrink the last node by that amount. */
    pbest->length -= HEAP_GUARD_AREA_SIZE;

    /* Create a new node at the end of the memory. */
    addr = (char *) pbest;
    addr += pbest->length + sizeof(mem_node);
    heap_guard = addr + sizeof(mem_node);
    newnode = (mem_node *) addr;
    newnode->length = HEAP_GUARD_AREA_SIZE;
    newnode->length -= sizeof(mem_node) + GUARD_SIZE;
    newnode->u.magic = MAGIC_WORD_WITH_OWNER();
#if defined(PMALLOC_DEBUG)
    newnode->file = __FILE__;
    newnode->line = __LINE__;
    newnode->guard = HEAP_GUARD_WORD;
    *((unsigned int *)(heap_guard + newnode->length)) = HEAP_GUARD_WORD;
#endif /* defined(PMALLOC_DEBUG) */

    /* Update the global heap statistics. */
    heap_debug_free = &pheap_info->heap_debug_free;
    heap_debug_min_free = &pheap_info->heap_debug_min_free;
    pheap_info->heap[heap_num].heap_free -= HEAP_GUARD_AREA_SIZE;
    pheap_info->heap[heap_num].heap_guard = heap_guard;
    *heap_debug_free -= HEAP_GUARD_AREA_SIZE;
    if (*heap_debug_min_free > *heap_debug_free)
    {
        *heap_debug_min_free = *heap_debug_free;
    }

    UNLOCK_INTERRUPTS;
}


/**
 * \brief Allocate memory from heap based on preference
 *
 * \param size : Size to be allocated
 * \param preference : Malloc preference
 *
 * \return pointer to the allocated memory
 */
static void *heap_alloc_on_preference(unsigned size, unsigned preference)
{

    /* pref_dm1: If TRUE: Memory allocation mechanism tries to allocate
     * evenly between DM1/DM2 for MALLOC_PREFERENCE_NONE.
     *
     * There is no need for a separate fallback option. If you ask for DM1,
     * you will get DM1. If you ask for DM2, you will get DM2 address. All
     * requests will be honoured from heap/slow_heap/shared_heap - the order
     * depending on specific MALLOC_PREFERENCE - until we run out of heap.
     * Exception is if request is explicitly for shared and external memory,
     * which can be denied for lack of free blocks in the requested heaps.
     */
    bool pref_dm1 = TRUE;
    void *addr = NULL;
    MALLOC_STRICTNESS strictness;

    patch_fn_shared(heap);

    strictness = heap_rules.relaxmallocstrictness;
    if (strictness == MALLOC_STRICTNESS_DM1_FALLBACK)
    {
        pref_dm1 = TRUE;
    }
    else if (strictness == MALLOC_STRICTNESS_DM2_FALLBACK)
    {
        pref_dm1 = FALSE;
    }
    else /* MALLOC_STRICTNESS_EVEN_FALLBACK */
    {
        heap_names heap;

        heap = HEAP_INVALID;
#if defined(CHIP_HAS_SLOW_DM_RAM)
        if (preference == MALLOC_PREFERENCE_NONE)
        {
            heap = HEAP_SLOW;
        }
        else if (preference == MALLOC_PREFERENCE_FAST)
        {
            heap = HEAP_MAIN;
        }
#else
        if (preference == MALLOC_PREFERENCE_NONE)
        {
            heap = HEAP_MAIN;
        }
#endif /* defined(CHIP_HAS_SLOW_DM_RAM) */
#if defined(DM_BANKS_CAN_BE_POWERED_OFF)
        else if (preference == MALLOC_PREFERENCE_EXTRA)
        {
            heap = HEAP_EXTRA;
        }
#endif

        if (heap != HEAP_INVALID)
        {
            pref_dm1 = pheap_info->pref_dm1[heap];
            pheap_info->pref_dm1[heap] = !pheap_info->pref_dm1[heap];
        }
    }

    /* If last time DM1 alloc failed prefer doing DM2 first.
       Note that we specifically DON'T want to try to find the
       'best-fit' block across all heaps - in a mostly-unallocated
       state that would tend just fill up the one with less free space. */
    switch (preference)
    {
        case MALLOC_PREFERENCE_DM1:
        {
            addr = heap_alloc_cheapest(size, TRUE);
            break;
        }
        case MALLOC_PREFERENCE_DM2:
        {
            addr = heap_alloc_cheapest(size, FALSE);
            break;
        }
#if defined(DM_BANKS_CAN_BE_POWERED_OFF)
        case MALLOC_PREFERENCE_EXTRA:
        {
            addr = heap_alloc_extra(size, pref_dm1);
            break;
        }
        case MALLOC_PREFERENCE_EXTRA_DM1:
        {
            addr = heap_alloc_extra(size, TRUE);
            break;
        }
        case MALLOC_PREFERENCE_EXTRA_DM2:
        {
            addr = heap_alloc_extra(size, FALSE);
            break;
        }
#endif /* defined(DM_BANKS_CAN_BE_POWERED_OFF) */
#if defined(CHIP_HAS_SLOW_DM_RAM)
        case MALLOC_PREFERENCE_FAST:
        {
            addr = heap_alloc_fastest(size, pref_dm1);
            break;
        }
#endif /* defined(CHIP_HAS_SLOW_DM_RAM) */
        case MALLOC_PREFERENCE_SHARED:
        {
            /* For shared heap, we could choose DM1 or DM2 address. For now,
               allocate as if DM1, because internally that is quicker. */
            addr = heap_alloc_internal(size, HEAP_SHARED, TRUE);
            break;
        }
#if defined(HAVE_EXTERNAL_HEAP)
        case MALLOC_PREFERENCE_EXTERNAL:
        {
            if (!ext_malloc_enabled())
            {
                /* The external memory is not powered on so it is not
                   possible to allocate memory. */
                addr = NULL;
            }
            else if (!ext_heap_is_disabled())
            {
                /* The external heap is enabled by MIBS and there is at
                   least one node of memory available. */
                addr = heap_alloc_internal(size, HEAP_EXT, TRUE);
            }
            break;
        }
#endif /* defined(HAVE_EXTERNAL_HEAP) */
        case MALLOC_PREFERENCE_NONE:
        default:
        {
            addr = heap_alloc_cheapest(size, pref_dm1);
            break;
        }
    }

    return addr;
}

/**
 * \brief Check whether shared memory ranges are set
 *
 * \param size : Size to be allocated
 *
 * \return bool TRUE if shared memory can be used
 */
static bool get_shared_mem_preference(unsigned size)
{
    shared_mem_preferences *pref;

    pref = &heap_rules.pref_shared_mem_ranges;

    return (((size >= pref->low_start)  && (size <= pref->low_end)) ||
            ((size >= pref->mid_start)  && (size <= pref->mid_end)) ||
            ((size >= pref->high_start) && (size <= pref->high_end)));
}

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Returns which heap the pointer points to.
 *
 * \note  Panics if memory is not in the heap.
 */
heap_names get_heap_num(void *ptr)
{
    heap_info *heap_cfg = pheap_info->heap;
    heap_names heap_num;

    patch_fn_shared(heap);

    for (heap_num = 0; heap_num < HEAP_ARRAY_SIZE; heap_num++)
    {
        if ((heap_cfg[heap_num].heap_start <= (char*) ptr) &&
            ((char*) ptr < heap_cfg[heap_num].heap_end))
        {
            return heap_num;
        }
    }
    PL_PRINT_P0(TR_PL_FREE, "Couldn't find anywhere\n");
    panic_diatribe(PANIC_AUDIO_FREE_INVALID, (DIATRIBE_TYPE)((uintptr_t)ptr));
#ifdef USE_DUMMY_PANIC
    /* panic_diatribe does not return in standard builds, but it does for
     * some test builds where this is defined. */
    return HEAP_INVALID;
#endif
}

/**
 * \brief Initialise the memory heap
 */
void init_heap(void)
{
    PROC_ID_NUM proc_id;
    heap_names heap_num;

    /* Initialise the heap config. */
    proc_id = proc_get_processor_id();
    pheap_info = &processor_heap_info_list[proc_id];

    /* The primary processor does more work. */
    if (proc_id == PROC_PROCESSOR_0)
    {
        /* Ensure all memory is allocated at the bottom of the main heap
           until the product configuration is available. */
        heap_rules.relaxmallocstrictness = MALLOC_STRICTNESS_DM1_FALLBACK;

        /* Configure the heaps based on build time information. */
        heap_config_static(processor_heap_info_list);
    }

    /* A potential place to fix the config. */
    patch_fn_shared(heap);

    /* pheap_info is processor specific static global and it now points to the
       heap_config specific to the current processor. */
    for (heap_num = 0; heap_num < HEAP_INTERNAL_SIZE; heap_num++)
    {
        heap_info *heap;

#if defined(COMMON_SHARED_HEAP)
        if ((heap_num == HEAP_SHARED) && (proc_id != PROC_PROCESSOR_0))
        {
            /* The primary processor already created a memory node for
               the shared heap when it is common between processors. */
            continue;
        }
#endif
#if defined(DM_BANKS_CAN_BE_POWERED_OFF)
        if ((heap_num == HEAP_EXTRA) && (proc_id != PROC_PROCESSOR_0))
        {
            /* The extra heap starts powered off. */
            continue;
        }
#endif
#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
        if ((heap_num == HEAP_NVRAM) && (proc_id != PROC_PROCESSOR_0))
        {
            /* The area reserved for the NVRAM window starts powered off. */
            continue;
        }
#endif

        heap = &pheap_info->heap[heap_num];
        heap->heap_free = 0;
        if (heap->heap_size != 0)
        {
            memset(heap->heap_start, 0, heap->heap_size);
            pheap_info->freelist[heap_num] = init_heap_node(heap->heap_start,
                                                            heap->heap_size);
            if (proc_id != PROC_PROCESSOR_0)
            {
                heap_update_guard(heap_num);
            }
        }
    }

    pheap_info->heap_debug_min_free = pheap_info->heap_debug_free;

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    /* For chips with NVRAM_ACCESS_TO_DM, unconditionally map DM in the
     * designated NVMEM window. Do it here so that it is applicable on
     * both P0 and P1 */
    hal_window_open(HAL_WINDOWS_DATA_MEMORY);
#endif

}

#if defined(HAVE_EXTERNAL_HEAP)
/**
 * \brief Enable the external heap.
 */
bool ext_heap_enable(bool enable)
{
    char *heap;
    unsigned heapsize;

    patch_fn_shared(heap);

    if (!enable || !ext_malloc_enabled())
    {
        PL_PRINT_P0(TR_PL_MALLOC, "Disabling SRAM heap\n");
        /* Disable the heap. */
        pheap_info->freelist[HEAP_EXT] = NULL;
        pheap_info->heap[HEAP_EXT].heap_guard = NULL;
        return TRUE;
    }

    heap = pheap_info->heap[HEAP_EXT].heap_start;
    heapsize = pheap_info->heap[HEAP_EXT].heap_size;

    PL_PRINT_P0(TR_PL_MALLOC, "Enabling SRAM heap\n");

    if (ext_heap_is_disabled() && (heapsize > 0))
    {
        /* Initialise the freelist for P0. */
        pheap_info->freelist[HEAP_EXT] = init_heap_node(heap, heapsize);
        /* Ensure the secondary core does not read outside of its window. */
        heap_update_guard(HEAP_EXT);
    }

    /* Validate the external memory initialisation. */
    if (ext_heap_is_disabled() ||
        get_heap_size_node(heap) != heapsize - HEAP_GUARD_AREA_SIZE)
    {
        if (!ext_heap_is_disabled())
        {
            /* The heap is not initialised properly, complain loudly. */
            PL_PRINT_P0(TR_PL_MALLOC, "External heap is invalid\n");
            fault_diatribe(FAULT_AUDIO_MEMORY_ACCESS_FAILED, (uintptr_t) heap);
        }

        PL_PRINT_P0(TR_PL_MALLOC, "SRAM is still disabled\n");
        return FALSE;
    }

    return TRUE;
}
#else
#define ext_heap_enable(x) FALSE
#endif /* defined(HAVE_EXTERNAL_HEAP) */

/**
 * \brief Configure the memory heap.
 *
 * \note  This is called after booting up the subsystem to claim the remaining
 *        memory area in the memory map as heap. It also reads the MIB key to
 *        decide how much memory to be allocated to each processors. Currently
 *        only dual core is handled.
 */
void config_heap(void)
{
    heap_info *original_info;
    heap_names heap_num;

    patch_fn_shared(heap);

    heap_get_allocation_rules(&heap_rules);

    /* Take note of the current heap size set for P0. */
    original_info = heap_alloc_boot(HEAP_ARRAY_SIZE * sizeof(heap_info));
    memcpy(original_info, pheap_info->heap, HEAP_ARRAY_SIZE * sizeof(heap_info));

    /* Update the heaps boundaries based on the product configuration. */
    heap_config_dynamic(processor_heap_info_list);

    /* When a secondary core is permanently disabled, its memory is transfered
       to the primary core. Ensure that the primary core will be able to
       access it. */
    if (proc_single_core_present())
    {
        heap_config_hardware(processor_heap_info_list);
    }

    /* Check all the heaps to see if their last free node need resizing
       or if a new node needs to be created. */
    for (heap_num = 0; heap_num < HEAP_ARRAY_SIZE; heap_num++)
    {
        heap_info *before;
        heap_info *after;
        int diff;

        if (!heap_is_powered_on(heap_num))
        {
            /* It is not possible to resize a heap if it is not powered on. */
            continue;
        }

        before = &original_info[heap_num];
        after = &pheap_info->heap[heap_num];

        /* After configuration, the primary processor's heap might have
           been resized. Start with freeing the heap_guard. heap_free will
           cope if heap_guard was not previously allocated.
           Freeing the guard ensures that there will be a free node
           available that can be resized to fill newly acquired memory. */
        heap_free(after->heap_guard);

        diff = after->heap_size - before->heap_size;
        if (diff != 0)
        {
            /* The last node in the heap's free list needs to be either
               enlarged if diff > 0 or shrinked otherwise. */
            update_freelist_with_new_end(heap_num,
                                         before->heap_end,
                                         after->heap_end);

            /* Adjust the statistics. */
            pheap_info->heap_debug_min_free += diff;
            pheap_info->heap_debug_free += diff;
            after->heap_free += diff;
        }

        /* And allocate a fresh guard node at the end of the heap. */
        heap_update_guard(heap_num);
    }

    /* After the per product settings have been taken into account, the primary core
       gives away some of its memory. Ensure that it will not be able to access it
       accidentaly. */
    if (proc_multiple_cores_present())
    {
        heap_config_hardware(processor_heap_info_list);
    }

    /* Get rid of temporary memory. */
    pfree(original_info);
}

/**
 *  \brief Check whether the pointer is in heap or not.
 */
bool is_addr_in_heap(void *addr)
{
    unsigned heap_num;
    heap_info *heap_cfg = pheap_info->heap;

    for (heap_num = 0; heap_num < HEAP_ARRAY_SIZE; heap_num++)
    {
        if ((heap_cfg[heap_num].heap_start <= (char*)addr) &&
            ((char*)addr < heap_cfg[heap_num].heap_end))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * \brief Allocate a block of (at least) the requested size.
 */
#ifdef PMALLOC_DEBUG
void *heap_alloc_debug(unsigned size, unsigned preference,
                       const char *file, unsigned int line)
#else
void *heap_alloc(unsigned size, unsigned preference)
#endif
{
    void *addr = NULL;

    /* Don't do anything if zero size requested. */
    if (size == 0)
    {
        return NULL;
    }

#if !defined(CHIP_HAS_SLOW_DM_RAM)
    /* If chip has no slow RAM, request for FAST is treated as
     * No preference
     */
    if (preference == MALLOC_PREFERENCE_FAST)
    {
        preference = MALLOC_PREFERENCE_NONE;
    }
#endif

   /* Allocate from the shared heap if requested */
   if ((preference == MALLOC_PREFERENCE_NONE) &&
       (get_shared_mem_preference(size) == TRUE))
   {
       addr = heap_alloc_internal(size, HEAP_SHARED, TRUE);
   }

   if (addr == NULL)
   {
      addr = heap_alloc_on_preference(size, preference);
   }

#ifdef PMALLOC_DEBUG
    if (addr != NULL)
    {
        /* Record where this block was allocated from */
        mem_node *node = (mem_node *)((char *)addr - sizeof(mem_node));

        node->file = file;
        node->line = line;
        node->guard = HEAP_GUARD_WORD;
        *((unsigned int *)((char *)addr + node->length)) = HEAP_GUARD_WORD;
    }
#endif

    if (addr == NULL)
    {
        PL_PRINT_P1(TR_PL_MALLOC, "Failed to allocate %u octets.\n", size);
        fault_diatribe(FAULT_AUDIO_INSUFFICIENT_MEMORY, size);
    }
    else
    {
        PL_PRINT_P3(TR_PL_MALLOC, "Allocated %u octets "
                                  "at address %p inside heap %u.\n",
                                  size, addr, get_heap_num(addr));
    }

    patch_fn_shared(heap);

    return addr;
}

/**
 * \brief Free a previously-allocated block.
 */
void heap_free(void *ptr)
{
    mem_node *node;
    heap_names heap_num;

    if (ptr == NULL)
    {
        /* free(NULL) is a no-op  */
        return;
    }

    patch_fn_shared(heap);

    PL_PRINT_P1(TR_PL_FREE, "Pointer to be freed (%p) ", ptr);
    /* Move DM2 addresses back into DM1 address range. */
    ptr = convert_to_dm1(ptr);

    heap_num = get_heap_num(ptr);
#ifdef USE_DUMMY_PANIC
    if (heap_num == HEAP_INVALID)
    {
        /* get_heap_num does not return when passed an invalid pointer
           in standard builds, but it does when USE_DUMMY_PANIC is defined. */
        return;
    }
#endif /* USE_DUMMY_PANIC */
#if defined(HAVE_EXTERNAL_HEAP)
    if ((heap_num == HEAP_EXT) && !ext_malloc_enabled())
    {
        /* Freed after disabling heap. Ignore. */
        PL_PRINT_P1(TR_PL_FREE, "Ignored attempt to free pointer (%p) "
                    "to disabled external memory.", ptr);
        return;
    }
#endif /* defined(HAVE_EXTERNAL_HEAP) */
    PL_PRINT_P1(TR_PL_FREE, "is in heap %u.\n", heap_num);

    /* Check that the address being freed looks sensible. */
    node = (mem_node *)((char *)ptr - sizeof(mem_node));
    if (IS_NOT_MAGIC_WORD(node->u.magic))
    {
        panic_diatribe(PANIC_AUDIO_FREE_INVALID,
                       (DIATRIBE_TYPE)((uintptr_t)ptr));
    }

    /* Check that the length seems plausible. Function will panic with
       PANIC_AUDIO_FREE_INVALID if memory is not in the heap. */
    get_heap_num((char *)ptr + node->length - 1);

#ifdef PMALLOC_DEBUG
    if (node->file == NULL)
    {
        panic_diatribe(PANIC_AUDIO_FREE_INVALID,
                       (DIATRIBE_TYPE)((uintptr_t)node));
    }
    if (node->guard != HEAP_GUARD_WORD)
    {
        panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION,
                       (DIATRIBE_TYPE)((uintptr_t)node));
    }
    if (*((unsigned int *)((char *)ptr + node->length)) != HEAP_GUARD_WORD)
    {
        panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION,
                       (DIATRIBE_TYPE)((uintptr_t)node));
    }
    node->file = NULL;
    node->line = 0;
    node->guard = 0;
#endif
    node->u.magic = 0;

    /* Coalesce the freed block. */
    coalesce_free_mem(heap_num,
                      node,
                      node->length + sizeof(mem_node) + GUARD_SIZE);
}

/**
 * \brief Get the size of a previously-allocated block.
 */
unsigned heap_sizeof(void *ptr)
{
    mem_node *node = (mem_node *)((char *)ptr - sizeof(mem_node));

    if (ptr == NULL)
    {
        return 0;
    }

    /* Check that the address looks sensible. */
    if (IS_NOT_MAGIC_WORD(node->u.magic))
    {
        /* Might want a (debug-only?) panic here. */
        return 0;
    }

    return node->length;
}

/**
 * \brief Heap size in words.
 */
unsigned heap_size(void)
{
    unsigned i, size = 0;

    for (i = 0; i < HEAP_ARRAY_SIZE; i++)
    {
#if defined(COMMON_SHARED_HEAP)
        if (!(PROC_SECONDARY_CONTEXT() && i == HEAP_SHARED))
#endif /* COMMON_SHARED_HEAP */
        {
            if (heap_is_powered_on((heap_names)i))
            {
                size += pheap_info->heap[i].heap_size;
            }
        }
    }

    return (size >> LOG2_ADDR_PER_WORD);
}

#ifdef PMALLOC_DEBUG
/**
 * \brief Check that a previously-allocated block looks sensible.
 */
void heap_validate(void *ptr)
{
    mem_node *node;

    if (ptr == NULL)
    {
        /* Shouldn't happen, but don't bother checking if NULL */
        return;
    }

    /* Move DM1 addresses back into DM2 address range */
    ptr = convert_to_dm1(ptr);

    /* Check that the address looks sensible */
    node = (mem_node *)((char *)ptr - sizeof(mem_node));
    if (IS_NOT_MAGIC_WORD(node->u.magic))
    {
        panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION,
                       (DIATRIBE_TYPE)((uintptr_t)ptr));
    }

    /* Check that the length seems plausible. get_heap_num panics if
       the pointer is not in any heap memory. */
    get_heap_num((char *)ptr + node->length - 1);
}
#endif /* PMALLOC_DEBUG */

mem_node *heap_get_freelist(heap_names heap)
{
    PROC_ID_NUM proc_id;

    proc_id = get_proc_id(heap);

    return processor_heap_info_list[proc_id].freelist[heap];
}

#ifdef INSTALL_DM_MEMORY_PROFILING
/**
 * \brief Function to 'tag' a block of allocated heap memory
 *
 * \param[in] ptr  Pointer to the allocated block to be tagged
 * \param[in] id   The owner id to tag the allocated block with
 *
 * \return Returns TRUE if  tagging succeeded,
 *                 FALSE if tagging failed (ptr is not a heap block).
 */
bool heap_tag_dm_memory(void *ptr, DMPROFILING_OWNER id)
{
    mem_node *node;

    node = (mem_node *)((unsigned char *)ptr - sizeof(mem_node));
    if (IS_NOT_MAGIC_WORD(node->u.magic))
    {
        return FALSE;
    }
    node->u.magic = MAGIC_WORD_WITH_ID(id);
    return TRUE;
}
#endif /* INSTALL_DM_MEMORY_PROFILING */

/**
 * Currently available heap memory.
 */
unsigned heap_cur(void)
{
    unsigned heap_debug_free;

    heap_debug_free = pheap_info->heap_debug_free;
    return (heap_debug_free >> LOG2_ADDR_PER_WORD);
}

/**
 * Minimum available heap. The minimum available memory is also called
 * memory watermarks.
 */
unsigned heap_min(void)
{
    unsigned heap_debug_min_free;

    heap_debug_min_free = pheap_info->heap_debug_min_free;
    return (heap_debug_min_free >> LOG2_ADDR_PER_WORD);
}

/**
 * Clear the memory watermarks.
 */
void heap_clear_watermarks(void)
{
    LOCK_INTERRUPTS;
    pheap_info->heap_debug_min_free = pheap_info->heap_debug_free;
    UNLOCK_INTERRUPTS;
}

void *heap_alloc_boot(unsigned size)
{
    void *result;

    /* Do not fail by construction. */
    result = heap_alloc_internal(size, HEAP_MAIN, TRUE);
    memset(result, 0, size);
#ifdef PMALLOC_DEBUG
    /* Record where this block was allocated from */
    mem_node *node = (mem_node *)((char *)result - sizeof(mem_node));

    /* We don't know where this was actually called from.
     * So just record the current file and line
     */
    node->file = __FILE__;
    node->line = __LINE__;
    node->guard = HEAP_GUARD_WORD;
    *((unsigned int *)((char *)result + node->length)) = HEAP_GUARD_WORD;
#endif
    return result;
}

#if defined(SUPPORTS_MULTI_CORE) && \
    defined(DM_BANKS_CAN_BE_POWERED_OFF)
void heap_processor_change(PROC_ID_NUM proc_id,
                           heap_names heap_num,
                           bool enable)
{
    heap_config *config;
    heap_info *info;

    patch_fn_shared(heap);

    config = &processor_heap_info_list[proc_id];
    info = &config->heap[heap_num];
    heap_change_power(proc_id, info, enable);
    if (!enable)
    {
        info->heap_free = 0;
        info->heap_guard = NULL;
        config->freelist[heap_num] = NULL;

        /* P1 is being stopped - clear its heap_debug_free and heap_debug_min_free */
        if ((proc_id == PROC_PROCESSOR_1) && (heap_num == HEAP_MAIN))
        {
            config->heap_debug_free = 0;
            config->heap_debug_min_free = 0;
        }
    }
}
#endif

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
bool heap_acquire_dm_window(DM_WINDOW *dm_window)
{
    heap_config *config;
    heap_info *info;
    PROC_ID_NUM proc_id;

    patch_fn_shared(heap);

    proc_id = proc_get_processor_id();
    config = &processor_heap_info_list[proc_id];
    info = &config->heap[HEAP_NVRAM];
    if (info->heap_size == 0)
    {
        return FALSE;
    }

    heap_change_power(proc_id, info, TRUE);

    dm_window->start = info->heap_start;
    dm_window->end = info->heap_end;

    return TRUE;
}

void heap_release_dm_window(void)
{
    heap_config *config;
    heap_info *info;
    PROC_ID_NUM proc_id;

    patch_fn_shared(heap);

    proc_id = proc_get_processor_id();
    config = &processor_heap_info_list[proc_id];
    info = &config->heap[HEAP_NVRAM];
    if (info->heap_size == 0)
    {
        return;
    }

    heap_change_power(proc_id, info, FALSE);
}
#endif /* defined(CHIP_HAS_NVRAM_ACCESS_TO_DM) */

#if defined(DM_BANKS_CAN_BE_POWERED_OFF) || \
    defined(INSTALL_EXTERNAL_MEM)
bool pmalloc_preference_is_available_internal(unsigned preference)
{
#if defined(DM_BANKS_CAN_BE_POWERED_OFF)
    if ((preference >= MALLOC_PREFERENCE_EXTRA_DM1) &&
        (preference <= MALLOC_PREFERENCE_EXTRA))
    {
        return (pheap_info->heap[HEAP_EXTRA].heap_size != 0);
    }
#endif

#if defined(HAVE_EXTERNAL_HEAP)
    if (preference == MALLOC_PREFERENCE_EXTERNAL)
    {
        return !ext_heap_is_disabled();
    }
#endif

    return TRUE;
}
#endif /* defined(DM_BANKS_CAN_BE_POWERED_OFF) ... */
