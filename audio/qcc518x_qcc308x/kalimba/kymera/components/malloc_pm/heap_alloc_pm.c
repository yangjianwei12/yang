/****************************************************************************
 * Copyright (c) 2018 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file heap_alloc_pm.c
 * \ingroup malloc_pm
 *
 * Memory allocation/free functionality for program memory (PM)
 */

/****************************************************************************
Include Files
*/

#include <string.h>
#include "malloc_pm/malloc_pm_private.h"
#include "pmalloc/pmalloc.h"
#include "hal/hal_pm_bank.h"
#include "hal/hal_windows.h"
#include "platform/pl_assert.h"
#include "common/interface/util.h"
#include "pm_cache/pm_cache.h"
#if defined(INSTALL_MIB)
#include "mib/mib.h"
#endif
#include "audio_log/audio_log.h"
#ifdef INSTALL_THREAD_OFFLOAD
#include "thread_offload/thread_offload.h"
#endif

/****************************************************************************
Private Macro Declarations
*/

#define LINKER_ADDRESS extern unsigned

/* Directly accessing the linker provided symbol as an address. */
#define GET_LINKER_ADDRESS(x) ((unsigned)(uintptr_t)&(x))

/* Utility macros to map an offset within physical PM into its equivalent
   in DM space. */
#define INTO_DM(x) (x + PM_RAM_WINDOW - PM_RAM_START_ADDRESS)

#define ROUND_UP_TO_BANK_BOUNDARY(x) ROUND_UP_TO_POW2_MULTIPLE(x, PM_BANK_SIZE)
#define ROUND_DOWN_TO_BANK_BOUNDARY(x) ROUND_DOWN_TO_POW2_MULTIPLE(x, PM_BANK_SIZE)

/* This macro gets the size of a variable in DM, and normalises it into platform words
 * It's assumed that a word that fits in DM, will fit in PM (e.g. 24 or 32 bit DM words will
 * fit in 32-bit PM words) */
#define SIZE_OF_DM_VAR_IN_PM_32(x) (sizeof(x)/ADDR_PER_WORD)
#define BYTES_INTO_PM_32(x) (x/PC_PER_INSTRUCTION)

#define MIN_SPARE_32 BYTES_INTO_PM_32(32)
#define MAGIC_WORD 0xabcd01ul

#ifdef INSTALL_THREAD_OFFLOAD
#define heap_pm_audio_thread_offload_is_configured() thread_offload_is_configured()
#else
#define heap_pm_audio_thread_offload_is_configured() FALSE
#endif

/* The prefetcher can read up to 32 bytes in advance. We don't want
   the prefetcher to access a bank that is shut down. */
#define PM_RAM_PREFETCH_GUARD 32

#if defined(PM_BANKS_CAN_BE_POWERED_OFF) && defined(INSTALL_MIB)
#define READ_MIB_NUMBER_PM_SD_BANKS() mibgetrequ16(PMNUMSHUTDOWNBANKS)
#else
#define READ_MIB_NUMBER_PM_SD_BANKS() 0
#endif

#if defined(INSTALL_MIB)
#define READ_MIB_NUMBER_PM_P1_BANKS() mibgetrequ16(P1PMHEAPALLOCATION)
#else
#define READ_MIB_NUMBER_PM_P1_BANKS() 0
#endif

#if defined(HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK) && \
    defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
#error "Additional heap is not compatible with NVRAM Access to DM."
#endif

/****************************************************************************
Private Type Declarations
*/

typedef enum
{
    P0_BANKS = 0,
    P1_BANKS = 1,
    SD_BANKS = 2,
#ifdef PM_BANKS_CAN_BE_POWERED_OFF
    NUMBER_PM_BANKS_TYPES = 3
#else
    NUMBER_PM_BANKS_TYPES = 2
#endif
} PM_BANK_TYPES;

typedef struct bank_config
{
    uint16 banks_info[NUMBER_PM_BANKS_TYPES];
    bool multi_core_active;
    bool thread_offload_active;
    bool p1_cache_active;
    bool sd_configured;
} PM_BANKS_CONFIG;

typedef struct pm_mem_node
{
    unsigned length_32;
    union
    {
        struct pm_mem_node *next;
        unsigned magic;
    } u;
} pm_mem_node;

typedef struct _pm_heap_block
{
    void   *start_addr; /*!< Pointer to start of the block in DM space. */
    void   *end_addr;   /*!< Pointer to the first element OUTSIDE of
                             the block. */
    uint32 offset;      /*!< Value to be substracted to the 2 previous fields
                             in order to get the address in PM space. */
} PM_BLOCK_LIMITS;

typedef enum
{
    PM_BLOCK_P0   = 0,
#if defined(SUPPORTS_MULTI_CORE)
    PM_BLOCK_P1   = 1,
#if defined(HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK)
    PM_BLOCK_ADDL = 2,
#endif
#endif
#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    PM_BLOCK_SLOW_P0,
#if defined(SUPPORTS_MULTI_CORE)
    PM_BLOCK_SLOW_P1,
#endif
#endif
    NUMBER_PM_BLOCKS
} PM_BLOCK;

