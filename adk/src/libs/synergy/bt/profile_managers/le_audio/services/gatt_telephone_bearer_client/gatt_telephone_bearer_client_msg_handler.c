/******************************************************************************
 Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "gatt_telephone_bearer_client_private.h"
#include "gatt_telephone_bearer_client_discover.h"
#include "gatt_telephone_bearer_client_notification.h"
#include "gatt_telephone_bearer_client_read.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "gatt_telephone_bearer_client_write.h"
#include "csr_bt_gatt_lib.h"

/****************************************************************************/
static void handleGattManagerMsg(void* task, MsgId id, Msg payload)
{
    GTBSC *tbs_client = (GTBSC *)task;
    
    switch (id)
    {
        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
        {
            handleTbsNotification(tbs_client->srvcElem->service_handle, (const CsrBtGattClientNotificationInd *)payload);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
        {
            handleDiscoverAllTbsCharacteristicsResp(tbs_client, (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)payload);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
        {
            handleDiscoverAllTbsCharacteristicDescriptorsResp(tbs_client, (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)payload);
        }
        break;

        case CSR_BT_GATT_READ_CFM:
        {
            handleTbsReadCharacteristicValueResp(tbs_client, (const CsrBtGattReadCfm *)payload);
        }
        break;

        case CSR_BT_GATT_WRITE_CFM:
        {
            handleTbsWriteCharacteristicValueResp(tbs_client, (const CsrBtGattWriteCfm *)payload);
        }
        break;

        default:
        {
            /* Unrecognised GATT Manager message */
            GATT_TBS_CLIENT_WARNING("TBSC: Client GattMgr Msg not handled [0x%x]\n", id);
        }
        break;
    }
}

/****************************************************************************/
static void handleInternalTbsMsg(void* task, MsgId id, Msg payload)
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
                                                     ((const TelephoneBearerInternalMsgSetNotification *)payload)->notificationsEnable,
                                                     ((const TelephoneBearerInternalMsgSetNotification *)payload)->handle );
        }
        break;

        case TELEPHONE_BEARER_INTERNAL_MSG_WRITE_SIGNAL_STRENGTH_INTERVAL:
        {
            tbsWriteSignalStrengthIntervalRequest(tbs_client,
                                                  ((const TelephoneBearerInternalMsgWriteSignalStrengthInterval *)payload)->interval,
                                                  ((const TelephoneBearerInternalMsgWriteSignalStrengthInterval *)payload)->writeWithoutResponse);
        }
        break;

        case TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT:
        {
            tbsWriteCallControlPointRequest(tbs_client,
                                            ((const TelephoneBearerInternalMsgWriteCallControlPoint *)payload)->opcode,
                                            ((const TelephoneBearerInternalMsgWriteCallControlPoint *)payload)->size,
                                            ((const TelephoneBearerInternalMsgWriteCallControlPoint *)payload)->param);


        }
        break;

        default:
        {
            /* Unrecognised Internal message */
            GATT_TBS_CLIENT_WARNING("TBSC: Client Internal Msg not handled [0x%x]\n", id);
        }
        break;
    }
}


/****************************************************************************/
void gattTbsClientMsgHandler(void **gash)
{
    uint16 eventClass = 0;
    void *message = NULL;
    gatt_tbs_client *inst = *((gatt_tbs_client **)gash);

    if (CsrSchedMessageGet(&eventClass, &message))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                CsrBtGattPrim *id = message;
                GTBSC *tbs_client = (GTBSC *) GetServiceClientByGattMsg(&inst->service_handle_list, message);
                void *msg = GetGattManagerMsgFromGattMsg(message, id);

                if(tbs_client)
                    handleGattManagerMsg(tbs_client, *id, msg);

                if(msg!=message)
                    CsrPmemFree(msg);
                CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, message);
                
            }
                break;
            case TBS_CLIENT_PRIM:
            {
                TelephoneBearerInternalGenericRead *msg = (TelephoneBearerInternalGenericRead *)message;
                GTBSC *tbs_client = (GTBSC *) ServiceHandleGetInstanceData(msg->srvcHndl);

                if(tbs_client)
                    handleInternalTbsMsg(tbs_client, msg->id, message);
            }
                break;
            default:
                GATT_TBS_CLIENT_WARNING("GTBSC: Client Msg not handled [0x%x]\n", eventClass);
        }

        SynergyMessageFree(eventClass, message);
    }
}

