/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <string.h>

#include <service_handle.h>

#include "gatt_tds_client.h"
#include "gatt_tds_client_private.h"
#include "gatt_tds_client_init.h"
#include "gatt_tds_client_discovery.h"
#include "gatt_tds_client_debug.h"
#include "gatt_tds_client_common_util.h"

extern GattTdsClient *tdsClientMain;

/******************************************************************************/
void gattTdsClientSendInitCfm(GTDSC *gattTdsClient,
                              GattTdsClientStatus status)
{
    GattTdsClientInitCfm *message = CsrPmemAlloc(sizeof(*message));

    message->cid = gattTdsClient->srvcElem->cid;
    message->srvcHndl = gattTdsClient->srvcElem->service_handle;
    message->status = status;

    TdsMessageSend(gattTdsClient->appTask, GATT_TDS_CLIENT_INIT_CFM, message);
}


/****************************************************************************/
void GattTdsClientInitReq(AppTask theAppTask,
                          const GattTdsClientInitData   *init_data,
                          const GattTdsClientDeviceData *device_data)
{
    GTDSC * gattTdsClient = NULL;
    GattClientRegistrationParams registration_params;
    ServiceHandle srvcHndl = 0;
    
    if ((theAppTask == CSR_SCHED_QID_INVALID) || (init_data == NULL))
    {
        GATT_TDS_CLIENT_PANIC("Invalid initialisation parameters\n");
    }
    srvcHndl = getTdsServiceHandle(&gattTdsClient, &(tdsClientMain->serviceHandleList));

    if (gattTdsClient)
    {
        memset(&registration_params, 0, sizeof(GattClientRegistrationParams));

        gattTdsClient->libTask = CSR_BT_TDS_CLIENT_IFACEQUEUE;
        /* Store the Task function parameter.
           All library messages need to be sent here */
        gattTdsClient->appTask = theAppTask;

        if (device_data)
        {
            memcpy(&(gattTdsClient->handles), device_data, sizeof(GattTdsClientDeviceData));
        }
        else
        {
            memset(&(gattTdsClient->handles), 0, sizeof(GattTdsClientDeviceData));
        }

        /* Save the start and the end handles */
        gattTdsClient->handles.startHandle = init_data->startHandle;
        gattTdsClient->handles.endHandle = init_data->endHandle;

        gattTdsClient->srvcElem->cid = init_data->cid;

        /* Setup data required for Gatt Client to be registered with the GATT */
        registration_params.cid = init_data->cid;
        registration_params.startHandle = init_data->startHandle;
        registration_params.endHandle = init_data->endHandle;

        if (gattRegisterTdsClient(&registration_params, gattTdsClient))
        {
            if (!device_data)
            {
                CsrBtGattDiscoverAllCharacOfAServiceReqSend(gattTdsClient->srvcElem->gattId,
                                                            gattTdsClient->srvcElem->cid,
                                                            gattTdsClient->handles.startHandle,
                                                            gattTdsClient->handles.endHandle);
            }
            else
            {
                gattTdsClientSendInitCfm(gattTdsClient, GATT_TDS_CLIENT_STATUS_SUCCESS);
            }
        }
        else
        {
            GATT_TDS_CLIENT_ERROR("Register with the GATT failed!\n");
            gattTdsClientSendInitCfm(gattTdsClient, GATT_TDS_CLIENT_STATUS_FAILED);
        }
    }
    else
    {
        GattTdsClientInitCfm *message = CsrPmemAlloc(sizeof(*message));

        message->cid = init_data->cid;
        message->srvcHndl = srvcHndl;
        message->status = GATT_TDS_CLIENT_STATUS_FAILED;

        TdsMessageSend(theAppTask, GATT_TDS_CLIENT_INIT_CFM, message);
    }
}
