/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include <string.h>
#include <stdio.h>

#include "gatt_telephone_bearer_client_read.h"

#include "gatt_telephone_bearer_client_discover.h"
#include "gatt_telephone_bearer_client_notification.h"
#include "gatt_telephone_bearer_client_write.h"



/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_PROVIDER_NAME_CFM message to the application task.
 */
static void makeTbsReadBearerProviderNameCfmMsg(GTBSC *tbs_client,
                                                    GattTelephoneBearerClientStatus status,
                                                    uint16 size,
                                                    char* providerName)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientReadProviderNameCfm, size);
    message->tbsHandle = tbs_client->srvcElem->service_handle;
    message->status = status;
    memmove(message->providerName, providerName, size);
    message->providerNameSize = size;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_PROVIDER_NAME_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_BEARER_UCI_CFM message to the application task.
 */
static void makeTbsReadBearerUciCfmMsg(GTBSC *tbs_client,
                                          GattTelephoneBearerClientStatus status,
                                          uint16 size,
                                          char* bearer_uci)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientReadBearerUciCfm,size);
    memset(message, 0, sizeof(GattTelephoneBearerClientReadBearerUciCfm));
    message->tbsHandle = tbs_client->srvcElem->service_handle;
    message->status = status;
    memmove(message->bearerUci, bearer_uci, size);
    message->bearerUciSize = size;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_BEARER_UCI_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_BEARER_TECHNOLOGY_CFM message to the application task.
 */
static void makeTbsReadBearerTechnologyCfmMsg(GTBSC *tbs_client,
                                                  GattTelephoneBearerClientStatus status,
                                                  uint16 size,
                                                  char* bearer_tech)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientReadBearerTechnologyCfm, size);
    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;

    memmove(message->bearerTech, bearer_tech, size);
    message->bearerTechSize = size;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_BEARER_TECHNOLOGY_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM message to the application task.
 */
static void makeTbsReadBearerUriSchemesCfmMsg(GTBSC *tbs_client,
                                                 GattTelephoneBearerClientStatus status,
                                                 uint16 size,
                                                 char* uri_schemes)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientReadBearerUriSchemesSupportedListCfm, size);
    memset(message, 0, sizeof(GattTelephoneBearerClientReadBearerUriSchemesSupportedListCfm));
    message->tbsHandle = tbs_client->srvcElem->service_handle;
    message->status = status;

    memmove(message->uriList, uri_schemes, size);
    message->uriListSize = size;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM, message);
}

/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_SIGNAL_STRENGTH_CFM message to the application task.
 */
