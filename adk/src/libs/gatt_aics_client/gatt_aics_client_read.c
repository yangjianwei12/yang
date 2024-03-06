/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_manager.h>

#include "gatt_aics_client.h"
#include "gatt_aics_client_debug.h"
#include "gatt_aics_client_private.h"
#include "gatt_aics_client_common.h"
#include "gatt_aics_client_read.h"

/***************************************************************************/
static void aicsClientHandleReadRequest(const GAICS *gatt_aics_client,
                                        uint16 handle,
                                        bool isReadCcc)
{
    if (isReadCcc)
    {
        MAKE_AICS_CLIENT_INTERNAL_MESSAGE(AICS_CLIENT_INTERNAL_MSG_READ_CCC);

        message->handle = handle;

        MessageSendConditionally((Task)&gatt_aics_client->lib_task,
                                 AICS_CLIENT_INTERNAL_MSG_READ_CCC,
                                 message,
                                 &gatt_aics_client->pending_cmd);
    }
    else
    {
        MAKE_AICS_CLIENT_INTERNAL_MESSAGE(AICS_CLIENT_INTERNAL_MSG_READ);

        message->handle = handle;

        MessageSendConditionally((Task)&gatt_aics_client->lib_task,
                                 AICS_CLIENT_INTERNAL_MSG_READ,
                                 message,
                                 &gatt_aics_client->pending_cmd);
    }
}

/***************************************************************************/
void GattAicsClientReadInputStateCccRequest(ServiceHandle clntHndl)
{
    GAICS *gatt_aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_aics_client)
    {
        aicsClientHandleReadRequest(gatt_aics_client,
                                    gatt_aics_client->handles.inputStateCccHandle,
                                    TRUE);
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Invalid AICS Client instance!\n"));
    }
}

/****************************************************************************/
static void aicsClientOptReadCccReq(GAICS * gatt_aics_client,
                                    uint16 handle,
                                    uint8 properties,
                                    GattAicsClientMessageId id)
{
    if (properties & AICS_CLIENT_NOTIFY_PROP)
    {
        /* Notify property supported */
        aicsClientHandleReadRequest(gatt_aics_client,
                                    handle,
                                    TRUE);
    }
    else
    {
        /* Notify property not supported */
        aicsSendReadCccCfm(gatt_aics_client,
                           gatt_status_request_not_supported,
                           1,
                           0,
                           id);
    }
}

/***************************************************************************/
void GattAicsClientReadInputStatusCccRequest(ServiceHandle clntHndl)
{
    GAICS *gatt_aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_aics_client)
    {
        aicsClientHandleReadRequest(gatt_aics_client,
                                    gatt_aics_client->handles.inputStatusCccHandle,
                                    TRUE);
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Invalid AICS Client instance!\n"));
    }
}

/****************************************************************************/
void GattAicsClientReadAudiInputDescCccRequest(ServiceHandle clntHndl)
{
    GAICS *gatt_aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_aics_client)
    {
        aicsClientOptReadCccReq(gatt_aics_client,
                                gatt_aics_client->handles.audioInputDescriptionCccHandle,
                                gatt_aics_client->handles.audioInputDescProperties,
                                GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CCC_CFM);
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Invalid AICS Client instance!\n"));
    }
}

/*******************************************************************************/
void aicsSendReadInputStateCfm(GAICS *aics_client,
                               gatt_status_t status,
                               const uint8 *value)
{
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientReadInputStateCfm);

    message->srvcHndl = aics_client->srvc_hndl;
    message->status = status;

    if (value)
    {
        message->gainSetting = (int8) value[0];
        message->mute = (GattAicsClientMute) value[1];
        message->gainMode = (GattAicsClientGainMode) value[2];
        message->changeCounter = value[3];
    }
    else
    {
        message->gainSetting = 0;
        message->mute = 0;
        message->gainMode = 0;
        message->changeCounter = 0;
    }

    MessageSend(aics_client->app_task, GATT_AICS_CLIENT_READ_INPUT_STATE_CFM, message);
}

/****************************************************************************/
void GattAicsClientReadInputStateRequest(ServiceHandle clntHndl)
{
    GAICS *gatt_aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_aics_client)
    {
        aicsClientHandleReadRequest(gatt_aics_client,
                                    gatt_aics_client->handles.inputStateHandle,
                                    FALSE);
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Invalid AICS Client instance!\n"));
    }
}

