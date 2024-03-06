/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>

#include <gatt_manager.h>
#include <service_handle.h>

#include "gatt_vcs_client.h"
#include "gatt_vcs_client_private.h"
#include "gatt_vcs_client_init.h"
#include "gatt_vcs_client_discovery.h"
#include "gatt_vcs_client_msg_handler.h"
#include "gatt_vcs_client_debug.h"

/******************************************************************************/
void gattVcsClientSendInitCfm(GVCSC *gatt_vcs_client,
                              GattVcsClientStatus status)
{
    MAKE_VCS_CLIENT_MESSAGE(GattVcsClientInitCfm);
    
    if (status == GATT_VCS_CLIENT_STATUS_SUCCESS)
    {
        message->srvcHndl = gatt_vcs_client->srvc_hndl;
    }
    else
    {
        message->srvcHndl = 0;
    }

    message->status = status;
    
    MessageSend(gatt_vcs_client->app_task, GATT_VCS_CLIENT_INIT_CFM, message);
}


/****************************************************************************/
void GattVcsClientInitReq(Task                           theAppTask,
                          const GattVcsClientInitData   *initData,
                          const GattVcsClientDeviceData *deviceData)
{
    GVCSC * gatt_vcs_client = NULL;
    gatt_manager_client_registration_params_t registration_params;
    ServiceHandle srvc_hndl = 0;
    
    if ((theAppTask == NULL) || (initData == NULL))
    {
        GATT_VCS_CLIENT_PANIC(("Invalid initialisation parameters\n"));
    }

    srvc_hndl = ServiceHandleNewInstance((void **) &gatt_vcs_client, sizeof(GVCSC));

    if (gatt_vcs_client)
    {
        memset(&registration_params, 0, sizeof(gatt_manager_client_registration_params_t));

        /* Set memory contents to all zeros */
        memset(gatt_vcs_client, 0, sizeof(GVCSC));

        /* Set up library handler for external messages */
        gatt_vcs_client->lib_task.handler = gattVcsClientMsgHandler;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        gatt_vcs_client->app_task = theAppTask;

        if (deviceData)
        {
            memcpy(&(gatt_vcs_client->handles), deviceData, sizeof(GattVcsClientDeviceData));
        }
        else
        {
            memset(&(gatt_vcs_client->handles), 0, sizeof(GattVcsClientDeviceData));
        }

        /* Save the start and the end handles */
        gatt_vcs_client->handles.startHandle = initData->startHandle;
        gatt_vcs_client->handles.endHandle = initData->endHandle;

        gatt_vcs_client->pending_handle = 0;
        gatt_vcs_client->pending_cmd = vcs_client_pending_none;
        gatt_vcs_client->srvc_hndl = srvc_hndl;

        /* Setup data required for Gatt Client to be registered with the GATT Manager */
        registration_params.client_task = &gatt_vcs_client->lib_task;
        registration_params.cid = initData->cid;
        registration_params.start_handle = initData->startHandle;
        registration_params.end_handle = initData->endHandle;

        /* Register with the GATT Manager and verify the result */
        if (GattManagerRegisterClient(&registration_params) == gatt_manager_status_success)
        {
            if (!deviceData)
            {
                GattManagerDiscoverAllCharacteristics(&gatt_vcs_client->lib_task);
            }
            else
            {
                gattVcsClientSendInitCfm(gatt_vcs_client, GATT_VCS_CLIENT_STATUS_SUCCESS);
            }
        }
        else
        {
            GATT_VCS_CLIENT_DEBUG_PANIC(("Register with the GATT Manager failed!\n"));

            gattVcsClientSendInitCfm(gatt_vcs_client, GATT_VCS_CLIENT_STATUS_FAILED);

            if(!ServiceHandleFreeInstanceData(gatt_vcs_client->srvc_hndl))
            {
                GATT_VCS_CLIENT_PANIC(("Freeing of memory instance failed\n"));
            }
        }
    }
    else
    {
        MAKE_VCS_CLIENT_MESSAGE(GattVcsClientInitCfm);

        message->srvcHndl = 0;
        message->status = GATT_VCS_CLIENT_STATUS_FAILED;

        MessageSend(theAppTask, GATT_VCS_CLIENT_INIT_CFM, message);
    }
}
