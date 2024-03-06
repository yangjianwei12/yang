/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ext_scan_enable_scanners_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_EXT_SCAN_ENABLE_SCANNERS_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_ext_scan_enable_scanners_req(
    phandle_t phandle,
    uint8_t enable,
    uint8_t num_of_scanners,
    DM_ULP_EXT_SCAN_SCANNERS_T scanners[],
    DM_UPRIM_T **pp_prim
    )
{
    uint8_t index;
    DM_ULP_EXT_SCAN_ENABLE_SCANNERS_REQ_T *prim =
            zpnew(DM_ULP_EXT_SCAN_ENABLE_SCANNERS_REQ_T);

    if (num_of_scanners > DM_ULP_EXT_SCAN_MAX_SCANNERS)
    {
        BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }

    prim->type = DM_ULP_EXT_SCAN_ENABLE_SCANNERS_REQ;
    prim->phandle = phandle;
    prim->enable = enable;
    prim->num_of_scanners = num_of_scanners;

    for (index = 0; index < num_of_scanners; index++)
    {
        prim->scanners[index] = scanners[index];
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
