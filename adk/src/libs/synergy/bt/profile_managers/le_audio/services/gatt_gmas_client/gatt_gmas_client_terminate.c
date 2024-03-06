/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/


#include "gatt_gmas_client_common_util.h"
#include "gatt_gmas_client_debug.h"

/****************************************************************************/
void GattGmasClientTerminateReq(ServiceHandle clntHndl)
{
    GGMASC *gmasClient = ServiceHandleGetInstanceData(clntHndl);

    if (gmasClient)
    {
            bool res = FALSE;
            AppTask appTask = gmasClient->appTask;
            GattGmasClientTerminateCfm *message = CsrPmemZalloc(sizeof(*message));
            GattGmasClient *gmasClientMain = gattGmasClientGetMainInstance();

            CsrBtGattUnregisterReqSend(gmasClient->srvcElem->gattId);

            memcpy(&(message->deviceData), &(gmasClient->handles), sizeof(GattGmasClientDeviceData));

            message->srvcHndl = gmasClient->srvcElem->service_handle;

            /* Remove the serevice element from main list */
            if (gmasClientMain)
                GATT_GMAS_CLIENT_REMOVE_SERVICE_HANDLE(gmasClientMain->serviceHandleList, gmasClient->srvcElem);

            /* Free the service instance memory */
            res = ServiceHandleFreeInstanceData(clntHndl);

            if (res)
            {
                message->status = GATT_GMAS_CLIENT_STATUS_SUCCESS;
            }
            else
            {
                message->status = GATT_GMAS_CLIENT_STATUS_FAILED;
            }
            GattGmasClientMessageSend(appTask, GATT_GMAS_CLIENT_TERMINATE_CFM, message);
        }
    else
    {
        GATT_GMAS_CLIENT_ERROR("Invalid GMAS memory instance!\n");
    }
}
