/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include <service_handle.h>

#include "gatt_telephone_bearer_client_init.h"

#include "gatt_telephone_bearer_client_discover.h"
#include "gatt_telephone_bearer_client_msg_handler.h"

/******************************************************************************/
void gattTbsClientSendInitComplete(GTBSC *tbs_client, GattTelephoneBearerClientStatus status)
{
    MAKE_TBSC_MESSAGE(GATT_TBS_CLIENT_INIT_CFM);
    message->status = status;

    /* if init failed then free instance data */
    if(status != GATT_TBS_CLIENT_STATUS_SUCCESS)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Init fail , 0x%02x\n", status));
        message->tbsHandle = 0;
        /* free any instance data, free function will check for NULL handles */
        ServiceHandleFreeInstanceData(tbs_client->srvcHandle);
    }
    else
    {
        message->tbsHandle = tbs_client->srvcHandle;
    }
    
    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_INIT_CFM, message);
}


/****************************************************************************/
void GattTelephoneBearerClientInit(
                           Task appTask,
                           uint16 cid,
                           uint16 startHandle,
                           uint16 endHandle,
                           GattTelephoneBearerClientDeviceData * device_data)
{
    gatt_manager_client_registration_params_t registration_params;
    GTBSC * tbs_client = NULL;
    ServiceHandle new_service_handle = 0;

    /* Check parameters */
    if (appTask == NULL)
    {
        gattTbsClientSendInitComplete(tbs_client, GATT_TBS_CLIENT_STATUS_INVALID_PARAMETER);
        return;
    }

    new_service_handle = ServiceHandleNewInstance((void **) &tbs_client, sizeof(GTBSC));

    if (new_service_handle !=0)
    {
        memset(&registration_params, 0, sizeof(gatt_manager_client_registration_params_t));

        /* Set memory contents to all zeros */
        memset(tbs_client, 0, sizeof(GTBSC));

        /* Set up library handler for external messages */
        tbs_client->lib_task.handler = tbsClientMsgHandler;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        tbs_client->appTask = appTask;

        /* The next characteristic descriptor */
        tbs_client->nextDescriptorHandle = 0;

        tbs_client->srvcHandle = new_service_handle;

        /* Setup data required for TBS Service to be registered with the GATT Manager */
        registration_params.client_task = &tbs_client->lib_task;
        registration_params.cid = cid;
        registration_params.start_handle = startHandle;
        registration_params.end_handle = endHandle;

        /* Register with the GATT Manager and verify the result */
        if (GattManagerRegisterClient(&registration_params) == gatt_manager_status_success)
        {
            /* If the device is already known, get the persistent data, */
            if (device_data)
            {
                /* Don't need to discover data from the device; use the data supplied instead */
                gattTbsClientSendInitComplete(tbs_client, GATT_TBS_CLIENT_STATUS_SUCCESS);
            }
            else
            {
                /* Discover all characteristics and descriptors after successful registration */
                discoverAllTbsCharacteristics(tbs_client);
            }
        }
        else
        {   /* register failed */
            gattTbsClientSendInitComplete(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED);
        }
    }
    else
    {
       gattTbsClientSendInitComplete(tbs_client, GATT_TBS_CLIENT_STATUS_INSUFFICIENT_RESOURCES);
    }

    return;
}
