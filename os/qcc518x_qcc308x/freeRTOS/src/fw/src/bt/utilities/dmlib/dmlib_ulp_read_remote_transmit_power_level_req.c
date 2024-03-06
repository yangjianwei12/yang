/*******************************************************************************

Copyright (C) 2020 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_read_remote_transmit_power_level_req
 *
 *  DESCRIPTION
 *      Build and send a dm_read_remote_transmit_power_level_req primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_read_remote_transmit_power_level_req(
    TP_BD_ADDR_T *p_tp_bd_addr,
    uint8_t phy,
    DM_UPRIM_T **pp_prim)
{
    DM_ULP_READ_REMOTE_TRANSMIT_POWER_LEVEL_REQ_T *p_prim =
        pnew(DM_ULP_READ_REMOTE_TRANSMIT_POWER_LEVEL_REQ_T);

    p_prim->type = DM_ULP_READ_REMOTE_TRANSMIT_POWER_LEVEL_REQ;
    p_prim->tp_addrt = *p_tp_bd_addr;
    p_prim->phy = phy;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *)p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}
