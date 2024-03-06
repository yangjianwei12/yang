/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_set_default_link_policy
 *
 *  DESCRIPTION
 *      Build and send a DM_SET_DEFAULT_LINK_POLICY_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_set_default_link_policy_req(
    link_policy_settings_t default_lp_in,
    link_policy_settings_t default_lp_out,
    DM_UPRIM_T **pp_prim
    )
{
    DM_SET_DEFAULT_LINK_POLICY_REQ_T *p_prim = pnew(DM_SET_DEFAULT_LINK_POLICY_REQ_T);

    p_prim->type = DM_SET_DEFAULT_LINK_POLICY_REQ;
    p_prim->link_policy_settings_in = default_lp_in;
    p_prim->link_policy_settings_out = default_lp_out;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

