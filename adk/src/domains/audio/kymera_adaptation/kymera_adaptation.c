/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Adaptation layer providing a generalised API into the world of kymera audio
*/

#include "kymera_adaptation.h"
#include "kymera_adaptation_audio_protected.h"
#include "kymera_config.h"
#include "kymera_adaptation_voice_protected.h"
#include "kymera.h"
#include "kymera_volume.h"
#include "volume_utils.h"
#include "audio_sources.h"
#include "voice_sources.h"
#include <logging.h>

static appKymeraScoMode kymeraAdaptation_ConvertToScoMode(hfp_codec_mode_t codec_mode)
{
    appKymeraScoMode sco_mode = NO_SCO;
    switch(codec_mode)
    {

        case hfp_codec_mode_narrowband:
            sco_mode = SCO_NB;
            break;
        case hfp_codec_mode_wideband:
            sco_mode = SCO_WB;
            break;
        case hfp_codec_mode_ultra_wideband:
            sco_mode = SCO_UWB;
            break;
        case hfp_codec_mode_super_wideband:
            sco_mode = SCO_SWB;
            break;
#ifdef INCLUDE_SWB_LC3
        case hfp_codec_mode_super_wideband_lc3:
            sco_mode = SCO_SWB_LC3;
            break;
#endif
        case hfp_codec_mode_none:
        default:
            break;
    }
    return sco_mode;
}

static void kymeraAdaptation_ConnectSco(connect_parameters_t * params)
{
    if(params->source_params.data_length == sizeof(voice_connect_parameters_t))
    {
        voice_connect_parameters_t * voice_params = (voice_connect_parameters_t *)params->source_params.data;
        appKymeraScoMode sco_mode = kymeraAdaptation_ConvertToScoMode(voice_params->codec_mode);
        int16 volume_in_db = VolumeUtils_GetVolumeInDb(voice_params->volume);
        appKymeraScoStart(voice_params->audio_sink, sco_mode, voice_params->wesco, volume_in_db,
                          voice_params->pre_start_delay, voice_params->synchronised_start,
                          voice_params->started_handler);
    }
    else
    {
        Panic();
    }
}

static void kymeraAdaptation_ConnectUsbVoice(connect_parameters_t * params)
{
    usb_voice_connect_parameters_t * usb_voice =
            (usb_voice_connect_parameters_t *)params->source_params.data;
    int16 volume_in_db = VolumeUtils_GetVolumeInDb(usb_voice->volume);

    appKymeraUsbVoiceStart(usb_voice->mode, usb_voice->spkr_channels, usb_voice->spkr_frame_size,
                           usb_voice->spkr_sample_rate, usb_voice->mic_sample_rate, usb_voice->spkr_src,
                           usb_voice->mic_sink, volume_in_db, usb_voice->min_latency_ms, usb_voice->max_latency_ms,
                           usb_voice->target_latency_ms, usb_voice->kymera_stopped_handler);
}

