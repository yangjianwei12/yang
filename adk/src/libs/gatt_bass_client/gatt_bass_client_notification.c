/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_manager.h>

#include "gatt_bass_client_private.h"
#include "gatt_bass_client_notification.h"
#include "gatt_bass_client_common.h"
#include "gatt_bass_client_debug.h"

/***************************************************************************/
static void bassClientHandleRegisterForNotification(const GBASSC *gatt_bass_client,
                                                    bool enable,
                                                    uint16 handle)
{
    MAKE_BASS_CLIENT_INTERNAL_MESSAGE(BASS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ);

    message->enable = enable;
    message->handle = handle;

    GATT_BASS_CLIENT_DEBUG_INFO(("BASS Notifications: enable = %x\n", message->enable));

    MessageSendConditionally((Task)&gatt_bass_client->lib_task,
                             BASS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,
                             message,
                             &(gatt_bass_client->pending_cmd));
}

/***************************************************************************/
void bassClientHandleInternalRegisterForNotification(GBASSC *const bass_client,
                                                     bool enable,
                                                     uint16 handle)
{
    uint8 value[BASS_CLIENT_CHARACTERISTIC_CONFIG_SIZE];

    value[0] = enable ? BASS_NOTIFICATION_VALUE : 0;
    value[1] = 0;

    GattManagerWriteCharacteristicValue((Task)&bass_client->lib_task,
                                        handle,
                                        BASS_CLIENT_CHARACTERISTIC_CONFIG_SIZE,
                                        value);
}

/****************************************************************************/
GattBassClientStatus GattBassClientBroadcastReceiveStateRegisterForNotificationReq(ServiceHandle clntHndl,
                                                                                   uint8 sourceId,
                                                                                   bool allSource,
                                                                                   bool notificationsEnable)
{
    GBASSC *gatt_bass_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_bass_client)
    {
        if (allSource)
        {
            bass_client_handle_t *ptr = gatt_bass_client->client_data.broadcast_receive_state_handles_first;

            while(ptr)
            {
                bassClientHandleRegisterForNotification(gatt_bass_client,
                                                        notificationsEnable,
                                                        ptr->handle_ccc);

                ptr = ptr->next;
            }
        }
        else
        {
            uint16 handle = bassClientHandleFromSourceId(gatt_bass_client, sourceId, TRUE);

            if(handle)
            {
                bassClientHandleRegisterForNotification(gatt_bass_client,
                                                        notificationsEnable,
                                                        handle);
            }
            else
            {
                GATT_BASS_CLIENT_DEBUG_PANIC(("Invalid Source Id!\n"));
                return GATT_BASS_CLIENT_STATUS_INVALID_PARAMETER;
            }
        }
    }
    else
    {
        GATT_BASS_CLIENT_DEBUG_PANIC(("Invalid BASS Client instance!\n"));
        return GATT_BASS_CLIENT_STATUS_INVALID_PARAMETER;
    }

    return GATT_BASS_CLIENT_STATUS_SUCCESS;
}

/****************************************************************************/
void bassClientHandleClientNotification(GBASSC *bass_client,
                                        const GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND_T *ind)
{
    bass_client_handle_t *ptr = bass_client->client_data.broadcast_receive_state_handles_first;

    while(ptr)
    {
        if (ind->handle == ptr->handle)
        {
            MAKE_BASS_CLIENT_MESSAGE_WITH_LEN(GattBassClientBroadcastReceiveStateInd,
                                              ind->value[BASS_CLIENT_BROADCAST_RECEIVE_STATE_VALUE_SIZE_MIN - 1]);

            message->clntHndl = bass_client->clnt_hndl;
            message->sourceId = ind->value[0];
            message->sourceAddress.type = ind->value[1];

            message->sourceAddress.addr.nap = (((uint16) ind->value[7]) << 8) | ((uint16) ind->value[6]);
            message->sourceAddress.addr.uap = ind->value[5];
            message->sourceAddress.addr.lap = (((uint32) ind->value[4]) << 16) |
                                                (((uint32) ind->value[3]) << 8) |
                                                ((uint32) ind->value[2]);

            message->advSid = ind->value[8];
            message->paSyncState = ind->value[9];

            message->bisSyncState = ((uint32) ind->value[13] << 24)   |
                                     ((uint32) ind->value[12] << 16 ) |
                                     ((uint32) ind->value[11] << 8 )  |
                                     ((uint32) ind->value[10]);

            message->bigEncryption = ind->value[14];
            message->metadataLen = ind->value[15];

            if(message->metadataLen)
            {
                bassClientSwapByteTrasmissionOrder(&(ind->value[16]),
                                                   message->metadataLen,
                                                   message->metadataValue);
            }

            /* Send the confirmation message to app task  */
            MessageSend(bass_client->app_task, GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_IND, message);
        }

        ptr = ptr->next;
    }
}
