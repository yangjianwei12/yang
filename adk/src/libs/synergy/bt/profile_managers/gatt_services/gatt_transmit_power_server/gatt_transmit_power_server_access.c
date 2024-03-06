/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#include "gatt_transmit_power_server_private.h"
#include "gatt_transmit_power_server_access.h"
#include "gatt_transmit_power_server_msg_handler.h"
#include "csr_bt_cm_lib.h"

/***************************************************************************
NAME
    handle_transmit_power_level_access_ind

DESCRIPTION
    Handles the GATT_MANAGER_SERVER_ACCESS_IND_T message sent to the HANDLE_TRANSMIT_POWER_LEVEL handle.
*/
static void tpsTransmitPowerLevelAccess(GTPSS* tps_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind, CsrBtTypedAddr address)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        /* Send Tx Power Level Read Request to CM */
        CsrBtCmReadTxPowerLevelReqSend(tps_server->lib_task,
                                       address.addr,
                                       address.type,
                                       CSR_BT_TRANSPORT_LE,
                                       0);
    }
    else
    {
        /* Other requests are not permitted */
        CsrBtGattDbReadAccessResSend(tps_server->gattId,
                                                 access_ind->cid,
                                                 access_ind->handle,
                                                 CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED,
                                                 0,
                                                 NULL);

    }
}

void handleTpsAccess(GTPSS* tps_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind, CsrBtTypedAddr address)
{
    switch (access_ind->handle)
    {
        case HANDLE_TRANSMIT_POWER_LEVEL:
        {
            tpsTransmitPowerLevelAccess(tps_server, access_ind, address);
        }
        break;

        default:
        {
            /* Respond to invalid handles */
           sendTpsServerReadAccessRsp(
                    tps_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE,
                    0,
                    NULL);
        }
        break;
    }
}
