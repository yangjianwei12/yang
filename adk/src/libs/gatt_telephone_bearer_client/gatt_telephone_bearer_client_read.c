/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include <gatt.h>

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
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_READ_PROVIDER_NAME_CFM, size);
    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;
    memmove(message->providerName, providerName, size);
    message->providerNameSize = size;

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_PROVIDER_NAME_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_BEARER_UCI_CFM message to the application task.
 */
static void makeTbsReadBearerUciCfmMsg(GTBSC *tbs_client,
                                          GattTelephoneBearerClientStatus status,
                                          uint8 size,
                                          char* bearer_uci)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_READ_BEARER_UCI_CFM, size);

    memset(message, 0, sizeof(GATT_TBS_CLIENT_READ_BEARER_UCI_CFM_T));

    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;
    memmove(message->bearerUci, bearer_uci, size);
    message->bearerUciSize = size;

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_BEARER_UCI_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_BEARER_TECHNOLOGY_CFM message to the application task.
 */
static void makeTbsReadBearerTechnologyCfmMsg(GTBSC *tbs_client,
                                                  GattTelephoneBearerClientStatus status,
                                                  uint8 size,
                                                  char* bearer_tech)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_READ_BEARER_TECHNOLOGY_CFM, size);
    memset(message, 0, sizeof(GATT_TBS_CLIENT_READ_BEARER_TECHNOLOGY_CFM_T));
    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;

    memmove(message->bearerTech, bearer_tech, size);
    message->bearerTechSize = size;

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_BEARER_TECHNOLOGY_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM message to the application task.
 */
static void makeTbsReadBearerUriSchemesCfmMsg(GTBSC *tbs_client,
                                                 GattTelephoneBearerClientStatus status,
                                                 uint8 size,
                                                 char* uri_schemes)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM, size);
    memset(message, 0, sizeof(GATT_TBS_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM_T));
    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;

    memmove(message->uriList, uri_schemes, size);
    message->uriListSize = size;

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM, message);
}

/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_SIGNAL_STRENGTH_CFM message to the application task.
 */
static void makeTbsReadSignalStrengthCfmMsg(GTBSC *tbs_client,
                                                GattTelephoneBearerClientStatus status,
                                                uint8 signal_strength)
{
    MAKE_TBSC_MESSAGE(GATT_TBS_CLIENT_READ_SIGNAL_STRENGTH_CFM);
    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;
    
    if (status == GATT_TBS_CLIENT_STATUS_SUCCESS)
    {
        message->signalStrength = signal_strength;
    }
    else
    {
        message->signalStrength = 0;
    }
    
    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_SIGNAL_STRENGTH_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_SINGAL_STRENGTH_INTERVAL_CFM message to the application task.
 */
static void makeTbsReadSignalStrengthIntervalCfmMsg(GTBSC *tbs_client,
                                                         GattTelephoneBearerClientStatus status,
                                                         uint8 signal_strength_interval)
{
    MAKE_TBSC_MESSAGE(GATT_TBS_CLIENT_READ_SINGAL_STRENGTH_INTERVAL_CFM);
    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;

    if (status == GATT_TBS_CLIENT_STATUS_SUCCESS)
    {
        message->interval = signal_strength_interval;
    }
    else
    {
        message->interval = 0;
    }

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_SIGNAL_STRENGTH_INTERVAL_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_CURRENT_CALLS_LIST_CFM message to the application task.
 */
static void makeTbsReadCurrentCallsListCfmMsg(GTBSC *tbs_client,
                                                  GattTelephoneBearerClientStatus status,
                                                  uint8 size,
                                                  uint8* current_calls)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_READ_CURRENT_CALLS_LIST_CFM, size);
    memset(message, 0, sizeof(GATT_TBS_CLIENT_READ_CURRENT_CALLS_LIST_CFM_T));
    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;

    memmove(message->currentCallsList, current_calls, size);
    message->currentCallsListSize = size;

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_CURRENT_CALLS_LIST_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_CONTENT_CONTROL_ID_CFM message to the application task.
 */
static void makeTbsReadContentControlIdCfmMsg(GTBSC *tbs_client,
                                                 GattTelephoneBearerClientStatus status,
                                                 uint16 ccid)
{
    MAKE_TBSC_MESSAGE(GATT_TBS_CLIENT_READ_CONTENT_CONTROL_ID_CFM);
    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;

    if (status == GATT_TBS_CLIENT_STATUS_SUCCESS)
    {
        message->contentControlId = ccid;
    }
    else
    {
        message->contentControlId = 0;
    }

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_CONTENT_CONTROL_ID_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_FEATURE_AND_STATUS_FLAGS_CFM message to the application task.
 */
static void makeTbsReadFlagsCfmMsg(GTBSC *tbs_client,
                                      GattTelephoneBearerClientStatus status,
                                      uint16 flags)
{
    MAKE_TBSC_MESSAGE(GATT_TBS_CLIENT_READ_FEATURE_AND_STATUS_FLAGS_CFM);
    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;

    if (status == GATT_TBS_CLIENT_STATUS_SUCCESS)
    {
        message->flags = flags;
    }
    else
    {
        message->flags = 0;
    }

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_FEATURE_AND_STATUS_FLAGS_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM message to the application task.
 */
static void makeTbsReadIncomingCallTargetBearerUriCfmMsg(GTBSC *tbs_client,
                                                               GattTelephoneBearerClientStatus status,
                                                               uint8 size,
                                                               char* uri)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM, size);
    memset(message, 0, sizeof(GATT_TBS_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM_T));
    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;

    memmove(message->uri, uri, size);
    message->uriSize = size;

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_MSG_READ_CALL_STATE_CFM message to the application task.
 */
static void makeTbsReadCallStateCfmMsg(GTBSC *tbs_client,
                                          GattTelephoneBearerClientStatus status,
                                          uint8 size,
                                          uint8* call_state)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_MSG_READ_CALL_STATE_CFM,size);
    memset(message, 0, sizeof(GATT_TBS_CLIENT_MSG_READ_CALL_STATE_CFM_T));
    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;

    memmove(message->callStateList, call_state, size);
    message->callStateListSize = size;

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_CALL_STATE_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_MSG_READ_INCOMING_CALL_CFM message to the application task.
 */
static void makeTbsReadIncomingCallCfmMsg(GTBSC *tbs_client,
                                              GattTelephoneBearerClientStatus status,
                                              uint8 size,
                                              uint8* incoming_call)
{

    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_MSG_READ_INCOMING_CALL_CFM,size);
    memset(message, 0, sizeof(GATT_TBS_CLIENT_MSG_READ_INCOMING_CALL_CFM_T));
    message->tbsHandle = tbs_client->srvcHandle;
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

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_INCOMING_CALL_CFM, message);
}


