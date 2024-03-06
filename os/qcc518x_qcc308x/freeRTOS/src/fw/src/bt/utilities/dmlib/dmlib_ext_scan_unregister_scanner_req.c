/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ext_scan_unregister_scanner_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_EXT_SCAN_UNREGISTER_SCANNER_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_ext_scan_unregister_scanner_req(
    phandle_t phandle,
    uint8_t scan_handle,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ULP_EXT_SCAN_UNREGISTER_SCANNER_REQ_T *prim =
            zpnew(DM_ULP_EXT_SCAN_UNREGISTER_SCANNER_REQ_T);

    prim->type = DM_ULP_EXT_SCAN_UNREGISTER_SCANNER_REQ;
    prim->phandle = phandle;
    prim->scan_handle = scan_handle;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
         DM_PutMsg(prim);
    }
}
