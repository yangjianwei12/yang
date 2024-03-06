/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    mirror_profile
    \brief      Mirror profile LE audio source control.
*/

#ifdef ENABLE_LEA_CIS_DELEGATION

#include "mirror_profile_private.h"
#include "mirror_profile_signalling.h"
#include "mirror_profile_le_audio_source.h"

#include "le_audio_messages.h"
#include "kymera_adaptation_audio_protected.h"
#include "audio_sources.h"
#include "logging.h"
#include "kymera.h"
#include "kymera_adaptation.h"
#include <qualcomm_connection_manager.h>

#include <panic.h>
#include <stream.h>
#include <stdlib.h>
#include <sink.h>

static bool mirrorProfile_GetLeAudioConnectParameters(audio_source_t source, source_defined_params_t *source_params);
static void mirrorProfile_FreeLeAudioConnectParameters(audio_source_t source, source_defined_params_t *source_params);
static bool mirrorProfile_GetLeAudioDisconnectParameters(audio_source_t source, source_defined_params_t *source_params);
static void mirrorProfile_FreeLeAudioDisconnectParameters(audio_source_t source, source_defined_params_t *source_params);
static bool mirrorProfile_IsLeAudioRouted(audio_source_t source);
static source_status_t mirrorProfile_LeAudioSourceSetState(audio_source_t source, source_state_t state);
static unsigned mirrorProfile_GetLeAudioContext(audio_source_t source);

static const audio_source_audio_interface_t mirror_le_audio_interface =
{
    .GetConnectParameters = mirrorProfile_GetLeAudioConnectParameters,
    .ReleaseConnectParameters = mirrorProfile_FreeLeAudioConnectParameters,
    .GetDisconnectParameters = mirrorProfile_GetLeAudioDisconnectParameters,
    .ReleaseDisconnectParameters = mirrorProfile_FreeLeAudioDisconnectParameters,
    .IsAudioRouted = mirrorProfile_IsLeAudioRouted,
    .IsAudioAvailable = NULL,
    .SetState = mirrorProfile_LeAudioSourceSetState,
    .GetState = NULL,
    .Device = NULL,
};

static const media_control_interface_t mirror_le_media_control_interface = {
    .Context = mirrorProfile_GetLeAudioContext,
};

static void mirrorProfile_FreeLeAudioConnectParameters(audio_source_t source, source_defined_params_t *source_params)
{
    if (source == audio_source_le_audio_unicast_1)
    {
        PanicNull(source_params);
        PanicFalse(source_params->data_length == sizeof(le_audio_connect_parameters_t));
        free(source_params->data);
        source_params->data = (void *) NULL;
        source_params->data_length = 0;
    }
}

static bool mirrorProfile_GetLeAudioDisconnectParameters(audio_source_t source, source_defined_params_t *source_params)
{
    UNUSED(source);

    source_params->data = (void *) NULL;
    source_params->data_length = 0;

    return TRUE;
}

static void mirrorProfile_FreeLeAudioDisconnectParameters(audio_source_t source,
                                                          source_defined_params_t *source_params)
{
    UNUSED(source_params);
    UNUSED(source);
}

static appKymeraLeStreamType mirrorProfile_GetStreamType(mirror_profile_cis_type cis_type)
{
    appKymeraLeStreamType stream_type = KYMERA_LE_STREAM_MONO;

    if (cis_type == mirror_profile_cis_type_both_render_left || cis_type == mirror_profile_cis_type_both_render_right)
    {
        stream_type = (cis_type == mirror_profile_cis_type_both_render_left ?
                                  KYMERA_LE_STREAM_STEREO_USE_LEFT :
                                  KYMERA_LE_STREAM_STEREO_USE_RIGHT);
    }

    return stream_type;
}

static bool mirrorProfile_ExtractLeAudioParameters(le_audio_connect_parameters_t *conn_param)
{
    bool result = FALSE;
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();

    conn_param->media_present = FALSE;
    conn_param->microphone_present = FALSE;

    if (lea_unicast->audio_config.audio_source != audio_source_none)
    {
        if (lea_unicast->audio_config.sink_sampling_frequency != 0)
        {
            conn_param->media_present = TRUE;
            conn_param->media.source_iso_handle = MirrorProfile_GetIsoHandleFromMirrorType(LE_AUDIO_ISO_DIRECTION_DL);
            conn_param->media.source_iso_handle_right = LE_AUDIO_INVALID_ISO_HANDLE;
            conn_param->media.volume = AudioSources_CalculateOutputVolume(audio_source_le_audio_unicast_1);
            conn_param->media.sample_rate = lea_unicast->audio_config.sink_sampling_frequency;
            conn_param->media.frame_length = lea_unicast->audio_config.sink_frame_length;
            conn_param->media.frame_duration = lea_unicast->audio_config.sink_frame_duration;
            conn_param->media.presentation_delay = lea_unicast->audio_config.sink_presentation_delay;
            conn_param->media.codec_type = lea_unicast->audio_config.sink_codec_id;
            conn_param->media.stream_type = mirrorProfile_GetStreamType(lea_unicast->peer_cis_type);
            conn_param->media.codec_version = lea_unicast->audio_config.codec_version;
            conn_param->media.codec_frame_blocks_per_sdu  = lea_unicast->audio_config.sink_blocks_per_sdu;
            conn_param->media.gaming_mode = lea_unicast->audio_config.sink_gaming_mode;
            result = TRUE;
        }

        if (lea_unicast->audio_config.source_sampling_frequency != 0)
        {
            conn_param->microphone_present = TRUE;
            conn_param->microphone.source_iso_handle = MirrorProfile_GetIsoHandleFromMirrorType(LE_AUDIO_ISO_DIRECTION_UL);
            conn_param->microphone.sample_rate = lea_unicast->audio_config.source_sampling_frequency;
            conn_param->microphone.frame_length = lea_unicast->audio_config.source_frame_length;
            conn_param->microphone.frame_duration = lea_unicast->audio_config.source_frame_duration;
            conn_param->microphone.presentation_delay = lea_unicast->audio_config.source_presentation_delay;
            conn_param->microphone.codec_type = lea_unicast->audio_config.source_codec_id;
            conn_param->microphone.codec_version = lea_unicast->audio_config.codec_version;
            conn_param->microphone.codec_frame_blocks_per_sdu  = lea_unicast->audio_config.source_blocks_per_sdu;
            conn_param->microphone.mic_sync = lea_unicast->audio_config.mic_sync;
            result = TRUE;
        }

        if (conn_param->microphone_present && conn_param->microphone.mic_sync)
        {
            /* Pass call back for mic start in case LE mic chain is present */
            conn_param->microphone.started_handler = MirrorProfile_HandleKymeraLeaMicStarted;
        }
    }

    DEBUG_LOG("mirrorProfile_ExtractLeAudioParameters media_present=%d, microphone_present=%d",
              conn_param->media_present, conn_param->microphone_present);

    return  result;
}

