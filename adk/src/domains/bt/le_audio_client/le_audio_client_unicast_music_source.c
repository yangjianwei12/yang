/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the audio source interface for LE Audio Client music sources.
*/

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
#include "le_audio_client_unicast_music_source.h"
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

static bool leAudioClient_GetAudioConnectParameters(audio_source_t source, source_defined_params_t *source_params);
static void leAudioClient_FreeAudioConnectParameters(audio_source_t source, source_defined_params_t *source_params);
static bool leAudioClient_GetAudioDisconnectParameters(audio_source_t source, source_defined_params_t *source_params);
static void leAudioClient_FreeAudioDisconnectParameters(audio_source_t source, source_defined_params_t *source_params);
static bool leAudioClient_IsAudioRouted(audio_source_t source);
static source_status_t leAudioClient_SetState(audio_source_t source, source_state_t state);
static device_t leAudioClient_GetAudioSourceDevice(audio_source_t source);

static const audio_source_audio_interface_t music_source_audio_interface =
{
    .GetConnectParameters = leAudioClient_GetAudioConnectParameters,
    .ReleaseConnectParameters = leAudioClient_FreeAudioConnectParameters,
    .GetDisconnectParameters = leAudioClient_GetAudioDisconnectParameters,
    .ReleaseDisconnectParameters = leAudioClient_FreeAudioDisconnectParameters,
    .IsAudioRouted = leAudioClient_IsAudioRouted,
    .IsAudioAvailable = NULL,
    .SetState = leAudioClient_SetState,
    .Device = leAudioClient_GetAudioSourceDevice
};

static bool leAudioClient_ExtractAudioParameters(le_audio_connect_parameters_t *conn_param)
{
    le_audio_client_context_t* client_ctxt = leAudioClient_GetContext();
    le_audio_unicast_session_data_t *unicast_session_data = &client_ctxt->session_data;
    
    DEBUG_LOG_INFO("leAudioClient_ExtractAudioParameters");
    
    conn_param->media_present = TRUE;
    conn_param->microphone_present = FALSE;
    conn_param->media_present = TRUE;
    conn_param->media.volume = AudioSources_CalculateOutputVolume(audio_source_le_audio_unicast_sender);
    conn_param->media.source_iso_handle = leAudioClient_GetSinkCisHandleForAudioLocation(BAP_AUDIO_LOCATION_FL);

    /* Get the iso handles and stream type to use for media path */
    conn_param->media.stream_type = leAudioClient_GetUnicastIsoHandles(LE_CIS_DIRECTION_DL,
                                                                       &conn_param->media.source_iso_handle,
                                                                       &conn_param->media.source_iso_handle_right);

    conn_param->media.codec_frame_blocks_per_sdu = unicast_session_data->codec_qos_config.sinkLc3BlocksPerSdu;
    conn_param->media.codec_type = leAudioClient_GetCodecType(unicast_session_data->codec_qos_config);
    conn_param->media.frame_length = leAudioClient_GetFrameLength(unicast_session_data->codec_qos_config.sinkOctetsPerFrame, conn_param->media.stream_type, conn_param->media.codec_type);
    conn_param->media.presentation_delay = 0; /* Ignored */
    conn_param->media.sample_rate = leAudioClient_GetSampleRate(unicast_session_data->audio_config->sink_stream_capability);
    conn_param->media.frame_duration = leAudioClient_GetFrameDuration(FALSE, LE_AUDIO_CLIENT_MODE_UNICAST, conn_param->media.stream_type);
    conn_param->media.codec_version = unicast_session_data->codec_qos_config.codecVersionNum;

    DEBUG_LOG_INFO("MEDIA: Sample Rate %d, frame_length :%d, Frame duration %d, Frame Blocks Per SDU %d enum:appKymeraLeAudioCodec:%d codec_version %d Stream Type %d",
                        conn_param->media.sample_rate,
                        conn_param->media.frame_length,
                        conn_param->media.frame_duration,
                        conn_param->media.codec_frame_blocks_per_sdu,
                        conn_param->media.codec_type,
                        conn_param->media.codec_version,
                        conn_param->media.stream_type);

    if (unicast_session_data->enable_vbc)
    {
        conn_param->microphone_present = TRUE;
        conn_param->microphone.codec_frame_blocks_per_sdu = 1;
        conn_param->microphone.frame_duration = leAudioClient_GetFrameDuration(TRUE, LE_AUDIO_CLIENT_MODE_UNICAST, conn_param->media.stream_type);

        /* Get the iso handles to use for MIC path */
        (void) leAudioClient_GetUnicastIsoHandles(LE_CIS_DIRECTION_UL, &conn_param->microphone.source_iso_handle,
                                                  &conn_param->microphone.source_iso_handle_right);

        conn_param->microphone.frame_length= unicast_session_data->codec_qos_config.srcOctetsPerFrame;
        conn_param->microphone.presentation_delay = 40000; /* TODO : Need this info from CAP client */
        conn_param->microphone.sample_rate = leAudioClient_GetSampleRate(unicast_session_data->audio_config->source_stream_capability);
        conn_param->microphone.codec_type = leAudioClient_GetCodecType(unicast_session_data->codec_qos_config);
        conn_param->microphone.codec_version = unicast_session_data->codec_qos_config.codecVersionNum;
        DEBUG_LOG_INFO("MICROPHONE: Sample Rate %d, frame_length :%d, Presentation delay: %d, Frame duration %d, Frame Blocks Per SDU %d",
                        conn_param->microphone.sample_rate,
                        conn_param->microphone.frame_length,
                        conn_param->microphone.presentation_delay,
                        conn_param->microphone.frame_duration,
                        conn_param->microphone.codec_frame_blocks_per_sdu);
    }

    return TRUE;
}

