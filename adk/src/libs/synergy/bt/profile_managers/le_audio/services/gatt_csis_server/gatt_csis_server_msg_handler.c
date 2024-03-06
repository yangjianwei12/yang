/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_csis_server_access.h"
#include "gatt_csis_server_private.h"
#include "gatt_csis_server_msg_handler.h"
#include "gatt_csis_server_lock_management.h"
#include "gatt_csis_server_debug.h"

static void populateCsisServerWriteAccessInd(CsrBtGattDbAccessWriteInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_WRITE;
    message->size_value = msg->writeUnit->valueLength;
    message->offset = msg->writeUnit->offset;
    CsrMemCpy(message->value, msg->writeUnit->value, msg->writeUnit->valueLength);
}

static void populateCsisServerReadAccessInd(CsrBtGattDbAccessReadInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_READ;
    message->offset = msg->offset;
}

/****************************************************************************/
void csisServerMsgHandler(void* task, MsgId id, Msg msg)
{
    GCSISS_T *csis_server = (GCSISS_T*)task;
    CsrBtGattDbAccessWriteInd *ind;
    GATT_MANAGER_SERVER_ACCESS_IND_T *message;

    switch (id)
    {
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)CsrPmemZalloc(
                                   sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            populateCsisServerReadAccessInd((CsrBtGattDbAccessReadInd*)msg, message);
            handleCsisServerAccessInd(csis_server,
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
            populateCsisServerWriteAccessInd((CsrBtGattDbAccessWriteInd*)ind, message);
            handleCsisServerAccessInd(csis_server,
                           (GATT_MANAGER_SERVER_ACCESS_IND_T *)message);
            CsrPmemFree(message->value);
            CsrPmemFree(message);
            break;
        }

        default:
        {
            /* Unrecognised GATT  message */
            GATT_CSIS_SERVER_WARNING("CSIS: GATT Manager Server Msg 0x%x not handled\n",id);
        }
        break;
    }

    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}

static void csisServerCmMsgHandler(void* task, MsgId id, Msg msg)
{
    GCSISS_T *csis = (GCSISS_T*)task;

    switch (id)
    {
        case CSR_BT_CM_LE_SIRK_OPERATION_CFM:
           GATT_CSIS_SERVER_DEBUG("CSIS: Sirk Encrpytion/Decryption 0x%x \n",id);
           csisServerhandleSirkOperation(csis, (const CsrBtCmLeSirkOperationCfm *) msg);
        break;
    }
    CsrBtCmFreeUpstreamMessageContents(CSR_BT_CM_PRIM, msg);
}

void csis_server_msg_handler(void** gash)
{
    uint16 eventClass = 0;
    void* msg = NULL;
    CsrBtGattPrim id;
    CsisServerServiceHandleType srvc_hndl = 0;
    GCSISS_T* csis = NULL;

    srvc_hndl = *((CsisServerServiceHandleType*)*gash);
    csis = ServiceHandleGetInstanceData(srvc_hndl);

    if (csis == NULL)
    {
        GATT_CSIS_SERVER_ERROR("GCSISS_T:CSIS is NULL!\n");
        return;
    }

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                id = *(CsrBtGattPrim*)msg;
                csisServerMsgHandler(csis, id, msg);
                break;
            }
            case CSR_BT_CM_PRIM:
            {
                id = *(CsrPrim*)msg;
                csisServerCmMsgHandler(csis, id, msg);
            }
            default:
                break;
         }

        SynergyMessageFree(eventClass, msg);
    }
}
