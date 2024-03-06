/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ext_scan_get_ctrl_scan_info_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_ext_scan_get_ctrl_scan_info_req(
    phandle_t phandle,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ULP_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ_T *prim =
            zpnew(DM_ULP_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ_T);

    prim->type = DM_ULP_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ;
    prim->phandle = phandle;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}