typedef enum
{
    PM_HEAP_P0   = 0,
#if defined(SUPPORTS_MULTI_CORE)
    PM_HEAP_P1   = 1,
#endif
#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    PM_HEAP_SLOW_P0,
#if defined(SUPPORTS_MULTI_CORE)
    PM_HEAP_SLOW_P1,
#endif
#endif
    NUMBER_PM_HEAPS,
    PM_HEAP_INVALID,
#if !defined(SUPPORTS_MULTI_CORE)
    PM_HEAP_P1      = PM_HEAP_INVALID,
#endif
#if !defined(SUPPORTS_MULTI_CORE) || !defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    PM_HEAP_SLOW_P1 = PM_HEAP_INVALID,
#endif

} PM_HEAP;

typedef enum
{
    NODE_PREV  = 0,
    NODE_FREED = 1,
    NODE_NEXT  = 2,
    NODE_TOTAL
} NODE_INDEX;

/****************************************************************************
Private Variable Definitions
*/

static unsigned int pm_reserved_size;

PM_BLOCK_LIMITS pm_heap_block[NUMBER_PM_BLOCKS];
static pm_mem_node *freelist_pm[NUMBER_PM_HEAPS];

/* The linker initialise this value which indicates where the PM heap starts */
LINKER_ADDRESS _pm_heap_start_addr;

static void_func_ptr heap_alloc_internal_pm(unsigned size_byte, PM_HEAP heap);


/****************************************************************************
Private Function Definitions
*/

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
static inline bool heap_is_slow(PM_HEAP heap)
{
    if (heap == PM_HEAP_SLOW_P0)
    {
        return TRUE;
    }

#if defined(SUPPORTS_MULTI_CORE)
    if (heap == PM_HEAP_SLOW_P1)
    {
        return TRUE;
    }
#endif

    return FALSE;
}
#else
#define heap_is_slow(x) IGNORE_FALSE(x)
#endif /* defined(CHIP_HAS_NVRAM_ACCESS_TO_DM) */

static inline PM_BLOCK get_default_block_from_heap(PM_HEAP heap)
{
    /* There is no overlap between having an additional block and
       having low heaps. */
    return (PM_BLOCK) heap;
}

static PM_HEAP get_heap_from_ptr(void_func_ptr ptr)
{
    PM_BLOCK block;

    for (block = PM_BLOCK_P0; block < NUMBER_PM_BLOCKS; block++)
    {
        PM_BLOCK_LIMITS *limits;

        limits = &pm_heap_block[block];
        if ((((uintptr_t) ptr) >= ((uintptr_t) limits->start_addr - limits->offset)) &&
            (((uintptr_t) ptr) < ((uintptr_t) limits->end_addr - limits->offset)))
        {
#if defined(SUPPORTS_MULTI_CORE) && \
    defined(HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK)
            if (block == PM_BLOCK_ADDL)
            {
                return PM_HEAP_P1;
            }
#endif
            return (PM_HEAP) block;
        }
    }

    return PM_HEAP_INVALID;
}

static inline PROC_ID_NUM get_last_proc(const PM_BANKS_CONFIG *pm_config)
{
    if (pm_config->multi_core_active)
    {
        return PROC_PROCESSOR_BUILD - 1;
    }
    else
    {
        return PROC_PROCESSOR_0;
    }
}

#ifdef PM_BANKS_CAN_BE_POWERED_OFF
static inline bool sd_is_configured(const PM_BANKS_CONFIG *pm_config)
{
    return pm_config->sd_configured;
}
#else
#define sd_is_configured(x) FALSE
#endif

static void init_and_adjust_pm_heap(const PM_BLOCK_LIMITS *heap_block,
                                    void *next_addr)
{
    unsigned freelength;
    pm_mem_node *heap_node;

    patch_fn_shared(heap_alloc_pm_init);

    heap_node = (pm_mem_node *) heap_block->start_addr;

    /* The free length is the size of the block minus the size
       of the header. It is expressed in words of 32 bits. */
    freelength = (uintptr_t) heap_block->end_addr;
    freelength -= (uintptr_t) heap_block->start_addr;
    freelength -= sizeof(pm_mem_node);
    freelength /= PC_PER_INSTRUCTION;

    /* Enable the PM memory window. */
    LOCK_INTERRUPTS;
    hal_window_open(HAL_WINDOWS_PROGRAM_MEMORY);

    /* Write into PM via the DM bus. */
    heap_node->length_32 = freelength;
    heap_node->u.next = next_addr;

    /* Disable the PM memory window. */
    hal_window_close(HAL_WINDOWS_PROGRAM_MEMORY);
    UNLOCK_INTERRUPTS;
}

/**
 * \brief Extract the information relevant to the component from the MIB.
 *
 * \param pm_config[out] The bank configuration extracted from the MIBs.
 *
 * \note The number of banks is approximative because the caches and
 *       the patches do not stop on a bank boundary.
 */
