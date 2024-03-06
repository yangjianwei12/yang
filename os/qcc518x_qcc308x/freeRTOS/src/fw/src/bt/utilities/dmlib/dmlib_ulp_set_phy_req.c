/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ulp_set_phy_req
 *
 *  DESCRIPTION
 *      Build and send a dm_ulp_set_phy_req primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_ulp_set_phy_req(
    phandle_t phandle,
    const TP_BD_ADDR_T *p_tp_bd_addr,
    phy_rate_t min_tx,
    phy_rate_t max_tx,
    phy_rate_t min_rx,
    phy_rate_t max_rx,
    phy_pref_t flags,
    DM_UPRIM_T **pp_prim)
{
    DM_ULP_SET_PHY_REQ_T *p_prim = pnew(DM_ULP_SET_PHY_REQ_T);

    p_prim->type = DM_ULP_SET_PHY_REQ;
    p_prim->phandle = phandle;
    p_prim->tp_addrt = *p_tp_bd_addr;
    p_prim->min_tx = min_tx;
    p_prim->max_tx = max_tx;
    p_prim->min_rx = min_rx;
    p_prim->max_rx = max_rx;
    p_prim->flags = flags;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

