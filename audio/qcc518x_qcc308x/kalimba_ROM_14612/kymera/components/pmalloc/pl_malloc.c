/****************************************************************************
 * Copyright (c) 2009 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/*****************************************************************************
 * \file pl_malloc.c
 * \ingroup pl_malloc
 *
 *
 ****************************************************************************/


/****************************************************************************
Include Files
*/
#ifdef PL_DEBUG_MEM_ON_HOST
#include <stdio.h>
#endif
#include "pl_malloc_private.h"


/****************************************************************************
Private Macro Declarations
*/

/**
 * Macro for the size of the header information.
 * This is a bit ugly - we define the block sizes in "words", but to keep
 * our tests working on 64-bit machines a "word" is actually a uintptr_t
 * This is the same size as a pointer - 24 bits on Kalimba, 32 bits on x86 and
 * 64 bits on 64-bit machines. This ensures that a structure that fits in a
 * given size pool in Kalimba builds will also fit in the same (or smaller)
 * pool in unit test builds.
 *
 * The -1 / +1 in the macro allow for the header size not being an exact
 * multiple of the "word" size.
 */

/**
 * Macro to calculate the number of words needed for a single block in a
 * pool, given the block size in words. Adds space for the block header
 */
#ifdef PMALLOC_DEBUG
/* Add extra guard words at the end of each block to check for corruption */
#define PL_MEM_BLOCK_OVERHEAD_WORDS ((sizeof(tPlMemoryBlockHeader) - 1)/sizeof(uintptr_t) + 2)
#define PMALLOC_DEBUG_GUARD_0 0xFEDCBAul
#define PMALLOC_DEBUG_GUARD_1 0x012345ul
#else
#define PL_MEM_BLOCK_OVERHEAD_WORDS ((sizeof(tPlMemoryBlockHeader) - 1)/sizeof(uintptr_t) + 1)
#endif

/* Total number of pools */
#define NUM_DM_BUSES 2

#define NUM_MEM_POOLS_PER_BUS 3
#define NUM_MEM_POOLS (NUM_DM_BUSES * NUM_MEM_POOLS_PER_BUS)
#define DM2_POOL(pool) (pool + NUM_MEM_POOLS_PER_BUS)

#define NUM_MEM_POOLS_LIMITS_PER_BUS (NUM_MEM_POOLS_PER_BUS + 1)
#define NUM_MEM_POOLS_LIMITS (NUM_DM_BUSES * NUM_MEM_POOLS_LIMITS_PER_BUS)

#define PMALLOC_PRIVATE_MEM_GUARD 0xDEAD

/* Special linker section for pmalloc. */
#ifdef __KCC__
#define DM_MEM_GUARD                _Pragma("datasection DM_MEM_GUARD")
#else
#define DM_MEM_GUARD
#endif

/****************************************************************************
Global Variable Definitions
*/

DM_MEM_GUARD unsigned private_mem_guard = PMALLOC_PRIVATE_MEM_GUARD;

/****************************************************************************
Private Type Declarations
*/

/**
 * tPlMemoryBlockHeader - header at the start of each memory block
 *
 * Contains either a pointer to the next block (for blocks that are free)
 * or the index of the pool which the block came from (for blocks that
 * are allocated)
 *
 * If PMALLOC_DEBUG is defined, the header also contains the file / line
 * of the pmalloc caller, to assist debugging of memory leaks etc.
 * This is conditional as it imposes a significant memory overhead.
 */
typedef struct tPlMemoryBlockHeaderTag
{
    union
    {
        struct tPlMemoryBlockHeaderTag *pNextBlock; /**< For free blocks */
        unsigned int poolIndex; /**< For alloced blocks */
    } u;
#ifdef PMALLOC_DEBUG
    const char *file;
    unsigned int line;
#endif
} tPlMemoryBlockHeader;

/**
 * tPlMemoryPoolControlStruct - Control structure for memory management
 *
 * We have one of these structures for each memory pool.
 */
typedef struct
{
    /**
     * Size (in words) of the blocks in this pool.
     * Excludes the size of the header
     */
    unsigned blockSizeWords;
     /** Number of free blocks in pool */
    uint16 numBlocksFree;
     /**
      * Minimum number of free blocks.
      * This is intended to help with post-mortem debugging (coredumps etc)
      * and hence is always enabled and not protected by PMALLOC_DEBUG
      */
    uint16 minBlocksFree;
    /** Pointer to the first free block in pool */
    tPlMemoryBlockHeader *pFirstFreeBlock;

#ifdef PMALLOC_DEBUG
    uintptr_t *pool_start;
    uintptr_t *pool_end;
#endif
} tPlMemoryPoolControlStruct;

typedef struct
{
    uint16 array[NUM_MEM_POOLS];
} POOL_MIB_VALUE;

/****************************************************************************
Private Variable Definitions
*/

/* For dynamic pools, just pointers for this core */
static uintptr_t *memPoolLimits[NUM_MEM_POOLS_LIMITS];

static uint16 pool_block_counts[NUM_MEM_POOLS];

#if defined(SUPPORTS_MULTI_CORE)
DM_SHARED static uint16 pool_block_counts_p1[NUM_MEM_POOLS];
#else
#define pool_block_counts_p1 pool_block_counts
#endif /* defined(SUPPORTS_MULTI_CORE) */

static uint16 * const pool_block_counts_array[] =
{
    pool_block_counts,
#if defined(SUPPORTS_MULTI_CORE)
    pool_block_counts_p1
#else
    NULL
#endif
};

static tPlMemoryPoolControlStruct aMemoryPoolControl[NUM_MEM_POOLS];

/* At boot all allocations will use heap */
static unsigned heap_threshold = 0;

static pmalloc_cached_report_handler cached_report_handler = NULL;

#ifdef INSTALL_DM_MEMORY_PROFILING
static struct _dmprof_override
{
    bool   use_stored_task_id;
    uint8  stored_taskid;
    taskid service_taskid;
} dmprof_override  = { FALSE, 0u, 0u };
#endif /* INSTALL_DM_MEMORY_PROFILING */

/****************************************************************************
Private Function Definitions
*/

