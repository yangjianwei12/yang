/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include <gatt.h>
#include <gatt_uuid.h>

#include "gatt_telephone_bearer_client_discover.h"
#include "gatt_telephone_bearer_client_init.h"
#include "gatt_telephone_bearer_client_notification.h"
#include "gatt_telephone_bearer_client_read.h"
#include "gatt_telephone_bearer_client_write.h"



/****************************************************************************/
void discoverAllTbsCharacteristics(GTBSC *tbs_client)
{
    GattManagerDiscoverAllCharacteristics(&tbs_client->lib_task);
}


/****************************************************************************/
void handleDiscoverAllTbsCharacteristicsResp(GTBSC *tbs_client, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *msg)
{
    if (msg->status == gatt_status_success)
    {
        if (msg->uuid[0] == GATT_TBS_UUID_BEARER_PROVIDER_NAME &&
                msg->properties == (gatt_char_prop_read | gatt_char_prop_notify) )
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM Bearer Provider Name Characteristic CFM: Success! Handle: 0x%04x Declaration: 0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->bearer_name_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_BEARER_UCI &&
                 msg->properties == gatt_char_prop_read)
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM Bearer UCI Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->bearer_uci_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_BEARER_TECHNOLOGY &&
                 msg->properties == (gatt_char_prop_read | gatt_char_prop_notify))
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM Bearer Technology Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->bearer_tech_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_BEARER_URI_PREFIX_LIST &&
                 msg->properties == gatt_char_prop_read)
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM URI Prefix List Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->bearer_uri_prefix_list_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_SIGNAL_STRENGTH &&
                 msg->properties == (gatt_char_prop_read | gatt_char_prop_notify))
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM Signal Strength Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->signal_strength_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_SIGNAL_STRENGTH_REPORTING_INTERVAL &&
                 msg->properties == (gatt_char_prop_read | gatt_char_prop_write | gatt_char_prop_write_no_response))
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM Signal Strength Reporting Interval Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->signal_strength_interval_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_LIST_CURRENT_CALLS &&
                 msg->properties == (gatt_char_prop_read | gatt_char_prop_notify))
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM List Current Calls Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->list_current_calls_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_CONTENT_CONTROL_ID &&
                 msg->properties == gatt_char_prop_read)
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM Content Control ID Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->content_control_id_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_STATUS_FLAGS &&
                 msg->properties == (gatt_char_prop_read | gatt_char_prop_notify))
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM Status Flags Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->status_flags_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_INCOMING_CALL_TARGET_BEARER_URI &&
                 msg->properties == (gatt_char_prop_read | gatt_char_prop_notify))
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM Incoming Call Target Bearer URI Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->incoming_target_bearer_uri_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_CALL_STATE &&
                 msg->properties == (gatt_char_prop_read | gatt_char_prop_notify))
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM List Call State Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->call_state_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_CALL_CONTROL_POINT &&
                 msg->properties == (gatt_char_prop_notify | gatt_char_prop_write | gatt_char_prop_write_no_response))
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM Call Control Point Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->call_control_point_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_CALL_CONTROL_POINT_OPCODES &&
                 msg->properties == gatt_char_prop_read)
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM Call Control Point Optional Opcodes Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->call_control_point_optional_opcodes_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_TERMINATION_REASON &&
                 msg->properties == gatt_char_prop_notify)
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM Termination Reason Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->termination_reason_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_INCOMING_CALL &&
                 msg->properties == (gatt_char_prop_read | gatt_char_prop_notify))
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM Incoming Call Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->incoming_call_handle = msg->handle;
        }
        else if (msg->uuid[0] == GATT_TBS_UUID_REMOTE_FRIENDLY_NAME &&
                 msg->properties == (gatt_char_prop_read | gatt_char_prop_notify))
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM Incoming Remote Friendly Name Characteristic CFM: Success! Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->handle, msg->declaration, msg->properties));

            tbs_client->remote_friendly_name_handle = msg->handle;
        }
        else
        {
            /* Others UUIDs not relevant */
            GATT_TBS_CLIENT_DEBUG_INFO(("handleDiscoverAllTbsCharacteristicsResp UNKNOWN or INCORRECT UUID:0x%04x Handle:0x%04x Declaration:0x%04x Prop:0x%04x\n",
                      msg->uuid[0], msg->handle, msg->declaration, msg->properties));
        }

    }
    
    if (!msg->more_to_come)
    {        
        /* Check for mandatory characteristics */
        if (!tbs_client->bearer_name_handle ||
            !tbs_client->bearer_uci_handle ||
            !tbs_client->bearer_tech_handle ||
            !tbs_client->bearer_uri_prefix_list_handle ||
            !tbs_client->list_current_calls_handle ||
            !tbs_client->content_control_id_handle ||
            !tbs_client->status_flags_handle ||
            !tbs_client->call_state_handle ||
            !tbs_client->call_control_point_handle ||
            !tbs_client->call_control_point_optional_opcodes_handle ||
            !tbs_client->incoming_call_handle )
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM not all characteristics found\n"));
            /* One of the TBS characteristics is not found or has incorrect properties, initialisation failure */
            gattTbsClientSendInitComplete(tbs_client, GATT_TBS_CLIENT_STATUS_DISCOVERY_ERR);
        }
        else
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTICS_CFM - OK, find descriptors\n"));
            /* All mandatory TBS characteristics found, find the descriptors */
            discoverAllTbsCharacteristicDescriptors(tbs_client);
        }

    }
}


