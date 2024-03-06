/****************************************************************************
 * Copyright (c) 2012 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file heap_alloc_kcc.c
 * \ingroup pl_malloc
 *
 * Kalimba specific heap memory management.
 *
 * The audio subsystem contains a number of physical memory banks which can
 * be powered on and off independently and are paired with a simple memory
 * protection unit. The number of memory banks vary per program and even per
 * chip. Search for "Product binning" for the rationale for this behaviour.
 * For each program, the absolute maximum number of banks is NUMBER_DM_BANKS
 * but some banks might be unavailable on some lower cost chips so
 * hal_dm_get_number_banks() has to be used instead.
 *
 * The banks are grouped into regions by the linker. Chips have at least
 * 2 regions (the shared region and the main region). Some chips also have
 * a slow region which as the name indicates runs slower than regular banks.
 * Similarly some chips also have a special external region that does not
 * use the physical memory banks but a memory chip connected via SQIF instead.
 *
 * Each region is then further divided into heaps. Each region has at least
 * one default heap and might have more optional heaps. The heaps must end
 * on a bank boundary to make it possible to power them off but they can
 * start at an arbitrary place in a bank. Only the shared heap makes use of
 * this degree of freedom and cannot be powered off.
 *
 * \verbatim
 *                  CSRA68100
 * +-------------------+  +-------------------+
 * |                   |  |    P1 Main Heap   |
 * +    Main Region    +  +-------------------+
 * |                   |  |    P0 Main Heap   |
 * +-------------------+  +-------------------+
 * |                   |  |    P1 Slow Heap   |
 * +    Slow Region    +  +-------------------+
 * |                   |  |    P0 Slow Heap   |
 * +-------------------+  +-------------------+
 * |                   |  |   P1 Shared Heap  |
 * +   Shared Region   +  +-------------------+
 * |                   |  |   P0 Shared Heap  |
 * +-------------------+  +-------------------+
 *
 *                   QCC517x
 * +-------------------+  +-------------------+
 * |                   |  |   P1 NVRAM Heap   |
 * +                   +  +-------------------+
 * |                   |  |   P0 NVRAM Heap   |
 * +                   +  +-------------------+
 * |                   |  |   P1 Extra Heap   |
 * +    Main Region    +  +-------------------+
 * |                   |  |   P0 Extra Heap   |
 * +                   +  +-------------------+
 * |                   |  |    P1 Main Heap   |
 * +                   +  +-------------------+
 * |                   |  |    P0 Main Heap   |
 * +-------------------+  +-------------------+
 * |   Shared Region   |  |    Shared Heap    |
 * +-------------------+  +-------------------+
 * \endverbatim
 *
 * Each heap is itself subdivided by the number of cores except for the
 * shared heap that be common between cores. This is because the goal is to
 * avoid contention between cores for the access of the banks. The shared heap
 * however is meant to exchange data between cores so contention is unavoidable
 * and it is better to save memory in that case.
 *
 * When starting the audio subsystem, there is no connection to the other
 * subsystems and therefore the per-product configuration (MIB) is not available
 * but the heaps are needed to download said configuration. So early during
 * the boot sequence, function "heap_config_static" configures the default
 * heaps to occupy the entire region they belong to. Once the per-product
 * configuration is available, the boundaries of all the heaps are recalculated
 * once again by function "heap_config_dynamic" and its subfunctions.
 *
 * During the reconfiguration, first function "get_heap_allocation" is called
 * to extracts the size of each heap either from the MIB or a stub version of
 * function "get_mib_value" which needs to be adopted by program. Then the
 * values are checked against configuration errors: each heap size must be
 * a multiple of the size of a bank and the default heaps must contain no
 * less than one bank. Further on, the space reserved for the heaps of the
 * secondary cores is given to the heaps of the primary core if only the
 * primary core is running. Finally function "heap_configure_and_align" stacks
 * the heaps taking the heap sizes as input.
 *
 * With the heap boundaries available, the file used again to set the banks
 * permissions and power off the ones that are unused. This is done by function
 * "heap_config_hardware" and its subfunctions "set_dm_memory_bank_permissions"
 * and "hal_dm_update_banks_power". Setting the banks permissions is
 * straightforward given the heaps boundaries except for the exception of 
 * the case of the Thread Offload mode where everything is shared between cores
 * and therefore contention between cores is expected. Unused banks are
 * assigned to the "invalid" core making it easier to identify and power off
 * later on.
 *
 * When a core is started or stopped, the main heap that belong to it can be
 * powered on and powered off respectively. This is done by function
 * "heap_change_power". The extra heaps and the nvmem heaps can be powered on
 * and off on first allocation and last free operation respectively. Powering
 * a heap on or off takes about 300 us and interrupts are blocked during that
 * period.
 */

/****************************************************************************
Include Files
*/

#include "pmalloc/pl_malloc_private.h"

/****************************************************************************
Private Macro Definitions
*/

#define LINKER_ADDRESS extern unsigned
#define LINKER_SIZE extern unsigned

/*
 * Directly accessing the linker provided symbol
 * as an address.
 */
#define GET_LINKER_ADDRESS(x) ((char*)(uintptr_t)&(x))

/*
 * Directly accessing the linker provided symbol
 * as an address and typecasting to integer to make
 * the compiler happy
 */
#define HEAP_SIZE(x) ((unsigned)(uintptr_t)&(x))

#define KIBYTE 1024

#if defined(INSTALL_THREAD_OFFLOAD)
#define heap_alloc_thread_offload_is_configured()    thread_offload_is_configured()
#else
#define heap_alloc_thread_offload_is_configured()    FALSE
#endif /* INSTALL_THREAD_OFFLOAD */

#define NUM_SHARED_MEM_OPTIONS 6

/****************************************************************************
Private Type Declarations
*/

/**
 * Enumeration of available memory regions.
 */
