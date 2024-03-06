/****************************************************************************
 * Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/****************************************************************************
 * \defgroup pl_malloc Memory allocation and free functionality
 *
 * \file pl_malloc.h
 * \ingroup pl_malloc
 *
 *
 * Interface for memory allocation/free functions
 *
 ****************************************************************************/

#if !defined(PL_MALLOC_H)
#define PL_MALLOC_H

/****************************************************************************
Include Files
*/
#include "proc/proc.h"
#include "pmalloc/pl_malloc_preference.h"
#include "pmalloc/dmprofiling.h"
#include "hal/hal_dm_sections.h"

/****************************************************************************
Public Type Declarations
*/

typedef unsigned (*pmalloc_cached_report_handler)(void);

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
typedef struct
{
    /** The first memory location which can be accessed via the NVRAM window. */
    char *start;

    /** The first memory location which cannot be accessed via the NVRAM window. */
    char *end;
} DM_WINDOW;
#endif

/****************************************************************************
Public Macro Declarations
*/

/**
 * Use Memory pool based malloc and free functions, defined in pl_malloc.c
 *
 * xpmalloc: a version of malloc similar to standard C library, that does not panic if it has not memory but instead returns a Null pointer.
 * pmalloc: a version of malloc that panics if it has no memory. .
 * xzpmalloc: a version of malloc that initialises the allocated memory to zero or returns a Null pointer if it has no memory.
 * zpmalloc: a version of malloc that initialises the allocated memory to zero or panics if it has no memory.
 *
 * The '[x][z]ppmalloc variants take an additional 'preference' parameter to specify DM1/DM2/either.
 *
 * pfree: a version of free (does not panic if passed a Null pointer).
 *
 * Note init_pmalloc must be called before any version of malloc or free is called
 */

#ifdef PMALLOC_DEBUG
extern void *xppmalloc_debug(unsigned int numBytes, unsigned int preference, const char *file, unsigned int line);
extern void *ppmalloc_debug(unsigned int numBytes, unsigned int preference, const char *file, unsigned int line);
extern void *xzppmalloc_debug(unsigned int numBytes, unsigned int preference, const char *file, unsigned int line);
extern void *zppmalloc_debug(unsigned int numBytes, unsigned int preference, const char *file, unsigned int line);

#define xppmalloc(numBytes, pref) xppmalloc_debug(numBytes, pref, __FILE__, __LINE__)
#define ppmalloc(numBytes, pref) ppmalloc_debug(numBytes, pref, __FILE__, __LINE__)
#define xzppmalloc(numBytes, pref) xzppmalloc_debug(numBytes, pref, __FILE__, __LINE__)
#define zppmalloc(numBytes, pref) zppmalloc_debug(numBytes, pref, __FILE__, __LINE__)

extern void pvalidate(void *pMemory);

#else
extern void *xppmalloc(unsigned int numBytes, unsigned int preference);
extern void *ppmalloc(unsigned int numBytes, unsigned int preference);
extern void *xzppmalloc(unsigned int numBytes, unsigned int preference);
extern void *zppmalloc(unsigned int numBytes, unsigned int preference);
#endif

#define xpmalloc(numBytes) xppmalloc(numBytes, MALLOC_PREFERENCE_NONE)
#define pmalloc(numBytes) ppmalloc(numBytes, MALLOC_PREFERENCE_NONE)
#define xzpmalloc(numBytes) xzppmalloc(numBytes, MALLOC_PREFERENCE_NONE)
#define zpmalloc(numBytes) zppmalloc(numBytes, MALLOC_PREFERENCE_NONE)

extern void pfree(void *pMemory);

extern int psizeof(void *pMemory);

extern void init_pmalloc(void);

extern void config_pmalloc(void);

#if defined(SUPPORTS_MULTI_CORE)
extern void pmalloc_processor_change(PROC_TRANSITION transition);
#endif

#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
/**
 * \brief Get boundaries of a Data Memory area that the core can
 *        execute instructions from.
 *
 * \param[out] dm_window Pointer to structure containing the result.
 *
 * \return TRUE if the function was successful.
 */
extern bool heap_acquire_dm_window(DM_WINDOW *dm_window);

/**
 * \brief Release memory acquired with heap_acquire_dm_window.
 */
extern void heap_release_dm_window(void);
#else
#define heap_acquire_dm_window(x)
#define heap_release_dm_window()
#endif /* defined(CHIP_HAS_NVRAM_ACCESS_TO_DM) */