/*******************************************************************************
 * Send a GATT_TBS_CLIENT_MSG_READ_CALL_FRIENDLY_NAME_CFM message to the application task.
 */
static void makeTbsReadCallFriendlyNameCfmMsg(GTBSC *tbs_client,
                                                           GattTelephoneBearerClientStatus status,
                                                           uint8 size,
                                                           uint8* friendly_name)
{
    MAKE_TBSC_MESSAGE_WITH_LEN(GATT_TBS_CLIENT_MSG_READ_CALL_FRIENDLY_NAME_CFM,size);
    memset(message, 0, sizeof(GATT_TBS_CLIENT_MSG_READ_CALL_FRIENDLY_NAME_CFM_T));
    message->tbsHandle = tbs_client->srvcHandle;
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

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_CALL_FRIENDLY_NAME_CFM, message);
}

/*******************************************************************************
 * Send a GATT_TBS_CLIENT_READ_SINGAL_STRENGTH_INTERVAL_CFM message to the application task.
 */
static void makeTbsReadCcpOptionalOpcodesCfmMsg(GTBSC *tbs_client,
                                                         GattTelephoneBearerClientStatus status,
                                                         uint8 opcodes)
{
    MAKE_TBSC_MESSAGE(GATT_TBS_CLIENT_READ_CCP_OPTIONAL_OPCODES_CFM);
    message->tbsHandle = tbs_client->srvcHandle;
    message->status = status;

    if (status == GATT_TBS_CLIENT_STATUS_SUCCESS)
    {
        message->opcodes = opcodes;
    }
    else
    {
        message->opcodes = 0;
    }

    MessageSend(tbs_client->appTask, GATT_TBS_CLIENT_READ_CCP_OPTIONAL_OPCODES_CFM, message);
}


/****************************************************************************
Internal functions
****************************************************************************/

