/****************************************************************************
 * Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file pl_malloc_preference.h
 * \ingroup pl_malloc
 *
 * Interface for memory allocation/free functions
 * Supplement for allocation preference definitions
 * Separate from pl_malloc.h so asm files can include it
 *
 ****************************************************************************/

#if !defined(PL_MALLOC_PREFERENCE_H)
#define PL_MALLOC_PREFERENCE_H

/****************************************************************************
Include Files
*/

/****************************************************************************
Public Macro Declarations
*/

/* These values should match the ones in Previn's malloc.h. They should also
 * NOT be larger than 9 bits otherwise shared memory structure doesn't have
 * sufficient space to store them. */
#define MALLOC_PREFERENCE_DM1       1
#define MALLOC_PREFERENCE_DM2       2
#define MALLOC_PREFERENCE_NONE      3
#define MALLOC_PREFERENCE_FAST      4
#define MALLOC_PREFERENCE_SHARED    5
#define MALLOC_PREFERENCE_EXTERNAL  6
#define MALLOC_PREFERENCE_EXTRA_DM1 7
#define MALLOC_PREFERENCE_EXTRA_DM2 8
#define MALLOC_PREFERENCE_EXTRA     9

#define MALLOC_PREFERENCE_SYSTEM 0x100

/****************************************************************************
Public Type Declarations
*/

/****************************************************************************
Global Variable Definitions
*/

/****************************************************************************
Public Function Prototypes
*/

#endif /* PL_MALLOC_PREFERENCE_H */
