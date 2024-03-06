/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_bass_client_msg_handler.h"
#include "gatt_bass_client_debug.h"
#include "gatt_bass_client_init.h"
#include "gatt_bass_client_discovery.h"
#include "gatt_bass_client_common.h"
#include "gatt_bass_client_read.h"
#include "gatt_bass_client_write.h"
#include "gatt_bass_client_notification.h"

/****************************************************************************/
static void bassClientHandleGattManagerMsg(Task task, MessageId id, Message msg)
{
    GBASSC *gatt_client = (GBASSC *)task;
    
    switch (id)
    {
        case GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM:
        {
            bassClientHandleDiscoverAllCharacteristicsResp(gatt_client,
                                                           (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)msg);
        }
        break;

        case GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM:
        {
            bassClientHandleDiscoverAllCharacteristicDescriptorsResp(gatt_client,
                                                                     (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)msg);
        }
        break;

        case GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM:
        {
            /* Set Notification Confirmation */
            bassClientHandleWriteValueRespCfm(gatt_client,
                                              (const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T*)msg);
        }
        break;

        case GATT_MANAGER_WRITE_WITHOUT_RESPONSE_CFM:
        {
            /* Write Confirmation */
            bassClientHandleWriteWithoutResponseRespCfm(gatt_client,
                                                        (const GATT_MANAGER_WRITE_WITHOUT_RESPONSE_CFM_T*)msg);
        }
        break;

        case GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM:
        {
            bassClientHandleReadValueResp(gatt_client,
                                          (const GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM_T *)msg);
        }
        break;

        case GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND:
        {
            bassClientHandleClientNotification(gatt_client,
                                        (const GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND_T *)msg);
        }
        break;

        default:
        {
            /* Unrecognised GATT Manager message */
            GATT_BASS_CLIENT_DEBUG_PANIC(("GBASSC: BASS Client GattMgr Msg not handled [0x%x]\n", id));
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
static void  bassClientHandleInternalMessage(Task task, MessageId id, Message msg)
{
    GBASSC * bass_client = (GBASSC *)task;

    GATT_BASS_CLIENT_DEBUG_INFO(("Message id (%d)\n",id));

    if (bass_client)
    {
        switch(id)
        {
            case BASS_CLIENT_INTERNAL_MSG_INIT_READ:
            {
                uint16 handle = ((const BASS_CLIENT_INTERNAL_MSG_INIT_READ_T*)msg)->handle;

                GattManagerReadCharacteristicValue((Task)&bass_client->lib_task, handle);

                bass_client->pending_cmd = bass_client_init_read_pending;
            }
            break;

            case BASS_CLIENT_INTERNAL_MSG_READ_CCC:
            {
                uint16 handle = ((const BASS_CLIENT_INTERNAL_MSG_READ_CCC_T*)msg)->handle;

                GattManagerReadCharacteristicValue((Task)&bass_client->lib_task, handle);

                bass_client->pending_cmd = bass_client_read_pending_ccc;
            }
            break;

            case BASS_CLIENT_INTERNAL_MSG_READ:
            {
                uint16 handle = ((const BASS_CLIENT_INTERNAL_MSG_READ_T *)msg)->handle;

                GattManagerReadCharacteristicValue((Task)&bass_client->lib_task, handle);

                bass_client->pending_cmd = bass_client_read_pending;
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

                if (message->no_response)
                {
                    GattManagerWriteWithoutResponse((Task)&bass_client->lib_task,
                                                    message->handle,
                                                    message->size_value,
                                                    message->value);
                }
                else
                {
                    GattManagerWriteCharacteristicValue((Task)&bass_client->lib_task,
                                                        message->handle,
                                                        message->size_value,
                                                        message->value);
                }


                bassClientSetFlagWritePending(bass_client,
                                              (bass_client_control_point_opcodes_t) message->value[0]);
            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                GATT_BASS_CLIENT_DEBUG_PANIC(("Unknown Message received from Internal To lib \n"));
            }
            break;
        }
    }
}

/****************************************************************************/
void gattBassClientMsgHandler(Task task, MessageId id, Message msg)
{
    if ((id >= GATT_MANAGER_MESSAGE_BASE) && (id < GATT_MANAGER_MESSAGE_TOP))
    {
        bassClientHandleGattManagerMsg(task, id, msg);
    }
    /* Check message is internal Message */
    else if((id > BASS_CLIENT_INTERNAL_MSG_BASE) && (id < BASS_CLIENT_INTERNAL_MSG_TOP))
    {
        bassClientHandleInternalMessage(task,id,msg);
    }
    else
    {
        GATT_BASS_CLIENT_DEBUG_PANIC(("GBASSC: Client Msg not handled [0x%x]\n", id));
    }
}

