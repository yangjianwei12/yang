/*******************************************************************************

Copyright (C) 2020  Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_set_link_behavior_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SET_LINK_BEHAVIOR_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_set_link_behavior_req(
    TYPED_BD_ADDR_T *addrt,
    uint16_t conftab_length,
    uint16_t *conftab,
    DM_UPRIM_T **pp_prim
    )
{
    DM_SET_LINK_BEHAVIOR_REQ_T *p_prim = pnew(DM_SET_LINK_BEHAVIOR_REQ_T);

    p_prim->type = DM_SET_LINK_BEHAVIOR_REQ;
    tbdaddr_copy(&p_prim->addrt, addrt);
    p_prim->conftab_length = conftab_length;
    p_prim->conftab = conftab;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        (void)DM_PutMsg(p_prim);
    }
}

