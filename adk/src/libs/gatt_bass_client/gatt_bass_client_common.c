/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_bass_client_debug.h"
#include "gatt_bass_client_private.h"
#include "gatt_bass_client_common.h"

/*******************************************************************************/
void bassClientSendInternalMsgInitRead(GBASSC *gatt_bass_client)
{
    bass_client_handle_t *ptr = gatt_bass_client->client_data.broadcast_receive_state_handles_first;

    while(ptr)
    {
        MAKE_BASS_CLIENT_INTERNAL_MESSAGE(BASS_CLIENT_INTERNAL_MSG_INIT_READ);

        message->handle = ptr->handle;

        MessageSendConditionally((Task)&gatt_bass_client->lib_task,
                                 BASS_CLIENT_INTERNAL_MSG_INIT_READ,
                                 message,
                                 &gatt_bass_client->pending_cmd);

        ptr = ptr->next;
    }
}

/*******************************************************************************/
void bassClientAddHandle(GBASSC *bass_inst,
                         uint8 source_id,
                         uint16 handle,
                         uint16 handle_ccc)
{
    if (!bass_inst->client_data.broadcast_receive_state_handles_first)
    {
        /* It's the first time I'm adding a new handle in the list */
        bass_inst->client_data.broadcast_receive_state_handles_first = PanicUnlessMalloc(sizeof(bass_client_handle_t));
        bass_inst->client_data.broadcast_receive_state_handles_first->source_id = source_id;
        bass_inst->client_data.broadcast_receive_state_handles_first->handle = handle;
        bass_inst->client_data.broadcast_receive_state_handles_first->handle_ccc = handle_ccc;
        bass_inst->client_data.broadcast_receive_state_handles_first->next = NULL;
        bass_inst->client_data.broadcast_receive_state_handles_last = bass_inst->client_data.broadcast_receive_state_handles_first;
    }
    else
    {
        /* There is already other elements in the list */
        bass_inst->client_data.broadcast_receive_state_handles_last->next = PanicUnlessMalloc(sizeof(bass_client_handle_t));
        bass_inst->client_data.broadcast_receive_state_handles_last->next->source_id = source_id;
        bass_inst->client_data.broadcast_receive_state_handles_last->next->handle = handle;
        bass_inst->client_data.broadcast_receive_state_handles_last->next->handle_ccc = handle_ccc;
        bass_inst->client_data.broadcast_receive_state_handles_last->next->next = NULL;
        bass_inst->client_data.broadcast_receive_state_handles_last = bass_inst->client_data.broadcast_receive_state_handles_last->next;
    }
}

/*******************************************************************************/
void bassClientSendBroadcastReceiveStateSetNtfCfm(GBASSC *const bass_client,
                                                  const gatt_status_t status,
                                                  uint8 source_id)
{
    MAKE_BASS_CLIENT_MESSAGE(GattBassClientBroadcastReceiveStateSetNtfCfm);

    message->clntHndl = bass_client->clnt_hndl;
    message->sourceId = source_id;
    message->status = status;

    MessageSend(bass_client->app_task,
                GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_SET_NTF_CFM,
                message);
}

/*******************************************************************************/
void bassClientSendBroadcastAudioScanControlOpCfm(GBASSC *const bass_client,
                                                  const gatt_status_t status,
                                                  GattBassClientMessageId id)
{
    if (id != GATT_BASS_CLIENT_MESSAGE_TOP)
    {
        /* We will use GATT_BASS_CLIENT_REMOTE_SCAN_STOP_CFM to
         * create the message, because the structute of all the write confermations
         * is the same, but we will send the right message using the id parameter */
        MAKE_BASS_CLIENT_MESSAGE(GattBassClientRemoteScanStopCfm);

        /* Fill in client reference */
        message->clntHndl = bass_client->clnt_hndl;

        /* Fill in the status */
        message->status = status;

        /* Send the confirmation message to app task  */
        MessageSend(bass_client->app_task, id, message);
    }
}

/*******************************************************************************/
GattBassClientStatus bassClientConvertGattStatus(gatt_status_t status)
{
    if (status == gatt_status_success)
        return GATT_BASS_CLIENT_STATUS_SUCCESS;
    else
        return GATT_BASS_CLIENT_STATUS_FAILED;
}

/*******************************************************************************/
bool bassClientSourceIdFromCccHandle(GBASSC *bass_client,
                                     uint16 handle_ccc,
                                     uint8 *source_id)
{
    bass_client_handle_t *ptr = bass_client->client_data.broadcast_receive_state_handles_first;

    while (ptr)
    {
        if(ptr->handle_ccc == handle_ccc)
        {
            (*source_id) = ptr->source_id;
            return TRUE;
        }

        ptr = ptr->next;
    }

    return FALSE;
}

/*******************************************************************************/
void bassClientDestroyBroadcastReceiveStateHandlesList(GBASSC *bass_inst)
{
    bass_client_handle_t *ptr = bass_inst->client_data.broadcast_receive_state_handles_first;
    bass_client_handle_t *tmp = NULL;

    while(ptr)
    {
        /* Save the next element of the list. */
        tmp = ptr->next;

        /* Free the actual element */
        free(ptr);

        /* Point to the next element of the list. */
        ptr = tmp;
    }

    bass_inst->client_data.broadcast_receive_state_handles_last = NULL;
    bass_inst->client_data.broadcast_receive_state_handles_first = NULL;
}

/*******************************************************************************/
void bassClientSendReadBroadcastReceiveStateCccCfm(GBASSC *bass_client,
                                                   gatt_status_t status,
                                                   uint16 size_value,
                                                   const uint8 *value,
                                                   uint8 source_id)
{
    MAKE_BASS_CLIENT_MESSAGE_WITH_LEN(GattBassClientReadBroadcastReceiveStateCccCfm, size_value);

    message->clntHndl = bass_client->clnt_hndl;
    message->status = status;
    message->sizeValue = size_value;
    message->sourceId = source_id;

    memmove(message->value, value, size_value);

    MessageSend(bass_client->app_task, GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CCC_CFM, message);
}

/*******************************************************************************/
uint16 bassClientHandleFromSourceId(GBASSC *bass_client,
                                    uint8 source_id,
                                    bool isCccHandle)
{
    bass_client_handle_t *ptr = bass_client->client_data.broadcast_receive_state_handles_first;

    while(ptr)
    {
        if(ptr->source_id == source_id)
        {
            if(isCccHandle)
                return ptr->handle_ccc;
            else
                return ptr->handle;
        }

        ptr = ptr->next;
    }

    return 0;
}

/****************************************************************************/
void bassClientSwapByteTrasmissionOrder(uint8 *value_to_swap,
                                        uint8 len,
                                        uint8 *value)
{
    uint8 i,j;

    for(i=0, j=len-1; i<len && j>=0; i++, j--)
    {
        value[i] = value_to_swap[j];
    }
}
