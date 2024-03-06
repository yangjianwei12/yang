/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of le audio broadcast functionality
*/

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

#include "le_audio_client_context.h"
#include "le_audio_client_broadcast_audio_source.h"

#include "local_addr.h"

#ifdef ENABLE_ACK_FOR_PA_TRANSMITTED
#include "qualcomm_connection_manager.h"
#endif

#include <bdaddr.h>

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

/* Get source parameter data for delegator */
#define leAudioClientBroadcast_GetAsstSrcParam() (&leAudioClient_GetContext()->broadcast_asst_src_param)

/* Set source added to assistant */
#define leAudioClientBroadcast_SetSourceAddedToAssistant() \
    (leAudioClientBroadcast_GetAsstSrcParam()->is_source_added_to_assistant = TRUE)

/* Clear source added to assistant */
#define leAudioClientBroadcast_ClearSourceAddedToAssistant() \
    (leAudioClientBroadcast_GetAsstSrcParam()->is_source_added_to_assistant = FALSE)

/* Check if a source have been added to assistant or not */
#define leAudioClientBroadcast_IsSourceAddedToAssistant() \
    leAudioClientBroadcast_GetAsstSrcParam()->is_source_added_to_assistant

#else /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#define leAudioClientBroadcast_IsSourceAddedToAssistant()           (FALSE)
#define leAudioClientBroadcast_AddSource()

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! \brief Sends stream start indication to registered clients */
static void leAudioClientBroadcast_SendStreamStartInd(bool stream_started, uint16 audio_context)
{
    /* Send message with a valid group handle in case of streaming with assistant role */
    leAudioClientMessages_SendBroadcastStreamStartInd(leAudioClientBroadcast_IsSourceAddedToAssistant() ? \
                                                      leAudioClient_GetContext()->group_handle : INVALID_GROUP_HANDLE,
                                                      stream_started,
                                                      audio_context);
}

/*! \brief Sends stream stop indication to registered clients */
static void leAudioClientBroadcast_SendStreamStopInd(bool stream_started, uint16 audio_context)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    /* Send message with a valid group handle in case of streaming with assistant role */
    leAudioClientMessages_SendBroadcastStreamStopInd(!leAudioClient_StateIsNotConnected() ? client_ctxt->group_handle :
                                                                                            INVALID_GROUP_HANDLE,
                                                      stream_started,
                                                      audio_context);
}


/* A broadcast audio session is starting. Preserve the audio parameters used for the session */
static void leAudioClientBroadcast_StartAudioSession(CapClientContext audio_context)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    DEBUG_LOG("leAudioClientBroadcast_StartAudioSession");

    client_ctxt->broadcast_session_data.audio_context = audio_context;
    client_ctxt->broadcast_session_data.audio_config = leAudioClient_GetBroadcastAudioConfig();
    PanicNull((void *) client_ctxt->broadcast_session_data.audio_config);
}

/* A broadcast audio session is ending. Clean the audio parameters used for the session */
static void leAudioClientBroadcast_EndAudioSession(le_audio_broadcast_session_data_t *session_data)
{
    DEBUG_LOG("leAudioClientBroadcast_EndAudioSession");

    memset(session_data, 0, sizeof(le_audio_broadcast_session_data_t));
}

/*! \brief Configure the broadcast source */
static void leAudioClientBroadcast_ConfigureSrc(const le_audio_client_audio_broadcast_config_t *config)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();
    CapClientBcastSrcAdvParams bcast_adv_settings;

    /* Set the router mode before configuring */
    leAudioClientBroadcastRouter_SetMode(client_ctxt->requested_router_mode);

    /* Update advertising settings before configuring */
    leAudioClient_GetBroadcastAdvConfig(&bcast_adv_settings);
    leAudioClientBroadcastRouter_UpdateAdvSettings(&bcast_adv_settings);

    /* Update Broadcast ID if to use before configuring */
    leAudioClientBroadcastRouter_SetBroadcastId(config->broadcast_id);

    leAudioClientBroadcastRouter_Configure(config->presentation_delay,
                                           config->num_sub_group,
                                           config->sub_group_info,
                                           config->broadcast_source_name_len,
                                           config->broadcast_source_name,
                                           config->broadcast_type,
                                           &config->broadcast_config_params);
}

