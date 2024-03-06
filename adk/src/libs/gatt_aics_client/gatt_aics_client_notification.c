/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_manager.h>

#include "gatt_aics_client.h"
#include "gatt_aics_client_debug.h"
#include "gatt_aics_client_private.h"
#include "gatt_aics_client_notification.h"
#include "gatt_aics_client_common.h"

/***************************************************************************/
static void aicsClientHandleRegisterForNotification(const GAICS *gatt_aics_client,
                                                    bool enable,
                                                    uint16 handle)
{
    MAKE_AICS_CLIENT_INTERNAL_MESSAGE(AICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ);

    message->enable = enable;
    message->handle = handle;

    GATT_AICS_CLIENT_DEBUG_INFO(("AICS Notifications: enable = %x\n", message->enable));

    MessageSendConditionally((Task)&gatt_aics_client->lib_task,
                             AICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,
                             message,
                             &gatt_aics_client->pending_cmd);
}

/***************************************************************************/
void GattAicsClientInputStateRegisterForNotificationReq(ServiceHandle clntHndl,
                                                        bool notificationsEnable)
{
    GAICS *gatt_aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_aics_client)
    {
        aicsClientHandleRegisterForNotification(gatt_aics_client,
                                                notificationsEnable,
                                                gatt_aics_client->handles.inputStateCccHandle);
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Invalid AICS Client instance!\n"));
    }
}

/***************************************************************************/
void aicsClientHandleInternalRegisterForNotification(GAICS *const aics_client,
                                                     bool enable,
                                                     uint16 handle)
{
    uint8 value[AICS_CLIENT_CHARACTERISTIC_CONFIG_SIZE];

    value[0] = enable ? AICS_NOTIFICATION_VALUE : 0;
    value[1] = 0;

    GattManagerWriteCharacteristicValue((Task)&aics_client->lib_task,
                                        handle,
                                        AICS_CLIENT_CHARACTERISTIC_CONFIG_SIZE,
                                        value);
}

/****************************************************************************/
static void aicsClientOptRegisterForNot(GAICS *gatt_aics_client,
                                   uint16 handle,
                                   bool not_enable,
                                   uint8 properties,
                                   GattAicsClientMessageId id)
{
    if (properties & AICS_CLIENT_NOTIFY_PROP)
    {
        /* The remote device support Notify property for the Input Status characteristic */
        aicsClientHandleRegisterForNotification(gatt_aics_client,
                                                not_enable,
                                                handle);
    }
    else
    {
        /* Notify property not supported */
        aicsClientSendAicsClientWriteCfm(gatt_aics_client,
                                         gatt_status_request_not_supported,
                                         id);
    }
}

/***************************************************************************/
void GattAicsClientInputStatusRegisterForNotificationReq(ServiceHandle clntHndl,
                                                         bool notificationsEnable)
{
    GAICS *gatt_aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_aics_client)
    {
        aicsClientHandleRegisterForNotification(gatt_aics_client,
                                                notificationsEnable,
                                                gatt_aics_client->handles.inputStatusCccHandle);
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Invalid AICS Client instance!\n"));
    }
}

/***************************************************************************/
void GattAicsClientAudioInputDescRegisterForNotificationReq(ServiceHandle clntHndl,
                                                            bool notificationsEnable)
{
    GAICS *gatt_aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_aics_client)
    {
        aicsClientOptRegisterForNot(gatt_aics_client,
                                    gatt_aics_client->handles.audioInputDescriptionCccHandle,
                                    notificationsEnable,
                                    gatt_aics_client->handles.audioInputDescProperties,
                                    GATT_AICS_CLIENT_AUDIO_INPUT_DESC_SET_NTF_CFM);
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Invalid AICS Client instance!\n"));
    }
}