/***************************************************************************/
void tbsReadBearerProviderNameRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read Bearer Provider Name - handle 0x%x\n",
                               tbs_client->bearer_name_handle));

    if (tbs_client->bearer_name_handle)
    {
        /* Read bearer provider name value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->bearer_name_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadBearerProviderNameCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadBearerUciRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read Bearer Uci - handle 0x%x\n",
                               tbs_client->bearer_uci_handle));

    if (tbs_client->bearer_uci_handle)
    {
        /* Read Bearer UCI value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->bearer_uci_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadBearerUciCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadBearerTechnologyRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read Bearer Tech - handle 0x%x\n",
                               tbs_client->bearer_tech_handle));

    if (tbs_client->bearer_tech_handle)
    {
        /* Read bearer technology value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->bearer_tech_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadBearerTechnologyCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadBearerUriSchemesRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read Bearer URI Prefixes - handle 0x%x\n",
                               tbs_client->bearer_uri_prefix_list_handle));

    if (tbs_client->bearer_uri_prefix_list_handle)
    {
        /* Read Bearer URI Schemes value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->bearer_uri_prefix_list_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadBearerUriSchemesCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadSignalStrengthRequest(GTBSC *tbs_client)
{    
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read Signal Strength - handle 0x%x\n",
                               tbs_client->signal_strength_handle));

    if (tbs_client->signal_strength_handle)
    {
        /* Read signal strength value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->signal_strength_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadSignalStrengthCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0);
    }
}

/***************************************************************************/
void tbsReadSignalStrengthIntervalRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read Signal Strength Interval - handle 0x%x\n",
                               tbs_client->signal_strength_interval_handle));

    if (tbs_client->signal_strength_interval_handle)
    {
        /* Read signal strength value interval direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->signal_strength_interval_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadSignalStrengthIntervalCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0);
    }
}

/***************************************************************************/
void tbsReadCurrentCallsListRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read Current Calls List - handle 0x%x\n",
                               tbs_client->list_current_calls_handle));

    if (tbs_client->list_current_calls_handle)
    {
        /* Read current calls list value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->list_current_calls_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadCurrentCallsListCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadContentControlIdRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read Content Control ID - handle 0x%x\n",
                               tbs_client->content_control_id_handle));

    if (tbs_client->content_control_id_handle)
    {
        /* Read content control id value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->content_control_id_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadContentControlIdCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0);
    }
}

/***************************************************************************/
void tbsReadFlagsRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read Flags - handle 0x%x\n",
                               tbs_client->status_flags_handle));

    if (tbs_client->status_flags_handle)
    {
        /* Read Flags value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->status_flags_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadFlagsCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0);
    }
}

/***************************************************************************/
void tbsReadIncomingCallTargetBearerUriRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read Incoming Call Target Bearer URI - handle 0x%x\n",
                               tbs_client->incoming_target_bearer_uri_handle));

    if (tbs_client->incoming_target_bearer_uri_handle)
    {
        /* Read Incoming Call Target Bearer Uri value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->incoming_target_bearer_uri_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadIncomingCallTargetBearerUriCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadCallStateRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read Call State - handle 0x%x\n",
                               tbs_client->call_state_handle));

    if (tbs_client->call_state_handle)
    {
        /* Read Call State value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->call_state_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadCallStateCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadIncomingCallRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read Incoming Call - handle 0x%x\n",
                               tbs_client->incoming_call_handle));

    if (tbs_client->incoming_call_handle)
    {
        /* Read Incoming Call value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->incoming_call_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadIncomingCallCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadCallFriendlyNameRequest(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read Call Friendly Name - handle 0x%x\n",
                               tbs_client->remote_friendly_name_handle));

    if (tbs_client->remote_friendly_name_handle)
    {
        /* Read Call Friendly Name value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->remote_friendly_name_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadCallFriendlyNameCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0, NULL);
    }
}

/***************************************************************************/
void tbsReadContentControlPointOptionalOpcodes(GTBSC *tbs_client)
{
    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Read CCp Optional Opcodes - handle 0x%x\n",
                               tbs_client->call_control_point_optional_opcodes_handle));

    if (tbs_client->call_control_point_optional_opcodes_handle)
    {
        /* Read content control id value direct, as handle is known */
        readCharacteristicValue(tbs_client, tbs_client->call_control_point_optional_opcodes_handle);
    }
    else
    {
        /* Send error message to application */
        makeTbsReadCcpOptionalOpcodesCfmMsg(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED, 0);
    }
}