typedef enum
{
    /** A region of memory that will be split between cores on bank
        boundaries so that there is no access contention between
        cores. */
    REGION_MAIN,

    /** A region of memory starting at the end of static allocations
        and that can be accessed simultaneously by the cores. */
    REGION_SHARED,

#if defined(CHIP_HAS_SLOW_DM_RAM)
    /** A region of memory similar to REGION_MAIN but running at half the
        processor maximum clock frequency. */
    REGION_SLOW,
#endif

#if defined(HAVE_EXTERNAL_HEAP)
    /** A region of memory located outside of the chip and accessed indirectly
        via a special memory window. */
    REGION_EXT,
#endif

    /** Number of memory regions present on the chip. */
    REGION_ARRAY_SIZE,

    /** Maximum number of internal heaps for a processor. */
#if defined(HAVE_EXTERNAL_HEAP)
    REGION_INTERNAL_SIZE = REGION_ARRAY_SIZE - 1,
#else
    REGION_INTERNAL_SIZE = REGION_ARRAY_SIZE,
#endif

#if !defined(HAVE_EXTERNAL_HEAP)
    /** The region does not exist so put it after the array size. */
    REGION_EXT,
#endif
} region_names;

typedef struct
{
    char   *start_addr;
    size_t  size;
} region_boundaries;

/**
 * Type definition for heap allocation mib key values.
 * This must match key "HeapAllocation" from file
 * "common/hydra/mibs/audio_mib.xml".
 */
typedef enum
{
    HEAP_MIB_HEAP_LSB_OFFSET  = 0,
    HEAP_MIB_HEAP_MSB_OFFSET  = 1,
#if defined(CHIP_HAS_SLOW_DM_RAM)
    HEAP_MIB_SLOW_HEAP_LSB_OFFSET = 2,
    HEAP_MIB_SLOW_HEAP_MSB_OFFSET = 3,
#endif
#if defined(DM_BANKS_CAN_BE_POWERED_OFF)
    HEAP_MIB_EXTRA_HEAP_LSB_OFFSET = 2,
    HEAP_MIB_EXTRA_HEAP_MSB_OFFSET = 3,
#endif
#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    HEAP_MIB_NVRAM_HEAP_LSB_OFFSET = 4,
    HEAP_MIB_NVRAM_HEAP_MSB_OFFSET = 5,
#endif
    HEAP_MIB_PER_PROC_SIZE,
    HEAP_MIB_TOTAL_SIZE = PROC_PROCESSOR_MAX * HEAP_MIB_PER_PROC_SIZE,
} HEAP_MIB;

typedef struct
{
    uint8 array[HEAP_MIB_PER_PROC_SIZE];
} HEAP_MIB_PER_PROC_VALUE;

typedef struct
{
    uint8 array[HEAP_MIB_TOTAL_SIZE];
} HEAP_MIB_VALUE;

#if defined(HAVE_EXTERNAL_HEAP)
/**
 * Type definition for heap allocation mib key values.
 */
typedef struct ext_heap_allocation_mib_value
{
    uint8 ext_ram_size_msb;
    uint8 ext_ram_size_lsb;
    uint8 ext_p0_data_heap_size_msb;
    uint8 ext_p0_data_heap_size_lsb;
    uint8 ext_p1_data_heap_size_msb;
    uint8 ext_p1_data_heap_size_lsb;
    uint8 reserved_0;
    uint8 reserved_1;
    uint8 reserved_2;
    uint8 reserved_3;
    uint8 reserved_4;
    uint8 reserved_5;
    uint8 reserved_7;
    uint8 reserved_8;
    /* Currently we only have 1 heap per CPU in SRAM.
     * Ignoring reserved values*/
} ext_heap_allocation_mib_value;

typedef enum
{
    HEAP_EXT_MIB_RAM_SIZE_OFFSET  = 0,
    HEAP_EXT_MIB_HEAPS_OFFSET     = 2,
    HEAP_EXT_MIB_PER_PROC_SIZE    = 2, /* One heap described with 2 octets. */
    HEAP_EXT_MIB_TOTAL_SIZE       = 14,
} HEAP_EXT_MIB;

typedef struct
{
    uint8 array[HEAP_EXT_MIB_PER_PROC_SIZE];
} HEAP_EXT_MIB_PER_PROC_VALUE;

typedef struct
{
    uint8 array[HEAP_EXT_MIB_TOTAL_SIZE];
} HEAP_EXT_MIB_VALUE;
#else
/* Dummy type to prevent compilation from breaking. */
typedef unsigned HEAP_EXT_MIB_VALUE;
#endif /* defined(HAVE_EXTERNAL_HEAP) */

/**
 * Heap sizes for a processor.
 */
typedef struct heap_sizes
{
    unsigned int heap_size[HEAP_ARRAY_SIZE];
} heap_sizes;

/**
 * Dynamic heap configuration for the chip.
 */
typedef struct heap_dyn_size_config
{
    /* Processor */
    heap_sizes p[PROC_PROCESSOR_MAX];
} heap_dyn_size_config;

/****************************************************************************
Private Variable Definitions
*/

/* The linker provides the start address and size of regions of
   memory dedicated to heaps. */
LINKER_ADDRESS _heap_start_addr;
#ifdef CHIP_HAS_SLOW_DM_RAM
LINKER_ADDRESS _slow_heap_start_addr;
LINKER_SIZE _slow_heap_size;
#endif
LINKER_ADDRESS _shared_heap_start_addr;
LINKER_SIZE _shared_heap_size;

/* This information is then gathered in a structure usable
   by the upper layer. */
static region_boundaries region_linker_info[REGION_ARRAY_SIZE] =
{
    {
        GET_LINKER_ADDRESS(_heap_start_addr),
        /* The actual size is calculated in "heap_config_static". */
        0
    },
    {
        GET_LINKER_ADDRESS(_shared_heap_start_addr),
        HEAP_SIZE(_shared_heap_size)
    },
#if defined(CHIP_HAS_SLOW_DM_RAM)
    {
        GET_LINKER_ADDRESS(_slow_heap_start_addr),
        HEAP_SIZE(_slow_heap_size)
    },
#endif
#if defined(HAVE_EXTERNAL_HEAP)
    {
        ((char*)(DM_NVMEM_WINDOW_START + (HAL_WINDOWS_SRAM_WRITE_INDEX * DM_NVMEM_WINDOW_SIZE))),
        DM_NVMEM_WINDOW_SIZE
    },
#endif
};