static void makeTbsReadSignalStrengthCfmMsg(GTBSC *tbs_client,
                                                GattTelephoneBearerClientStatus status,
                                                uint8 signal_strength)
{
    MAKE_TBSC_MESSAGE(GattTelephoneBearerClientReadSignalStrengthCfm);
    message->tbsHandle = tbs_client->srvcElem->service_handle;
    message->status = status;
    message->signalStrength = signal_strength;
    
    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_SIGNAL_STRENGTH_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_SINGAL_STRENGTH_INTERVAL_CFM message to the application task.
 */
static void makeTbsReadSignalStrengthIntervalCfmMsg(GTBSC *tbs_client,
                                                         GattTelephoneBearerClientStatus status,
                                                         uint8 signal_strength_interval)
{
    MAKE_TBSC_MESSAGE(GattTelephoneBearerClientReadSignalStrengthIntervalCfm);
    message->tbsHandle = tbs_client->srvcElem->service_handle;
    message->status = status;
    message->interval = signal_strength_interval;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_SIGNAL_STRENGTH_INTERVAL_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_CURRENT_CALLS_LIST_CFM message to the application task.
 */
static void makeTbsReadCurrentCallsListCfmMsg(GTBSC *tbs_client,
                                                  GattTelephoneBearerClientStatus status,
                                                  uint16 size,
                                                  uint8* current_calls)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientReadCurrentCallsListCfm, size);
    memset(message, 0, sizeof(GattTelephoneBearerClientReadCurrentCallsListCfm));
    message->tbsHandle = tbs_client->srvcElem->service_handle;
    message->status = status;

    memmove(message->currentCallsList, current_calls, size);
    message->currentCallsListSize = size;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_CURRENT_CALLS_LIST_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_CONTENT_CONTROL_ID_CFM message to the application task.
 */
static void makeTbsReadContentControlIdCfmMsg(GTBSC *tbs_client,
                                                 GattTelephoneBearerClientStatus status,
                                                 uint16 ccid)
{
    MAKE_TBSC_MESSAGE(GattTelephoneBearerClientReadContentControlIdCfm);
    message->tbsHandle = tbs_client->srvcElem->service_handle;
    message->status = status;
    message->contentControlId = ccid;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_CONTENT_CONTROL_ID_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_FEATURE_AND_STATUS_FLAGS_CFM message to the application task.
 */
static void makeTbsReadFlagsCfmMsg(GTBSC *tbs_client,
                                      GattTelephoneBearerClientStatus status,
                                      uint16 flags)
{
    MAKE_TBSC_MESSAGE(GattTelephoneBearerClientReadFeatureAndStatusFlagsCfm);
    message->tbsHandle = tbs_client->srvcElem->service_handle;
    message->status = status;
    message->flags = (GattTbsStatusFlags)flags;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_FEATURE_AND_STATUS_FLAGS_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM message to the application task.
 */
static void makeTbsReadIncomingCallTargetBearerUriCfmMsg(GTBSC *tbs_client,
                                                               GattTelephoneBearerClientStatus status,
                                                               uint16 size,
                                                               char* uri)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientReadIncomingCallTargetBearerUriCfm, size);
    memset(message, 0, sizeof(GattTelephoneBearerClientReadIncomingCallTargetBearerUriCfm));
    message->tbsHandle = tbs_client->srvcElem->service_handle;
    message->status = status;

    if(size>=2)
    {
        uint8 uriSize = size-1;

        /* first octet is the call id */
        message->callId = uri[0];

        /* further octets are the uri */
        memmove(message->uri, uri+1, uriSize);
        message->uriSize= uriSize;
    }

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_MSG_READ_CALL_STATE_CFM message to the application task.
 */
static void makeTbsReadCallStateCfmMsg(GTBSC *tbs_client,
                                          GattTelephoneBearerClientStatus status,
                                          uint16 size,
                                          uint8* call_state)
{
    uint8 stateSize = sizeof(TbsCallState);
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientMsgReadCallStateCfm,size);
    memset(message, 0, sizeof(GattTelephoneBearerClientMsgReadCallStateCfm));
    message->tbsHandle = tbs_client->srvcElem->service_handle;
    message->status = status;

    /* check if the size is correct */
    if (size % stateSize == 0)
    {
        memmove(message->callStateList, call_state, size);
        message->callStateListSize = size / stateSize;
    }
    else
    {
        message->status = GATT_TELEPHONE_BEARER_CLIENT_STATUS_READ_ERR;
    }

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_CALL_STATE_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_MSG_READ_INCOMING_CALL_CFM message to the application task.
 */
static void makeTbsReadIncomingCallCfmMsg(GTBSC *tbs_client,
                                              GattTelephoneBearerClientStatus status,
                                              uint16 size,
                                              uint8* incoming_call)
{

    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientMsgReadIncomingCallCfm,size);
    memset(message, 0, sizeof(GattTelephoneBearerClientMsgReadIncomingCallCfm));
    message->tbsHandle = tbs_client->srvcElem->service_handle;
    message->status = status;

    if(size>=1)
    {
        /* first octet is the call id */
        message->callId = incoming_call[0];
        if(size >= 2)
        {
            /* further octets are the uri */
            uint8 uriSize = size-1;
            memmove(message->callUri, incoming_call+1, uriSize);
            message->callUriSize = uriSize;
        }
    }

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_INCOMING_CALL_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_MSG_READ_CALL_FRIENDLY_NAME_CFM message to the application task.
 */
static void makeTbsReadCallFriendlyNameCfmMsg(GTBSC *tbs_client,
                                                           GattTelephoneBearerClientStatus status,
                                                           uint16 size,
                                                           uint8* friendly_name)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GattTelephoneBearerClientMsgReadCallFriendlyNameCfm,size);
    memset(message, 0, sizeof(GattTelephoneBearerClientMsgReadCallFriendlyNameCfm));
    message->tbsHandle = tbs_client->srvcElem->service_handle;
    message->status = status;

    if(size>=2)
    {
        uint8 nameSize = size-1;

        /* first octet is the call id */
        message->callId = friendly_name[0];

        /* further octets are the uri */
        memmove(message->friendlyName, friendly_name+1, nameSize);
        message->friendlyNameSize = nameSize;
    }

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_CALL_FRIENDLY_NAME_CFM, message);
}

