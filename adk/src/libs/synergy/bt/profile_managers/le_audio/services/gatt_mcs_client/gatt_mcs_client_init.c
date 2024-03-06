/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <string.h>

#include <service_handle.h>

#include "gatt_mcs_client.h"
#include "gatt_mcs_client_private.h"
#include "gatt_mcs_client_init.h"
#include "gatt_mcs_client_discovery.h"
#include "gatt_mcs_client_debug.h"
#include "gatt_mcs_client_common_util.h"

#ifdef GATT_DATA_LOGGER
#include "gatt_data_logger_lib.h"
#endif /* GATT_DATA_LOGGER */

extern GattMcsClient *mcsClientMain;
/******************************************************************************/
void gattMcsClientSendInitCfm(GMCSC *gattMcsClient,
                              GattMcsClientStatus status)
{
    GattMcsClientInitCfm *message = CsrPmemAlloc(sizeof(*message));

    message->cid = gattMcsClient->srvcElem->cid;
    message->srvcHndl = gattMcsClient->srvcElem->service_handle;
    message->status = status;

    McsMessageSend(gattMcsClient->appTask, GATT_MCS_CLIENT_INIT_CFM, message);

#ifdef GATT_DATA_LOGGER
    if(status == GATT_MCS_CLIENT_STATUS_SUCCESS)
    {
        (void)GattDataLoggerRegisterClientHandles(CSR_BT_MCS_CLIENT_IFACEQUEUE, 
            gattMcsClient->srvcElem->cid, (void *)&(gattMcsClient->handles));
    }
#endif /* GATT_DATA_LOGGER */
}


/****************************************************************************/
void GattMcsClientInitReq(AppTask theAppTask,
                          const GattMcsClientInitData   *init_data,
                          const GattMcsClientDeviceData *device_data)
{
    GMCSC * gattMcsClient = NULL;
    GattClientRegistrationParams registration_params;
    ServiceHandle srvcHndl = 0;
    
    if ((theAppTask == CSR_SCHED_QID_INVALID) || (init_data == NULL))
    {
        GATT_MCS_CLIENT_PANIC("Invalid initialisation parameters\n");
    }
    srvcHndl = getMcsServiceHandle(&gattMcsClient, &(mcsClientMain->serviceHandleList));

    if (gattMcsClient)
    {
        memset(&registration_params, 0, sizeof(GattClientRegistrationParams));

        gattMcsClient->libTask = CSR_BT_MCS_CLIENT_IFACEQUEUE;
        /* Store the Task function parameter.
           All library messages need to be sent here */
        gattMcsClient->appTask = theAppTask;

        if (device_data)
        {
            memcpy(&(gattMcsClient->handles), device_data, sizeof(GattMcsClientDeviceData));
        }
        else
        {
            memset(&(gattMcsClient->handles), 0, sizeof(GattMcsClientDeviceData));
        }

        /* Save the start and the end handles */
        gattMcsClient->handles.startHandle = init_data->startHandle;
        gattMcsClient->handles.endHandle = init_data->endHandle;

        gattMcsClient->writeCccCount = 0;
        gattMcsClient->pendingHandle = 0;
        gattMcsClient->pendingCmd = MCS_CLIENT_PENDING_OP_NONE;
        gattMcsClient->srvcElem->cid = init_data->cid;

        /* Setup data required for Gatt Client to be registered with the GATT */
        registration_params.cid = init_data->cid;
        registration_params.startHandle = init_data->startHandle;
        registration_params.endHandle = init_data->endHandle;

        CSR_UNUSED(srvcHndl);

        if (gattRegisterMcsClient(&registration_params, gattMcsClient))
        {
            if (!device_data)
            {
                CsrBtGattDiscoverAllCharacOfAServiceReqSend(gattMcsClient->srvcElem->gattId,
                                                            gattMcsClient->srvcElem->cid,
                                                            gattMcsClient->handles.startHandle,
                                                            gattMcsClient->handles.endHandle);
            }
            else
            {
                gattMcsClientSendInitCfm(gattMcsClient, GATT_MCS_CLIENT_STATUS_SUCCESS);
            }
        }
        else
        {
            GATT_MCS_CLIENT_ERROR("Register with the GATT failed!\n");
            gattMcsClientSendInitCfm(gattMcsClient, GATT_MCS_CLIENT_STATUS_FAILED);
        }
    }
    else
    {
        GattMcsClientInitCfm *message = CsrPmemAlloc(sizeof(*message));

        message->cid = init_data->cid;
        message->srvcHndl = 0;
        message->status = GATT_MCS_CLIENT_STATUS_FAILED;

        McsMessageSend(theAppTask, GATT_MCS_CLIENT_INIT_CFM, message);
    }
}
