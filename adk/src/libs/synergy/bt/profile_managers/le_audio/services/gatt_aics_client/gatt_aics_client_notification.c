/* Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "csr_bt_gatt_lib.h"

#include "gatt_aics_client.h"
#include "gatt_aics_client_debug.h"
#include "gatt_aics_client_private.h"
#include "gatt_aics_client_notification.h"
#include "gatt_aics_client_common.h"

/***************************************************************************/
static void aicsClientHandleRegisterForNotification(const GAICS *client,
                                                    bool enable,
                                                    uint16 handle)
{
    MAKE_AICS_CLIENT_INTERNAL_MESSAGE(AICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ);

    message->enable = enable;
    message->handle = handle;

    GATT_AICS_CLIENT_DEBUG("AICS Notifications: enable = %x\n", message->enable);

    AicsMessageSendConditionally(client->lib_task,
                                 AICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,
                                 message,
                                 &client->pending_cmd);
}

/***************************************************************************/
void GattAicsClientInputStateRegisterForNotificationReq(ServiceHandle clntHndl, bool notificationsEnable)
{
    GAICS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        aicsClientHandleRegisterForNotification(client,
                                                notificationsEnable,
                                                client->handles.inputStateCccHandle);
    }
    else
    {
        gattAicsClientError();
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

    CsrBtGattWriteReqSend(aics_client->srvcElem->gattId,
                          aics_client->srvcElem->cid,
                          handle,
                          0,
                          AICS_CLIENT_CHARACTERISTIC_CONFIG_SIZE,
                          value);
}

/****************************************************************************/
static void aicsClientOptRegisterForNot(GAICS *client,
                                   uint16 handle,
                                   bool not_enable,
                                   uint8 properties,
                                   GattAicsClientMessageId id)
{
    if (properties & AICS_CLIENT_NOTIFY_PROP)
    {
        /* The remote device support Notify property for the Input Status characteristic */
        aicsClientHandleRegisterForNotification(client,
                                                not_enable,
                                                handle);
    }
    else
    {
        /* Notify property not supported */
        aicsClientSendAicsClientWriteCfm(client,
                                         CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED,
                                         id);
    }
}

/***************************************************************************/
void GattAicsClientInputStatusRegisterForNotificationReq(ServiceHandle clntHndl, bool notificationsEnable)
{
    GAICS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        aicsClientHandleRegisterForNotification(client,
                                                notificationsEnable,
                                                client->handles.inputStatusCccHandle);
    }
    else
    {
        gattAicsClientError();
    }
}

/***************************************************************************/
void GattAicsClientAudioInputDescRegisterForNotificationReq(ServiceHandle clntHndl, bool notificationsEnable)
{
    GAICS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        aicsClientOptRegisterForNot(client,
                                    client->handles.audioInputDescriptionCccHandle,
                                    notificationsEnable,
                                    client->handles.audioInputDescProperties,
                                    GATT_AICS_CLIENT_AUDIO_INPUT_DESC_SET_NTF_CFM);
    }
    else
    {
        gattAicsClientError();
    }
}
