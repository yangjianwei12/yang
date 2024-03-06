/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the audio source interface for LE Audio Client broadcast audio sources.
*/

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
#include "le_audio_client_broadcast_audio_source.h"
#include "le_audio_client_context.h"
#include "audio_sources.h"
#include "audio_sources_audio_interface.h"
#include "bt_device.h"
#include "kymera.h"
#include "kymera_adaptation_audio_protected.h"
#include "ui.h"
#include "volume_types.h"

#include <logging.h>
#include <panic.h>
#include <stdlib.h>

static bool leAudioClientBroadcast_GetAudioConnectParameters(audio_source_t source,
                                                             source_defined_params_t *source_params);
static void leAudioClientBroadcast_FreeAudioConnectParameters(audio_source_t source,
                                                              source_defined_params_t *source_params);
static bool leAudioClientBroadcast_GetAudioDisconnectParameters(audio_source_t source,
                                                                source_defined_params_t *source_params);
static void leAudioClientBroadcast_FreeAudioDisconnectParameters(audio_source_t source,
                                                                 source_defined_params_t *source_params);
static bool leAudioClientBroadcast_IsAudioRouted(audio_source_t source);
static source_status_t leAudioClientBroadcast_SetState(audio_source_t source, source_state_t state);
static device_t leAudioClientBroadcast_GetAudioSourceDevice(audio_source_t source);

static const audio_source_audio_interface_t broadcast_audio_source_interface =
{
    .GetConnectParameters = leAudioClientBroadcast_GetAudioConnectParameters,
    .ReleaseConnectParameters = leAudioClientBroadcast_FreeAudioConnectParameters,
    .GetDisconnectParameters = leAudioClientBroadcast_GetAudioDisconnectParameters,
    .ReleaseDisconnectParameters = leAudioClientBroadcast_FreeAudioDisconnectParameters,
    .IsAudioRouted = leAudioClientBroadcast_IsAudioRouted,
    .IsAudioAvailable = NULL,
    .SetState = leAudioClientBroadcast_SetState,
    .Device = leAudioClientBroadcast_GetAudioSourceDevice
};

static appKymeraLeStreamType leAudioClientBroadcast_GetStreamType(TmapClientBigSubGroup *subgroup_info)
{
    appKymeraLeStreamType stream_type;

    if (subgroup_info->numBis == 2)
    {
        stream_type = KYMERA_LE_STREAM_DUAL_MONO;
    }
    else
    {
        if (subgroup_info->bisInfo[0].audioLocation == BAP_AUDIO_LOCATION_MONO)
        {
            stream_type = KYMERA_LE_STREAM_MONO;
        }
        else
        {
            stream_type = KYMERA_LE_STREAM_STEREO_USE_BOTH;
        }
    }

    return stream_type;
}
static void leAudioClientBroadcast_ExtractAudioParameters(le_audio_connect_parameters_t *conn_param)
{
    le_audio_client_context_t* client_ctxt = leAudioClient_GetContext();
    le_audio_broadcast_session_data_t *session_data = &client_ctxt->broadcast_session_data;
    uint32 transport_latency_big = session_data->transport_latency_big;

    conn_param->media.stream_type = leAudioClientBroadcast_GetStreamType(session_data->audio_config->sub_group_info);
    conn_param->media.codec_type = KYMERA_LE_AUDIO_CODEC_LC3;
    conn_param->media.codec_version = 0x00;
    conn_param->media_present = TRUE;
    conn_param->media.start_muted = FALSE;
    conn_param->microphone_present = FALSE;
    conn_param->media.codec_frame_blocks_per_sdu = session_data->audio_config->sub_group_info->lc3BlocksPerSdu;
    conn_param->media.sample_rate = leAudioClientBroadcast_GetSampleRate(session_data->sampling_frequency);
    conn_param->media.frame_length = leAudioClient_GetFrameLength(session_data->octets_per_frame,
                                                                  conn_param->media.stream_type, conn_param->media.codec_type);
    conn_param->media.frame_duration = leAudioClient_GetFrameDuration(FALSE, LE_AUDIO_CLIENT_MODE_BROADCAST, conn_param->media.stream_type);
    conn_param->media.source_iso_handle = session_data->bis_handles[0];
    conn_param->media.source_iso_handle_right = session_data->bis_handles[1];
    conn_param->media.volume = AudioSources_CalculateOutputVolume(audio_source_le_audio_broadcast_sender);
    conn_param->media.presentation_delay = session_data->audio_config->presentation_delay;
#if defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER)
    /* if iso interval is 30ms, we need a workaround temporarily to add frame duration to transport latency.
       this is required for sync with broadcast receivers for supporting concurrency use-cases.
       iso interval is calculated in steps of 1.25, so 30ms = 24*1.25 */
    conn_param->media.transport_latency_big = (session_data->iso_interval != 24) ? transport_latency_big : transport_latency_big + session_data->frame_duration;
#endif

    DEBUG_LOG_INFO("leAudioClientBroadcast_ExtractAudioParameters sample rate %d, frame_length :%d, Frame duration %d, Frame Blocks Per SDU %d codec_type %d codec_version %d Stream Type %d BIG Transport Latency %d",
                    conn_param->media.sample_rate,
                    conn_param->media.frame_length,
                    conn_param->media.frame_duration,
                    conn_param->media.codec_frame_blocks_per_sdu,
                    conn_param->media.codec_type,
                    conn_param->media.codec_version,
                    conn_param->media.stream_type,
                    transport_latency_big);
}