/****************************************************************************/
void discoverAllTbsCharacteristicDescriptors(GTBSC *tbs_client)
{
    gatt_manager_client_service_data_t service_data;

    if (GattManagerGetClientData(&tbs_client->lib_task, &service_data))
    {
        GattManagerDiscoverAllCharacteristicDescriptors(&tbs_client->lib_task,
                                                        service_data.start_handle,
                                                        service_data.end_handle);
    }
    else
    {
        GATT_TBS_CLIENT_DEBUG_PANIC(("GTBSC: Characteristic discovery error\n"));
        gattTbsClientSendInitComplete(tbs_client, GATT_TBS_CLIENT_STATUS_FAILED);
    }
}


/****************************************************************************/
void handleDiscoverAllTbsCharacteristicDescriptorsResp(GTBSC *tbs_client, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T * msg = (GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)cfm;

    GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                                            cfm->status,
                                            cfm->handle,
                                            cfm->uuid[0],
                                            cfm->more_to_come));

    if (msg->status == gatt_status_success)
    {
        if(msg->uuid_type == gatt_uuid16)
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM: Success! Handle:0x%04x UUID: 0x%04x\n",
                      msg->handle, msg->uuid[0]));
        }
        else if(msg->uuid_type == gatt_uuid32)
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM: Success! Handle:0x%04x UUID: 0x%08x\n",
                      msg->handle, msg->uuid[0]));
        }
        else if(msg->uuid_type == gatt_uuid128)
        {
             GATT_TBS_CLIENT_DEBUG_INFO(("GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM: Success! Handle:0x%04x UUID: 0x%08x 0x%08x 0x%08x 0x%08x\n",
                       msg->handle, msg->uuid[0], msg->uuid[1], msg->uuid[2], msg->uuid[3]));
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
                GATT_TBS_CLIENT_DEBUG_INFO(("GTBSC: Descriptor UUID: 0x%04x\n", msg->uuid[0]));
                tbs_client->nextDescriptorHandle = msg->uuid[0];
            break;

            case GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID:
            {
                GATT_TBS_CLIENT_DEBUG_INFO(("GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID: UUID: 0x%04x, Desc handle:0x%04x\n",
                                            msg->uuid[0], tbs_client->nextDescriptorHandle));

                switch (tbs_client->nextDescriptorHandle)
                {
                case GATT_TBS_UUID_BEARER_PROVIDER_NAME:
                    tbs_client->bearer_name_ccc_handle = msg->handle;
                    break;
                case GATT_TBS_UUID_BEARER_TECHNOLOGY:
                    tbs_client->bearer_tech_ccc_handle = msg->handle;
                    break;
                case GATT_TBS_UUID_SIGNAL_STRENGTH:
                    tbs_client->signal_strength_ccc_handle = msg->handle;
                    break;
                case GATT_TBS_UUID_LIST_CURRENT_CALLS:
                    tbs_client->list_current_calls_ccc_handle = msg->handle;
                    break;
                case GATT_TBS_UUID_STATUS_FLAGS:
                    tbs_client->status_flags_ccc_handle = msg->handle;
                    break;
                case GATT_TBS_UUID_INCOMING_CALL_TARGET_BEARER_URI:
                    tbs_client->incoming_target_bearer_uri_ccc_handle = msg->handle;
                    break;
                case GATT_TBS_UUID_CALL_STATE:
                    tbs_client->call_state_ccc_handle = msg->handle;
                    break;
                case GATT_TBS_UUID_CALL_CONTROL_POINT:
                    tbs_client->call_control_point_ccc_handle = msg->handle;
                    break;
                case GATT_TBS_UUID_TERMINATION_REASON:
                    tbs_client->termination_reason_ccc_handle = msg->handle;
                    break;
                case GATT_TBS_UUID_INCOMING_CALL:
                    tbs_client->incoming_call_ccc_handle = msg->handle;
                    break;
                case GATT_TBS_UUID_REMOTE_FRIENDLY_NAME:
                    tbs_client->remote_friendly_name_ccc_handle = msg->handle;
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
        GATT_TBS_CLIENT_DEBUG_INFO(("Discovery of all the TBS descriptors failed! Status: %d", msg->status));
    }


    if (!cfm->more_to_come)
    {
        /* Check all mandatory handles have been found */
        if (!tbs_client->bearer_name_ccc_handle ||
            !tbs_client->bearer_tech_ccc_handle ||
            !tbs_client->list_current_calls_ccc_handle ||
            !tbs_client->status_flags_ccc_handle ||
            !tbs_client->call_state_ccc_handle ||
            !tbs_client->call_control_point_ccc_handle ||
            !tbs_client->incoming_call_ccc_handle )
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("TBSC: Client characteristc discovery - FAILED\n"));
            gattTbsClientSendInitComplete(tbs_client,
                                   GATT_TBS_CLIENT_STATUS_FAILED);
        }
        else
        {
            GATT_TBS_CLIENT_DEBUG_INFO(("TBSC: Client characteristc discovery - SUCCESS\n"));
            gattTbsClientSendInitComplete(tbs_client,
                                   GATT_TBS_CLIENT_STATUS_SUCCESS);
        }
    }
}
