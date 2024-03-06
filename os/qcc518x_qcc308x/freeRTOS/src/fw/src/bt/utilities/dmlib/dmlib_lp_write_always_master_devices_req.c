/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_lp_write_always_master_devices_req
 *
 *  DESCRIPTION
 *      Build and send a DM_LP_WRITE_ALWAYS_MASTER_DEVICES_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_lp_write_always_master_devices_req(
    uint16_t operation,
    BD_ADDR_T *bd_addr,
    DM_UPRIM_T  **pp_prim
    )
{
    DM_LP_WRITE_ALWAYS_MASTER_DEVICES_REQ_T *prim =
        pnew(DM_LP_WRITE_ALWAYS_MASTER_DEVICES_REQ_T);

    prim->type = DM_LP_WRITE_ALWAYS_MASTER_DEVICES_REQ;
    prim->operation = operation;
    bd_addr_copy(&prim->bd_addr, bd_addr);

    if (pp_prim == NULL)
        DM_PutMsg(prim);
    else
        *pp_prim = (DM_UPRIM_T*)prim;
}
