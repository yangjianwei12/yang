/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_manager.h>

#include "gatt_vcs_client.h"
#include "gatt_vcs_client_debug.h"
#include "gatt_vcs_client_private.h"
#include "gatt_vcs_client_notification.h"
#include "gatt_vcs_client_common.h"

/***************************************************************************/
static void vcsClientHandleRegisterForNotification(const GVCSC *gatt_vcs_client,
                                                   bool enable,
                                                   uint16 handle)
{
    /* Validate the Input Parameters */
    if (gatt_vcs_client == NULL)
    {
        GATT_VCS_CLIENT_PANIC(("Null instance\n"));
    }

    MAKE_VCS_CLIENT_INTERNAL_MESSAGE(VCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ);

    message->enable = enable;
    message->handle = handle;

    GATT_VCS_CLIENT_DEBUG_INFO(("VCS Notifications: enable = %x\n", message->enable));

    MessageSendConditionally((Task)&gatt_vcs_client->lib_task,
                             VCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,
                             message,
                             &(gatt_vcs_client->pending_cmd));
}

/***************************************************************************/
void vcsClientHandleInternalRegisterForNotification(GVCSC *const vcs_client,
                                                    bool enable,
                                                    uint16 handle)
{
    uint8 value[VCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE];

    value[0] = enable ? VCS_NOTIFICATION_VALUE : 0;
    value[1] = 0;

    GattManagerWriteCharacteristicValue((Task)&vcs_client->lib_task,
                                        handle,
                                        VCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE,
                                        value);
}

/****************************************************************************/
void GattVcsClientVolStateRegisterForNotificationReq(ServiceHandle clntHndl,
                                                     bool notificationsEnable)
{
    GVCSC *gatt_vcs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vcs_client)
    {
        vcsClientHandleRegisterForNotification(gatt_vcs_client,
                                               notificationsEnable,
                                               gatt_vcs_client->handles.volumeStateCccHandle);
    }
    else
    {
        GATT_VCS_CLIENT_DEBUG_PANIC(("Invalid VCS Client instance!\n"));
    }
}
/****************************************************************************/
void GattVcsClientVolFlagRegisterForNotificationReq(ServiceHandle clntHndl,
                                                    bool notificationsEnable)
{
    GVCSC *gatt_vcs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vcs_client && gatt_vcs_client->handles.volumeFlagsCccHandle)
    {
        vcsClientHandleRegisterForNotification(gatt_vcs_client,
                                               notificationsEnable,
                                               gatt_vcs_client->handles.volumeFlagsCccHandle);
    }
    else
    {
        if (!gatt_vcs_client)
        {
            GATT_VCS_CLIENT_DEBUG_PANIC(("Invalid VCS Client instance!\n"));
        }
        else
        {
            GATT_VCS_CLIENT_DEBUG_INFO(("Notifications for Volume Flag not supported by the remote device\n"));
        }
    }
}
