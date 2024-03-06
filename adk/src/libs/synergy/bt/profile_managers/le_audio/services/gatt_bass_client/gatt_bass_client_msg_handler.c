/* Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_bass_client_msg_handler.h"
#include "gatt_bass_client_debug.h"
#include "gatt_bass_client_init.h"
#include "gatt_bass_client_discovery.h"
#include "gatt_bass_client_common.h"
#include "gatt_bass_client_read.h"
#include "gatt_bass_client_write.h"
#include "gatt_bass_client_notification.h"

#include "csr_bt_gatt_lib.h"

/****************************************************************************/
static void bassClientHandleGattManagerMsg(void *task, MsgId id, Msg msg)
{
    GBASSC *gatt_client = (GBASSC *)task;
    
    switch (id)
    {
        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
        {
            bassClientHandleDiscoverAllCharacteristicsResp(gatt_client,
                                                           (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
        {
            bassClientHandleDiscoverAllCharacteristicDescriptorsResp(gatt_client,
                                                                     (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_WRITE_CFM:
        {
            /* Set Notification Confirmation */
            bassClientHandleWriteValueRespCfm(gatt_client,
                                              (const CsrBtGattWriteCfm*)msg);
        }
        break;

        case CSR_BT_GATT_READ_CFM:
        {
            bassClientHandleReadValueResp(gatt_client,
                                          (const CsrBtGattReadCfm *)msg);
        }
        break;

        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
        {
            bassClientHandleClientNotification(gatt_client,
                                        (const CsrBtGattClientNotificationInd *)msg);
        }
        break;

        default:
        {
            /* Unrecognised GATT Manager message */
            GATT_BASS_CLIENT_WARNING("GBASSC: BASS Client GattMgr Msg not handled [0x%x]\n", id);
        }
        break;
    }
}

/****************************************************************************/
static void bassClientSetFlagWritePending(GBASSC * bass_client,
                                          bass_client_control_point_opcodes_t opcode)
{
    switch(opcode)
    {
        case bass_client_remote_scan_stop_op:
            bass_client->pending_cmd = bass_client_write_remote_scan_stop_pending;
        break;

        case bass_client_remote_scan_start_op:
            bass_client->pending_cmd = bass_client_write_remote_scan_start_pending;
        break;

        case bass_client_add_source_op:
            bass_client->pending_cmd = bass_client_write_add_source_pending;
        break;

        case bass_client_modify_source_op:
            bass_client->pending_cmd = bass_client_write_modify_source_pending;
        break;

        case bass_client_set_broadcast_code_op:
            bass_client->pending_cmd = bass_client_write_set_broadcast_code_pending;
        break;

        case bass_client_remove_source_op:
            bass_client->pending_cmd = bass_client_write_remove_source_pending;
        break;

        default:
        break;
    }
}

/***************************************************************************/
static void  bassClientHandleInternalMessage(void * task, MsgId id, Msg msg)
{
    GBASSC * bass_client = (GBASSC *)task;

    GATT_BASS_CLIENT_INFO("Message id (%d)\n",id);

    if (bass_client)
    {
        switch(id)
        {
            case BASS_CLIENT_INTERNAL_MSG_INIT_READ:
            {
                uint16 handle = ((const BASS_CLIENT_INTERNAL_MSG_INIT_READ_T*)msg)->handle;

                CsrBtGattReadReqSend(bass_client->srvcElem->gattId,
                         bass_client->srvcElem->cid,
                         handle,
                         0);

                bass_client->pending_cmd = bass_client_init_read_pending;
            }
            break;

            case BASS_CLIENT_INTERNAL_MSG_READ_CCC:
            {
                uint16 handle = ((const BASS_CLIENT_INTERNAL_MSG_READ_CCC_T*)msg)->handle;

                CsrBtGattReadReqSend(bass_client->srvcElem->gattId,
                         bass_client->srvcElem->cid,
                         handle,
                         0);

                bass_client->pending_cmd = bass_client_read_pending_ccc;
            }
            break;

            case BASS_CLIENT_INTERNAL_MSG_READ:
            {
                uint16 handle = ((const BASS_CLIENT_INTERNAL_MSG_READ_T *)msg)->handle;

                CsrBtGattReadReqSend(bass_client->srvcElem->gattId,
                         bass_client->srvcElem->cid,
                         handle,
                         0);

                bass_client->pending_cmd = bass_client_read_pending;
            }
            break;

            case BASS_CLIENT_INTERNAL_MSG_NOTIFY_READ:
            {
                uint16 handle = ((const BASS_CLIENT_INTERNAL_MSG_NOTIFY_READ_T *)msg)->handle;

                CsrBtGattReadReqSend(bass_client->srvcElem->gattId,
                         bass_client->srvcElem->cid,
                         handle,
                         0);

                bass_client->pending_cmd = bass_client_notify_read_pending;
            }
            break;

            case BASS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ:
            {
                BASS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T* message = (BASS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T*) msg;

                bassClientHandleInternalRegisterForNotification(bass_client,
                                                                message->enable,
                                                                message->handle);

                bass_client->pending_cmd = bass_client_write_notification_pending;
            }
            break;

            case BASS_CLIENT_INTERNAL_MSG_WRITE:
            {
                BASS_CLIENT_INTERNAL_MSG_WRITE_T* message = (BASS_CLIENT_INTERNAL_MSG_WRITE_T*) msg;
                uint8_t *value = (uint8 *) CsrPmemAlloc(sizeof(uint8)*(message->size_value));

                CsrMemCpy(value, &(message->value[0]), message->size_value);

                if (message->no_response)
                {
                    CsrBtGattWriteCmdReqSend(bass_client->srvcElem->gattId,
                             bass_client->srvcElem->cid,
                             message->handle,
                             message->size_value,
                             value);
                }
                else
                {
                    CsrBtGattWriteReqSend(bass_client->srvcElem->gattId,
                             bass_client->srvcElem->cid,
                             message->handle,
                             0,
                             message->size_value,
                             value);
                }


                bassClientSetFlagWritePending(bass_client,
                                              (bass_client_control_point_opcodes_t) message->value[0]);
            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                GATT_BASS_CLIENT_WARNING("Unknown Message received from Internal To lib \n");
            }
            break;
        }
    }
}


/****************************************************************************/
void gattBassClientMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *message = NULL;
    gatt_bass_client *inst = *((gatt_bass_client **)gash);

    if (CsrSchedMessageGet(&eventClass, &message))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                CsrBtGattPrim *id = message;
                GBASSC * bass_client = (GBASSC *) GetServiceClientByGattMsg(&inst->service_handle_list, message);
                void *msg = GetGattManagerMsgFromGattMsg(message, id);

                if(bass_client)
                   bassClientHandleGattManagerMsg(bass_client, *id, msg);

                if(msg!=message)
                {
                    CsrPmemFree(msg);
                }

                CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, message);
            }
                break;
            case BASS_CLIENT_PRIM:
            {
                bass_client_internal_msg_t *id = (bass_client_internal_msg_t *)message;
                GBASSC *bass_client = (GBASSC *) GetServiceClientByServiceHandle(message);

                bassClientHandleInternalMessage(bass_client, *id, message);
            }
                break;
            default:
                GATT_BASS_CLIENT_WARNING("GBASSC: Client Msg not handled [0x%x]\n", eventClass);
        }

        SynergyMessageFree(eventClass, message);
    }
}


