/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ble_update_connection_update_req
 *
 *  DESCRIPTION
 *      Build and send a DM_BLE_UPDATE_CONNECTION_PARAMETERS_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_ble_update_connection_update_req(
    TYPED_BD_ADDR_T *addrt,
    uint16_t conn_interval_min,
    uint16_t conn_interval_max,
    uint16_t conn_latency,
    uint16_t supervision_timeout,
    uint16_t minimum_ce_length,
    uint16_t maximum_ce_length,
    DM_UPRIM_T **pp_prim
    )
{
    DM_BLE_UPDATE_CONNECTION_PARAMETERS_REQ_T *prim = zpnew(DM_BLE_UPDATE_CONNECTION_PARAMETERS_REQ_T);

    prim->common.op_code = DM_BLE_UPDATE_CONNECTION_PARAMETERS_REQ;
    prim->conn_interval_min = conn_interval_min;
    prim->conn_interval_max = conn_interval_max;
    prim->conn_latency = conn_latency;
    prim->supervision_timeout = supervision_timeout;
    prim->minimum_ce_length = minimum_ce_length;
    prim->maximum_ce_length = maximum_ce_length;

    tbdaddr_copy(&prim->addrt, addrt);

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

