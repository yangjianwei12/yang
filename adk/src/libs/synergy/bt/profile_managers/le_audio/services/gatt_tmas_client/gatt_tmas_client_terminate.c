/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #58 $
******************************************************************************/


#include "gatt_tmas_client_common_util.h"
#include "gatt_tmas_client_debug.h"

/****************************************************************************/
void GattTmasClientTerminateReq(ServiceHandle clntHndl)
{
    GTMASC *tmasClient = ServiceHandleGetInstanceData(clntHndl);

    if (tmasClient)
    {
            bool res = FALSE;
            AppTask appTask = tmasClient->appTask;
            GattTmasClientTerminateCfm *message = CsrPmemZalloc(sizeof(*message));
            GattTmasClient *tmasClientMain = gattTmasClientGetMainInstance();

            CsrBtGattUnregisterReqSend(tmasClient->srvcElem->gattId);

            memcpy(&(message->deviceData), &(tmasClient->handles), sizeof(GattTmasClientDeviceData));

            message->srvcHndl = tmasClient->srvcElem->service_handle;

            /* Remove the serevice element from main list */
            if (tmasClientMain)
                GATT_TMAS_CLIENT_REMOVE_SERVICE_HANDLE(tmasClientMain->serviceHandleList, tmasClient->srvcElem);

            /* Free the service instance memory */
            res = ServiceHandleFreeInstanceData(clntHndl);

            if (res)
            {
                message->status = GATT_TMAS_CLIENT_STATUS_SUCCESS;
            }
            else
            {
                message->status = GATT_TMAS_CLIENT_STATUS_FAILED;
            }
            GattTmasClientMessageSend(appTask, GATT_TMAS_CLIENT_TERMINATE_CFM, message);
        }
    else
    {
        GATT_TMAS_CLIENT_ERROR("Invalid TMAS memory instance!\n");
    }
}