/*! \brief Handle broadcast router related messages in initializing state */
static void leAudioClientBroadcast_HandleRouterMessagesInInitializingState(MessageId id,
                                                                           Message message)
{
    switch (id)
    {
        case LEA_CLIENT_INTERNAL_BCAST_SRC_INIT_COMPLETE:
        {
            const LEA_CLIENT_INTERNAL_BCAST_SRC_INIT_COMPLETE_T *msg = message;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInInitializingState: init complete status %d",
                       msg->status);

            LeAudioClient_SetState(LE_AUDIO_CLIENT_STATE_INITIALIZED);

            /* Inform application that LE audio client init is completed */
            MessageSend(SystemState_GetTransitionTask(), LE_AUDIO_CLIENT_INIT_CFM, NULL);
        }
        break;

        default:
            DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInInitializingState: Unhandled msg %d", id);
        break;
    }
}

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

static bool leAudioClientBroadcast_AddSource(void)
{
    bool status = FALSE;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    /* Collocated broadcast source can be added to the assistant only if it is configured */
    if (leAudioClient_IsInConnectedState() &&
        client_ctxt->configured_broadcast_config_type != LE_AUDIO_CLIENT_BROADCAST_CONFIG_INVALID)
    {
        leAudioClientBroadcastRouter_StartScanningForSource(client_ctxt->group_handle,
                                                            client_ctxt->gatt_cid,
                                                            leAudioClient_GetBroadcastAudioConfig()->broadcast_type,
                                                            leAudioClient_GetBroadcastAudioConfig()->sub_group_info->useCase);

        status = TRUE;
    }

    DEBUG_LOG("leAudioClientBroadcast_AddSource status: %d, state enum:le_audio_client_state_t:%d"
              "configured source type %d source_added: %d",
               status, LeAudioClient_GetState(),
               client_ctxt->configured_broadcast_config_type,
               leAudioClientBroadcast_IsSourceAddedToAssistant());

    return status;
}

void leAudioClientBroadcast_AddSourceToAssistant(void)
{
    if (!leAudioClientBroadcast_IsSourceAddedToAssistant())
    {
        leAudioClientBroadcast_AddSource();
    }
}

/*! \brief Remove the added source from broadcast assistant */
static void leAudioClientBroadcast_RemoveSource(void)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    DEBUG_LOG("leAudioClientBroadcast_RemoveSource");

    /* Remove the added source */
    leAudioClientBroadcastRouter_RemoveSource(client_ctxt->group_handle,
                                              client_ctxt->gatt_cid,
                                              leAudioClientBroadcast_GetAsstSrcParam()->source_id);
}

/* Check if the received source information matches with currently active */
static bool leAudioClientBroadcast_IsMatchingSource(uint8 adv_sid, const typed_bdaddr *bd_taddr)
{
    bool is_matching = FALSE;
    bdaddr bd_addr;

    LocalAddr_GetProgrammedBtAddress(&bd_addr);

    /* Check if received address matches with local address */
    if (adv_sid == leAudioClientBroadcast_GetAsstSrcParam()->adv_sid &&
        BdaddrIsSame(&bd_taddr->addr, &bd_addr))
    {
        is_matching = TRUE;
    }

    return is_matching;
}