/*******************************************************************************
 * Send a GATT_TELEPHONE_BEARER_CLIENT_READ_CCP_OPTIONAL_OPCODES_CFM message to the application task.
 */
static void makeTbsReadCcpOptionalOpcodesCfmMsg(GTBSC *tbs_client,
                                                         GattTelephoneBearerClientStatus status,
                                                         uint16 opcodes)
{
    MAKE_TBSC_MESSAGE(GattTelephoneBearerClientReadOptionalOpcodesCfm);
    memset(message, 0, sizeof(GattTelephoneBearerClientReadOptionalOpcodesCfm));
    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;
    message->opcodes = opcodes;

    TbsClientMessageSend(tbs_client->appTask, GATT_TELEPHONE_BEARER_CLIENT_READ_CCP_OPTIONAL_OPCODES_CFM, message);
}


/****************************************************************************
Internal functions
****************************************************************************/

/***************************************************************************/
void tbsReadBearerProviderNameRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read Bearer Provider Name - handle 0x%x\n",
                          tbs_client->handles.bearerNameHandle);

    if (tbs_client->handles.bearerNameHandle)
    {
        /* Read bearer provider name value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.bearerNameHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadBearerProviderNameCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadBearerUciRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read Bearer Uci - handle 0x%x\n",
                          tbs_client->handles.bearerUciHandle);

    if (tbs_client->handles.bearerUciHandle)
    {
        /* Read Bearer UCI value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.bearerUciHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadBearerUciCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadBearerTechnologyRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read Bearer Tech - handle 0x%x\n",
                          tbs_client->handles.bearerTechHandle);

    if (tbs_client->handles.bearerTechHandle)
    {
        /* Read bearer technology value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.bearerTechHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadBearerTechnologyCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadBearerUriSchemesRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read Bearer URI Prefixes - handle 0x%x\n",
                          tbs_client->handles.bearerUriPrefixListHandle);

    if (tbs_client->handles.bearerUriPrefixListHandle)
    {
        /* Read Bearer URI Schemes value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.bearerUriPrefixListHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadBearerUriSchemesCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadSignalStrengthRequest(GTBSC *tbs_client)
{    
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read Signal Strength - handle 0x%x\n",
                          tbs_client->handles.signalStrengthHandle);

    if (tbs_client->handles.signalStrengthHandle)
    {
        /* Read signal strength value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.signalStrengthHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadSignalStrengthCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0);
    }
}

/***************************************************************************/
void tbsReadSignalStrengthIntervalRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read Signal Strength Interval - handle 0x%x\n",
                          tbs_client->handles.signalStrengthIntervalHandle);

    if (tbs_client->handles.signalStrengthIntervalHandle)
    {
        /* Read signal strength value interval direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.signalStrengthIntervalHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadSignalStrengthIntervalCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0);
    }
}

/***************************************************************************/
void tbsReadCurrentCallsListRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read Current Calls List - handle 0x%x\n",
                          tbs_client->handles.listCurrentCallsCccHandle);

    if (tbs_client->handles.listCurrentCallsHandle)
    {
        /* Read current calls list value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.listCurrentCallsHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadCurrentCallsListCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadContentControlIdRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read Content Control ID - handle 0x%x\n",
                          tbs_client->handles.contentControlIdHandle);

    if (tbs_client->handles.contentControlIdHandle)
    {
        /* Read content control id value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.contentControlIdHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadContentControlIdCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0);
    }
}

/***************************************************************************/
void tbsReadFlagsRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read Flags - handle 0x%x\n",
                          tbs_client->handles.statusFlagsHandle);

    if (tbs_client->handles.statusFlagsHandle)
    {
        /* Read Flags value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.statusFlagsHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadFlagsCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0);
    }
}

/***************************************************************************/
void tbsReadIncomingCallTargetBearerUriRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read Incoming Call Target Bearer URI - handle 0x%x\n",
                          tbs_client->handles.incomingTargetBearerUriHandle);

    if (tbs_client->handles.incomingTargetBearerUriHandle)
    {
        /* Read Incoming Call Target Bearer Uri value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.incomingTargetBearerUriHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadIncomingCallTargetBearerUriCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadCallStateRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read Call State - handle 0x%x\n",
                          tbs_client->handles.callStateHandle);

    if (tbs_client->handles.callStateHandle)
    {
        /* Read Call State value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.callStateHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadCallStateCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadIncomingCallRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read Incoming Call - handle 0x%x\n",
                          tbs_client->handles.incomingCallHandle);

    if (tbs_client->handles.incomingCallHandle)
    {
        /* Read Incoming Call value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.incomingCallHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadIncomingCallCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadCallFriendlyNameRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read Call Friendly Name - handle 0x%x\n",
                          tbs_client->handles.remoteFriendlyNameHandle);

    if (tbs_client->handles.remoteFriendlyNameHandle)
    {
        /* Read Call Friendly Name value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.remoteFriendlyNameHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadCallFriendlyNameCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadContentControlPointOptionalOpcodes(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG("GTBSC: Read CCp Optional Opcodes - handle 0x%x\n",
                          tbs_client->handles.callControlPointOptionalOpcodesHandle);

    if (tbs_client->handles.callControlPointOptionalOpcodesHandle)
    {
        /* Read content control id value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->handles.callControlPointOptionalOpcodesHandle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadCcpOptionalOpcodesCfmMsg(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED, 0);
    }
}

/****************************************************************************/
void handleTbsReadCharacteristicValueResp(GTBSC *tbs_client, const CsrBtGattReadCfm *read_cfm)
{
    GattTelephoneBearerClientStatus result;

    CsrBtGattReadCfm * msg = (CsrBtGattReadCfm *)read_cfm;

    /* Check Characteristic read length */

    /* if this is a Ccc handle length must be 2 */
    if((msg->valueLength != 2) &&
       (msg->handle == tbs_client->handles.bearerNameCccHandle ||
        msg->handle == tbs_client->handles.bearerTechCccHandle ||
        msg->handle == tbs_client->handles.signalStrengthCccHandle ||
        msg->handle == tbs_client->handles.listCurrentCallsCccHandle ||
        msg->handle == tbs_client->handles.statusFlagsCccHandle ||
        msg->handle == tbs_client->handles.callStateCccHandle ||
        msg->handle == tbs_client->handles.callControlPointCccHandle ||
        msg->handle == tbs_client->handles.terminationReasonCccHandle ||
        msg->handle == tbs_client->handles.incomingCallCccHandle ||
        msg->handle == tbs_client->handles.remoteFriendlyNameCccHandle))
    {
        GATT_TBS_CLIENT_ERROR("Read Operation Failure, Invalid Message Length Ccc Handle:0x%04x Size:0x%04x\n", msg->handle, msg->valueLength);
        return;
    }

    /* These chracteristic lengths must be at least 2 */
    if ((msg->valueLength < 2) &&
        (msg->handle == tbs_client->handles.statusFlagsHandle ||
         msg->handle == tbs_client->handles.incomingTargetBearerUriHandle ||
         msg->handle == tbs_client->handles.callControlPointOptionalOpcodesHandle ||
         msg->handle == tbs_client->handles.incomingCallHandle))
    {
        GATT_TBS_CLIENT_ERROR("Read Operation Failure, Invalid Message Length Handle:0x%04x Size:0x%04x\n", msg->handle, msg->valueLength);
        result = GATT_TELEPHONE_BEARER_CLIENT_STATUS_READ_ERR;
    }
    /* These chracteristic lengths must be at least 1 */
    else if ((msg->valueLength < 1) &&
             (msg->handle == tbs_client->handles.signalStrengthHandle ||
              msg->handle == tbs_client->handles.contentControlIdHandle ||
              msg->handle == tbs_client->handles.signalStrengthIntervalHandle))
    {
        GATT_TBS_CLIENT_ERROR("Read Operation Failure, Invalid Message Length Handle:0x%04x Size:0x%04x\n", msg->handle, msg->valueLength);
        result = GATT_TELEPHONE_BEARER_CLIENT_STATUS_READ_ERR;
    }
    /* All other characteristics maybe 0 length */
    else
    {
        result = (msg->resultCode == CSR_BT_GATT_RESULT_SUCCESS)? GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS : GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED;
    }

    if (msg->handle == tbs_client->handles.bearerNameHandle)
    {
        makeTbsReadBearerProviderNameCfmMsg(tbs_client, result, msg->valueLength, (char*)msg->value);
    }
    else if (msg->handle == tbs_client->handles.bearerNameCccHandle)
    {
        tbs_client->bearer_name_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
    }
    else if (msg->handle == tbs_client->handles.bearerUciHandle)
    {
        makeTbsReadBearerUciCfmMsg(tbs_client, result, msg->valueLength, (char*)msg->value);
    }
    else if (msg->handle == tbs_client->handles.bearerTechHandle)
    {
        makeTbsReadBearerTechnologyCfmMsg(tbs_client, result, msg->valueLength, (char*)msg->value);
    }
    else if (msg->handle == tbs_client->handles.bearerTechCccHandle)
    {
        tbs_client->bearer_tech_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
    }
    else if (msg->handle == tbs_client->handles.bearerUriPrefixListHandle)
    {
        makeTbsReadBearerUriSchemesCfmMsg(tbs_client, result, msg->valueLength, (char*)msg->value);
    }
    else if (msg->handle == tbs_client->handles.signalStrengthHandle)
    {
         uint8 signalStength = 0;
         if(result == GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS)
         {
             signalStength = msg->value[0];
         }
         makeTbsReadSignalStrengthCfmMsg(tbs_client, result, signalStength);
    }
    else if (msg->handle == tbs_client->handles.signalStrengthCccHandle)
    {
        tbs_client->signal_strength_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
    }
    else if (msg->handle == tbs_client->handles.signalStrengthIntervalHandle)
    {
        uint8 interval = 0;
        if(result == GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS)
        {
            interval = msg->value[0];
        }
        makeTbsReadSignalStrengthIntervalCfmMsg(tbs_client, result, interval);
    }
    else if (msg->handle == tbs_client->handles.listCurrentCallsHandle)
    {
        makeTbsReadCurrentCallsListCfmMsg(tbs_client, result, msg->valueLength, (uint8*)msg->value);
    }
    else if (msg->handle == tbs_client->handles.listCurrentCallsCccHandle)
    {
        tbs_client->list_current_calls_ccc_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
    }
    else if (msg->handle == tbs_client->handles.contentControlIdHandle)
    {
        uint16 content_control_id = 0;
        if(result == GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS)
        {
            content_control_id = ((uint16) msg->value[0]);
        }
        makeTbsReadContentControlIdCfmMsg(tbs_client, result, content_control_id);
    }
    else if (msg->handle == tbs_client->handles.statusFlagsHandle)
    {
        uint16 supported_features = 0;
        if(result == GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS)
        {
            supported_features = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        }
        makeTbsReadFlagsCfmMsg(tbs_client, result, supported_features);
    }
    else if (msg->handle == tbs_client->handles.statusFlagsCccHandle)
    {
        tbs_client->supported_features_ccc_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
    }
    else if (msg->handle == tbs_client->handles.incomingTargetBearerUriHandle)
    {
        makeTbsReadIncomingCallTargetBearerUriCfmMsg(tbs_client, result,  msg->valueLength, (char*)msg->value);
    }
    else if (msg->handle == tbs_client->handles.incomingTargetBearerUriCccHandle)
    {
        tbs_client->incoming_target_bearer_uri_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
    }
    else if (msg->handle == tbs_client->handles.callStateHandle)
    {
        makeTbsReadCallStateCfmMsg(tbs_client, result, msg->valueLength, (uint8*)msg->value);
    }
    else if (msg->handle == tbs_client->handles.callStateCccHandle)
    {
        tbs_client->call_state_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
    }
    else if (msg->handle == tbs_client->handles.callControlPointCccHandle)
    {
        tbs_client->call_control_point_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
    }
    else if (msg->handle == tbs_client->handles.terminationReasonHandle)
    {
        GATT_TBS_CLIENT_DEBUG("GATT_READ_CHARACTERISTIC_VALUE_CFM Termination Reason: Success! Handle: 0x%04x\n", msg->handle);
        /* Characteristic is notify only */
    }
    else if (msg->handle == tbs_client->handles.terminationReasonCccHandle)
    {
        tbs_client->termination_reason_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
    }
    else if (msg->handle == tbs_client->handles.incomingCallHandle)
    {
        makeTbsReadIncomingCallCfmMsg(tbs_client, result, msg->valueLength, (uint8*)msg->value);
    }
    else if (msg->handle == tbs_client->handles.incomingCallCccHandle)
    {
        tbs_client->incoming_call_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
    }
    else if (msg->handle == tbs_client->handles.remoteFriendlyNameHandle)
    {
        makeTbsReadCallFriendlyNameCfmMsg(tbs_client, result, msg->valueLength, (uint8*)msg->value);
    }
    else if (msg->handle == tbs_client->handles.remoteFriendlyNameCccHandle)
    {
        tbs_client->incoming_remote_friendly_name_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
    }
    else if (msg->handle == tbs_client->handles.callControlPointOptionalOpcodesHandle)
    {
        uint16 opcodes = 0;
        if(result == GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS)
        {
            opcodes = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        }
        makeTbsReadCcpOptionalOpcodesCfmMsg(tbs_client, result, opcodes);
    }
}


/****************************************************************************/
void readCharacteristicValue(GTBSC *tbs_client, uint16 handle)
{   
    CsrBtGattReadReqSend(tbs_client->srvcElem->gattId,
                         tbs_client->srvcElem->cid,
                         handle,
                         0);
}

/****************************************************************************/
void readCharacteristicDescriptorValue(GTBSC *tbs_client, uint16 handle)
{
    CsrBtGattReadReqSend(tbs_client->srvcElem->gattId,
                         tbs_client->srvcElem->cid,
                         handle,
                         0);
}


/****************************************************************************
Public API
****************************************************************************/

/****************************************************************************/

void GattTelephoneBearerClientReadProviderNameRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read Provider Name Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_PROVIDER_NAME, message);
}

void GattTelephoneBearerClientReadBearerUciRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read Bearer Uci Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_UCI, message);
}

void GattTelephoneBearerClientReadBearerTechnologyRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read Bearer Tech Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_TECHNOLOGY, message);
}

void GattTelephoneBearerClientReadBearerUriRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read Bearer URI Schemes Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST, message);
}

void GattTelephoneBearerClientReadSignalStrengthRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read Signal Strength Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_SIGNAL_STRENGTH, message);
}

void GattTelephoneBearerClientReadSignalStrengthIntervalRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read Signal Strength Interval Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_SIGNAL_STRENGTH_INTERVAL, message);
}

void GattTelephoneBearerClientReadCurrentCallsRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read Current Calls Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_CURRENT_CALLS_LIST, message);
}

void GattTelephoneBearerClientReadContentControlIdRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read Content Control ID Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_CONTENT_CONTROL_ID, message);
}

void GattTelephoneBearerClientReadStatusAndFeatureFlagsRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read Flags Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_FEATURE_AND_STATUS_FLAGS, message);
}

void GattTelephoneBearerClientReadIncomingTargetBearerUriRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read Target Bearer URI Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_INCOMING_CALL_TARGET_BEARER_URI, message);
}

void GattTelephoneBearerClientReadCallStateRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read Call State Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_CALL_STATE, message);
}

void GattTelephoneBearerClientReadIncomingCallRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read Incoming Call Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_INCOMING_CALL, message);
}

void GattTelephoneBearerClientReadCallFriendlyNameRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read Call Friendly Name Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_CALL_FRIENDLY_NAME, message);
}


void GattTelephoneBearerClientReadCallControlPointOptionalOpcodesRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    MAKE_TBSC_MESSAGE(TelephoneBearerInternalGenericRead);

    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_ERROR("GTBSC: Invalid parameters - Read CCP Optional Opcodes Request\n");
        CsrPmemFree(message);
        return;
    }
    message->srvcHndl = tbsHandle;
    TbsClientMessageSend((AppTask)tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_CCP_OPTIONAL_OPCODES, message);
}

/****************************************************************************/
