/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_bass_client.h"
#include "gatt_bass_client_debug.h"
#include "gatt_bass_client_common.h"
#include "gatt_bass_client_read.h"
#include "gatt_bass_client_init.h"

/****************************************************************************/
void bassClientHandleReadValueResp(GBASSC *bass_client,
                                   const GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM_T *read_cfm)
{
    switch (bass_client->pending_cmd)
    {
        case bass_client_init_read_pending:
        {
            if (read_cfm->status == gatt_status_success)
            {
                bass_client_handle_t *ptr = bass_client->client_data.broadcast_receive_state_handles_first;

                while(ptr)
                {
                    /* Find the right Broadacast Receive State characteristic by the handle */
                    if (read_cfm->handle == ptr->handle)
                    {
                        bass_client->counter += 1;

                        /* Found the right handle:
                         * save the sourceId that is the first byte in the read value.*/
                        ptr->source_id = read_cfm->value[0];

                        if (bass_client->counter == bass_client->client_data.broadcast_source_num)
                        {
                            bass_client->counter = 0;

                            /* All the Broadcast Receive State Chracteristics have been read */
                            gattBassClientSendInitCfm(bass_client, GATT_BASS_CLIENT_STATUS_SUCCESS);
                        }

                        break;
                    }

                    ptr = ptr->next;
                }

                bass_client->pending_cmd = bass_client_pending_none;
            }
            else
            {
                gattBassClientSendInitCfm(bass_client, GATT_BASS_CLIENT_STATUS_READ_ERR);

                /* Clear pending messages */
                MessageFlushTask((Task)&bass_client->lib_task);

                if(!ServiceHandleFreeInstanceData(bass_client->clnt_hndl))
                {
                    GATT_BASS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
                }
            }
        }
        break;

        case bass_client_read_pending:
        {
            /* Send GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CFM message to the application */
            bassClientSendReadBroadcastReceiveStateCfm(bass_client,
                                                       read_cfm->status,
                                                       read_cfm->size_value,
                                                       read_cfm->value);

            bass_client->pending_cmd = bass_client_pending_none;
        }
        break;

        case bass_client_read_pending_ccc:
        {
            bass_client_handle_t *ptr = bass_client->client_data.broadcast_receive_state_handles_first;

            while(ptr)
            {
                if (read_cfm->handle == ptr->handle_ccc)
                {
                    uint8 source_id = 0;

                    if(bassClientSourceIdFromCccHandle(bass_client,
                                                       read_cfm->handle,
                                                       &source_id))
                    {
                        /* Send read GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CCC_CFM message to the application */
                        bassClientSendReadBroadcastReceiveStateCccCfm(bass_client,
                                                                      read_cfm->status,
                                                                      read_cfm->size_value,
                                                                      read_cfm->value,
                                                                      source_id);
                    }
                    else
                    {
                        bassClientSendReadBroadcastReceiveStateCccCfm(bass_client,
                                                                      GATT_BASS_CLIENT_STATUS_INVALID_PARAMETER,
                                                                      read_cfm->size_value,
                                                                      read_cfm->value,
                                                                      source_id);
                    }
                    break;
                }

                ptr = ptr->next;
            }

            bass_client->pending_cmd = bass_client_pending_none;
        }
        break;

        default:
        {
            /* No other pending read values expected */
            GATT_BASS_CLIENT_DEBUG_PANIC(("GBASSC: Read value response not expected [0x%x]\n",
                                          bass_client->pending_cmd));
            bass_client->pending_cmd = bass_client_pending_none;
        }
        break;
    }
}

/***************************************************************************/
static void bassClientHandleReadRequest(const GBASSC *gatt_bass_client,
                                        uint16 handle,
                                        bool isReadCcc)
{
    if (isReadCcc)
    {
        MAKE_BASS_CLIENT_INTERNAL_MESSAGE(BASS_CLIENT_INTERNAL_MSG_READ_CCC);

        message->handle = handle;

        MessageSendConditionally((Task)&gatt_bass_client->lib_task,
                                 BASS_CLIENT_INTERNAL_MSG_READ_CCC,
                                 message,
                                 &gatt_bass_client->pending_cmd);
    }
    else
    {
        MAKE_BASS_CLIENT_INTERNAL_MESSAGE(BASS_CLIENT_INTERNAL_MSG_READ);

        message->handle = handle;

        MessageSendConditionally((Task)&gatt_bass_client->lib_task,
                                 BASS_CLIENT_INTERNAL_MSG_READ,
                                 message,
                                 &gatt_bass_client->pending_cmd);
    }
}