static void heap_pm_create_descriptor(PM_BANKS_CONFIG *pm_config)
{
    uint16 sd_banks;
    uint16 min_sd_banks;
    uint16 cache_banks;
    uint16 free_banks;
    unsigned patch_end, num_banks_crossed;

    patch_fn_shared(heap_alloc_pm_init);

    /* Calculate how many banks should be shutdown below the secondary
       processor cache. This can be zero. */
    sd_banks = READ_MIB_NUMBER_PM_SD_BANKS();
    min_sd_banks = hal_pm_get_min_powered_off_banks();
    if (sd_banks < min_sd_banks)
    {
        sd_banks = min_sd_banks;
    }
    pm_config->sd_configured = (sd_banks != 0);
    if (pm_config->sd_configured)
    {
        pm_config->banks_info[SD_BANKS] = sd_banks;
    }

    /* Find out which global mode the chip will be using. */
    pm_config->thread_offload_active = heap_pm_audio_thread_offload_is_configured();
    pm_config->multi_core_active = proc_multiple_cores_present();
    pm_config->p1_cache_active = pm_config->multi_core_active |
                                 pm_config->thread_offload_active;

    /* Calculate how many complete or partial banks are available for heaps. */
    cache_banks = pm_cache_num_p0_cache_banks();
    if (pm_config->p1_cache_active)
    {
        /* The cache does not necessarily start at the beginning of a bank,
           this will allow the last heap to grab as much of the free space
           in the first bank used by P1 cache. */
        cache_banks += (uint16) (pm_cache_num_p1_cache_banks() - 1);
    }
    free_banks = NUMBER_PM_BANKS;
    free_banks -= cache_banks;
    if (sd_banks > free_banks)
    {
        panic_diatribe(PANIC_AUDIO_HEAP_CONFIGURATION_INVALID, free_banks);
    }
    free_banks -= sd_banks;

    /* Calculate how many complete or partial banks are available for
       the secondary processor. This can be zero. */
    pm_config->banks_info[P1_BANKS] = READ_MIB_NUMBER_PM_P1_BANKS();
    if (pm_config->banks_info[P1_BANKS] > free_banks)
    {
        panic_diatribe(PANIC_AUDIO_HEAP_CONFIGURATION_INVALID, free_banks);
    }
    if (!pm_config->multi_core_active)
    {
        pm_config->banks_info[P1_BANKS] = 0;
    }

    /* Calculate how many complete or partial banks are available for
       the primary processor. This can be zero. */
    pm_config->banks_info[P0_BANKS] = free_banks;
    pm_config->banks_info[P0_BANKS] -= pm_config->banks_info[P1_BANKS];
    /* P0 bank numbers need to corrected if it is built with direct-mapped cache, 
     * or 2-way cache is enabled and we stick with the default address.
     */
    if (!pm_cache_update_end_addr(&patch_end))
    {
        /* Get the end address of the patch */
        patch_end = GET_LINKER_ADDRESS(_pm_heap_start_addr) + pm_reserved_size;
        /* Get the number of the bank in which the patch ends */
        num_banks_crossed  = ROUND_DOWN_TO_BANK_BOUNDARY(patch_end)/ PM_BANK_SIZE;
        /* Subtract the number of bank in which the patch starts to get the difference or
         * the number of banks crossed by patch. 
         */
        num_banks_crossed -= ROUND_DOWN_TO_BANK_BOUNDARY(GET_LINKER_ADDRESS(_pm_heap_start_addr))/ PM_BANK_SIZE;
        pm_config->banks_info[(PM_BANK_TYPES) PM_HEAP_P0] -= (uint16) num_banks_crossed;
    }
}

#if defined(PM_BANKS_CAN_BE_POWERED_OFF)
static inline bool heap_starts_powered_on(PM_HEAP heap)
{
    return (heap == PM_HEAP_P0);
}
#else
#define heap_starts_powered_on(x) TRUE
#endif /* defined(PM_BANKS_CAN_BE_POWERED_OFF) */


#ifdef INSTALL_SWITCHABLE_CACHE

/**
 * \brief Arrange P0 heap to avoid static code and patches, if necessary.
 *
 * \note When P0 DM cache is requested, arrange PM heap to use all of the 
 *       available RAM but avoid the static code and patches. This is done 
 *       using the normal PM heap allocator, to avoid manipulating the node 
 *       structures directly. 
 *       1. Allocate a block corresponding to all of the heap above the code 
 *       and patches.
 *       2. Allocate a smaller block to cover the code and patches. This will 
 *       naturally fit immediately below the previous block.
 *       3. Free the block allocated in 1. above.
 */
static void update_heap_for_p0_cache(void)
{
    unsigned cache_end_addr;

    patch_fn_shared(heap_alloc_pm_init);

    if (pm_cache_update_end_addr(&cache_end_addr))
    {
        void_func_ptr main_block, used_block;
        unsigned main_block_start = INTO_DM(GET_LINKER_ADDRESS(_pm_heap_start_addr) + pm_reserved_size);
        unsigned main_block_end = (uintptr_t)(pm_heap_block[PM_HEAP_P0].end_addr);
        unsigned main_block_size = main_block_end - main_block_start - sizeof(pm_mem_node);

        unsigned used_block_start = INTO_DM(PM_RAM_START_ADDRESS + PM_RAM_2WAY_CACHE_BANK_SIZE_WORDS * ADDR_PER_WORD);
        unsigned used_block_end = main_block_start;
        unsigned used_block_size = used_block_end - used_block_start;

        L2_DBG_MSG4("update_heap_for_p0_cache, main block start = %08X end = %08X, used block start = %08X end = %08X", 
            main_block_start, main_block_end,
            used_block_start, used_block_end);

        hal_window_open(HAL_WINDOWS_PROGRAM_MEMORY);
        /* 1. Allocate the main part of the heap. It gets freed again below */
        main_block = heap_alloc_internal_pm(main_block_size, PM_HEAP_P0);
        /* 2. Allocate the "used" part, covering static PM and patches */
        used_block = heap_alloc_internal_pm(used_block_size, PM_HEAP_P0);
        hal_window_close(HAL_WINDOWS_PROGRAM_MEMORY);

        /* Make sure the allocations ended up where we expected */
        PL_ASSERT(INTO_DM((uintptr_t)main_block) == main_block_start + sizeof(pm_mem_node));
        PL_ASSERT(INTO_DM((uintptr_t)used_block) == used_block_start);

        /* 3. Now free the block allocated in 1. above */
        heap_free_pm(main_block);
    }
}