/****************************************************************************/
void handleTbsReadCharacteristicValueResp(GTBSC *tbs_client, const GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM_T *read_cfm)
{
    GattTelephoneBearerClientStatus result;

    GATT_READ_CHARACTERISTIC_VALUE_CFM_T * msg = (GATT_READ_CHARACTERISTIC_VALUE_CFM_T *)read_cfm;

    result = (msg->status == gatt_status_success)? GATT_TBS_CLIENT_STATUS_SUCCESS : GATT_TBS_CLIENT_STATUS_FAILED;

    if (msg->handle == tbs_client->bearer_name_handle)
    {
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Bearer name: Success! Handle:0x%04x Size:0x%04x Value:0x%02x 0x%02x 0x%02x\n",
                  msg->handle, msg->size_value, msg->value[0], msg->value[1], msg->value[2]));

        makeTbsReadBearerProviderNameCfmMsg(tbs_client, result, msg->size_value, (char*)&msg->value[0]);
    }
    else if (msg->handle == tbs_client->bearer_name_ccc_handle)
    {
        tbs_client->bearer_name_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Bearer Name CCC: Success! Handle:0x%04x Value:0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
    }
    else if (msg->handle == tbs_client->bearer_uci_handle)
    {
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Bearer UCI: Success! Handle: 0x%04x Value:%x\n", msg->handle, msg->value[0]));
        makeTbsReadBearerUciCfmMsg(tbs_client, result, msg->size_value, (char*)&msg->value[0]);
    }
    else if (msg->handle == tbs_client->bearer_tech_handle)
    {
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Bearer Technology: Success! Handle: 0x%04x Value: 0x%04x\n", msg->handle, msg->value[0]));
        makeTbsReadBearerTechnologyCfmMsg(tbs_client, result, msg->size_value, (char*)&msg->value[0]);
    }
    else if (msg->handle == tbs_client->bearer_tech_ccc_handle)
    {
        tbs_client->bearer_tech_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Bearer Tech CCC: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
    }
    else if (msg->handle == tbs_client->bearer_uri_prefix_list_handle)
    {
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Bearer Technology: Success! Handle: 0x%04x Value: 0x%04x\n", msg->handle, msg->value[0]));
        makeTbsReadBearerUriSchemesCfmMsg(tbs_client, result, msg->size_value, (char*)&msg->value[0]);
    }
    else if (msg->handle == tbs_client->signal_strength_handle)
    {
        uint8 signalStength = msg->value[0];
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Signal Strength: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
        makeTbsReadSignalStrengthCfmMsg(tbs_client, result, signalStength);
    }
    else if (msg->handle == tbs_client->signal_strength_ccc_handle)
    {
        tbs_client->signal_strength_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Signal Strength CCC: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
    }
    else if (msg->handle == tbs_client->signal_strength_interval_handle)
    {
        uint8 interval = msg->value[0];
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Signal Strength Interval: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
        makeTbsReadSignalStrengthIntervalCfmMsg(tbs_client, result, interval);
    }
    else if (msg->handle == tbs_client->list_current_calls_handle)
    {
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM List Current Calls: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
        makeTbsReadCurrentCallsListCfmMsg(tbs_client, result, msg->size_value, (uint8*)&msg->value[0]);
    }
    else if (msg->handle == tbs_client->list_current_calls_ccc_handle)
    {
        tbs_client->list_current_calls_ccc_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM List Current Calls CCC: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
    }
    else if (msg->handle == tbs_client->content_control_id_handle)
    {
        uint16 content_control_id = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Bearer Tech CCC: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
        makeTbsReadContentControlIdCfmMsg(tbs_client, result, content_control_id);
    }
    else if (msg->handle == tbs_client->status_flags_handle)
    {
        uint16 supported_features = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Supported Features: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
        makeTbsReadFlagsCfmMsg(tbs_client, result, supported_features);
    }
    else if (msg->handle == tbs_client->status_flags_ccc_handle)
    {
        tbs_client->supported_features_ccc_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Supported Features CCC: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
    }
    else if (msg->handle == tbs_client->incoming_target_bearer_uri_handle)
    {

        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Target Bearer URI: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
        makeTbsReadIncomingCallTargetBearerUriCfmMsg(tbs_client, result,  msg->size_value, (char*)&msg->value[0]);
    }
    else if (msg->handle == tbs_client->incoming_target_bearer_uri_ccc_handle)
    {
        tbs_client->incoming_target_bearer_uri_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Target Bearer URI CCC: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
    }
    else if (msg->handle == tbs_client->call_state_handle)
    {
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Call State: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
        makeTbsReadCallStateCfmMsg(tbs_client, result, msg->size_value, (uint8*)&msg->value[0]);
    }
    else if (msg->handle == tbs_client->call_state_ccc_handle)
    {
        tbs_client->call_state_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Call State CCC: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
    }
    else if (msg->handle == tbs_client->call_control_point_ccc_handle)
    {
        tbs_client->call_control_point_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Call Control Point CCC: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
    }
    else if (msg->handle == tbs_client->termination_reason_handle)
    {
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Termination Reason: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
        /* Characteristic is notify only */
    }
    else if (msg->handle == tbs_client->termination_reason_ccc_handle)
    {
        tbs_client->termination_reason_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Termination Reason CCC: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
    }
    else if (msg->handle == tbs_client->incoming_call_handle)
    {
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Incoming call handle: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
        makeTbsReadIncomingCallCfmMsg(tbs_client, result, msg->size_value, (uint8*)&msg->value[0]);
    }
    else if (msg->handle == tbs_client->incoming_call_ccc_handle)
    {
        tbs_client->incoming_call_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM Incoming call handle CCC: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
    }
    else if (msg->handle == tbs_client->remote_friendly_name_handle)
    {
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM remote friendly name: Success! Handle: 0x%04x Value: 0x%04x\n", msg->handle, msg->value[0]));
        makeTbsReadCallFriendlyNameCfmMsg(tbs_client, result, msg->size_value, (uint8*)&msg->value[0]);
    }
    else if (msg->handle == tbs_client->remote_friendly_name_ccc_handle)
    {
        tbs_client->incoming_remote_friendly_name_client_cfg = ((uint16) msg->value[0]) | (((uint16) msg->value[1]) << 8);
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM remote friendly name CCC: Success! Handle: 0x%04x Value: 0x%04x 0x%04x\n", msg->handle, msg->value[0], msg->value[1]));
    }
    else if (msg->handle == tbs_client->call_control_point_optional_opcodes_handle)
    {
        uint8 opcodes = msg->value[0];
        GATT_TBS_CLIENT_DEBUG_INFO(("GATT_READ_CHARACTERISTIC_VALUE_CFM CCP optional opcodes: Success! Handle: 0x%04x Value: 0x%04x \n", msg->handle, opcodes));
        makeTbsReadCcpOptionalOpcodesCfmMsg(tbs_client, result, opcodes);
    }
}


