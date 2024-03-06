/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "gatt_mics_server_msg_handler.h"
#include "gatt_mics_server_access.h"
#include "gatt_mics_server_debug.h"
#include <stdio.h>

/******************************************************************************/

static void populateMicsServerWriteAccessInd(CsrBtGattDbAccessWriteInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_WRITE;
    message->size_value = msg->writeUnit->valueLength;
    message->offset = msg->writeUnit->offset;
    CsrMemCpy(message->value, msg->writeUnit->value, msg->writeUnit->valueLength);
}

static void populateMicsServerReadAccessInd(CsrBtGattDbAccessReadInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_READ;
    message->offset = msg->offset;

    /* print the handle of character being read*/
    GATT_MICS_SERVER_INFO("\n(GMICS) Read request recieved for handle:%X \n", msg->attrHandle);
}

void micsServerMsgHandler(void* task, MsgId id, Msg msg)
{
    GMICS_T *mics = (GMICS_T*)task;
    CsrBtGattDbAccessWriteInd *ind;
    GATT_MANAGER_SERVER_ACCESS_IND_T *message;

    switch (id)
    {
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)CsrPmemZalloc(
                                   sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));

            populateMicsServerReadAccessInd((CsrBtGattDbAccessReadInd*)msg, message);
            micsHandleAccessIndication(mics,
                    (GATT_MANAGER_SERVER_ACCESS_IND_T *)message);

            CsrPmemFree(message);
            break;
        }
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            ind = (CsrBtGattDbAccessWriteInd*)msg;
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)CsrPmemZalloc(
                                 sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            message->value = (uint8*)CsrPmemZalloc(ind->writeUnit->valueLength);

            populateMicsServerWriteAccessInd((CsrBtGattDbAccessWriteInd*)ind, message);
            micsHandleAccessIndication(mics,
                    (GATT_MANAGER_SERVER_ACCESS_IND_T*)message);

            CsrPmemFree(message->value);
            CsrPmemFree(message);
            break;
        }
        default:
        {
            GATT_MICS_SERVER_WARNING(
                        "GMICS: GATT  message 0x%04x not handled\n",
                        id);
            break;
        }
    } /* switch */

    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}

void mics_server_msg_handler(void** gash)
{
    uint16 eventClass = 0;
    void* msg = NULL;
    CsrBtGattPrim id;
    ServiceHandle srvc_hndl = 0;

    GMICS_T* mics = NULL;

    srvc_hndl = *((ServiceHandle*)*gash);
    mics = ServiceHandleGetInstanceData(srvc_hndl);

    if (mics == NULL)
    {
        GATT_MICS_SERVER_ERROR("\nGMICS: is NULL\n");
        return;
    }

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                id = *(CsrBtGattPrim*)msg;
                micsServerMsgHandler(mics, id, msg);
                break;
            }
            case MICS_SERVER_PRIM:
                break;
            default:
                break;
        }

        SynergyMessageFree(eventClass, msg);
    }
}

