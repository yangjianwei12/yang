/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_manager.h>

#include "gatt_bass_client_common.h"
#include "gatt_bass_client_init.h"
#include "gatt_bass_client_msg_handler.h"
#include "gatt_bass_client_debug.h"

/******************************************************************************/
void gattBassClientSendInitCfm(GBASSC *gatt_bass_client,
                               GattBassClientStatus status)
{
    MAKE_BASS_CLIENT_MESSAGE(GattBassClientInitCfm);
    
    if (status == GATT_BASS_CLIENT_STATUS_SUCCESS)
    {
        message->clntHndl = gatt_bass_client->clnt_hndl;
    }
    else
    {
        message->clntHndl = 0;
    }

    message->status = status;
    
    MessageSend(gatt_bass_client->app_task, GATT_BASS_CLIENT_INIT_CFM, message);
}


/****************************************************************************/
void GattBassClientInitReq(Task theAppTask,
                           const GattBassClientInitData   *initData,
                           const GattBassClientDeviceData *deviceData)
{
    GBASSC * gatt_bass_client = NULL;
    gatt_manager_client_registration_params_t registration_params;
    ServiceHandle clnt_hndl = 0;
    
    if ((theAppTask == NULL) || (initData == NULL))
    {
        GATT_BASS_CLIENT_PANIC(("Invalid initialization parameters\n"));
    }

    clnt_hndl = ServiceHandleNewInstance((void **) &gatt_bass_client, sizeof(GBASSC));

    if (gatt_bass_client)
    {
        memset(&registration_params, 0, sizeof(gatt_manager_client_registration_params_t));

        /* Set memory contents to all zeros */
        memset(gatt_bass_client, 0, sizeof(GBASSC));

        /* Set up library handler for external messages */
        gatt_bass_client->lib_task.handler = gattBassClientMsgHandler;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        gatt_bass_client->app_task = theAppTask;

        gatt_bass_client->client_data.start_handle = initData->startHandle;
        gatt_bass_client->client_data.end_handle = initData->endHandle;

        if (deviceData)
        {
            uint8 i;

            gatt_bass_client->client_data.broadcast_source_num = deviceData->broadcastSourceNum;
            gatt_bass_client->client_data.broadcast_audio_scan_control_point_handle = deviceData->broadcastAudioScanControlPointHandle;

            for (i=0; i<deviceData->broadcastSourceNum; i++)
            {
                bassClientAddHandle(gatt_bass_client,
                                    0,
                                    deviceData->broadcastReceiveStateHandle[i],
                                    deviceData->broadcastReceiveStateHandleCcc[i]);
            }
        }
        else
        {
            gatt_bass_client->client_data.broadcast_source_num = 0;
            gatt_bass_client->client_data.broadcast_audio_scan_control_point_handle = 0;

            gatt_bass_client->client_data.broadcast_receive_state_handles_first = NULL;
            gatt_bass_client->client_data.broadcast_receive_state_handles_last = NULL;
        }

        gatt_bass_client->pending_handle = 0;
        gatt_bass_client->pending_cmd = bass_client_pending_none;
        gatt_bass_client->clnt_hndl = clnt_hndl;
        gatt_bass_client->counter = 0;

        /* Setup data required for Gatt Client to be registered with the GATT Manager */
        registration_params.client_task = &gatt_bass_client->lib_task;
        registration_params.cid = initData->cid;
        registration_params.start_handle = initData->startHandle;
        registration_params.end_handle = initData->endHandle;

        /* Register with the GATT Manager and verify the result */
        if (GattManagerRegisterClient(&registration_params) == gatt_manager_status_success)
        {
            if (!deviceData)
            {
                GattManagerDiscoverAllCharacteristics(&gatt_bass_client->lib_task);
            }
            else
            {
                /* We need to read all the Broadacast Receive State Characteristics to save their
                 * Source IDs. */
                bassClientSendInternalMsgInitRead(gatt_bass_client);
            }
        }
        else
        {
            GATT_BASS_CLIENT_DEBUG_PANIC(("Register with the GATT Manager failed!\n"));

            gattBassClientSendInitCfm(gatt_bass_client, GATT_BASS_CLIENT_STATUS_FAILED);
            if(!ServiceHandleFreeInstanceData(gatt_bass_client->clnt_hndl))
            {
                GATT_BASS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
            }
        }
    }
    else
    {
        MAKE_BASS_CLIENT_MESSAGE(GattBassClientInitCfm);

        message->clntHndl = 0;
        message->status = GATT_BASS_CLIENT_STATUS_FAILED;

        MessageSend(theAppTask, GATT_BASS_CLIENT_INIT_CFM, message);
    }
}
