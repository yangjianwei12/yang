/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_manager.h>

#include "gatt_bass_client.h"
#include "gatt_bass_client_private.h"
#include "gatt_bass_client_debug.h"
#include "gatt_bass_client_common.h"

static void bassClientSendTerminateCfm(GBASSC *bass_client, GattBassClientStatus status)
{
    bool res = FALSE;
    Task app_task = bass_client->app_task;
    bass_client_handle_t *ptr = NULL;
    uint8 i = 0;
    uint16 len = 0;

    if (status == GATT_BASS_CLIENT_STATUS_SUCCESS)
    {
        /* We need to calculate the right number of elements of broadcast_receive_state_handles
         * in GATT_BASS_CLIENT_TERMINATE_CFM message: this number will be used to allocate the right
         * amount of memory for the message.
         * broadcast_receive_state_handles will contain the handles of all the Broadcast Receive State
         * Characteristics on the remote device and the handles of their Client Characteristic
         * Configurations. So, if the number of Broadcast Receive State characteristics on the remote
         * device is equal to broadcast_source_num, the elements of the array will be
         * 2 times broadcast_source_num.
         */
        len = (2 * bass_client->client_data.broadcast_source_num);
    }

    MAKE_BASS_CLIENT_MESSAGE_WITH_LEN_U16(GattBassClientTerminateCfm, len);

    message->clntHndl = bass_client->clnt_hndl;

    if (len)
    {
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

        res = ServiceHandleFreeInstanceData(bass_client->clnt_hndl);

        if (res)
        {
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

    MessageSend(app_task, GATT_BASS_CLIENT_TERMINATE_CFM, message);
}

/****************************************************************************/
void GattBassClientTerminateReq(ServiceHandle clntHndl)
{
    GBASSC *bass_client = ServiceHandleGetInstanceData(clntHndl);

    if (bass_client)
    {
        /* Unregister with the GATT Manager and verify the result */
        if (GattManagerUnregisterClient(&bass_client->lib_task) == gatt_manager_status_success)
        {
            /* Clear pending messages */
            MessageFlushTask((Task)&bass_client->lib_task);
            bassClientSendTerminateCfm(bass_client, GATT_BASS_CLIENT_STATUS_SUCCESS);
        }
        else
        {
            GATT_BASS_CLIENT_DEBUG_PANIC(("Unregister with the GATT Manager failed!\n"));

            bassClientSendTerminateCfm(bass_client, GATT_BASS_CLIENT_STATUS_FAILED);
        }
    }
    else
    {
        GATT_BASS_CLIENT_DEBUG_PANIC(("Invalid BASS memory instance!\n"));
    }
}
