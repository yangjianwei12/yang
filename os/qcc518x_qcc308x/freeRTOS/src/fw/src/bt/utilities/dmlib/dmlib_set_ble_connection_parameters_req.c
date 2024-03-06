/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_set_ble_connection_parameters_req
 *
 *  DESCRIPTION
 *      Build and send a DM_SET_BLE_CONNECTION_PARAMETERS_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_set_ble_connection_parameters_req(uint16_t scan_interval,
                                         uint16_t scan_window,
                                         uint16_t conn_interval_min,
                                         uint16_t conn_interval_max,
                                         uint16_t conn_latency,
                                         uint16_t supervision_timeout,
                                         uint16_t conn_attempt_timeout,
                                         uint16_t conn_latency_max,
                                         uint16_t supervision_timeout_min,
                                         uint16_t supervision_timeout_max,
                                         uint8_t own_address_type,
                                         DM_UPRIM_T  **pp_prim
                                        )
{
    DM_SET_BLE_CONNECTION_PARAMETERS_REQ_T *p_prim
                                    = zpnew(DM_SET_BLE_CONNECTION_PARAMETERS_REQ_T);
    
    p_prim->type                 = DM_SET_BLE_CONNECTION_PARAMETERS_REQ;
    p_prim->scan_interval        = scan_interval;
    p_prim->scan_window          = scan_window;
    p_prim->conn_interval_min    = conn_interval_min;
    p_prim->conn_interval_max    = conn_interval_max;
    p_prim->conn_latency         = conn_latency;
    p_prim->supervision_timeout  = supervision_timeout;
    p_prim->conn_attempt_timeout = conn_attempt_timeout;
    p_prim->conn_latency_max     = conn_latency_max;
    p_prim->supervision_timeout_min = supervision_timeout_min;
    p_prim->supervision_timeout_max = supervision_timeout_max;
    p_prim->own_address_type = own_address_type;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

