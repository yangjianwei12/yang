/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_bass_server_msg_handler.h"
#include "gatt_bass_server_access.h"
#include "gatt_bass_server_debug.h"
#include "gatt_bass_server_private.h"

/*****************************************************************************/
static void populateBassServerWriteAccessInd(CsrBtGattDbAccessWriteInd* msg,
                                             CsrBtGattAccessInd* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_WRITE;
    message->numWriteUnits = msg->writeUnitCount;
    message->writeUnit = msg->writeUnit;
}

static void populateBassServerReadAccessInd(CsrBtGattDbAccessReadInd* msg,
                                            CsrBtGattAccessInd* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_READ;
    message->offset = msg->offset;
}


/****************************************************************************/
void bassServerMsgHandler(void* task, MsgId id, Msg payload)
{
    GBASSSS *bass_server = (GBASSSS *)task;
    CsrBtGattDbAccessWriteInd *ind;
    CsrBtGattAccessInd *message;

    switch (id)
    {
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            message = (CsrBtGattAccessInd *)CsrPmemZalloc(sizeof(CsrBtGattAccessInd));
            populateBassServerReadAccessInd((CsrBtGattDbAccessReadInd*)payload, message);
            handleBassServerAccess(bass_server, (CsrBtGattAccessInd *)message);
            CsrPmemFree(message);
            break;
        }
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            ind = (CsrBtGattDbAccessWriteInd*)payload;
            message = (CsrBtGattAccessInd *)CsrPmemZalloc(sizeof(CsrBtGattAccessInd));
            populateBassServerWriteAccessInd((CsrBtGattDbAccessWriteInd*)ind, message);
            handleBassServerAccess(bass_server, (CsrBtGattAccessInd *)message);
            CsrPmemFree(message);
            break;
        }
        case CSR_BT_GATT_CONFIG_MODE_CFM:
        {
            CsrBtGattStdCfm *cfm = (CsrBtGattStdCfm *) payload;

            if(cfm->resultCode != CSR_BT_GATT_RESULT_SUCCESS)
                GATT_BASS_SERVER_PANIC("BASS: GATT Configure mode failed!\n")
        }
        break;
        default:
        {
            /* Unrecognised GATT  message */
            GATT_BASS_SERVER_WARNING("GATT Manager Server Msg not handled\n");
        }
        break;
    }

    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, payload);
}

void GattBassServerMsgHandler(void** gash)
{
    uint16 eventClass = 0;
    void* msg = NULL;
    CsrBtGattPrim id;
    ServiceHandle srvc_hndl = 0;
    GBASSSS* bass = NULL;

    srvc_hndl = *((ServiceHandle*)*gash);
    bass = ServiceHandleGetInstanceData(srvc_hndl);

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                if(bass)
                {
                    id = *(CsrBtGattPrim*)msg;
                    bassServerMsgHandler(bass, id, msg);
                }
                break;
            }
            case BASS_SERVER_PRIM:
            {
                break;
            }
            default:
                break;
         }

        SynergyMessageFree(eventClass, msg);
    }
}

