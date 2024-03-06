/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    tmap_profile
    \brief      Initializes TBS server for TMAP
*/

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

#include "tmap_profile_mcs_tbs_private.h"
#include "tmap_server_role.h"
#include "telephony_messages.h"

#define TMAP_TBS_LOG     DEBUG_LOG

static tmap_profile_tbs_server_data_t tmapProfile_tbs_data =
{
    .service_handle = TMAP_INVALID_SERVICE_HANDLE,
    .call_id        = 0,
    .valid_uri_flag = TRUE,
};

void tmapProfileTbsServer_Init(Task app_task)
{
    GattTbsInitData tbs_init_data = {0};
    uint8 callIndex;

    TMAP_TBS_LOG("tmapProfile_TbsServerInit");

    TaskList_InitialiseWithCapacity(tmapProfileTbsServer_GetStatusNotifyList(), TMAP_PROFILE_REMOTE_CALL_CONTROLS_LIST_INIT_CAPACITY);

    /* Initialise with default values */
    tbs_init_data.providerNameLen = TMAP_DEFAULT_TBS_PROVIDER_NAME_LEN;
    if (tbs_init_data.providerNameLen)
    {
        tbs_init_data.providerName = (char*)PanicUnlessMalloc(tbs_init_data.providerNameLen);
        memcpy(tbs_init_data.providerName, TMAP_DEFAULT_TBS_PROVIDER_NAME, tbs_init_data.providerNameLen);
    }

    for(callIndex = 0; callIndex < TBS_CURRENT_CALLS_LIST_SIZE; callIndex++)
    {
        tbs_init_data.currentCallsList[callIndex].callId = 0xFF; /* FREE_CALL_SLOT */
        tbs_init_data.currentCallsList[callIndex].callState = TBS_CALL_STATE_INVALID;
    }

    tbs_init_data.contentControlId = TMAP_PROFILE_TBS_CCID;
    tbs_init_data.signalStrength = TMAP_DEFAULT_TBS_SIGNAL_STRENGTH ;
    tbs_init_data.signalStrengthReportingInterval = TMAP_DEFAULT_TBS_SIGNAL_STRENGTH_REPORTING_INTERVAL;
    tbs_init_data.technology = TBS_TECHNOLOGY_WIFI;
    tbs_init_data.statusFlags = TMAP_DEFAULT_TBS_STATUS_FLAGS;

    tmapProfile_tbs_data.service_handle = GattTelephoneBearerServerInit(TrapToOxygenTask(app_task),
                                                HANDLE_GENERIC_TELEPHONE_BEARER_SERVICE,
                                                HANDLE_GENERIC_TELEPHONE_BEARER_SERVICE_END,
                                                &tbs_init_data);

    PanicFalse(tmapProfile_tbs_data.service_handle != TMAP_INVALID_SERVICE_HANDLE);

    /* Set default UCI and URI values */
    PanicFalse(GattTelephoneBearerServerSetUci(tmapProfile_tbs_data.service_handle,
                                               TMAP_DEFAULT_TBS_UCI,
                                               TMAP_DEFAULT_TBS_UCI_LEN));
    PanicFalse(GattTelephoneBearerServerSetUriPrefixList(tmapProfile_tbs_data.service_handle,
                                                         TMAP_DEFAULT_TBS_URI_PREFIX,
                                                         TMAP_DEFAULT_TBS_URI_PREFIX_LEN));
}

void tmapProfileTbsServer_RegisterForRemoteCallControls(Task task)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(tmapProfileTbsServer_GetStatusNotifyList()), task);
}

void tmapProfileTbsServer_AddConfig(gatt_cid_t cid)
{
    if (tmapProfile_tbs_data.service_handle != TMAP_INVALID_SERVICE_HANDLE)
    {
        GattTbsServerConfig *tbs_config = NULL;

        GattTelephoneBearerServerAddConfig(tmapProfile_tbs_data.service_handle, cid, tbs_config);
    }
}

void tmapProfileTbsServer_RemoveConfig(gatt_cid_t cid)
{
    if (tmapProfile_tbs_data.service_handle != TMAP_INVALID_SERVICE_HANDLE)
    {
        GattTbsServerConfig *config;

        config = GattTelephoneBearerServerRemoveConfig(tmapProfile_tbs_data.service_handle, cid);
        pfree(config);
    }
}

static bool tmapProfileTbsServer_IsUriValid(const char *uri)
{
    UNUSED(uri);

    /* Todo : Implement proper way to distinguish between valid and invalid URI's */
    return tmapProfile_tbs_data.valid_uri_flag;
}

