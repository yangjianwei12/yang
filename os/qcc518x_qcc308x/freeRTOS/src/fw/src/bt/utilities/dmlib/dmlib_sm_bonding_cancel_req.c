/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_bonding_cancel_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_BONDING_CANCEL_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_bonding_cancel_req(
    TYPED_BD_ADDR_T *addrt,
    uint16_t flags,
    DM_UPRIM_T **pp_prim
    )
{
    DM_SM_BONDING_CANCEL_REQ_T *p_prim = pnew(DM_SM_BONDING_CANCEL_REQ_T);
    p_prim->type    = DM_SM_BONDING_CANCEL_REQ;
    p_prim->flags   = flags;
    tbdaddr_copy(&p_prim->addrt, addrt);

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

