/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ext_scan_configure_scanner_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_EXT_SCAN_CONFIGURE_SCANNER_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_ext_scan_configure_scanner_req(
    uint8_t scan_handle,
    uint8_t use_only_global_params,
    uint16_t scanning_phys,
    DM_ULP_EXT_SCAN_PHY_T phys[],
    DM_UPRIM_T **pp_prim
    )
{
    uint8_t index, num_of_phys = 0;
    DM_ULP_EXT_SCAN_CONFIGURE_SCANNER_REQ_T *prim =
            zpnew(DM_ULP_EXT_SCAN_CONFIGURE_SCANNER_REQ_T);

    prim->type = DM_ULP_EXT_SCAN_CONFIGURE_SCANNER_REQ;
    prim->scan_handle = scan_handle;
    prim->use_only_global_params = use_only_global_params;
    prim->scanning_phys = scanning_phys;

    /* Work out the number of phys in parameter phys */
    for (index = 0; index < 16; index++)
    {
        if (scanning_phys & 1)
        {
            num_of_phys++;
        }
        scanning_phys = scanning_phys >> 1;
    }
    if (num_of_phys > DM_ULP_EXT_SCAN_MAX_SCANNING_PHYS)
    {
        BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }

    /* Copy each phy config into allocated memory */
    for (index = 0; index < num_of_phys; index++)
    {
        prim->phys[index] = phys[index];
    }

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}
