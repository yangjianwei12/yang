/****************************************************************************
 * Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file pl_malloc_mem_usage.h
 * Interface for reading the memory usage of the system.
 *
 * \ingroup platform
 *
 ****************************************************************************/

#if !defined(PL_MALLOC_USAGE_H)
#define PL_MALLOC_USAGE_H

/****************************************************************************
Include Files
*/
#include "platform/pl_intrinsics.h"
#if defined(COMMON_SHARED_HEAP)
#include "proc/proc.h"
#endif /* COMMON_SHARED_HEAP */

/****************************************************************************
Public Macro Declarations
*/

/**
 * Clear the minimum available heap memory. In other words, the minimum available heap
 * memory will be equal to the currently available heap memory.
 */
/**
 * Currently available heap memory.
 */
unsigned heap_cur(void);

/**
 * Minimum available heap. The minimum available memory is also called memory watermarks.
 */
unsigned heap_min(void);

void heap_clear_watermarks(void);

/****************************************************************************
Public Type Declarations
*/

/****************************************************************************
Global Variable Definitions
*/

/****************************************************************************
Public Function Prototypes
*/
/*
 * Expose memory usage interface functions from the pool memory module
 */
extern unsigned pool_cur(void);
extern unsigned pool_min(void);
extern unsigned pool_size_total(void);
extern void pool_clear_watermarks(void);

extern unsigned heap_size(void);



#endif /* PL_MALLOC_USAGE_H */
