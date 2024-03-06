/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/****************************************************************************
 * \defgroup pl_malloc Memory allocation and free functionality
 *
 * \file dmprofiling.h
 * \ingroup pl_malloc
 *
 *
 * Interface for tagging DM memory allocations
 *
 ****************************************************************************/

#if !defined(DMPROFILING_H)
#define DMPROFILING_H

/****************************************************************************
Include Files
*/

/****************************************************************************
Public Type Declarations
*/

#ifdef INSTALL_DM_MEMORY_PROFILING
/**
 * DM profiling tag values. Note, this is used by ACAT to detect
 * that INSTALL_DM_MEMORY_PROFILING is enabled. If this is removed
 * or amended, please amend ACAT as well.
 */
typedef enum
{
    DMPROFILING_OWNER_ACCMD            = 0x00,
    DMPROFILING_OWNER_NOTASK           = 0xFF,
} DMPROFILING_OWNER;
#endif

/****************************************************************************
Public Macro Declarations
*/

#ifdef INSTALL_DM_MEMORY_PROFILING
#define tag_dm_memory(ptr, id)      pmalloc_tag_dm_memory(ptr, sched_pack_taskid(id))
#define tag_dm_memory_override(id)  pmalloc_tag_dm_memory_override(sched_pack_taskid(id))
#define tag_dm_memory_reset()       pmalloc_tag_dm_memory_reset()
#else
#define tag_dm_memory(ptr, id)
#define tag_dm_memory_override(id)
#define tag_dm_memory_reset()
#endif

/****************************************************************************
Global Variable Definitions
*/

/****************************************************************************
Public Function Prototypes
*/

#ifdef INSTALL_DM_MEMORY_PROFILING
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
extern bool pmalloc_tag_dm_memory(void *ptr, DMPROFILING_OWNER id);

/**
 * NAME
 *   pmalloc_tag_dm_memory_override
 *
 * \brief Function to 'tag' a block of allocated heap or pool memory
 *        with 'owner id', as per the parameter given, for all
 *        DM memory allocations after this call. This overides the
 *        usual method of using the current task id for DM Memory
 *        profiling.
 *
 * \param[in] id   The owner id to tag any allocated blocks with
 */
extern void pmalloc_tag_dm_memory_override(DMPROFILING_OWNER id);

/**
 * NAME
 *   pmalloc_tag_dm_memory_reset
 *
 * \brief Function to reset 'tagging' a block of allocated heap or pool
 *        memory after a call to 'pmalloc_tag_dm_memory_override'. This 
 *        resets DM Profiling to return to the usual method of using the
 *        current task id for DM Memory profiling.
 */
extern void pmalloc_tag_dm_memory_reset(void);
#endif

#endif /* DMPROFILING_H */
