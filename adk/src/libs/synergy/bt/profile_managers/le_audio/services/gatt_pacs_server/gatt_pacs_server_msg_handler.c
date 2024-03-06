/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_pacs_server_debug.h"
#include "csr_bt_gatt_prim.h"
#include "gatt_pacs_server_msg_handler.h"
#include "gatt_pacs_server_access.h"
#include "gatt_pacs_server_utils.h"

/****************************************************************************/
static void populatePacsServerWriteAccessInd(CsrBtGattDbAccessWriteInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_WRITE;
    message->size_value = msg->writeUnit->valueLength;
    CsrMemCpy(message->value, msg->writeUnit->value, msg->writeUnit->valueLength);
}

static void populatePacsServerReadAccessInd(CsrBtGattDbAccessReadInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_READ;
    message->offset = msg->offset;
}

void pacsServerMsgHandler(void* task, MsgId id, Msg msg)
{
    GPACSS_T *pacs = (GPACSS_T*)task;
    GATT_MANAGER_SERVER_ACCESS_IND_T *message;
    CsrBtGattDbAccessWriteInd *ind;

    GATT_PACS_SERVER_INFO("PACS: GATT Manager Server Msg 0x%x \n",id);

    switch (id)
    {
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            ind = (CsrBtGattDbAccessWriteInd*)msg;
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)
                                              CsrPmemZalloc(sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            message->value = (uint8*) CsrPmemZalloc(ind->writeUnit->valueLength);
            populatePacsServerWriteAccessInd(ind, message);
            handlePacsServerAccessInd(pacs, message, 0);
            CsrPmemFree(message->value);
            CsrPmemFree(message);
            break;
        }
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)
                                              CsrPmemZalloc(sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            populatePacsServerReadAccessInd((CsrBtGattDbAccessReadInd*)msg, message);
            handlePacsServerAccessInd(pacs, message, ((CsrBtGattDbAccessReadInd*)msg)->maxRspValueLength);
            CsrPmemFree(message);
            break;
        }
        default:
        {
            /* Unrecognised GATT  message */
            GATT_PACS_SERVER_WARNING("PACS: GATT  Server Msg 0x%x not handled\n",id);
        }
        break;
    }

    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}


void pacs_server_msg_handler(void** gash)
{
    uint16 eventClass = 0;
    void* msg = NULL;
    CsrBtGattPrim id;
    PacsServiceHandleType srvc_hndl = 0;

    GPACSS_T* pacs = NULL;

    srvc_hndl = *((PacsServiceHandleType*)*gash);
    pacs = ServiceHandleGetInstanceData(srvc_hndl);

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
        case CSR_BT_GATT_PRIM:
        {
            id = *(CsrBtGattPrim*)msg;
            if (pacs)
            {
                pacsServerMsgHandler(pacs, id, msg);
            }
            break;
        }
        case PACS_SERVER_PRIM:
        default:
            break;
        }
    }

    SynergyMessageFree(eventClass, msg);
}
