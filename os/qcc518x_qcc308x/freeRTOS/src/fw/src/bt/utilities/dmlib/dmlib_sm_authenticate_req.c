/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_authenticate_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_AUTHENTICATE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_authenticate_req(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    )
{
    DM_SM_AUTHENTICATE_REQ_T *p_prim = pnew(DM_SM_AUTHENTICATE_REQ_T);

    p_prim->type = DM_SM_AUTHENTICATE_REQ;
    bd_addr_copy(&p_prim->bd_addr, p_bd_addr);

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}
