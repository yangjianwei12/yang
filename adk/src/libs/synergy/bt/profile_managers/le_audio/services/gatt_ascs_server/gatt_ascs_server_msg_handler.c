/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_ascs_server_private.h"
#include "gatt_ascs_server_msg_handler.h"
#include "gatt_ascs_server_access.h"
#include "csr_bt_gatt_prim.h"
#include "gatt_ascs_server_private.h"
#include "gatt_ascs_server_debug.h"

/* Required octets for values sent to Client Configuration Descriptor */
#define GATT_CLIENT_CONFIG_NUM_OCTETS   2

/***************************************************************************/
static void accessIndConstructFromGattAccessWriteInd(CsrBtGattDbAccessWriteInd *msg,
                                                     CsrBtGattAccessInd *message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_WRITE;

    /* reliable write will have writeUnitCount > 1 as well. Long write will have
       flags set to CSR_BT_GATT_ACCESS_CHECK_NONE */
    if ((msg->writeUnitCount > 1) && (msg->check == CSR_BT_GATT_ACCESS_CHECK_NONE))
    {
        message->flags |= ATT_ACCESS_WRITE_COMPLETE; /*  Long gatt write complete*/
    }

    message->writeUnit = msg->writeUnit;
    /* will be 1 if not reliable or long write */
    message->numWriteUnits = msg->writeUnitCount;
}

static void populateAscsServerReadAccessInd(CsrBtGattDbAccessReadInd *msg,
                                            CsrBtGattAccessInd *message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->offset = msg->offset;
    message->flags = ATT_ACCESS_READ;
}



/***************************************************************************
NAME
    sendAscsServerReadAccessRsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
void sendAscsServerReadAccessRsp(CsrBtGattId gattId,
                             ConnectionId cid,
                             uint16 handle,
                             uint16 result,
                             uint16 sizeValue,
                             uint8 *const value)
{
    uint8* data;
    data = (uint8*)zpmalloc(sizeof(uint8)*sizeValue);
    CsrMemCpy(data, value, sizeValue);
    CsrBtGattDbReadAccessResSend(gattId, cid, handle, result, sizeValue, data);
}

/***************************************************************************
NAME
    sendAscsServerWriteAccessRsp

DESCRIPTION
    Send an access response to the GATT  library.
*/
void sendAscsServerWriteAccessRsp(CsrBtGattId task,
                                  ConnectionId cid,
                                  uint16 handle,
                                  uint16 result,
                                  uint16 sizeValue,
                                  uint8* value)
{
    CSR_UNUSED(sizeValue);
    CSR_UNUSED(value);
    CsrBtGattDbWriteAccessResSend(task,
                                  cid,
                                  handle,
                                 (CsrBtGattDbAccessRspCode)result);
}


/***************************************************************************
NAME
    ascsServerMsgHandler

DESCRIPTION
    Handle Indications from GATT.

****************************************************************************/
void ascsServerMsgHandler(void* task, MsgId id, Msg msg)
{
    GattAscsServer *ascs = (GattAscsServer*)task;
    CsrBtGattAccessInd* message;
    CsrBtGattDbAccessWriteInd* ind;
    GATT_ASCS_SERVER_INFO("ASCS: GATT Manager Server Msg 0x%x \n",id);

    switch (id)
    {
        /* Read/write access to characteristic */
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            message = (CsrBtGattAccessInd*)zpmalloc(
                                          sizeof(CsrBtGattAccessInd));
            populateAscsServerReadAccessInd((CsrBtGattDbAccessReadInd*)msg, message);
            handleAscsAccess(ascs, message);
            CsrPmemFree(message);
        }
        break;
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            ind = (CsrBtGattDbAccessWriteInd*)msg;

            message = (CsrBtGattAccessInd*)zpmalloc(
                                          sizeof(CsrBtGattAccessInd));

            accessIndConstructFromGattAccessWriteInd(ind, message);
            handleAscsAccess(ascs, message);

            CsrPmemFree(message);
        }
        break;
        case CSR_BT_GATT_CLIENT_INDICATION_IND:
        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
        {
            /* Library just absorbs confirmation messages */
        }
        break;
        case CSR_BT_GATT_CONFIG_MODE_CFM:
        {
            GATT_ASCS_SERVER_DEBUG("ASCS: New long gatt Write");
        }
        break;
        default:
        {
            /* Unrecognised GATT  message */
            GATT_ASCS_SERVER_WARNING("ASCS: GATT Manager Server Msg 0x%x not handled\n",id);
        }
        break;
    }
    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}

/*Scheduler Task msg handler for ASCS server*/

void ascs_server_msg_handler(void **gash)
{
    uint16_t eventClass = 0;
    void* msg = NULL;

    if(CsrSchedMessageGet(&eventClass, &msg))
    {
        switch(eventClass)
        {
        case CSR_BT_GATT_PRIM:
        {
            ServiceHandle serviceHandle = *((ServiceHandle*)*gash);
            GattAscsServer *ascs = ServiceHandleGetInstanceData(serviceHandle);
            if (ascs != NULL)
            {
                CsrBtGattPrim id = *(CsrBtGattPrim*)msg;
                ascsServerMsgHandler(ascs, id, msg);
            }
            break;
        }
        default:
            break;
        }
    }
    SynergyMessageFree(eventClass, msg);
}
