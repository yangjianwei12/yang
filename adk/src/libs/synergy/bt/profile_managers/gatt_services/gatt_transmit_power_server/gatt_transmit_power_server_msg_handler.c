/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/


#include <stdio.h>

#include "gatt_transmit_power_server_msg_handler.h"
#include "gatt_transmit_power_server_private.h"
#include "gatt_transmit_power_server_access.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_client_util_lib.h"

/***************************************************************************/
static void populateTpsServerReadAccessInd(CsrBtGattDbAccessReadInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_READ;
    message->offset = msg->offset;
}


/***************************************************************************
NAME
    sendTpsServerReadAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
void sendTpsServerReadAccessRsp(CsrBtGattId gattId,
                             connection_id_t cid,
                             uint16 handle,
                             uint16 result,
                             uint16 size_value,
                             uint8 *const value)
{
    uint8* data;
    data = (uint8*)CsrPmemZalloc(sizeof(uint8)*size_value);
    CsrMemCpy(data, value, size_value);
    CsrBtGattDbReadAccessResSend(gattId, cid, handle, result, size_value, data);
}

/***************************************************************************
NAME
    sendTpsServerWriteAccessRsp

DESCRIPTION
    Send an access response to the GATT  library.
*/
void sendTpsServerWriteAccessRsp(CsrBtGattId gattId,
                                  connection_id_t cid,
                                  uint16 handle,
                                  uint16 result,
                                  uint16 size_value,
                                  uint8* value)
{
    GATT_TPS_SERVER_DEBUG_INFO((" \n TPS Server: sendtpsServerWriteAccessRsp, "
                                                      "Send Write DB Access Response \n"));
    CSR_UNUSED(size_value);
    CSR_UNUSED(value);
    CsrBtGattDbWriteAccessResSend(gattId,
                                  cid,
                                  handle,
                                 (CsrBtGattDbAccessRspCode)result);
}


/****************************************************************************/
static void tpsServerGattMsgHandler(void* task, MsgId id, Msg msg)
{
    GTPSS *tps = (GTPSS*)task;
    GATT_MANAGER_SERVER_ACCESS_IND_T* message;
    CsrBtGattDbAccessWriteInd* ind;
    GATT_TPS_SERVER_DEBUG_INFO(("TPS: GATT Manager Server Msg 0x%x \n",id));

    switch (id)
    {
        /* Read access to characteristic */
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)CsrPmemZalloc(
                                          sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            populateTpsServerReadAccessInd((CsrBtGattDbAccessReadInd*)msg, message);
            handleTpsAccess(tps, message, ((CsrBtGattDbAccessReadInd*)msg)->address);
            CsrPmemFree(message);
        }
        break;
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            ind = (CsrBtGattDbAccessWriteInd*)msg;

            sendTpsServerWriteAccessRsp(
                     tps->gattId,
                     ind->btConnId,
                     ind->writeUnit->attrHandle,
                     CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED,
                     0,
                     NULL);
        }
        break;

        default:
        {
            /* Unrecognised GATT  message */
            GATT_TPS_SERVER_DEBUG_INFO(("TPS: GATT Manager Server Msg 0x%x not handled\n",id));
        }
        break;
    }
    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}

static void tpsServerCMMsgHandler(void* task, MsgId id, Msg msg)
{
    GTPSS *tps = (GTPSS*)task;
    uint8 txPower;
    CsrBtTypedAddr typedAddr;
    connection_id_t cid;

    switch (id)
    {
        case CSR_BT_CM_READ_TX_POWER_LEVEL_CFM:
        {
            CsrBtCmReadTxPowerLevelCfm *readTxCfm = (CsrBtCmReadTxPowerLevelCfm *)msg;
            if(readTxCfm != NULL)
            {
                typedAddr.addr = readTxCfm->deviceAddr;
                typedAddr.type = readTxCfm->addressType;
                cid = CsrBtGattClientUtilFindConnIdByAddr(&typedAddr);

                if(readTxCfm->resultCode == CSR_BT_RESULT_CODE_SUCCESS)
                {
                    txPower = readTxCfm->powerLevel;
                    sendTpsServerReadAccessRsp(
                                        tps->gattId,
                                        cid,
                                        HANDLE_TRANSMIT_POWER_LEVEL,
                                        CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                        1,
                                        &txPower);
                }
                else
                {
                    sendTpsServerReadAccessRsp(
                                        tps->gattId,
                                        cid,
                                        HANDLE_TRANSMIT_POWER_LEVEL,
                                        CSR_BT_GATT_ACCESS_RES_UNLIKELY_ERROR,
                                        0,
                                        NULL);
                }
            }
        }
        break;
        default:
        {
            /* Ignore unrecognised messages */
        }
        break;
    }
}

/*Scheduler Task msg handler for TPS server*/

void TpsServerMsgHandler(void **gash)
{
    uint16_t eventClass = 0;
    void* msg = NULL;
    MsgId id;
    GTPSS_T *tpsServerInst = NULL;
    GTPSS *tps;

    tpsServerInst = (GTPSS_T*)*gash;
    tps = tpsServerInst->tps;

    if(CsrSchedMessageGet(&eventClass, &msg))
    {
        switch(eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                id = *(MsgId*)msg;
                tpsServerGattMsgHandler(tps, id, msg);
                break;
            }
            case CSR_BT_CM_PRIM:
            {
                id = *(MsgId*)msg;
                tpsServerCMMsgHandler(tps, id, msg);
                break;
            }

            case TPS_SERVER_PRIM:
                break;
            default:
                break;
        }
    }
    SynergyMessageFree(eventClass, msg);
}

