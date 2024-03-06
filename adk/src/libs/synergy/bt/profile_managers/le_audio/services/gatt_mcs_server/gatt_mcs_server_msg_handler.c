/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_mcs_server_msg_handler.h"
#include "gatt_mcs_server_access.h"
#include "gatt_mcs_server_debug.h"

/******************************************************************************/

static void populateMcsServerWriteAccessInd(CsrBtGattDbAccessWriteInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_WRITE;
    message->size_value = msg->writeUnit->valueLength;
    message->offset = msg->writeUnit->offset;
    CsrMemCpy(message->value, msg->writeUnit->value, msg->writeUnit->valueLength);
}

static void populateMcsServerReadAccessInd(CsrBtGattDbAccessReadInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_READ;
    message->offset = msg->offset;
}

void mcsServerMsgHandler(void* task, MsgId id, Msg msg)
{
    GMCS_T *mcs = (GMCS_T*)task;
    CsrBtGattDbAccessWriteInd *ind;
    GATT_MANAGER_SERVER_ACCESS_IND_T *message;

    switch (id)
    {
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)CsrPmemZalloc(
                                   sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));

            populateMcsServerReadAccessInd((CsrBtGattDbAccessReadInd*)msg, message);
            mcsHandleAccessIndication(mcs,
                    (GATT_MANAGER_SERVER_ACCESS_IND_T *)message,
                    ((CsrBtGattDbAccessReadInd*)msg)->maxRspValueLength);

            CsrPmemFree(message);
            break;
        }
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            ind = (CsrBtGattDbAccessWriteInd*)msg;
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)CsrPmemZalloc(
                                 sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            message->value = (uint8*)CsrPmemZalloc(ind->writeUnit->valueLength);

            populateMcsServerWriteAccessInd((CsrBtGattDbAccessWriteInd*)ind, message);
            mcsHandleAccessIndication(mcs,
                    (GATT_MANAGER_SERVER_ACCESS_IND_T*)message,
                     0);

            CsrPmemFree(message->value);
            CsrPmemFree(message);
            break;
        }
        default:
        {
            GATT_MCS_SERVER_WARNING(
                        "GMCS: GATT  message 0x%04x not handled\n",
                        id);
            break;
        }
    } /* switch */

    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}

void gattMcsServerMsgHandler(void** gash)
{
    uint16 eventClass = 0;
    void* msg = NULL;
    CsrBtGattPrim id;
    ServiceHandle srvcHndl = 0;

    GMCS_T* mcs = NULL;

    srvcHndl = *((ServiceHandle*)*gash);
    mcs = ServiceHandleGetInstanceData(srvcHndl);

    if (mcs == NULL)
    {
        GATT_MCS_SERVER_ERROR("/nGMCS: is NULL/n");
        return;
    }

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                id = *(CsrBtGattPrim*)msg;
                mcsServerMsgHandler(mcs, id, msg);
                break;
            }
            case MCS_SERVER_PRIM:
                break;
            default:
                break;
        }

        SynergyMessageFree(eventClass, msg);
    }
}