/* TODO: This can be removed once ACAT has been updated. */
static char *heap_single_mode = GET_LINKER_ADDRESS(_heap_start_addr);
#ifdef CHIP_HAS_SLOW_DM_RAM
static char *slow_heap = NULL;
#endif

/* An array listing the internal heaps in the order in which they are
   laid out in memory. */
static const heap_names ordered_heaps[HEAP_INTERNAL_SIZE] =
{
    HEAP_SHARED,
#ifdef CHIP_HAS_SLOW_DM_RAM
    HEAP_SLOW,
#endif
    HEAP_MAIN,
#ifdef DM_BANKS_CAN_BE_POWERED_OFF
    HEAP_EXTRA,
#endif
#ifdef CHIP_HAS_NVRAM_ACCESS_TO_DM
    HEAP_NVRAM,
#endif
};

/****************************************************************************
Private Function Definitions
*/

#if defined(COMMON_SHARED_HEAP)
/**
 * \brief Find out if a heap reuse the boundaries of the primary core.
 *
 * \param core     The core the heap belongs to.
 * \param heap_num The heap to query.
 *
 * \return TRUE if the boundaries of the heap are common for all cores.
 *         FALSE if the boundaries of the heap are stacked.
 */
static inline bool heap_boundaries_are_reused(PROC_ID_NUM core,
                                              heap_names heap_num)
{
    return (heap_num == HEAP_SHARED) && (core != PROC_PROCESSOR_0);
}
#else
#define heap_boundaries_are_reused(x, y) FALSE
#endif

/**
 * \brief Find the region the specified heap belongs to.
 *
 * \param heap_num The heap to query.
 *
 * \return The region the specified heap belongs to.
 */
static inline region_names get_region_from_heap(heap_names heap_num)
{
#if defined(DM_BANKS_CAN_BE_POWERED_OFF)
    if (heap_num == HEAP_EXTRA)
    {
        return REGION_MAIN;
    }
#endif

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    if (heap_num == HEAP_NVRAM)
    {
        return REGION_MAIN;
    }
#endif

    return (region_names) heap_num;
}

/**
 * \brief Find a specified region's only non optional heap.
 *
 * \param region_num The region to query.
 *
 * \return The specified region's only non optional heap.
 */
static inline heap_names get_region_default_heap(region_names region_num)
{
#if defined(HAVE_EXTERNAL_HEAP)
    if (region_num == REGION_EXT)
    {
        return HEAP_EXT;
    }
#endif

    return (heap_names) region_num;
}

/**
 * \brief Returns the size of a specified region.
 *
 * \param region_num The region to query.
 *
 * \return The size of the specified region in octets.
 */
static inline unsigned get_region_max_size(region_names region_num)
{
    if (region_num >= REGION_ARRAY_SIZE)
    {
        return 0;
    }
    return region_linker_info[region_num].size;
}

#if defined(DM_BANKS_CAN_BE_POWERED_OFF)
/**
 * \brief Find if the specified heap is the default heap in a region.
 *
 * \param region_num The region to query.
 *
 * \return TRUE if the specified heap is the default heap in a region.
 */
static inline bool heap_is_region_default(heap_names heap_num)
{
    if (heap_num == HEAP_EXTRA)
    {
        return FALSE;
    }

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    if (heap_num == HEAP_NVRAM)
    {
        return FALSE;
    }
#endif

    return TRUE;
}
#else
#define heap_is_region_default(x) TRUE
#endif /* defined(DM_BANKS_CAN_BE_POWERED_OFF) */

/**
 * \brief Find the mid point for the given heap
 */
static char *heap_midpoint(const heap_info *pinfo)
{
    return pinfo->heap_start + pinfo->heap_size / 2;
}

/**
 * \brief For the given bank, find the heap that occupies it and return the
 *        preferred core and DM access method for setting arbiter permissions.
 */
static heap_names get_preferred_ownership_for_bank(heap_config *processor_heap_info_list,
                                                   HAL_DM_BANK bank,
                                                   PROC_ID_NUM *core,
                                                   HAL_DM_BUS *pref_dem)
{
    heap_names heap_num;
    PROC_ID_NUM core_index;

    patch_fn_shared(heap);

    /* Loop through the number of cores */
    for (core_index = 0; core_index < PROC_PROCESSOR_BUILD; core_index++)
    {
        heap_config *pconfig = &processor_heap_info_list[core_index];
        /* loop through each heap in the core */
        for (heap_num = 0; heap_num < HEAP_ARRAY_SIZE; heap_num++)
        {
            heap_info *info = &pconfig->heap[heap_num];
            char *bank_midpoint;

            if (!heap_is_region_default(heap_num))
            {
                /* All non default heaps are powered off by default. */
                continue;
            }

            bank_midpoint = (char *)((bank * DM_RAM_BANK_SIZE) +
                                     (DM_RAM_BANK_SIZE / 2));

            /* Check if the mid point of the bank address falls within the heap */
            if ((info->heap_start <= bank_midpoint) &&
                (info->heap_end > bank_midpoint))
            {
                char *bank_end;

                /* If a core has only 1 bank in a given heap, the permissions
                 * will be set so that the core's DM1 access is the default
                 * owner.
                 *
                 * If a core has more than 1 bank, half of the total banks will
                 * be set to be owned by core's DM2 access and the remaining
                 * owned by core's DM1 access.
                 *
                 * If the end address of the DM bank is within the midpoint of
                 * the heap, use DM2 as preferred address space, since the lower
                 * allocations from the heap is usually allocated as DM2 blocks.
                 */
                bank_end = (char *)(((bank+1) * DM_RAM_BANK_SIZE) - 1);
                *core = core_index;
                *pref_dem = (heap_midpoint(info) >= bank_end) ?
                            HAL_DM_BUS_DM2 : HAL_DM_BUS_DM1;
                return heap_num;
            }
        }
    }

    *core = PROC_PROCESSOR_INVALID;
    *pref_dem = HAL_DM_BUS_DM1;
    return HEAP_INVALID;
}

