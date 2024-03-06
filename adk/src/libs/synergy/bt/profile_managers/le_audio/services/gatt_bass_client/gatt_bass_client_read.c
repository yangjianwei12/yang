/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_bass_client.h"
#include "gatt_bass_client_debug.h"
#include "gatt_bass_client_common.h"
#include "gatt_bass_client_read.h"
#include "gatt_bass_client_init.h"

/****************************************************************************/
void bassClientHandleReadValueResp(GBASSC *bass_client,
                                   const CsrBtGattReadCfm *read_cfm)
{
    switch (bass_client->pending_cmd)
    {
        case bass_client_init_read_pending:
        {
            if (read_cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
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
                        if(read_cfm->valueLength)
                        {
                            ptr->source_id = read_cfm->value[0];
                        }

                        if (bass_client->counter == bass_client->client_data.broadcast_source_num)
                        {
                            bass_client->counter = 0;

                            bass_client->pending_cmd = bass_client_pending_none;
                            /* All the Broadcast Receive State Chracteristics have been read */
                            gattBassClientSendInitCfm(bass_client, GATT_BASS_CLIENT_STATUS_SUCCESS);
                        }

                        break;
                    }

                    ptr = ptr->next;
                }
            }
            else
            {
                gattBassClientSendInitCfm(bass_client, GATT_BASS_CLIENT_STATUS_READ_ERR);

                /* Clear pending messages */
                /*MessageFlushTask((Task)&bass_client->lib_task);*/

                if(!ServiceHandleFreeInstanceData(bass_client->srvcElem->service_handle))
                {
                    GATT_BASS_CLIENT_PANIC("Freeing of memory instance failed\n");
                }
            }
        }
        break;

        case bass_client_notify_read_pending:
        {
            MAKE_BASS_CLIENT_MESSAGE(GattBassClientBroadcastReceiveStateInd);

            bassClientSetSourceId(bass_client,
                                  read_cfm->handle,
                                  read_cfm->value[0]);

            bass_client->pending_cmd = bass_client_pending_none;

            message->clntHndl = bass_client->srvcElem->service_handle;

            bassClientExtractBroadcastReceiveStateCharacteristicValue(read_cfm->value, &(message->brsValue));

            /* Send the confirmation message to app task  */
            BassMessageSend(bass_client->app_task, GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_IND, message);
        }
        break;

        case bass_client_read_pending:
        {
            if(read_cfm->valueLength)
            {
                bassClientSetSourceId(bass_client,
                                      read_cfm->handle,
                                      read_cfm->value[0]);
            }
            else
            {
                bassClientSetSourceId(bass_client,
                                      read_cfm->handle,
                                      0);
            }

            /* Send GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CFM message to the application */
            bassClientSendReadBroadcastReceiveStateCfm(bass_client,
                                                       read_cfm->resultCode,
                                                       read_cfm->valueLength,
                                                       read_cfm->value);
            bass_client->counter += 1;
            if (bass_client->counter == bass_client->client_data.broadcast_source_num)
            {
                bass_client->counter = 0;
                bass_client->pending_cmd = bass_client_pending_none;
            }
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
                                                                      read_cfm->resultCode,
                                                                      read_cfm->valueLength,
                                                                      read_cfm->value,
                                                                      source_id);
                    }
                    else
                    {
                        bassClientSendReadBroadcastReceiveStateCccCfm(bass_client,
                                                                      GATT_BASS_CLIENT_STATUS_INVALID_PARAMETER,
                                                                      read_cfm->valueLength,
                                                                      read_cfm->value,
                                                                      source_id);
                    }
                    break;
                }

                ptr = ptr->next;
            }

            bass_client->counter += 1;
            if (bass_client->counter == bass_client->client_data.broadcast_source_num)
            {
                bass_client->counter = 0;
                 bass_client->pending_cmd = bass_client_pending_none;
            }

        }
        break;

        default:
        {
            /* No other pending read values expected */
            GATT_BASS_CLIENT_WARNING("GBASSC: Read value response not expected [0x%x]\n",
                                      bass_client->pending_cmd);
            bass_client->pending_cmd = bass_client_pending_none;
        }
        break;
    }
}

/***************************************************************************/
static void bassClientHandleReadRequest(const GBASSC *client,
                                        ServiceHandle clnt_hndl,
                                        uint16 handle,
                                        bool isReadCcc)
{
    if (isReadCcc)
    {
        MAKE_BASS_CLIENT_INTERNAL_MESSAGE(BASS_CLIENT_INTERNAL_MSG_READ_CCC);

        message->clnt_hndl = clnt_hndl;
        message->handle = handle;

        BassMessageSendConditionally(client->lib_task,
                                 BASS_CLIENT_INTERNAL_MSG_READ_CCC,
                                 message,
                                 &client->pending_cmd);
    }
    else
    {
        MAKE_BASS_CLIENT_INTERNAL_MESSAGE(BASS_CLIENT_INTERNAL_MSG_READ);

        message->clnt_hndl = clnt_hndl;
        message->handle = handle;

        BassMessageSendConditionally(client->lib_task,
                                 BASS_CLIENT_INTERNAL_MSG_READ,
                                 message,
                                 &client->pending_cmd);
    }
}

/****************************************************************************/
GattBassClientStatus GattBassClientReadBroadcastReceiveStateCccRequest(ServiceHandle clntHndl,
                                                                       uint8 sourceId,
                                                                       bool allSource)
{
    GBASSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        if (allSource)
        {
            bass_client_handle_t *ptr = client->client_data.broadcast_receive_state_handles_first;

            while(ptr)
            {
                bassClientHandleReadRequest(client,
                                            client->srvcElem->service_handle,
                                            ptr->handle_ccc,
                                            TRUE);
                ptr = ptr->next;
            }
        }
        else
        {
            uint16 handle = bassClientHandleFromSourceId(client, sourceId, TRUE);

            if(handle)
            {
                bassClientHandleReadRequest(client,
                                            client->srvcElem->service_handle,
                                            handle,
                                            TRUE);
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
GattBassClientStatus GattBassClientReadBroadcastReceiveStateRequest(ServiceHandle clntHndl,
                                                                    uint8 sourceId,
                                                                    bool allSource)
{
    GBASSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        if (allSource)
        {
            bass_client_handle_t *ptr = client->client_data.broadcast_receive_state_handles_first;

            while(ptr)
            {
                bassClientHandleReadRequest(client,
                                            client->srvcElem->service_handle,
                                            ptr->handle,
                                            FALSE);
                ptr = ptr->next;
            }
        }
        else
        {
            uint16 handle = bassClientHandleFromSourceId(client, sourceId, FALSE);

            if(handle)
            {
                bassClientHandleReadRequest(client,
                                            client->srvcElem->service_handle,
                                            handle,
                                            FALSE);
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

/*******************************************************************************/
void bassClientSendReadBroadcastReceiveStateCfm(GBASSC *bass_client,
                                                status_t status,
                                                uint16 size_value,
                                                uint8 *value)
{
    MAKE_BASS_CLIENT_MESSAGE(GattBassClientReadBroadcastReceiveStateCfm);

    message->clntHndl = bass_client->srvcElem->service_handle;
    message->status = status;

    if(!status && size_value)
    {
        bassClientExtractBroadcastReceiveStateCharacteristicValue(value, &(message->brsValue));
    }
    else
    {
        memset(&(message->brsValue), 0, sizeof(GattBassClientBroadcastReceiveState));
    }

    BassMessageSend(bass_client->app_task,
                GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CFM,
                message);
}
