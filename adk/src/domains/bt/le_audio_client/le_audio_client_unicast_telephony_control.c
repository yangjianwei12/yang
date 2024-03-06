/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the telephony control interface for LE unicast voice source.
*/

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
#include "le_audio_client_unicast_telephony_control.h"
#include "le_audio_client_context.h"

#include "voice_sources.h"
#include "voice_sources_telephony_control_interface.h"
#include "tmap_server_role.h"
#include "tmap_profile_mcs_tbs_private.h"

#include "logging.h"
#include "panic.h"
#include "ui.h"

static void leAudioClient_IncomingCallAccept(voice_source_t source);
static void leAudioClient_IncomingCallReject(voice_source_t source);
static void leAudioClient_OngoingCallTerminate(voice_source_t source);
static void leAudioClient_ToggleMicMute(voice_source_t source);
static unsigned leAudioClient_GetCurrentContext(voice_source_t source);

static const voice_source_telephony_control_interface_t le_voice_telephony_interface =
{
    .IncomingCallAccept = leAudioClient_IncomingCallAccept,
    .IncomingCallReject = leAudioClient_IncomingCallReject,
    .OngoingCallTerminate = leAudioClient_OngoingCallTerminate,
    .OngoingCallTransferAudio = NULL,
    .InitiateCallUsingNumber = NULL,
    .InitiateVoiceDial = NULL,
    .InitiateCallLastDialled = NULL,
    .ToggleMicrophoneMute = leAudioClient_ToggleMicMute,
    .GetUiProviderContext = leAudioClient_GetCurrentContext
};

static voice_source_provider_context_t leAudioClient_GetVoiceContextFromCallState(void)
{
    voice_source_provider_context_t voice_context = BAD_CONTEXT;

    switch (LeTmapServer_GetCallState())
    {
        case TBS_CALL_STATE_INCOMING:
            voice_context = context_voice_ringing_incoming;
            break;
    
        case TBS_CALL_STATE_ALERTING:
        case TBS_CALL_STATE_DIALING:
            voice_context = context_voice_ringing_outgoing;
            break;
    
        case TBS_CALL_STATE_ACTIVE:
            voice_context = context_voice_in_call;
            break;
    
        case TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD:
        case TBS_CALL_STATE_REMOTELY_HELD:
        case TBS_CALL_STATE_LOCALLY_HELD:
            voice_context = context_voice_call_held;
            break;
    
        default:
            voice_context = context_voice_connected;
            break;
    }

    return voice_context;
}

static unsigned leAudioClient_GetCurrentContext(voice_source_t source)
{
    voice_source_provider_context_t context = BAD_CONTEXT;
    le_audio_client_context_t* client_ctxt = leAudioClient_GetContext();
    CapClientContext audio_context = CAP_CLIENT_CONTEXT_TYPE_PROHIBITED;

    UNUSED(source);

    switch (LeAudioClient_GetState())
    {
        case LE_AUDIO_CLIENT_STATE_CONNECTED:
        {
            /* TODO check the active/configured context to be that of Voice ?*/
            context = context_voice_connected;

            if (leAudioClient_IsInUnicastStreaming())
            {
                audio_context = client_ctxt->session_data.audio_context;

                if (audio_context == CAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL)
                {
                    context = leAudioClient_GetVoiceContextFromCallState();
                }
            }
        }
        break;

        default:
        {
            context = context_voice_disconnected;
        }
        break;
    }

    DEBUG_LOG("leAudioClient_GetCurrentContext Current CAP audio context : %x, UI voice Context : %d", audio_context, context);
    return context;
}

static void leAudioClient_IncomingCallAccept(voice_source_t source)
{
    UNUSED(source);

    DEBUG_LOG("leAudioClient_IncomingCallAccept");

    if (LeTmapServer_IsCallPresent() && LeTmapServer_GetCallState() == TBS_CALL_STATE_INCOMING)
    {
        LeTmapServer_SetCallState(TBS_CALL_STATE_ACTIVE);
    }
}

static void leAudioClient_IncomingCallReject(voice_source_t source)
{
    /* Rejecting is only needs to be done if call is present but not active.
       If call is active, call termination should be done through leAudioClient_OngoingCallTerminate */
    if (LeTmapServer_IsCallPresent() && !LeTmapServer_IsCallInActiveState())
    {
        LeTmapServer_TerminateCall(TBS_CALL_TERMINATION_UNSPECIFIED);
    }

    DEBUG_LOG("leAudioClient_IncomingCallReject: source : %u", source);
}

static void leAudioClient_OngoingCallTerminate(voice_source_t source)
{
    if (LeTmapServer_IsCallPresent())
    {
        LeTmapServer_TerminateCall(TBS_CALL_TERMINATION_UNSPECIFIED);
    }

    DEBUG_LOG("leAudioClient_OngoingCallTerminate: source : %u", source);
}

static void leAudioClient_ToggleMicMute(voice_source_t source)
{
    /* TODO */
    UNUSED(source);
}

const voice_source_telephony_control_interface_t * LeAudioClient_GetTelephonyControlInterface(void)
{
    return &le_voice_telephony_interface;
}

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