#else
#define update_heap_for_p0_cache() (void)0
#endif

/**
 * \brief Configure the regular heaps.
 *
 * Calculate the limits of the heaps based on the bank configuration,
 * create a node in them and link them into the lists of free nodes.
 *
 * \param pm_config[in] The bank configuration extracted from the MIBs.
 *
 * \return The index of the last usable regular heap.
 */
static PM_HEAP configure_regular_pm_heaps(const PM_BANKS_CONFIG *pm_config)
{
    PM_HEAP heap, last_heap;
    unsigned start = 0;

    patch_fn_shared(heap_alloc_pm_init);

    if (!pm_cache_update_end_addr(&start))
    {
        /* The start of the region of Program Memory usable by the heap
           is provided by the linker. */

        start = GET_LINKER_ADDRESS(_pm_heap_start_addr) + pm_reserved_size;
    }

    /* There is one regular heap per core. */
    last_heap = (PM_HEAP) get_last_proc(pm_config);
    for (heap = PM_HEAP_P0; heap <= last_heap; heap++)
    {
        PM_BLOCK_LIMITS *limits;
        unsigned end;

        /* Calculate the end of the heap taking into account the desired
           number of banks. Align the end to the beginning of the next
           bank. */
        end = start;
        if (pm_config->banks_info[(PM_BANK_TYPES) heap] != 0)
        {
            end += PM_BANK_SIZE * pm_config->banks_info[(PM_BANK_TYPES) heap];
            end = ROUND_UP_TO_BANK_BOUNDARY(end);
        }

        /* Clamp the end of the heap... */
        if ((pm_config->p1_cache_active) &&
                 (end > pm_cache_p1_cache_start()))
        {
            /* ... to the start of the upper cache if it is enabled. */
            end = pm_cache_p1_cache_start();
        }
        else if (end > PM_RAM_START_ADDRESS + HAL_PM_BANK_NUMBER * PM_BANK_SIZE)
        {
            /* ... to the end of the Program Memory address space. */
            end = PM_RAM_START_ADDRESS + HAL_PM_BANK_NUMBER * PM_BANK_SIZE;
        }

        limits = &pm_heap_block[heap];
        limits->offset = INTO_DM(0);

        /* If the heap has some reserved space for it... */
        if (start != end)
        {
            /* ... store the limits of the heap. */
            limits->start_addr = (void *)(uintptr_t) (start + limits->offset);
            limits->end_addr = (void *)(uintptr_t) (end +
                                                    limits->offset -
                                                    PM_RAM_PREFETCH_GUARD);

            if (heap_starts_powered_on(heap))
            {
                /* Create a memory node at the beginning of it. */
                init_and_adjust_pm_heap(limits, NULL);

                /* And finally make the list of free nodes for
                   that heap point to it. */
                freelist_pm[heap] = (pm_mem_node *) limits->start_addr;
            }
        }
        
        start = end;
    }

    update_heap_for_p0_cache();

    return last_heap;
}

#if defined(SUPPORTS_MULTI_CORE) && \
    defined(HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK)
/**
 * \brief Configure the additional heap.
 *
 * Calculate the limits of the additional heap based on the bank configuration,
 * create a node in them and link them into the lists of free nodes.
 *
 * \param pm_config[in] The bank configuration extracted from the MIBs.
 * \param heap[in]      The index of the last usable regular heap.
 */
static void configure_addl_pm_heap(const PM_BANKS_CONFIG *pm_config,
                                   PM_HEAP heap)
{
    patch_fn_shared(heap_alloc_pm_init);

    if (pm_config->p1_cache_active)
    {
        PM_BLOCK_LIMITS *limits;
        unsigned start;

        limits = &pm_heap_block[PM_BLOCK_ADDL];
        start = PM_RAM_P1_CACHE_START_ADDRESS;
        start += PM_RAM_CACHE_BANK_SIZE_WORDS * ADDR_PER_WORD;
        limits->offset = INTO_DM(0);
        limits->start_addr = (void *)(uintptr_t) (start + limits->offset);
        limits->end_addr = (void *)(uintptr_t) (PM_RAM_END_ADDRESS + limits->offset);
        init_and_adjust_pm_heap(limits, freelist_pm[heap]);
    }
}
#else
#define configure_addl_pm_heap(x, y) NOT_USED(y)
#endif /* defined(SUPPORTS_MULTI_CORE) && defined(HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK) */

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
/**
 * \brief Configure a slow heap.
 *
 * Queries the pmalloc component to calculate the limits of a slow heap.
 * Creates a node in them and link them into the lists of free nodes.
 *
 * \param heap Either PM_HEAP_SLOW_P0 or PM_HEAP_SLOW_P1.
 *
 * \note If the heap is already configured, the function has no effect.
 */
