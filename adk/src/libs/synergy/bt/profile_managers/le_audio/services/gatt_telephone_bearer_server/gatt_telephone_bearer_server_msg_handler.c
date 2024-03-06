/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_telephone_bearer_server_msg_handler.h"
#include "gatt_telephone_bearer_server_private.h"
#include "gatt_telephone_bearer_server_access.h"
#include "gatt_telephone_bearer_server_debug.h"

/******************************************************************************/

static void populateTbsServerWriteAccessInd(CsrBtGattDbAccessWriteInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_WRITE;

    if (msg->writeUnit)
    {
        message->size_value = msg->writeUnit->valueLength;
        message->offset = msg->writeUnit->offset;
        CsrMemCpy(message->value, msg->writeUnit->value, msg->writeUnit->valueLength);
    }
    else
    {
        message->size_value = 0;
        message->offset = 0;
        message->value = NULL;
    }

}

static void populateTbsServerReadAccessInd(CsrBtGattDbAccessReadInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_READ;
    message->offset = msg->offset;
}

void gattTbsServerMsgHandler(void* task, MsgId id, Msg msg)
{
    GTBS_T *telephoneBearerServer = (GTBS_T *)task;
    CsrBtGattDbAccessWriteInd *ind;
    GATT_MANAGER_SERVER_ACCESS_IND_T *message;

    switch (id)
    {
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)CsrPmemZalloc(
                                   sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            populateTbsServerReadAccessInd((CsrBtGattDbAccessReadInd*)msg, message);
            tbsHandleAccessIndication(
                    telephoneBearerServer,
                    (GATT_MANAGER_SERVER_ACCESS_IND_T *)message
                    );
            CsrPmemFree(message);
            break;
        }
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            ind = (CsrBtGattDbAccessWriteInd*)msg;
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)CsrPmemZalloc(
                                 sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            message->value = (uint8*)CsrPmemZalloc(ind->writeUnit->valueLength);
            populateTbsServerWriteAccessInd((CsrBtGattDbAccessWriteInd*)ind, message);
            tbsHandleAccessIndication(
                    telephoneBearerServer,
                    (GATT_MANAGER_SERVER_ACCESS_IND_T *)message
                    );
            CsrPmemFree(message->value);
            CsrPmemFree(message);
            break;
        }
        case GATT_TELEPHONE_BEARER_SERVER_INTERNAL_SIGNAL_STRENGTH_TIMER:
        {
            /* Notify Clients if the latest value has not been notified */
            if(telephoneBearerServer->data.signalStrengthNotified != telephoneBearerServer->data.signalStrength)
            {
                gattTelephoneBearerServerNotifySignalStrength(telephoneBearerServer, NULL);
            }
            else
            {   /* Finished notifying then the reporting flag can be cleared */
                telephoneBearerServer->data.signalStrengthTimerFlag = FALSE;
            }
        }
        break;

        default:
        {
            GATT_TBS_SERVER_WARNING(
                        "GTBS: GATT Manager message 0x%04x not handled\n",
                        id
                        );
            break;
        }
    } /* switch */
    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}

void tbsServerMsgHandler(void** gash)
{
    uint16 eventClass = 0;
    void* msg = NULL;
    CsrBtGattPrim id;
    ServiceHandle srvcHndl = 0;

    GTBS_T* tbs = NULL;

    srvcHndl = *((ServiceHandle*)*gash);
    tbs = ServiceHandleGetInstanceData(srvcHndl);

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                id = *(CsrBtGattPrim*)msg;
                if(tbs)
                    gattTbsServerMsgHandler(tbs, id, msg);
                break;
            }
            case TBS_SERVER_PRIM:
            {
                id = *(GattTelephoneBearerServerInternal*)msg;
                if(tbs)
                    gattTbsServerMsgHandler(tbs, id, msg);
                break;
            }
            default:
                break;
         }
        SynergyMessageFree(eventClass, msg);
    }
}



