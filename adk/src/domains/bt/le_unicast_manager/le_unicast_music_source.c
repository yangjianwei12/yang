/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup le_unicast_manager
    \brief      Implementation of the audio source interface for LE music sources.
*/

#ifndef LE_UNICAST_MUSIC_SOURCE_C_
#define LE_UNICAST_MUSIC_SOURCE_C_

#if defined(INCLUDE_LE_AUDIO_UNICAST)

#include "le_unicast_manager_instance.h"
#include "le_unicast_manager_private.h"
#include "le_unicast_music_source.h"

#include "audio_sources.h"
#include "audio_sources_audio_interface.h"
#include "bt_device.h"
#include "gatt_connect.h"
#include "kymera.h"
#include "kymera_adaptation.h"
#include "kymera_adaptation_audio_protected.h"
#include "ui.h"
#include "volume_types.h"
#include "ltv_utilities.h"
#include "gatt.h"

#include <logging.h>
#include <panic.h>
#include <stdlib.h>


static bool leUnicastMusicSource_GetAudioConnectParameters(audio_source_t source, source_defined_params_t *source_params);
static void leUnicastMusicSource_FreeAudioConnectParameters(audio_source_t source, source_defined_params_t *source_params);
static bool leUnicastMusicSource_GetAudioDisconnectParameters(audio_source_t source, source_defined_params_t *source_params);
static void leUnicastMusicSource_FreeAudioDisconnectParameters(audio_source_t source, source_defined_params_t *source_params);
static bool leUnicastMusicSource_IsAudioRouted(audio_source_t source);
static bool leUnicastMusicSource_IsAudioAvailable(audio_source_t source);
static source_status_t leUnicastMusicSource_SetState(audio_source_t source, source_state_t state);
static source_state_t leUnicastMusicSource_GetState(audio_source_t source);

static const audio_source_audio_interface_t music_source_audio_interface =
{
    .GetConnectParameters = leUnicastMusicSource_GetAudioConnectParameters,
    .ReleaseConnectParameters = leUnicastMusicSource_FreeAudioConnectParameters,
    .GetDisconnectParameters = leUnicastMusicSource_GetAudioDisconnectParameters,
    .ReleaseDisconnectParameters = leUnicastMusicSource_FreeAudioDisconnectParameters,
    .IsAudioRouted = leUnicastMusicSource_IsAudioRouted,
    .IsAudioAvailable = leUnicastMusicSource_IsAudioAvailable,
    .SetState = leUnicastMusicSource_SetState,
    .GetState = leUnicastMusicSource_GetState,
    .Device = leUnicastManager_GetBtAudioDevice
};

