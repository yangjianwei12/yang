/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "service_handle.h"
#include "csr_bt_gatt_lib.h"

#include "gatt_vocs_client_debug.h"
#include "gatt_vocs_client_private.h"
#include "gatt_vocs_client_notification.h"
#include "gatt_vocs_client_common.h"

/***************************************************************************/
static void vocsClientHandleRegisterForNotification(const GVOCS *client,
                                                    bool enable,
                                                    uint16 handle)
{
    MAKE_VOCS_CLIENT_INTERNAL_MESSAGE(VOCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ);

    message->srvc_hndl = client->srvcElem->service_handle;
    message->enable = enable;
    message->handle = handle;

    GATT_VOCS_CLIENT_DEBUG("VOCS Notifications: enable = %x\n", message->enable);

    VocsMessageSendConditionally(client->lib_task,
                                 VOCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,
                                 message,
                                 &(client->pending_cmd));
}

/***************************************************************************/
void vocsClientHandleInternalRegisterForNotification(GVOCS *const vocs_client,
                                                     bool enable,
                                                     uint16 handle)
{
    uint8 value[VOCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE];

    value[0] = enable ? VOCS_NOTIFICATION_VALUE : 0;
    value[1] = 0;

    CsrBtGattWriteReqSend(vocs_client->srvcElem->gattId,
                          vocs_client->srvcElem->cid,
                          handle,
                          0,
                          VOCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE,
                          value);
}

/****************************************************************************/
void GattVocsClientOffsetStateRegisterForNotificationReq(ServiceHandle clntHndl, bool notificationsEnable)
{
    GVOCS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vocsClientHandleRegisterForNotification(client,
                                                notificationsEnable,
                                                client->handles.offsetStateCccHandle);
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Invalid VOCS Client instance!\n");
    }
}

/****************************************************************************/
static void vocsClientOptRegisterForNot(GVOCS *client,
                                        uint16 handle,
                                        bool not_enable,
                                        uint8 properties,
                                        GattVocsClientMessageId id)
{
    if (properties & VOCS_CLIENT_NOTIFY_PROP)
    {
        /* The remote device support Notify property for the Audio Location characteristic */
        vocsClientHandleRegisterForNotification(client,
                                                not_enable,
                                                handle);
    }
    else
    {
        /* Notify property not supported */
        vocsClientSendVocsClientWriteCfm(client,
                                         CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED,
                                         id);
    }
}

/****************************************************************************/
void GattVocsClientAudioLocationRegisterForNotificationReq(ServiceHandle clntHndl, bool notificationsEnable)
{
    GVOCS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vocsClientOptRegisterForNot(client,
                                    client->handles.audioLocationCccHandle,
                                    notificationsEnable,
                                    client->handles.audioLocationProperties,
                                    GATT_VOCS_CLIENT_AUDIO_LOCATION_SET_NTF_CFM);
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Invalid VOCS Client instance!\n");
    }
}

/****************************************************************************/
void GattVocsClientAudioOutputDescRegisterForNotificationReq(ServiceHandle clntHndl,
                                                             bool notificationsEnable)
{
    GVOCS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vocsClientOptRegisterForNot(client,
                                    client->handles.audioOutputDescriptionCccHandle,
                                    notificationsEnable,
                                    client->handles.audioOutputDescProperties,
                                    GATT_VOCS_CLIENT_AUDIO_OUTPUT_DESC_SET_NTF_CFM);
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Invalid VOCS Client instance!\n");
    }
}