/* When dynamic pools are used, all the pool addresses are also in the heap
 * so the check for calling heap functions is modified to "address not in pools"
 */
static bool is_addr_in_pool(void *addr)
{
    uintptr_t *ptr = (uintptr_t *) addr;
    bool insideDM1;
    bool insideDM2;

    insideDM1 = (ptr >= memPoolLimits[0]) &&
                (ptr < memPoolLimits[NUM_MEM_POOLS_LIMITS_PER_BUS - 1]);
    insideDM2 = (ptr >= memPoolLimits[NUM_MEM_POOLS_LIMITS_PER_BUS]) &&
                (ptr < memPoolLimits[NUM_MEM_POOLS_LIMITS - 1]);
    return insideDM1 || insideDM2;
}

#ifdef PMALLOC_DEBUG
/**
 * \brief debug function to check that a dynamically-allocated block looks
 *        sensible
 * \param[in] pMemory pointer to the block
 * \param[in] id Type of error to report if the pointer was not found.
 * \note If the pointer was found but the associated header was corrupted,
         then PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION is used to panic.
 */
static void pvalidate_internal(void *pMemory, panicid id)
{
    unsigned i;
    unsigned int poolIndex;
    tPlMemoryBlockHeader *pThisBlock;
    uintptr_t *pBlock;
    unsigned int guard;

    if (pMemory == NULL)
    {
        return;
    }

    if (!is_addr_in_pool(pMemory))
    {
        heap_validate(pMemory);
        return;
    }

    /* The pointer does not belong to the heap. If it is valid, then it must
     * be in the pools.
     * Adjust pointer to point to header in this block. Pointer arithmentic
     * means the -1 decrements the pointer by size of the header */
    pThisBlock = ((tPlMemoryBlockHeader *) pMemory) - 1;

    /* At this point, it is unknown whether the pointer is valid or not,
       so scan through all the pool to validate it without dereferencing it. */
    for (i=0; i<NUM_MEM_POOLS; i++)
    {
        if ((uintptr_t *)(pThisBlock) >= aMemoryPoolControl[i].pool_start &&
            (uintptr_t *)(pThisBlock) < aMemoryPoolControl[i].pool_end)
        {
            /* The pointer is within one of the pool boundaries so it is safe
               to dereference it. */
            if (i != GET_POOLINDEX(pThisBlock->u.poolIndex))
            {
                /* The pointer is not in the right pool. */
                panic_diatribe(id,
                               (DIATRIBE_TYPE)((uintptr_t)pMemory));
            }
            break;
        }
    }
    if (i == NUM_MEM_POOLS)
    {
        /* The pointer to free is neither in the heaps nor the pools. */
        panic_diatribe(id,
                       (DIATRIBE_TYPE)((uintptr_t)pMemory));
    }
    poolIndex = GET_POOLINDEX(pThisBlock->u.poolIndex);

    /* Check the guard data */
    pBlock = (uintptr_t *)(pThisBlock + 1);
    guard = aMemoryPoolControl[poolIndex].blockSizeWords;
    while ((pBlock[guard] == PMALLOC_DEBUG_GUARD_1) && (guard > 0))
    {
        guard--;
    }
    if ((guard == 0) || (pBlock[guard] != PMALLOC_DEBUG_GUARD_0))
    {
        panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION,
                       (DIATRIBE_TYPE)((uintptr_t)pMemory));
    }

    /* Check the block really was allocated */
    if (pThisBlock->file == NULL)
    {
        panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION,
                       (DIATRIBE_TYPE)((uintptr_t)pThisBlock));
    }
}
#endif /* PMALLOC_DEBUG */

/**
 * \brief Try to find a free block in the preferred pool. If preferred pool is not present,
 *  it will fallback
 *
 * \param[in] pool Pool number to look in (ignoring DM1/DM2 split)
 * \param[in] preference choice of DM1, DM2 or don't care
 * \param[out] found_pool actual index of pool (allowing for DM1/DM2 split)
 *
 * \return TRUE if a block was found
 */
/* Try to find a free block in the preferred pool */
static bool is_block_free(int pool, int preference, int *found_pool)
{
    /* Select the DM1 pool as a starting point */
    *found_pool = pool;
    switch (preference)
    {
    case MALLOC_PREFERENCE_DM1:
        /* Just check the default DM1 pool */
        break;
    case MALLOC_PREFERENCE_DM2:
        if (pool < NUM_MEM_POOLS_PER_BUS)
        {
            /* Look in the DM2 pool */
            *found_pool = DM2_POOL(pool);
        }
        else
        {
            /* No DM2 pool, can't allocate anything */
            return FALSE;
        }
        break;
    case MALLOC_PREFERENCE_SYSTEM:
        /* Pick the DM2 pool if it has more free blocks */
        if ((pool < NUM_MEM_POOLS_PER_BUS) &&
            (aMemoryPoolControl[DM2_POOL(pool)].numBlocksFree >
             aMemoryPoolControl[pool].numBlocksFree))
        {
            *found_pool = DM2_POOL(pool);
        }
        return (aMemoryPoolControl[*found_pool].numBlocksFree > 0);

    case MALLOC_PREFERENCE_NONE:
    default:
        /* Pick the DM2 pool if it has more free blocks */
        if ((pool < NUM_MEM_POOLS_PER_BUS) &&
            (aMemoryPoolControl[DM2_POOL(pool)].numBlocksFree >
             aMemoryPoolControl[pool].numBlocksFree))
        {
            *found_pool = DM2_POOL(pool);
        }
        break;
    }

    return (aMemoryPoolControl[*found_pool].numBlocksFree > 0);
}

/**
 * \brief Initialise the memory pool blocks
 *
 * For each pool, this populates a linked list of free blocks
 * which is initially all of the blocks, in order.
 */