static inline bool heap_needs_dm_bank_permissions(heap_names name)
{
#ifdef CHIP_HAS_SLOW_DM_RAM
    return ((name == HEAP_MAIN) || (name == HEAP_SLOW));
#else
    return (name == HEAP_MAIN);
#endif
}

static inline bool bank_can_be_powered_off(HAL_DM_BANK bank,
                                           heap_names name)
{
#ifdef DM_BANKS_CAN_BE_POWERED_OFF
    /* The first bank does not belong to a heap but should not be
       turned off. */
    return (bank != HAL_DM_BANK_FIRST) && (name == HEAP_INVALID);
#else
    NOT_USED(name);
    return FALSE;
#endif
}

/**
 * \brief Configure DM bank permissions based on configuration of the heaps.
 *
 *        Banks are divided between the 2 cores for use as various heaps, with
 *        each core getting at least 1 bank (except for shared heap). P1 will
 *        get the lower bank(s) and P0 will get the upper bank(s).
 *        If a core has only 1 bank in a given heap, the permissions will be
 *        set so that the core's DM1 access is the default owner.
 *        If a core has more than 1 bank, the lower half of the total banks
 *        will be set to be owned by core's DM2 access and the remaining upper
 *        half owned by core's DM1 access.
 *
 *        When setting the ownership for a core, all writes to the bank from
 *        the other core will be blocked. DM1/DM2 ownership for the same core
 *        only applies when there is simultaneous access from the same core
 *        on the 2 address spaces. This is not blocked, but the ownership will
 *        cause stalls on simultaneous access.
 *
 *        For shared heap and banks that do not fall in any heap (for example
 *        bank0 may be fully occupied by private and global vars), configure
 *        the default/reset value in the arbiter permissions. The default/reset
 *        value gives preferred owner as Core 0's DM1 access and other core or
 *        DM2 access are not blocked
 */
static void set_dm_memory_bank_permissions(heap_config *processor_heap_info_list)
{
    HAL_DM_BANK bank;
    HAL_DM_BANK number_dm_banks;

    patch_fn_shared(heap);

    number_dm_banks = hal_dm_get_number_banks();
    for (bank = 0; bank < number_dm_banks; bank++)
    {
        HAL_DM_BUS pref_dm;
        bool allow_other_cores;
        PROC_ID_NUM core;
        heap_names heap_num;

        heap_num = get_preferred_ownership_for_bank(processor_heap_info_list,
                                                    bank,
                                                    &core,
                                                    &pref_dm);

        if (bank_can_be_powered_off(bank, heap_num))
        {
            core = PROC_PROCESSOR_INVALID;
            pref_dm = HAL_DM_BUS_DM1;
            allow_other_cores = FALSE;
        }
        else if (heap_needs_dm_bank_permissions(heap_num))
        {
            if (proc_multiple_cores_present())
            {
                allow_other_cores = FALSE;
            }
            else
            {
                /* By default, core specific banks exclude write access from the
                 * other core. If thread offload is enabled, allow access to P1 */
                core = PROC_PROCESSOR_0;
                allow_other_cores = heap_alloc_thread_offload_is_configured();
            }
        }
        else
        {
            core = PROC_PROCESSOR_0;
            pref_dm = HAL_DM_BUS_DM1;
            allow_other_cores = TRUE;
        }
        hal_dm_configure_arbiter(bank, core, pref_dm, allow_other_cores);
    }
}

/* Rounds down the address to the bank start address. In other words,
   it returns the start address of the bank that contains the address
   given as argument. */
static inline char *round_down_to_dm_bank_addr(char *address)
{
    uintptr_t retval = (uintptr_t)(address);
    retval = (retval / DM_RAM_BANK_SIZE) * DM_RAM_BANK_SIZE;
    return (char*) retval;
}

#if !defined(UNIT_TEST_BUILD)
/**
 * \brief Get the chip's dynamic heap configuration from the system.
 *
 * \param mib_value Pointer to store the chip's dynamic heap configuration.
 *
 * \return TRUE if the configuration was retrieved successfully.
 */
