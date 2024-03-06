/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <string.h>

#include "csr_bt_gatt_lib.h"

#include "gatt_vocs_client.h"
#include "gatt_vocs_client_private.h"
#include "gatt_vocs_client_debug.h"

static void vocsClientSendTerminateCfm(GVOCS *vocs_client, GattVocsClientStatus status)
{
    MAKE_VOCS_CLIENT_MESSAGE(GattVocsClientTerminateCfm);

    message->srvcHdnl = vocs_client->srvcElem->service_handle;
    message->status = status;
    memcpy(&(message->deviceData), &(vocs_client->handles), sizeof(GattVocsClientDeviceData));

    VocsMessageSend(vocs_client->app_task, GATT_VOCS_CLIENT_TERMINATE_CFM, message);
}

/****************************************************************************/
void GattVocsClientTerminateReq(ServiceHandle clntHndl)
{
    GVOCS *vocs_client = ServiceHandleGetInstanceData(clntHndl);
    /* Check parameters */

    if (vocs_client)
    {
        /* Unregister with the GATT */
        CsrBtGattUnregisterReqSend(vocs_client->srvcElem->gattId);

        vocsClientSendTerminateCfm(vocs_client,
                                   GATT_VOCS_CLIENT_STATUS_SUCCESS);

        ServiceHandleFreeInstanceData(clntHndl);
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Null instance\n");
    }
}

