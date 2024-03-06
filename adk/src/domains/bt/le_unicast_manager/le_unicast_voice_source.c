/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_unicast_manager
    \brief      Implementation of the voice source interface for LE Voice sources.
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST)

#include "le_unicast_voice_source.h"
#include "le_unicast_manager_instance.h"
#include "le_unicast_telephony_control.h"
#include "le_unicast_manager_private.h"
#include "kymera_adaptation.h"
#include "kymera_adaptation_voice_protected.h"
#include "voice_sources.h"
#include "micp_server.h"

#include <logging.h>
#include <panic.h>
#include <stdlib.h>

static bool leUnicastVoiceSource_GetAudioConnectParameters(voice_source_t source, source_defined_params_t *source_params);
static void leUnicastVoiceSource_FreeAudioConnectParameters(voice_source_t source, source_defined_params_t *source_params);
static bool leUnicastVoiceSource_GetAudioDisconnectParameters(voice_source_t source, source_defined_params_t *source_params);
static void leUnicastVoiceSource_FreeAudioDisconnectParameters(voice_source_t source, source_defined_params_t *source_params);
static bool leUnicastVoiceSource_IsVoiceChannelAvailable(voice_source_t source);
static bool leUnicastVoiceSource_IsAudioRouted(voice_source_t source);
static source_status_t leUnicastVoiceSource_SetState(voice_source_t source, source_state_t state);
static source_state_t leUnicastVoiceSource_GetState(voice_source_t source);

static const voice_source_audio_interface_t voice_source_audio_interface =
{
    .GetConnectParameters = leUnicastVoiceSource_GetAudioConnectParameters,
    .ReleaseConnectParameters = leUnicastVoiceSource_FreeAudioConnectParameters,
    .GetDisconnectParameters = leUnicastVoiceSource_GetAudioDisconnectParameters,
    .ReleaseDisconnectParameters = leUnicastVoiceSource_FreeAudioDisconnectParameters,
    .IsAudioRouted = leUnicastVoiceSource_IsAudioRouted,
    .IsVoiceChannelAvailable = leUnicastVoiceSource_IsVoiceChannelAvailable,
    .SetState = leUnicastVoiceSource_SetState,
    .GetState = leUnicastVoiceSource_GetState
};

static uint16 leUnicastVoiceSource_GetFrameLength(le_um_ase_t *ase)
{
    uint8 frame_blocks_per_sdu = LeUnicastManager_GetCodecFrameBlocksPerSdu(ase->codec_info);

    return LeUnicastManager_GetFramelength(ase->qos_info->maximumSduSize, 
                                                            frame_blocks_per_sdu, LeUnicastManager_GetAudioLocation(ase->codec_info));
}