static inline bool get_mib_value(HEAP_MIB_VALUE *mib_value)
{
    uint8 *buf;
    int16 len_read;

    buf = &mib_value->array[0];
    len_read = mibgetreqstr(HEAPALLOCATIONEX,
                            buf,
                            sizeof(HEAP_MIB_VALUE));
    if (len_read != sizeof(HEAP_MIB_VALUE))
    {
        /* Return straight away if MIB get fails. */
        return FALSE;
    }

    return TRUE;
}
#else
static inline bool get_mib_value(HEAP_MIB_VALUE *mib_value)
{
    /* Initialise MIB to a known standard value that will give enough memory
     * to second core, enough for unit tests that do not care about exact memory
     * split but do care about not including the MIB module.
     * The values here is based on Stre which has least of DM RAM. On StrePlus
     * because of the way memory is split, it will result in P1 getting more
     * memory than P0. But again, this is for unit tests that do not care about
     * the exact split.
     * See dual core user guide for more info
     */
#if defined(CHIP_CRESCENDO)
    mib_value->array[0] = 0xA0; /* Core 0, HEAP_MIB_HEAP_LSB_OFFSET. */
    mib_value->array[1] = 0x00; /* Core 0, HEAP_MIB_HEAP_MSB_OFFSET. */
    mib_value->array[2] = 0x60; /* Core 0, HEAP_MIB_SLOW_HEAP_LSB_OFFSET. */
    mib_value->array[3] = 0x00; /* Core 0, HEAP_MIB_SLOW_HEAP_MSB_OFFSET. */
    mib_value->array[4] = 0x60; /* Core 1, HEAP_MIB_HEAP_LSB_OFFSET. */
    mib_value->array[5] = 0x00; /* Core 1, HEAP_MIB_HEAP_MSB_OFFSET. */
    mib_value->array[6] = 0x60; /* Core 1, HEAP_MIB_SLOW_HEAP_LSB_OFFSET. */
    mib_value->array[7] = 0x00; /* Core 1, HEAP_MIB_SLOW_HEAP_MSB_OFFSET. */
#elif defined(CHIP_STRE)
    mib_value->array[0] = 0xA0; /* Core 0, HEAP_MIB_HEAP_LSB_OFFSET. */
    mib_value->array[1] = 0x00; /* Core 0, HEAP_MIB_HEAP_MSB_OFFSET. */
    mib_value->array[2] = 0x40; /* Core 1, HEAP_MIB_HEAP_LSB_OFFSET. */
    mib_value->array[3] = 0x00; /* Core 1, HEAP_MIB_HEAP_MSB_OFFSET. */
#elif defined(CHIP_MAOR) && defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    /* Assumes defined(COMMON_SHARED_HEAP) */
    mib_value->array[0]  = 0xE0; /* Core 0, HEAP_MIB_HEAP_LSB_OFFSET. */
    mib_value->array[1]  = 0x00; /* Core 0, HEAP_MIB_HEAP_MSB_OFFSET. */
    mib_value->array[2]  = 0xC0; /* Core 0, HEAP_MIB_EXTRA_HEAP_LSB_OFFSET. */
    mib_value->array[3]  = 0x01; /* Core 0, HEAP_MIB_EXTRA_HEAP_MSB_OFFSET. */
    mib_value->array[4]  = 0x20; /* Core 0, HEAP_MIB_NVRAM_HEAP_LSB_OFFSET. */
    mib_value->array[5]  = 0x00; /* Core 0, HEAP_MIB_NVRAM_HEAP_MSB_OFFSET. */
    mib_value->array[6]  = 0xA0; /* Core 1, HEAP_MIB_HEAP_LSB_OFFSET. */
    mib_value->array[7]  = 0x00; /* Core 1, HEAP_MIB_HEAP_MSB_OFFSET. */
    mib_value->array[8]  = 0xC0; /* Core 1, HEAP_MIB_EXTRA_HEAP_LSB_OFFSET. */
    mib_value->array[9]  = 0x01; /* Core 1, HEAP_MIB_EXTRA_HEAP_MSB_OFFSET. */
    mib_value->array[10] = 0x20; /* Core 1, HEAP_MIB_NVRAM_HEAP_LSB_OFFSET. */
    mib_value->array[11] = 0x00; /* Core 1, HEAP_MIB_NVRAM_HEAP_MSB_OFFSET. */
#elif defined(CHIP_MAOR)
    /* Assumes defined(COMMON_SHARED_HEAP) */
    mib_value->array[0] = 0xE0; /* Core 0, HEAP_MIB_HEAP_LSB_OFFSET. */
    mib_value->array[1] = 0x00; /* Core 0, HEAP_MIB_HEAP_MSB_OFFSET. */
    mib_value->array[2] = 0xE0; /* Core 0, HEAP_MIB_EXTRA_HEAP_LSB_OFFSET. */
    mib_value->array[3] = 0x01; /* Core 0, HEAP_MIB_EXTRA_HEAP_MSB_OFFSET. */
    mib_value->array[4] = 0xA0; /* Core 1, HEAP_MIB_HEAP_LSB_OFFSET. */
    mib_value->array[5] = 0x00; /* Core 1, HEAP_MIB_HEAP_MSB_OFFSET. */
    mib_value->array[6] = 0xE0; /* Core 1, HEAP_MIB_EXTRA_HEAP_LSB_OFFSET. */
    mib_value->array[7] = 0x01; /* Core 1, HEAP_MIB_EXTRA_HEAP_MSB_OFFSET. */
#elif defined(CHIP_STREPLUS)
    /* Assumes defined(COMMON_SHARED_HEAP) */
    mib_value->array[0] = 0xE0; /* Core 0, HEAP_MIB_HEAP_LSB_OFFSET. */
    mib_value->array[1] = 0x00; /* Core 0, HEAP_MIB_HEAP_MSB_OFFSET. */
    mib_value->array[2] = 0xA0; /* Core 1, HEAP_MIB_HEAP_LSB_OFFSET. */
    mib_value->array[3] = 0x00; /* Core 1, HEAP_MIB_HEAP_MSB_OFFSET. */
#else
#error "Unsupported chip in function get_mib_value"
#endif
    return TRUE;
}
#endif /* !defined(UNIT_TEST_BUILD) */

#if !defined(UNIT_TEST_BUILD) && defined(HAVE_EXTERNAL_HEAP)
/**
 * \brief Get the chip's external heap configuration from the system.
 *
 * \param mib_value Pointer to store the chip's dynamic heap configuration.
 *
 * \return TRUE if the configuration was retrieved successfully.
 */
static inline bool get_ext_mib_value(HEAP_EXT_MIB_VALUE *mib_value)
{
    uint8 *buf;
    int16 len_read;

    buf = &mib_value->array[0];
    memset(buf, 0, sizeof(HEAP_EXT_MIB_VALUE));
    len_read = mibgetreqstr(HEAPALLOCATIONSRAM,
                            buf,
                            sizeof(HEAP_EXT_MIB_VALUE));
    if (len_read != sizeof(HEAP_EXT_MIB_VALUE))
    {
        /* Return straight away if MIB get fails. */
        return FALSE;
    }

    return TRUE;
}
#elif defined(UNIT_TEST_BUILD) && defined(HAVE_EXTERNAL_HEAP)
static inline bool get_ext_mib_value(HEAP_EXT_MIB_VALUE *mib_value)
{
    /* Initialise MIB to a known standard value that will give enough memory
     * to second core, enough for unit tests that do not care about exact memory
     * split but do care about not including the MIB module. */
    uint8 *buf;

    buf = &mib_value->array[0];
    memset(buf, 0, sizeof(HEAP_EXT_MIB_VALUE));
    /* Total SRAM size is set to 256 KiB. */
    mib_value->array[0] = 0x01;
    mib_value->array[1] = 0x00;
    /* Reserve 128 KiB for the primary processor. */
    mib_value->array[2] = 0x00;
    mib_value->array[3] = 0x80;
    /* Reserve 128 KiB for the primary processor. */
    mib_value->array[2] = 0x00;
    mib_value->array[3] = 0x80;

    return TRUE;
}
#else
#define get_ext_mib_value(x) IGNORE_TRUE(x)
#endif /* !defined(UNIT_TEST_BUILD) && defined(HAVE_EXTERNAL_HEAP) */

