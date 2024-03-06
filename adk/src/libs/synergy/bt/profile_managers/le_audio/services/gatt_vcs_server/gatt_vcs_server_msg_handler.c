/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "gatt_vcs_server_common.h"
#include "gatt_vcs_server_msg_handler.h"
#include "gatt_vcs_server_access.h"
#include "gatt_vcs_server_debug.h"

/****************************************************************************/
static void populateVcsServerWriteAccessInd(CsrBtGattDbAccessWriteInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_WRITE;
    message->offset = msg->writeUnit->offset;
    message->size_value = msg->writeUnit->valueLength;
    CsrMemCpy(message->value, msg->writeUnit->value, msg->writeUnit->valueLength);
}

static void populateVcsServerReadAccessInd(CsrBtGattDbAccessReadInd* msg,
                               GATT_MANAGER_SERVER_ACCESS_IND_T* message)
{
    message->cid = msg->btConnId;
    message->handle = msg->attrHandle;
    message->flags = ATT_ACCESS_READ;
    message->offset = msg->offset;
}

/******************************************************************************/
void vcsServerMsgHandler(void* task, MsgId id, Msg msg)
{
    GVCS *volume_control_server = (GVCS *)task;
    CsrBtGattRegisterCfm* cfm;
    CsrBtGattDbAccessWriteInd *ind;
    GATT_MANAGER_SERVER_ACCESS_IND_T* message;

    switch (id)
    {
        case CSR_BT_GATT_REGISTER_CFM:
        {
            GATT_VCS_SERVER_DEBUG(" \n GVCS Server: CSR_BT_GATT_REGISTER_CFM recievced \n");

            cfm = (CsrBtGattRegisterCfm*)msg;
            if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS &&
                    cfm->resultSupplier == CSR_BT_SUPPLIER_GATT)
            {
                volume_control_server->gattId = cfm->gattId;

                CsrBtGattFlatDbRegisterHandleRangeReqSend(volume_control_server->gattId,
                                                          volume_control_server->start_handle,
                                                          volume_control_server->end_handle);
            }
            break;
        }
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            GATT_VCS_SERVER_DEBUG(" \n GVCS Server: CSR_BT_GATT_DB_ACCESS_WRITE_IND recievced \n");

            ind = (CsrBtGattDbAccessWriteInd*)msg;
            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)
                                              CsrPmemZalloc(sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            message->value = (uint8*)CsrPmemZalloc(ind->writeUnit->valueLength);
            populateVcsServerWriteAccessInd(ind, message);
            vcsServerHandleAccessIndication(volume_control_server, message);
            CsrPmemFree(message->value);
            CsrPmemFree(message);
            break;
        }
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            GATT_VCS_SERVER_DEBUG(" \n GVCS Server: CSR_BT_GATT_DB_ACCESS_READ_IND recievced \n");

            message = (GATT_MANAGER_SERVER_ACCESS_IND_T*)
                                              CsrPmemZalloc(sizeof(GATT_MANAGER_SERVER_ACCESS_IND_T));
            populateVcsServerReadAccessInd((CsrBtGattDbAccessReadInd*)msg, message);
            vcsServerHandleAccessIndication(volume_control_server, message);
            CsrPmemFree(message);
            break;
        }

        default:
        {
            GATT_VCS_SERVER_WARNING(
                        "GVCS: GATT Manager message 0x%04x not handled\n",
                        id
                        );
            break;
        }
    } /* switch */

    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, msg);
}

/* Synergy task scheduler message handler */

void vcs_server_msg_handler(void** gash)
{
    void* msg = NULL;
    GVCS *vcs = NULL;
    CsrBtGattPrim id;
    uint16 eventClass = 0;
    ServiceHandle srvc_hndl = 0;

    srvc_hndl = *((ServiceHandle*)*gash);
    vcs = ServiceHandleGetInstanceData(srvc_hndl);

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                if(vcs)
                {
                    id = *(CsrBtGattPrim*)msg;
                    vcsServerMsgHandler(vcs, id, msg);
                }
                break;
            }

            case VCS_SERVER_PRIM:
            default:
            break;
        }

        SynergyMessageFree(eventClass, msg);
    }
}