/* \brief Extract audio parameters from Codec and Qos Information */
static bool leUnicastVoiceSource_ExtractAudioParameters(le_um_instance_t *inst,
                                                        le_voice_connect_parameters_t *conn_param)
{
    bool populate_success = FALSE;
    le_um_ase_t *sink_ase;
    le_um_ase_t *source_ase;
    multidevice_side_t side = Multidevice_GetSide();
    le_um_ase_t *sink_ase_r;

    UNICAST_MANAGER_LOG("leUnicastVoiceSource_ExtractAudioParameters :");

    LeUnicastManager_GetAsesForGivenSide(inst, side, &sink_ase, &source_ase);

    /* To prevent non-zero, garbage values during initialization in handle */
    conn_param->speaker.iso_handle = LE_AUDIO_INVALID_ISO_HANDLE;
    conn_param->speaker.iso_handle_right = LE_AUDIO_INVALID_ISO_HANDLE;
    conn_param->microphone.source_iso_handle = LE_AUDIO_INVALID_ISO_HANDLE;
    conn_param->microphone.source_iso_handle_right = LE_AUDIO_INVALID_ISO_HANDLE;
    conn_param->microphone_present = FALSE;
    conn_param->speaker_present = FALSE;
    conn_param->reconfig = TRUE;

    /* Fill in the Audio Parameters */
    if (sink_ase != NULL && (sink_ase->state == le_um_ase_state_streaming || sink_ase->state == le_um_ase_state_routed))
    {
        sink_ase_r = NULL;
        le_um_cis_t *voice_cis_info = sink_ase->cis_data;

        if (Multidevice_IsDeviceStereo())
        {
            sink_ase_r = LeUnicastManager_InstanceGetRightSinkAse(inst);
            if (LeUnicastManager_IsAseActive(sink_ase_r))
            {
                sink_ase_r->state = le_um_ase_state_routed;
                conn_param->speaker.iso_handle_right = sink_ase_r->cis_data->cis_handle;
            }
            else
            {
                sink_ase_r = NULL;
            }
        }

        conn_param->volume = VoiceSources_CalculateOutputVolume(voice_source_le_audio_unicast_1);
        conn_param->speaker.iso_handle = voice_cis_info->cis_handle;
        conn_param->speaker.sample_rate = LeUnicastManager_GetSampleRate(sink_ase->codec_info);
        conn_param->speaker.frame_duration = LeUnicastManager_GetFrameDuration(sink_ase);
        conn_param->speaker.codec_type = KYMERA_LE_AUDIO_CODEC_LC3; /* @TODO this has to be derived by application from the ASE configuration data. */
        conn_param->speaker.codec_version = sink_ase->codec_version;
        conn_param->speaker_present = TRUE;

        conn_param->speaker.codec_frame_blocks_per_sdu = LeUnicastManager_GetCodecFrameBlocksPerSdu(sink_ase->codec_info);
        conn_param->speaker.frame_length = leUnicastVoiceSource_GetFrameLength(sink_ase);
        conn_param->speaker.presentation_delay = sink_ase->qos_info->presentationDelay;
        conn_param->speaker.stream_type = leUnicastManager_DetermineStreamType(sink_ase, sink_ase_r);
        sink_ase->state = le_um_ase_state_routed;

        populate_success = TRUE;
        UNICAST_MANAGER_LOG("Speaker path iso_handle : 0x%x, iso_handle_right : 0x%x, Volume %d",
                            conn_param->speaker.iso_handle,
                            conn_param->speaker.iso_handle_right,
                            conn_param->volume.value);

        UNICAST_MANAGER_LOG(" sample_rate : %d, frame_duration %d, codec_type: %d, codec_version: %d",
                            conn_param->speaker.sample_rate,
                            conn_param->speaker.frame_duration,
                            conn_param->speaker.codec_type,
                            conn_param->speaker.codec_version);
    }

    if (source_ase != NULL)
    {
        le_um_cis_t *mic_cis_info = source_ase->cis_data;
        conn_param->microphone.source_iso_handle = mic_cis_info->cis_handle;
        conn_param->microphone.mic_mute_state = (uint8)MicpServer_GetMicState();

        conn_param->microphone.codec_frame_blocks_per_sdu = LeUnicastManager_GetCodecFrameBlocksPerSdu(source_ase->codec_info);
        conn_param->microphone.frame_length = leUnicastVoiceSource_GetFrameLength(source_ase);
        conn_param->microphone.presentation_delay = source_ase->qos_info->presentationDelay;
        conn_param->microphone.sample_rate = LeUnicastManager_GetSampleRate(source_ase->codec_info);
        conn_param->microphone.frame_duration = LeUnicastManager_GetFrameDuration(source_ase);
        conn_param->microphone.codec_type = KYMERA_LE_AUDIO_CODEC_LC3; /* @TODO this has to be derived by application from the ASE configuration data. */
        conn_param->microphone.codec_version = source_ase->codec_version;
        conn_param->microphone_present = TRUE;

        populate_success = TRUE;

        /* Don't move source ASE state to streaming as it needs to be done only after
         * getting ReadyToReceive unicast client
         */
        source_ase->state = le_um_ase_state_routed;
        UNICAST_MANAGER_LOG("Microphone path frame_length: %d, Presentation delay: %d, Sample Rate %d, Frame duration %d, codec_type %d, Codec Version %d, Frame Blocks Per SDU %d",
                    conn_param->microphone.frame_length,
                    conn_param->microphone.presentation_delay,
                    conn_param->microphone.sample_rate,
                    conn_param->microphone.frame_duration,
                    conn_param->microphone.codec_type,
                    conn_param->microphone.codec_version,
                    conn_param->microphone.codec_frame_blocks_per_sdu);
        UNICAST_MANAGER_LOG("Microphone path iso_handle: 0x%x", conn_param->microphone.source_iso_handle);
    }

    return  populate_success;
}

