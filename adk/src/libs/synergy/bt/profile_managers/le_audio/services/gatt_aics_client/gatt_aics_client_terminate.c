/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "csr_bt_gatt_lib.h"

#include "gatt_aics_client.h"
#include "gatt_aics_client_private.h"
#include "gatt_aics_client_debug.h"

static void aicsClientSendTerminateCfm(GAICS *aics_client, GattAicsClientStatus status)
{
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientTerminateCfm);

    message->srvcHndl = aics_client->srvcElem->service_handle;
    message->status = status;
    memcpy(&(message->deviceData), &(aics_client->handles), sizeof(GattAicsClientDeviceData));

    AicsMessageSend(aics_client->app_task, GATT_AICS_CLIENT_TERMINATE_CFM, message);
}

/****************************************************************************/
void GattAicsClientTerminateReq(ServiceHandle clntHndl)
{
    GAICS *aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (aics_client)
    {
        /* Unregister with the GATT Manager and verify the result */
        CsrBtGattUnregisterReqSend(aics_client->srvcElem->gattId);

        aicsClientSendTerminateCfm(aics_client,
                                   GATT_AICS_CLIENT_STATUS_SUCCESS);

        ServiceHandleFreeInstanceData(clntHndl);
    }
    else
    {
        GATT_AICS_CLIENT_PANIC("Null instance\n");
    }
}

