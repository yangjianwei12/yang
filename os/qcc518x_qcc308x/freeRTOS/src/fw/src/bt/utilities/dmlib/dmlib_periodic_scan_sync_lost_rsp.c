/*******************************************************************************

Copyright (C) 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_periodic_scan_sync_lost_rsp
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_PERIODIC_SCAN_SYNC_LOST_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_periodic_scan_sync_lost_rsp(
    uint16_t sync_handle,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ULP_PERIODIC_SCAN_SYNC_LOST_RSP_T *prim =
            zpnew(DM_ULP_PERIODIC_SCAN_SYNC_LOST_RSP_T);

    prim->type = DM_ULP_PERIODIC_SCAN_SYNC_LOST_RSP;
    prim->sync_handle = sync_handle;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
         DM_PutMsg(prim);
    }
}
