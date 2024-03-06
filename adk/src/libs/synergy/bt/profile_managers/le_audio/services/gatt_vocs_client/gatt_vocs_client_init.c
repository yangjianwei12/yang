/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <string.h>

#include "csr_bt_gatt_lib.h"
#include <service_handle.h>

#include "gatt_vocs_client.h"
#include "gatt_vocs_client_private.h"
#include "gatt_vocs_client_init.h"
#include "gatt_vocs_client_discovery.h"
#include "gatt_vocs_client_debug.h"
#include "gatt_vocs_client_common_util.h"

extern gatt_vocs_client *vocs_client_main;

/******************************************************************************/
void vocsClientSendInitCfm(GVOCS *client,
                           GattVocsClientStatus status)
{
    MAKE_VOCS_CLIENT_MESSAGE(GattVocsClientInitCfm);

    message->cid = client->srvcElem->cid;
    message->srvcHdnl = client->srvcElem->service_handle;
    message->status = status;

    VocsMessageSend(client->app_task, GATT_VOCS_CLIENT_INIT_CFM, message);
}

/****************************************************************************/
void GattVocsClientInitReq(AppTask theAppTask,
                           const GattVocsClientInitData   *initData,
                           const GattVocsClientDeviceData *deviceData)
{
    GVOCS * client = NULL;
    gatt_vocs_client_registration_params_t registration_params;
    ServiceHandle srvc_hndl = 0;

    if ((theAppTask == CSR_SCHED_QID_INVALID) || (initData == NULL))
    {
        GATT_VOCS_CLIENT_PANIC("Invalid initialisation parameters\n");
        return;
    }

    srvc_hndl = getVocsServiceHandle(&client, &(vocs_client_main->service_handle_list));
    CSR_UNUSED(srvc_hndl);

    if (client)
    {
        memset(&registration_params, 0, sizeof(gatt_vocs_client_registration_params_t));

#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
        /* Set up library handler for external messages */
        client->lib_task = CSR_BT_VOCS_CLIENT_IFACEQUEUE;
#endif
        /* Store the Task function parameter.
           All library messages need to be sent here */
        client->app_task = theAppTask;

        if (deviceData)
        {
            memcpy(&(client->handles), deviceData, sizeof(GattVocsClientDeviceData));
        }
        else
        {
            memset(&(client->handles), 0, sizeof(GattVocsClientDeviceData));
        }

        client->start_handle = initData->startHandle;
        client->end_handle = initData->endHandle;

        client->pending_handle = 0;
        client->pending_cmd = vocs_client_pending_none;
        client->srvcElem->cid = initData->cid;

        /* Setup data required for Gatt Client to be registered with the GATT */
        registration_params.cid = initData->cid;
        registration_params.start_handle = initData->startHandle;
        registration_params.end_handle = initData->endHandle;

        if (GattRegisterVocsClient(&registration_params, client))
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
                vocsClientSendInitCfm(client, GATT_VOCS_CLIENT_STATUS_SUCCESS);
            }
        }
        else
        {
            GATT_VOCS_CLIENT_PANIC("Register with the GATT failed!\n");
            vocsClientSendInitCfm(client, GATT_VOCS_CLIENT_STATUS_FAILED);
        }
    }
    else
    {
        MAKE_VOCS_CLIENT_MESSAGE(GattVocsClientInitCfm);

        message->cid = initData->cid;
        message->srvcHdnl = 0;
        message->status = GATT_VOCS_CLIENT_STATUS_FAILED;

        VocsMessageSend(theAppTask, GATT_VOCS_CLIENT_INIT_CFM, message);
    }
}
