/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>

#include <gatt_manager.h>
#include <service_handle.h>

#include "gatt_aics_client.h"
#include "gatt_aics_client_private.h"
#include "gatt_aics_client_init.h"
#include "gatt_aics_client_discovery.h"
#include "gatt_aics_client_debug.h"
#include "gatt_aics_client_msg_handler.h"

/******************************************************************************/
void gattAicsClientSendInitCfm(GAICS *gatt_aics_client,
                               GattAicsClientStatus status)
{
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientInitCfm);

    if (status == GATT_AICS_CLIENT_STATUS_SUCCESS)
    {
        message->srvcHndl = gatt_aics_client->srvc_hndl;
    }
    else
    {
        message->srvcHndl = 0;
    }

    message->status = status;

    MessageSend(gatt_aics_client->app_task, GATT_AICS_CLIENT_INIT_CFM, message);
}

/****************************************************************************/
void GattAicsClientInitReq(Task theAppTask,
                           const GattAicsClientInitData   *initData,
                           const GattAicsClientDeviceData *deviceData)
{
    GAICS * gatt_aics_client = NULL;
    gatt_manager_client_registration_params_t registration_params;
    ServiceHandle srvc_hndl = 0;

    if ((theAppTask == NULL) || (initData == NULL))
    {
        GATT_AICS_CLIENT_PANIC(("Invalid initialisation parameters\n"));
    }

    srvc_hndl = ServiceHandleNewInstance((void **)&gatt_aics_client, sizeof(GAICS));

    if (gatt_aics_client)
    {
        memset(&registration_params, 0, sizeof(gatt_manager_client_registration_params_t));

        /* Set memory contents to all zeros */
        memset(gatt_aics_client, 0, sizeof(GAICS));

        /* Set up library handler for external messages */
        gatt_aics_client->lib_task.handler = gattAicsClientMsgHandler;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        gatt_aics_client->app_task = theAppTask;

        if (deviceData)
        {
            memcpy(&(gatt_aics_client->handles), deviceData, sizeof(GattAicsClientDeviceData));
        }
        else
        {
            memset(&(gatt_aics_client->handles), 0, sizeof(GattAicsClientDeviceData));
        }

        gatt_aics_client->start_handle = initData->startHandle;
        gatt_aics_client->end_handle = initData->endHandle;

        gatt_aics_client->pending_handle = 0;
        gatt_aics_client->srvc_hndl = srvc_hndl;
        gatt_aics_client->pending_cmd = aics_client_pending_none;

        /* Setup data required for Gatt Client to be registered with the GATT Manager */
        registration_params.client_task = &gatt_aics_client->lib_task;
        registration_params.cid = initData->cid;
        registration_params.start_handle = gatt_aics_client->start_handle;
        registration_params.end_handle = gatt_aics_client->end_handle;

        /* Register with the GATT Manager and verify the result */
        if (GattManagerRegisterClient(&registration_params) == gatt_manager_status_success)
        {
            if (!deviceData)
            {
                GattManagerDiscoverAllCharacteristics(&gatt_aics_client->lib_task);
            }
            else
            {
                gattAicsClientSendInitCfm(gatt_aics_client, GATT_AICS_CLIENT_STATUS_SUCCESS);
            }
        }
        else
        {
            GATT_AICS_CLIENT_DEBUG_PANIC(("Register with the GATT Manager failed!\n"));

            gattAicsClientSendInitCfm(gatt_aics_client, GATT_AICS_CLIENT_STATUS_FAILED);

            if(!ServiceHandleFreeInstanceData(gatt_aics_client->srvc_hndl))
            {
                GATT_AICS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
            }
        }
    }
    else
    {
        gattAicsClientSendInitCfm(gatt_aics_client, GATT_AICS_CLIENT_STATUS_INSUFFICIENT_RESOURCES);
    }
}
