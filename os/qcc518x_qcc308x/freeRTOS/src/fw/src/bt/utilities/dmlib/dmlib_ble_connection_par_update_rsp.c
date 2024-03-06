/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ble_connection_par_update_rsp
 *
 *  DESCRIPTION
 *      Build and send a DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_ble_connection_par_update_rsp(
    l2ca_identifier_t signal_id,
    TYPED_BD_ADDR_T bd_addrt,
    uint16_t conn_interval_min,
    uint16_t conn_interval_max,
    uint16_t conn_latency,
    uint16_t supervision_timeout,
    uint16_t result,
    DM_UPRIM_T **pp_prim
    )
{
    DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_RSP_T *prim = zpnew(DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_RSP_T);

    prim->type = DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_RSP;
    prim->signal_id = signal_id;
    prim->bd_addrt = bd_addrt;
    prim->conn_interval_min = conn_interval_min;
    prim->conn_interval_max = conn_interval_max;
    prim->conn_latency = conn_latency;
    prim->supervision_timeout = supervision_timeout;
    prim->result = result;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}
