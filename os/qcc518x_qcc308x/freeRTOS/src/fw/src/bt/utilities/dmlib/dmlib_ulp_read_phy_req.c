/*******************************************************************************

Copyright (C) 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ulp_read_phy_req
 *
 *  DESCRIPTION
 *      Build and send dm_ulp_read_phy_req primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_ulp_read_phy_req(
    phandle_t phandle,
    const TP_BD_ADDR_T *p_tp_bd_addr,
    DM_UPRIM_T **pp_prim)
{
    DM_ULP_READ_PHY_REQ_T *p_prim = pnew(DM_ULP_READ_PHY_REQ_T);

    p_prim->type = DM_ULP_READ_PHY_REQ;
    p_prim->phandle = phandle;
    p_prim->tp_addrt = *p_tp_bd_addr;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