/****************************************************************************/
void readCharacteristicValue(GTBSC *tbs_client, uint16 handle)
{   
    GattManagerReadCharacteristicValue((Task)&tbs_client->lib_task, handle);
}

/****************************************************************************/
void readCharacteristicDescriptorValue(GTBSC *tbs_client, uint16 handle)
{
    GattManagerReadCharacteristicValue((Task)&tbs_client->lib_task, handle);
}


/****************************************************************************
Public API
****************************************************************************/

/****************************************************************************/

void GattTelephoneBearerClientReadProviderNameRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read Provider Name Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_PROVIDER_NAME, NULL);
}

void GattTelephoneBearerClientReadBearerUciRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read Bearer Uci Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_UCI, NULL);
}

void GattTelephoneBearerClientReadBearerTechnologyRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read Bearer Tech Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_TECHNOLOGY, NULL);
}

void GattTelephoneBearerClientReadBearerUriRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read Bearer URI Schemes Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST, NULL);
}

void GattTelephoneBearerClientReadSignalStrengthRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read Signal Strength Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_SIGNAL_STRENGTH, NULL);
}

void GattTelephoneBearerClientReadSignalStrengthIntervalRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read Signal Strength Interval Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_SIGNAL_STRENGTH_INTERVAL, NULL);
}

void GattTelephoneBearerClientReadCurrentCallsRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read Current Calls Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_CURRENT_CALLS_LIST, NULL);
}

void GattTelephoneBearerClientReadContentControlIdRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read Content Control ID Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_CONTENT_CONTROL_ID, NULL);
}

void GattTelephoneBearerClientReadStatusAndFeatureFlagsRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read Flags Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_FEATURE_AND_STATUS_FLAGS, NULL);
}

void GattTelephoneBearerClientReadIncomingTargetBearerUriRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read Target Bearer URI Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_INCOMING_CALL_TARGET_BEARER_URI, NULL);
}

void GattTelephoneBearerClientReadCallStateRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read Call State Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_CALL_STATE, NULL);
}

void GattTelephoneBearerClientReadIncomingCallRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read Incoming Call Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_INCOMING_CALL, NULL);
}

void GattTelephoneBearerClientReadCallFriendlyNameRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read Call Friendly Name Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_CALL_FRIENDLY_NAME, NULL);
}


void GattTelephoneBearerClientReadCallControlPointOptionalOpcodesRequest(const ServiceHandle tbsHandle)
{
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Invalid parameters - Read CCP Optional Opcodes Request\n"));
        return;
    }
    MessageSend((Task)&tbs_client->lib_task, TELEPHONE_BEARER_INTERNAL_MSG_READ_CCP_OPTIONAL_OPCODES, NULL);
}

/****************************************************************************/