void LeUnicastVoiceSource_Reconfig(le_um_instance_t *inst)
{
    le_voice_connect_parameters_t le_voice_params;

    connect_parameters_t connect_params =
        {.source = {.type = source_type_voice, .u = {.voice = voice_source_le_audio_unicast_1}},
         .source_params = {sizeof(le_voice_params), &le_voice_params}};

    UNICAST_MANAGER_LOG("LeUnicastVoiceSource_Reconfig");

    memset(&le_voice_params, 0, sizeof(le_voice_params));

    le_voice_params.reconfig = TRUE;

    if (leUnicastVoiceSource_ExtractAudioParameters(inst, &le_voice_params))
    {
        KymeraAdaptation_Connect(&connect_params);
        return;
    }

    UNICAST_MANAGER_LOG("LeUnicastVoiceSource_Reconfig : Failed to extract LE audio connect params");
}


static bool leUnicastVoiceSource_GetAudioConnectParameters(voice_source_t source, source_defined_params_t *source_params)
{
    bool populate_success = FALSE;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByVoiceSource(source);

    UNICAST_MANAGER_LOG("leUnicastVoiceSource_GetAudioConnectParameters");

    if (inst)
    {
        le_voice_connect_parameters_t *conn_param =
            (le_voice_connect_parameters_t *) PanicUnlessMalloc(sizeof(le_voice_connect_parameters_t));
        memset(conn_param, 0, sizeof(le_voice_connect_parameters_t));

        conn_param->reconfig = FALSE;
        if (leUnicastVoiceSource_ExtractAudioParameters(inst, conn_param))
        {
            source_params->data = (void *)conn_param;
            source_params->data_length = sizeof(le_voice_connect_parameters_t);
            populate_success = TRUE;
        }
        else
        {
            /* Free the allocated voice connect parameter */
            pfree(conn_param);
        }
    }

    UNICAST_MANAGER_LOG("leUnicastVoiceSource_GetAudioConnectParameters success %d", populate_success);
    return populate_success;
}

static void leUnicastVoiceSource_FreeAudioConnectParameters(voice_source_t source,
                                                            source_defined_params_t *source_params)
{
    if (source == voice_source_le_audio_unicast_1)
    {
        PanicNull(source_params);
        PanicFalse(source_params->data_length == sizeof(le_voice_connect_parameters_t));

        pfree(source_params->data);
        source_params->data = (void *)NULL;
        source_params->data_length = 0;
    }
}

static bool leUnicastVoiceSource_GetAudioDisconnectParameters(voice_source_t source,
                                                              source_defined_params_t *source_params)
{
    UNUSED(source);

    PanicNull(source_params);
    source_params->data = (void *)NULL;
    source_params->data_length = 0;

    return TRUE;
}

static void leUnicastVoiceSource_FreeAudioDisconnectParameters(voice_source_t source,
                                                               source_defined_params_t *source_params)
{
    UNUSED(source);

    PanicNull(source_params);
    source_params->data = (void *)NULL;
    source_params->data_length = 0;;
}

static bool leUnicastVoiceSource_IsAseReady(const le_um_ase_t *ase)
{
    bool result = FALSE;
    le_um_cis_t *cis_info = ase->cis_data;

    if (cis_info != NULL && 
        LeUnicastManager_IsContextTypeConversational(ase->audio_context) &&
        LeUnicastManager_IsCisEstablished(cis_info->state) &&
        LeUnicastManager_IsAseActive(ase))
    {
        result = TRUE;
    }

    UNICAST_MANAGER_LOG("leUnicastVoiceSource_IsAseReady result %d", result);
    return result;
}

