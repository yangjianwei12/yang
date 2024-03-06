/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/
#include "gatt_gmas_server_common.h"
#include "gatt_gmas_server_access.h"
#include "gatt_gmas_server_msg_handler.h"
#include "gatt_gmas_server_debug.h"

static void gattGmasServerPopulateReadAccessInd(CsrBtGattDbAccessReadInd* msg,
                                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_READ;
    message->offset = msg->offset;
}

/******************************************************************************/
void gattGmasServerHandleGattMsg(void* task, MsgId id, Msg msg)
{
    GGMAS *gmasServer = (GGMAS *)task;
    CsrBtGattRegisterCfm* cfm;
    GATT_MANAGER_SERVER_ACCESS_IND_T* message;

    GATT_GMAS_SERVER_DEBUG("gattGmasServerHandleGattMsg MsgId:0x%04x", id);
    switch (id)
    {
        case CSR_BT_GATT_REGISTER_CFM:
        {
            cfm = (CsrBtGattRegisterCfm*)msg;
            if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS &&
                    cfm->resultSupplier == CSR_BT_SUPPLIER_GATT)
            {
                gmasServer->gattId = cfm->gattId;

                CsrBtGattFlatDbRegisterHandleRangeReqSend(gmasServer->gattId,
                                                          gmasServer->startHandle,
                                                          gmasServer->endHandle);
            }
            break;
        }

        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)
                                              CsrPmemZalloc(sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            gattGmasServerPopulateReadAccessInd((CsrBtGattDbAccessReadInd*)msg, message);
            gattGmasServerHandleAccessIndication(gmasServer, message);
            CsrPmemFree(message);
            break;
        }

        default:
        {
            GATT_GMAS_SERVER_WARNING(
                        "GMAS: GATT Manager message 0x%04x not handled\n",
                        id
                        );
            break;
        }
    } /* switch */

    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}

/* Synergy task scheduler message handler */
void gattGmasServerMsgHandler(void** gash)
{
    void* msg = NULL;
    GGMAS *gmasServer = NULL;
    CsrBtGattPrim id;
    uint16 eventClass = 0;
    ServiceHandle srvcHndl = 0;

    srvcHndl = *((ServiceHandle*)*gash);
    gmasServer = ServiceHandleGetInstanceData(srvcHndl);

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                if(gmasServer)
                {
                    id = *(CsrBtGattPrim*)msg;
                    gattGmasServerHandleGattMsg(gmasServer, id, msg);
                }
                break;
            }

            case GMAS_SERVER_PRIM:
            default:
            break;
        }

        SynergyMessageFree(eventClass, msg);
    }
}
