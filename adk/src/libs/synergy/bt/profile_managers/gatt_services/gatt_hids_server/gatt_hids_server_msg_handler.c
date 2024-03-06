/*******************************************************************************

Copyright (C) 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "gatt_hids_server_private.h"
#include "gatt_hids_server_msg_handler.h"
#include "gatt_hids_server_access.h"
#include "csr_bt_gatt_prim.h"
#include "gatt_hids_server_private.h"

static void populateHidsServerReadAccessInd(CsrBtGattDbAccessReadInd* msg,
                                            CsrBtGattAccessInd* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_READ;
    message->offset = msg->offset;
}
/*******************************************************/

void hidsServerMsgHandler(void* task, MsgId id, Msg msg)
{
    GHIDS* hids = (GHIDS*)task;
    CsrBtGattAccessInd *message ;
    CsrBtGattRegisterCfm* cfm;

    switch (id)
    {
        case CSR_BT_GATT_REGISTER_CFM:
        {
            GATT_HIDS_SERVER_DEBUG(" \n HIDS Server: CSR_BT_GATT_REGISTER_CFM received \n" );

            cfm = (CsrBtGattRegisterCfm*)msg;
            if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS &&
                cfm->resultSupplier == CSR_BT_SUPPLIER_GATT)
            {
                hids->gattId = cfm->gattId;


                CsrBtGattFlatDbRegisterHandleRangeReqSend(hids->gattId,
                                                      hids->startHandle,
                                                      hids->endHandle);
            }
        break;
        }

        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            GATT_HIDS_SERVER_INFO(" \n HIDS Server: CSR_BT_GATT_DB_ACCESS_WRITE_IND received \n");
            hidsServerHandleAccessWriteIndication(hids, (CsrBtGattDbAccessWriteInd*)msg);
            break;
        }
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            message = (CsrBtGattAccessInd*) CsrPmemZalloc(sizeof(CsrBtGattAccessInd));
            populateHidsServerReadAccessInd((CsrBtGattDbAccessReadInd*)msg, message);
            handleHidsServerAccess(hids, message);
            CsrPmemFree(message);
            break;
        }

    }

}

void gattHidsServerMsgHandler(void** gash)
{
    uint16 eventClass = 0;
    void* msg = NULL;
    CsrBtGattPrim id;
    ServiceHandle srvcHndl = 0;

    GHIDS* hids = NULL;

    srvcHndl = *((ServiceHandle*)*gash);
    hids = ServiceHandleGetInstanceData(srvcHndl);

    if (hids == NULL)
    {
        GATT_HIDS_SERVER_ERROR("/nGHIDS: is NULL/n");
        return;
    }

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                id = *(CsrBtGattPrim*)msg;
                hidsServerMsgHandler(hids, id, msg);
                break;
            }
            case HIDS_SERVER_PRIM:
                break;
            default:
                break;
        }

        SynergyMessageFree(eventClass, msg);
    }
}