static void pmalloc_link_pools(void)
{
    int poolCount;

    for (poolCount = 0; poolCount < NUM_MEM_POOLS; poolCount++)
    {
        tPlMemoryBlockHeader *pCurrentBlock;
        int blockCount, blockSizeWords;
        tPlMemoryPoolControlStruct *pCurrentPool;

        pCurrentPool = &aMemoryPoolControl[poolCount];

#ifdef PMALLOC_DEBUG
        /* If we know what the pool limits are, check that the first block address looks valid */
        if (pCurrentPool->pFirstFreeBlock != (tPlMemoryBlockHeader *)(aMemoryPoolControl[poolCount].pool_start))
        {
            panic(PANIC_AUDIO_INVALID_POOL_INFO);
        }
#endif

        /* Go through each block in this pool, except the last block,  setting
         * the next free block pointer. Last block has its pointer set to NULL */
        pCurrentBlock = pCurrentPool->pFirstFreeBlock;
        blockSizeWords = pCurrentPool->blockSizeWords;
        blockSizeWords += PL_MEM_BLOCK_OVERHEAD_WORDS;
        for (blockCount = 0; blockCount < pCurrentPool->numBlocksFree-1; blockCount++)
        {
            tPlMemoryBlockHeader *pNextBlock;

#ifdef PMALLOC_DEBUG
            /* Initialise the file and line members. */
            pCurrentBlock->file = NULL;
            pCurrentBlock->line = 0;
            /* Initialise the guard word */
            *((uintptr_t *)(pCurrentBlock) + blockSizeWords - 1) = PMALLOC_DEBUG_GUARD_0;
#endif

            /* Calc the address of the next free block. Note careful casting is
             * needed to ensure the pointer arithmetic works correctly */
            pNextBlock = (tPlMemoryBlockHeader *) ((uintptr_t *) pCurrentBlock + blockSizeWords);

            /* Set the header of the curent block to point to the next and
             * update the current pointer for the next iteration */
            pCurrentBlock->u.pNextBlock = pNextBlock;
            pCurrentBlock = pNextBlock;
        }

        /* Set the last pointer to NULL */
        pCurrentBlock->u.pNextBlock = (tPlMemoryBlockHeader *)  NULL;
#ifdef PMALLOC_DEBUG
        /* Initialise the guard word */
        *((uintptr_t *)(pCurrentBlock) + blockSizeWords - 1) = PMALLOC_DEBUG_GUARD_0;
#endif
    }
}

/**
 * \brief Calculate the boundaries of each pool and allocate memory for it.
 *
 * This allocates the pool arrays from the heap, then fills in the
 * control structure with the allocated pointers and block counts.
 */
static void pmalloc_allocate_pools(void)
{
    /* The size of the blocks in each pool was determined empirically.
       The pools are sorted from smaller blocks to bigger blocks.
       The size of the blocks in each pool is identical per bus but the actual
       number of blocks can be different. */
    const uint8 blockSizeWords[NUM_MEM_POOLS_PER_BUS] = { 4, 7, 12 };
    unsigned memPoolSize[NUM_MEM_POOLS];
    unsigned dm_size_words[NUM_DM_BUSES];
    uintptr_t **memPoolLimit;
    unsigned i, j;

    memPoolLimit = &memPoolLimits[0];
    for (i = 0; i < NUM_DM_BUSES; i++)
    {
        uintptr_t *memPoolStart;

        dm_size_words[i] = 0;
        for (j = 0; j < NUM_MEM_POOLS_PER_BUS; j++)
        {
            unsigned k;

            k = i * NUM_MEM_POOLS_PER_BUS + j;
            aMemoryPoolControl[k].blockSizeWords = blockSizeWords[j];
            memPoolSize[k] = blockSizeWords[j];
            memPoolSize[k] += PL_MEM_BLOCK_OVERHEAD_WORDS;
            memPoolSize[k] *= pool_block_counts[k];
            dm_size_words[i] += memPoolSize[k];
        }

        memPoolStart = heap_alloc(dm_size_words[i] * sizeof(uintptr_t),
                                  MALLOC_PREFERENCE_DM1 + i);
        if (memPoolStart == NULL)
        {
            panic(PANIC_AUDIO_INVALID_POOL_INFO);
        }

        for (j = 0; j < NUM_MEM_POOLS_PER_BUS; j++)
        {
            tPlMemoryPoolControlStruct *control;
            unsigned k;
            
            k = i * NUM_MEM_POOLS_PER_BUS + j;
            control = &aMemoryPoolControl[k];
            control->pFirstFreeBlock = (tPlMemoryBlockHeader *) memPoolStart;
            control->numBlocksFree = pool_block_counts[k];
            control->minBlocksFree = pool_block_counts[k];
#ifdef PMALLOC_DEBUG
            control->pool_start = memPoolStart;
            control->pool_end = memPoolStart;
            control->pool_end += memPoolSize[k];
#endif /* PMALLOC_DEBUG */
            *memPoolLimit++ = memPoolStart;
            memPoolStart += memPoolSize[k];
        }
        *memPoolLimit++ = memPoolStart;
    }
}

#if defined(INSTALL_MIB)
/**
 * \brief Get the chip's dynamic pools configuration from the system.
 *
 * \param core      The core to be configured.
 * \param mib_value Pointer to store the chip's dynamic pool configuration.
 *
 * \return TRUE if the configuration was retrieved successfully.
 */
static inline bool get_mib_value(PROC_ID_NUM core, POOL_MIB_VALUE *mib_value)
{
    uint8 *buf;
    int16 len_read;
    mibkey key;

    if (core == PROC_PROCESSOR_0)
    {
        key = MALLOCPOOLBLOCKCOUNTS;
    }
    else
    {
        key = MALLOCP1POOLBLOCKCOUNTS;
    }

    buf = (uint8 *) &mib_value->array[0];
    memset(buf, 0, sizeof(POOL_MIB_VALUE));
    len_read = mibgetreqstr(key,
                            buf,
                            sizeof(POOL_MIB_VALUE));
    if (len_read != sizeof(POOL_MIB_VALUE))
    {
        return FALSE;
    }

    return TRUE;
}
#else
static inline bool get_mib_value(PROC_ID_NUM core, POOL_MIB_VALUE *mib_value)
{
    uint16 *buf;

    /* The values have been copied from "audio_mib.xml". */
    buf = &mib_value->array[0];
    if (core == PROC_PROCESSOR_0)
    {
        buf[0] = (uint16) swap_16b(0x0050);
        buf[1] = (uint16) swap_16b(0x00C0);
        buf[2] = (uint16) swap_16b(0x0050);

        buf[4] = (uint16) swap_16b(0x0050);
        buf[5] = (uint16) swap_16b(0x00C0);
        buf[6] = (uint16) swap_16b(0x0050);
    }
    else
    {
        buf[0] = (uint16) swap_16b(0x0040);
        buf[1] = (uint16) swap_16b(0x0060);
        buf[2] = (uint16) swap_16b(0x0040);

        buf[4] = (uint16) swap_16b(0x0040);
        buf[5] = (uint16) swap_16b(0x0060);
        buf[6] = (uint16) swap_16b(0x0040);
    }

    return TRUE;
}
#endif /* defined(INSTALL_MIB) */

