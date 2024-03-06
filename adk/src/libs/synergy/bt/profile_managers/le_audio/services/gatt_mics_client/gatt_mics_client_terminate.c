/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#include "gatt_mics_client.h"
#include "gatt_mics_client_private.h"
#include "gatt_mics_client_debug.h"
#include "gatt_mics_client_common_util.h"

extern gatt_mics_client *mics_client_main;
/****************************************************************************/
void GattMicsClientTerminateReq(ServiceHandle clntHndl)
{
    GMICSC *mics_client = ServiceHandleGetInstanceData(clntHndl);

    if (mics_client)
    {
        /* Unregister with the GATT Manager and verify the result */
        bool res = FALSE;
        AppTask app_task = mics_client->app_task;
        MAKE_MICS_CLIENT_MESSAGE(GattMicsClientTerminateCfm);

        CsrBtGattUnregisterReqSend(mics_client->srvcElem->gattId);

        CsrMemCpy(&(message->deviceData), &(mics_client->handles), sizeof(GattMicsClientDeviceData));

        message->srvcHndl = mics_client->srvcElem->service_handle;

        res = ServiceHandleFreeInstanceData(clntHndl);

        if (res)
        {
            message->status = GATT_MICS_CLIENT_STATUS_SUCCESS;
            MICS_REMOVE_SERVICE_HANDLE(mics_client_main->service_handle_list, clntHndl);
        }
        else
        {
            message->status = GATT_MICS_CLIENT_STATUS_FAILED;
        }

        MicsMessageSend(app_task, GATT_MICS_CLIENT_TERMINATE_CFM, message);
    }
    else
    {
        GATT_MICS_CLIENT_PANIC("Invalid MICS memory instance!\n");
    }
}
