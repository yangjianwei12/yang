/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_unicast_manager
    \brief      Interfaces for the Unicast manager peer signal
*/
#if defined(INCLUDE_LE_AUDIO_UNICAST)

#ifdef INCLUDE_MIRRORING

#include "le_unicast_manager_instance.h"
#include "le_unicast_manager_peer_signal.h"
#include "le_unicast_manager_private.h"
#include "micp_server.h"
#include "ltv_utilities.h"
#include "audio_sources.h"
#include "voice_sources.h"
#include "panic.h"
#include "kymera.h"

bool LeUnicastManagerPeerSignal_FillCodecAndQoSCfg(multidevice_side_t side,
                                                   mirror_profile_lea_unicast_audio_conf_req_t *conf_data)
{
    le_um_ase_t *sink_ase = NULL;
    le_um_ase_t *source_ase = NULL;
    bool codec_qos_filled = FALSE;
    le_um_instance_t *inst = LeUnicastManager_GetInstance();

    PanicNull(conf_data);

    memset(conf_data, 0, sizeof(*conf_data));

    LeUnicastManager_GetAsesForGivenSide(inst, side, &sink_ase, &source_ase);

    if (sink_ase != NULL && LeUnicastManager_IsAseActive(sink_ase))
    {
        PanicNull((void *) sink_ase->qos_info);
        PanicNull((void *) sink_ase->codec_info);

        /* Fill in the Codec and Qos config data for sink ASE */
        conf_data->sink_codec_id = leUnicastManager_GetCodecType(sink_ase->codec_info);
        conf_data->codec_version = sink_ase->codec_version;
        conf_data->sink_frame_duration = LeUnicastManager_GetFrameDuration(sink_ase);
        conf_data->sink_framing = sink_ase->qos_info->framing;
        conf_data->sink_blocks_per_sdu = LeUnicastManager_GetCodecFrameBlocksPerSdu(sink_ase->codec_info);
        conf_data->sink_frame_length = LeUnicastManager_GetFramelength(sink_ase->qos_info->maximumSduSize, 
                                                conf_data->sink_blocks_per_sdu, LeUnicastManager_GetAudioLocation(sink_ase->codec_info));
        conf_data->sink_max_transport_latency = sink_ase->qos_info->maxTransportLatency;
        conf_data->sink_phy = sink_ase->qos_info->phy;
        conf_data->sink_presentation_delay = sink_ase->qos_info->presentationDelay;
        conf_data->sink_retransmission_num = sink_ase->qos_info->retransmissionNumber;
        conf_data->sink_sampling_frequency = LeUnicastManager_GetSampleRate(sink_ase->codec_info);
        conf_data->sink_sdu_interval = sink_ase->qos_info->sduInterval;
        conf_data->sink_target_latency = sink_ase->codec_info->targetLatency;
        conf_data->sink_target_phy = sink_ase->codec_info->targetPhy;
        conf_data->sink_gaming_mode = LeUnicastManager_IsContextTypeGaming(inst->audio_context);

        codec_qos_filled = TRUE;
    }

    if (source_ase != NULL)
    {
        PanicNull((void *) source_ase->qos_info);
        PanicNull((void *) source_ase->codec_info);

        /* Fill in the Codec and Qos config data for source ASE */
        conf_data->source_frame_duration = LeUnicastManager_GetFrameDuration(source_ase);
        conf_data->source_framing = source_ase->qos_info->framing;
        conf_data->source_blocks_per_sdu = LeUnicastManager_GetCodecFrameBlocksPerSdu(source_ase->codec_info);
        conf_data->source_frame_length = source_ase->qos_info->maximumSduSize / conf_data->source_blocks_per_sdu;
        conf_data->source_max_transport_latency = source_ase->qos_info->maxTransportLatency;
        conf_data->source_phy = source_ase->qos_info->phy;
        conf_data->source_presentation_delay = source_ase->qos_info->presentationDelay;
        conf_data->source_retransmission_num = source_ase->qos_info->retransmissionNumber;
        conf_data->source_sampling_frequency = LeUnicastManager_GetSampleRate(source_ase->codec_info);
        conf_data->source_codec_id = leUnicastManager_GetCodecType(source_ase->codec_info);
        conf_data->codec_version = source_ase->codec_version;
        conf_data->source_sdu_interval = source_ase->qos_info->sduInterval;
        conf_data->source_target_latency = source_ase->codec_info->targetLatency;
        conf_data->source_target_phy = source_ase->codec_info->targetPhy;
        conf_data->mic_mute_state = (uint8) MicpServer_GetMicState();
        conf_data->mic_sync = LeUnicastManager_IsBothSourceAseActive(inst);
        codec_qos_filled = TRUE;
    }

    conf_data->audio_context = LeUnicastManager_InstanceGetAudioContext(inst);
    conf_data->mirror_type = inst->mirror_type;

    if (LeUnicastManager_IsContextTypeConversational(conf_data->audio_context))
    {
        conf_data->voice_source = voice_source_le_audio_unicast_1;
        conf_data->audio_source = audio_source_none;
    }
    else
    {
        conf_data->voice_source = voice_source_none;
        conf_data->audio_source = audio_source_le_audio_unicast_1;
    }

    return codec_qos_filled;
}

#endif /* INCLUDE_MIRRORING */
#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) */
