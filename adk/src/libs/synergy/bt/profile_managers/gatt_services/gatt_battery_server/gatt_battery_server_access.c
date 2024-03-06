/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_bt_gatt_lib.h"

#include "gatt_battery_server_private.h"
#include "gatt_battery_server_access.h"
#include "gatt_battery_server_db.h"

/***************************************************************************
NAME
    sendBatteryAccessRsp

DESCRIPTION
    Send an access response to the GATT library.
*/
static void sendBatteryAccessRsp(CsrBtGattId gattId,
                                    uint32 btConnId,
                                    uint16 handle,
                                    uint16 result,
                                    uint16 size_value,
                                    uint8 *value)
{
    CsrBtGattDbReadAccessResSend(gattId,
                            btConnId,
                            handle,
                            result,
                            size_value,
                            value);
}

/***************************************************************************
NAME
    sendBatteryWriteAccessErrorRsp

DESCRIPTION
    Send a write access error response to the GATT library.
*/
static void sendBatteryWriteAccessErrorRsp(const GBASS *battery_server, const CsrBtGattDbAccessWriteInd *access_ind, uint16 error)
{
    CsrBtGattDbWriteAccessResSend(battery_server->gattId,
                            access_ind->btConnId,
                            access_ind->attrHandle,
                            error);
}

/***************************************************************************
NAME
    sendBatteryReadAccessErrorRsp

DESCRIPTION
    Send a read access error response to the GATT library.
*/
static void sendBatteryReadAccessErrorRsp(const GBASS *battery_server, const CsrBtGattDbAccessReadInd *access_ind, uint16 error)
{
    CsrBtGattDbReadAccessResSend(battery_server->gattId,
                            access_ind->btConnId,
                            access_ind->attrHandle,
                            error,
                            0,
                            NULL);
}

/***************************************************************************
NAME
    batteryLevelAccess

DESCRIPTION
    Deals with access of the HANDLE_BATTERY_LEVEL handle.
*/
static void batteryLevelReadAccess(GBASS *battery_server, const CsrBtGattDbAccessReadInd *access_ind)
{
    /* Send read level message to app_task so it can return the current level */
    MAKE_BATTERY_MESSAGE(GATT_BATTERY_SERVER_READ_LEVEL_IND);
    message->battery_server = battery_server;     /* Pass the instance which can be returned in the response */
    message->cid = access_ind->btConnId;                 /* Pass the CID which can be returned in the response */
    BasMessageSend(battery_server->app_task, GATT_BATTERY_SERVER_READ_LEVEL_IND, message);
}

/***************************************************************************
NAME
    batteryClientConfigWriteAccess

DESCRIPTION
    Deals with access of the HANDLE_BATTERY_LEVEL_CLIENT_CONFIG handle.
*/
static void batteryClientConfigWriteAccess(GBASS *battery_server, const CsrBtGattDbAccessWriteInd *access_ind)
{
    if (access_ind->writeUnit[0].valueLength == GATT_CLIENT_CONFIG_OCTET_SIZE)
    {
        /* On a Write, send new client config value to the app */
        MAKE_BATTERY_MESSAGE(GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND);
        message->battery_server = battery_server;
        message->cid = access_ind->btConnId;
        message->config_value = (access_ind->writeUnit[0].value[0] & 0xFF) | ((access_ind->writeUnit[0].value[1] << 8) & 0xFF00);
        BasMessageSend(battery_server->app_task, GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND, message);

        CsrBtGattDbWriteAccessResSend(battery_server->gattId,
                            access_ind->btConnId,
                            access_ind->attrHandle,
                            CSR_BT_GATT_ACCESS_RES_SUCCESS);
    }
    else
    {
        sendBatteryWriteAccessErrorRsp(battery_server, access_ind, CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH);
    }
}

/***************************************************************************
NAME
    batteryClientConfigReadAccess

DESCRIPTION
    Deals with access of the HANDLE_BATTERY_LEVEL_CLIENT_CONFIG handle.
*/
static void batteryClientConfigReadAccess(GBASS *battery_server, const CsrBtGattDbAccessReadInd *access_ind)
{
    /* On a Read, ask the app for current client config value */
    MAKE_BATTERY_MESSAGE(GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND);
    message->battery_server = battery_server;     /* Pass the instance which can be returned in the response */
    message->cid = access_ind->btConnId;                 /* Pass the CID so the client can be identified */
    BasMessageSend(battery_server->app_task, GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND, message);
}

/***************************************************************************
NAME
    batteryPresentationAccess

DESCRIPTION
    Deals with access of the HANDLE_BATTERY_LEVEL_PRESENTATION handle.
*/
static void batteryPresentationReadAccess(GBASS *battery_server, const CsrBtGattDbAccessReadInd *access_ind)
{
    /* Send read level message to app_task so it can return the current level */
    MAKE_BATTERY_MESSAGE(GATT_BATTERY_SERVER_READ_PRESENTATION_IND);        
    message->battery_server = battery_server;     /* Pass the instance which can be returned in the response */
    message->cid = access_ind->btConnId;                 /* Pass the CID which can be returned in the response */
    BasMessageSend(battery_server->app_task, GATT_BATTERY_SERVER_READ_PRESENTATION_IND, message);
}