#ifdef INCLUDE_LE_AUDIO_UNICAST
#ifdef COMMON_LE_AUDIO_CHAIN
static void kymeraAdaptation_ConnectLeVoiceThroughLeAudio(le_voice_connect_parameters_t *connect_voice)
{
    le_audio_connect_parameters_t connect_params;
    int16 volume_in_db = VolumeUtils_GetVolumeInDb(connect_voice->volume);

    connect_params.media_present = TRUE;
    connect_params.microphone_present = TRUE;

    connect_params.media.codec_frame_blocks_per_sdu = connect_voice->le_voice_config.speaker_stream.codec_frame_blocks_per_sdu;
    connect_params.media.codec_type = connect_voice->le_voice_config.codec_type;
    connect_params.media.codec_version = connect_voice->le_voice_config.codec_version;
    connect_params.media.frame_duration= connect_voice->le_voice_config.frame_duration;
    connect_params.media.frame_length = connect_voice->le_voice_config.speaker_stream.frame_length;
    connect_params.media.gaming_mode = FALSE;
    connect_params.media.presentation_delay = connect_voice->le_voice_config.speaker_stream.presentation_delay;
    connect_params.media.sample_rate = connect_voice->le_voice_config.sample_rate;
    connect_params.media.source_iso_handle = connect_voice->le_voice_config.source_iso_handle;
    connect_params.media.source_iso_handle_right = connect_voice->le_voice_config.source_iso_handle_right;
    connect_params.media.start_muted = FALSE;
    connect_params.media.stream_type = connect_voice->le_voice_config.speaker_stream.stream_type;
    connect_params.media.use_cvc = TRUE;
    connect_params.media.volume = connect_voice->volume;

    connect_params.microphone.codec_frame_blocks_per_sdu= connect_voice->le_voice_config.mic_stream.codec_frame_blocks_per_sdu;
    connect_params.microphone.codec_type = connect_voice->le_voice_config.codec_type;
    connect_params.microphone.codec_version = connect_voice->le_voice_config.codec_version;
    connect_params.microphone.frame_duration = connect_voice->le_voice_config.frame_duration;
    connect_params.microphone.frame_length = connect_voice->le_voice_config.mic_stream.frame_length;
    connect_params.microphone.presentation_delay = connect_voice->le_voice_config.mic_stream.presentation_delay;
    connect_params.microphone.sample_rate = connect_voice->le_voice_config.sample_rate;
    connect_params.microphone.source_iso_handle = connect_voice->le_voice_config.source_iso_handle;
    connect_params.microphone.source_iso_handle_right = connect_voice->le_voice_config.source_iso_handle_right;
    connect_params.microphone.started_handler = NULL;
    connect_params.microphone.mic_mute_state = connect_voice->le_voice_config.mic_mute_state;

    Kymera_LeAudioStart(connect_params.media_present, connect_params.microphone_present, FALSE,
                        volume_in_db, &connect_params.media, &connect_params.microphone);
}
#endif
static void kymeraAdaptation_ConnectLeVoice(connect_parameters_t * params)
{
#ifdef COMMON_LE_AUDIO_CHAIN
    kymeraAdaptation_ConnectLeVoiceThroughLeAudio((le_voice_connect_parameters_t *)params->source_params.data);
#else
    le_voice_connect_parameters_t * connect_params = (le_voice_connect_parameters_t *)params->source_params.data;
    int16 volume_in_db = VolumeUtils_GetVolumeInDb(connect_params->volume);
    Kymera_LeVoiceStart(connect_params->speaker_present, connect_params->microphone_present, connect_params->reconfig,
                        volume_in_db, &connect_params->speaker, &connect_params->microphone);
#endif
}
#endif

static void kymeraAdaptation_ConnectVoice(connect_parameters_t * params)
{
    switch(params->source.u.voice)
    {
        case voice_source_hfp_1:
        case voice_source_hfp_2:
            kymeraAdaptation_ConnectSco(params);
            break;

        case voice_source_usb:
            kymeraAdaptation_ConnectUsbVoice(params);
            break;
#ifdef INCLUDE_LE_AUDIO_UNICAST
        case voice_source_le_audio_unicast_1:
            kymeraAdaptation_ConnectLeVoice(params);
            break;
#endif
        default:
            Panic();
            break;
    }
}

static void kymeraAdaptation_ConnectA2DP(connect_parameters_t * params)
{
    a2dp_connect_parameters_t * connect_params = (a2dp_connect_parameters_t *)params->source_params.data;
    a2dp_codec_settings codec_settings =
    {
        .rate = connect_params->rate, .channel_mode = connect_params->channel_mode,
        .seid = connect_params->seid, .sink = connect_params->sink,
        .codecData =
        {
            .content_protection = connect_params->content_protection, .bitpool = connect_params->bitpool,
            .format = connect_params->format, .packet_size = connect_params->packet_size,
            .aptx_ad_params.features = connect_params->aptx_features
        }
    };

    int16 volume_in_db = VolumeUtils_GetVolumeInDb(connect_params->volume);
    appKymeraA2dpStart(connect_params->client_lock, connect_params->client_lock_mask,
                        &codec_settings, connect_params->max_bitrate,
                        volume_in_db, connect_params->master_pre_start_delay, connect_params->q2q_mode,
                        connect_params->nq2q_ttp);
}

