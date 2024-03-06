/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/

#include "gatt_transport_discovery_server_private.h"
#include "gatt_transport_discovery_server_access.h"
#include "gatt_transport_discovery_server_msg_handler.h"
#include "csr_bt_cm_lib.h"
#include "csr_util.h"



/***************************************************************************
NAME
    tdsServerControlPointAccess

DESCRIPTION
    Deals with access of the HANDLE_TRANSPORT_DISCOVERY_CONTROL_POINT handle.
*/
static void tdsServerControlPointAccess(GTDS_T *tds, const GATT_MANAGER_SERVER_ACCESS_IND_T *accessInd)
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        /* Read of TDS service control point not allowed. */
        CsrBtGattDbReadAccessResSend(tds->gattId,
                                     accessInd->cid,
                                     accessInd->handle,
                                     CSR_BT_GATT_ACCESS_RES_READ_NOT_PERMITTED,
                                     0,
                                     NULL);

    }
    else if (accessInd->flags & ATT_ACCESS_WRITE)
    {
        /* On a Write, send new control point value to the app */
        MAKE_TDS_MESSAGE_WITH_LEN(GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CONTROL_POINT_IND_T, accessInd->size_value - 1);
        CsrMemSet(message, 0, sizeof(GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CONTROL_POINT_IND_T));
        message->tds = tds;
        message->cid = accessInd->cid;
        message->handle = accessInd->handle;
        message->size_value = accessInd->size_value;
        memmove(message->value, accessInd->value, message->size_value);
        
        TdsMessageSend(tds->app_task, GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CONTROL_POINT_IND, message);
        /* The app will handle the response to the request, whether successful or resulting in an error. */
    }
}


/***************************************************************************
NAME
    tdsServerClientConfigAccess

DESCRIPTION
    Deals with access of the HANDLE_TRANSPORT_DISCOVERY_SERVICE_CLIENT_CONFIG handle.
*/
static void tdsServerClientConfigAccess(GTDS_T *tds, const GATT_MANAGER_SERVER_ACCESS_IND_T *accessInd)
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        /* On a Read, ask the app for current client config value */
        MAKE_TDS_MESSAGE(GATT_TRANSPORT_DISCOVERY_SERVER_READ_CLIENT_CONFIG_IND_T);
        CsrMemSet(message, 0, sizeof(GATT_TRANSPORT_DISCOVERY_SERVER_READ_CLIENT_CONFIG_IND_T));

        message->tds = tds;     /* Pass the instance which can be returned in the response */
        GATT_TDS_SERVER_DEBUG_INFO(("\nTDS: %s cid=%d\n", __FUNCTION__, accessInd->cid));

        message->cid = accessInd->cid;                 /* Pass the CID so the client can be identified */
        TdsMessageSend(tds->app_task, GATT_TRANSPORT_DISCOVERY_SERVER_READ_CLIENT_CONFIG_IND, message);
    }
    else if (accessInd->flags & ATT_ACCESS_WRITE)
    {
        if (accessInd->size_value == GATT_CLIENT_CONFIG_NUM_OCTETS)
        {
            /* On a Write, send new client config value to the app */
            MAKE_TDS_MESSAGE(GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CLIENT_CONFIG_IND_T);
            CsrMemSet(message, 0, sizeof(GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CLIENT_CONFIG_IND_T));

            message->tds = tds;
            message->cid = accessInd->cid;
            message->config_value = (accessInd->value[0] & 0xFF) | ((accessInd->value[1] << 8) & 0xFF00);
            GATT_TDS_SERVER_DEBUG_INFO(("\nTDS: %s cid=%d config value=%d\n", __FUNCTION__, accessInd->cid, message->config_value));

            TdsMessageSend(tds->app_task, GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CLIENT_CONFIG_IND, message);
            /* Library response to the access request */
            sendTdsServerWriteAccessRsp(
                                     tds->gattId,
                                     accessInd->cid,
                                     accessInd->handle,
                                     CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                     0,
                                     NULL);
        }
        else
        {
            sendTdsServerWriteAccessRsp(
                                 tds->gattId,
                                 accessInd->cid,
                                 accessInd->handle,
                                 CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH,
                                 0,
                                 NULL);

        }
    }
}