/*! \brief Handle broadcast router related messages in connected state */
static void leAudioClientBroadcast_HandleRouterMessagesInConnectedState(MessageId id, Message message)
{
    typed_bdaddr bd_taddr;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();
    le_audio_broadcast_asst_src_param_t *asst_src_param = leAudioClientBroadcast_GetAsstSrcParam();

    switch (id)
    {
        case LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_IND:
        {
            const LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_IND_T *brs_ind = message;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: brs ind");

            if (leAudioClientBroadcast_IsMatchingSource(brs_ind->adv_sid, &brs_ind->source_address))
            {
                /* Store the source ID */
                asst_src_param->source_id = brs_ind->source_id;

                /* Remove the source upon PA sync lost if the source is marked for removal */
                if (asst_src_param->is_current_source_to_be_removed &&
                   (brs_ind->pa_sync_state == CAP_CLIENT_PA_SYNC_NOT_SYNCHRONIZE ||
                    brs_ind->pa_sync_state == CAP_CLIENT_PA_SYNC_LOST))
                {
                    leAudioClientBroadcast_RemoveSource();
                    asst_src_param->is_current_source_to_be_removed = FALSE;
                }
            }
        }
        break;

        case LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_IND:
        {
            const LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_IND_T *brs_read_ind = message;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: brs read");

            if (leAudioClientBroadcast_IsMatchingSource(brs_read_ind->adv_sid, &brs_read_ind->source_address))
            {
                DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: matching source exists");

                /* A matching source exists in sink device */
                asst_src_param->source_id = brs_read_ind->source_id;

                if (brs_read_ind->pa_sync_state == CAP_CLIENT_PA_SYNC_SYNCHRONIZE_NO_PAST ||
                    brs_read_ind->pa_sync_state == CAP_CLIENT_PA_SYNC_SYNCHRONIZE_PAST)
                {
                    /* Matching source exists and able to PA sync */
                    asst_src_param->is_current_source_pa_synced = TRUE;
                }
                else
                {
                    /* Matching source not able to PA sync, remove the source and add it again
                       Note: Remove source can only be called when PA is not synced */
                    asst_src_param->is_current_source_to_be_removed = TRUE;
                }
            }
        }
        break;

        case LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_CFM:
        {
            const LEA_CLIENT_INTERNAL_BCAST_ASST_BRS_READ_CFM_T *brs_read_cfm = message;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: receiver state read cfm, status %d",
                       brs_read_cfm->status);

            /* Receiver state read completed. Now check if we can directly add the source or not */
            if (asst_src_param->is_current_source_to_be_removed)
            {
                /* There exists a source with same adv handle and source adddress. It can happen if
                   a reset or power off/on happen on source side while in broadcast streaming.
                   Remove the source before adding to avoid duplicate entries in sink side */
                leAudioClientBroadcast_RemoveSource();
            }
            else if (asst_src_param->is_current_source_pa_synced)
            {
                leAudioClientBroadcast_SetSourceAddedToAssistant();

                if (!leAudioClient_IsStreamingEnabled())
                {
                    leAudioClient_SetStreamingState();
                    /* Streaming was not enabled and source got added. Send the start stream indication */
                    leAudioClientBroadcast_SendStreamStartInd(TRUE, client_ctxt->broadcast_session_data.audio_context);
                }
            }
            else
            {
                /* No duplicate source in sink. Proceed with adding the source */
                DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: register notifications");
                leAudioClientBroadcastRouter_RegisterForGattNotification(client_ctxt->group_handle,
                                                                         0, asst_src_param->source_id);
            }
        }
        break;

        case LEA_CLIENT_INTERNAL_BCAST_ASST_SCAN_REPORT:
        {
            const LEA_CLIENT_INTERNAL_BCAST_ASST_SCAN_REPORT_T *source_report = message;

            PanicFalse(source_report->collocated);

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: source report");

            /* Store the source parameters here */
            asst_src_param->adv_sid = source_report->adv_sid;
            asst_src_param->adv_handle = source_report->adv_handle;
            asst_src_param->broadcast_id = source_report->broadcast_id;

            /* Store the bitmasked bis index's to use in add source API */
            asst_src_param->bis_index = 0;
            PanicFalse(source_report->num_subgroup > 0);

            for (int i = 0; i < source_report->subgroup_info->numBis; i++)
            {
                 asst_src_param->bis_index |= source_report->subgroup_info->bisInfo[i].bisIndex;
            }

            if (source_report->subgroup_info != NULL)
            {
                pfree(source_report->subgroup_info->bisInfo);
                pfree(source_report->subgroup_info);
            }
        }
        break;

        case LEA_CLIENT_INTERNAL_BCAST_ASST_START_SCAN_CFM:
        {
            const LEA_CLIENT_INTERNAL_BCAST_ASST_START_SCAN_CFM_T *start_scan_cfm = message;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: start scan status %d",
                       start_scan_cfm->status);

            /* Stop the scanning */
            leAudioClientBroadcastRouter_StopScanningForSource(client_ctxt->group_handle,
                                                               client_ctxt->gatt_cid);
        }
        break;

        case LEA_CLIENT_INTERNAL_BCAST_ASST_STOP_SCAN_CFM:
        {
            const LEA_CLIENT_INTERNAL_BCAST_ASST_STOP_SCAN_CFM_T *scan_stop_cfm = message;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: scan stop status %d",
                       scan_stop_cfm->status);

            /* Clear the flag before reading the receiver state */
            asst_src_param->is_current_source_to_be_removed = FALSE;
            asst_src_param->is_current_source_pa_synced = FALSE;

            /* Read the receiver state to see if there is a matching source already exists in sink */
            leAudioClientBroadcastRouter_ReadReceiverSinkState(client_ctxt->group_handle, client_ctxt->gatt_cid);
        }
        break;

        case LEA_CLIENT_INTERNAL_BCAST_ASST_REGISTER_NOTIFICATION_CFM:
        {
            const LEA_CLIENT_INTERNAL_BCAST_ASST_REGISTER_NOTIFICATION_CFM_T *register_cfm = message;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: register notification status %d",
                       register_cfm->status);

            /* Use the dongle local address to add the source as source is collocated */
            LocalAddr_GetProgrammedBtAddress(&bd_taddr.addr);
            bd_taddr.type = TYPED_BDADDR_PUBLIC;

            /* Add the collocated source */
            leAudioClientBroadcastRouter_AddSource(client_ctxt->group_handle,
                                                   client_ctxt->gatt_cid,
                                                   bd_taddr,
                                                   asst_src_param->adv_handle,
                                                   asst_src_param->adv_sid,
                                                   asst_src_param->broadcast_id,
                                                   asst_src_param->bis_index);
        }
        break;

        case LEA_CLIENT_INTERNAL_BCAST_ASST_ADD_SOURCE_CFM:
        {
            const LEA_CLIENT_INTERNAL_BCAST_ASST_ADD_SOURCE_CFM_T *add_src_cfm = message;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: add source status %d",
                       add_src_cfm->status);

            if (add_src_cfm->status == LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS)
            {
                leAudioClientBroadcast_SetSourceAddedToAssistant();
            }

            /* Add source cfm can be received in 2 sceanarios.
               a) When a connection happens with sink when broadcast source only streaming is
                  already ongoing
               b) When a broadcast streaming is started while connection with sink exists. In this case
                  we need to send stream start indication upon receiving add source cfm */
            if (!leAudioClient_IsStreamingEnabled())
            {
                leAudioClientBroadcast_SendStreamStartInd(add_src_cfm->status == LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS,
                                                          client_ctxt->broadcast_session_data.audio_context);
                leAudioClient_SetStreamingState();
            }
        }
        break;

        case LEA_CLIENT_INTERNAL_BCAST_ASST_MODIFY_SOURCE_CFM:
        {
            const LEA_CLIENT_INTERNAL_BCAST_ASST_MODIFY_SOURCE_CFM_T *modify_src_cfm = message;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: modify source status %d",
                       modify_src_cfm->status);

            if (modify_src_cfm->status == LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS)
            {
                /* Source have modified to make sure to not to PA sync. Remove the source when PA sync gets lost */
                asst_src_param->is_current_source_to_be_removed = TRUE;
            }
            else
            {
                /* Send stream start failed indication */
                leAudioClientBroadcast_SendStreamStartInd(FALSE, client_ctxt->broadcast_session_data.audio_context);
                leAudioClientBroadcast_EndAudioSession(&client_ctxt->broadcast_session_data);
            }
        }
        break;

        case LEA_CLIENT_INTERNAL_BCAST_ASST_REMOVE_SOURCE_CFM:
        {
            const LEA_CLIENT_INTERNAL_BCAST_ASST_REMOVE_SOURCE_CFM_T *remove_source_cfm = message;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: remove source status %d",
                       remove_source_cfm->status);

            if (remove_source_cfm->status == LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS)
            {
                leAudioClientBroadcast_ClearSourceAddedToAssistant();
            }

            if (asst_src_param->is_current_source_to_be_removed)
            {
                /* Clear the flag as source now removed*/
                asst_src_param->is_current_source_to_be_removed = FALSE;

                DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: register");
                leAudioClientBroadcastRouter_RegisterForGattNotification(client_ctxt->group_handle,
                                                                      0, asst_src_param->source_id);
            }
            else
            {
                /* Source is removed as part of adding a new source as part of start streaming.
                   Configure the new source */
                leAudioClientBroadcast_ConfigureSrc(leAudioClient_GetBroadcastAudioConfig());
            }
        }
        break;

        default:
            DEBUG_LOG("leAudioClientBroadcast_HandleRouterMessagesInConnectedState: Unhandled msg %d", id);
        break;
    }
}

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! \brief Handle TMAP domain broadcast related streaming messages */
static void leAudioClientBroadcast_HandleRouterStreamingMessages(MessageId id,
                                                                 Message message,
                                                                 le_audio_client_state_t state)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    PanicFalse(state >= LE_AUDIO_CLIENT_STATE_INITIALIZED);

    switch (id)
    {
        case LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_REMOVE:
        {
            const LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_REMOVE_T *msg = message;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterStreamingMessages: state enum:le_audio_client_state_t:%d stream remove status %d",
                       state, msg->status);

            /* LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_REMOVE can be received in 2 scenarios.
               a) When stop broadcast streaming API is called with remove_config parameter as TRUE
               b) When start broadcast streaming API is called with a new configuration and needs to reconfigure
             */
            if (LeAudioClient_IsBroadcastSourceStreamingActive())
            {
                /* Send stream stop indication to registered clients */
                leAudioClientBroadcast_SendStreamStopInd(msg->status == LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS,
                                                         client_ctxt->broadcast_session_data.audio_context);
                /* Cleanup the session data */
                leAudioClientBroadcast_EndAudioSession(&client_ctxt->broadcast_session_data);

                leAudioClient_ClearStreamingState();
            }
            else
            {
                if (msg->status == LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS)
                {
                    client_ctxt->configured_broadcast_config_type = LE_AUDIO_CLIENT_BROADCAST_CONFIG_INVALID;

                    if (leAudioClientBroadcast_IsSourceAddedToAssistant())
                    {
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
                        le_audio_broadcast_asst_src_param_t *asst_src_param = leAudioClientBroadcast_GetAsstSrcParam();

                        /* Now we need to remove the existing source using assistant API in order to add new
                           source. However removing of source can only be done when PA is not synced. Ideally
                           since the broadcast source stream have now removed, sink shouldn't be able to PA sync
                           but certain sinks have timeout on their side before it will really consider the PA sync
                           have lost. So here we first modify the source asking not to sync with PA and then remove
                           the source using assistant APIs.
                         */
                        leAudioClientBroadcastRouter_ModifySource(client_ctxt->group_handle,
                                                                  client_ctxt->gatt_cid,
                                                                  asst_src_param->adv_handle,
                                                                  asst_src_param->adv_sid,
                                                                  asst_src_param->bis_index,
                                                                  asst_src_param->source_id,
                                                                  FALSE);
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
                    }
                    else
                    {
                        /* Proceed with configuring the new source */
                        leAudioClientBroadcast_ConfigureSrc(leAudioClient_GetBroadcastAudioConfig());
                    }
                }
                else
                {
                    /* Send stream start failed indication */
                    leAudioClientBroadcast_SendStreamStartInd(FALSE, client_ctxt->broadcast_session_data.audio_context);
                    leAudioClientBroadcast_EndAudioSession(&client_ctxt->broadcast_session_data);
                }
            }
        }
        break;

        case LEA_CLIENT_INTERNAL_BCAST_SRC_CONFIG_COMPLETE:
        {
            const LEA_CLIENT_INTERNAL_BCAST_SRC_CONFIG_COMPLETE_T *msg = message;
            const uint8 *broadcast_code = leAudioClient_GetBroadcastAudioConfig()->broadcast_code;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterStreamingMessages: state enum:le_audio_client_state_t:%d config complete status %d",
                       state, msg->status);

            if (msg->status == LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS)
            {
                /* Store the configured broadcast configuration type */
                client_ctxt->configured_broadcast_config_type = leAudioClient_GetBroadcastAudioConfig()->config_type;

                /* Configuration success. Now start the streaming */
                leAudioClientBroadcastRouter_StartStreaming(broadcast_code != NULL,
                                                            broadcast_code);
            }
            else
            {
                /* Send stream start failed indication */
                leAudioClientBroadcast_SendStreamStartInd(FALSE, client_ctxt->broadcast_session_data.audio_context);
                leAudioClientBroadcast_EndAudioSession(&client_ctxt->broadcast_session_data);
            }
        }
        break;

        case LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_START_CFM:
        {
            const LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_START_CFM_T *msg = message;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterStreamingMessages: state enum:le_audio_client_state_t:%d stream start status %d",
                       state, msg->status);
            if (msg->status == LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS)
            {
                if (leAudioClient_IsInConnectedState())
                {
                    /* LE audio client is in connected state. So add the source to assistant */
                    leAudioClientBroadcast_AddSource();
                }
                else
                {
                    leAudioClient_SetStreamingState();
                    /* LE audio client is not connected. Send stream start indication */
                    leAudioClientBroadcast_SendStreamStartInd(TRUE, client_ctxt->broadcast_session_data.audio_context);
                }

                leAudioClientBroadcast_StoreAudioParams(msg);
            }
            else
            {
                /* LE audio client is not connected. Send stream start indication */
                leAudioClientBroadcast_SendStreamStartInd(FALSE, client_ctxt->broadcast_session_data.audio_context);
                                    leAudioClientBroadcast_EndAudioSession(&client_ctxt->broadcast_session_data);
            }
        }
        break;

        case LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_STOP_CFM:
        {
            const LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_STOP_CFM_T *msg = message;

            DEBUG_LOG("leAudioClientBroadcast_HandleRouterStreamingMessages: state enum:le_audio_client_state_t:%d stream stop status:%d",
                       state, msg->status);

            if (client_ctxt->broadcast_session_data.release_config)
            {
                if (LeAudioClient_IsBroadcastSourceStreamingActive())
                {
                    client_ctxt->broadcast_session_data.release_config = FALSE;
                    leAudioClientBroadcastRouter_RemoveStream();
                }
            }
            else
            {
                leAudioClientBroadcast_SendStreamStopInd(msg->status == LEA_CLIENT_BCAST_ROUTER_MSG_STATUS_SUCCESS,
                                                         client_ctxt->broadcast_session_data.audio_context);
                /* Cleanup the session data */
                leAudioClientBroadcast_EndAudioSession(&client_ctxt->broadcast_session_data);

                leAudioClient_ClearStreamingState();
            }
        }
        break;

        default:
        break;
    }
}