GattTbsCcpNotificationResultCodes tmapProfileTbsServer_CreateCall(GattTbsCallStates call_state,
                                                                  GattTbsCallFlags call_flags,
                                                                  uint16 target_uri_size,
                                                                  const char *target_uri)
{
    GattTbsCcpNotificationResultCodes status;

    if (tmapServer_IsPtsModeEnabled() && target_uri_size == 0)
    {
        /* In PTS mode, use the default target URI if it is not provided */
        target_uri_size = TMAP_PROFILE_TBS_SERVER_PTS_TARGET_BEARER_URI_LEN;
        target_uri = TMAP_PROFILE_TBS_SERVER_PTS_TARGET_BEARER_URI;
    }

    if (target_uri != NULL && !tmapProfileTbsServer_IsUriValid(target_uri))
    {
        /* Return invalid URI status as given URI is not valid */
        return TBS_CCP_RESULT_INVALID_OUTGOING_URI;
    }

    status = GattTelephoneBearerServerCreateCall(tmapProfile_tbs_data.service_handle, call_state, call_flags,
                                                 9, "skype:xyz", target_uri_size, (char*)target_uri, &tmapProfile_tbs_data.call_id);

    if (tmapServer_IsPtsModeEnabled())
    {
        GattTelephoneBearerServerSetCallFriendlyName(tmapProfile_tbs_data.service_handle, tmapProfile_tbs_data.call_id,
                                                     TMAP_PROFILE_TBS_SERVER_PTS_FRIENDLY_NAME_LEN,
                                                     TMAP_PROFILE_TBS_SERVER_PTS_FRIENDLY_NAME);
    }

    TMAP_TBS_LOG("tmapProfileTbsServer_CreateCall call id:%d status:%d", tmapProfile_tbs_data.call_id, status);

    return status;
}

bool tmapProfileTbsServer_SetCallState(GattTbsCallStates call_state)
{
    TMAP_TBS_LOG("tmapProfileTbsServer_SetCallState call_state: %d", call_state);

    return GattTelephoneBearerServerSetCallState(tmapProfile_tbs_data.service_handle, tmapProfile_tbs_data.call_id,
                                                 call_state, TRUE);
}

GattTbsCallStates tmapProfileTbsServer_GetCallState(void)
{
    GattTbsCallStates call_state = TBS_CALL_STATE_INVALID;

    GattTelephoneBearerServerGetCallState(tmapProfile_tbs_data.service_handle, tmapProfile_tbs_data.call_id, &call_state);

    TMAP_TBS_LOG("tmapProfileTbsServer_GetCallState call id:%d call_state:%d", tmapProfile_tbs_data.call_id, call_state);

    return call_state;
}

bool tmapProfileTbsServer_TerminateCall(GattTbsCallTerminationReason reason)
{
    bool status;

    status = GattTelephoneBearerServerTerminateCall(tmapProfile_tbs_data.service_handle,
                                                    tmapProfile_tbs_data.call_id, reason);

    TMAP_TBS_LOG("tmapProfileTbsServer_TerminateCall reason:%d success:%d", reason, status);

    return status;
}

bool tmapProfileTbsServer_IsCallPresent(void)
{
    bool call_active;
    GattTbsCallStates call_state;

    call_active = GattTelephoneBearerServerGetCallState(tmapProfile_tbs_data.service_handle, tmapProfile_tbs_data.call_id, &call_state);

    TMAP_TBS_LOG("tmapProfileTbsServer_IsCallPresent:%d state:%d", call_active, call_state);

    return call_active;
}

/*! Enable all the optional opcodes */
static bool tmapProfileTbsServer_EnableOptionalOpcodes(void)
{
    return GattTelephoneBearerServerSetControlPointOpcodes(tmapProfile_tbs_data.service_handle, TBS_CCP_OPTIONAL_ALL);
}

bool tmapProfileTbsServer_SetProviderName(uint8 name_len, char *provider_name)
{
    bool status;

    status = GattTelephoneBearerServerSetProviderName(tmapProfile_tbs_data.service_handle, provider_name, name_len);

    TMAP_TBS_LOG("tmapProfileTbsServer_SetProviderName status %d", status);

    return status;
}

bool tmapProfileTbsServer_SetTechnology(GattTbsTechnology technology)
{
    bool status;

    status = GattTelephoneBearerServerSetTechnology(tmapProfile_tbs_data.service_handle, technology);

    TMAP_TBS_LOG("tmapProfileTbsServer_SetTechnology technology: %d status %d", technology, status);

    return status;
}

bool tmapProfileTbsServer_SetUriPrefixList(uint8 prefix_len, char *prefix)
{
    bool status;

    status = GattTelephoneBearerServerSetUriPrefixList(tmapProfile_tbs_data.service_handle, prefix, prefix_len);

    TMAP_TBS_LOG("tmapProfileTbsServer_SetUriPrefixList status %d", status);

    return status;
}