/**
 * \brief Get the number of blocks for each pool.
 *
 * Read and validate block counts from MIB or use default
 * values if MIB is not available.
 */
static void pmalloc_get_pool_configuration(void)
{
    PROC_ID_NUM core;

    core = proc_get_processor_id();
    if (core == PROC_PROCESSOR_0)
    {
        for (core = PROC_PROCESSOR_0; core < PROC_PROCESSOR_BUILD; core++)
        {
            POOL_MIB_VALUE mib_value;
            unsigned poolnum;
            uint16 *counts;

            counts = pool_block_counts_array[core];
            PL_ASSERT(get_mib_value(core, &mib_value));

            /* Check the MIB values, replace static counts if they look OK */
            for (poolnum = 0; poolnum < NUM_MEM_POOLS; poolnum++)
            {
                uint16 blockcount;

                blockcount = swap_16b(mib_value.array[poolnum]);
                counts[poolnum] = 1;
                if (blockcount >= 1)
                {
                    counts[poolnum] = blockcount;
                }
            }
#ifdef __KCC__
            L2_DBG_MSG4("PMALLOC_DYNAMIC_POOLS P%u DM1 block counts: %u %u %u",
                        core, counts[0], counts[1], counts[2]);
            L2_DBG_MSG4("PMALLOC_DYNAMIC_POOLS P%u DM2 block counts: %u %u %u",
                        core, counts[3], counts[4], counts[5]);
#endif
        }
    }
    else
    {
        /* Populate P1's block counts with the values initialised by P0 */
        memcpy(pool_block_counts, pool_block_counts_array[core], sizeof(pool_block_counts));
    }
}

/****************************************************************************
Public Function Definitions
*/

/**
 * NAME
 *   init_pmalloc
 *
 * \brief   Initiallise the memory pools
 *
 * Initial heap configuration happens here.
 *
 * With static pools the pool control setup follows.
 *
 * For dynamic pools on P0, pool setup is deferred until the heap
 * reconfiguration which happens in config_pmalloc. Dynamic pool config
 * for P1 happens here, because in that case init_heap does the full heap
 * configuration.
 *
 */
void init_pmalloc(void)
{
    init_heap();
}

/**
 * \brief configure the memory pools and heap
 *        This will be called only after completely booting up
 *        the audio subsystem
 */
void config_pmalloc(void)
{
    if (PROC_PRIMARY_CONTEXT())
    {
        /* Once a communication link to the rest of the system
           is available, it becomes possible to resize the heaps. */
        config_heap();
    }

    /* Check that the memory guard has not be overwritten. */
    if (private_mem_guard != PMALLOC_PRIVATE_MEM_GUARD)
    {
        panic(PANIC_AUDIO_PRIVATE_MEMORY_CORRUPTED);
    }

    if (heap_threshold == 0)
    {
        /* Get the number of blocks for each pool. */
        pmalloc_get_pool_configuration();

        /* Calculate the boundaries of each pool and allocate memory for it. */
        pmalloc_allocate_pools();

        /* Create a linked link of free blocks for each pool. */
        pmalloc_link_pools();

        /* Now start using the pool blocks for sizes below HEAP_THRESHOLD */
        heap_threshold = HEAP_THRESHOLD;
    }
}

#if defined(SUPPORTS_MULTI_CORE)
/**
 * \brief Function called when the state of the secondary processor changes.
 *
 * \param transition What is happening to the secondary core.
 */
void pmalloc_processor_change(PROC_TRANSITION transition)
{
    if (transition == PROC_DISABLE)
    {
        config_pmalloc();
    }
    else if (transition == PROC_START)
    {
        heap_processor_change(PROC_PROCESSOR_1, HEAP_MAIN, TRUE);
    }
    else if (transition == PROC_STOP)
    {
        heap_processor_change(PROC_PROCESSOR_1, HEAP_MAIN, FALSE);
    }
}
#endif /* defined(SUPPORTS_MULTI_CORE) */

/**
 * NAME
 *   xppmalloc
 *
 * \brief Memory allocation, based on memory pools (does not panic, does not zero)
 *
 * FUNCTION
 *   Allocate a chunk of memory from one of the memory pools pointed to from
 *   aMemoryPoolControl. A pointer to the smallest availaible block is
 *   returned. Returns a Null pointer and raises a fault if no suitable block
 *   is available.
 *   The memory is not initialised.
 *
 * \param[in] numBytes number of bytes required, as returned by sizeof. See NOTES
 * for details of what a byte is (its not necessarily 8 bits!)
 * \param[in] preference choice of DM1, DM2 or don't care
 *
 * \return pointer to the block of memory allocated
 *
 * \note
 *   Here a "byte" is the smallest addressable storage unit on the processor
 *   used. This is 8 bits on a pentium or 24 bits on Kalimba. The code is written
 *   so that a call xpmalloc(sizeof(SomeStruct)) will work as expected
 *
 */