/*! \brief Handles messages from LE audio client broadcast router */
bool leAudioClientBroadcast_ProcessMsgIfFromBcastRouter(MessageId id, Message message)
{
    bool is_bcast_message = FALSE;

    if (id >= LEA_CLIENT_INTERNAL_BCAST_SRC_INIT_COMPLETE &&
        id < LEA_CLIENT_INTERNAL_BCAST_MAX)
    {
        is_bcast_message = TRUE;
    }

    /* Process broadcast router streaming related messages regardless of state */
    switch (id)
    {
        case LEA_CLIENT_INTERNAL_BCAST_SRC_CONFIG_COMPLETE:
        case LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_START_CFM:
        case LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_STOP_CFM:
        case LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_REMOVE:
            leAudioClientBroadcast_HandleRouterStreamingMessages(id, message, LeAudioClient_GetState());
        return TRUE;
        
#ifdef ENABLE_ACK_FOR_PA_TRANSMITTED
        case QCOM_CON_MANAGER_PA_TRANSMITTED_ACK_IND:
            DEBUG_LOG("Received Ack for PA getting transmitted");
            leAudioClientMessages_SendPATransmittedInd();
        return TRUE;
#endif

        default:
        break;
    }

    switch (LeAudioClient_GetState())
    {
        case LE_AUDIO_CLIENT_STATE_INITIALIZING:
            leAudioClientBroadcast_HandleRouterMessagesInInitializingState(id, message);
        break;

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
        case LE_AUDIO_CLIENT_STATE_CONNECTED:
            leAudioClientBroadcast_HandleRouterMessagesInConnectedState(id, message);
        break;
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

        default:
            DEBUG_LOG("leAudioClientBroadcast_ProcessMsgIfFromBcastRouter: Unhandled enum:le_audio_client_internal_msg_t:%d"
                      "state enum:le_audio_client_state_t:%d", id, LeAudioClient_GetState());
        break;
    }

    return is_bcast_message;
}

