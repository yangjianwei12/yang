/*******************************************************************************

Copyright (C) 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_set_path_loss_reporting_enable_req
 *
 *  DESCRIPTION
 *      Build and send a dm_set_path_loss_reporting_enable_req primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_set_path_loss_reporting_enable_req(
    TP_BD_ADDR_T    *p_tp_bd_addr,
    uint8_t         enable,
    DM_UPRIM_T      **pp_prim)
{
    DM_ULP_SET_PATH_LOSS_REPORTING_ENABLE_REQ_T *p_prim =
        pnew(DM_ULP_SET_PATH_LOSS_REPORTING_ENABLE_REQ_T);

    p_prim->type = DM_ULP_SET_PATH_LOSS_REPORTING_ENABLE_REQ;
    p_prim->tp_addrt = *p_tp_bd_addr;
    p_prim->enable = enable;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *)p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}
