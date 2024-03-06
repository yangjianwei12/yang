/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_bass_client.h"
#include "gatt_bass_client_private.h"
#include "gatt_bass_client_debug.h"
#include "gatt_bass_client_common.h"

extern gatt_bass_client *bass_client_main;

static void bassClientSendTerminateCfm(GBASSC *bass_client, GattBassClientStatus status)
{
    bool res = FALSE;
    AppTask app_task = bass_client->app_task;

    /* We need to calculate the right number of elements of broadcast_receive_state_handles
     * in GATT_BASS_CLIENT_TERMINATE_CFM message: this number will be used to allocate the right
     * amount of memory for the message.
     * broadcast_receive_state_handles will contain the handles of all the Broadcast Receive State
     * Characteristics on the remote device and the handles of their Client Characteristic
     * Configurations. So, if the number of Broadcast Receive State characteristics on the remote
     * device is equal to broadcast_source_num, the elements of the array will be
     * 2 times broadcast_source_num.
     */
    uint16 len = (status == GATT_BASS_CLIENT_STATUS_SUCCESS)? (2 * bass_client->client_data.broadcast_source_num) : 0;

    MAKE_BASS_CLIENT_MESSAGE_WITH_LEN_U16(GattBassClientTerminateCfm, len);

    message->clntHndl = bass_client->srvcElem->service_handle;

    if (len)
    {
        uint8 i = 0;
        bass_client_handle_t *ptr = NULL;

        message->startHandle = bass_client->client_data.start_handle;
        message->endHandle = bass_client->client_data.end_handle;
        message->broadcastSourceNum = bass_client->client_data.broadcast_source_num;
        message->broadcastAudioScanControlPointHandle = bass_client->client_data.broadcast_audio_scan_control_point_handle;

        ptr = bass_client->client_data.broadcast_receive_state_handles_first;

        while(ptr)
        {
            message->broadcastReceiveStateHandles[i++] = ptr->handle;
            message->broadcastReceiveStateHandles[i++] = ptr->handle_ccc;
            ptr = ptr->next;
        }

        /* Destroy the list contained in the memory instance before to free it */
        bassClientDestroyBroadcastReceiveStateHandlesList(bass_client);

        res = ServiceHandleFreeInstanceData(bass_client->srvcElem->service_handle);

        if (res)
        {
            BASS_REMOVE_SERVICE_HANDLE(bass_client_main->service_handle_list, message->clntHndl);
            message->status = GATT_BASS_CLIENT_STATUS_SUCCESS;
        }
        else
        {
            message->status = GATT_BASS_CLIENT_STATUS_FAILED;
        }
    }
    else
    {
        message->startHandle = 0;
        message->endHandle = 0;
        message->broadcastSourceNum = 0;
        message->broadcastAudioScanControlPointHandle = 0;
        message->broadcastReceiveStateHandles[0] = 0;
        message->status = status;
    }

    BassMessageSend(app_task, GATT_BASS_CLIENT_TERMINATE_CFM, message); 
}

/****************************************************************************/
void GattBassClientTerminateReq(ServiceHandle clntHndl)
{
    GBASSC *bass_client = ServiceHandleGetInstanceData(clntHndl);

    if (bass_client)
    {
        /* Unregister with the GATT Manager and verify the result */
        CsrBtGattUnregisterReqSend(bass_client->srvcElem->gattId);
        bassClientSendTerminateCfm(bass_client, GATT_BASS_CLIENT_STATUS_SUCCESS);
    }
    else
    {
        gattBassClientPanic();
    }
}