void leAudioClientBroadcast_Init(void)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    client_ctxt->configured_broadcast_config_type = LE_AUDIO_CLIENT_BROADCAST_CONFIG_INVALID;

    /* Initialise broadcast source role */
    leAudioClientBroadcastRouter_Init();
    leAudioClient_InitBroadcastAudioSource();
    leAudioClient_SetBroadcastAudioConfigType(LE_AUDIO_CLIENT_BROADCAST_CONFIG_TYPE_HQ);
}

void LeAudioClientBroadcast_BroadcastConfigChanged(void)
{
    if (le_audio_client_context.configured_broadcast_config_type != LE_AUDIO_CLIENT_BROADCAST_CONFIG_INVALID)
    {
        le_audio_client_context.is_bcast_config_changed = TRUE;
    }
}

void LeAudioClientBroadcast_SetPbpMode(bool pbp_enable)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    client_ctxt->requested_router_mode = pbp_enable ? LEA_CLIENT_BCAST_ROUTER_MODE_PBP :
                                                      LEA_CLIENT_BCAST_ROUTER_MODE_TMAP;

    DEBUG_LOG_INFO("LeAudioClientBroadcast_SetPbpMode pbp_mode = %d", pbp_enable);
}

bool LeAudioClientBroadcast_IsInPbpMode(void)
{
    return leAudioClient_GetContext()->requested_router_mode == LEA_CLIENT_BCAST_ROUTER_MODE_PBP;
}

