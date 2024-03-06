/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include <string.h>
#include <stdio.h>

#include <service_handle.h>
#include "csr_pmem.h"

#include "gatt_telephone_bearer_client_init.h"

#include "gatt_telephone_bearer_client_discover.h"
#include "gatt_telephone_bearer_client_util.h"

#ifdef GATT_DATA_LOGGER
#include "gatt_data_logger_lib.h"
#endif /* GATT_DATA_LOGGER */

gatt_tbs_client *tbs_client_main;

/******************************************************************************/
void gattTbsClientSendInitComplete(GTBSC *tbs_client, GattTelephoneBearerClientStatus status)
{
    MAKE_TBSC_MESSAGE(GattTelephoneBearerClientInitCfm);
    message->status = status;
    message->cid = tbs_client->srvcElem->cid;

    /* if init failed then free instance data */
    if(status != GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS)
    {
        GATT_TBS_CLIENT_INFO("GTBSC: Init fail");
        message->tbsHandle = 0;
        /* free any instance data, free function will check for NULL handles */
        ServiceHandleFreeInstanceData(tbs_client->srvcHandle);
    }
    else
    {
        message->tbsHandle = tbs_client->srvcElem->service_handle;
    }
    
    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_INIT_CFM, message);

#ifdef GATT_DATA_LOGGER
    if(status == GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS)
    {
        (void)GattDataLoggerRegisterClientHandles(CSR_BT_TBS_CLIENT_IFACEQUEUE, 
            tbs_client->srvcElem->cid, (void *)&(tbs_client->handles));
    }
#endif /* GATT_DATA_LOGGER */
}


/****************************************************************************/
void GattTelephoneBearerClientInit(
                           AppTask appTask,
                           const GattTelephoneBearerClientInitData *initData,
                           const GattTelephoneBearerClientDeviceData * device_data)
{
    gatt_tbs_client_registration_params_t registration_params;
    GTBSC * tbs_client = NULL;
    ServiceHandle new_service_handle = 0;

    new_service_handle = getTbsClientServiceHandle(&tbs_client, &(tbs_client_main->service_handle_list));

    /* Check parameters */
    if (appTask == CSR_SCHED_QID_INVALID || (initData == NULL))
    {
        if(tbs_client)
            gattTbsClientSendInitComplete(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_INVALID_PARAMETER);
        return;
    }

    if (new_service_handle !=0)
    {
        memset(&registration_params, 0, sizeof(gatt_tbs_client_registration_params_t));

        /* Set up library handler for external messages */
        tbs_client->lib_task = CSR_BT_TBS_CLIENT_IFACEQUEUE;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        tbs_client->appTask = appTask;

        /* The next characteristic descriptor */
        tbs_client->nextDescriptorHandle = 0;

        tbs_client->srvcElem->service_handle = new_service_handle;
        tbs_client->srvcElem->cid = initData->cid;
        tbs_client->startHandle = initData->startHandle;
        tbs_client->endHandle = initData->endHandle;

        /* Setup data required for TBS Service to be registered with the GATT Manager */
        registration_params.cid = initData->cid;
        registration_params.start_handle = initData->startHandle;
        registration_params.end_handle = initData->endHandle;

        /* Register with the GATT Manager and verify the result */
        if (GattRegisterTbsClient(&registration_params, tbs_client))
        {
            /* If the device is already known, get the persistent data, */
            if (device_data)
            {
                memcpy(&tbs_client->handles, device_data, sizeof(GattTelephoneBearerClientDeviceData));

                /* Don't need to discover data from the device; use the data supplied instead */
                gattTbsClientSendInitComplete(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS);
            }
            else
            {
                /* Discover all characteristics and descriptors after successful registration */
                discoverAllTbsCharacteristics(tbs_client);
            }
        }
        else
        {   /* register failed */
            gattTbsClientSendInitComplete(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED);
        }
    }
    else
    {
       MAKE_TBSC_MESSAGE(GattTelephoneBearerClientInitCfm);
       message->status = GATT_TELEPHONE_BEARER_CLIENT_STATUS_INSUFFICIENT_RESOURCES;
       message->cid = initData->cid;
       message->tbsHandle = 0;
       TbsClientMessageSend(appTask, GATT_TELEPHONE_BEARER_CLIENT_INIT_CFM, message);
    }

    return;
}



/****************************************************************************/
void GattTelephoneBearerClientTerminateReq(ServiceHandle clntHndl)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(clntHndl);

    if (tbs_client)
    {
        /* Unregister with the GATT Manager and verify the result */
        bool res = FALSE;
        AppTask app_task = tbs_client->appTask;
        MAKE_TBSC_MESSAGE(GattTelephoneBearerClientTerminateCfm);

        CsrBtGattUnregisterReqSend(tbs_client->srvcElem->gattId);

        memcpy(&(message->deviceData), &(tbs_client->handles), sizeof(GattTelephoneBearerClientDeviceData));

        message->srvcHndl = tbs_client->srvcElem->service_handle;

        /* Remove the serevice element from main list */
        if(tbs_client_main)
        {
            TBS_REMOVE_SERVICE_HANDLE(tbs_client_main->service_handle_list, tbs_client->srvcElem);
        }

        /* Free the service instance memory */
        res = ServiceHandleFreeInstanceData(clntHndl);

        if (res)
        {
            message->status = GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS;
        }
        else
        {
            message->status = GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED;
        }

        TbsClientMessageSend(app_task, GATT_TELEPHONE_BEARER_CLIENT_TERMINATE_CFM, message);
    }
    else
    {
        GATT_TBS_CLIENT_PANIC("Invalid TBSC memory instance!\n");
    }
}


