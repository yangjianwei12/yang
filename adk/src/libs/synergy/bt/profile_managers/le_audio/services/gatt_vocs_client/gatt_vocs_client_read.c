/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "csr_bt_gatt_lib.h"

#include "gatt_vocs_client.h"
#include "gatt_vocs_client_debug.h"
#include "gatt_vocs_client_private.h"
#include "gatt_vocs_client_common.h"
#include "gatt_vocs_client_read.h"

/***************************************************************************/
void vocsClientHandleInternalRead(const GVOCS * vocs_client, uint16 handle)
{
    CsrBtGattReadReqSend(vocs_client->srvcElem->gattId,
                         vocs_client->srvcElem->cid,
                         handle,
                         0);
}

/***************************************************************************/
static void vocsClientHandleReadRequest(const GVOCS *client,
                                        uint16 handle,
                                        bool isReadCcc)
{
    if (isReadCcc)
    {
        MAKE_VOCS_CLIENT_INTERNAL_MESSAGE(VOCS_CLIENT_INTERNAL_MSG_READ_CCC);

        message->srvc_hndl = client->srvcElem->service_handle;
        message->handle = handle;

        VocsMessageSendConditionally(client->lib_task,
                                     VOCS_CLIENT_INTERNAL_MSG_READ_CCC,
                                     message,
                                     &client->pending_cmd);
    }
    else
    {
        MAKE_VOCS_CLIENT_INTERNAL_MESSAGE(VOCS_CLIENT_INTERNAL_MSG_READ);

        message->srvc_hndl = client->srvcElem->service_handle;
        message->handle = handle;

        VocsMessageSendConditionally(client->lib_task,
                                     VOCS_CLIENT_INTERNAL_MSG_READ,
                                     message,
                                     &client->pending_cmd);
    }
}

/****************************************************************************/
static void vocsClientOptReadCccReq(GVOCS * client,
                                    uint16 handle,
                                    uint8 properties,
                                    GattVocsClientMessageId id)
{
    if (properties & VOCS_CLIENT_NOTIFY_PROP)
    {
        /* Notify property supported */
        vocsClientHandleReadRequest(client,
                                    handle,
                                    TRUE);
    }
    else
    {
        /* Notify property not supported */
        vocsSendReadCccCfm(client,
                           CSR_BT_GATT_ACCESS_RES_READ_NOT_PERMITTED,
                           0,
                           NULL,
                           id);
    }
}

/****************************************************************************/
void GattVocsClientReadOffsetStateCccRequest(ServiceHandle clntHndl)
{
    GVOCS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vocsClientHandleReadRequest(client,
                                    client->handles.offsetStateCccHandle,
                                    TRUE);
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Invalid VOCS Client instance!\n");
    }
}

/****************************************************************************/
void GattVocsClientReadAudioLocationCccRequest(ServiceHandle clntHndl)
{
    GVOCS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vocsClientOptReadCccReq(client,
                                client->handles.audioLocationCccHandle,
                                client->handles.audioLocationProperties,
                                GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CCC_CFM);
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Invalid VOCS Client instance!\n");
    }
}

/****************************************************************************/
void GattVocsClientReadAudiOutputDescCccRequest(ServiceHandle clntHndl)
{
    GVOCS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vocsClientOptReadCccReq(client,
                                client->handles.audioOutputDescriptionCccHandle,
                                client->handles.audioOutputDescProperties,
                                GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CCC_CFM);
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Invalid VOCS Client instance!\n");
    }
}

/*******************************************************************************/
void vocsSendReadOffsetStateCfm(GVOCS *vocs_client,
                                status_t status,
                                const uint8 *value)
{
    MAKE_VOCS_CLIENT_MESSAGE(GattVocsClientReadOffsetStateCfm);

    message->srvcHdnl = vocs_client->srvcElem->service_handle;
    message->status = status;

    if (value)
    {
        message->volumeOffset = (int16) (((uint16) value[0]) | (((uint16) value[1]) << 8));
        message->changeCounter = value[2];
    }
    else
    {
        message->volumeOffset = 0;
        message->changeCounter = 0;
    }

    VocsMessageSend(vocs_client->app_task, GATT_VOCS_CLIENT_READ_OFFSET_STATE_CFM, message);
}

/*******************************************************************************/
void vocsSendReadAudioLocationCfm(GVOCS *vocs_client,
                                  status_t status,
                                  const uint8 *value)
{
    MAKE_VOCS_CLIENT_MESSAGE(GattVocsClientReadAudioLocationCfm);

    message->srvcHdnl = vocs_client->srvcElem->service_handle;
    message->status = status;

    if (value)
    {
        message->audioLocation = (GattVocsClientAudioLoc)(((uint32) value[0]) |
                                                          (((uint32) value[1]) << 8) |
                                                          (((uint32) value[2]) << 16)|
                                                          (((uint32) value[3]) << 24));
    }
    else
    {
        message->audioLocation = GATT_VOCS_CLIENT_AUDIO_LOC_INVALID;
    }

    VocsMessageSend(vocs_client->app_task, GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CFM, message);
}

/*******************************************************************************/
void vocsSendReadAudioOutputDescCfm(GVOCS *vocs_client,
                                    status_t status,
                                    uint16 size_value,
                                    const uint8 *value)
{
    MAKE_VOCS_CLIENT_MESSAGE_WITH_LEN(GattVocsClientReadAudioOutputDescCfm,
                                      size_value);

    message->audioOuputDesc = (uint8 *) CsrPmemZalloc(size_value);

    message->srvcHdnl = vocs_client->srvcElem->service_handle;
    message->status = status;
    message->sizeValue = size_value;
    memcpy(message->audioOuputDesc, value, size_value);

    VocsMessageSend(vocs_client->app_task, GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CFM, message);
}

/****************************************************************************/
void GattVocsClientReadOffsetStateRequest(ServiceHandle clntHndl)

{
    GVOCS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vocsClientHandleReadRequest(client,
                                    client->handles.offsetStateHandle,
                                    FALSE);
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Invalid VOCS Client instance!\n");
    }
}

/*******************************************************************************/
void GattVocsClientReadAudioLocationRequest(ServiceHandle clntHndl)
{
    GVOCS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vocsClientHandleReadRequest(client,
                                    client->handles.audioLocationHandle,
                                    FALSE);
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Invalid VOCS Client instance!\n");
    }
}

/*******************************************************************************/
void GattVocsClientReadAudioOutputDescRequest(ServiceHandle clntHndl)
{
    GVOCS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        vocsClientHandleReadRequest(client,
                                    client->handles.audioOutputDescriptionHandle,
                                    FALSE);
    }
    else
    {
        GATT_VOCS_CLIENT_PANIC("Invalid VOCS Client instance!\n");
    }
}