static bool leUnicastVoiceSource_IsVoiceChannelAvailable(voice_source_t source)
{
    bool audio_available = FALSE;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByVoiceSource(source);

    if (inst)
    {
        le_um_ase_t *sink_ase = NULL;
        le_um_ase_t *source_ase = NULL;

        LeUnicastManager_GetAsesForGivenSide(inst, Multidevice_GetSide(), &sink_ase, &source_ase);

        if (sink_ase != NULL && leUnicastVoiceSource_IsAseReady(sink_ase))
        {
            audio_available = TRUE;
        }

        UNICAST_MANAGER_LOG("leUnicastVoiceSource_IsVoiceChannelAvailable source enum:voice_source_t:%u inst %p is_available %d",
                            source, inst, audio_available);
    }

    return audio_available;
}

static bool leUnicastVoiceSource_IsAudioRouted(voice_source_t source)
{
    bool is_routed = FALSE;

    le_um_instance_t *inst = LeUnicastManager_InstanceGetByVoiceSource(source);

    if (inst)
    {
        if (LeUnicastManager_IsContextTypeConversational(inst->audio_context) &&
            LeUnicastManager_IsAnyAseEnabled(inst))
        {
            is_routed = TRUE;
        }

        DEBUG_LOG("leUnicastVoiceSource_IsAudioRouted is_routed %d is_conversational %d is_any_ase_enabled %d",
                  is_routed,
                  LeUnicastManager_IsContextTypeConversational(inst->audio_context),
                  LeUnicastManager_IsAnyAseEnabled(inst));
    }

    return is_routed;
}

static source_status_t leUnicastVoiceSource_SetState(voice_source_t source, source_state_t state)
{
    le_um_instance_t *inst;

    inst = LeUnicastManager_InstanceGetByVoiceSource(source);
    if (inst)
    {
        UNICAST_MANAGER_LOG("leUnicastVoiceSource_SetState: source=enum:voice_source_t:%d, "
                            "old state=enum:source_state_t:%d, new state=enum:source_state_t:%d",
                           source, inst->source_state, state);

        switch (state)
        {
            case source_state_disconnecting:
                if (leUnicastVoiceSource_IsAudioRouted(source))
                {
                    LeUnicastManager_RemoveAllDataPaths(inst);
                }
                break;

            default:
                break;
        }

        inst->source_state = state;
    }
    else
    {
        UNICAST_MANAGER_LOG("leUnicastVoiceSource_SetState: no LE UV instance found for source enum:voice_source_t:%d",
                            source);
    }

    return source_status_ready;
}

static source_state_t leUnicastVoiceSource_GetState(voice_source_t source)
{
    source_state_t state = source_state_invalid;

    le_um_instance_t *inst = LeUnicastManager_InstanceGetByVoiceSource(source);

    if (inst)
    {
        state = inst->source_state;
    }
    else
    {
        UNICAST_MANAGER_LOG("leUnicastVoiceSource_GetState: no LE UV instance found for source enum:voice_source_t:%d",
                            source);
    }

    UNICAST_MANAGER_LOG("leUnicastVoiceSource_GetState: source=enum:voice_source_t:%d, state=enum:source_state_t:%d",
              source, state);

    return state;
}

const voice_source_audio_interface_t * LeUnicastVoiceSource_GetAudioInterface(void)
{
    return &voice_source_audio_interface;
}

void LeUnicastVoiceSource_Init(void)
{
    /* Register with audio source for Audio use case */
    VoiceSources_RegisterAudioInterface(voice_source_le_audio_unicast_1,
                                        LeUnicastVoiceSource_GetAudioInterface());

    VoiceSources_RegisterTelephonyControlInterface(voice_source_le_audio_unicast_1,
                                                   LeVoiceSource_GetTelephonyControlInterface());
}

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) */
