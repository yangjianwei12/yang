/*******************************************************************************

Copyright (C) 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_periodic_scan_sync_transfer_params_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_REQ_T primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_periodic_scan_sync_transfer_params_req(
    phandle_t       phandle,
    TYPED_BD_ADDR_T *addrt,
    uint8_t         mode,
    uint16_t        skip,
    uint16_t        sync_timeout,
    uint8_t         cte_type,
    DM_UPRIM_T      **pp_prim
    )
{
    DM_ULP_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_REQ_T *prim =
            zpnew(DM_ULP_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_REQ_T);

    prim->type = DM_ULP_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_REQ;
    prim->phandle = phandle;
    prim->addrt = *addrt;
    prim->mode = mode;
    prim->skip = skip;
    prim->sync_timeout = sync_timeout;
    prim->cte_type = cte_type;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
         DM_PutMsg(prim);
    }
}
