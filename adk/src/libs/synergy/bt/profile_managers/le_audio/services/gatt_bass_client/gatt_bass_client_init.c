/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <string.h>
#include <service_handle.h>

#include "gatt_bass_client_common.h"
#include "gatt_bass_client_init.h"
#include "gatt_bass_client_msg_handler.h"
#include "gatt_bass_client_debug.h"
#include "csr_pmem.h"

extern gatt_bass_client *bass_client_main;

/******************************************************************************/
void gattBassClientSendInitCfm(GBASSC *client,
                               GattBassClientStatus status)
{
    MAKE_BASS_CLIENT_MESSAGE(GattBassClientInitCfm);
    
    if (status == GATT_BASS_CLIENT_STATUS_SUCCESS)
    {
        message->clntHndl = client->srvcElem->service_handle;
    }
    else
    {
        message->clntHndl = 0;
    }

    message->cid = client->srvcElem->cid;

    message->status = status;
    
    BassMessageSend(client->app_task, GATT_BASS_CLIENT_INIT_CFM, message);
}


/****************************************************************************/
void GattBassClientInitReq(AppTask theAppTask,
                           const GattBassClientInitData   *initData,
                           const GattBassClientDeviceData *deviceData)
{
    GBASSC * client = NULL;
    gatt_bass_client_registration_params_t registration_params;
    ServiceHandle clnt_hndl = 0;
    
    if ((theAppTask == CSR_SCHED_QID_INVALID) || (initData == NULL))
    {
        GATT_BASS_CLIENT_PANIC("Invalid initialization parameters\n");
        return;
    }

    clnt_hndl = getBassServiceHandle(&client, &(bass_client_main->service_handle_list));
    CSR_UNUSED(clnt_hndl);

    GATT_BASS_CLIENT_INFO("Bass:  GattBassClientInitReq\n\n");

    if (client)
    {

        memset(&registration_params, 0, sizeof(gatt_bass_client_registration_params_t));

        /* Set up library handler for external messages */
        client->lib_task = CSR_BT_BASS_CLIENT_IFACEQUEUE;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        client->app_task = theAppTask;

        client->client_data.start_handle = initData->startHandle;
        client->client_data.end_handle = initData->endHandle;
        client->srvcElem->cid = initData->cid;

        if (deviceData)
        {
            uint8 i;

            client->client_data.broadcast_source_num = deviceData->broadcastSourceNum;
            client->client_data.broadcast_audio_scan_control_point_handle = deviceData->broadcastAudioScanControlPointHandle;

            for (i=0; i<deviceData->broadcastSourceNum; i++)
            {
                bassClientAddHandle(client,
                                    0,
                                    deviceData->broadcastReceiveStateHandle[i],
                                    deviceData->broadcastReceiveStateHandleCcc[i]);
            }
        }
        else
        {
            client->client_data.broadcast_source_num = 0;
            client->client_data.broadcast_audio_scan_control_point_handle = 0;

            client->client_data.broadcast_receive_state_handles_first = NULL;
            client->client_data.broadcast_receive_state_handles_last = NULL;
        }

        client->pending_handle = 0;
        client->pending_cmd = bass_client_pending_none;
        client->counter = 0;

        /* Setup data required for Gatt Client to be registered with the GATT Manager */
        registration_params.cid = initData->cid;
        registration_params.start_handle = initData->startHandle;
        registration_params.end_handle = initData->endHandle;

        /* Register Client to GATT */
        if (GattBassRegisterClient(&registration_params, client))
        {
            if (!deviceData)
            {
                CsrBtGattDiscoverAllCharacOfAServiceReqSend(client->srvcElem->gattId,
                                                            client->srvcElem->cid,
                                                            client->client_data.start_handle,
                                                            client->client_data.end_handle);
            }
            else
            {
                /* We need to read all the Broadacast Receive State Characteristics to save their
                 * Source IDs. */
                bassClientSendInternalMsgInitRead(client);
            }
        }
        else
        {
            GATT_BASS_CLIENT_ERROR("Register with the GATT Manager failed!\n");

            gattBassClientSendInitCfm(client, GATT_BASS_CLIENT_STATUS_FAILED);
            if(!ServiceHandleFreeInstanceData(client->srvcElem->service_handle))
            {
                GATT_BASS_CLIENT_PANIC("Freeing of memory instance failed\n");
            }
        }
    }
    else
    {
        MAKE_BASS_CLIENT_MESSAGE(GattBassClientInitCfm);

        message->clntHndl = 0;
        message->cid = initData->cid;
        message->status = GATT_BASS_CLIENT_STATUS_FAILED;

        BassMessageSend(theAppTask, GATT_BASS_CLIENT_INIT_CFM, message);
    }
}

