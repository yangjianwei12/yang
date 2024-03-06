/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt.h>

#include "gatt_telephone_bearer_client_private.h"
#include "gatt_telephone_bearer_client_msg_handler.h"
#include "gatt_telephone_bearer_client_discover.h"
#include "gatt_telephone_bearer_client_notification.h"
#include "gatt_telephone_bearer_client_read.h"
#include "gatt_telephone_bearer_client_write.h"

/****************************************************************************/
static void handleGattManagerMsg(Task task, MessageId id, Message payload)
{
    GTBSC *tbs_client = (GTBSC *)task;
    
    switch (id)
    {
        case GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND:
        {
            handleTbsNotification(tbs_client->srvcHandle, (const GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND_T *)payload);
        }
        break;

        case GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM:
        {
            handleDiscoverAllTbsCharacteristicsResp(tbs_client, (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)payload);
        }
        break;

        case GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM:
        {
            handleDiscoverAllTbsCharacteristicDescriptorsResp(tbs_client, (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)payload);
        }
        break;

        case GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM:
        {
            handleTbsReadCharacteristicValueResp(tbs_client, (const GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM_T *)payload);
        }
        break;

        case GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM:
        {
            handleTbsWriteCharacteristicValueResp(tbs_client, (const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *)payload);
        }
        break;

        case GATT_MANAGER_WRITE_WITHOUT_RESPONSE_CFM:
        {
            handleTbsWriteWithoutResponseResp(tbs_client, (const GATT_MANAGER_WRITE_WITHOUT_RESPONSE_CFM_T *)payload);
        }
        break;

        default:
        {
            /* Unrecognised GATT Manager message */
            GATT_TBS_CLIENT_DEBUG_PANIC(("TBSC: Client GattMgr Msg not handled [0x%x]\n", id));
        }
        break;
    }
}

/****************************************************************************/
static void handleGattMsg(MessageId id)
{
    UNUSED(id);

    /* Unrecognised GATT message */
    GATT_TBS_CLIENT_DEBUG_PANIC(("TBSC: Client Gatt Msg not handled [0x%x]\n", id));
}

/****************************************************************************/
static void handleInternalTbsMsg(Task task, MessageId id, Message payload)
{
    GTBSC *tbs_client = (GTBSC *)task;
    
    switch (id)
    {
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_PROVIDER_NAME:
            tbsReadBearerProviderNameRequest(tbs_client);
        break;
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_UCI:
            tbsReadBearerUciRequest(tbs_client);
        break;
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_TECHNOLOGY:
            tbsReadBearerTechnologyRequest(tbs_client);
        break;
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST:
            tbsReadBearerUriSchemesRequest(tbs_client);
        break;
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_SIGNAL_STRENGTH:
            tbsReadSignalStrengthRequest(tbs_client);
        break;
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_SIGNAL_STRENGTH_INTERVAL:
            tbsReadSignalStrengthIntervalRequest(tbs_client);
        break;
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_CURRENT_CALLS_LIST:
            tbsReadCurrentCallsListRequest(tbs_client);
        break;
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_CONTENT_CONTROL_ID:
            tbsReadContentControlIdRequest(tbs_client);
        break;
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_FEATURE_AND_STATUS_FLAGS:
            tbsReadFlagsRequest(tbs_client);
        break;
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_INCOMING_CALL_TARGET_BEARER_URI:
            tbsReadIncomingCallTargetBearerUriRequest(tbs_client);
        break;
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_CALL_STATE:
            tbsReadCallStateRequest(tbs_client);
        break;
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_INCOMING_CALL:
            tbsReadIncomingCallRequest(tbs_client);
        break;
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_CALL_FRIENDLY_NAME:
            tbsReadCallFriendlyNameRequest(tbs_client);
        break;
        case TELEPHONE_BEARER_INTERNAL_MSG_READ_CCP_OPTIONAL_OPCODES:
            tbsReadContentControlPointOptionalOpcodes(tbs_client);
        break;

        case TELEPHONE_BEARER_INTERNAL_MSG_SET_NOTIFICATION:
        {     
            tbsHandleInternalRegisterForNotification(tbs_client,
                                                     ((const TELEPHONE_BEARER_INTERNAL_MSG_SET_NOTIFICATION_T *)payload)->notificationsEnable,
                                                     ((const TELEPHONE_BEARER_INTERNAL_MSG_SET_NOTIFICATION_T *)payload)->handle );
        }
        break;

        case TELEPHONE_BEARER_INTERNAL_MSG_WRITE_SIGNAL_STRENGTH_INTERVAL:
        {
            tbsWriteSignalStrengthIntervalRequest(tbs_client,
                                                  ((const TELEPHONE_BEARER_INTERNAL_MSG_WRITE_SIGNAL_STRENGTH_INTERVAL_T *)payload)->interval,
                                                  ((const TELEPHONE_BEARER_INTERNAL_MSG_WRITE_SIGNAL_STRENGTH_INTERVAL_T *)payload)->writeWithoutResponse );

        }
        break;

        case TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT:
        {
            tbsWriteCallControlPointRequest(tbs_client,
                                            ((const TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT_T *)payload)->opcode,
                                            ((const TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT_T *)payload)->size,
                                            ((const TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT_T *)payload)->param);


        }
        break;

        default:
        {
            /* Unrecognised Internal message */
            GATT_TBS_CLIENT_DEBUG_PANIC(("TBSC: Client Internal Msg not handled [0x%x]\n", id));
        }
        break;
    }
}

/****************************************************************************/
void tbsClientMsgHandler(Task task, MessageId id, Message payload)
{
    if ((id >= GATT_MANAGER_MESSAGE_BASE) && (id < GATT_MANAGER_MESSAGE_TOP))
    {
        handleGattManagerMsg(task, id, payload);
    }
    else if ((id >= GATT_MESSAGE_BASE) && (id < GATT_MESSAGE_TOP))
    {
        handleGattMsg(id);
    }
    else
    {
        handleInternalTbsMsg(task, id, payload);
    }
}

