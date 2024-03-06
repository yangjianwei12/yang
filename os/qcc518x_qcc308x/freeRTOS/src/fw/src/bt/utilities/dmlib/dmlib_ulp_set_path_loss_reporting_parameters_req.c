/*******************************************************************************

Copyright (C) 2020 - 2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_set_path_loss_reporting_parameters_req
 *
 *  DESCRIPTION
 *      Build and send a dm_set_path_loss_reporting_parameters_req primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_set_path_loss_reporting_parameters_req(
    TP_BD_ADDR_T    *p_tp_bd_addr,
    int8_t          high_threshold,
    int8_t          high_hysteresis,
    int8_t          low_threshold,
    int8_t          low_hysteresis,
    uint16_t        min_time_spent,
    DM_UPRIM_T      **pp_prim)
{
    DM_ULP_SET_PATH_LOSS_REPORTING_PARAMETERS_REQ_T *p_prim =
        pnew(DM_ULP_SET_PATH_LOSS_REPORTING_PARAMETERS_REQ_T);

    p_prim->type = DM_ULP_SET_PATH_LOSS_REPORTING_PARAMETERS_REQ;
    p_prim->tp_addrt = *p_tp_bd_addr;
    p_prim->high_threshold = high_threshold;
    p_prim->high_hysteresis = high_hysteresis;
    p_prim->low_threshold = low_threshold;
    p_prim->low_hysteresis = low_hysteresis;
    p_prim->min_time_spent = min_time_spent;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *)p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}
