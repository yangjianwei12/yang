/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <service_handle.h>.h>
#include <gatt_manager.h>

#include "gatt_vocs_client_debug.h"
#include "gatt_vocs_client_private.h"
#include "gatt_vocs_client_notification.h"
#include "gatt_vocs_client_common.h"

/***************************************************************************/
static void vocsClientHandleRegisterForNotification(const GVOCS *gatt_vocs_client,
                                                    bool enable,
                                                    uint16 handle)
{
    MAKE_VOCS_CLIENT_INTERNAL_MESSAGE(VOCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ);

    message->enable = enable;
    message->handle = handle;

    GATT_VOCS_CLIENT_DEBUG_INFO(("VOCS Notifications: enable = %x\n", message->enable));

    MessageSendConditionally((Task)&gatt_vocs_client->lib_task,
                             VOCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,
                             message,
                             &(gatt_vocs_client->pending_cmd));
}

/***************************************************************************/
void vocsClientHandleInternalRegisterForNotification(GVOCS *const vocs_client,
                                                     bool enable,
                                                     uint16 handle)
{
    uint8 value[VOCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE];

    value[0] = enable ? VOCS_NOTIFICATION_VALUE : 0;
    value[1] = 0;

    GattManagerWriteCharacteristicValue((Task)&vocs_client->lib_task,
                                        handle,
                                        VOCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE,
                                        value);
}

/****************************************************************************/
void GattVocsClientOffsetStateRegisterForNotificationReq(ServiceHandle clntHndl,
                                                         bool notificationsEnable)
{
    GVOCS *gatt_vocs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vocs_client)
    {
        vocsClientHandleRegisterForNotification(gatt_vocs_client,
                                                notificationsEnable,
                                                gatt_vocs_client->handles.offsetStateCccHandle);
    }
    else
    {
        GATT_VOCS_CLIENT_DEBUG_PANIC(("Invalid VOCS Client instance!\n"));
    }
}

/****************************************************************************/
static void vocsClientOptRegisterForNot(GVOCS *gatt_vocs_client,
                                        uint16 handle,
                                        bool not_enable,
                                        uint8 properties,
                                        GattVocsClientMessageId id)
{
    if (properties & VOCS_CLIENT_NOTIFY_PROP)
    {
        /* The remote device support Notify property for the Audio Location characteristic */
        vocsClientHandleRegisterForNotification(gatt_vocs_client,
                                                not_enable,
                                                handle);
    }
    else
    {
        /* Notify property not supported */
        vocsClientSendVocsClientWriteCfm(gatt_vocs_client,
                                         gatt_status_request_not_supported,
                                         id);
    }
}

/****************************************************************************/
void GattVocsClientAudioLocationRegisterForNotificationReq(ServiceHandle clntHndl,
                                                           bool notificationsEnable)
{
    GVOCS *gatt_vocs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vocs_client)
    {
        vocsClientOptRegisterForNot(gatt_vocs_client,
                                    gatt_vocs_client->handles.audioLocationCccHandle,
                                    notificationsEnable,
                                    gatt_vocs_client->handles.audioLocationProperties,
                                    GATT_VOCS_CLIENT_AUDIO_LOCATION_SET_NTF_CFM);
    }
    else
    {
        GATT_VOCS_CLIENT_DEBUG_PANIC(("Invalid VOCS Client instance!\n"));
    }
}

/****************************************************************************/
void GattVocsClientAudioOutputDescRegisterForNotificationReq(ServiceHandle clntHndl,
                                                             bool notificationsEnable)
{
    GVOCS *gatt_vocs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vocs_client)
    {
        vocsClientOptRegisterForNot(gatt_vocs_client,
                                    gatt_vocs_client->handles.audioOutputDescriptionCccHandle,
                                    notificationsEnable,
                                    gatt_vocs_client->handles.audioOutputDescProperties,
                                    GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_SET_NTF_CFM);
    }
    else
    {
        GATT_VOCS_CLIENT_DEBUG_PANIC(("Invalid VOCS Client instance!\n"));
    }
}