#ifdef PMALLOC_DEBUG
void *xppmalloc_debug(unsigned int numBytes, unsigned int preference, const char *file, unsigned int line)
#else
void *xppmalloc(unsigned int numBytes, unsigned int preference)
#endif
{
    int poolCount, found_pool;
    int numWords;

    /* xpmalloc must return NULL if requested to allocate zero bytes */
    if (numBytes == 0)
    {
        return NULL;
    }

    /* Some allocations always go with heap. */
    if ((numBytes > heap_threshold * sizeof(uintptr_t)) ||
        (preference == MALLOC_PREFERENCE_EXTRA) ||
        (preference == MALLOC_PREFERENCE_EXTRA_DM1) ||
        (preference == MALLOC_PREFERENCE_EXTRA_DM2) ||
        (preference == MALLOC_PREFERENCE_FAST) ||
        (preference == MALLOC_PREFERENCE_SHARED) ||
        (preference == MALLOC_PREFERENCE_EXTERNAL))
    {
        void *heap_mem;
#ifdef PMALLOC_DEBUG
        heap_mem = heap_alloc_debug(numBytes, preference, file, line);
#else
        heap_mem = heap_alloc(numBytes, preference);
#endif
        return heap_mem;
    }

    numWords = (numBytes - 1)/sizeof(uintptr_t) + 1;

    /* Go through all the pools, till we find one big enough and with free pools */
    for (poolCount = 0; poolCount < NUM_MEM_POOLS_PER_BUS; poolCount++)
    {
        tPlMemoryPoolControlStruct *control;
        tPlMemoryBlockHeader *pThisBlock, *pNextBlock;
        void *pPointer;

        if (aMemoryPoolControl[poolCount].blockSizeWords < numWords)
        {
            continue;
        }

        LOCK_INTERRUPTS;
        if (!is_block_free(poolCount, preference, &found_pool))
        {
            /* If we get here then the current memory pool has blocks big enough,
             * but they are all used. Need to unlock IRQs and try next pool */
            UNLOCK_INTERRUPTS;
            PL_PRINT_P2(TR_PL_MALLOC_POOL_EMPTY, "PL Malloc for %i 'bytes'. Pool %u has blocks big enough, but none left\n",
                                       numBytes, poolCount);
            continue;
        }

        control = &aMemoryPoolControl[found_pool];
        /* There are available blocks in this pool. Allocate one,
           unlock interrupts and break out of the for loop */
        control->numBlocksFree--;

        /* Update the minimum-free tracking */
        if (control->numBlocksFree < control->minBlocksFree)
        {
            control->minBlocksFree = control->numBlocksFree;
        }

        pThisBlock = control->pFirstFreeBlock;

#ifdef PMALLOC_DEBUG
        /* Check that the block points into the right pool */
        if ((uintptr_t *)(pThisBlock) < control->pool_start ||
            (uintptr_t *)(pThisBlock) >= control->pool_end)
        {
            panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION, (DIATRIBE_TYPE)((uintptr_t)pThisBlock));
        }
#endif

        /* Update the head pointer for this pool to point to the next block */
        pNextBlock = pThisBlock->u.pNextBlock;
        control->pFirstFreeBlock = pNextBlock;

        /*Update the header for this block to indicate which pool the block came from */
        pThisBlock->u.poolIndex = SET_POOLINDEX(found_pool);

#ifdef PMALLOC_DEBUG
        /* Check that the block looks free */
        if (pThisBlock->file != NULL)
        {
            panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION, (DIATRIBE_TYPE)((uintptr_t)pThisBlock));
        }

        pThisBlock->file = file;
        pThisBlock->line = line;

        /* Check the guard word */
        if (*((uintptr_t *)(pThisBlock + 1) +
            control->blockSizeWords) != PMALLOC_DEBUG_GUARD_0)
        {
            panic_diatribe(PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION, (DIATRIBE_TYPE)((uintptr_t)pThisBlock));
        }
#endif

        /* Set output pointer to point at the memory after the block header.
         * Pointer arithmetic means +1 increments pThisBlock pointer by size of header*/
        pPointer = (void  *) (pThisBlock + 1);

        UNLOCK_INTERRUPTS;

#ifdef PMALLOC_DEBUG
        /* Fill in the unused part of the block with some known data
         * so we can see if any of it got overwritten
         */
        {
            uintptr_t *pBlock = (uintptr_t *)pPointer;
            unsigned int guard;

            /* There is always at least one guard word, hence the +1 below */
            for (guard = numWords+1; guard < control->blockSizeWords+1; guard++)
            {
                pBlock[guard] = PMALLOC_DEBUG_GUARD_1;
            }
            pBlock[numWords] = PMALLOC_DEBUG_GUARD_0;
        }
#endif

        PL_PRINT_P3(TR_PL_MALLOC, "PL Malloc for %i 'bytes' - pointer %p allocated from pool %u\n",
                                   numBytes, pPointer, found_pool);
#ifdef PL_DEBUG_MEM_ON_HOST
        printf("[MEM] 0x%x alloc of %i bytes ",pPointer, numBytes);
#endif

        return(pPointer);
    }

    patch_fn(xppmalloc);

    if (preference == MALLOC_PREFERENCE_SYSTEM)
    {
        /* See if we can allocate a block from the heap */
#ifdef PMALLOC_DEBUG
        return heap_alloc_debug(numBytes, preference, file, line);
#else
        return heap_alloc(numBytes, preference);
#endif
    }

    fault_diatribe(FAULT_AUDIO_INSUFFICIENT_MEMORY, numBytes);
    /* If we get here, no block has been allocated. Return NULL  */
    PL_PRINT_P1(TR_PL_MALLOC_FAIL,"PL Malloc for %i 'bytes' failed\n",numBytes);
    return(NULL);
}


/**
 * NAME
 *   ppmalloc
 *
 * \brief Memory allocation (panics if no memory)
 *
 * FUNCTION
 *   As xppmalloc, except that it panics if it does not have enough memory to allocate.
 *
 * \param[in] numBytes number of bytes required, as returned by sizeof.
 * See xppmalloc for details of what a "byte" is.
 * \param[in] preference choice of DM1, DM2 or don't care
 *
 * \return pointer to the block of memory allocated
 *
 */