/***************************************************************************
NAME
    tdsServerBredrHandoverDataAccess

DESCRIPTION
    Deals with access of the HANDLE_TRANSPORT_DISCOVERY_BREDR_HANDOVER_DATA handle.
*/
static void tdsServerBredrHandoverDataAccess(GTDS_T *tds, const GATT_MANAGER_SERVER_ACCESS_IND_T *accessInd)
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        MAKE_TDS_MESSAGE(GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_IND_T);
        CsrMemSet(message, 0, sizeof(GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_IND_T));

        message->tds = tds;     /* Pass the instance which can be returned in the response */
        GATT_TDS_SERVER_DEBUG_INFO(("\nTDS: %s cid=%d\n", __FUNCTION__, accessInd->cid));

        message->cid = accessInd->cid;                 /* Pass the CID so the client can be identified */
        TdsMessageSend(tds->app_task, GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_IND, message);
    }
    else
    {
        sendTdsServerWriteAccessRsp(tds->gattId,
                                    accessInd->cid,
                                    accessInd->handle,
                                    CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED,
                                    0,
                                    NULL);
    }
}

/***************************************************************************
NAME
    tdsServerSigDataAccess

DESCRIPTION
    Deals with access of the HANDLE_TRANSPORT_DISCOVERY_SIG_DATA handle.
*/
static void tdsServerSigDataAccess(GTDS_T *tds, const GATT_MANAGER_SERVER_ACCESS_IND_T *accessInd)
{
    /* Neither write nor read Operation allowed on TDS server SIG Data. */
    CsrBtGattDbReadAccessResSend(tds->gattId,
                                 accessInd->cid,
                                 accessInd->handle,
                                 CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED,
                                 0,
                                 NULL);
}

/***************************************************************************
NAME
    tdsServerDiscoveryBredrTransportBlockDataAccess

DESCRIPTION
    Deals with access of the HANDLE_TRANSPORT_DISCOVERY_BREDR_TRANSPORT_BLOCK_DATA  handle.
*/
static void tdsServerDiscoveryBredrTransportBlockDataAccess(GTDS_T *tds, const GATT_MANAGER_SERVER_ACCESS_IND_T *accessInd)
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        MAKE_TDS_MESSAGE(GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_IND_T);
        CsrMemSet(message, 0, sizeof(GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_HANDOVER_DATA_IND_T));

        message->tds = tds;     /* Pass the instance which can be returned in the response */
        GATT_TDS_SERVER_DEBUG_INFO(("\nTDS: %s cid=%d\n", __FUNCTION__, accessInd->cid));

        message->cid = accessInd->cid;                 /* Pass the CID so the client can be identified */
        TdsMessageSend(tds->app_task, GATT_TRANSPORT_DISCOVERY_SERVER_READ_BREDR_TRANSPORT_BLOCK_DATA_IND, message);
    }
    else
    {
        sendTdsServerWriteAccessRsp(tds->gattId,
                                    accessInd->cid,
                                    accessInd->handle,
                                    CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED,
                                    0,
                                    NULL);
    }
}


void handleTdsAccess(GTDS_T* tdsServer, const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd)
{
    switch (accessInd->handle)
    {
        case HANDLE_TRANSPORT_DISCOVERY_CONTROL_POINT:
        {
            tdsServerControlPointAccess(tdsServer, accessInd);
        }
        break;

        case HANDLE_TRANSPORT_DISCOVERY_SERVICE_CLIENT_CONFIG:
        {
            tdsServerClientConfigAccess(tdsServer, accessInd);
        }
        break;

        case HANDLE_TRANSPORT_DISCOVERY_BREDR_HANDOVER_DATA:
        {
            tdsServerBredrHandoverDataAccess(tdsServer, accessInd);
        }
        break;

        case HANDLE_TRANSPORT_DISCOVERY_SIG_DATA:
        {
            tdsServerSigDataAccess(tdsServer, accessInd);
        }
        break;

        case HANDLE_TRANSPORT_DISCOVERY_BREDR_TRANSPORT_BLOCK_DATA:
        {
            tdsServerDiscoveryBredrTransportBlockDataAccess(tdsServer, accessInd);
        }
        break;

        default:
        {
            /* Respond to invalid handles */
           sendTdsServerReadAccessRsp(
                    tdsServer->gattId,
                    accessInd->cid,
                    accessInd->handle,
                    CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE,
                    0,
                    NULL);
        }
        break;
    }
}

