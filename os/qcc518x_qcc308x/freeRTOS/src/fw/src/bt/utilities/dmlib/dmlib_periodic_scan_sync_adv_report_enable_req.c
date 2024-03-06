/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_periodic_scan_sync_report_periodic_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_periodic_scan_sync_adv_report_enable_req(
    phandle_t phandle,
    uint16_t sync_handle,
    uint8_t enable,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_REQ_T *prim =
            zpnew(DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_REQ_T);

    prim->type = DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_REQ;
    prim->phandle = phandle;
    prim->sync_handle = sync_handle;
    prim->enable = enable;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
         DM_PutMsg(prim);
    }
}