bool leAudioClientBroadcast_StartStreaming(uint16 audio_context)
{
    bool stream_start_req = FALSE;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();
    const le_audio_client_audio_broadcast_config_t *broadcast_audio_config = NULL;

    DEBUG_LOG("leAudioClient_StartBroadcastStreaming");

    if (LeAudioClient_IsBroadcastSourcePresent())
    {
        /* Not in streaming state. Get the configuration to use for starting the streaming */
        broadcast_audio_config = leAudioClient_GetBroadcastAudioConfig();

        leAudioClientBroadcast_StartAudioSession(audio_context);

        if (client_ctxt->configured_broadcast_config_type == LE_AUDIO_CLIENT_BROADCAST_CONFIG_INVALID)
        {
            leAudioClientBroadcast_ConfigureSrc(broadcast_audio_config);
        }
        else if(client_ctxt->configured_broadcast_config_type == broadcast_audio_config->config_type &&
                !le_audio_client_context.is_bcast_config_changed &&
                client_ctxt->requested_router_mode == leAudioClientBroadcastRouter_GetMode())
        {
            /* Configuration to use is already configured. Start the streaming directly */
            leAudioClientBroadcastRouter_StartStreaming(broadcast_audio_config->broadcast_code != NULL,
                                                        broadcast_audio_config->broadcast_code);
        }
        else
        {
            /* Configuration to use is different than currently configured. First remove
               the current configuration and then stream after reconfiguration */
            leAudioClientBroadcastRouter_RemoveStream();
            client_ctxt->is_bcast_config_changed = FALSE;
        }

        stream_start_req = TRUE;
    }

    return stream_start_req;
}

