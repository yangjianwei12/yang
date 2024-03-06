/****************************************************************************
 * Copyright 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file pl_nvmem.c
 * \ingroup platform
 *
 * Wrapper functions to handle memory map access windows.
 */

/****************************************************************************
Include Files
*/
#include "pl_nvmem.h"
#include "hal/hal_windows.h"
#include "patch/patch.h"

/****************************************************************************
Public Data Definitions
*/

/****************************************************************************
Public Function Definitions
*/
#if defined(RUN_CAPABILITIES_FROM_SQIF)

/**
 * \brief This wrapper API will help to map 
 *        Audio SQIF_FLASH into NVMEM Window 1 of the DM and PM.
 */
void pl_nvmem_map_audio_sqif_flash_into_nvmem_1(void)
{
    patch_fn_shared(pl_nvmem);
    hal_window_open(HAL_WINDOWS_CAP_FROM_SQIF);
}

#endif /* RUN_CAPABILITIES_FROM_SQIF */