static bool mirrorProfile_GetLeAudioConnectParameters(audio_source_t source, source_defined_params_t *source_params)
{
    bool populate_success = FALSE;

    DEBUG_LOG("mirrorProfile_GetLeAudioConnectParameters source=enum:audio_source_t:%d", source);

    if (source == audio_source_le_audio_unicast_1)
    {
        le_audio_connect_parameters_t *conn_param = PanicUnlessNew(le_audio_connect_parameters_t);

        memset(conn_param, 0, sizeof(le_audio_connect_parameters_t));

        if (mirrorProfile_ExtractLeAudioParameters(conn_param))
        {
            source_params->data = (void *)conn_param;
            source_params->data_length = sizeof(le_audio_connect_parameters_t);
            populate_success = TRUE;
        }
        else
        {
            /* Free the allocated audio connect parameter */
            free(conn_param);
        }
    }

    DEBUG_LOG("mirrorProfile_GetLeAudioConnectParameters status=%d", populate_success);

    return populate_success;
}

static bool mirrorProfile_IsLeAudioRouted(audio_source_t source)
{
    bool is_available = FALSE;
    audio_source_t audio_source = MirrorProfile_GetLeAudioSource();
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();

    DEBUG_LOG("mirrorProfile_IsLeAudioRouted req_src=enum:audio_source_t:%d, avail_src=enum:audio_source_t:%d, Mirror state=enum:mirror_profile_state_t:%d",
              source, audio_source, MirrorProfile_GetState());

    if(audio_source == source && MirrorProfile_IsSecondary())
    {
        switch (MirrorProfile_GetState())
        {
            case MIRROR_PROFILE_STATE_CIS_CONNECTED:
            {
                /* Return TRUE under following conditions:
                 * 1) When own CIS is connected.(OR)
                 * 2) The delegation type is "Mirror" and peer CIS is connected.
                 */
                is_available = (lea_unicast->own_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED ||
                                (lea_unicast->audio_config.mirror_type == le_um_cis_mirror_type_mirror &&
                                lea_unicast->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED));
            }
            break;

            default:
            break;
        }
    }
    return is_available;
}

static unsigned mirrorProfile_GetLeAudioContext(audio_source_t source)
{
    audio_source_provider_context_t context = context_audio_disconnected;


    if (mirrorProfile_IsLeAudioRouted(source))
    {
        context = context_audio_is_playing;
    }

    return (unsigned)context;
}

static source_status_t mirrorProfile_LeAudioSourceSetState(audio_source_t source, source_state_t state)
{
    DEBUG_LOG("mirrorProfile_LeAudioSourceSetState source=%d state=%d", source, state);
    /* @TODO */
    return source_status_ready;
}

void MirrorProfile_StartLeAudio(void)
{
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();

    {
        /* Inform Media player to route unicast audio */
        LeAudioMessages_SendUnicastMediaConnected(audio_source_le_audio_unicast_1,
                                                  lea_unicast->audio_config.audio_context);
    }
}

void MirrorProfile_StopLeAudio(void)
{
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();

    {
        /* Inform Media player to stop routing unicast audio */
        LeAudioMessages_SendUnicastMediaDisconnected(audio_source_le_audio_unicast_1,
                                                     lea_unicast->audio_config.audio_context);
    }
}

const audio_source_audio_interface_t * MirrorProfile_GetLeAudioInterface(void)
{
    return &mirror_le_audio_interface;
}

const media_control_interface_t * MirrorProfile_GetLeMediaControlInterface(void)
{
    return &mirror_le_media_control_interface;
}

/*! \brief Function for reconfiguring existing audio graph at secondary */
void MirrorProfile_ReconfigureLeaAudioGraph(void)
{
    bool extract_param_status;
    le_audio_connect_parameters_t le_audio_params;
    connect_parameters_t connect_params =
        {.source = {.type = source_type_audio, .u = {.audio = audio_source_le_audio_unicast_1}},
         .source_params = {sizeof(le_audio_params), &le_audio_params}};

    memset(&le_audio_params, 0, sizeof(le_audio_params));
    le_audio_params.reconfig = TRUE;

    extract_param_status = mirrorProfile_ExtractLeAudioParameters(&le_audio_params);

    if (extract_param_status)
    {
        KymeraAdaptation_Connect(&connect_params);
    }

    MIRROR_LOG("mirrorProfile_ReconfigureLeaAudioGraph status: %d", extract_param_status);
}

#endif /* ENABLE_LEA_CIS_DELEGATION */

