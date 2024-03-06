/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>

#include <gatt_manager.h>

#include "gatt_vocs_client.h"
#include "gatt_vocs_client_private.h"

static void vocsClientSendTerminateCfm(GVOCS *vocs_client, GattVocsClientStatus status)
{
    MAKE_VOCS_CLIENT_MESSAGE(GattVocsClientTerminateCfm);

    message->srvcHdnl = vocs_client->srvc_hndl;
    message->status = status;
    message->startHandle = vocs_client->start_handle;
    message->endHandle = vocs_client->end_handle;
    memcpy(&(message->deviceData), &(vocs_client->handles), sizeof(GattVocsClientDeviceData));

    MessageSend(vocs_client->app_task, GATT_VOCS_CLIENT_TERMINATE_CFM, message);
}

/****************************************************************************/
void GattVocsClientTerminateReq(ServiceHandle clntHndl)
{
    GVOCS *vocs_client = ServiceHandleGetInstanceData(clntHndl);
    /* Check parameters */

    if (vocs_client)
    {
        /* Unregister with the GATT Manager and verify the result */
        if (GattManagerUnregisterClient(&(vocs_client->lib_task)) == gatt_manager_status_success)
        {
            /* Clear pending messages */
            MessageFlushTask((Task)&vocs_client->lib_task);

            vocsClientSendTerminateCfm(vocs_client, GATT_VOCS_CLIENT_STATUS_SUCCESS);

            ServiceHandleFreeInstanceData(clntHndl);
        }
        else
        {
             vocsClientSendTerminateCfm(vocs_client, GATT_VOCS_CLIENT_STATUS_FAILED);
        }
    }
    else
    {
        vocsClientSendTerminateCfm(vocs_client, GATT_VOCS_CLIENT_STATUS_FAILED);
    }
}