static bool configure_slow_pm_heap(PM_HEAP heap)
{
    PM_BLOCK pm_block;
    PM_BLOCK_LIMITS *limits;

    patch_fn_shared(heap_alloc_pm_init);

    /* Check if the heap has been configured.
       The first DM bank can never be used to store instructions
       so a start address of 0 means not configured. */
    pm_block = get_default_block_from_heap(heap);
    limits = &pm_heap_block[pm_block];
    if (limits->start_addr == NULL)
    {
        DM_WINDOW dm_window;

        /* Power on the underlying reserved banks. */
        if (!heap_acquire_dm_window(&dm_window))
        {
            return FALSE;
        }

        /* Record the boundaries. */
        limits->start_addr = dm_window.start;
        limits->end_addr = dm_window.end - PM_RAM_PREFETCH_GUARD;
        limits->offset = 0;
        limits->offset -= HAL_WINDOWS_DATA_MEMORY_INDEX * DM_NVMEM_WINDOW_SIZE;

        /* Create a free node. */
        init_and_adjust_pm_heap(limits, NULL);
        freelist_pm[heap] = (pm_mem_node *) limits->start_addr;
    }

    return TRUE;
}
#else
#define configure_slow_pm_heap(x) TRUE
#endif /* defined(CHIP_HAS_NVRAM_ACCESS_TO_DM) */

/**
 * \brief Set the permission of the Program Memory banks based on the bank
 *        configuration.
 *
 * \param p1_cache_active True if the cache for the secondary processor is enabled.
 */
static void set_pm_memory_bank_permissions(bool p1_cache_active,
                                           bool p1_heap_active)
{
    HAL_PM_BANK bank;
    unsigned address;

    patch_fn_shared(heap_alloc_pm_init);

    address = PM_RAM_WINDOW;
    for (bank = 0; bank < HAL_PM_BANK_NUMBER; bank++)
    {
        HAL_PM_OWNER owner;

        if (address < INTO_DM(PM_RAM_START_ADDRESS + pm_cache_size_p0_words() * ADDR_PER_WORD))
        {
            /* The bank contains some of the cache for the primary processor.
               It might also contain some heap data, functions statically
               stashed in Program Memory and patches. */
            owner = HAL_PM_OWNER_CACHE;
        }
        else if (p1_cache_active &&
                 (address >= INTO_DM(ROUND_DOWN_TO_BANK_BOUNDARY(pm_cache_p1_cache_start()))))
        {
            /* The bank contains some of the cache for the secondary processor
               and the cache is enabled. It might also contain some heap data. */
            owner = HAL_PM_OWNER_CACHE;
        }
        else if (address < (uintptr_t) pm_heap_block[0].start_addr)
        {
            /* The bank contains functions statically stashed in Program Memory
               and patches. */
            owner = HAL_PM_OWNER_PROCESSOR_0;
        }
        else if ((address >= (uintptr_t) pm_heap_block[0].start_addr) &&
                 (address < (uintptr_t) pm_heap_block[0].end_addr))
        {
            /* The bank contains some of the heap for the primary processor. */
            owner = HAL_PM_OWNER_PROCESSOR_0;
        }
#if defined(SUPPORTS_MULTI_CORE)
        else if ((address >= (uintptr_t) pm_heap_block[1].start_addr) &&
                 (address < (uintptr_t) pm_heap_block[1].end_addr))
        {
            /* The bank contains some of the heap for the secondary processor. */
            if (p1_heap_active)
            {
                owner = HAL_PM_OWNER_PROCESSOR_1;
            }
            else
            {
                owner = HAL_PM_OWNER_INVALID;
            }
        }
#endif
        else
        {
            /* The bank contains nothing and should be powered off. */
            owner = HAL_PM_OWNER_INVALID;
        }
        hal_pm_configure_arbiter(bank, owner);
        address += PM_BANK_SIZE;
    }
}

static void_func_ptr heap_alloc_internal_pm(unsigned size_byte, PM_HEAP heap)
{
    unsigned bestsize_32;
    pm_mem_node *bestnode_prev;
    pm_mem_node *prevnode;
    pm_mem_node *curnode;
    pm_mem_node *bestnode;
    pm_mem_node **pfreelist;

    patch_fn(heap_alloc_pm_heap_alloc_internal_pm);

    /* Assume that the additional block can never be the largest block. */
    bestsize_32 = (uintptr_t) pm_heap_block[(PM_BLOCK) heap].end_addr;
    bestsize_32 -= (uintptr_t) pm_heap_block[(PM_BLOCK) heap].start_addr;
    bestsize_32 /= PC_PER_INSTRUCTION;
    pfreelist = &freelist_pm[heap];

    /* Traverse the list looking for the best-fit free block. The best fit is
       the smallest one of at least the requested size. This will help minimise
       wastage. */
    bestnode = NULL;
    prevnode = NULL;
    bestnode_prev = NULL;
    curnode = *pfreelist;
    while (curnode != NULL)
    {
        if ((curnode->length_32 >= BYTES_INTO_PM_32(size_byte)) &&
            (curnode->length_32 < bestsize_32))
        {
            /* Save the pointer that pointed to the best node,
               if it was the first one in DM, this would be NULL. */
            bestnode = curnode;
            bestnode_prev = prevnode;
            bestsize_32 = curnode->length_32;
        }
        /* Save the node (PM Address) that will point to next one */
        prevnode = curnode;
        curnode = curnode->u.next;
    }

    if (bestnode != NULL)
    {
        uint32 requested_size;

        requested_size = sizeof(pm_mem_node);
        requested_size += size_byte;
        requested_size /= PC_PER_INSTRUCTION;

        if (bestsize_32 >= requested_size + MIN_SPARE_32)
        {
            /* There's enough space to allocate something else so keep the
               existing free block and allocate the space at the top. In
               this case the allocation size is exactly what was requested. */
            pm_mem_node *old_node;
            uintptr_t tmp;

            old_node = bestnode;

            tmp = (uintptr_t) bestnode;
            tmp += bestnode->length_32 * PC_PER_INSTRUCTION;
            tmp -= size_byte;
            bestnode = (pm_mem_node *) tmp;

            old_node->length_32 -= requested_size;
        }
        else
        {
            /* Not enough extra space to be useful. Replace the free block with
               an allocated one. The allocation size is the whole free block. */
            size_byte = bestnode->length_32 * PC_PER_INSTRUCTION;

            /* Update pointer that pointed to the best node */
            if (bestnode_prev != NULL)
            {
                bestnode_prev->u.next = bestnode->u.next;
            }
            else
            {
                /* This node was pointed to by an address in DM, update it */
                *pfreelist = bestnode->u.next;
            }
        }
        /* Finally populate the header for the newly-allocated block */
        bestnode->length_32 = BYTES_INTO_PM_32(size_byte);
        bestnode->u.magic = MAGIC_WORD;
        return (void_func_ptr) ((uintptr_t) bestnode +
                                sizeof(pm_mem_node) -
                                pm_heap_block[(PM_BLOCK) heap].offset);
    }
    /* No suitable block found */
    return NULL;
}

