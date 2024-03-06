/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_client.h"
#include "gatt_ams_client_private.h"
#include "gatt_ams_client_msg_handler.h"
#include "gatt_ams_client_ready_state.h"
#include <gatt_lib.h>
#include <string.h>
#include <logging.h>

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(gatt_ams_client_message_id_t)

typedef enum
{
    remote_command_size = 1,
    entity_attribute_size = 2,
    entity_update_size_not_counting_attributes = 1
} value_sizes_t;

static void initClient(GAMS *ams, uint32 cid, uint16 start_handle, uint16 end_handle)
{
    memset(ams, 0, sizeof(GAMS));
    ams->lib_task.handler = gattAmsMsgHandler;
    ams->cid = cid;
    ams->service_start_handle = start_handle;
    ams->service_end_handle = end_handle;
    ams->remote_command_handle = GATT_AMS_INVALID_HANDLE;
    ams->entity_attribute_handle = GATT_AMS_INVALID_HANDLE;
    ams->entity_update_handle = GATT_AMS_INVALID_HANDLE;
    ams->entity_update_ccd = GATT_AMS_INVALID_HANDLE;
    ams->remote_command_ccd = GATT_AMS_INVALID_HANDLE;
}

bool GattAmsInit(Task task, uint32 cid, uint16 start_handle, uint16 end_handle, void *ams_data_dynamic, void* ams_data_const)
{
    UNUSED(ams_data_const);

    if ((task == NULL) || (ams_data_dynamic == NULL))
        return FALSE;

    GAMS* gatt_ams = (GAMS*)ams_data_dynamic;
    
    initClient(gatt_ams, cid, start_handle, end_handle);

    gatt_ams->pending_cmd = ams_pending_init;
    gatt_ams->task_pending_cfm = task;
    gatt_ams->app_task = task;

    GattRegisterReqSend(&gatt_ams->lib_task, 1234);

    PRINT(("AMS: Initialised instance [%p] with cid [0x%02x]\n", (void *) gatt_ams, gatt_ams->cid));

    return TRUE;
}

bool GattAmsDestroy(void *ams_client_dysnamic_data)
{
    GAMS* ams = (GAMS*)ams_client_dysnamic_data;
    if (ams != NULL)
    {
        GattUnregisterReqSend(ams->gattId);
        return TRUE;
    }
    return FALSE;
}

void GattAmsSetRemoteCommandNotificationEnableRequest(Task task, const GAMS *ams, bool notifications_enable)
{
    MAKE_AMS_CLIENT_MESSAGE(MSG_SET_CHARACTERISTIC_NOTIFICATION);

    PRINT(("AMS: Remote Command, enable notifications request = %u\n", notifications_enable));

    message->task_pending_cfm = task;
    message->cmd_to_set_as_pending = ams_pending_set_remote_command_notification;
    message->notifications_enable = notifications_enable;

    MessageSendConditionally((Task) &ams->lib_task, MSG_SET_CHARACTERISTIC_NOTIFICATION, message, &ams->pending_cmd);
}

void GattAmsSetEntityUpdateNotificationEnableRequest(Task task, const GAMS *ams, bool notifications_enable)
{
    MAKE_AMS_CLIENT_MESSAGE(MSG_SET_CHARACTERISTIC_NOTIFICATION);

    PRINT(("AMS: Entity Update, enable notifications request = %u\n", notifications_enable));

    message->task_pending_cfm = task;
    message->cmd_to_set_as_pending = ams_pending_set_entity_update_notification;
    message->notifications_enable = notifications_enable;

    MessageSendConditionally((Task) &ams->lib_task, MSG_SET_CHARACTERISTIC_NOTIFICATION, message, &ams->pending_cmd);
}

void GattAmsWriteRemoteCommand(Task task, const GAMS *ams, gatt_ams_remote_command_id_t remote_command_id)
{
    MAKE_AMS_CLIENT_MESSAGE_WITH_LEN(MSG_WRITE_CHARACTERISTIC, remote_command_size);

    message->task_pending_cfm      = task;
    message->cmd_to_set_as_pending = ams_pending_write_remote_command;
    message->size_command_data     = remote_command_size;
    message->command_data[0]       = remote_command_id;

    MessageSendConditionally((Task) &ams->lib_task, MSG_WRITE_CHARACTERISTIC, message, &ams->pending_cmd);
}

void GattAmsWriteEntityUpdate(Task task, const GAMS *ams, gatt_ams_entity_id_t entity_id, const uint8 *attribute_id_list, uint16 size_attribute_list)
{
    uint16 command_data_size = entity_update_size_not_counting_attributes + size_attribute_list;

    MAKE_AMS_CLIENT_MESSAGE_WITH_LEN(MSG_WRITE_CHARACTERISTIC, command_data_size);

    message->task_pending_cfm      = task;
    message->cmd_to_set_as_pending = ams_pending_write_entity_update;
    message->size_command_data     = command_data_size;
    message->command_data[0]       = entity_id;
    memcpy(&message->command_data[1], attribute_id_list, size_attribute_list);

    MessageSendConditionally((Task) &ams->lib_task, MSG_WRITE_CHARACTERISTIC, message, &ams->pending_cmd);
}

void GattAmsWriteEntityAttribute(Task task, const GAMS *ams, gatt_ams_entity_id_t entity_id, uint8 attribute_id)
{
    MAKE_AMS_CLIENT_MESSAGE_WITH_LEN(MSG_WRITE_CHARACTERISTIC, entity_attribute_size);

    message->task_pending_cfm      = task;
    message->cmd_to_set_as_pending = ams_pending_write_entity_attribute;
    message->size_command_data     = entity_attribute_size;
    message->command_data[0]       = entity_id;
    message->command_data[1]       = attribute_id;

    MessageSendConditionally((Task) &ams->lib_task, MSG_WRITE_CHARACTERISTIC, message, &ams->pending_cmd);
}

void GattAmsReadEntityAttribute(Task task, const GAMS *ams)
{
    MAKE_AMS_CLIENT_MESSAGE(MSG_READ_CHARACTERISTIC);

    message->task_pending_cfm      = task;
    message->cmd_to_set_as_pending = ams_pending_read_entity_attribute;

    MessageSendConditionally((Task) &ams->lib_task, MSG_READ_CHARACTERISTIC, message, &ams->pending_cmd);
}

uint16 GattAmsGetRemoteCommandHandle(const GAMS *ams)
{
    return ams->remote_command_handle;
}

uint16 GattAmsGetEntityUpdateHandle(const GAMS *ams)
{
    return ams->entity_update_handle;
}

uint32 GattAmsGetConnectionId(const GAMS *ams)
{
    return ams->cid;
}
