/* Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "csr_bt_gatt_lib.h"

#include "gatt_aics_client.h"
#include "gatt_aics_client_debug.h"
#include "gatt_aics_client_private.h"
#include "gatt_aics_client_common.h"
#include "gatt_aics_client_read.h"

/***************************************************************************/
void aicsClientHandleInternalRead(const GAICS * aics_client, uint16 handle)
{
    CsrBtGattReadReqSend(aics_client->srvcElem->gattId,
                         aics_client->srvcElem->cid,
                         handle,
                         0);
}

/***************************************************************************/
static void aicsClientHandleReadRequest(const GAICS *client,
                                        uint16 handle,
                                        bool isReadCcc)
{
    if (isReadCcc)
    {
        MAKE_AICS_CLIENT_INTERNAL_MESSAGE(AICS_CLIENT_INTERNAL_MSG_READ_CCC);

        message->srvc_hndl = client->srvcElem->service_handle;
        message->handle = handle;

        AicsMessageSendConditionally(client->lib_task,
                                     AICS_CLIENT_INTERNAL_MSG_READ_CCC,
                                     message,
                                     &client->pending_cmd);
    }
    else
    {
        MAKE_AICS_CLIENT_INTERNAL_MESSAGE(AICS_CLIENT_INTERNAL_MSG_READ);

        message->srvc_hndl = client->srvcElem->service_handle;
        message->handle = handle;

        AicsMessageSendConditionally(client->lib_task,
                                     AICS_CLIENT_INTERNAL_MSG_READ,
                                     message,
                                     &client->pending_cmd);
    }
}

/***************************************************************************/
void GattAicsClientReadInputStateCccRequest(ServiceHandle clntHndl)
{
    GAICS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        aicsClientHandleReadRequest(client,
                                    client->handles.inputStateCccHandle,
                                    TRUE);
    }
    else
    {
        gattAicsClientError();
    }
}

/****************************************************************************/
static void aicsClientOptReadCccReq(GAICS * client,
                                    uint16 handle,
                                    uint8 properties,
                                    GattAicsClientMessageId id)
{
    if (properties & AICS_CLIENT_NOTIFY_PROP)
    {
        /* Notify property supported */
        aicsClientHandleReadRequest(client,
                                    handle,
                                    TRUE);
    }
    else
    {
        /* Notify property not supported */
        aicsSendReadCccCfm(client,
                           CSR_BT_GATT_ACCESS_RES_READ_NOT_PERMITTED,
                           0,
                           NULL,
                           id);
    }
}

/***************************************************************************/
void GattAicsClientReadInputStatusCccRequest(ServiceHandle clntHndl)
{
    GAICS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        aicsClientHandleReadRequest(client,
                                    client->handles.inputStatusCccHandle,
                                    TRUE);
    }
    else
    {
        gattAicsClientError();
    }
}

/****************************************************************************/
void GattAicsClientReadAudiInputDescCccRequest(ServiceHandle clntHndl)
{
    GAICS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        aicsClientOptReadCccReq(client,
                                client->handles.audioInputDescriptionCccHandle,
                                client->handles.audioInputDescProperties,
                                GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CCC_CFM);
    }
    else
    {
        gattAicsClientError();
    }
}

/*******************************************************************************/
void aicsSendReadInputStateCfm(GAICS *aics_client,
                               status_t status,
                               const uint8 *value)
{
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientReadInputStateCfm);

    message->srvcHndl = aics_client->srvcElem->service_handle;
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

    AicsMessageSend(aics_client->app_task, GATT_AICS_CLIENT_READ_INPUT_STATE_CFM, message);
}

/****************************************************************************/
void GattAicsClientReadInputStateRequest(ServiceHandle clntHndl)
{
    GAICS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        aicsClientHandleReadRequest(client,
                                    client->handles.inputStateHandle,
                                    FALSE);
    }
    else
    {
        gattAicsClientError();
    }
}

/*******************************************************************************/
void aicsSendReadGainSetPropertiesCfm(GAICS *aics_client,
                                      status_t status,
                                      const uint8 *value)
{
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientReadGainSetPropertiesCfm);

    message->srvcHndl = aics_client->srvcElem->service_handle;
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

    AicsMessageSend(aics_client->app_task, GATT_AICS_CLIENT_READ_GAIN_SET_PROPERTIES_CFM, message);
}

/*******************************************************************************/
void GattAicsClientReadGainSetPropertiesReq(ServiceHandle clntHndl)
{
    GAICS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        aicsClientHandleReadRequest(client,
                                    client->handles.gainSettingPropertiesHandle,
                                    FALSE);
    }
    else
    {
        gattAicsClientError();
    }
}

/*******************************************************************************/
void GattAicsClientReadInputTypeReq(ServiceHandle clntHndl)
{
    GAICS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        aicsClientHandleReadRequest(client,
                                    client->handles.inputTypeHandle,
                                    FALSE);
    }
    else
    {
        gattAicsClientError();
    }
}

/*******************************************************************************/
void aicsSendReadInputTypeCfm(GAICS *aics_client,
                              status_t status,
                              const uint8 *value)
{
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientReadInputTypeCfm);

    message->srvcHndl = aics_client->srvcElem->service_handle;
    message->status = status;

    if (value)
    {
        message->inputType = value[0];
    }
    else
    {
        message->inputType = 0;
    }

    AicsMessageSend(aics_client->app_task, GATT_AICS_CLIENT_READ_INPUT_TYPE_CFM, message);
}

/*******************************************************************************/
void GattAicsClientReadInputStatusReq(ServiceHandle clntHndl)
{
    GAICS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        aicsClientHandleReadRequest(client,
                                    client->handles.inputStatusHandle,
                                    FALSE);
    }
    else
    {
        gattAicsClientError();
    }
}

/*******************************************************************************/
void aicsSendReadInputStatusCfm(GAICS *aics_client,
                                status_t status,
                                const uint8 *value)
{
    MAKE_AICS_CLIENT_MESSAGE(GattAicsClientReadInputStatusCfm);

    message->srvcHndl = aics_client->srvcElem->service_handle;
    message->status = status;

    if (value)
    {
        message->inputStatus = value[0];
    }
    else
    {
        message->inputStatus = 0;
    }

    AicsMessageSend(aics_client->app_task, GATT_AICS_CLIENT_READ_INPUT_STATUS_CFM, message);
}

/*******************************************************************************/
void GattAicsClientReadAudioInputDescRequest(ServiceHandle clntHndl)
{
    GAICS *client = ServiceHandleGetInstanceData(clntHndl);

    if (client)
    {
        aicsClientHandleReadRequest(client,
                                    client->handles.audioInputDescriptionHandle,
                                    FALSE);
    }
    else
    {
        gattAicsClientError();
    }
}

/*******************************************************************************/
void aicsSendReadAudioInputDescCfm(GAICS *aics_client,
                                   status_t status,
                                   uint16 size_value,
                                   const uint8 *value)
{
    MAKE_AICS_CLIENT_MESSAGE_WITH_LEN(GattAicsClientReadAudioInputDescCfm,
                                      size_value);
    message->audioInputDesc = (uint8 *) CsrPmemZalloc(size_value);

    message->srvcHndl = aics_client->srvcElem->service_handle;
    message->status = status;
    message->sizeValue = size_value;
    memcpy(message->audioInputDesc, value, size_value);

    AicsMessageSend(aics_client->app_task, GATT_AICS_CLIENT_READ_AUDIO_INPUT_DESC_CFM, message);
}
