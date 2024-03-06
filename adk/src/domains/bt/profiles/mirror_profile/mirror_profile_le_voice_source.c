/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    mirror_profile
    \brief      The LE voice source interface implementation for Mirror Profile
*/

#ifdef ENABLE_LEA_CIS_DELEGATION

#include "mirror_profile_private.h"
#include "mirror_profile_le_voice_source.h"
#include "source_param_types.h"
#include "voice_sources.h"
#include "telephony_messages.h"
#include "le_audio_messages.h"
#include "kymera.h"
#include "kymera_adaptation.h"
#include <stdlib.h>
#include <panic.h>

static bool mirrorProfile_GetLeVoiceConnectParameters(voice_source_t source, source_defined_params_t * source_params);
static void mirrorProfile_FreeLeVoiceConnectParameters(voice_source_t source, source_defined_params_t * source_params);
static bool mirrorProfile_GetLeVoiceDisconnectParameters(voice_source_t source, source_defined_params_t * source_params);
static void mirrorProfile_FreeLeVoiceDisconnectParameters(voice_source_t source, source_defined_params_t * source_params);
static bool mirrorProfile_IsLeVoiceAvailable(voice_source_t source);
static unsigned mirrorProfile_GetLeVoiceCurrentContext(voice_source_t source);
static bool mirrorProfile_ExtractLeVoiceParameters(le_voice_connect_parameters_t *conn_param);

static const voice_source_audio_interface_t mirror_le_voice_interface =
{
    .GetConnectParameters = mirrorProfile_GetLeVoiceConnectParameters,
    .ReleaseConnectParameters = mirrorProfile_FreeLeVoiceConnectParameters,
    .GetDisconnectParameters = mirrorProfile_GetLeVoiceDisconnectParameters,
    .ReleaseDisconnectParameters = mirrorProfile_FreeLeVoiceDisconnectParameters,
    .IsAudioRouted = mirrorProfile_IsLeVoiceAvailable,
    .IsVoiceChannelAvailable = mirrorProfile_IsLeVoiceAvailable,
    .SetState = NULL,
    .GetState = NULL
};

static const voice_source_telephony_control_interface_t mirror_le_telephony_interface =
{
    .GetUiProviderContext = mirrorProfile_GetLeVoiceCurrentContext
};

static appKymeraLeStreamType mirrorProfile_GetStreamTypeForVoice(mirror_profile_cis_type cis_type)
{
    appKymeraLeStreamType stream_type = KYMERA_LE_STREAM_MONO;

    switch (cis_type)
    {
        case mirror_profile_cis_type_both_render_left:
            stream_type = KYMERA_LE_STREAM_STEREO_USE_LEFT;
        break;

        case mirror_profile_cis_type_both_render_right:
            stream_type = KYMERA_LE_STREAM_STEREO_USE_RIGHT;
        break;

        default:
            stream_type = KYMERA_LE_STREAM_MONO;
        break;
    }

    return stream_type;
}

/* \brief Extract voice connect parameters from stored mirror unicast state Information */
static bool mirrorProfile_ExtractLeVoiceParameters(le_voice_connect_parameters_t *conn_param)
{
    bool populate_success = FALSE;
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();

    /* Fill in the voice Parameters */
    if (lea_unicast->audio_config.voice_source != voice_source_none)
    {
        if (lea_unicast->audio_config.sink_sampling_frequency != 0)
        {
            conn_param->speaker_present = TRUE;
            conn_param->speaker.iso_handle = MirrorProfile_GetIsoHandleFromMirrorType(LE_AUDIO_ISO_DIRECTION_DL);
            conn_param->speaker.iso_handle_right = LE_AUDIO_INVALID_ISO_HANDLE;

            conn_param->volume = VoiceSources_CalculateOutputVolume(voice_source_le_audio_unicast_1);
            conn_param->speaker.sample_rate = lea_unicast->audio_config.sink_sampling_frequency;
            conn_param->speaker.frame_length = lea_unicast->audio_config.sink_frame_length;
            conn_param->speaker.frame_duration = lea_unicast->audio_config.sink_frame_duration;
            conn_param->speaker.presentation_delay = lea_unicast->audio_config.sink_presentation_delay;
            conn_param->speaker.codec_type = lea_unicast->audio_config.sink_codec_id;
            conn_param->speaker.stream_type = mirrorProfile_GetStreamTypeForVoice(lea_unicast->peer_cis_type);
            conn_param->speaker.codec_version = lea_unicast->audio_config.codec_version;
            conn_param->speaker.codec_frame_blocks_per_sdu  = lea_unicast->audio_config.sink_blocks_per_sdu;
            populate_success = TRUE;
        }

        if (lea_unicast->audio_config.source_sampling_frequency != 0)
        {
            conn_param->microphone_present = TRUE;
            conn_param->microphone.source_iso_handle = MirrorProfile_GetIsoHandleFromMirrorType(LE_AUDIO_ISO_DIRECTION_UL);
            conn_param->microphone.source_iso_handle_right = LE_AUDIO_INVALID_ISO_HANDLE;
            conn_param->microphone.sample_rate = lea_unicast->audio_config.source_sampling_frequency;
            conn_param->microphone.frame_length = lea_unicast->audio_config.source_frame_length;
            conn_param->speaker.stream_type = mirrorProfile_GetStreamTypeForVoice(lea_unicast->peer_cis_type);;
            conn_param->microphone.frame_duration = lea_unicast->audio_config.source_frame_duration;
            conn_param->microphone.presentation_delay = lea_unicast->audio_config.source_presentation_delay;
            conn_param->microphone.codec_type = lea_unicast->audio_config.source_codec_id;
            conn_param->microphone.codec_version = lea_unicast->audio_config.codec_version;
            conn_param->microphone.codec_frame_blocks_per_sdu  = lea_unicast->audio_config.source_blocks_per_sdu;
            conn_param->microphone.mic_mute_state = lea_unicast->audio_config.mic_mute_state;
            populate_success = TRUE;
        }
    }

    DEBUG_LOG("mirrorProfile_ExtractLeVoiceParameters speaker_present=%d, microphone_present=%d",
              conn_param->speaker_present, conn_param->microphone_present);

    return  populate_success;
}

