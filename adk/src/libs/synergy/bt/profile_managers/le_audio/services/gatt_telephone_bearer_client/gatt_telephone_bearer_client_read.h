/******************************************************************************
 Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_TBS_CLIENT_READ_H_
#define GATT_TBS_CLIENT_READ_H_


#include "gatt_telephone_bearer_client_private.h"

/***************************************************************************
NAME
    tbsReadBearerProviderNameRequest

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_PROVIDER_NAME message.
*/
void tbsReadBearerProviderNameRequest(GTBSC *tbs_client);

/***************************************************************************
NAME
    tbsReadBearerUciRequest

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_UCI message.
*/
void tbsReadBearerUciRequest(GTBSC *tbs_client);

/***************************************************************************
NAME
    tbsReadBearerTechnologyRequest

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_TECHNOLOGY message.
*/
void tbsReadBearerTechnologyRequest(GTBSC *tbs_client);

/***************************************************************************
NAME
    tbsReadBearerUriSchemesRequest

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST
    message.
*/
void tbsReadBearerUriSchemesRequest(GTBSC *tbs_client);

/***************************************************************************
NAME
    tbsReadSignalStrengthRequest

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_SIGNAL_STRENGTH message.
*/
void tbsReadSignalStrengthRequest(GTBSC *tbs_client);

/***************************************************************************
NAME
    tbsReadSignalStrengthIntervalRequest

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_SIGNAL_STRENGTH_INTERVAL message.
*/
void tbsReadSignalStrengthIntervalRequest(GTBSC *tbs_client);

/***************************************************************************
NAME
    tbsReadCurrentCallsListRequest

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_CURRENT_CALLS_LIST message.
*/
void tbsReadCurrentCallsListRequest(GTBSC *tbs_client);

/***************************************************************************
NAME
    tbsReadContentControlIdRequest

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_CONTENT_CONTROL_ID message.
*/
void tbsReadContentControlIdRequest(GTBSC *tbs_client);

/***************************************************************************
NAME
    tbsReadFlagsRequest

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_FEATURE_AND_STATUS_FLAGS message.
*/
void tbsReadFlagsRequest(GTBSC *tbs_client);

/***************************************************************************
NAME
    tbsReadIncomingCallTargetBearerUriRequest

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_INCOMING_CALL_TARGET_BEARER_URI message.
*/
void tbsReadIncomingCallTargetBearerUriRequest(GTBSC *tbs_client);

/***************************************************************************
NAME
    tbsReadCallStateRequest

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_CALL_STATE message.
*/
void tbsReadCallStateRequest(GTBSC *tbs_client);

/***************************************************************************
NAME
    tbsReadIncomingCallRequest

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_INCOMING_CALL message.
*/
void tbsReadIncomingCallRequest(GTBSC *tbs_client);

/***************************************************************************
NAME
    tbsReadCallFriendlyNameRequest

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_CALL_FRIENDLY_NAME message.
*/
void tbsReadCallFriendlyNameRequest(GTBSC *tbs_client);

/***************************************************************************
NAME
    tbsReadContentControlPointOptionalOpcodes

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_CCP_OPTIONAL_OPCODES message.
*/
void tbsReadContentControlPointOptionalOpcodes(GTBSC *tbs_client);

/***************************************************************************
NAME
    handleTbsReadCharacteristicValueResp

DESCRIPTION
    Handles the GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM message that was sent to the TBS library.
*/
void handleTbsReadCharacteristicValueResp(GTBSC *tbs_client, const CsrBtGattReadCfm *read_cfm);


/***************************************************************************
NAME
    tbsReadDescriptor

DESCRIPTION
    Handles the internal TELEPHONE_BEARER_INTERNAL_MSG_READ_DESCRIPTOR message.
*/
void tbsReadDescriptor(GTBSC *tbs_client, uint16 descriptor_uuid, bool discover_if_unknown);


/***************************************************************************
NAME
    readCharacteristicValue

DESCRIPTION
    Reads characteristic value using known handle.
*/
void readCharacteristicValue(GTBSC *tbs_client, uint16 handle);

/***************************************************************************
NAME
    readCharacteristicDescriptorValue

DESCRIPTION
    Reads characteristic descriptor value depending on the pending function.
*/
void readCharacteristicDescriptorValue(GTBSC *tbs_client, uint16 handle);


#endif