#ifdef PMALLOC_DEBUG
void *ppmalloc_debug(unsigned int numBytes, unsigned int preference, const char *file, unsigned int line)
#else
void *ppmalloc(unsigned int numBytes, unsigned int preference)
#endif
{
    /* [p]pmalloc must panic if requested to allocate Zero bytes */
    if(numBytes == 0)
    {
        panic(PANIC_AUDIO_REQ_ZERO_MEMORY);
    }

    /* If a version of malloc which can panic is called with no DM1/DM2 preference
     * assume it's a "system" allocation that can eat into the reserved blocks
     */
    if (preference == MALLOC_PREFERENCE_NONE)
    {
        preference = MALLOC_PREFERENCE_SYSTEM;
    }

#ifdef PMALLOC_DEBUG
    void *ptr = xppmalloc_debug(numBytes, preference, file, line);
#else
    void *ptr = xppmalloc(numBytes, preference);
#endif

    /* panic out of memory */
    if(ptr == NULL)
    {
        panic_diatribe(PANIC_AUDIO_HEAP_EXHAUSTION, numBytes);
    }

    return ptr;
}




/**
 * NAME
 *   xzppmalloc
 *
 * \brief Memory allocation and zeroing (returns Null if no memory)
 *
 * FUNCTION
 *   As xppmalloc, except that the allocated memory is zeroed.
 *
 * \param[in] numBytes number of bytes required, as returned by sizeof.
 * See xppmalloc for details of what a "byte" is.
 * \param[in] preference choice of DM1, DM2 or don't care
 *
 * \return pointer to the block of memory allocated
 *
 */
#ifdef PMALLOC_DEBUG
void *xzppmalloc_debug(unsigned int numBytes, unsigned int preference, const char *file, unsigned int line)
#else
void *xzppmalloc(unsigned int numBytes, unsigned int preference)
#endif
{

    /* xz[p]pmalloc must return Null if requested to allocate Zero bytes */
    if(numBytes <= 0)
    {
        return(NULL);
    }

#ifdef PMALLOC_DEBUG
    void *ptr = xppmalloc_debug(numBytes, preference, file, line);
#else
    void *ptr = xppmalloc(numBytes, preference);
#endif

    if (ptr)
    {
        memset(ptr, 0, numBytes);
    }

    return ptr;
}



/**
 * NAME
 *   zppmalloc
 *
 * \brief Memory allocation and zeroing (panics if no memory)
 *
 * FUNCTION
 *   As ppmalloc, except that the allocated memory is zeroed
 *
 * \param[in] numBytes number of bytes required, as returned by sizeof.
 * See xppmalloc for details of what a "byte" is.
 * \param[in] preference choice of DM1, DM2 or don't care
 *
 * \return pointer to the block of memory allocated
 *
 */
#ifdef PMALLOC_DEBUG
void *zppmalloc_debug(unsigned int numBytes, unsigned int preference, const char *file, unsigned int line)
#else
void *zppmalloc(unsigned int numBytes, unsigned int preference)
#endif
{
    /* Allocate the memory in pmalloc but we will zero it here */
#ifdef PMALLOC_DEBUG
    void *ptr = ppmalloc_debug(numBytes, preference, file, line);
#else
    void *ptr = ppmalloc(numBytes, preference);
#endif
    /* ppmalloc always returns pointer or panic, therefore no need to check
     * if ptr is null.*/
    memset(ptr, 0, numBytes);
    return ptr;
}

/**
 * NAME
 *   pfree
 *
 * \brief free memory allocated using versions of malloc (does not panic if passed Null)
 *
 * FUNCTION
 *   Looks up which memory pool this block cames from, and returns the memory
 *   to the pool. NOTE requires that the memory just above the top of the
 *   buffer pointed to by pMemory contains the block header information, with
 *   the index of pool from which this block came. This will be the case
 *   assuming the memory was allocated using xpmalloc and there has been no
 *   memory corruption
 *
 * \param[in] pMemory pointer to the memory to be freed
 *
 */
void pfree(void *pMemory)
{
    unsigned int poolIndex;
    tPlMemoryBlockHeader *pThisBlock;
    tPlMemoryPoolControlStruct *pThisPool;

    if (pMemory == NULL)
    {
        return;
    }

#ifdef PMALLOC_DEBUG
    pvalidate_internal(pMemory, PANIC_AUDIO_FREE_INVALID);
#endif
    PL_PRINT_P1(TR_PL_FREE,"freeing memory %p\n", pMemory);

    if (!is_addr_in_pool(pMemory))
    {
        PL_PRINT_P0(TR_PL_FREE,"freeing memory from heap\n");
        heap_free(pMemory);
        return;
    }

    PL_PRINT_P0(TR_PL_FREE,"freeing memory from pools\n");

    /* Adjust pointer to point to header in this block. Pointer arithmentic
     * means the -1 decrements the pointer by size of the header */
    pThisBlock = ((tPlMemoryBlockHeader *) pMemory) - 1;

    /* Look up which pool this block came from */
    poolIndex = GET_POOLINDEX(pThisBlock->u.poolIndex);
    if (poolIndex >= NUM_MEM_POOLS)
    {
        panic_diatribe(PANIC_AUDIO_FREE_INVALID, (DIATRIBE_TYPE)((uintptr_t) pMemory));
    }
    pThisPool = &aMemoryPoolControl[poolIndex];

#ifdef PMALLOC_DEBUG
    {
        uintptr_t *pBlock = (uintptr_t *)pMemory;
        /* Reinstate the end-of-block guard word */
        pBlock[pThisPool->blockSizeWords] = PMALLOC_DEBUG_GUARD_0;
        pThisBlock->file = NULL;
        pThisBlock->line = 0;
    }
#endif

    PL_PRINT_P2(TR_PL_FREE, "Free of pointer %p from pool %u\n", pMemory, poolIndex);

    /* Put this block back into the linked list for this pool */
    LOCK_INTERRUPTS;
    pThisPool->numBlocksFree++;
    pThisBlock->u.pNextBlock = pThisPool->pFirstFreeBlock;
    pThisPool->pFirstFreeBlock = pThisBlock;
    UNLOCK_INTERRUPTS;
}

/**
 * NAME
 *   psizeof
 *
 * \brief get the actual size of a dynamically-allocated block
 *
 * FUNCTION
 *   Looks up which memory pool this block cames from, and returns
 *   the block size for the pool.
 *   NOTE requires that the memory just above the top of the
 *   buffer pointed to by pMemory contains the block header information, with
 *   the index of pool from which this block came. This will be the case
 *   assuming the memory was allocated using xpmalloc and there has been no
 *   memory corruption
 *
 * \param[in] pMemory pointer to the block to get the size of
 *
 */
