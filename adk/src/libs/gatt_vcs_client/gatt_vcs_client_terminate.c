/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_manager.h>

#include "gatt_vcs_client.h"
#include "gatt_vcs_client_private.h"
#include "gatt_vcs_client_debug.h"

static void vcsClientSendTerminateCfm(GVCSC *vcs_client, GattVcsClientStatus status)
{
    MAKE_VCS_CLIENT_MESSAGE(GattVcsClientTerminateCfm);

    message->status = status;
    memcpy(&(message->deviceData), &(vcs_client->handles), sizeof(GattVcsClientDeviceData));

    MessageSend(vcs_client->app_task, GATT_VCS_CLIENT_TERMINATE_CFM, message);
}

/****************************************************************************/
void GattVcsClientTerminateReq(ServiceHandle clntHndl)
{
    GVCSC *vcs_client = ServiceHandleGetInstanceData(clntHndl);

    if (vcs_client)
    {
        /* Unregister with the GATT Manager and verify the result */
        if (GattManagerUnregisterClient(&vcs_client->lib_task) == gatt_manager_status_success)
        {
            bool res = FALSE;
            Task app_task = vcs_client->app_task;

            /* Clear pending messages */
            MessageFlushTask((Task)&vcs_client->lib_task);

            MAKE_VCS_CLIENT_MESSAGE(GattVcsClientTerminateCfm);
            memcpy(&(message->deviceData), &(vcs_client->handles), sizeof(GattVcsClientDeviceData));


            res = ServiceHandleFreeInstanceData(clntHndl);

            if (res)
            {
                message->status = GATT_VCS_CLIENT_STATUS_SUCCESS;
            }
            else
            {
                message->status = GATT_VCS_CLIENT_STATUS_FAILED;
            }

            MessageSend(app_task, GATT_VCS_CLIENT_TERMINATE_CFM, message);
        }
        else
        {
            vcsClientSendTerminateCfm(vcs_client, GATT_VCS_CLIENT_STATUS_FAILED);
        }
    }
    else
    {
        GATT_VCS_CLIENT_DEBUG_PANIC(("Invalid VCS memory instance!\n"));
    }
}