/**
 * \brief Get a pointer to a core's dynamic heap configuration.
 *
 * \param core      Indicates which core to request.
 * \param mib_value Pointer to the chip's dynamic heap configuration.
 *
 * \return A pointer to the core's dynamic heap configuration.
 */
static inline HEAP_MIB_PER_PROC_VALUE *get_proc_mib(PROC_ID_NUM core,
                                                    HEAP_MIB_VALUE *mib_value)
{
    uint8 *core_array;

    core_array = &mib_value->array[core * HEAP_MIB_PER_PROC_SIZE];
    return (HEAP_MIB_PER_PROC_VALUE *) core_array;
}

#if defined(HAVE_EXTERNAL_HEAP)
/**
 * \brief Get how much externally memory to give to a processor.
 *
 * \param core      Indicates which core to request.
 * \param mib_value Pointer to the chip's dynamic heap configuration.
 *
 * \return The size of the heap in KiB.
 */
static inline uint16 get_proc_ext_mib(PROC_ID_NUM core,
                                      HEAP_EXT_MIB_VALUE *mib_value)
{
    uint16 *core_array;
    unsigned offset;

    offset = HEAP_EXT_MIB_HEAPS_OFFSET;
    offset += core * HEAP_EXT_MIB_PER_PROC_SIZE;
    core_array = (uint16 *) &mib_value->array[offset];

    return swap_16b(core_array[0]);
}

/**
 * \brief Get the size of the external memory.
 *
 * \param mib_value Pointer to the chip's dynamic heap configuration.
 *
 * \return An unsigned value.
 */
static inline unsigned get_ext_ram_size(HEAP_EXT_MIB_VALUE *mib_value)
{
    uint16 *core_array;

    core_array = (uint16 *) &mib_value->array[0];

    return swap_16b(core_array[0]);
}

#else
#define get_proc_ext_mib(x, y) 0
#define get_ext_ram_size(x) 0
#endif /* defined(HAVE_EXTERNAL_HEAP) */

/**
 * \brief Get the size of a heap from a core's dynamic heap configuration.
 *
 * \param heap_name Indicates the name of the heap to request.
 * \param mib_value Pointer to the core's dynamic heap configuration.
 *
 * \return The size of the heap in multiple of 1 KiB.
 *
 * \note The heap must exist.
 */
static uint16 get_heap_size_from_mib(heap_names heap_name,
                                     const HEAP_MIB_PER_PROC_VALUE *mib_value)
{
    uint16 value;

    value = 0;

    /* HeapAllocationEx stores each heap's size in a little-endian uint16. */
    if (heap_name == HEAP_MAIN)
    {
        value = mib_value->array[HEAP_MIB_HEAP_LSB_OFFSET];
        value |= (uint16) (mib_value->array[HEAP_MIB_HEAP_MSB_OFFSET] << 8);
    }
#if defined(DM_BANKS_CAN_BE_POWERED_OFF)
    else if (heap_name == HEAP_EXTRA)
    {
        value = mib_value->array[HEAP_MIB_EXTRA_HEAP_LSB_OFFSET];
        value |= (uint16) (mib_value->array[HEAP_MIB_EXTRA_HEAP_MSB_OFFSET] << 8);
    }
#endif
#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    else if (heap_name == HEAP_NVRAM)
    {
        value = mib_value->array[HEAP_MIB_NVRAM_HEAP_LSB_OFFSET];
        value |= (uint16) (mib_value->array[HEAP_MIB_NVRAM_HEAP_MSB_OFFSET] << 8);
    }
#endif
#ifdef CHIP_HAS_SLOW_DM_RAM
    else if (heap_name == HEAP_SLOW)
    {
        value = mib_value->array[HEAP_MIB_SLOW_HEAP_LSB_OFFSET];
        value |= (uint16) (mib_value->array[HEAP_MIB_SLOW_HEAP_MSB_OFFSET] << 8);
    }
#endif
    return value;
}

/**
 * \brief Check whether a given size is acceptable for a heap
 *
 * \param heap_num The heap to query
 * \param size     The desired size in octets.
 *
 * \return TRUE if the size is not correct.
 */
static inline bool invalid_heap_size(heap_names heap_num, unsigned size)
{
    NOT_USED(heap_num);

    /* All the heaps that can be configured via MIB must have a size
       that is a multiple of the size of a Data Memory bank. */
    if ((size % DM_RAM_BANK_SIZE) != 0)
    {
        return TRUE;
    }

    /* The extra heap can have a size of 0. */
#if defined(DM_BANKS_CAN_BE_POWERED_OFF)
    if (heap_num == HEAP_EXTRA)
    {
        return FALSE;
    }
#endif

    /* So does the heap used by the instruction cache. */
#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    if (heap_num == HEAP_NVRAM)
    {
        return FALSE;
    }
#endif

    /* All other heaps must allocate at least one Data Memory bank per core. */
    if (size < DM_RAM_BANK_SIZE)
    {
        return TRUE;
    }
    return FALSE;
}

/**
 * \brief Get and validate the chip's dynamic heap configuration from the system.
 *
 * \param[out] value A pointer to a structure containing the size of all heaps.
 *
 * \return TRUE if the function succeeded and *value is usable.
 */