/**
 * Macros for type specific memory allocation
 */

/* Allocate memory of a given type (t) from the pool. */
#define pnew(t)             ((t *)( pmalloc( sizeof(t) )))
#define xpnew(t)            ((t *)( xpmalloc( sizeof(t) )))

/* Allocate memory of a given type (t) with preference (p). */
#define ppnew(t, p)         ((t *)( ppmalloc( sizeof(t), (p) )))
#define xppnew(t, p)        ((t *)( xppmalloc( sizeof(t), (p) )))

/* Allocate zeroed memory of a given type (t) from the pool. */
#define zpnew(t)            ((t *)( zpmalloc( sizeof(t) )))
#define xzpnew(t)           ((t *)( xzpmalloc( sizeof(t) )))

/* Allocate zeroed memory of a given type (t) with preference (p). */
#define zppnew(t, p)        ((t *)( zppmalloc( sizeof(t), (p) )))
#define xzppnew(t, p)       ((t *)( xzppmalloc( sizeof(t), (p) )))

/* Similar macros which allocate arrays of objects. */
#define pnewn(n, t)         ((t *)( pmalloc ( sizeof(t) * (n) )))
#define xpnewn(n, t)        ((t *)( xpmalloc ( sizeof(t) * (n) )))
#define zpnewn(n, t)        ((t *)( zpmalloc ( sizeof(t) * (n) )))
#define xzpnewn(n, t)       ((t *)( xzpmalloc ( sizeof(t) * (n) )))

/* As above with preference (p). */
#define ppnewn(n, t, p)     ((t *)( ppmalloc ( sizeof(t) * (n), (p) )))
#define xppnewn(n, t, p)    ((t *)( xppmalloc ( sizeof(t) * (n), (p) )))
#define zppnewn(n, t, p)    ((t *)( zppmalloc ( sizeof(t) * (n), (p) )))
#define xzppnewn(n, t, p)   ((t *)( xzppmalloc ( sizeof(t) * (n), (p) )))

/* Return the [x]pmalloc()ed memory block (d) to the pool. */
#define pdelete(d)          (pfree((void *)(d)))


/****************************************************************************
Global Variable Definitions
*/

/****************************************************************************
Public Function Prototypes
*/
#ifdef PL_MEM_POOL_TEST
/* Used for module testing only */
extern void PlMemPoolTest(void);
#endif

/**
 * \brief Function to allow pmalloc clients to report size of any cached data items
 *
 * \param[in] new_handler Function called when calculating free pools
 *
 * \return  Previously-installed handler function
 *
 * \note Report handlers are statically chained - the caller of this function should
 * store the old handler function returned, and (if non-NULL) call it from its handler,
 * adding the result to the size returned.
 */
extern pmalloc_cached_report_handler pmalloc_cached_report(pmalloc_cached_report_handler new_handler);

#if defined(COMMON_SHARED_HEAP)
/**
 * \brief Function for checking if the address supplied is in the shared heap.
 *
 * \param ptr_mem pointer to the address
 *
 * \return Returns True is the address supplied is in the shared heap.
 */
extern bool is_addr_in_shared_memory(void *ptr_mem);
#endif /* COMMON_SHARED_HEAP */

/* Do not use directly. Use pmalloc_preference_is_available instead. */
#if defined(DM_BANKS_CAN_BE_POWERED_OFF) || \
    defined(INSTALL_EXTERNAL_MEM)
extern bool pmalloc_preference_is_available_internal(unsigned preference);
#else
#define pmalloc_preference_is_available_internal(x) FALSE
#endif

/**
 * \brief Is a given preference available on the chip?
 *
 * \param preference The preference to be tested.
 *
 * \return True if it is possible to satify the preference when
 *         allocating memory. False if fallback will be used.
 *
 * \note This is useful to detect if a heap has been disabled using
 *       the per chip configuration.
 */
static inline bool pmalloc_preference_is_available(unsigned preference)
{
    switch (preference)
    {
        case MALLOC_PREFERENCE_EXTRA_DM1:
        case MALLOC_PREFERENCE_EXTRA_DM2:
        case MALLOC_PREFERENCE_EXTRA:
        case MALLOC_PREFERENCE_EXTERNAL:
            return pmalloc_preference_is_available_internal(preference);
        default:
            return TRUE;
    }
}
#endif /* PL_MALLOC_H */