/* \brief Extract audio parameters from Codec and Qos Information */
static bool leUnicastMusicSource_ExtractAudioParameters(le_um_instance_t *inst,
                                                        multidevice_side_t side,
                                                        le_audio_connect_parameters_t *conn_param)
{
    bool result = FALSE;
    le_um_ase_t *sink_ase = NULL;
    le_um_ase_t *source_ase = NULL;
    le_um_cis_t *cis_info = NULL;

    UNICAST_MANAGER_LOG("leUnicastMusicSource_ExtractAudioParameters");

    LeUnicastManager_GetAsesForGivenSide(inst, side, &sink_ase, &source_ase);

    conn_param->media_present = FALSE;
    conn_param->microphone_present = FALSE;

    /* Fill in the media Parameters */
    if (sink_ase != NULL && (sink_ase->state == le_um_ase_state_streaming || sink_ase->state == le_um_ase_state_routed))
    {
        le_um_ase_t *sink_ase_r = NULL;

        /* To prevent non-zero, garbage values during initialization in handle */
        conn_param->media.source_iso_handle_right = LE_AUDIO_INVALID_ISO_HANDLE;

        if (Multidevice_IsDeviceStereo())
        {
            sink_ase_r = LeUnicastManager_InstanceGetRightSinkAse(inst);
            if (LeUnicastManager_IsAseActive(sink_ase_r))
            {
                sink_ase_r->state = le_um_ase_state_routed;
                cis_info = sink_ase_r->cis_data;
                conn_param->media.source_iso_handle_right = cis_info->cis_handle;
            }
            else
            {
                sink_ase_r = NULL;
            }
        }

        cis_info = sink_ase->cis_data;
        conn_param->media_present = TRUE;
        conn_param->media.gaming_mode = LeUnicastManager_IsContextTypeGaming(inst->audio_context);
        conn_param->media.volume = AudioSources_CalculateOutputVolume(audio_source_le_audio_unicast_1);
        conn_param->media.source_iso_handle = cis_info->cis_handle;
        conn_param->media.codec_frame_blocks_per_sdu = LeUnicastManager_GetCodecFrameBlocksPerSdu(sink_ase->codec_info);

        if (LeUnicastManager_isVSAptxLite(sink_ase->codec_info))
        {
            conn_param->media.frame_length = sink_ase->qos_info->maximumSduSize;
        }
        else
        {
            conn_param->media.frame_length = LeUnicastManager_GetFramelength(sink_ase->qos_info->maximumSduSize, 
                                                conn_param->media.codec_frame_blocks_per_sdu, LeUnicastManager_GetAudioLocation(sink_ase->codec_info));
        }
        conn_param->media.presentation_delay = sink_ase->qos_info->presentationDelay;
        conn_param->media.sample_rate = LeUnicastManager_GetSampleRate(sink_ase->codec_info);
        conn_param->media.frame_duration = LeUnicastManager_GetFrameDuration(sink_ase);
        conn_param->media.codec_type = leUnicastManager_GetCodecType(sink_ase->codec_info);
        conn_param->media.codec_version = sink_ase->codec_version;
        conn_param->media.stream_type = leUnicastManager_DetermineStreamType(sink_ase, sink_ase_r);
        sink_ase->state = le_um_ase_state_routed;
        result = TRUE;
        UNICAST_MANAGER_LOG("Speaker path frame_length :%d, Presentation delay: %d, Sample Rate %d, Frame duration %d",
                            conn_param->media.frame_length,
                            conn_param->media.presentation_delay,
                            conn_param->media.sample_rate,
                            conn_param->media.frame_duration);

        UNICAST_MANAGER_LOG("codec_type enum:appKymeraLeAudioCodec:%d, Codec Version %d, Frame Blocks Per SDU %d, stream type enum:appKymeraLeStreamType:%d, gaming mode %d",
                            conn_param->media.codec_type,
                            conn_param->media.codec_version,
                            conn_param->media.codec_frame_blocks_per_sdu,
                            conn_param->media.stream_type,
                            conn_param->media.gaming_mode);
    }

    /* Fill in the microphone Parameters */
    if (source_ase != NULL)
    {
        cis_info = source_ase->cis_data;
        conn_param->microphone_present = TRUE;
        conn_param->microphone.codec_frame_blocks_per_sdu = LeUnicastManager_GetCodecFrameBlocksPerSdu(source_ase->codec_info);
        conn_param->microphone.frame_length = LeUnicastManager_isVSAptxLite(source_ase->codec_info) ? source_ase->qos_info->maximumSduSize : 
                                              source_ase->qos_info->maximumSduSize / conn_param->microphone.codec_frame_blocks_per_sdu;
        conn_param->microphone.source_iso_handle = cis_info->cis_handle;
        conn_param->microphone.presentation_delay = LeUnicastManager_isVSAptxLite(source_ase->codec_info) ? 40000 : source_ase->qos_info->presentationDelay;
        conn_param->microphone.frame_duration = LeUnicastManager_GetFrameDuration(source_ase);
        conn_param->microphone.sample_rate = LeUnicastManager_GetSampleRate(source_ase->codec_info);
        conn_param->microphone.codec_type = leUnicastManager_GetCodecType(source_ase->codec_info);
        conn_param->microphone.codec_version = source_ase->codec_version;
        conn_param->microphone.mic_sync = LeUnicastManager_IsBothSourceAseActive(inst);
        source_ase->state = le_um_ase_state_routed;
        /* Don't move source ASE state to streaming as it needs to be done only after
         * getting ReadyToReceive unicast client
         */
        PanicFalse(conn_param->microphone.codec_type != KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE);
        result = TRUE;
        UNICAST_MANAGER_LOG("Microphone path frame_length: %d, Presentation delay: %d, Sample Rate %d, Frame duration %d, codec_type %d, Codec Version %d, Frame Blocks Per SDU %d",
                            conn_param->microphone.frame_length,
                            conn_param->microphone.presentation_delay,
                            conn_param->microphone.sample_rate,
                            conn_param->microphone.frame_duration,
                            conn_param->microphone.codec_type,
                            conn_param->microphone.codec_version,
                            conn_param->microphone.codec_frame_blocks_per_sdu);
    }

    return  result;
}