static void get_heap_allocation(heap_dyn_size_config *value)
{
    PROC_ID_NUM core;
    unsigned total_heap_sizes[HEAP_ARRAY_SIZE];
    unsigned ext_ram_size;
    HEAP_MIB_VALUE mib_value;
    HEAP_EXT_MIB_VALUE mib_ext_value;
    heap_names heap_num;

    patch_fn_shared(heap);

    if (!get_mib_value(&mib_value) ||
        !get_ext_mib_value(&mib_ext_value))
    {
        /* At the very least, MIB should have returned
           the default values from ROM. */
        panic(PANIC_AUDIO_HEAP_CONFIGURATION_INVALID);
    }

    ext_ram_size = get_ext_ram_size(&mib_ext_value);
    ext_ram_size *= KIBYTE;
    if (ext_ram_size > get_region_max_size(REGION_EXT))
    {
        /* The external memory cannot be larger than the chip
           memory space reserved for it. */
        panic(PANIC_AUDIO_HEAP_CONFIGURATION_INVALID);
    }

    memset(total_heap_sizes, 0, sizeof(total_heap_sizes));
    for (core = PROC_PROCESSOR_0; core < PROC_PROCESSOR_MAX; core++)
    {
        HEAP_MIB_PER_PROC_VALUE *per_core_mib;
        uint16 per_core_ext_mib;
        unsigned int *sizes;

        sizes = value->p[core].heap_size;
        per_core_mib = get_proc_mib(core, &mib_value);
        per_core_ext_mib = get_proc_ext_mib(core, &mib_ext_value);

        for (heap_num = HEAP_MAIN; heap_num < HEAP_ARRAY_SIZE; heap_num++)
        {
            unsigned size, total;

            total = total_heap_sizes[heap_num];
            if (heap_num == HEAP_SHARED)
            {
                /* The shared heap is not configurable via MIB. Deal with it
                   separately. */
                size = get_region_max_size(REGION_SHARED);
#if !defined(COMMON_SHARED_HEAP)
                size /= PROC_PROCESSOR_BUILD;
#endif
            }
            else if (heap_num < HEAP_INTERNAL_SIZE)
            {
                region_names region_num;
                unsigned heap_max_size;

                size = get_heap_size_from_mib(heap_num, per_core_mib);
                size *= KIBYTE;
                if (invalid_heap_size(heap_num, size))
                {
                    panic(PANIC_AUDIO_HEAP_CONFIGURATION_INVALID);
                }
                total += size;

                region_num = get_region_from_heap(heap_num);
                heap_max_size = get_region_max_size(region_num);
                if (total > heap_max_size)
                {
                    panic(PANIC_AUDIO_HEAP_CONFIGURATION_INVALID);
                }
            }
            else
            {
                size = per_core_ext_mib;
                size *= KIBYTE;
                total += size;
                if (total > ext_ram_size)
                {
                    panic(PANIC_AUDIO_HEAP_CONFIGURATION_INVALID);
                }
            }
            sizes[heap_num] = size;
            total_heap_sizes[heap_num] = total;
        }
    }
}

/**
 * \brief Attempt to extend configured heap boundaries to a block boundary.
 */
static void heap_configure_and_align(heap_config *core_config,
                                     heap_dyn_size_config *dyn_heap_config)
{
    char *current;
    unsigned i;

    patch_fn_shared(heap);

    /* Lay the heaps in the order in which they are stacked in memory. */
    current = core_config[PROC_PROCESSOR_0].heap[ordered_heaps[0]].heap_start;

    /* Configure and align all the heaps using on-chip memory to a bank
     * boundary. */
    for (i = 0; i < ARRAY_LENGTH(ordered_heaps); i++)
    {
        PROC_ID_NUM core;
        heap_names heap_num;

        /* Use the memory layout order instead of the API order. */
        heap_num = ordered_heaps[i];

        for (core = PROC_PROCESSOR_0; core < PROC_PROCESSOR_BUILD; core++)
        {
            heap_info *heap;
            char *start[PROC_PROCESSOR_BUILD];
            char *end[PROC_PROCESSOR_BUILD];
            unsigned size;

            heap = &core_config[core].heap[heap_num];

            /* get_heap_allocation makes sure that the required heaps always
               have at least one bank of memory. So when there is no memory,
               it means that the core has been disabled. */
            size = dyn_heap_config->p[core].heap_size[heap_num];
            if (size != 0)
            {
                /* The heaps are either stacked on one another or share
                   the same boundaries. */
                if (heap_boundaries_are_reused(core, heap_num))
                {
                    start[core] = start[PROC_PROCESSOR_0];
                    end[core] = end[PROC_PROCESSOR_0];
                }
                else
                {
                    start[core] = current;
                    end[core] = start[core] + size;
                }

                size = end[core] - start[core];
                current = end[core];
            }

            if (size != 0)
            {
                heap->heap_start = start[core];
                heap->heap_end = end[core];
                heap->heap_size = size;
                /* heap->heap_free will be taken care of when either the last
                   node is resized or a new node is created. */
            }
            else
            {
                memset(heap, 0, sizeof(heap_info));
            }
        }
    }
}

#if defined(HAVE_EXTERNAL_HEAP)
static void heap_configure_external(heap_config *core_config,
                                    heap_dyn_size_config *dyn_heap_config)
{
    PROC_ID_NUM core;
    char *start;

    patch_fn_shared(heap);

    start = core_config[PROC_PROCESSOR_0].heap[HEAP_EXT].heap_start;
    for (core = PROC_PROCESSOR_0; core < PROC_PROCESSOR_BUILD; core++)
    {
        heap_info *heap;
        unsigned size;

        heap = &core_config[core].heap[HEAP_EXT];
        size = dyn_heap_config->p[core].heap_size[HEAP_EXT];
        if (size == 0)
        {
            memset(heap, 0, sizeof(heap_info));
            continue;
        }

        heap->heap_start = start;
        heap->heap_end = start;
        heap->heap_end += size;
        heap->heap_size = size;

        start = heap->heap_end;
    }
}
#else
#define heap_configure_external(x, y)
#endif /* defined(HAVE_EXTERNAL_HEAP) */

/****************************************************************************
Public Function Definitions
*/

#if defined(INSTALL_MIB)
void heap_get_allocation_rules(heap_allocation_rules *rules)
{
    uint16 value;
    unsigned len_read, i;
    uint16 mib_data[NUM_SHARED_MEM_OPTIONS];

    patch_fn_shared(heap);

    len_read = mibgetreqstr(PREFERREDSHAREDMALLOCRANGES,
                            (uint8 *) mib_data,
                            sizeof(mib_data));
    if (len_read == sizeof(mib_data))
    {
       uint16 *shared_mem_pref;

       shared_mem_pref = (uint16*) &rules->pref_shared_mem_ranges;
       for (i = 0; i < NUM_SHARED_MEM_OPTIONS; i++)
       {
           shared_mem_pref[i] = swap_16b(mib_data[i]);
       }
    }

    value = MALLOC_STRICTNESS_DM1_FALLBACK;
    (void) mibgetu16(RELAXMALLOCSTRICTNESS, &value);
    if (value > MALLOC_STRICTNESS_MAX)
    {
        value = MALLOC_STRICTNESS_DM1_FALLBACK;
    }
}
#else
void heap_get_allocation_rules(heap_allocation_rules *rules)
{
    memset(rules, 0, sizeof(heap_allocation_rules));
    rules->relaxmallocstrictness = MALLOC_STRICTNESS_DM1_FALLBACK;
}
#endif /* defined(INSTALL_MIB) */

