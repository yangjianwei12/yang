/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_manager.h>

#include "gatt_vocs_client.h"
#include "gatt_vocs_client_debug.h"
#include "gatt_vocs_client_private.h"
#include "gatt_vocs_client_common.h"
#include "gatt_vocs_client_read.h"

/***************************************************************************/
void vocsClientHandleInternalRead(const GVOCS * vocs_client, uint16 handle)
{
    GattManagerReadCharacteristicValue((Task)&vocs_client->lib_task, handle);
}

/***************************************************************************/
static void vocsClientHandleReadRequest(const GVOCS *gatt_vocs_client,
                                        uint16 handle,
                                        bool isReadCcc)
{
    if (isReadCcc)
    {
        MAKE_VOCS_CLIENT_INTERNAL_MESSAGE(VOCS_CLIENT_INTERNAL_MSG_READ_CCC);

        message->handle = handle;

        MessageSendConditionally((Task)&gatt_vocs_client->lib_task,
                                 VOCS_CLIENT_INTERNAL_MSG_READ_CCC,
                                 message,
                                 &gatt_vocs_client->pending_cmd);
    }
    else
    {
        MAKE_VOCS_CLIENT_INTERNAL_MESSAGE(VOCS_CLIENT_INTERNAL_MSG_READ);

        message->handle = handle;

        MessageSendConditionally((Task)&gatt_vocs_client->lib_task,
                                 VOCS_CLIENT_INTERNAL_MSG_READ,
                                 message,
                                 &gatt_vocs_client->pending_cmd);
    }
}

/****************************************************************************/
static void vocsClientOptReadCccReq(GVOCS * gatt_vocs_client,
                                    uint16 handle,
                                    uint8 properties,
                                    GattVocsClientMessageId id)
{
    if (properties & VOCS_CLIENT_NOTIFY_PROP)
    {
        /* Notify property supported */
        vocsClientHandleReadRequest(gatt_vocs_client,
                                    handle,
                                    TRUE);
    }
    else
    {
        /* Notify property not supported */
        vocsSendReadCccCfm(gatt_vocs_client,
                           gatt_status_request_not_supported,
                           1,
                           0,
                           id);
    }
}

/****************************************************************************/
void GattVocsClientReadOffsetStateCccRequest(ServiceHandle clntHndl)
{
    GVOCS *gatt_vocs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vocs_client)
    {
        vocsClientHandleReadRequest(gatt_vocs_client,
                                    gatt_vocs_client->handles.offsetStateCccHandle,
                                    TRUE);
    }
    else
    {
        GATT_VOCS_CLIENT_DEBUG_PANIC(("Invalid VOCS Client instance!\n"));
    }
}

/****************************************************************************/
void GattVocsClientReadAudioLocationCccRequest(ServiceHandle clntHndl)
{
    GVOCS *gatt_vocs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vocs_client)
    {
        vocsClientOptReadCccReq(gatt_vocs_client,
                                gatt_vocs_client->handles.audioLocationCccHandle,
                                gatt_vocs_client->handles.audioLocationProperties,
                                GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CCC_CFM);
    }
    else
    {
        GATT_VOCS_CLIENT_DEBUG_PANIC(("Invalid VOCS Client instance!\n"));
    }
}

/****************************************************************************/
void GattVocsClientReadAudiOutputDescCccRequest(ServiceHandle clntHndl)
{
    GVOCS *gatt_vocs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vocs_client)
    {
        vocsClientOptReadCccReq(gatt_vocs_client,
                                gatt_vocs_client->handles.audioOutputDescriptionCccHandle,
                                gatt_vocs_client->handles.audioOutputDescProperties,
                                GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CCC_CFM);
    }
    else
    {
        GATT_VOCS_CLIENT_DEBUG_PANIC(("Invalid VOCS Client instance!\n"));
    }
}

/*******************************************************************************/
void vocsSendReadOffsetStateCfm(GVOCS *vocs_client,
                                gatt_status_t status,
                                const uint8 *value)
{
    MAKE_VOCS_CLIENT_MESSAGE(GattVocsClientReadOffsetStateCfm);

    message->srvcHdnl = vocs_client->srvc_hndl;
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

    MessageSend(vocs_client->app_task, GATT_VOCS_CLIENT_READ_OFFSET_STATE_CFM, message);
}

/*******************************************************************************/
void vocsSendReadAudioLocationCfm(GVOCS *vocs_client,
                                  gatt_status_t status,
                                  const uint8 *value)
{
    MAKE_VOCS_CLIENT_MESSAGE(GattVocsClientReadAudioLocationCfm);

    message->srvcHdnl = vocs_client->srvc_hndl;
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

    MessageSend(vocs_client->app_task, GATT_VOCS_CLIENT_READ_AUDIO_LOCATION_CFM, message);
}

/*******************************************************************************/
void vocsSendReadAudioOutputDescCfm(GVOCS *vocs_client,
                                    gatt_status_t status,
                                    uint16 size_value,
                                    const uint8 *value)
{
    MAKE_VOCS_CLIENT_MESSAGE_WITH_LEN(GattVocsClientReadAudioOutputDescCfm,
                                      size_value);

    message->srvcHdnl = vocs_client->srvc_hndl;
    message->status = status;
    message->sizeValue = size_value;
    memcpy(message->audioOuputDesc, value, size_value);

    MessageSend(vocs_client->app_task, GATT_VOCS_CLIENT_READ_AUDIO_OUTPUT_DESC_CFM, message);
}

/****************************************************************************/
void GattVocsClientReadOffsetStateRequest(ServiceHandle clntHndl)

{
    GVOCS *gatt_vocs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vocs_client)
    {
        vocsClientHandleReadRequest(gatt_vocs_client,
                                    gatt_vocs_client->handles.offsetStateHandle,
                                    FALSE);
    }
    else
    {
        GATT_VOCS_CLIENT_DEBUG_PANIC(("Invalid VOCS Client instance!\n"));
    }
}

/*******************************************************************************/
void GattVocsClientReadAudioLocationRequest(ServiceHandle clntHndl)
{
    GVOCS *gatt_vocs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vocs_client)
    {
        vocsClientHandleReadRequest(gatt_vocs_client,
                                    gatt_vocs_client->handles.audioLocationHandle,
                                    FALSE);
    }
    else
    {
        GATT_VOCS_CLIENT_DEBUG_PANIC(("Invalid VOCS Client instance!\n"));
    }
}

/*******************************************************************************/
void GattVocsClientReadAudioOutputDescRequest(ServiceHandle clntHndl)
{
    GVOCS *gatt_vocs_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_vocs_client)
    {
        vocsClientHandleReadRequest(gatt_vocs_client,
                                    gatt_vocs_client->handles.audioOutputDescriptionHandle,
                                    FALSE);
    }
    else
    {
        GATT_VOCS_CLIENT_DEBUG_PANIC(("Invalid VOCS Client instance!\n"));
    }
}
