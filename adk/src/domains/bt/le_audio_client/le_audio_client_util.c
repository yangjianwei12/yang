/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief    Utility functions for the LE Audio Client
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)
#include "le_audio_client_context.h"

uint16 leAudioClient_GetFrameDuration(bool is_source, le_audio_client_mode_t mode, uint8 stream_type)
{
    uint16 frame_duration = 0;
    uint8 bap_frame_duration = 0;
    le_audio_client_context_t* client_ctxt = leAudioClient_GetContext();

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    if (mode == LE_AUDIO_CLIENT_MODE_UNICAST)
    {
        bap_frame_duration = is_source ? client_ctxt->session_data.codec_qos_config.srcFrameDuaration
                                       : client_ctxt->session_data.codec_qos_config.sinkFrameDuaration;
    }
#else
    UNUSED(is_source);
#endif

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
    if (mode == LE_AUDIO_CLIENT_MODE_BROADCAST)
    {
        bap_frame_duration = client_ctxt->broadcast_session_data.frame_duration;
    }
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    if (mode == LE_AUDIO_CLIENT_MODE_UNICAST)
    {
        if (leAudioClient_isVSAptxLite(client_ctxt->session_data.codec_qos_config))
        {
            return (stream_type == KYMERA_LE_STREAM_DUAL_MONO ? LE_APTX_LITE_FRAME_DURATION_6P25 : LE_APTX_LITE_DEFAULT_FRAME_DURATION);
        }
        else if (leAudioClient_isVSAptxAdaptive(client_ctxt->session_data.codec_qos_config))
        {
            return LE_APTX_ADAPTIVE_FRAME_DURATION;
        }
    }
#else
    UNUSED(stream_type);
#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) && defined(INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE) */

    if (bap_frame_duration == BAP_SUPPORTED_FRAME_DURATION_10MS)
    {
        frame_duration = 10000;
    }
    else if (bap_frame_duration == BAP_SUPPORTED_FRAME_DURATION_7P5MS)
    {
        frame_duration = 7500;
    }
    else
    {
        DEBUG_LOG_INFO("leAudioClient_GetFrameDuration Unsupported ");
        Panic();
    }

    return frame_duration;
}

uint16 leAudioClient_GetFrameLength(uint16 max_sdu_size, uint16 stream_type, uint16 codec_type)
{
    uint16 frame_length = max_sdu_size;

    if (stream_type == KYMERA_LE_STREAM_STEREO_USE_BOTH && codec_type == KYMERA_LE_AUDIO_CODEC_LC3)
    {
        frame_length /= 2;
    }

    return frame_length;
}

uint8 leAudioClient_GetCodecType(CapClientAudioConfig cap_client_audio_config)
{
    uint8 codec_used = KYMERA_LE_AUDIO_CODEC_LC3;

#if !defined(INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE) || !defined(INCLUDE_LE_APTX_ADAPTIVE)
    UNUSED(cap_client_audio_config);
#endif

#if defined(INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE) || defined(INCLUDE_LE_APTX_ADAPTIVE)
    if (cap_client_audio_config.codecId == BAP_CODEC_ID_VENDOR_DEFINED)
    {
        switch (cap_client_audio_config.vendorCodecId)
        {
            case CAP_CLIENT_VS_CODEC_ID_APTX_LITE :
            {
                codec_used = KYMERA_LE_AUDIO_CODEC_APTX_LITE;
            }
            break;

            case CAP_CLIENT_VS_CODEC_ID_APTX_ADAPTIVE :
            {
                codec_used = KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE;
            }
            break;

            default:
            break;
        }
    }
#endif

    return codec_used;
}

uint16 leAudioClient_GetSampleRate(uint16 cap_stream_capability)
{
    uint32 sample_rate = 0;

    switch(cap_stream_capability)
    {
        case CAP_CLIENT_STREAM_CAPABILITY_8_1:
        case CAP_CLIENT_STREAM_CAPABILITY_8_2:
            sample_rate = 8000;
            break;

        case CAP_CLIENT_STREAM_CAPABILITY_16_1:
        case CAP_CLIENT_STREAM_CAPABILITY_16_2:
            sample_rate = 16000;
            break;

        case CAP_CLIENT_STREAM_CAPABILITY_24_1:
        case CAP_CLIENT_STREAM_CAPABILITY_24_2:
            sample_rate = 24000;
            break;

        case CAP_CLIENT_STREAM_CAPABILITY_48_1:
        case CAP_CLIENT_STREAM_CAPABILITY_48_2:
        case CAP_CLIENT_STREAM_CAPABILITY_48_3:
        case CAP_CLIENT_STREAM_CAPABILITY_48_4:
        case CAP_CLIENT_STREAM_CAPABILITY_48_5:
        case CAP_CLIENT_STREAM_CAPABILITY_48_6:
            sample_rate = 48000;
            break;

        case CAP_CLIENT_STREAM_CAPABILITY_32_1:
        case CAP_CLIENT_STREAM_CAPABILITY_32_2:
            sample_rate = 32000;
            break;

        case CAP_CLIENT_STREAM_CAPABILITY_441_1:
        case CAP_CLIENT_STREAM_CAPABILITY_441_2:
            sample_rate = 44100;
            break;

        default:
            DEBUG_LOG_INFO("leAudioClient_GetSampleRate Not Found");
            break;
    }

    return sample_rate;
}

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

