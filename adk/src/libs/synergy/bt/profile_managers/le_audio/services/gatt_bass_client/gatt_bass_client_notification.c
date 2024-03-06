/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_bass_client.h"
#include "gatt_bass_client_private.h"
#include "gatt_bass_client_notification.h"
#include "gatt_bass_client_common.h"
#include "gatt_bass_client_debug.h"

/* Broadcast Receive State characteristic field offsets */
#define BIG_ENCRYPTION_OFFSET             (14)

#define BIS_SYNC_STATE_SIZE  (4)

/***************************************************************************/
static void bassClientHandleRegisterForNotification(const GBASSC *client,
                                                    ServiceHandle clnt_hndl,
                                                    bool enable,
                                                    uint16 handle)
{
    MAKE_BASS_CLIENT_INTERNAL_MESSAGE(BASS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ);

    message->clnt_hndl = clnt_hndl;
    message->enable = enable;
    message->handle = handle;

    GATT_BASS_CLIENT_DEBUG("BASS Notifications: enable = %x\n", message->enable);

    BassMessageSendConditionally(client->lib_task,
                             BASS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,
                             message,
                             &(client->pending_cmd));
}

/***************************************************************************/
void bassClientHandleInternalRegisterForNotification(GBASSC *const bass_client,
                                                     bool enable,
                                                     uint16 handle)
{
    uint8* value = (uint8*)(CsrPmemZalloc(BASS_CLIENT_CHARACTERISTIC_CONFIG_SIZE));

    value[0] = enable ? BASS_NOTIFICATION_VALUE : 0;
    value[1] = 0;

    CsrBtGattWriteReqSend(bass_client->srvcElem->gattId,
                          bass_client->srvcElem->cid,
                          handle,
                          0,
                          BASS_CLIENT_CHARACTERISTIC_CONFIG_SIZE,
                          value);
}

/****************************************************************************/
GattBassClientStatus GattBassClientBroadcastReceiveStateRegisterForNotificationReq(ServiceHandle clntHndl,
                                                                                   uint8 sourceId,
                                                                                   bool allSource,
                                                                                   bool notificationsEnable)
{
    GBASSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        if (allSource)
        {
            bass_client_handle_t *ptr = client->client_data.broadcast_receive_state_handles_first;

            while(ptr)
            {
                bassClientHandleRegisterForNotification(client,
                                                        client->srvcElem->service_handle,
                                                        notificationsEnable,
                                                        ptr->handle_ccc);

                ptr = ptr->next;
            }
        }
        else
        {
            uint16 handle = bassClientHandleFromSourceId(client, sourceId, TRUE);

            if(handle)
            {
                bassClientHandleRegisterForNotification(client,
                                                        client->srvcElem->service_handle,
                                                        notificationsEnable,
                                                        handle);
            }
            else
            {
                GATT_BASS_CLIENT_ERROR("Invalid Source Id!\n");
                return GATT_BASS_CLIENT_STATUS_INVALID_PARAMETER;
            }
        }
    }
    else
    {
        gattBassClientPanic();
        return GATT_BASS_CLIENT_STATUS_INVALID_PARAMETER;
    }

    return GATT_BASS_CLIENT_STATUS_SUCCESS;
}

