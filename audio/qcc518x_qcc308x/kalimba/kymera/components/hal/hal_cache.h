/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/** 
 * \file hal_cache.h
 * \ingroup HAL
 *
 * Public header file for HAL cache functions.
 */

#ifndef HAL_CACHE_H
#define HAL_CACHE_H

/****************************************************************************
Include Files
*/

#include "types.h"

/****************************************************************************
Public Type Declarations
*/

typedef enum
{
    HAL_CACHE_DISABLED  = 0,
    HAL_CACHE_2WAY      = 1,
    HAL_CACHE_2WAY_HALF = 2,
    HAL_CACHE_DIRECT    = 3,

#if !defined(INSTALL_CACHE)
    HAL_CACHE_DEFAULT   = HAL_CACHE_DISABLED,
#elif defined(INSTALL_2WAY_CACHE)
    HAL_CACHE_DEFAULT   = HAL_CACHE_2WAY,
#elif defined(INSTALL_2WAYHALF_CACHE)
    HAL_CACHE_DEFAULT   = HAL_CACHE_2WAY_HALF,
#elif defined(INSTALL_SWITCHABLE_CACHE)
    /* Switchable mode defaults to 2-way but may be changed later */
    HAL_CACHE_DEFAULT   = HAL_CACHE_2WAY,
#else
    HAL_CACHE_DEFAULT   = HAL_CACHE_DIRECT
#endif
} HAL_CACHE;

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief  Enable or disable PM cache
 */
extern void hal_cache_configure(HAL_CACHE type);

/**
 * \brief  Flush caches
 */
extern void hal_cache_flush(void);

/**
 * \brief Invalidate the content of the instruction cache.
 *
 * \param type Type of cache to use.
 */
extern void hal_cache_invalidate(HAL_CACHE type);

#endif /* HAL_CACHE_H */

