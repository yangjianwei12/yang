/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup le_unicast_manager
    \brief      Implementation of the telephony control interface for LE unicast voice sources.
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST)

#include "le_unicast_manager_instance.h"
#include "le_unicast_telephony_control.h"
#include "le_unicast_manager_private.h"
#include "voice_sources.h"
#include "voice_sources_telephony_control_interface.h"

#ifdef USE_SYNERGY
#include "call_control_client.h"
#include "micp_server.h"
#endif
#include <logging.h>

#ifdef USE_SYNERGY
static void leVoiceSource_IncomingCallAccept(voice_source_t source);
static void leVoiceSource_IncomingCallReject(voice_source_t source);
static void leVoiceSource_OngoingCallTerminate(voice_source_t source);
static void leVoiceSource_TwcControl(voice_source_t source, voice_source_twc_control_t action);
static void leVoiceSource_RefreshCallState(voice_source_t source);
static device_t leVoiceSource_GetDevice(voice_source_t source);
static void leVoiceSource_ToggleMicMute(voice_source_t source);

#else
#define leVoiceSource_IncomingCallAccept               NULL
#define leVoiceSource_IncomingCallReject               NULL
#define leVoiceSource_OngoingCallTerminate             NULL
#define leVoiceSource_TwcControl                       NULL
#define leVoiceSource_RefreshCallState                 NULL
#define leVoiceSource_GetDevice                        NULL
#define leVoiceSource_ToggleMicMute                    NULL
#endif

static unsigned leVoiceSource_GetCurrentContext(voice_source_t source);

static const voice_source_telephony_control_interface_t le_voice_source_telephony_control =
{
    .IncomingCallAccept = leVoiceSource_IncomingCallAccept,
    .IncomingCallReject = leVoiceSource_IncomingCallReject,
    .OngoingCallTerminate = leVoiceSource_OngoingCallTerminate,
    .OngoingCallTransferAudio = NULL,               /* CCP doesn't have support */
    .InitiateCallUsingNumber = NULL,
    .InitiateVoiceDial = NULL,                      /* @Todo : Needs to be implemented once requirement comes */
    .InitiateCallLastDialled = NULL,                /* CCP doesn't have support */
    .ToggleMicrophoneMute = leVoiceSource_ToggleMicMute,
    .GetUiProviderContext = leVoiceSource_GetCurrentContext,
    .TwcControl = leVoiceSource_TwcControl,
    .RefreshCallState = leVoiceSource_RefreshCallState,
    .GetDevice = leVoiceSource_GetDevice,
};

#ifdef USE_SYNERGY
static void leVoiceSource_IncomingCallAccept(voice_source_t source)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByVoiceSource(source);

    if (inst)
    {
        CallControlClient_SendCallControlOpcode(inst->cid, TBS_CCP_ACCEPT, 0);
        UNICAST_MANAGER_LOG("leVoiceSource_IncomingCallAccept: source=enum:voice_source_t:%d, cid=0x%x", source, inst->cid);
    }
}

static void leVoiceSource_IncomingCallReject(voice_source_t source)
{
    CallControlClient_TerminateCalls(source, CCP_REJECT_INCOMING_CALL);
    UNICAST_MANAGER_LOG("leVoiceSource_IncomingCallReject: source=enum:voice_source_t:%d", source);
}

static void leVoiceSource_OngoingCallTerminate(voice_source_t source)
{
    CallControlClient_TerminateCalls(source, CCP_HANGUP_ONGOING_CALLS);
    UNICAST_MANAGER_LOG("leVoiceSource_OngoingCallTerminate: source=enum:voice_source_t:%d", source);
}

static void leVoiceSource_TwcControl(voice_source_t source, voice_source_twc_control_t action)
{
    DEBUG_LOG("leVoiceSource_TwcControl: source=enum:voice_source_t:%d, action=enum:voice_source_twc_control_t:%d", source, action);

    switch (action)
    {
        case voice_source_release_held_reject_waiting:
            CallControlClient_TwcReleaseHeldRejectWaiting(source);
        break;

        case voice_source_release_active_accept_other:
            CallControlClient_TwcReleaseActiveAcceptOther(source);
        break;

        case voice_source_hold_active_accept_other:
            CallControlClient_TwcHoldActiveAcceptOther(source);
        break;

        case voice_source_add_held_to_multiparty:
        case voice_source_join_calls_and_hang_up: /* Fall through*/
            /* Todo As of now CCP is limited to joining the call only and we cannot hangup.
               For now these two control will be joining all the call in the call control client instance.
            */
            CallControlClient_TwcJoinCalls(source);
        break;

        default:
            Panic();
            break;
    }
}

static device_t leVoiceSource_GetDevice(voice_source_t source)
{
    return CallControlClient_FindDeviceFromVoiceSource(source);
}

static void leVoiceSource_RefreshCallState(voice_source_t source)
{
    CallControlClient_RefreshCallState(source);
    UNICAST_MANAGER_LOG("leVoiceSource_RefreshCallState: source=enum:voice_source_t:%d", source);
}

static void leVoiceSource_ToggleMicMute(voice_source_t source)
{
    UNICAST_MANAGER_LOG("leVoiceSource_ToggleMicMute: source=enum:voice_source_t:%d", source);
    if (source == voice_source_le_audio_unicast_1)
    {
        MicpServer_ToggleMicMute();
    }
}

#endif

static unsigned leVoiceSource_GetCurrentContext(voice_source_t source)
{
    voice_source_provider_context_t le_voice_context = context_voice_disconnected;

    if (source == voice_source_le_audio_unicast_1)
    {
        le_voice_context = CallClientControl_GetContext(source);
    }

    UNICAST_MANAGER_LOG("leVoiceSource_GetCurrentContext source enum:voice_source_t:%u enum:voice_source_provider_context_t:%d",
                        source, le_voice_context);

    return (unsigned) le_voice_context;
}

const voice_source_telephony_control_interface_t * LeVoiceSource_GetTelephonyControlInterface(void)
{
    return &le_voice_source_telephony_control;
}

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) */
