/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_mcs_client_common_util.h"
#include "gatt_mcs_client_debug.h"

/****************************************************************************/
void GattMcsClientTerminateReq(ServiceHandle clntHndl)
{
    GMCSC *mcsClient = ServiceHandleGetInstanceData(clntHndl);

    if (mcsClient)
    {
        bool res = FALSE;
        AppTask appTask = mcsClient->appTask;
        GattMcsClientTerminateCfm *message = CsrPmemAlloc(sizeof(*message));
        GattMcsClient *mcsClientMain = mcsClientGetMainInstance();

        CsrBtGattUnregisterReqSend(mcsClient->srvcElem->gattId);

        memcpy(&(message->deviceData), &(mcsClient->handles), sizeof(GattMcsClientDeviceData));

        message->srvcHndl = mcsClient->srvcElem->service_handle;

        /* Remove the service element from main list */
        if (mcsClientMain)
            MCS_REMOVE_SERVICE_HANDLE(mcsClientMain->serviceHandleList, mcsClient->srvcElem);

        /* Free the service instance memory */
        res = ServiceHandleFreeInstanceData(clntHndl);

        if (res)
        {
            message->status = GATT_MCS_CLIENT_STATUS_SUCCESS;
        }
        else
        {
            message->status = GATT_MCS_CLIENT_STATUS_FAILED;
        }
        McsMessageSend(appTask, GATT_MCS_CLIENT_TERMINATE_CFM, message);
    }
    else
    {
        GATT_MCS_CLIENT_ERROR("Invalid MCS memory instance!\n");
    }
}