int psizeof(void *pMemory)
{
    unsigned int poolIndex;
    tPlMemoryBlockHeader *pThisBlock;

    if (pMemory == NULL)
    {
        return 0;
    }

#ifdef PMALLOC_DEBUG
    pvalidate(pMemory);
#endif

    if (!is_addr_in_pool(pMemory))
    {
        return heap_sizeof(pMemory);
    }

    /* Adjust pointer to point to header in this block. */
    pThisBlock = ((tPlMemoryBlockHeader *) pMemory) - 1;

    /* Look up which pool this block came from */
    poolIndex = GET_POOLINDEX(pThisBlock->u.poolIndex);

    if (poolIndex >= NUM_MEM_POOLS)
    {
        /* Probably not a valid dynamically-allocated block.
         * Could panic here, but for now just get out
         */
        return 0;
    }

#ifdef PMALLOC_DEBUG
    /* Adjust guard words, because anyone calling this
     * might use more memory than they originally asked for
     */
    ((uintptr_t *)pMemory)[aMemoryPoolControl[poolIndex].blockSizeWords] = PMALLOC_DEBUG_GUARD_0;
#endif

    return aMemoryPoolControl[poolIndex].blockSizeWords * sizeof(uintptr_t);
}

/**
 * NAME
 *   pool_cur
 *
 * \brief Function for getting the currently available pools size.
 *
 * FUNCTION
 *   Looks up the currently available memory pools, and returns the sum of all the
 *   available memory pool size.
 *
 * \return Returns the size of the currently available pool memory in words.
 */

unsigned pool_cur(void)
{
    unsigned poolCount;
    unsigned total_cur_size = 0;
    /* Calculate the total pool size. */
    /* Go through all the pools to calculate the total pool size. */
    for(poolCount = 0; poolCount < NUM_MEM_POOLS; poolCount++)
    {
        total_cur_size += aMemoryPoolControl[poolCount].blockSizeWords * aMemoryPoolControl[poolCount].numBlocksFree;
    }
    /* Add in any cached data structures */
    if (cached_report_handler != NULL)
    {
        total_cur_size += (cached_report_handler() / sizeof(unsigned));
    }
    return total_cur_size;
}

/**
 * NAME
 *   pool_min
 *
 * \brief Function for getting the minimum available pools size.
 *
 * FUNCTION
 *   Looks up the minimum available memory pools, and returns the sum of all the
 *   minimum available memory pool size.
 *
 * \return Returns the size of the minimum available pool memory in words.
 */
unsigned pool_min(void)
{
    unsigned poolCount;
    unsigned total_min_size = 0;
    /* Calculate the total pool size. */
    /* Go through all the pools to calculate the total pool size. */
    for(poolCount = 0; poolCount < NUM_MEM_POOLS; poolCount++)
    {
        total_min_size += aMemoryPoolControl[poolCount].blockSizeWords * aMemoryPoolControl[poolCount].minBlocksFree;
    }
    return total_min_size;
}

#if defined(COMMON_SHARED_HEAP)
/**
 * \brief Function for checking if the address supplied is in the shared heap.
 *
 * \param ptr_mem pointer to the address
 *
 * \return Returns True is the address supplied is in the shared heap.
 */
bool is_addr_in_shared_memory(void *ptr_mem)
{
    if (get_heap_num(ptr_mem) == HEAP_SHARED)
    {
        return TRUE;
    }

    return FALSE;
}
#endif /* COMMON_SHARED_HEAP */

/**
 * \brief Function for getting the total (used and free) pools size.
 *
 * \return Returns total size of the pool memory in words.
 */
unsigned pool_size_total(void)
{
    unsigned i;
    unsigned total_size;

    total_size = 0;
    for (i = 0; i < NUM_MEM_POOLS; i++)
    {
        total_size += aMemoryPoolControl[i].blockSizeWords * pool_block_counts[i];
    }

    return total_size;
}

/**
 * NAME
 *   pool_clear_watermarks
 *
 * \brief Function for clearing the minimum available pools size (watermarks).
 */
void pool_clear_watermarks(void)
{
    unsigned poolCount;
    /* Calculate the total pool size. */
    LOCK_INTERRUPTS;
    /* Go through all the pools to calculate the total pool size. */
    for(poolCount = 0; poolCount < NUM_MEM_POOLS; poolCount++)
    {
        aMemoryPoolControl[poolCount].minBlocksFree = aMemoryPoolControl[poolCount].numBlocksFree;
    }
    UNLOCK_INTERRUPTS;
}

/*
 *   pmalloc_cached_report
 */
pmalloc_cached_report_handler pmalloc_cached_report(pmalloc_cached_report_handler new_handler)
{
    pmalloc_cached_report_handler old_handler = cached_report_handler;
    cached_report_handler = new_handler;
    /* Pass the previous handler back to the caller to save */
    return old_handler;
}

#ifdef PMALLOC_DEBUG
/**
 * NAME
 *   pvalidate
 *
 * \brief debug function to check that a dynamically-allocated block looks sensible
 *
 * \param[in] pMemory pointer to the block
 *
 */
void pvalidate(void *pMemory)
{
    pvalidate_internal(pMemory, PANIC_AUDIO_DEBUG_MEMORY_CORRUPTION);
}
#endif /* PMALLOC_DEBUG */

#ifdef INSTALL_DM_MEMORY_PROFILING
/**
 * NAME
 *   pool_tag_dm_memory
 *
 * \brief Function to 'tag' a block of allocated pool memory
 *
 * \param[in] ptr  Pointer to the allocated block to be tagged
 * \param[in] id   The owner id to tag the allocated block with
 *
 * \return Returns TRUE if  tagging succeeded,
 *                 FALSE if tagging failed (ptr is not a pool block).
 */