static bool mirrorProfile_GetLeVoiceConnectParameters(voice_source_t source, source_defined_params_t * source_params)
{
    bool populate_success = FALSE;

    if (source == voice_source_le_audio_unicast_1)
    {
        le_voice_connect_parameters_t *conn_param =
            (le_voice_connect_parameters_t *) PanicUnlessMalloc(sizeof(le_voice_connect_parameters_t));

        memset(conn_param, 0, sizeof(le_voice_connect_parameters_t));

        if (mirrorProfile_ExtractLeVoiceParameters(conn_param))
        {
            source_params->data = (void *) conn_param;
            source_params->data_length = sizeof(le_voice_connect_parameters_t);
            populate_success = TRUE;
        }
        else
        {
            /* Free the allocated voice connect parameter */
            pfree(conn_param);
        }
    }

    DEBUG_LOG("mirrorProfile_GetLeVoiceConnectParameters success %d", populate_success);

    return populate_success;
}

static void mirrorProfile_FreeLeVoiceConnectParameters(voice_source_t source, source_defined_params_t * source_params)
{
    UNUSED(source);

    pfree(source_params->data);
    source_params->data = (void *) NULL;
    source_params->data_length = 0;
}

static bool mirrorProfile_GetLeVoiceDisconnectParameters(voice_source_t source, source_defined_params_t * source_params)
{
    UNUSED(source);

    PanicNull(source_params);
    source_params->data = (void *) NULL;
    source_params->data_length = 0;

    return TRUE;
}

static void mirrorProfile_FreeLeVoiceDisconnectParameters(voice_source_t source, source_defined_params_t * source_params)
{
    UNUSED(source);

    PanicNull(source_params);
    source_params->data = (void *) NULL;
    source_params->data_length = 0;
}

static bool mirrorProfile_IsLeVoiceAvailable(voice_source_t source)
{
    bool is_available = FALSE;
    voice_source_t voice_source = MirrorProfile_GetLeVoiceSource();

    DEBUG_LOG("mirrorProfile_IsLeVoiceAvailable source=%d Mirror state = %d", source, MirrorProfile_GetState());

    if(voice_source == source && MirrorProfile_IsSecondary())
    {
        switch (MirrorProfile_GetState())
        {
            case MIRROR_PROFILE_STATE_CIS_CONNECTED:
                is_available = (MirrorProfile_GetLeaUnicastState()->own_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED);
#ifdef INCLUDE_CIS_MIRRORING
                if (!is_available)
                {
                    /* In case of Single CIS Mirroring we will be having only one of the CIS either Peer or Own */
                    is_available = (MirrorProfile_GetLeaUnicastState()->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED);
                }
#endif
            break;
            default:
            break;
        }
    }

    return is_available;
}

static unsigned mirrorProfile_GetLeVoiceCurrentContext(voice_source_t source)
{
    voice_source_provider_context_t context = context_voice_disconnected;

    if (mirrorProfile_IsLeVoiceAvailable(source))
    {
        context = context_voice_in_call;
    }
    return context;
}

const voice_source_audio_interface_t * MirrorProfile_GetLeVoiceInterface(void)
{
    return &mirror_le_voice_interface;
}

const voice_source_telephony_control_interface_t * MirrorProfile_GetLeTelephonyControlInterface(void)
{
    return &mirror_le_telephony_interface;
}

void MirrorProfile_StartLeVoice(void)
{
    voice_source_t voice_source = MirrorProfile_GetLeVoiceSource();

    LeAudioMessages_SendUnicastVoiceConnected(voice_source_le_audio_unicast_1);

    Telephony_NotifyCallAudioConnected(voice_source);
}

void MirrorProfile_StopLeVoice(void)
{
    voice_source_t voice_source = MirrorProfile_GetLeVoiceSource();

    LeAudioMessages_SendUnicastVoiceDisconnected(voice_source_le_audio_unicast_1);

    Telephony_NotifyCallAudioDisconnected(voice_source);
}

/*! \brief Handler for reconfiguring existing voice graph at secondary */
void MirrorProfile_ReconfigureLeVoiceGraph(void)
{
    bool extract_param_status;

    le_voice_connect_parameters_t le_voice_params;
    connect_parameters_t connect_params =
        {.source = {.type = source_type_voice, .u = {.voice = voice_source_le_audio_unicast_1}},
         .source_params = {sizeof(le_voice_params), &le_voice_params}};

    memset(&le_voice_params, 0, sizeof(le_voice_params));
    le_voice_params.reconfig = TRUE;

    extract_param_status = mirrorProfile_ExtractLeVoiceParameters(&le_voice_params);

    if (extract_param_status)
    {
        KymeraAdaptation_Connect(&connect_params);
    }

    MIRROR_LOG("MirrorProfile_ReconfigureLeVoiceGraph status: %d", extract_param_status);
}

#endif /* ENABLE_LEA_CIS_DELEGATION */

