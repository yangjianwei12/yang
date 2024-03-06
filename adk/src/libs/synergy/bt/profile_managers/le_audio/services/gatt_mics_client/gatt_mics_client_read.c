/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/


#include "gatt_mics_client.h"
#include "gatt_mics_client_debug.h"
#include "gatt_mics_client_private.h"
#include "gatt_mics_client_common.h"
#include "gatt_mics_client_read.h"


/***************************************************************************/
void micsClientHandleInternalRead(const GMICSC * mics_client, uint16 handle)
{
    CsrBtGattReadReqSend(mics_client->srvcElem->gattId,
                         mics_client->srvcElem->cid,
                         handle,
                         0);
}

/***************************************************************************/
static void micsClientHandleReadRequest(const GMICSC *client,
                                       uint16 handle,
                                       bool isReadCcc)
{
    if (isReadCcc)
    {
        MAKE_MICS_CLIENT_INTERNAL_MESSAGE(MICS_CLIENT_INTERNAL_MSG_READ_CCC);

        message->srvc_hndl = client->srvcElem->service_handle;
        message->handle = handle;

        MicsMessageSendConditionally(client->lib_task,
                                    MICS_CLIENT_INTERNAL_MSG_READ_CCC,
                                    message,
                                    &client->pending_cmd);
    }
    else
    {
        MAKE_MICS_CLIENT_INTERNAL_MESSAGE(MICS_CLIENT_INTERNAL_MSG_READ);

        message->srvc_hndl = client->srvcElem->service_handle;
        message->handle = handle;

        MicsMessageSendConditionally(client->lib_task,
                                    MICS_CLIENT_INTERNAL_MSG_READ,
                                    message,
                                    &client->pending_cmd);
    }
}

/****************************************************************************/
void GattMicsClientReadMuteValueCccReq(ServiceHandle clntHndl)
{
    GMICSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        micsClientHandleReadRequest(client,
                                   client->handles.muteCccHandle,
                                   TRUE);
    }
    else
    {
        GATT_MICS_CLIENT_ERROR("Invalid MICS Client instance!\n");
    }
}

/****************************************************************************/
void GattMicsClientReadMuteValueReq(ServiceHandle clntHndl)
{
    GMICSC *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        micsClientHandleReadRequest(client,
                                   client->handles.muteHandle,
                                   FALSE);
    }
    else
    {
        GATT_MICS_CLIENT_ERROR("Invalid MICS Client instance!\n");
    }
}

/*******************************************************************************/
void micsSendReadMuteValueCfm(GMICSC *mics_client,
                               status_t status,
                               const uint8 *value)
{
    MAKE_MICS_CLIENT_MESSAGE(GattMicsClientReadMuteValueCfm);

    message->svcHndl = mics_client->srvcElem->service_handle;
    message->status = status;

    if (value)
    {
        message->muteValue = value[0];

    }
    else
    {
        message->muteValue = 0;
    }

    MicsMessageSend(mics_client->app_task, GATT_MICS_CLIENT_READ_MUTE_VALUE_CFM, message);
}

