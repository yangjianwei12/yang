/****************************************************************************
 * Copyright 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file pl_nvmem.h
 * \ingroup platform
 *
 * Header for functions to handle memory map access windows.
 */

#ifndef PL_NVMEM_H
#define PL_NVMEM_H

/****************************************************************************
Include Files
*/
#include "types.h"

/****************************************************************************
Public Function Prototypes
*/
#if defined(RUN_CAPABILITIES_FROM_SQIF)
/**
 * \brief This wrapper API will help to map 
 *        Audio SQIF_FLASH into NVMEM Window 1 of the DM and PM.
 */
extern void pl_nvmem_map_audio_sqif_flash_into_nvmem_1(void);
#endif /* RUN_CAPABILITIES_FROM_SQIF */

#endif   /* PL_NVMEM_H */