/**
 * \brief Merge 2 contiguous nodes.
 */
static inline void heap_merge_nodes(pm_mem_node *target, const pm_mem_node *source)
{
    target->length_32 += source->length_32;
    target->length_32 += SIZE_OF_DM_VAR_IN_PM_32(pm_mem_node);
    target->u.next = source->u.next;
}

/**
 * \brief Link 2 nodes separated by a gap.
 */
static inline void heap_link_nodes(pm_mem_node *target, pm_mem_node *source)
{
    target->u.next = source;
}

/**
 * \brief Free a previously-allocated block
 */
static void heap_free_internal_pm(pm_mem_node *ptr, pm_mem_node **pfreelist)
{
    pm_mem_node *cur_node;
    pm_mem_node *nodes[NODE_TOTAL];
    unsigned i;

    patch_fn(heap_alloc_pm_heap_free_pm);

    /* Search for the closest nodes before and after the node being freed. */
    cur_node = *pfreelist;
    nodes[NODE_PREV] = NULL;
    nodes[NODE_FREED] = ptr;
    nodes[NODE_NEXT] = NULL;
    while (cur_node != NULL)
    {
        if (cur_node < nodes[NODE_FREED])
        {
            nodes[NODE_PREV] = cur_node;
        }
        else if (cur_node > nodes[NODE_FREED])
        {
            nodes[NODE_NEXT] = cur_node;
            break;
        }
        cur_node = cur_node->u.next;
    }

    /* Link or merge the nodes before and after the node being freed if
       they exist. */
    nodes[NODE_FREED]->u.next = NULL;
    for (i = NODE_NEXT; i > NODE_PREV; i--)
    {
        if ((nodes[i - 1] != NULL) && (nodes[i] != NULL))
        {
            uint32 end;

            end = (uintptr_t) nodes[i - 1];
            end += sizeof(pm_mem_node);
            end += nodes[i - 1]->length_32 * PC_PER_INSTRUCTION;

            if (end == (uintptr_t) nodes[i])
            {
                heap_merge_nodes(nodes[i - 1], nodes[i]);
            }
            else
            {
                heap_link_nodes(nodes[i - 1], nodes[i]);
            }
        }
    }

    /* If there is no node before the node being freed, then the node
       being freed must be pointed by the head of free list. */
    if (nodes[NODE_PREV] == NULL)
    {
       *pfreelist = nodes[NODE_FREED];
    }
}

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Initialise the offset used to calculate the heap start address
 *
 * \param offset The new value to be used
 *
 * \note The offset address has to be set before PM heap is initialised
 */
void heap_pm_init_start_offset(unsigned offset)
{
    /* Should never be called after heap PM is initialised */
    PL_ASSERT(NULL == pm_heap_block[PM_HEAP_P0].start_addr);
    pm_reserved_size = ROUND_UP_TO_WHOLE_WORDS(offset);
    return;
}

/**
 * \brief Initialise the memory heap
 */
void init_heap_pm(void)
{
    PM_BANKS_CONFIG pm_config;
    PM_HEAP heap;
    bool p1_heap_active;
    bool p1_cache_active;

    patch_fn_shared(heap_alloc_pm_init);

    heap_pm_create_descriptor(&pm_config);
    heap = configure_regular_pm_heaps(&pm_config);
    configure_addl_pm_heap(&pm_config, heap);

    p1_cache_active = pm_config.p1_cache_active;
    p1_heap_active = pm_config.p1_cache_active;
#if defined(PM_BANKS_CAN_BE_POWERED_OFF)
    p1_heap_active = FALSE;
#endif
    set_pm_memory_bank_permissions(p1_cache_active, p1_heap_active);

    hal_pm_update_banks_power();
}

#if defined(SUPPORTS_MULTI_CORE)
/**
 * \brief Reclaim P1's memory heap as P0's heap
 */