bool leAudioClientBroadcast_StopStreaming(bool remove_config)
{
    bool stop_streaming = FALSE;
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    DEBUG_LOG("leAudioClientBroadcast_StopStreaming");

    if (LeAudioClient_IsBroadcastSourceStreamingActive())
    {
        /* Stop the broadcast streaming */
        leAudioClientBroadcastRouter_StopStreaming();

        /* Remove the config later once the streaming has stopped and all the underlying
         * BIS is disconnected by TMAP.
         */
        client_ctxt->broadcast_session_data.release_config = remove_config;
        stop_streaming = TRUE;
    }

    return stop_streaming;
}

bool LeAudioClient_IsBroadcastSourcePresent(void)
{
    return (LeAudioClient_GetState() >= LE_AUDIO_CLIENT_STATE_INITIALIZED);
}

bool LeAudioClient_IsBroadcastSourceStreamingActive(void)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();

    return (leAudioClient_IsStreamingEnabled() &&
            client_ctxt->mode == LE_AUDIO_CLIENT_MODE_BROADCAST);
}

uint16 leAudioClientBroadcast_GetSampleRate(uint16 bap_sampling_freq)
{
    uint32 sample_rate = 0;

    switch(bap_sampling_freq)
    {
        case BAP_SAMPLING_FREQUENCY_16kHz:
            sample_rate = 16000;
            break;

        case BAP_SAMPLING_FREQUENCY_24kHz:
            sample_rate = 24000;
            break;

        case BAP_SAMPLING_FREQUENCY_32kHz:
            sample_rate = 32000;
            break;

        case BAP_SAMPLING_FREQUENCY_48kHz:
            sample_rate = 48000;
            break;

        default:
            DEBUG_LOG_INFO("leAudioClientBroadcast_GetSampleRate Not Found");
            break;
    }

    return sample_rate;
}