/*******************************************************************************/
void aicsSendReadGainSetPropertiesCfm(GAICS *aics_client,
                                      gatt_status_t status,
                                      const uint8 *value)
{
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientReadGainSetPropertiesCfm);

    message->srvcHndl = aics_client->srvc_hndl;
    message->status = status;

    if (value)
    {
        message->gainSettingUnits = value[0];
        message->gainSettingMin = (int8) value[1];
        message->gainSettingMax = (int8) value[2];
    }
    else
    {
        message->gainSettingUnits = 0;
        message->gainSettingMin = 0;
        message->gainSettingMax = 0;
    }

    MessageSend(aics_client->app_task, GATT_AICS_CLIENT_READ_GAIN_SET_PROPERTIES_CFM, message);
}

/*******************************************************************************/
void GattAicsClientReadGainSetPropertiesReq(ServiceHandle clntHndl)
{
    GAICS *gatt_aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_aics_client)
    {
        aicsClientHandleReadRequest(gatt_aics_client,
                                    gatt_aics_client->handles.gainSettingPropertiesHandle,
                                    FALSE);
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Invalid AICS Client instance!\n"));
    }
}

/*******************************************************************************/
void GattAicsClientReadInputTypeReq(ServiceHandle clntHndl)
{
    GAICS *gatt_aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_aics_client)
    {
        aicsClientHandleReadRequest(gatt_aics_client,
                                    gatt_aics_client->handles.inputTypeHandle,
                                    FALSE);
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Invalid AICS Client instance!\n"));
    }
}

/*******************************************************************************/
void aicsSendReadInputTypeCfm(GAICS *aics_client,
                              gatt_status_t status,
                              const uint8 *value)
{
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientReadInputTypeCfm);

    message->srvcHndl = aics_client->srvc_hndl;
    message->status = status;

    if (value)
    {
        message->inputType = value[0];
    }
    else
    {
        message->inputType = 0;
    }

    MessageSend(aics_client->app_task, GATT_AICS_CLIENT_READ_INPUT_TYPE_CFM, message);
}

/*******************************************************************************/
void GattAicsClientReadInputStatusReq(ServiceHandle clntHndl)
{
    GAICS *gatt_aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_aics_client)
    {
        aicsClientHandleReadRequest(gatt_aics_client,
                                    gatt_aics_client->handles.inputStatusHandle,
                                    FALSE);
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Invalid AICS Client instance!\n"));
    }
}

/*******************************************************************************/
void aicsSendReadInputStatusCfm(GAICS *aics_client,
                                gatt_status_t status,
                                const uint8 *value)
{
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientReadInputStatusCfm);

    message->srvcHndl = aics_client->srvc_hndl;
    message->status = status;

    if (value)
    {
        message->inputStatus = value[0];
    }
    else
    {
        message->inputStatus = 0;
    }

    MessageSend(aics_client->app_task, GATT_AICS_CLIENT_READ_INPUT_STATUS_CFM, message);
}

/*******************************************************************************/
void GattAicsClientReadAudioInputDescRequest(ServiceHandle clntHndl)
{
    GAICS *gatt_aics_client = ServiceHandleGetInstanceData(clntHndl);

    if (gatt_aics_client)
    {
        aicsClientHandleReadRequest(gatt_aics_client,
                                    gatt_aics_client->handles.audioInputDescriptionHandle,
                                    FALSE);
    }
    else
    {
        GATT_AICS_CLIENT_DEBUG_PANIC(("Invalid AICS Client instance!\n"));
    }
}

/*******************************************************************************/
void aicsSendReadAudioInputDescCfm(GAICS *aics_client,
                                   gatt_status_t status,
                                   uint16 size_value,
                                   const uint8 *value)
{
    MAKE_AICS_CLIENT_MESSAGE_WITH_LEN(GattAicsClientReadAudioInputDescCfm,
                                               size_value);

    message->srvcHndl = aics_client->srvc_hndl;
    message->status = status;
    message->sizeValue = size_value;
    memcpy(message->audioInputDesc, value, size_value);

    MessageSend(aics_client->app_task, GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CFM, message);
}
