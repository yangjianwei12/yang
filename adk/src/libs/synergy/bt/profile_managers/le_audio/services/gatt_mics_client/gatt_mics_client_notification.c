/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#include "gatt_mics_client.h"
#include "gatt_mics_client_debug.h"
#include "gatt_mics_client_private.h"
#include "gatt_mics_client_notification.h"
#include "gatt_mics_client_common.h"

/***************************************************************************/
static void micsClientHandleRegisterForNotification(const GMICSC *client,
                                                   bool enable,
                                                   uint16 handle)
{
    /* Validate the Input Parameters */
    if (client == NULL)
    {
        GATT_MICS_CLIENT_PANIC("Null instance\n");
        return;
    }
    else
    {
        MAKE_MICS_CLIENT_INTERNAL_MESSAGE(MICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ);

        message->srvc_hndl = client->srvcElem->service_handle;
        message->enable = enable;
        message->handle = handle;

        GATT_MICS_CLIENT_INFO("MICS Notifications: enable = %x\n", message->enable);

        MicsMessageSendConditionally(client->lib_task,
                                    MICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,
                                    message,
                                    &(client->pending_cmd));
    }
}

/***************************************************************************/
void micsClientHandleInternalRegisterForNotification(GMICSC *const mics_client,
                                                    bool enable,
                                                    uint16 handle)
{
    uint8* value = (uint8*)(CsrPmemZalloc(MICS_CLIENT_CHARACTERISTIC_CONFIG_SIZE));

    value[0] = enable ? MICS_NOTIFICATION_VALUE : 0;
    value[1] = 0;

    CsrBtGattWriteReqSend(mics_client->srvcElem->gattId,
                          mics_client->srvcElem->cid,
                          handle,
                          0,
                          MICS_CLIENT_CHARACTERISTIC_CONFIG_SIZE,
                          value);
}

/****************************************************************************/
void GattMicsClientRegisterForNotificationReq(ServiceHandle clntHndl, bool notificationsEnable)
{
    GMICSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        micsClientHandleRegisterForNotification(client,
                                               notificationsEnable,
                                               client->handles.muteCccHandle);
    }
    else
    {
        GATT_MICS_CLIENT_PANIC("Invalid MICS Client instance!\n");
    }
}