static void kymeraAdaptation_ConnectLineIn(connect_parameters_t * params)
{
    wired_analog_connect_parameters_t * connect_params = (wired_analog_connect_parameters_t *)params->source_params.data;
    int16 volume_in_db = VolumeUtils_GetVolumeInDb(connect_params->volume);
    Kymera_StartWiredAnalogAudio(volume_in_db, connect_params->rate, connect_params->min_latency, connect_params->max_latency, connect_params->target_latency);
}

static void kymeraAdaptation_ConnectUsbAudio(connect_parameters_t * params)
{
    usb_audio_connect_parameters_t * usb_audio =
            (usb_audio_connect_parameters_t *)params->source_params.data;
    int16 volume_in_db = VolumeUtils_GetVolumeInDb(usb_audio->volume);

    appKymeraUsbAudioStart(usb_audio->channels, usb_audio->frame_size,
                           usb_audio->spkr_src, volume_in_db, usb_audio->mute_status,
                           usb_audio->sample_freq, usb_audio->min_latency_ms,
                           usb_audio->max_latency_ms, usb_audio->target_latency_ms);
}

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
static void kymeraAdaptation_ConnectLeAudio(connect_parameters_t * params)
{
    le_audio_connect_parameters_t * connect_params = (le_audio_connect_parameters_t *)params->source_params.data;
    int16 volume_in_db = VolumeUtils_GetVolumeInDb(connect_params->media.volume);

    Kymera_LeAudioStart(connect_params->media_present, connect_params->microphone_present, connect_params->reconfig,
                        volume_in_db, &connect_params->media, &connect_params->microphone);
}
#endif

static void kymeraAdaptation_ConnectAudio(connect_parameters_t * params)
{
    switch(params->source.u.audio)
    {
        case audio_source_a2dp_1:
        case audio_source_a2dp_2:
            kymeraAdaptation_ConnectA2DP(params);
            break;

        case audio_source_line_in:
            kymeraAdaptation_ConnectLineIn(params);
            break;

        case audio_source_usb:
            kymeraAdaptation_ConnectUsbAudio(params);
            break;
#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
        case audio_source_le_audio_unicast_1:
        case audio_source_le_audio_broadcast:
            kymeraAdaptation_ConnectLeAudio(params);
            break;
#endif
        default:
            Panic();
            break;
    }
}

static void kymeraAdaptation_DisconnectSco(disconnect_parameters_t * params)
{
    if(params->source_params.data_length == 0)
    {
        appKymeraScoStop();
    }
    else
    {
        Panic();
    }
}

static void kymeraAdaptation_DisconnectUsbVoice(disconnect_parameters_t * params)
{
    usb_voice_disconnect_parameters_t * voice_params =
            (usb_voice_disconnect_parameters_t *)params->source_params.data;

    appKymeraUsbVoiceStop(voice_params->spkr_src, voice_params->mic_sink,
                          voice_params->kymera_stopped_handler);
}

#ifdef INCLUDE_LE_AUDIO_UNICAST
static void kymeraAdaptation_DisconnectLeVoice(disconnect_parameters_t * params)
{
    if(params->source_params.data_length == 0)
    {
        Kymera_LeVoiceStop();
    }
    else
    {
        Panic();
    }
}
#endif

static void kymeraAdaptation_DisconnectVoice(disconnect_parameters_t * params)
{
    switch(params->source.u.voice)
    {
        case voice_source_hfp_1:
        case voice_source_hfp_2:
            kymeraAdaptation_DisconnectSco(params);
            break;

        case voice_source_usb:
            kymeraAdaptation_DisconnectUsbVoice(params);
            break;
#ifdef INCLUDE_LE_AUDIO_UNICAST
        case voice_source_le_audio_unicast_1:
            kymeraAdaptation_DisconnectLeVoice(params);
            break;
#endif
        default:
            Panic();
            break;
    }
}

