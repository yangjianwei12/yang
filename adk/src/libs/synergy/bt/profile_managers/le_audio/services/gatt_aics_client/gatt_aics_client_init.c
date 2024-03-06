/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <string.h>

#include "csr_bt_gatt_lib.h"
#include <service_handle.h>

#include "gatt_aics_client.h"
#include "gatt_aics_client_private.h"
#include "gatt_aics_client_init.h"
#include "gatt_aics_client_discovery.h"
#include "gatt_aics_client_debug.h"
#include "gatt_aics_client_common_util.h"


extern gatt_aics_client *aics_client_main;

/******************************************************************************/
void gattAicsClientSendInitCfm(GAICS *client,
                               GattAicsClientStatus status)
{
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientInitCfm);

    message->cid = client->srvcElem->cid;
    message->srvcHndl = client->srvcElem->service_handle;
    message->status = status;

    AicsMessageSend(client->app_task, GATT_AICS_CLIENT_INIT_CFM, message);
}

/****************************************************************************/
void GattAicsClientInitReq(AppTask theAppTask,
                           const GattAicsClientInitData   *initData,
                           const GattAicsClientDeviceData *deviceData)
{
    GAICS * client = NULL;
    gatt_aics_client_registration_params_t registration_params;
    ServiceHandle srvc_hndl = 0;

    if ((theAppTask == CSR_SCHED_QID_INVALID) || (initData == NULL))
    {
        GATT_AICS_CLIENT_PANIC("Invalid initialisation parameters\n");
        return;
    }

    srvc_hndl = getAicsServiceHandle(&client, &(aics_client_main->service_handle_list));
    CSR_UNUSED(srvc_hndl);

    if (client)
    {
        memset(&registration_params, 0, sizeof(gatt_aics_client_registration_params_t));

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
        /* Set up library handler for external messages */
        client->lib_task = CSR_BT_AICS_CLIENT_IFACEQUEUE;
#endif
        /* Store the Task function parameter.
           All library messages need to be sent here */
        client->app_task = theAppTask;

        if (deviceData)
        {
            memcpy(&(client->handles), deviceData, sizeof(GattAicsClientDeviceData));
        }
        else
        {
            memset(&(client->handles), 0, sizeof(GattAicsClientDeviceData));
        }

        client->start_handle = initData->startHandle;
        client->end_handle = initData->endHandle;

        client->pending_handle = 0;
        client->pending_cmd = aics_client_pending_none;
        client->srvcElem->cid = initData->cid;

        /* Setup data required for Gatt Client to be registered with the GATT */
        registration_params.cid = initData->cid;
        registration_params.start_handle = initData->startHandle;
        registration_params.end_handle = initData->endHandle;

        if (GattRegisterAicsClient(&registration_params, client))
        {
            if (!deviceData)
            {
                CsrBtGattDiscoverAllCharacOfAServiceReqSend(client->srvcElem->gattId,
                                                            client->srvcElem->cid,
                                                            client->handles.startHandle,
                                                            client->handles.endHandle);
            }
            else
            {
                gattAicsClientSendInitCfm(client, GATT_AICS_CLIENT_STATUS_SUCCESS);
            }
        }
        else
        {
            GATT_AICS_CLIENT_ERROR("Register with the GATT failed!\n");
            gattAicsClientSendInitCfm(client, GATT_AICS_CLIENT_STATUS_FAILED);
        }

    }
    else
    {
        MAKE_AICS_CLIENT_MESSAGE(GattAicsClientInitCfm);

        message->cid = initData->cid;
        message->srvcHndl = 0;
        message->status = GATT_AICS_CLIENT_STATUS_INSUFFICIENT_RESOURCES;

        AicsMessageSend(theAppTask, GATT_AICS_CLIENT_INIT_CFM, message);
    }
}