/* \brief Get Audio Connect parameters */
static bool leAudioClientBroadcast_GetAudioConnectParameters(audio_source_t source,
                                                             source_defined_params_t *source_params)
{
    bool populate_success = FALSE;

    if (source == audio_source_le_audio_broadcast_sender && LeAudioClient_IsBroadcastSourceStreamingActive())
    {
        source_params->data = PanicUnlessMalloc(sizeof(le_audio_connect_parameters_t));
        source_params->data_length = sizeof(le_audio_connect_parameters_t);
        memset(source_params->data, 0, sizeof(le_audio_connect_parameters_t));

        leAudioClientBroadcast_ExtractAudioParameters((le_audio_connect_parameters_t *) source_params->data);
        populate_success = TRUE;
    }

    DEBUG_LOG_INFO("leAudioClientBroadcast_GetAudioConnectParameters for enum:audio_source_t:%d, status: %d", source, populate_success);

    return populate_success;
}

static void leAudioClientBroadcast_FreeAudioConnectParameters(audio_source_t source,
                                                              source_defined_params_t *source_params)
{
    if (source == audio_source_le_audio_broadcast_sender)
    {
        PanicNull(source_params);
        PanicFalse(source_params->data_length == sizeof(le_audio_connect_parameters_t));

        free(source_params->data);
        source_params->data = (void *) NULL;
        source_params->data_length = 0;
    }
}

static bool leAudioClientBroadcast_GetAudioDisconnectParameters(audio_source_t source,
                                                                source_defined_params_t *source_params)
{
    UNUSED(source_params);
    UNUSED(source);
    return TRUE;
}

static void leAudioClientBroadcast_FreeAudioDisconnectParameters(audio_source_t source,
                                                                 source_defined_params_t *source_params)
{
    UNUSED(source_params);
    UNUSED(source);
}

static bool leAudioClientBroadcast_IsAudioRouted(audio_source_t source)
{
    /* Todo */
    UNUSED(source);
    return FALSE;
}

static source_status_t leAudioClientBroadcast_SetState(audio_source_t source, source_state_t state)
{
    DEBUG_LOG("leAudioClientBroadcast_SetState source=%d state=%d", source, state);
    /* TODO */
    return source_status_ready;
}

static device_t leAudioClientBroadcast_GetAudioSourceDevice(audio_source_t source)
{
    UNUSED(source);
    return NULL;
}

void leAudioClient_InitBroadcastAudioSource(void)
{
    AudioSources_RegisterAudioInterface(audio_source_le_audio_broadcast_sender, &broadcast_audio_source_interface);
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

