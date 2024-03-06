/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/
#include "gatt_tmas_server_common.h"
#include "gatt_tmas_server_access.h"
#include "gatt_tmas_server_msg_handler.h"
#include "gatt_tmas_server_debug.h"

static void gattTmasServerPopulateRadAccessInd(CsrBtGattDbAccessReadInd* msg,
                                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_READ;
    message->offset = msg->offset;
}

/******************************************************************************/
void gattTmasServerHandleGattMsg(void* task, MsgId id, Msg msg)
{
    GTMAS *tmasServer = (GTMAS *)task;
    CsrBtGattRegisterCfm* cfm;
    GATT_MANAGER_SERVER_ACCESS_IND_T* message;

    switch (id)
    {
        case CSR_BT_GATT_REGISTER_CFM:
        {
            GATT_TMAS_SERVER_DEBUG(" \n TMAS Server: CSR_BT_GATT_REGISTER_CFM recievced \n");

            cfm = (CsrBtGattRegisterCfm*)msg;
            if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS &&
                    cfm->resultSupplier == CSR_BT_SUPPLIER_GATT)
            {
                tmasServer->gattId = cfm->gattId;

                CsrBtGattFlatDbRegisterHandleRangeReqSend(tmasServer->gattId,
                                                          tmasServer->startHandle,
                                                          tmasServer->endHandle);
            }
            break;
        }

        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            GATT_TMAS_SERVER_DEBUG(" \n TMAS Server: CSR_BT_GATT_DB_ACCESS_READ_IND recievced \n");

            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)
                                              CsrPmemZalloc(sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            gattTmasServerPopulateRadAccessInd((CsrBtGattDbAccessReadInd*)msg, message);
            gattTmasServerHandleAccessIndication(tmasServer, message);
            CsrPmemFree(message);
            break;
        }

        default:
        {
            GATT_TMAS_SERVER_WARNING(
                        "TMAS: GATT Manager message 0x%04x not handled\n",
                        id
                        );
            break;
        }
    } /* switch */

    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}

/* Synergy task scheduler message handler */

void gattTmasServerMsgHandler(void** gash)
{
    void* msg = NULL;
    GTMAS *tmasServer = NULL;
    CsrBtGattPrim id;
    uint16 eventClass = 0;
    ServiceHandle srvcHndl = 0;

    srvcHndl = *((ServiceHandle*)*gash);
    tmasServer = ServiceHandleGetInstanceData(srvcHndl);

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                if(tmasServer)
                {
                    id = *(CsrBtGattPrim*)msg;
                    gattTmasServerHandleGattMsg(tmasServer, id, msg);
                }
                break;
            }

            case TMAS_SERVER_PRIM:
            default:
            break;
        }

        SynergyMessageFree(eventClass, msg);
    }
}