static void kymeraAdaptation_DisconnectA2DP(disconnect_parameters_t * params)
{
    a2dp_disconnect_parameters_t * disconnect_params = (a2dp_disconnect_parameters_t *)params->source_params.data;
    appKymeraA2dpStop(disconnect_params->seid, disconnect_params->source);
}

static void kymeraAdaptation_DisconnectUsbAudio(disconnect_parameters_t * params)
{
    usb_audio_disconnect_parameters_t * audio_params =
            (usb_audio_disconnect_parameters_t *)params->source_params.data;

    appKymeraUsbAudioStop(audio_params->source,
                          audio_params->kymera_stopped_handler);
}

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
static void kymeraAdaptation_DisconnectLeAudio(void)
{
    Kymera_LeAudioStop();
}
#endif

static void kymeraAdaptation_DisconnectAudio(disconnect_parameters_t * params)
{
    switch(params->source.u.audio)
    {
        case audio_source_a2dp_1:
        case audio_source_a2dp_2:
            kymeraAdaptation_DisconnectA2DP(params);
            break;
        case audio_source_line_in:
            Kymera_StopWiredAnalogAudio();
            break;
        case audio_source_usb:
            kymeraAdaptation_DisconnectUsbAudio(params);
            break;
#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
        case audio_source_le_audio_unicast_1:
        case audio_source_le_audio_broadcast:
            kymeraAdaptation_DisconnectLeAudio();
            break;
#endif
        default:
            Panic();
            break;
    }
}

void KymeraAdaptation_Connect(connect_parameters_t * params)
{
    switch(params->source.type)
    {
        case source_type_voice:
            kymeraAdaptation_ConnectVoice(params);
            break;
        case source_type_audio:
            kymeraAdaptation_ConnectAudio(params);
            break;
        default:
            Panic();
            break;
    }
}

void KymeraAdaptation_Disconnect(disconnect_parameters_t * params)
{
    switch(params->source.type)
    {
        case source_type_voice:
            kymeraAdaptation_DisconnectVoice(params);
            break;
        case source_type_audio:
            kymeraAdaptation_DisconnectAudio(params);
            break;
        default:
            Panic();
            break;
    }
}

static void kymeraAdaptation_SetVoiceVolume(generic_source_t source, volume_t volume)
{
    voice_source_t voice_source = source.u.voice;

    switch(voice_source)
    {
        case voice_source_hfp_1:
        case voice_source_hfp_2:
            appKymeraScoSetVolume(VolumeUtils_GetVolumeInDb(volume));
            break;

        case voice_source_usb:
            appKymeraUsbVoiceSetVolume(VolumeUtils_GetVolumeInDb(volume));
            break;
#ifdef INCLUDE_LE_AUDIO_UNICAST
        case voice_source_le_audio_unicast_1:
            Kymera_LeVoiceSetVolume(VolumeUtils_GetVolumeInDb(volume));
            break;
#endif
        default:
            break;
    }
}

static void kymeraAdaptation_SetAudioVolume(generic_source_t source, volume_t volume)
{
    audio_source_t audio_source = source.u.audio;

    switch(audio_source)
    {
        case audio_source_a2dp_1:
        case audio_source_a2dp_2:
            appKymeraA2dpSetVolume(VolumeUtils_GetVolumeInDb(volume));
            break;

        case audio_source_usb:
            appKymeraUsbAudioSetVolume(VolumeUtils_GetVolumeInDb(volume));
            break;

        case audio_source_line_in:
            appKymeraWiredAudioSetVolume(VolumeUtils_GetVolumeInDb(volume));
            break;
#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
        case audio_source_le_audio_unicast_1:
        case audio_source_le_audio_broadcast:
            Kymera_LeAudioSetVolume(VolumeUtils_GetVolumeInDb(volume));
            break;
#endif
        default:
            break;
    }
}

void KymeraAdaptation_SetVolume(volume_parameters_t * params)
{
    generic_source_t source = params->source;

    switch(params->source.type)
    {
        case source_type_voice:
            kymeraAdaptation_SetVoiceVolume(source, params->volume);
            break;

        case source_type_audio:
            kymeraAdaptation_SetAudioVolume(source, params->volume);
            break;

        default:
            Panic();
            break;
    }
}