/* \brief Get Audio Connect parameters */
static bool leUnicastMusicSource_GetAudioConnectParameters(audio_source_t source, source_defined_params_t *source_params)
{
    bool populate_success = FALSE;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    UNICAST_MANAGER_LOG("leUnicastMusicSource_GetAudioConnectParameters");

    if (inst)
    {
        le_audio_connect_parameters_t *conn_param =
            (le_audio_connect_parameters_t *)PanicUnlessMalloc(sizeof(le_audio_connect_parameters_t));
        memset(conn_param, 0, sizeof(le_audio_connect_parameters_t));

        if (leUnicastMusicSource_ExtractAudioParameters(inst, Multidevice_GetSide(), conn_param))
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

    return populate_success;
}

static void leUnicastMusicSource_FreeAudioConnectParameters(audio_source_t source,
                                                            source_defined_params_t *source_params)
{
    if (source == audio_source_le_audio_unicast_1)
    {
        PanicNull(source_params);
        PanicFalse(source_params->data_length == sizeof(le_audio_connect_parameters_t));

        free(source_params->data);
        source_params->data = (void *)NULL;
        source_params->data_length = 0;
    }
}

static bool leUnicastMusicSource_GetAudioDisconnectParameters(audio_source_t source,
                                                              source_defined_params_t *source_params)
{
    UNUSED(source_params);
    UNUSED(source);
    return TRUE;
}

static void leUnicastMusicSource_FreeAudioDisconnectParameters(audio_source_t source,
                                                               source_defined_params_t *source_params)
{
    UNUSED(source_params);
    UNUSED(source);
}

static bool leUnicastMusicSource_IsAudioRouted(audio_source_t source)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    if (inst)
    {
        if (LeUnicastManager_IsContextOfTypeMedia(inst->audio_context) &&
            LeUnicastManager_IsAnyAseEnabled(inst))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static bool leUnicastMusicSource_IsAudioAvailable(audio_source_t source)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    if (inst)
    {
        if (LeUnicastManager_IsContextOfTypeMedia(inst->audio_context) &&
            LeUnicastManager_IsAnyAseEnabled(inst))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static source_status_t leUnicastMusicSource_SetState(audio_source_t source, source_state_t state)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByAudioSource(source);

    if (inst)
    {
        UNICAST_MANAGER_LOG("leUnicastMusicSource_SetState source enum:audio_source_t:%u state enum:source_state_t:%u", source, state);

        switch(state)
        {
            case source_state_disconnecting:
                if(leUnicastMusicSource_IsAudioRouted(source))
                {
                    LeUnicastManager_RemoveAllDataPaths(inst);
                }
                break;

            default:
                break;
        }

        LeUnicastManager_GetTaskData()->audio_interface_state = state;
    }

    return source_status_ready;
}

static source_state_t leUnicastMusicSource_GetState(audio_source_t source)
{
    source_state_t state = source_state_invalid;

    if (source == audio_source_le_audio_unicast_1)
    {
        state =  LeUnicastManager_GetTaskData()->audio_interface_state;
    }

    return state;
}

void LeUnicastMusicSource_Reconfig(le_um_instance_t *inst)
{
    le_audio_connect_parameters_t le_audio_params;
    connect_parameters_t connect_params =
        {.source = {.type = source_type_audio, .u = {.audio = audio_source_le_audio_unicast_1}},
         .source_params = {sizeof(le_audio_params), &le_audio_params}};

    UNICAST_MANAGER_LOG("LeUnicastMusicSource_Reconfig");

    memset(&le_audio_params, 0, sizeof(le_audio_params));

    le_audio_params.reconfig = TRUE;

    if (leUnicastMusicSource_ExtractAudioParameters(inst, Multidevice_GetSide(), &le_audio_params))
    {
        KymeraAdaptation_Connect(&connect_params);
        return;
    }

    UNICAST_MANAGER_LOG("LeUnicastMusicSource_Reconfig : Failed to extract LE audio connect params");
}

void LeUnicastMusicSource_Init(void)
{
    AudioSources_RegisterAudioInterface(audio_source_le_audio_unicast_1, &music_source_audio_interface);
}

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) */

#endif /* LE_UNICAST_MUSIC_SOURCE_C_ */
