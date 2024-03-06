/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_vcs_client.h"
#include "gatt_vcs_client_debug.h"
#include "gatt_vcs_client_private.h"
#include "gatt_vcs_client_notification.h"
#include "gatt_vcs_client_common.h"

/***************************************************************************/
static void vcsClientHandleRegisterForNotification(const GVCSC *client,
                                                   bool enable,
                                                   uint16 handle)
{
    /* Validate the Input Parameters */
    if (client == NULL)
    {
        GATT_VCS_CLIENT_PANIC("Null instance\n");
        return;
    }
    else
    {
        MAKE_VCS_CLIENT_INTERNAL_MESSAGE(VCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ);

        message->srvc_hndl = client->srvcElem->service_handle;
        message->enable = enable;
        message->handle = handle;

        GATT_VCS_CLIENT_INFO("VCS Notifications: enable = %x\n", message->enable);

        VcsMessageSendConditionally(client->lib_task,
                                    VCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,
                                    message,
                                    &(client->pending_cmd));
    }
}

/***************************************************************************/
void vcsClientHandleInternalRegisterForNotification(GVCSC *const vcs_client,
                                                    bool enable,
                                                    uint16 handle)
{
    uint8* value = (uint8*)(CsrPmemZalloc(VCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE));

    value[0] = enable ? VCS_NOTIFICATION_VALUE : 0;
    value[1] = 0;

    CsrBtGattWriteReqSend(vcs_client->srvcElem->gattId,
                          vcs_client->srvcElem->cid,
                          handle,
                          0,
                          VCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE,
                          value);
}

/****************************************************************************/
void GattVcsClientVolStateRegisterForNotificationReq(ServiceHandle clntHndl, bool notificationsEnable)
{
    GVCSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vcsClientHandleRegisterForNotification(client,
                                               notificationsEnable,
                                               client->handles.volumeStateCccHandle);
    }
    else
    {
        GATT_VCS_CLIENT_PANIC("Invalid VCS Client instance!\n");
    }
}
/****************************************************************************/
void GattVcsClientVolFlagRegisterForNotificationReq(ServiceHandle clntHndl,
                                                    bool notificationsEnable)
{
    GVCSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client && client->handles.volumeFlagsCccHandle)
    {
        vcsClientHandleRegisterForNotification(client,
                                               notificationsEnable,
                                               client->handles.volumeFlagsCccHandle);
    }
    else
    {
        if (!client)
        {
            GATT_VCS_CLIENT_PANIC("Invalid VCS Client instance!\n");
        }
        else
        {
            GATT_VCS_CLIENT_ERROR("Notifications for Volume Flag not supported by the remote device\n");
        }
    }
}