/****************************************************************************/
static bool bassClientCheckBroadcastReceiveStateValue(uint16 size, uint8 *value)
{
    uint8 numSubGroupOffset = BASS_CLIENT_BROADCAST_RECEIVE_STATE_MIN_SIZE;
    uint8 numSubgroups = 0;

    /* Check all the conditions for which the read value is not complete
     * and we need to do a read long characteristic. */
    if((size < BASS_CLIENT_BROADCAST_RECEIVE_STATE_MIN_SIZE) ||
       (value[BIG_ENCRYPTION_OFFSET-1] == GATT_BASS_CLIENT_BAD_CODE &&
        size < (BASS_CLIENT_BROADCAST_RECEIVE_STATE_MIN_SIZE + BASS_CLIENT_BROADCAST_CODE_SIZE)))
    {
        /* Condition 1: the length of the value is less than the minimum one
         * (length of the Broadcast Receive State characteristic with length equal
         * to zero for all the dynamic fields);
         * Condition 2: the length of the value is less than the minumum one plus
         * the bad code length when a bad code is present;
         */
        return FALSE;
    }

    if(value[BIG_ENCRYPTION_OFFSET-1] == GATT_BASS_CLIENT_BAD_CODE)
    {
        /* Adjust the offset of the number of subgroups field
         * in case of a bad code is present */
        numSubGroupOffset += BASS_CLIENT_BROADCAST_CODE_SIZE;
    }

    /* Get the value of the number of subgroups field */
    numSubgroups = value[numSubGroupOffset-1];

    if(numSubgroups)
    {
        /* If at least one subgroup is present and for each of them,
         * we need to check that the value includes all the necessary fields
         * (bis_state and metadata) */
        uint8 i = 0;
        uint8 metadataLenOffset = numSubGroupOffset + BIS_SYNC_STATE_SIZE + 1;
        uint8 noMetadataSize = numSubGroupOffset + (BIS_SYNC_STATE_SIZE * numSubgroups) + numSubgroups;
        uint8 metadataSize = noMetadataSize;

        if(size < noMetadataSize)
        {
            /* Condiction 3: the length of the value is less the minumum one
             * we could have in case of one or more subgroups are present */
            return FALSE;
        }

        for(i=0; i < numSubgroups; i++)
        {
            /*Check if there are metadata for the current subgroup */
            if(value[metadataLenOffset-1])
            {
                if(size < (metadataSize + value[metadataLenOffset-1]))
                {
                    /* Condiction 4: the length of the value is not enough to
                     * include all the metadata of this subgroup */
                    return FALSE;
                }
            }

            /* Include the metadata length of the subgroup just checked
             * in the variables used to check the next one */
            metadataSize += value[metadataLenOffset-1];
            metadataLenOffset += (value[metadataLenOffset-1] + BIS_SYNC_STATE_SIZE + 1);
        }
    }

    return TRUE;
}

/****************************************************************************/
void bassClientHandleClientNotification(GBASSC *bass_client,
                                        const CsrBtGattClientNotificationInd *ind)
{
    bass_client_handle_t *ptr = bass_client->client_data.broadcast_receive_state_handles_first;

    while(ptr)
    {
        if (ind->valueHandle == ptr->handle && ind->valueLength)
        {
            if(bassClientCheckBroadcastReceiveStateValue(ind->valueLength, ind->value))
            {
                MAKE_BASS_CLIENT_MESSAGE(GattBassClientBroadcastReceiveStateInd);

                bassClientSetSourceId(bass_client,
                                      ind->valueHandle,
                                      ind->value[0]);

                message->clntHndl = bass_client->srvcElem->service_handle;

                bassClientExtractBroadcastReceiveStateCharacteristicValue(ind->value, &(message->brsValue));

                /* Send the confirmation message to app task  */
                BassMessageSend(bass_client->app_task, GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_IND, message);
            }
            else
            {
                MAKE_BASS_CLIENT_INTERNAL_MESSAGE(BASS_CLIENT_INTERNAL_MSG_NOTIFY_READ);

                message->clnt_hndl = bass_client->srvcElem->service_handle;
                message->handle = ind->valueHandle;

                BassMessageSendConditionally(bass_client->lib_task,
                                             BASS_CLIENT_INTERNAL_MSG_NOTIFY_READ,
                                             message,
                                             &bass_client->pending_cmd);
            }
        }
        else if(ind->valueHandle == ptr->handle && !ind->valueLength)
        {
            MAKE_BASS_CLIENT_MESSAGE(GattBassClientBroadcastReceiveStateInd);

            bassClientSetSourceId(bass_client,
                                  ind->valueHandle,
                                  0);

            message->clntHndl = bass_client->srvcElem->service_handle;

            memset(&(message->brsValue), 0, sizeof(GattBassClientBroadcastReceiveState));

            /* Send the confirmation message to app task  */
            BassMessageSend(bass_client->app_task, GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_IND, message);
        }

        ptr = ptr->next;
    }
}
