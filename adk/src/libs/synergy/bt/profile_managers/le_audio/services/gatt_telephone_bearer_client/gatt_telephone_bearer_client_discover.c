/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#include <string.h>
#include <stdio.h>

#include "gatt_telephone_bearer_client_discover.h"
#include "gatt_telephone_bearer_client_init.h"
#include "gatt_telephone_bearer_client_notification.h"
#include "gatt_telephone_bearer_client_read.h"
#include "gatt_telephone_bearer_client_write.h"



/****************************************************************************/
void discoverAllTbsCharacteristics(GTBSC *tbs_client)
{
    CsrBtGattDiscoverAllCharacOfAServiceReqSend(tbs_client->srvcElem->gattId,
                                                tbs_client->srvcElem->cid,
                                                tbs_client->startHandle,
                                                tbs_client->endHandle);
}


/****************************************************************************/
void handleDiscoverAllTbsCharacteristicsResp(GTBSC *tbs_client, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *msg)
{
    GATT_TBS_CLIENT_INFO("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM \n");

    if (msg->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        if (msg->uuid[0] == GATT_TBS_UUID_BEARER_PROVIDER_NAME)
        {
            tbs_client->handles.bearerNameHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_BEARER_UCI &&
                 msg->properties == (uint8)(ATT_PERM_READ))
        {
            tbs_client->handles.bearerUciHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_BEARER_TECHNOLOGY &&
                 msg->properties == (uint8)(ATT_PERM_READ | ATT_PERM_NOTIFY))
        {
            tbs_client->handles.bearerTechHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_BEARER_URI_PREFIX_LIST &&
                 (msg->properties & (uint8)(ATT_PERM_READ)))
        {
            tbs_client->handles.bearerUriPrefixListHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_SIGNAL_STRENGTH &&
                 msg->properties == (uint8)(ATT_PERM_READ | ATT_PERM_NOTIFY))
        {
            tbs_client->handles.signalStrengthHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_SIGNAL_STRENGTH_REPORTING_INTERVAL &&
                 msg->properties == (uint8)(ATT_PERM_READ | ATT_PERM_WRITE_REQ | ATT_PERM_WRITE_CMD))
        {
            tbs_client->handles.signalStrengthIntervalHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_LIST_CURRENT_CALLS &&
                 msg->properties == (uint8)(ATT_PERM_READ | ATT_PERM_NOTIFY))
        {
            tbs_client->handles.listCurrentCallsHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_CONTENT_CONTROL_ID &&
                 msg->properties == (uint8)(ATT_PERM_READ))
        {
            tbs_client->handles.contentControlIdHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_STATUS_FLAGS &&
                 msg->properties == (uint8)(ATT_PERM_READ | ATT_PERM_NOTIFY))
        {
            tbs_client->handles.statusFlagsHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_INCOMING_CALL_TARGET_BEARER_URI &&
                 msg->properties == (uint8)(ATT_PERM_READ | ATT_PERM_NOTIFY))
        {
            tbs_client->handles.incomingTargetBearerUriHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_CALL_STATE &&
                 msg->properties == (uint8)(ATT_PERM_READ | ATT_PERM_NOTIFY))
        {
            tbs_client->handles.callStateHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_CALL_CONTROL_POINT &&
                 (msg->properties & (ATT_PERM_NOTIFY | ATT_PERM_WRITE_REQ | ATT_PERM_WRITE_CMD)))
        {
            tbs_client->handles.callControlPointHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_CALL_CONTROL_POINT_OPCODES &&
                 msg->properties == (uint8)(ATT_PERM_READ))
        {
            tbs_client->handles.callControlPointOptionalOpcodesHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_TERMINATION_REASON &&
                 msg->properties == (uint8)(ATT_PERM_NOTIFY))
        {
            tbs_client->handles.terminationReasonHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_INCOMING_CALL &&
                 msg->properties == (uint8)(ATT_PERM_READ | ATT_PERM_NOTIFY))
        {
            tbs_client->handles.incomingCallHandle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_REMOTE_FRIENDLY_NAME &&
                 msg->properties == (uint8)(ATT_PERM_READ | ATT_PERM_NOTIFY))
        {
            tbs_client->handles.remoteFriendlyNameHandle = msg->handle;
        }
        else
        {
            /* Others UUIDs not relevant */
            GATT_TBS_CLIENT_INFO("handleDiscoverAllTbsCharacteristicsResp UNKNOWN or INCORRECT UUID:0x%04x Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->uuid[0], msg->handle, msg->declaration, msg->properties);
        }

    }
    
    if (!msg->more_to_come)
    {        
        /* Check for mandatory characteristics */
        if (!tbs_client->handles.bearerNameHandle ||
            !tbs_client->handles.bearerUciHandle ||
            !tbs_client->handles.bearerTechHandle ||
            !tbs_client->handles.bearerUriPrefixListHandle ||
            !tbs_client->handles.listCurrentCallsHandle ||
            !tbs_client->handles.contentControlIdHandle ||
            !tbs_client->handles.statusFlagsHandle ||
            !tbs_client->handles.callStateHandle ||
            !tbs_client->handles.callControlPointHandle ||
            !tbs_client->handles.callControlPointOptionalOpcodesHandle ||
            !tbs_client->handles.incomingCallHandle )
        {
            GATT_TBS_CLIENT_WARNING("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM not all characteristics found\n");
            /* One of the TBS characteristics is not found or has incorrect properties, initialisation failure */
            gattTbsClientSendInitComplete(tbs_client, GATT_TELEPHONE_BEARER_CLIENT_STATUS_DISCOVERY_ERR);
        }
        else
        {
            GATT_TBS_CLIENT_DEBUG("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM - OK, find descriptors\n");
            /* All mandatory TBS characteristics found, find the descriptors */
            discoverAllTbsCharacteristicDescriptors(tbs_client);
        }

    }
}


/****************************************************************************/
void discoverAllTbsCharacteristicDescriptors(GTBSC *tbs_client)
{
    CsrBtGattDiscoverAllCharacDescriptorsReqSend(tbs_client->srvcElem->gattId,
                                                tbs_client->srvcElem->cid,
                                                tbs_client->startHandle + 1,
                                                tbs_client->endHandle);
}


/****************************************************************************/
void handleDiscoverAllTbsCharacteristicDescriptorsResp(GTBSC *tbs_client, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T * msg = (GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)cfm;

    GATT_TBS_CLIENT_INFO("GTBSC: DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                          cfm->status,
                          cfm->handle,
                          cfm->uuid[0],
                          cfm->more_to_come);

    if (msg->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        if(msg->uuid_type == ATT_UUID16)
        {
            GATT_TBS_CLIENT_DEBUG("GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM: Success! Handle:0x%04x UUID: 0x%04x\n",
                      msg->handle, msg->uuid[0]);
        }
        else if(msg->uuid_type == ATT_UUID32)
        {
            GATT_TBS_CLIENT_DEBUG("GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM: Success! Handle:0x%04x UUID: 0x%08x\n",
                      msg->handle, msg->uuid[0]);
        }
        else if(msg->uuid_type == ATT_UUID128)
        {
             GATT_TBS_CLIENT_DEBUG("GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM: Success! Handle:0x%04x UUID: 0x%08x 0x%08x 0x%08x 0x%08x\n",
                       msg->handle, msg->uuid[0], msg->uuid[1], msg->uuid[2], msg->uuid[3]);
        }

        switch(msg->uuid[0])
        {
            /* Characteristics that have a Client Characteristic Config */
            case GATT_TBS_UUID_BEARER_PROVIDER_NAME:
            case GATT_TBS_UUID_BEARER_TECHNOLOGY:
            case GATT_TBS_UUID_SIGNAL_STRENGTH:
            case GATT_TBS_UUID_LIST_CURRENT_CALLS:
            case GATT_TBS_UUID_STATUS_FLAGS:
            case GATT_TBS_UUID_INCOMING_CALL_TARGET_BEARER_URI:
            case GATT_TBS_UUID_CALL_STATE:
            case GATT_TBS_UUID_CALL_CONTROL_POINT:
            case GATT_TBS_UUID_TERMINATION_REASON:
            case GATT_TBS_UUID_INCOMING_CALL:
            case GATT_TBS_UUID_REMOTE_FRIENDLY_NAME:
                GATT_TBS_CLIENT_DEBUG("GTBSC: Descriptor UUID: 0x%04x\n", msg->uuid[0]);
                tbs_client->nextDescriptorHandle = msg->uuid[0];
            break;

            case GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID:
            {
                GATT_TBS_CLIENT_INFO("GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID: UUID: 0x%04x, Desc handle:0x%04x\n",
                                       msg->uuid[0], tbs_client->nextDescriptorHandle);

                switch (tbs_client->nextDescriptorHandle)
                {
                case GATT_TBS_UUID_BEARER_PROVIDER_NAME:
                    tbs_client->handles.bearerNameCccHandle = msg->handle;
                    break;
                case GATT_TBS_UUID_BEARER_TECHNOLOGY:
                    tbs_client->handles.bearerTechCccHandle = msg->handle;
                    break;
                case GATT_TBS_UUID_SIGNAL_STRENGTH:
                    tbs_client->handles.signalStrengthCccHandle = msg->handle;
                    break;
                case GATT_TBS_UUID_LIST_CURRENT_CALLS:
                    tbs_client->handles.listCurrentCallsCccHandle = msg->handle;
                    break;
                case GATT_TBS_UUID_STATUS_FLAGS:
                    tbs_client->handles.statusFlagsCccHandle = msg->handle;
                    break;
                case GATT_TBS_UUID_INCOMING_CALL_TARGET_BEARER_URI:
                    tbs_client->handles.incomingTargetBearerUriCccHandle = msg->handle;
                    break;
                case GATT_TBS_UUID_CALL_STATE:
                    tbs_client->handles.callStateCccHandle = msg->handle;
                    break;
                case GATT_TBS_UUID_CALL_CONTROL_POINT:
                    tbs_client->handles.callControlPointCccHandle = msg->handle;
                    break;
                case GATT_TBS_UUID_TERMINATION_REASON:
                    tbs_client->handles.terminationReasonCccHandle = msg->handle;
                    break;
                case GATT_TBS_UUID_INCOMING_CALL:
                    tbs_client->handles.incomingCallCccHandle = msg->handle;
                    break;
                case GATT_TBS_UUID_REMOTE_FRIENDLY_NAME:
                    tbs_client->handles.remoteFriendlyNameCccHandle = msg->handle;
                    break;

                default:
                    break;
                }

                /* reset the handle */
                tbs_client->nextDescriptorHandle = 0;
            }
            break;

            default:
                break;
        }
    }
    else
    {
        GATT_TBS_CLIENT_ERROR("Discovery of all the TBS descriptors failed! Status: %d", msg->status);
    }


    if (!cfm->more_to_come)
    {
        /* Check all mandatory handles have been found */
        if (!tbs_client->handles.bearerNameCccHandle ||
            !tbs_client->handles.bearerTechCccHandle ||
            !tbs_client->handles.listCurrentCallsCccHandle ||
            !tbs_client->handles.statusFlagsCccHandle ||
            !tbs_client->handles.callStateCccHandle ||
            !tbs_client->handles.callControlPointCccHandle ||
            !tbs_client->handles.incomingCallCccHandle )
        {
            GATT_TBS_CLIENT_ERROR("TBSC: Client characteristc discovery - FAILED\n");
            gattTbsClientSendInitComplete(tbs_client,
                                   GATT_TELEPHONE_BEARER_CLIENT_STATUS_FAILED);
        }
        else
        {
            GATT_TBS_CLIENT_INFO("TBSC: Client characteristc discovery - SUCCESS\n");
            gattTbsClientSendInitComplete(tbs_client,
                                   GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS);
        }
    }
}