/***************************************************************************/
void handleBatteryServerWriteAccess(GBASS *battery_server, const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 handle = access_ind->attrHandle;

    switch (handle)
    {   
        case HANDLE_BATTERY_LEVEL_CLIENT_CONFIG:
            if (battery_server->notifications_enabled)
            {
                batteryClientConfigWriteAccess(battery_server, access_ind);
            }
            else
            {
                /* Handle shouldn't be accessed if notifications disabled */
                sendBatteryWriteAccessErrorRsp(battery_server, access_ind, CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
            }
            break;
        default:
            /* Respond to invalid handles */
            sendBatteryWriteAccessErrorRsp(battery_server, access_ind, CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
            break;
    }
}

/***************************************************************************/
void handleBatteryServerReadAccess(GBASS *battery_server, const CsrBtGattDbAccessReadInd *access_ind)
{
    uint16 handle = access_ind->attrHandle;

    switch (handle)
    {   
        case HANDLE_BATTERY_LEVEL:
        {
            batteryLevelReadAccess(battery_server, access_ind);
        }
        break;
        
        case HANDLE_BATTERY_LEVEL_CLIENT_CONFIG:
        {
            if (battery_server->notifications_enabled)
            {
                batteryClientConfigReadAccess(battery_server, access_ind);
            }
            else
            {
                /* Handle shouldn't be accessed if notifications disabled */
                sendBatteryReadAccessErrorRsp(battery_server, access_ind, CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
            }
        }
        break;
        
        case HANDLE_BATTERY_LEVEL_PRESENTATION:
        {
            batteryPresentationReadAccess(battery_server, access_ind);
        }
        break;
        
        default:
        {
            /* Respond to invalid handles */
            sendBatteryReadAccessErrorRsp(battery_server, access_ind, CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
        }
        break;
    }
}
/***************************************************************************/
void sendBatteryLevelAccessRsp(const GBASS *battery_server, uint32 cid, uint8 battery_level, uint16 result)
{
    uint8 *value = (uint8*) CsrPmemAlloc(GATT_BATTERY_LEVEL_OCTET_SIZE);
    memcpy(value, &battery_level, GATT_BATTERY_LEVEL_OCTET_SIZE);
    sendBatteryAccessRsp(battery_server->gattId, cid, HANDLE_BATTERY_LEVEL, result, GATT_BATTERY_LEVEL_OCTET_SIZE, value);
}


/***************************************************************************/
void sendBatteryConfigAccessRsp(const GBASS *battery_server, uint32 cid, uint16 client_config)
{
    uint8 config_resp[GATT_CLIENT_CONFIG_OCTET_SIZE];
    uint8 *value = NULL;

    config_resp[0] = client_config & 0xFF;
    config_resp[1] = (client_config >> 8) & 0xFF;

    value = (uint8*) CsrPmemAlloc(GATT_CLIENT_CONFIG_OCTET_SIZE);
    memcpy(value, config_resp, GATT_CLIENT_CONFIG_OCTET_SIZE);
    sendBatteryAccessRsp(battery_server->gattId, cid, HANDLE_BATTERY_LEVEL_CLIENT_CONFIG, CSR_BT_GATT_ACCESS_RES_SUCCESS, GATT_CLIENT_CONFIG_OCTET_SIZE, value);
}


/***************************************************************************/
void sendBatteryPresentationAccessRsp(const GBASS *battery_server, uint32 cid, uint8 name_space, uint16 description)
{
    uint8 presentation[GATT_PRESENTATION_OCTET_SIZE];
    uint8 *value = NULL;

    /* Fill in Presentation Attribute Value */
    /* Format - 1 octet */
    presentation[0] = 0x04; /* unsigned 8-bit integer */
    /* Exponent - 1 octet */
    presentation[1] = 0; /* actual value = characteristic value */
    /* Unit - 2 octets */
    presentation[2] = 0xAD; /* % lower 8-bits */
    presentation[3] = 0x27; /* % upper 8-bits */
    /* Name Space - 1 octet */
    presentation[4] = name_space;
    /* Description - 2 octets */
    presentation[5] = description & 0xFF;
    presentation[6] = (description >> 8) & 0xFF;
    /* Send complete presentation response */

    value = (uint8*) CsrPmemAlloc(GATT_PRESENTATION_OCTET_SIZE);
    memcpy(value, presentation, GATT_PRESENTATION_OCTET_SIZE);
    sendBatteryAccessRsp(battery_server->gattId, cid, HANDLE_BATTERY_LEVEL_PRESENTATION, CSR_BT_GATT_ACCESS_RES_SUCCESS, GATT_PRESENTATION_OCTET_SIZE, value);
}
