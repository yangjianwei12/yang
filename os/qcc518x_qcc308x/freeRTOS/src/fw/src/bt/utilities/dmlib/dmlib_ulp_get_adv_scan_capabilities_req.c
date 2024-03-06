/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ulp_get_adv_scan_capabilities_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_GET_ADV_SCAN_CAPABILITIES_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_ulp_get_adv_scan_capabilities_req(
    DM_UPRIM_T **pp_prim
    )
{
    DM_ULP_GET_ADV_SCAN_CAPABILITIES_REQ_T *prim = zpnew(DM_ULP_GET_ADV_SCAN_CAPABILITIES_REQ_T);

    prim->type = DM_ULP_GET_ADV_SCAN_CAPABILITIES_REQ;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
         DM_PutMsg(prim);
    }
}
