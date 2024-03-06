/* Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_vcs_client.h"
#include "gatt_vcs_client_private.h"
#include "gatt_vcs_client_debug.h"
#include "gatt_vcs_client_common_util.h"

extern gatt_vcs_client *vcs_client_main;
/****************************************************************************/
void GattVcsClientTerminateReq(ServiceHandle clntHndl)
{
    GVCSC *vcs_client = ServiceHandleGetInstanceData(clntHndl);

    if (vcs_client)
    {
        /* Unregister with the GATT Manager and verify the result */
        bool res = FALSE;
        AppTask app_task = vcs_client->app_task;
        MAKE_VCS_CLIENT_MESSAGE(GattVcsClientTerminateCfm);

        CsrBtGattUnregisterReqSend(vcs_client->srvcElem->gattId);

        memcpy(&(message->deviceData), &(vcs_client->handles), sizeof(GattVcsClientDeviceData));

        message->srvcHndl = vcs_client->srvcElem->service_handle;

        res = ServiceHandleFreeInstanceData(clntHndl);

        if (res)
        {
            message->status = GATT_VCS_CLIENT_STATUS_SUCCESS;
            VCS_REMOVE_SERVICE_HANDLE(vcs_client_main->service_handle_list, clntHndl);
        }
        else
        {
            message->status = GATT_VCS_CLIENT_STATUS_FAILED;
        }

        VcsMessageSend(app_task, GATT_VCS_CLIENT_TERMINATE_CFM, message);
    }
    else
    {
        GATT_VCS_CLIENT_PANIC("Invalid VCS memory instance!\n");
    }
}