/****************************************************************************/
GattBassClientStatus GattBassClientReadBroadcasReceiveStateCccRequest(ServiceHandle clntHndl,
                                                                      uint8 sourceId,
                                                                      bool allSource)
{
    GBASSC *gatt_bass_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_bass_client)
    {
        if (allSource)
        {
            bass_client_handle_t *ptr = gatt_bass_client->client_data.broadcast_receive_state_handles_first;

            while(ptr)
            {
                bassClientHandleReadRequest(gatt_bass_client,
                                            ptr->handle_ccc,
                                            TRUE);
                ptr = ptr->next;
            }
        }
        else
        {
            uint16 handle = bassClientHandleFromSourceId(gatt_bass_client, sourceId, TRUE);

            if(handle)
            {
                bassClientHandleReadRequest(gatt_bass_client,
                                            handle,
                                            TRUE);
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
GattBassClientStatus GattBassClientReadBroadcastReceiveStateRequest(ServiceHandle clntHndl,
                                                                    uint8 sourceId,
                                                                    bool allSource)
{
    GBASSC *gatt_bass_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_bass_client)
    {
        if (allSource)
        {
            bass_client_handle_t *ptr = gatt_bass_client->client_data.broadcast_receive_state_handles_first;

            while(ptr)
            {
                bassClientHandleReadRequest(gatt_bass_client,
                                            ptr->handle,
                                            FALSE);
                ptr = ptr->next;
            }
        }
        else
        {
            uint16 handle = bassClientHandleFromSourceId(gatt_bass_client, sourceId, FALSE);

            if(handle)
            {
                bassClientHandleReadRequest(gatt_bass_client,
                                            handle,
                                            FALSE);
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

/*******************************************************************************/
void bassClientSendReadBroadcastReceiveStateCfm(GBASSC *bass_client,
                                                gatt_status_t status,
                                                uint16 value_size,
                                                uint8 *value)
{
    MAKE_BASS_CLIENT_MESSAGE_WITH_LEN(GattBassClientReadBroadcastReceiveStateCfm,
                                      value_size);

    message->clntHndl = bass_client->clnt_hndl;
    message->status = status;

    if (value)
    {
        /* The format of Broadcast Receive State characteristic is:
         * value[0] -> Source_ID
         * value[1] -> Source_Address_Type
         * value[2] -> value[7] -> Source_Address
         * value[8] -> Source_Adv_SID
         * value[9] -> PA_Sync_State
         * value[10] - value[13] -> BIS_Sync State
         * value[14] -> BIG_Encryption
         * value[15] -> Metadata_Length
         * value[16] - value[Metadata_Length-1] -> Metada
         * NOTE: Metadata exists only if the Metadata_Length parameter value is ≠ 0x00.
        */
        message->sourceId = value[0];
        message->sourceAddress.type = value[1];

        message->sourceAddress.addr.nap = (((uint16) value[7]) << 8) | ((uint16) value[6]);
        message->sourceAddress.addr.uap = value[5];
        message->sourceAddress.addr.lap = (((uint32) value[4]) << 16) |
                                           (((uint32) value[3]) << 8) |
                                           ((uint32) value[2]);

        message->advSid = value[8];
        message->paSyncState = value[9];

        message->bisSyncState = ((uint32) value[13] << 24)   |
                                 ((uint32) value[12] << 16 ) |
                                 ((uint32) value[11] << 8 )  |
                                 ((uint32) value[10]);

        message->bigEncryption = value[14];

        message->metadataLen = value[15];

        if(message->metadataLen)
        {
            bassClientSwapByteTrasmissionOrder(&value[16],
                                               message->metadataLen,
                                               message->metadataValue);
        }
        else
        {
            message->metadataValue[0] = 0;
        }
    }

    MessageSend(bass_client->app_task,
                GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CFM,
                message);
}
