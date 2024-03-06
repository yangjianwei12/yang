/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #2 $
******************************************************************************/

#include <string.h>

#include <service_handle.h>

#include "gatt_gmas_client.h"
#include "gatt_gmas_client_private.h"
#include "gatt_gmas_client_init.h"
#include "gatt_gmas_client_discovery.h"
#include "gatt_gmas_client_debug.h"
#include "gatt_gmas_client_common_util.h"

extern GattGmasClient *gmasClientMain;
/******************************************************************************/
void gattGmasClientSendInitCfm(GGMASC *gattGmasClient,
                               GattGmasClientStatus status)
{
    GattGmasClientInitCfm *message = CsrPmemZalloc(sizeof(*message));

    message->cid = gattGmasClient->srvcElem->cid;
    message->srvcHndl = gattGmasClient->srvcElem->service_handle;
    message->status = status;

    GattGmasClientMessageSend(gattGmasClient->appTask, GATT_GMAS_CLIENT_INIT_CFM, message);
}


/****************************************************************************/
void GattGmasClientInitReq(AppTask theAppTask,
                           const GattGmasClientInitData   *initData,
                           const GattGmasClientDeviceData *deviceData)
{
    GGMASC * gattGmasClient = NULL;
    GattGmasClientRegistrationParams registrationParams;
    ServiceHandle srvcHndl = 0;
    
    if ((theAppTask == CSR_SCHED_QID_INVALID) || (initData == NULL))
    {
        GATT_GMAS_CLIENT_PANIC("Invalid initialisation parameters\n");
    }
    srvcHndl = gattGmasClientGetServiceHandle(&gattGmasClient, &(gmasClientMain->serviceHandleList));

    if (gattGmasClient)
    {
        memset(&registrationParams, 0, sizeof(GattGmasClientRegistrationParams));

        gattGmasClient->libTask = CSR_BT_GMAS_CLIENT_IFACEQUEUE;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        gattGmasClient->appTask = theAppTask;

        if (deviceData)
        {
            memcpy(&(gattGmasClient->handles), deviceData, sizeof(GattGmasClientDeviceData));
        }
        else
        {
            memset(&(gattGmasClient->handles), 0, sizeof(GattGmasClientDeviceData));
        }

        /* Save the start and the end handles */
        gattGmasClient->handles.startHandle = initData->startHandle;
        gattGmasClient->handles.endHandle = initData->endHandle;

        gattGmasClient->srvcElem->cid = initData->cid;

        /* Setup data required for Gatt Client to be registered with the GATT */
        registrationParams.cid = initData->cid;
        registrationParams.startHandle = initData->startHandle;
        registrationParams.endHandle = initData->endHandle;

        if (gattGmasClientRegister(&registrationParams, gattGmasClient))
        {
            if (!deviceData)
            {
                CsrBtGattDiscoverAllCharacOfAServiceReqSend(gattGmasClient->srvcElem->gattId,
                                                            gattGmasClient->srvcElem->cid,
                                                            gattGmasClient->handles.startHandle,
                                                            gattGmasClient->handles.endHandle);
            }
            else
            {
                gattGmasClientSendInitCfm(gattGmasClient, GATT_GMAS_CLIENT_STATUS_SUCCESS);
            }
        }
        else
        {
            GATT_GMAS_CLIENT_ERROR("Register with the GATT failed!\n");
            gattGmasClientSendInitCfm(gattGmasClient, GATT_GMAS_CLIENT_STATUS_FAILED);

            /* Remove the serevice element from main list */
            if (gmasClientMain)
                GATT_GMAS_CLIENT_REMOVE_SERVICE_HANDLE(gmasClientMain->serviceHandleList, gattGmasClient->srvcElem);

            /* Free the service instance memory */
            ServiceHandleFreeInstanceData(srvcHndl);
        }
    }
    else
    {
        GattGmasClientInitCfm *message = CsrPmemZalloc(sizeof(*message));

        message->cid = initData->cid;
        message->srvcHndl = srvcHndl;
        message->status = GATT_GMAS_CLIENT_STATUS_FAILED;

        GattGmasClientMessageSend(theAppTask, GATT_GMAS_CLIENT_INIT_CFM, message);
    }
}

GattGmasClientDeviceData *GattGmasClientGetDeviceDataReq(ServiceHandle clntHndl)
{
    GGMASC*gmasClient = ServiceHandleGetInstanceData(clntHndl);

    if (gmasClient)
    {
        GattGmasClientDeviceData *deviceData = CsrPmemZalloc(sizeof(GattGmasClientDeviceData));

        memcpy(deviceData, &(gmasClient->handles), sizeof(GattGmasClientDeviceData));
        return deviceData;
    }
    
    return NULL;
}

