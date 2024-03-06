/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #58 $
******************************************************************************/

#include "gatt_transport_discovery_server_msg_handler.h"
#include "gatt_transport_discovery_server_private.h"
#include "gatt_transport_discovery_server_access.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_client_util_lib.h"

/***************************************************************************/
static void populateTdsServerWriteAccessInd(CsrBtGattDbAccessWriteInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->writeUnit->attrHandle;
    message->flags = ATT_ACCESS_WRITE;
    message->size_value = msg->writeUnit->valueLength;
    message->offset = msg->writeUnit->offset;
    CsrMemCpy(message->value, msg->writeUnit->value, msg->writeUnit->valueLength);
}

static void populateTdsServerReadAccessInd(CsrBtGattDbAccessReadInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_READ;
    message->offset = msg->offset;
}

/***************************************************************************
NAME
    sendTdsServerIndication

DESCRIPTION
    Sends an indication to the GATT Manager library.
*/
void sendTdsServerIndication(CsrBtGattId gattId,
                             connection_id_t cid,
                             uint16 handle,
                             uint16 sizeValue,
                             uint8 *const value)
{
    uint8* data;
    data = (uint8*)CsrPmemZalloc(sizeof(uint8)*sizeValue);
    CsrMemCpy(data, value, sizeValue);

    CsrBtGattIndicationEventReqSend(
             gattId,
             cid,
             handle,
             sizeValue,
             data);

}

/***************************************************************************
NAME
    sendTdsServerReadAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
void sendTdsServerReadAccessRsp(CsrBtGattId gattId,
                             connection_id_t cid,
                             uint16 handle,
                             uint16 result,
                             uint16 sizeValue,
                             uint8 *const value)
{
    uint8* data;
    data = (uint8*)CsrPmemZalloc(sizeof(uint8)*sizeValue);
    CsrMemCpy(data, value, sizeValue);
    CsrBtGattDbReadAccessResSend(gattId, cid, handle, result, sizeValue, data);
}

/***************************************************************************
NAME
    sendTdsServerWriteAccessRsp

DESCRIPTION
    Send an access response to the GATT  library.
*/
void sendTdsServerWriteAccessRsp(CsrBtGattId gattId,
                                  connection_id_t cid,
                                  uint16 handle,
                                  uint16 result,
                                  uint16 sizeValue,
                                  uint8* value)
{
    GATT_TDS_SERVER_DEBUG_INFO((" \n TDS Server: sendtdsServerWriteAccessRsp, "
                                                      "Send Write DB Access Response \n"));
    CSR_UNUSED(sizeValue);
    CSR_UNUSED(value);
    CsrBtGattDbWriteAccessResSend(gattId,
                                  cid,
                                  handle,
                                 (CsrBtGattDbAccessRspCode)result);
}

/***************************************************************************/
void sendTdsServerClientConfigReadAccessRsp(const GTDS_T *tds, connection_id_t cid, uint16 clientConfig)
{
    uint8 configResp[GATT_CLIENT_CONFIG_NUM_OCTETS];
    
    configResp[0] = clientConfig & 0xFF;
    configResp[1] = (clientConfig >> 8) & 0xFF;
    GATT_TDS_SERVER_DEBUG_INFO((" \n TDS %s gattId=%d cid =%d", __FUNCTION__, tds->gattId, cid));

    sendTdsServerReadAccessRsp(
             tds->gattId,
             cid,
             HANDLE_TRANSPORT_DISCOVERY_SERVICE_CLIENT_CONFIG,
             CSR_BT_GATT_ACCESS_RES_SUCCESS,
             GATT_CLIENT_CONFIG_NUM_OCTETS,
             configResp);
}

/***************************************************************************/
void sendTdsServerControlPointIndication(const GTDS_T *tds, connection_id_t cid, uint16 tdsIndSize, uint8 *tdsIndData)
{
    GATT_TDS_SERVER_DEBUG_INFO(("\nTDS %s gattid=%d", __FUNCTION__, tds->gattId));

    sendTdsServerIndication(
             tds->gattId,
             cid,
             HANDLE_TRANSPORT_DISCOVERY_CONTROL_POINT,
             tdsIndSize,
             tdsIndData);
}

void tdsServerGattMsgHandler(void* task, MsgId id, Msg msg)
{
    GTDS_T *tds = (GTDS_T*)task;
    GATT_MANAGER_SERVER_ACCESS_IND_T* message;
    CsrBtGattDbAccessWriteInd* ind;
    GATT_TDS_SERVER_DEBUG_INFO(("TDS: GATT Manager Server Msg 0x%x \n",id));

    switch (id)
    {
        /* Read/write access to characteristic */
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)CsrPmemZalloc(
                                          sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            populateTdsServerReadAccessInd((CsrBtGattDbAccessReadInd*)msg, message);
            handleTdsAccess(tds, message);
            CsrPmemFree(message);
        }
        break;
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            ind = (CsrBtGattDbAccessWriteInd*)msg;
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)CsrPmemZalloc(
                                          sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            message->value = (uint8*)CsrPmemZalloc(ind->writeUnit->valueLength);
            populateTdsServerWriteAccessInd(ind, message);
            handleTdsAccess(tds, message);
            CsrPmemFree(message->value);
            CsrPmemFree(message);
        }
        break;
        case CSR_BT_GATT_CLIENT_INDICATION_IND:
        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
        {
            /* Library just absorbs confirmation messages */
        }
        break;
        default:
        {
            /* Unrecognised GATT  message */
            GATT_TDS_SERVER_DEBUG_INFO(("TDS: GATT Manager Server Msg 0x%x not handled\n",id));
        }
        break;
    }
    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}

/*Scheduler Task msg handler for TDS server*/

void TdsServerMsgHandler(void **gash)
{
    uint16_t eventClass = 0;
    void* msg = NULL;
    MsgId id;
    GTDS *tdsInst = NULL;
    GTDS_T *tds;

    tdsInst = (GTDS*)*gash;
    tds = tdsInst->tds;
    GATT_TDS_SERVER_DEBUG_INFO(("TDS: %s tds = %u\n",__FUNCTION__, (unsigned int)tds));
    if(CsrSchedMessageGet(&eventClass, &msg))
    {
        switch(eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                id = *(MsgId*)msg;
                tdsServerGattMsgHandler(tds, id, msg);
            }
            break;

            case TDS_SERVER_PRIM:
            default:
                break;
        }
    }
    SynergyMessageFree(eventClass, msg);
}