bool tmapProfileTbsServer_SetStatusFlags(uint16 status_flags)
{
    TMAP_TBS_LOG("tmapProfileTbsServer_SetStatusFlags status_flags : %d", status_flags);

    return GattTelephoneBearerServerSetStatusFlags(tmapProfile_tbs_data.service_handle, status_flags);
}

void tmapProfileTbsServer_SetValidUriFlag(bool is_valid)
{
    tmapProfile_tbs_data.valid_uri_flag = is_valid;
}

/*! \brief Notofy the received call control to all registered clients */
static void tmapProfileTbsServer_NotifyCallControlsToClients(tmap_profile_remote_call_control_msg_t message)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(tmapProfileTbsServer_GetStatusNotifyList()), message);
}

/*! \brief Handles call control point write operations */
static void tmapProfileTbsServer_HandleCallControlPointWriteInd(GattTelephoneBearerServerCallControlPointInd *message)
{
    uint8 opcode, opcode_param_length, *opcode_param;
    TbsCallControlPointNotification* notification = (TbsCallControlPointNotification*) PanicUnlessMalloc(sizeof(TbsCallControlPointNotification));

    opcode = message->controlPoint.opcode;
    opcode_param_length = message->cpLen > 1 ? message->cpLen - 1 : 0;
    opcode_param = opcode_param_length == 0 ? NULL : &message->controlPoint.param1[0];
    notification->resultCode = TBS_CCP_RESULT_OPCODE_NOT_SUPPORTED;

    TMAP_TBS_LOG("tmapProfileTbsServer_HandleCallControlPointWriteInd opcode: %u", opcode);

    switch (opcode)
    {
        case TBS_OPCODE_ACCEPT:
            tmapProfileTbsServer_SetCallState(TBS_CALL_STATE_ACTIVE);
            Telephony_NotifyCallAudioConnected(voice_source_le_audio_unicast_1);
            tmapProfileTbsServer_NotifyCallControlsToClients(TMAP_SERVER_REMOTE_CALL_CONTROL_CALL_ACCEPT);
            notification->resultCode = TBS_CCP_RESULT_SUCCESS;
            break;

        case TBS_OPCODE_TERMINATE:
            Telephony_NotifyCallAudioDisconnected(voice_source_le_audio_unicast_1);
            tmapProfileTbsServer_NotifyCallControlsToClients(TMAP_SERVER_REMOTE_CALL_CONTROL_CALL_TERMINATE);
            notification->resultCode = TBS_CCP_RESULT_SUCCESS;

            if (tmapServer_IsPtsModeEnabled())
            {
                /* Incase in PTS mode, terminate the call immediately. In non-PTS scenario this is done
                   when TMAP_SERVER_REMOTE_CALL_CONTROL_CALL_TERMINATE propagated and call eventually terminated. */
                tmapProfileTbsServer_TerminateCall(TBS_CALL_TERMINATION_CLIENT_TERMINATED);
            }
            break;

        case TBS_OPCODE_ORIGINATE:
            if (tmapServer_IsPtsModeEnabled())
            {
                /* Create an outgoing call with the received target URI */
                notification->resultCode = tmapProfileTbsServer_CreateCall(TBS_CALL_STATE_DIALING, TBS_CALL_FLAG_CALL_DIRECTION_OUTGOING,
                                                                           opcode_param_length, (char*)opcode_param);
            }
            break;

        default:
            TMAP_TBS_LOG("tmapProfileTbsServer_HandleCallControlPointWriteInd Unhandled opcode : 0x%x", opcode);
            break;
    }

    notification->callId = tmapProfile_tbs_data.call_id;
    notification->opcode = opcode;
    PanicFalse(GattTelephoneBearerServerCallControlPointResponse(tmapProfile_tbs_data.service_handle, notification));
    pfree(notification);
}

void tmapProfileTbsServer_ProcessTbsServerMessage(Message message)
{
    GattTelephoneBearerServerMessageId tbs_msg_id = *(GattTelephoneBearerServerMessageId*)message;

    switch (tbs_msg_id)
    {
        case GATT_TELEPHONE_BEARER_SERVER_CALL_CONTROL_POINT_IND:
            tmapProfileTbsServer_HandleCallControlPointWriteInd((GattTelephoneBearerServerCallControlPointInd*)message);
        break;

        default:
            TMAP_TBS_LOG("tmapProfileTbsServer_ProcessTbsServerMessage Unhandled message:0x%x", tbs_msg_id);
        break;
    }
}

void tmapProfileTbsServer_EnablePtsMode(bool enable)
{
    if (enable)
    {
        tmapProfileTbsServer_EnableOptionalOpcodes();
    }
}

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