void pm_disable_secondary_processor(void)
{
    PM_BLOCK_LIMITS p1_heap_block;
    unsigned sd_banks;
    void *p1_node;
    unsigned heap_end;
    uint32 offset;

    patch_fn_shared(heap_alloc_pm_init);

    /* The secondary processor already had no heap so there is nothing to do.
       This is a valid case as an application might want to reclaim some Data
       memory but never has the need to use Program Memory on the second 
       core. */
    if (pm_heap_block[PM_BLOCK_P1].start_addr == NULL)
    {
        return;
    }

    /* The offset is identical for all blocks in Program Memory. */
    offset = pm_heap_block[PM_BLOCK_P0].offset;

    /* Calculate the end of the remaining heap starting by
       the end of addressable memory in PM space then removing
       the banks powered off by configuration and finally translating
       into DM space. */
    heap_end = PM_RAM_END_ADDRESS;
    sd_banks = READ_MIB_NUMBER_PM_SD_BANKS();
    if (sd_banks != 0)
    {
        heap_end -= sd_banks * PM_BANK_SIZE;
        heap_end -= PM_RAM_PREFETCH_GUARD;
    }
    heap_end += offset;

    /* Clear the boundaries of the disabled heap and
       clear the pointer to the free list. */
    memcpy(&p1_heap_block, &pm_heap_block[PM_BLOCK_P1], sizeof(PM_BLOCK_LIMITS));
    memset(&pm_heap_block[PM_BLOCK_P1], 0, sizeof(PM_BLOCK_LIMITS));
#ifdef HAVE_USABLE_PM_ABOVE_P1_CACHE_BANK
    memset(&pm_heap_block[PM_BLOCK_ADDL], 0, sizeof(PM_BLOCK_LIMITS));
#endif
    freelist_pm[PM_HEAP_P1] = NULL;

    /* Grow the boundaries of the remaining heap and
       update the bank permissions. */
    pm_heap_block[PM_BLOCK_P0].end_addr = (void *)(uintptr_t) heap_end;
    set_pm_memory_bank_permissions(FALSE, FALSE);
    hal_pm_update_banks_power();

    /* Make a fake allocated node occupying all the space previously
       reserved by P1. */
    p1_heap_block.end_addr = (void *)(uintptr_t) heap_end;
    init_and_adjust_pm_heap(&p1_heap_block, (void *) MAGIC_WORD);

    /* And then add it to the list of free nodes. */
    p1_node = p1_heap_block.start_addr;
    heap_free_pm((void_func_ptr) ((uintptr_t)p1_node - offset + sizeof(pm_mem_node)));
}

void pm_start_secondary_processor(void)
{
    PM_BLOCK_LIMITS *limits;

    patch_fn_shared(heap_alloc_pm_init);

    /* If the heap was powered on during boot then there is no need
       to power it on now. */
    if (heap_starts_powered_on(PM_HEAP_P1))
    {
        return;
    }

    /* Ensure that the banks for the heap and cache of the
       secondary processor be accessed. */
    set_pm_memory_bank_permissions(TRUE, TRUE);

    /* Power on the banks allocated to its heap. */
    hal_pm_update_banks_power();

    /* If there is no memory reserved for the secondary processor,
       then go no further. */
    limits = &pm_heap_block[PM_BLOCK_P1];
    if (limits->start_addr == NULL)
    {
        return;
    }

    /* Create a memory node at the beginning of it. */
    init_and_adjust_pm_heap(limits, NULL);

    /* And finally make the list of free nodes for
       that heap point to it. */
    freelist_pm[PM_HEAP_P1] = (pm_mem_node *) limits->start_addr;
}

void pm_stop_secondary_processor(void)
{
    patch_fn_shared(heap_alloc_pm_init);

    /* If the heap was powered on during boot then there is no need
       to power it on now. */
    if (heap_starts_powered_on(PM_HEAP_P1))
    {
        return;
    }

    /* Ensure that the banks for the heap and cache of the
       secondary processor cannot be accessed. */
    set_pm_memory_bank_permissions(FALSE, FALSE);

    /* Power off the banks was were allocated to its heap and its cache. */
    hal_pm_update_banks_power();

    /* And finally clear the list of free nodes. */
    freelist_pm[PM_HEAP_P1] = NULL;
}
#endif

/**
 * \brief Allocate a block of (at least) the requested size
 */
void_func_ptr heap_alloc_pm(unsigned size_byte, unsigned preference_core)
{
    void_func_ptr addr;
    PM_HEAP heap;

    patch_fn_shared(heap_alloc_pm);

    if ((size_byte % PC_PER_INSTRUCTION) != 0)
    {
        /* Make sure we always try to allocate a 32bit-aligned amount of
           PM memory, even on 8-bit addressable PMs. */
        size_byte += PC_PER_INSTRUCTION - (size_byte % PC_PER_INSTRUCTION);
    }

    switch (preference_core)
    {
#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
        case MALLOC_PM_PREFERENCE_SLOW:
        {
            if (PROC_PRIMARY_CONTEXT())
            {
                heap = PM_HEAP_SLOW_P0;
            }
            else
            {
                heap = PM_HEAP_SLOW_P1;
            }
            break;
        }
#endif /* defined(CHIP_HAS_NVRAM_ACCESS_TO_DM) */
#if defined(SUPPORTS_MULTI_CORE)
        case MALLOC_PM_PREFERENCE_CORE_1:
        {
            heap = PM_HEAP_P1;
            break;
        }
#endif
        case MALLOC_PM_PREFERENCE_CORE_0:
        default:
        {
            heap = PM_HEAP_P0;
            break;
        }
    }

    if (heap_is_slow(heap))
    {
        if (!configure_slow_pm_heap(heap))
        {
            return NULL;
        }

        addr = heap_alloc_internal_pm(size_byte, heap);
    }
    else
    {
        /* Do all the list-traversal and update with interrupts blocked */
        LOCK_INTERRUPTS;
        hal_window_open(HAL_WINDOWS_PROGRAM_MEMORY);

        addr = heap_alloc_internal_pm(size_byte, heap);

        /* Restore initial state */
        hal_window_close(HAL_WINDOWS_PROGRAM_MEMORY);
        UNLOCK_INTERRUPTS;
    }

    return addr;
}

