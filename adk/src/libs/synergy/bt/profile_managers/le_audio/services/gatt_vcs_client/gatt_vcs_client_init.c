/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <string.h>

#include <service_handle.h>

#include "gatt_vcs_client.h"
#include "gatt_vcs_client_private.h"
#include "gatt_vcs_client_init.h"
#include "gatt_vcs_client_discovery.h"
#include "gatt_vcs_client_debug.h"
#include "csr_pmem.h"
#include "gatt_vcs_client_common_util.h"

extern gatt_vcs_client *vcs_client_main;

/******************************************************************************/
void gattVcsClientSendInitCfm(GVCSC *client,
                              GattVcsClientStatus status)
{
    MAKE_VCS_CLIENT_MESSAGE(GattVcsClientInitCfm);

    message->cid = client->srvcElem->cid;
    message->srvcHndl = client->srvcElem->service_handle;
    message->status = status;

    VcsMessageSend(client->app_task, GATT_VCS_CLIENT_INIT_CFM, message);
}


/****************************************************************************/
void GattVcsClientInitReq(AppTask theAppTask,
                          const GattVcsClientInitData *initData,
                          const GattVcsClientDeviceData *deviceData)
{
    GVCSC * client = NULL;
    gatt_client_registration_params_t registration_params;
    ServiceHandle srvc_hndl = 0;
    
    if ((theAppTask == CSR_SCHED_QID_INVALID) || (initData == NULL))
    {
        GATT_VCS_CLIENT_PANIC("Invalid initialisation parameters\n");
        return;
    }

    srvc_hndl = getServiceHandle(&client, &(vcs_client_main->service_handle_list));
    CSR_UNUSED(srvc_hndl);

    if (client)
    {
        memset(&registration_params, 0, sizeof(gatt_client_registration_params_t));

        /* Set up library handler for external messages */
        client->lib_task = CSR_BT_VCS_CLIENT_IFACEQUEUE;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        client->app_task = theAppTask;

        if (deviceData)
        {
            memcpy(&(client->handles), deviceData, sizeof(GattVcsClientDeviceData));
        }
        else
        {
            memset(&(client->handles), 0, sizeof(GattVcsClientDeviceData));
        }

        /* Save the start and the end handles */
        client->handles.startHandle = initData->startHandle;
        client->handles.endHandle = initData->endHandle;

        client->pending_handle = 0;
        client->pending_cmd = vcs_client_pending_none;
        client->srvcElem->cid = initData->cid;

        /* Setup data required for Gatt Client to be registered with the GATT */

        registration_params.cid = initData->cid;
        registration_params.start_handle = initData->startHandle;
        registration_params.end_handle = initData->endHandle;

        if (GattRegisterClient(&registration_params, client))
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
                gattVcsClientSendInitCfm(client, GATT_VCS_CLIENT_STATUS_SUCCESS);
            }
        }
        else
        {
            GATT_VCS_CLIENT_ERROR("Register with the GATT failed!\n");
            gattVcsClientSendInitCfm(client, GATT_VCS_CLIENT_STATUS_FAILED);
        }
    }
    else
    {
        MAKE_VCS_CLIENT_MESSAGE(GattVcsClientInitCfm);

        message->cid = initData->cid;
        message->srvcHndl = 0;
        message->status = GATT_VCS_CLIENT_STATUS_FAILED;

        VcsMessageSend(theAppTask, GATT_VCS_CLIENT_INIT_CFM, message);
    }
}
