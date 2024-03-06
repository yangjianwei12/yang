/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_tds_client_common_util.h"
#include "gatt_tds_client_debug.h"
#include "service_handle.h"

/****************************************************************************/
void GattTdsClientTerminateReq(ServiceHandle clntHndl)
{
    GTDSC *tdsClient = ServiceHandleGetInstanceData(clntHndl);

    if (tdsClient)
    {
            bool res = FALSE;
            AppTask appTask = tdsClient->appTask;
            GattTdsClientTerminateCfm *message = CsrPmemAlloc(sizeof(*message));
            GattTdsClient *tdsClientMain = tdsClientGetMainInstance();

            CsrBtGattUnregisterReqSend(tdsClient->srvcElem->gattId);

            memcpy(&(message->deviceData), &(tdsClient->handles), sizeof(GattTdsClientDeviceData));

            message->srvcHndl = tdsClient->srvcElem->service_handle;

            /* Remove the service element from main list */
            if (tdsClientMain)
                TDS_REMOVE_SERVICE_HANDLE(tdsClientMain->serviceHandleList, tdsClient->srvcElem);

            /* Free the service instance memory */
            res = ServiceHandleFreeInstanceData(clntHndl);

            if (res)
            {
                message->status = GATT_TDS_CLIENT_STATUS_SUCCESS;
            }
            else
            {
                message->status = GATT_TDS_CLIENT_STATUS_FAILED;
            }

            TdsMessageSend(appTask, GATT_TDS_CLIENT_TERMINATE_CFM, message);
        }
    else
    {
        GATT_TDS_CLIENT_ERROR("Invalid TDS memory instance!\n");
    }
}