/**
 * \brief Configure the heaps based build time information.
 *
 * \note This is called only from the primary processor before the
 *       Management Information Base (MIB) becomes available.
 */
void heap_config_static(heap_config *processor_heap_info_list)
{
    region_names region_num;

    patch_fn_shared(heap);

    /* This check is only here to ensure that "heap_single_mode"
       is not optimized away so that ACAT keep working. */
    if (heap_single_mode == NULL)
    {
        return;
    }

#ifdef CHIP_HAS_SLOW_DM_RAM
    /* This check is only here to ensure that "slow_heap"
       is not optimized away so that ACAT keep working. */
    if (slow_heap == heap_single_mode)
    {
        return;
    }
#endif /* CHIP_HAS_SLOW_DM_RAM */

    for (region_num = 0; region_num < REGION_ARRAY_SIZE; region_num++)
    {
        region_boundaries *boundaries;
        heap_names heap_num;
        heap_info *heap;

        heap_num = get_region_default_heap(region_num);
        heap = &processor_heap_info_list[0].heap[heap_num];
        boundaries = &region_linker_info[heap_num];

        if (heap_num == HEAP_MAIN)
        {
            HAL_DM_BANK number_dm_banks;

            /* Calculate the size of the main heap taking into account
               the number of powered on banks. */
            number_dm_banks = hal_dm_get_number_banks();
            boundaries->size = number_dm_banks * DM_RAM_BANK_SIZE;
            boundaries->size -= (uintptr_t) boundaries->start_addr;
        }

        heap->heap_start = boundaries->start_addr;
        heap->heap_end = boundaries->start_addr;
        heap->heap_end += boundaries->size;
        heap->heap_size = boundaries->size;
        heap->heap_free = 0;
    }
}

/**
 * \brief Configure the heaps based on product specific configuration.
 *
 * \note This is called only from the primary processor after the
 *       Management Information Base (MIB) becomes available.
 */
void heap_config_dynamic(heap_config *processor_heap_info_list)
{
    heap_dyn_size_config *dyn_heap_config;
    unsigned i;

    patch_fn_shared(heap);

    /* Get the product specific memory configuration. */
    dyn_heap_config = heap_alloc_boot(sizeof(heap_dyn_size_config));
    get_heap_allocation(dyn_heap_config);

    /* Make the primary processor take over all the memory
       reserved for disabled cores. */
    if (proc_single_core_present())
    {
        PROC_ID_NUM core;
        heap_sizes *primary;

        primary = &dyn_heap_config->p[PROC_PROCESSOR_0];
        for (core = PROC_PROCESSOR_1; core < PROC_PROCESSOR_MAX; core++)
        {
            heap_sizes *secondary;

            /* The build might have less cores present than is supported
               by the dynamic configuration. This is the case for example
               when running on the simulator. */
            if (core < PROC_PROCESSOR_BUILD)
            {
                memset(&processor_heap_info_list[core], 0, sizeof(heap_config));
            }

            secondary = &dyn_heap_config->p[core];
#if defined(COMMON_SHARED_HEAP)
            secondary->heap_size[HEAP_SHARED] = 0;
#endif
            for (i = 0; i < HEAP_ARRAY_SIZE; i++)
            {
                primary->heap_size[i] += secondary->heap_size[i];
                secondary->heap_size[i] = 0;
            }
        }
    }

    /* Divide the data memory banks between the various heaps. */
    heap_configure_and_align(processor_heap_info_list, dyn_heap_config);

    /* Split the external memory between cores. */
    heap_configure_external(processor_heap_info_list, dyn_heap_config);

    /* Free all temporary memory allocated on the heap to reduce stack usage. */
    pdelete(dyn_heap_config);
}

void heap_config_hardware(heap_config *processor_heap_info_list)
{
    /* In multi-core configuration, ensure that:
       - only one core can write in the banks from the main and slow heap.
       - nothing can access disabled banks. */
    set_dm_memory_bank_permissions(processor_heap_info_list);

    /* For chips that support it, disable the banks that are not used. */
    hal_dm_update_banks_power(1, hal_dm_get_number_banks() - 1);
}

#if defined(SUPPORTS_MULTI_CORE) && \
    defined(DM_BANKS_CAN_BE_POWERED_OFF)
void heap_change_power(PROC_ID_NUM proc_id, const heap_info *info, bool enable)
{
    unsigned bank;
    unsigned first_bank;
    unsigned last_bank;
    unsigned middle_bank;
    PROC_ID_NUM core;

    patch_fn_shared(heap);

    /* By design, both the start and the end of the heap are aligned
       on a bank boundary. */
    first_bank = ((uintptr_t) info->heap_start) / DM_RAM_BANK_SIZE;
    last_bank = ((uintptr_t) info->heap_end) / DM_RAM_BANK_SIZE;
    if (first_bank == last_bank)
    {
        /* Product configuration has not reserved any banks for this heap . */
        return;
    }
    middle_bank = (first_bank + last_bank) / 2;
    last_bank -= 1;

    core = PROC_PROCESSOR_INVALID;
    if (enable)
    {
        core = proc_id;
    }

    for (bank = first_bank; bank <= last_bank; bank++)
    {
        HAL_DM_BUS bus;

        bus = HAL_DM_BUS_DM1;
        if (bank < middle_bank)
        {
            bus = HAL_DM_BUS_DM2;
        }
        hal_dm_configure_arbiter((HAL_DM_BANK) bank, core, bus, FALSE);
    }

    hal_dm_update_banks_power((HAL_DM_BANK) first_bank, (HAL_DM_BANK) last_bank);
}
#endif /* defined(SUPPORTS_MULTI_CORE) */