/* \brief Get Audio Connect parameters */
static bool leAudioClient_GetAudioConnectParameters(audio_source_t source, source_defined_params_t *source_params)
{
    DEBUG_LOG_INFO("leAudioClient_GetAudioConnectParameters");

    bool populate_success = FALSE;
    le_audio_connect_parameters_t *conn_param = (le_audio_connect_parameters_t*)PanicUnlessMalloc(sizeof(le_audio_connect_parameters_t));
    memset(conn_param, 0, sizeof(le_audio_connect_parameters_t));

    PanicFalse(leAudioClient_IsInUnicastStreaming());

    if (source == audio_source_le_audio_unicast_sender)
    {
        if (leAudioClient_ExtractAudioParameters(conn_param))
        {
            source_params->data = (void *)conn_param;
            source_params->data_length = sizeof(le_audio_connect_parameters_t);
            populate_success = TRUE;

            if (conn_param->microphone_present)
            {
                /* Execute a ASE Receiver Ready for sink ASEs*/
            }
        }
        else
        {
            /* Free the allocated audio connect parameter */
            free(conn_param);
        }
    }
    return populate_success;
}

static void leAudioClient_FreeAudioConnectParameters(audio_source_t source,
                                                     source_defined_params_t *source_params)
{
    if (source == audio_source_le_audio_unicast_sender)
    {
        PanicNull(source_params);
        PanicFalse(source_params->data_length == sizeof(le_audio_connect_parameters_t));

        free(source_params->data);
        source_params->data = (void *) NULL;
        source_params->data_length = 0;
    }
}

static bool leAudioClient_GetAudioDisconnectParameters(audio_source_t source,
                                                       source_defined_params_t *source_params)
{
    UNUSED(source_params);
    UNUSED(source);
    return TRUE;
}

static void leAudioClient_FreeAudioDisconnectParameters(audio_source_t source,
                                                        source_defined_params_t *source_params)
{
    UNUSED(source_params);
    UNUSED(source);
}

static bool leAudioClient_IsAudioRouted(audio_source_t source)
{
    if (source == audio_source_le_audio_unicast_sender)
    {
    }

    return FALSE;
}

static source_status_t leAudioClient_SetState(audio_source_t source, source_state_t state)
{
    DEBUG_LOG("leAudioClient_SetState source=%d state=%d", source, state);
    /* TODO */
    return source_status_ready;
}

static device_t leAudioClient_GetAudioSourceDevice(audio_source_t source)
{
    UNUSED(source);
    return NULL;
}

void leAudioClient_InitMusicSource(void)
{
    AudioSources_RegisterAudioInterface(audio_source_le_audio_unicast_sender, &music_source_audio_interface);
}

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