void leAudioClientBroadcast_StoreAudioParams(const LEA_CLIENT_INTERNAL_BCAST_SRC_STREAM_START_CFM_T *stream_params)
{
    le_audio_client_context_t *client_ctxt = leAudioClient_GetContext();
    uint8 index;

    client_ctxt->broadcast_session_data.frame_duration = stream_params->frame_duration;
    client_ctxt->broadcast_session_data.sampling_frequency = stream_params->sampling_frequency;
    client_ctxt->broadcast_session_data.octets_per_frame = stream_params->octets_per_frame;

    for(index = 0; index < stream_params->num_bis; index++)
    {
        client_ctxt->broadcast_session_data.bis_handles[index] = stream_params->bis_handles[index];
    }

    client_ctxt->broadcast_session_data.transport_latency_big = stream_params->transport_latency_big;
    client_ctxt->broadcast_session_data.iso_interval = stream_params->iso_interval;
}

void leAudioClientBroadcast_ResetSourceContext(void)
{
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    le_audio_broadcast_asst_src_param_t *asst_src_param = leAudioClientBroadcast_GetAsstSrcParam();

    memset(asst_src_param, 0, sizeof(*asst_src_param));
#endif
}

bool LeAudioClientBroadcast_UpdateMetadataInPeriodicTrain(uint8 length, uint8 *metadata)
{
    bool status = FALSE;

    DEBUG_LOG("LeAudioClientBroadcast_UpdateMetadataInPeriodicTrain metadata_length: 0x%x", length);

    if (LeAudioClient_IsBroadcastSourceStreamingActive())
    {
        leAudioClientBroadcastRouter_UpdateMetadataInPeriodicTrain(length, metadata);

        status = TRUE;
    }

    return status;
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
