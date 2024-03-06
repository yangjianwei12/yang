/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #60 $
******************************************************************************/

#include <string.h>

#include <service_handle.h>

#include "gatt_tmas_client.h"
#include "gatt_tmas_client_private.h"
#include "gatt_tmas_client_init.h"
#include "gatt_tmas_client_discovery.h"
#include "gatt_tmas_client_debug.h"
#include "gatt_tmas_client_common_util.h"

extern GattTmasClient *tmasClientMain;
/******************************************************************************/
void gattTmasClientSendInitCfm(GTMASC *gattTmasClient,
                               GattTmasClientStatus status)
{
    GattTmasClientInitCfm *message = CsrPmemZalloc(sizeof(*message));

    message->cid = gattTmasClient->srvcElem->cid;
    message->srvcHndl = gattTmasClient->srvcElem->service_handle;
    message->status = status;

    GattTmasClientMessageSend(gattTmasClient->appTask, GATT_TMAS_CLIENT_INIT_CFM, message);
}


/****************************************************************************/
void GattTmasClientInitReq(AppTask theAppTask,
                           const GattTmasClientInitData   *init_data,
                           const GattTmasClientDeviceData *device_data)
{
    GTMASC * gattTmasClient = NULL;
    GattTmasClientRegistrationParams registrationParams;
    ServiceHandle srvcHndl = 0;
    
    if ((theAppTask == CSR_SCHED_QID_INVALID) || (init_data == NULL))
    {
        GATT_TMAS_CLIENT_PANIC("Invalid initialisation parameters\n");
    }
    srvcHndl = gattTmasClientGetServiceHandle(&gattTmasClient, &(tmasClientMain->serviceHandleList));

    if (gattTmasClient)
    {
        memset(&registrationParams, 0, sizeof(GattTmasClientRegistrationParams));

        gattTmasClient->libTask = CSR_BT_TMAS_CLIENT_IFACEQUEUE;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        gattTmasClient->appTask = theAppTask;

        if (device_data)
        {
            memcpy(&(gattTmasClient->handles), device_data, sizeof(GattTmasClientDeviceData));
        }
        else
        {
            memset(&(gattTmasClient->handles), 0, sizeof(GattTmasClientDeviceData));
        }

        /* Save the start and the end handles */
        gattTmasClient->handles.startHandle = init_data->startHandle;
        gattTmasClient->handles.endHandle = init_data->endHandle;

        gattTmasClient->srvcElem->cid = init_data->cid;

        /* Setup data required for Gatt Client to be registered with the GATT */
        registrationParams.cid = init_data->cid;
        registrationParams.startHandle = init_data->startHandle;
        registrationParams.endHandle = init_data->endHandle;

        if (gattTmasClientRegister(&registrationParams, gattTmasClient))
        {
            if (!device_data)
            {
                CsrBtGattDiscoverAllCharacOfAServiceReqSend(gattTmasClient->srvcElem->gattId,
                                                            gattTmasClient->srvcElem->cid,
                                                            gattTmasClient->handles.startHandle,
                                                            gattTmasClient->handles.endHandle);
            }
            else
            {
                gattTmasClientSendInitCfm(gattTmasClient, GATT_TMAS_CLIENT_STATUS_SUCCESS);
            }
        }
        else
        {
            GATT_TMAS_CLIENT_ERROR("Register with the GATT failed!\n");
            gattTmasClientSendInitCfm(gattTmasClient, GATT_TMAS_CLIENT_STATUS_FAILED);

            /* Remove the serevice element from main list */
            if (tmasClientMain)
                GATT_TMAS_CLIENT_REMOVE_SERVICE_HANDLE(tmasClientMain->serviceHandleList, gattTmasClient->srvcElem);

            /* Free the service instance memory */
            ServiceHandleFreeInstanceData(srvcHndl);
        }
    }
    else
    {
        GattTmasClientInitCfm *message = CsrPmemZalloc(sizeof(*message));

        message->cid = init_data->cid;
        message->srvcHndl = srvcHndl;
        message->status = GATT_TMAS_CLIENT_STATUS_FAILED;

        GattTmasClientMessageSend(theAppTask, GATT_TMAS_CLIENT_INIT_CFM, message);
    }
}

GattTmasClientDeviceData *GattTmasClientGetDeviceDataReq(ServiceHandle clntHndl)
{
    GTMASC*tmasClient = ServiceHandleGetInstanceData(clntHndl);

    if (tmasClient)
    {
        GattTmasClientDeviceData *deviceData = CsrPmemZalloc(sizeof(GattTmasClientDeviceData));

        memcpy(deviceData, &(tmasClient->handles), sizeof(GattTmasClientDeviceData));
        return deviceData;
    }
    
    return NULL;
}

