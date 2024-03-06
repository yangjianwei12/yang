/****************************************************************************
 * Copyright (c) 2011 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup HAL Hardware Abstraction Layer
 */ 
/** 
 * \file hal.h
 * \ingroup HAL
 *
 * Public header file for HAL functions.
 * Currently just initialisation
 * Likely to get split between functional areas later.
 */

#ifndef HAL_H
#define HAL_H

/****************************************************************************
Include Files
*/

#include "types.h"
#include "io_map.h"
#include "hal_macros.h"
#include "hal_perfstats.h"
#include "hal_alias.h"

/****************************************************************************
Public Type Declarations
*/

/****************************************************************************
Public Constant Declarations
*/

/****************************************************************************
Public Macro Declarations
*/

/****************************************************************************
Public Variable Definitions
*/

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief  Initialise the hardware
 */
extern void hal_init(void);

#endif /* HAL_H */
