/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_manager.h>

#include "gatt_aics_client.h"
#include "gatt_aics_client_private.h"

static void aicsClientSendTerminateCfm(GAICS *aics_client, GattAicsClientStatus status)
{
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientTerminateCfm);

    message->srvcHndl = aics_client->srvc_hndl;
    message->status = status;
    memcpy(&(message->deviceData), &(aics_client->handles), sizeof(GattAicsClientDeviceData));
    message->startHandle = aics_client->start_handle;
    message->endHandle = aics_client->end_handle;

    MessageSend(aics_client->app_task, GATT_AICS_CLIENT_TERMINATE_CFM, message);
}

/****************************************************************************/
void GattAicsClientTerminateReq(ServiceHandle clntHndl)
{
    GAICS *aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (aics_client)
    {
        /* Unregister with the GATT Manager and verify the result */
        if (GattManagerUnregisterClient(&(aics_client->lib_task)) == gatt_manager_status_success)
        {
            /* Clear pending messages */
            MessageFlushTask((Task)&aics_client->lib_task);

            aicsClientSendTerminateCfm(aics_client, GATT_AICS_CLIENT_STATUS_SUCCESS);

            ServiceHandleFreeInstanceData(clntHndl);
        }
        else
        {
            aicsClientSendTerminateCfm(aics_client, GATT_AICS_CLIENT_STATUS_FAILED);
        }
    }
    else
    {
        aicsClientSendTerminateCfm(aics_client, GATT_AICS_CLIENT_STATUS_FAILED);
    }
}

