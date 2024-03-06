/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>

#include <gatt_manager.h>
#include <service_handle.h>

#include "gatt_vocs_client.h"
#include "gatt_vocs_client_private.h"
#include "gatt_vocs_client_init.h"
#include "gatt_vocs_client_discovery.h"
#include "gatt_vocs_client_debug.h"
#include "gatt_vocs_client_msg_handler.h"

/******************************************************************************/
void vocsClientSendInitCfm(GVOCS *gatt_vocs_client,
                           GattVocsClientStatus status)
{
    MAKE_VOCS_CLIENT_MESSAGE(GattVocsClientInitCfm);

    if (status == GATT_VOCS_CLIENT_STATUS_SUCCESS)
    {
        message->srvcHdnl = gatt_vocs_client->srvc_hndl;
    }
    else
    {
        message->srvcHdnl = 0;
    }

    message->status = status;

    MessageSend(gatt_vocs_client->app_task, GATT_VOCS_CLIENT_INIT_CFM, message);
}

/****************************************************************************/
void GattVocsClientInitReq(Task theAppTask,
                           const GattVocsClientInitData   *initData,
                           const GattVocsClientDeviceData *deviceData)
{
    GVOCS * gatt_vocs_client = NULL;
    gatt_manager_client_registration_params_t registration_params;
    ServiceHandle srvc_hndl = 0;

    if ((theAppTask == NULL) || (initData == NULL))
    {
        GATT_VOCS_CLIENT_PANIC(("Invalid initialisation parameters\n"));
    }

    srvc_hndl = ServiceHandleNewInstance((void **)&gatt_vocs_client, sizeof(GVOCS));

    if (gatt_vocs_client)
    {
        memset(&registration_params, 0, sizeof(gatt_manager_client_registration_params_t));

        /* Set memory contents to all zeros */
        memset(gatt_vocs_client, 0, sizeof(GVOCS));

        /* Set up library handler for external messages */
        gatt_vocs_client->lib_task.handler = gattVocsClientMsgHandler;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        gatt_vocs_client->app_task = theAppTask;

        if (deviceData)
        {
            memcpy(&(gatt_vocs_client->handles), deviceData, sizeof(GattVocsClientDeviceData));
        }
        else
        {
            memset(&(gatt_vocs_client->handles), 0, sizeof(GattVocsClientDeviceData));
        }

        gatt_vocs_client->start_handle = initData->startHandle;
        gatt_vocs_client->end_handle = initData->endHandle;

        gatt_vocs_client->pending_handle = 0;
        gatt_vocs_client->srvc_hndl = srvc_hndl;
        gatt_vocs_client->pending_cmd = vocs_client_pending_none;

        /* Setup data required for Gatt Client to be registered with the GATT Manager */
        registration_params.client_task = &gatt_vocs_client->lib_task;
        registration_params.cid = initData->cid;
        registration_params.start_handle = initData->startHandle;
        registration_params.end_handle = initData->endHandle;

        /* Register with the GATT Manager and verify the result */
        if (GattManagerRegisterClient(&registration_params) == gatt_manager_status_success)
        {
            if (!deviceData)
            {
                GattManagerDiscoverAllCharacteristics(&gatt_vocs_client->lib_task);
            }
            else
            {
                vocsClientSendInitCfm(gatt_vocs_client, GATT_VOCS_CLIENT_STATUS_SUCCESS);
            }
        }
        else
        {
            GATT_VOCS_CLIENT_DEBUG_PANIC(("Register with the GATT Manager failed!\n"));

            vocsClientSendInitCfm(gatt_vocs_client, GATT_VOCS_CLIENT_STATUS_FAILED);

            if(!ServiceHandleFreeInstanceData(gatt_vocs_client->srvc_hndl))
            {
                GATT_VOCS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
            }
        }
    }
    else
    {
        vocsClientSendInitCfm(gatt_vocs_client, GATT_VOCS_CLIENT_STATUS_INSUFFICIENT_RESOURCES);
    }
}
