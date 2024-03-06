/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_periodic_scan_stop_find_trains_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_PERIODIC_SCAN_STOP_FIND_TRAINS_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_periodic_scan_stop_find_trains_req(
    phandle_t phandle,
    uint8_t scan_handle,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ULP_PERIODIC_SCAN_STOP_FIND_TRAINS_REQ_T *prim =
            zpnew(DM_ULP_PERIODIC_SCAN_STOP_FIND_TRAINS_REQ_T);

    prim->type = DM_ULP_PERIODIC_SCAN_STOP_FIND_TRAINS_REQ;
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
