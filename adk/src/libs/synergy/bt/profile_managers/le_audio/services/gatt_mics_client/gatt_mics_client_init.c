/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#include <service_handle.h>

#include "gatt_mics_client.h"
#include "gatt_mics_client_private.h"
#include "gatt_mics_client_init.h"
#include "gatt_mics_client_discovery.h"
#include "gatt_mics_client_debug.h"
#include "csr_pmem.h"
#include "gatt_mics_client_common_util.h"

extern gatt_mics_client *mics_client_main;

/******************************************************************************/
void gattMicsClientSendInitCfm(GMICSC *client,
                              GattMicsClientStatus status)
{
    MAKE_MICS_CLIENT_MESSAGE(GattMicsClientInitCfm);

    message->cid = client->srvcElem->cid;
    message->srvcHndl = client->srvcElem->service_handle;
    message->status = status;

    MicsMessageSend(client->app_task, GATT_MICS_CLIENT_INIT_CFM, message);
}


/****************************************************************************/
void GattMicsClientInitReq(AppTask theAppTask,
                          const GattMicsClientInitData *initData,
                          const GattMicsClientDeviceData *deviceData)
{
    GMICSC * client = NULL;
    gatt_client_registration_params_t registration_params;
    ServiceHandle srvc_hndl = 0;
    
    if ((theAppTask == CSR_SCHED_QID_INVALID) || (initData == NULL))
    {
        GATT_MICS_CLIENT_PANIC("Invalid initialisation parameters\n");
        return;
    }

    srvc_hndl = getMicsServiceHandle(&client, &(mics_client_main->service_handle_list));
    CSR_UNUSED(srvc_hndl);

    if (client)
    {
        CsrMemSet(&registration_params, 0, sizeof(gatt_client_registration_params_t));

        /* Set up library handler for external messages */
        client->lib_task = CSR_BT_MICS_CLIENT_IFACEQUEUE;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        client->app_task = theAppTask;

        if (deviceData)
        {
            CsrMemCpy(&(client->handles), deviceData, sizeof(GattMicsClientDeviceData));
        }
        else
        {
            CsrMemSet(&(client->handles), 0, sizeof(GattMicsClientDeviceData));
        }

        /* Save the start and the end handles */
        client->handles.startHandle = initData->startHandle;
        client->handles.endHandle = initData->endHandle;

        client->pending_handle = 0;
        client->pending_cmd = mics_client_pending_none;
        client->srvcElem->cid = initData->cid;

        /* Setup data required for Gatt Client to be registered with the GATT */

        registration_params.cid = initData->cid;
        registration_params.start_handle = initData->startHandle;
        registration_params.end_handle = initData->endHandle;

        if (GattMicsRegisterClient(&registration_params, client))
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
                gattMicsClientSendInitCfm(client, GATT_MICS_CLIENT_STATUS_SUCCESS);
            }
        }
        else
        {
            GATT_MICS_CLIENT_ERROR("Register with the GATT failed!\n");
            gattMicsClientSendInitCfm(client, GATT_MICS_CLIENT_STATUS_FAILED);
        }
    }
    else
    {
        MAKE_MICS_CLIENT_MESSAGE(GattMicsClientInitCfm);

        message->cid = initData->cid;
        message->srvcHndl = 0;
        message->status = GATT_MICS_CLIENT_STATUS_FAILED;

        MicsMessageSend(theAppTask, GATT_MICS_CLIENT_INIT_CFM, message);
    }
}