static bool pool_tag_dm_memory(void *ptr, DMPROFILING_OWNER id)
{
    tPlMemoryBlockHeader *pThisBlock;
    unsigned int poolIndex;

    /* Adjust pointer to point to header in this block. Pointer arithmentic
     * means the -1 decrements the pointer by size of the header */
    pThisBlock = ((tPlMemoryBlockHeader *) ptr) - 1;

    /* Look up which pool this block came from */
    poolIndex = GET_POOLINDEX(pThisBlock->u.poolIndex);
    if (poolIndex >= NUM_MEM_POOLS)
    {
        return FALSE;
    }
    pThisBlock->u.poolIndex = SET_POOLINDEX_WITH_ID(poolIndex, id);
    return TRUE;
}

/**
 * NAME
 *   pmalloc_tag_dm_memory
 *
 * \brief Function to 'tag' a block of allocated heap or pool memory
 *        with its 'owner id'. The 'owner id' is usually derived from
 *        'get_current_id', but in a handful of cases where the owner
 *        is not the current task (as, for example, in 'create_task'),
 *        this function allows an allocated block to be assigned to
 *        a non-current-task owner.
 *
 * \param[in] ptr  Pointer to the allocated block to be tagged
 * \param[in] id   The owner id to tag the allocated block with
 *
 * \return Returns TRUE if  tagging succeeded,
 *                 FALSE if tagging failed.
 *         FALSE would have been caused by a pointer not pointing
 *         to an allocated heap or pool block.
 */
bool pmalloc_tag_dm_memory(void *ptr, DMPROFILING_OWNER id)
{
    if (is_addr_in_pool(ptr))
    {
        return pool_tag_dm_memory(ptr, id);
    }
    else
    {
        return heap_tag_dm_memory(ptr, id);
    }
}

/**
 * NAME
 *   pmalloc_tag_dm_memory_override
 *
 * \brief Function to 'tag' a block of allocated heap or pool memory
 *        with 'owner id', as per the parameter given, for all
 *        DM memory allocations after this call. This overrides the
 *        usual method of using the current task id for DM Memory
 *        profiling.
 *
 * \param[in] id   The owner id to tag any allocated blocks with
 */
void pmalloc_tag_dm_memory_override(DMPROFILING_OWNER id)
{
    dmprof_override.stored_taskid = id;
    dmprof_override.service_taskid = get_current_task();
    dmprof_override.use_stored_task_id = TRUE;
}

/**
 * NAME
 *   pmalloc_tag_dm_memory_reset
 *
 * \brief Function to reset 'tagging' a block of allocated heap or pool
 *        memory after a call to 'pmalloc_tag_dm_memory_override'. This 
 *        resets DM Profiling to return to the usual method of using the
 *        current task id for DM Memory profiling.
 */
void pmalloc_tag_dm_memory_reset(void)
{
    dmprof_override.use_stored_task_id = FALSE;
    dmprof_override.service_taskid = 0u;
    dmprof_override.stored_taskid = 0u;
}

/**
 * NAME
 *   pmalloc_get_packed_task
 *
 * \brief Return the (packed) task id of the current task, except
 *        when this is overridden (by 'pmalloc_tag_dm_memory_override');
 *        if overridden, return the stored packed task id.
 *
 * \return Returns packed task id to use for heap and pool tagging
 */
uint8 pmalloc_get_packed_task(void)
{
    if ((dmprof_override.use_stored_task_id) &&
        (get_current_task() == dmprof_override.service_taskid))
    {
        return dmprof_override.stored_taskid;
    }
    else
    {
        return sched_get_packed_task();
    }
}
#endif

/****************************************************************************
Module Test
*/

#ifdef PL_MEM_POOL_TEST
/* Code needed only for testing of the real code in this file */

/**
 * Global data used by the test harness to know how many blocks each pool has
 */
int aPlMemPoolNumBlocks[NUM_MEM_POOLS];
int aPlMem1PoolNumBlocks[NUM_MEM_POOLS_PER_BUS];
int aPlMem2PoolNumBlocks[NUM_MEM_POOLS_PER_BUS];

int aPlMemPoolNumReservedBlocks[NUM_MEM_POOLS];
int aPlMem1PoolNumReservedBlocks[NUM_MEM_POOLS_PER_BUS];
int aPlMem2PoolNumReservedBlocks[NUM_MEM_POOLS_PER_BUS];

/**
 * Global data used by the test harness to know how big the blocks are
 */
int aPlMemPoolBlockSizes[NUM_MEM_POOLS];

int numPools, numDM1Pools, numDM2Pools;

/**
 * NAME
 *   PlMemPoolTest
 *
 *   sets up number of pools and test arrays above
 *
 *   Prototype here to avoid contamination public pl_malloc.h
 *
 * \note   USED FOR MODULE TESTING ONLY
 */
void PlMemPoolTest(void)
{
    int n;

    init_pmalloc();
    config_pmalloc();

    for(n=0; n<NUM_MEM_POOLS_PER_BUS; n++)
    {
        aPlMemPoolBlockSizes[n] = aMemoryPoolControl[n].blockSizeWords;
        aPlMemPoolNumBlocks[n] = aMemoryPoolControl[n].numBlocksFree;
        aPlMem1PoolNumBlocks[n] = aMemoryPoolControl[n].numBlocksFree;
        aPlMemPoolNumReservedBlocks[n] = 0;
        aPlMem1PoolNumReservedBlocks[n] = 0;
        if (n < NUM_MEM_POOLS_PER_BUS)
        {
            aPlMemPoolNumBlocks[n] += aMemoryPoolControl[DM2_POOL(n)].numBlocksFree;
            aPlMemPoolNumReservedBlocks[n] += 0;
        }
    }

    for(n=0; n<NUM_MEM_POOLS_PER_BUS; n++)
    {
        aPlMem2PoolNumBlocks[n] = aMemoryPoolControl[DM2_POOL(n)].numBlocksFree;
        aPlMem2PoolNumReservedBlocks[n] = 0;
    }

    numPools = NUM_MEM_POOLS_PER_BUS;
    numDM1Pools = NUM_MEM_POOLS_PER_BUS;
    numDM2Pools = NUM_MEM_POOLS_PER_BUS;
}
#endif /* PL_MEM_POOL_TEST */
