/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_acl_close_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ACL_CLOSE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_acl_close_req(
    TYPED_BD_ADDR_T *addrt,
    uint16_t flags,
    uint8_t reason,
    DM_UPRIM_T **pp_prim
    )
{
    DM_ACL_CLOSE_REQ_T *p_prim = zpnew(DM_ACL_CLOSE_REQ_T);

    p_prim->type = DM_ACL_CLOSE_REQ;
    p_prim->flags = flags;
    p_prim->reason = reason;
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