/**
 * \brief Free a previously-allocated block
 */
void heap_free_pm(void_func_ptr ptr)
{
    PM_HEAP heap;
    pm_mem_node *node;

    patch_fn_shared(heap_alloc_pm);

    /* free(NULL) is a no-op  */
    if (ptr == NULL)
    {
        return;
    }

    PL_PRINT_P1(TR_PL_FREE, "PM ptr to be freed %p..", ptr);

    node = NULL;
    heap = get_heap_from_ptr(ptr);
    if (heap == PM_HEAP_INVALID)
    {
        PL_PRINT_P0(TR_PL_FREE, "Couldn't find in any PM heap\n");
        panic_diatribe(PANIC_AUDIO_FREE_INVALID, (uintptr_t) ptr);
    }
    else
    {
        uintptr_t base;

        PL_PRINT_P1(TR_PL_FREE, "is in PM heap %u\n", heap);
        base = (uintptr_t) ptr;
        base += (uintptr_t) pm_heap_block[heap].offset;
        base -= sizeof(pm_mem_node);
        node = (pm_mem_node *) base;
    }

    LOCK_INTERRUPTS;
    if (!heap_is_slow(heap))
    {
        /* Enable access to PM through DM window */
        hal_window_open(HAL_WINDOWS_PROGRAM_MEMORY);
    }

    /* Check that the address being freed looks sensible */
    if (node->u.magic != MAGIC_WORD)
    {
        panic_diatribe(PANIC_AUDIO_FREE_INVALID, (uintptr_t) ptr);
    }
    else
    {
        /* Check that the length seems plausible */
        uintptr_t end;

        end = (uintptr_t) ptr;
        end += (node->length_32 - 1) * PC_PER_INSTRUCTION;
        if (!is_in_pm_heap((void_func_ptr) end))
        {
            panic_diatribe(PANIC_AUDIO_FREE_INVALID, (uintptr_t) ptr);
        }
    }

    heap_free_internal_pm(node, &freelist_pm[heap]);

    if (!heap_is_slow(heap))
    {
        /* Restore initial state */
        hal_window_close(HAL_WINDOWS_PROGRAM_MEMORY);
    }
    else if (freelist_pm[heap] != NULL)
    {
        /* If having freeing the node, there is a single node remaining
           in the free list then the heap can be powered down. */
        if (freelist_pm[heap]->u.next == NULL)
        {
            PM_BLOCK pm_block;
            PM_BLOCK_LIMITS *limits;

            heap_release_dm_window();
            pm_block = get_default_block_from_heap(heap);
            limits = &pm_heap_block[pm_block];
            memset(limits, 0, sizeof(PM_BLOCK_LIMITS));
            freelist_pm[heap] = NULL;
        }
    }

    UNLOCK_INTERRUPTS;
}

bool is_in_pm_heap(void_func_ptr ptr)
{
    patch_fn_shared(heap_alloc_pm);
    return get_heap_from_ptr(ptr) != PM_HEAP_INVALID;
}

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
bool is_in_slow_pm_heap(void_func_ptr ptr)
{
    patch_fn_shared(heap_alloc_pm);
    PM_HEAP pm_heap;

    pm_heap = get_heap_from_ptr(ptr);
    return ((pm_heap == PM_HEAP_SLOW_P0) ||
            (pm_heap == PM_HEAP_SLOW_P1));
}

bool is_a_patch_address(unsigned address)
{
    patch_fn_shared(heap_alloc_pm);

    unsigned patch_start;
    unsigned patch_end;

    patch_start = GET_LINKER_ADDRESS(_pm_heap_start_addr);
    patch_end = patch_start + pm_reserved_size;

    if (address >= patch_start && address < patch_end)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
#endif

/**
 * \brief Get a pointer to a piece of program memory accessible
 *        via the data bus.
 *
 * \param ptr A pointer returned by function xpmalloc_pm.
 *
 * \note The pointer remains valid until function malloc_pm_close_window
 *       is called.
 */
uint32 *malloc_pm_open_window(void_func_ptr ptr)
{
    PM_HEAP heap;
    uintptr_t result;

    patch_fn_shared(heap_alloc_pm);

    heap = get_heap_from_ptr(ptr);
    if (!heap_is_slow(heap))
    {
        /* Enable access to PM through DM window */
        hal_window_open(HAL_WINDOWS_PROGRAM_MEMORY);

    }
    else
    {
        /* The window is opened just before the first allocation. */
    }

    /* Convert address to a manageable number in PM address space (32 bit). */
    result = (uintptr_t) ptr;
    result += pm_heap_block[(PM_BLOCK) heap].offset;

    return (uint32 *) result;
}

/**
 * \brief Return ownership of a piece of program memory to the
 *        instruction bus.
 *
 * \param ptr A pointer returned by function xpmalloc_pm.
 */
void malloc_pm_close_window(void_func_ptr ptr)
{
    PM_HEAP heap;

    patch_fn_shared(heap_alloc_pm);

    heap = get_heap_from_ptr(ptr);
    if (!heap_is_slow(heap))
    {
        /* Disable access to PM through DM window */
        hal_window_close(HAL_WINDOWS_PROGRAM_MEMORY);

    }
    else
    {
        /* The window was closed when the last allocation was freed. */
    }
}

