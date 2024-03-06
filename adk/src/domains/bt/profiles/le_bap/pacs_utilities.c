/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_bap
    \brief
*/

#include "pacs_utilities.h"
#include "bt_device.h"
#include "device.h"
#include "device_db_serialiser.h"
#include "device_properties.h"
#include "gatt_connect.h"
#include "ltv_utilities.h"
#include "multidevice.h"
#include "pddu_map.h"
#include <gatt_handler_db_if.h>
#include <logging.h>
#include <panic.h>
#include <stdlib.h>
#include "gatt.h"
#include <feature.h>
#include <pacs_config.h>
#include <kymera.h>

#if defined(INCLUDE_CIS_MIRRORING) && defined(INCLUDE_LE_STEREO_RECORDING)
    #error Stereo recording is not supported when CIS Mirroring is enabled
#endif

#define PACS_UTILITIES_LOG      DEBUG_LOG

#define PACS_INVALID_HANDLE     0

#ifndef DISABLE_LE_AUDIO_MEDIA
static const uint8 metadata_audio[] = {3, BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS,
                                          ALL_MEDIA_AUDIO_CONTEXTS & 0xFF, ALL_MEDIA_AUDIO_CONTEXTS >> 8 };

static const uint8 metadata_audio_vs_lc3[] = {3, BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS, 
                                                 ALL_MEDIA_AUDIO_CONTEXTS & 0xFF, ALL_MEDIA_AUDIO_CONTEXTS >> 8 ,
                                                 VS_METADATA_LTV_SIZE_LC3, 
                                                 BAP_METADATA_LTV_TYPE_VENDOR_SPECIFIC_CONTEXTS, 
                                                 (VS_METADATA_COMPANY_ID_QUALCOMM & 0xFF), (VS_METADATA_COMPANY_ID_QUALCOMM >> 8), 
                                                 VS_METADATA_LENGTH_LC3, VS_METADATA_TYPE_LC3, 
                                                 LC3_ENCODER_VERSION_ID, LC3_DECODER_VERSION_ID_PAC,
                                                 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
#elif defined(INCLUDE_LE_AUDIO_BROADCAST)
#error "INCLUDE_LE_AUDIO_BROADCAST requires media context, can't have DISABLE_LE_AUDIO_MEDIA defined"
#endif /* !DISABLE_LE_AUDIO_MEDIA */

#ifdef INCLUDE_LE_AUDIO_UNICAST

#ifndef DISABLE_LE_AUDIO_VOICE
static const uint8 metadata_voice[] = {3, BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS,
                                          AVAILABLE_VOICE_CONTEXTS & 0xFF, AVAILABLE_VOICE_CONTEXTS >> 8 };

static const uint8 metadata_voice_vs_lc3[] = {3, BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS, 
                                                 AVAILABLE_VOICE_CONTEXTS & 0xFF, AVAILABLE_VOICE_CONTEXTS >> 8,
                                                 VS_METADATA_LTV_SIZE_LC3, 
                                                 BAP_METADATA_LTV_TYPE_VENDOR_SPECIFIC_CONTEXTS, 
                                                 (VS_METADATA_COMPANY_ID_QUALCOMM & 0xFF), (VS_METADATA_COMPANY_ID_QUALCOMM >> 8), 
                                                 VS_METADATA_LENGTH_LC3, VS_METADATA_TYPE_LC3, 
                                                 LC3_ENCODER_VERSION_ID, LC3_DECODER_VERSION_ID_PAC,
                                                 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
#endif /* !DISABLE_LE_AUDIO_VOICE */

#ifdef INCLUDE_LE_STEREO_RECORDING
static const uint8 wmap_metadata_vs_lc3[] = {3, BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS, 
                                                AUDIO_CONTEXT_TYPE_LIVE & 0xFF, AUDIO_CONTEXT_TYPE_LIVE >> 8 ,
                                                VS_METADATA_LTV_SIZE_LC3, 
                                                BAP_METADATA_LTV_TYPE_VENDOR_SPECIFIC_CONTEXTS, 
                                                (VS_METADATA_COMPANY_ID_QUALCOMM & 0xFF), (VS_METADATA_COMPANY_ID_QUALCOMM >> 8), 
                                                VS_METADATA_LENGTH_LC3, VS_METADATA_TYPE_LC3, 
                                                LC3_ENCODER_VERSION_ID, LC3_DECODER_VERSION_ID_PAC,
                                                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

static const uint8 wmap_metadata[] = {3, BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS, 
                                         AUDIO_CONTEXT_TYPE_LIVE & 0xFF, AUDIO_CONTEXT_TYPE_LIVE >> 8};
#endif /* INCLUDE_LE_STEREO_RECORDING */

#ifdef INCLUDE_LE_GAMING_MODE
/* gmap metadata */
static const uint8 gmap_metadata[] = {3, BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS, 
                                         AUDIO_CONTEXT_TYPE_GAME & 0xFF, AUDIO_CONTEXT_TYPE_GAME >> 8 };

static const uint8 gmap_metadata_vs_lc3[] = {3, BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS,
                                                AUDIO_CONTEXT_TYPE_GAME & 0xFF, AUDIO_CONTEXT_TYPE_GAME >> 8 ,
                                                VS_METADATA_LTV_SIZE_LC3, 
                                                BAP_METADATA_LTV_TYPE_VENDOR_SPECIFIC_CONTEXTS, 
                                                (VS_METADATA_COMPANY_ID_QUALCOMM & 0xFF), (VS_METADATA_COMPANY_ID_QUALCOMM >> 8), 
                                                VS_METADATA_LENGTH_LC3, VS_METADATA_TYPE_LC3, 
                                                LC3_ENCODER_VERSION_ID,LC3_DECODER_VERSION_ID_PAC,
                                                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

#endif /* INCLUDE_LE_GAMING_MODE */

#endif /* INCLUDE_LE_AUDIO_UNICAST */


/* PACS record for LE unicast/broadcast sink */
static const GattPacsServerRecordType default_sink_pacs[] =
{
#ifndef DISABLE_LE_AUDIO_MEDIA
    {
        /* Sink PAC for Broadcast and unicast Media streaming only, 16_2, 24_2, 32_2, 32_1, 48_1/2/3/4/5/6 */
        .codecId                            = PACS_LC3_CODEC_ID,
        .companyId                          = 0x0000,
        .vendorCodecId                      = 0x0000,
        .supportedSamplingFrequencies       = PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_MEDIA_SINK,
        .supportedFrameDuration             = PACS_SUPPORTED_FRAME_DURATION_FOR_MEDIA_SINK,
        .audioChannelCounts                 = PACS_SINK_AUDIO_CHANNEL_COUNT,
        .minSupportedOctetsPerCodecFrame    = PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_MEDIA_SINK,
        .maxSupportedOctetsPerCodecFrame    = PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_MEDIA_SINK,
        .supportedMaxCodecFramePerSdu       = PACS_SINK_MAX_CODEC_FRAMES_PER_SDU,
        .metadataLength                     = ARRAY_DIM(metadata_audio),
        .metadata                           = metadata_audio
    },
#endif /* !DISABLE_LE_AUDIO_MEDIA */

#ifdef INCLUDE_LE_AUDIO_UNICAST
#ifndef DISABLE_LE_AUDIO_VOICE
    {
        /* Sink PAC for unicast Conversation and Live context, 16_2, 32_1, 32_2 */
        .codecId                            = PACS_LC3_CODEC_ID,
        .companyId                          = 0x0000,
        .vendorCodecId                      = 0x0000,
        .supportedSamplingFrequencies       = PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_VOICE_SINK,
        .supportedFrameDuration             = PACS_SUPPORTED_FRAME_DURATION_FOR_VOICE_SINK,
        .audioChannelCounts                 = PACS_SINK_AUDIO_CHANNEL_COUNT,
        .minSupportedOctetsPerCodecFrame    = PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_VOICE_SINK,
        .maxSupportedOctetsPerCodecFrame    = PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_VOICE_SINK,
        .supportedMaxCodecFramePerSdu       = PACS_SINK_MAX_CODEC_FRAMES_PER_SDU,
        .metadataLength                     = ARRAY_DIM(metadata_voice),
        .metadata                           = metadata_voice
    },
#endif /* !DISABLE_LE_AUDIO_VOICE */
#ifdef INCLUDE_LE_GAMING_MODE
    {
        /* Sink PAC for Gaming mode only, 48_1, 48_2 */
        .codecId                            = PACS_LC3_CODEC_ID,
        .companyId                          = 0x0000,
        .vendorCodecId                      = 0x0000,
        .supportedSamplingFrequencies       = PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_GAMING_SINK,
        .supportedFrameDuration             = PACS_SUPPORTED_FRAME_DURATION_FOR_GAMING_SINK,
        .audioChannelCounts                 = PACS_SINK_AUDIO_CHANNEL_COUNT,
        .minSupportedOctetsPerCodecFrame    = PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SINK,
        .maxSupportedOctetsPerCodecFrame    = PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SINK,
        .supportedMaxCodecFramePerSdu       = PACS_GAMING_MAX_CODEC_FRAMES_PER_SDU,
        .metadataLength                     = ARRAY_DIM(gmap_metadata),
        .metadata                           = gmap_metadata
    },
#endif /* INCLUDE_LE_GAMING_MODE */
#endif /* INCLUDE_LE_AUDIO_UNICAST */
};

#ifdef INCLUDE_LE_AUDIO_UNICAST
/* PACS record for LE unicast source */
static const GattPacsServerRecordType default_source_pacs[] =
{
#ifndef DISABLE_LE_AUDIO_VOICE
    {
        /* Source PAC for Conversation and Live context 16_2, 32_1, 32_2 */
        .codecId                            = PACS_LC3_CODEC_ID,
        .companyId                          = 0x0000,
        .vendorCodecId                      = 0x0000,
        .supportedSamplingFrequencies       = PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_VOICE_SOURCE,
        .supportedFrameDuration             = PACS_SUPPORTED_FRAME_DURATION_FOR_VOICE_SOURCE,
        .audioChannelCounts                 = PACS_SOURCE_AUDIO_CHANNEL_COUNT,
        .minSupportedOctetsPerCodecFrame    = PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_VOICE_SOURCE,
        .maxSupportedOctetsPerCodecFrame    = PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_VOICE_SOURCE,
        .supportedMaxCodecFramePerSdu       = PACS_SOURCE_MAX_CODEC_FRAMES_PER_SDU,
        .metadataLength                     = ARRAY_DIM(metadata_voice),
        .metadata                           = metadata_voice
    },
#endif /* !DISABLE_LE_AUDIO_VOICE */
#ifdef INCLUDE_LE_STEREO_RECORDING
    {
        /* Source PAC for unicast stereo recording 48_2_2, 48_1 */
        .codecId                            = PACS_LC3_CODEC_ID,
        .companyId                          = 0x0000,
        .vendorCodecId                      = 0x0000,
        .supportedSamplingFrequencies       = PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_STEREO_RECORDING_SOURCE,
        .supportedFrameDuration             = PACS_SUPPORTED_FRAME_DURATION_FOR_STEREO_RECORDING_SOURCE,
        .audioChannelCounts                 = PACS_SOURCE_AUDIO_CHANNEL_COUNT,
        .minSupportedOctetsPerCodecFrame    = PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_STEREO_RECORDING_SOURCE,
        .maxSupportedOctetsPerCodecFrame    = PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_STEREO_RECORDING_SOURCE,
        .supportedMaxCodecFramePerSdu       = PACS_SOURCE_MAX_CODEC_FRAMES_PER_SDU,
        .metadataLength                     = ARRAY_DIM(wmap_metadata),
        .metadata                           = wmap_metadata
    },
#endif
#ifdef INCLUDE_LE_GAMING_MODE
    {
        /* Source PAC for unicast Gaming VBC 16_2, 16_1 */
        .codecId                            = PACS_LC3_CODEC_ID,
        .companyId                          = 0x0000,
        .vendorCodecId                      = 0x0000,
        .supportedSamplingFrequencies       = PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_GAMING_SOURCE,
        .supportedFrameDuration             = PACS_SUPPORTED_FRAME_DURATION_FOR_GAMING_SOURCE,
        .audioChannelCounts                 = PACS_SOURCE_AUDIO_CHANNEL_COUNT,
        .minSupportedOctetsPerCodecFrame    = PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SOURCE,
        .maxSupportedOctetsPerCodecFrame    = PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SOURCE,
        .supportedMaxCodecFramePerSdu       = PACS_SOURCE_MAX_CODEC_FRAMES_PER_SDU,
        .metadataLength                     = ARRAY_DIM(gmap_metadata),
        .metadata                           = gmap_metadata
    },
#endif /* INCLUDE_LE_GAMING_MODE */
};
#endif /* INCLUDE_LE_AUDIO_UNICAST */

/* PACS record for LE unicast/broadcast sink */
static const GattPacsServerRecordType default_sink_pacs_vs_lc3[] =
{
#ifndef DISABLE_LE_AUDIO_MEDIA
    {
        /* Sink PAC for Broadcast and unicast Media streaming only, 16_2, 24_2, 32_2, 32_1, 48_1/2/3/4/5/6 */
        .codecId                            = PACS_LC3_CODEC_ID,
        .companyId                          = 0x0000,
        .vendorCodecId                      = 0x0000,
        .supportedSamplingFrequencies       = PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_MEDIA_SINK,
        .supportedFrameDuration             = PACS_SUPPORTED_FRAME_DURATION_FOR_MEDIA_SINK,
        .audioChannelCounts                 = PACS_SINK_AUDIO_CHANNEL_COUNT,
        .minSupportedOctetsPerCodecFrame    = PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_MEDIA_SINK,
        .maxSupportedOctetsPerCodecFrame    = PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_MEDIA_SINK,
        .supportedMaxCodecFramePerSdu       = PACS_SINK_MAX_CODEC_FRAMES_PER_SDU,
        .metadataLength                     = ARRAY_DIM(metadata_audio_vs_lc3),
        .metadata                           = metadata_audio_vs_lc3
    },
#endif /* !DISABLE_LE_AUDIO_MEDIA */

#ifdef INCLUDE_LE_AUDIO_UNICAST
#ifndef DISABLE_LE_AUDIO_VOICE
    {
        /* Sink PAC for unicast Conversation and Live context, 16_2, 32_1, 32_2 */
        .codecId                            = PACS_LC3_CODEC_ID,
        .companyId                          = 0x0000,
        .vendorCodecId                      = 0x0000,
        .supportedSamplingFrequencies       = PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_VOICE_SINK,
        .supportedFrameDuration             = PACS_SUPPORTED_FRAME_DURATION_FOR_VOICE_SINK,
        .audioChannelCounts                 = PACS_SINK_AUDIO_CHANNEL_COUNT,
        .minSupportedOctetsPerCodecFrame    = PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_VOICE_SINK,
        .maxSupportedOctetsPerCodecFrame    = PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_VOICE_SINK,
        .supportedMaxCodecFramePerSdu       = PACS_SINK_MAX_CODEC_FRAMES_PER_SDU,
        .metadataLength                     = ARRAY_DIM(metadata_voice_vs_lc3),
        .metadata                           = metadata_voice_vs_lc3
    },
#endif /* !DISABLE_LE_AUDIO_VOICE */
#ifdef INCLUDE_LE_GAMING_MODE
    {
        /* Sink PAC for Gaming mode only, 48_1, 48_2 */
        .codecId                            = PACS_LC3_CODEC_ID,
        .companyId                          = 0x0000,
        .vendorCodecId                      = 0x0000,
        .supportedSamplingFrequencies       = PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_GAMING_SINK,
        .supportedFrameDuration             = PACS_SUPPORTED_FRAME_DURATION_FOR_GAMING_SINK,
        .audioChannelCounts                 = PACS_SINK_AUDIO_CHANNEL_COUNT,
        .minSupportedOctetsPerCodecFrame    = PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SINK,
        .maxSupportedOctetsPerCodecFrame    = PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SINK,
        .supportedMaxCodecFramePerSdu       = PACS_GAMING_MAX_CODEC_FRAMES_PER_SDU,
        .metadataLength                     = ARRAY_DIM(gmap_metadata_vs_lc3),
        .metadata                           = gmap_metadata_vs_lc3
    },
#endif /* INCLUDE_LE_GAMING_MODE */
#endif /* INCLUDE_LE_AUDIO_UNICAST */
};

#ifdef INCLUDE_LE_APTX_ADAPTIVE
static const uint8 metadata_audio_aptx[] = {3, BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS,
                                               AVAILABLE_AUDIO_CONTEXTS & 0xFF, AVAILABLE_AUDIO_CONTEXTS >> 8,
                                            15, BAP_METADATA_LTV_TYPE_VENDOR_SPECIFIC_CONTEXTS,
                                               (VS_METADATA_COMPANY_ID_QUALCOMM & 0xFF), (VS_METADATA_COMPANY_ID_QUALCOMM >> 8),
                                               VS_METADATA_LENGTH_APTX, VS_METADATA_TYPE_APTX,
                                               APTX_ENCODER_VERSION_ID, APTX_DECODER_VERSION_ID,
                                               APTX_LTV_MIN_FRAME_DURATION,
                                               APTX_LTV_FEATURE_SCALE_TO_LOSSLESS,
                                               0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

static const uint8 vs_codec_specific_caps_aptx[] = {3 , APTX_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_SAMPLING_FREQUENCIES,
                                                       (APTX_SUPPORTED_SAMPLING_FREQS & 0xff), APTX_SUPPORTED_SAMPLING_FREQS >> 8,
                                                    2, APTX_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_AUDIO_CHANNEL_COUNTS,
                                                       PACS_AUDIO_CHANNEL_1,
                                                    5, APTX_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
                                                       (APTX_SUPPORTED_OCTETS_MIN & 0xff), APTX_SUPPORTED_OCTETS_MIN >> 8,(APTX_SUPPORTED_OCTETS_MAX & 0xff), APTX_SUPPORTED_OCTETS_MAX >> 8
                                                    };

static const GattPacsServerVSPacRecord default_sink_pacs_vs_aptx[] =
{
    {
        .codecId                            = PACS_VENDOR_CODEC_ID,
        .companyId                          = PACS_VENDOR_COMPANY_ID_QCOM,
        .vendorCodecId                      = PACS_RECORD_VENDOR_CODEC_APTX_ADAPTIVE_R3,
        .vsConfigLen                        = ARRAY_DIM(vs_codec_specific_caps_aptx),
        .vsConfig                           = vs_codec_specific_caps_aptx,
        .metadataLength                     = ARRAY_DIM(metadata_audio_aptx),
        .metadata                           = metadata_audio_aptx,
    }
};
#endif

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
static const uint8 metadata_audio_aptx_lite[] = {3, BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS,
                                               AUDIO_CONTEXT_TYPE_GAME & 0xFF, AUDIO_CONTEXT_TYPE_GAME >> 8,
                                            15, BAP_METADATA_LTV_TYPE_VENDOR_SPECIFIC_CONTEXTS,
                                               (VS_METADATA_COMPANY_ID_QUALCOMM & 0xFF), (VS_METADATA_COMPANY_ID_QUALCOMM >> 8),
                                               VS_METADATA_LENGTH_APTX_LITE, VS_METADATA_TYPE_APTX_LITE,
                                               APTX_LITE_ENCODER_VERSION_ID, APTX_LITE_DECODER_VERSION_ID,
                                               APTX_LITE_DEVICE_TYPE_INFO,
                                               0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

static const uint8 vs_codec_specific_caps_aptx_lite_snk[] = {3 , APTX_LITE_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_SAMPLING_FREQUENCIES,
                                                       (PACS_APTX_LITE_SINK_SUPPORTED_SAMPLING_FREQS & 0xff), PACS_APTX_LITE_SINK_SUPPORTED_SAMPLING_FREQS >> 8,
                                                    2, APTX_LITE_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_AUDIO_CHANNEL_COUNTS,
                                                       PACS_SINK_AUDIO_CHANNEL_COUNT,
                                                    5, APTX_LITE_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
                                                       (PACS_APTX_LITE_SUPPORTED_OCTETS_MIN & 0xff), PACS_APTX_LITE_SUPPORTED_OCTETS_MIN >> 8,(PACS_APTX_LITE_SUPPORTED_OCTETS_MAX & 0xff), PACS_APTX_LITE_SUPPORTED_OCTETS_MAX >> 8
                                                    };

static const uint8 vs_codec_specific_caps_aptx_lite_src[] = {3 , APTX_LITE_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_SAMPLING_FREQUENCIES,
                                                       (PACS_APTX_LITE_SRC_SUPPORTED_SAMPLING_FREQS & 0xff), PACS_APTX_LITE_SRC_SUPPORTED_SAMPLING_FREQS >> 8,
                                                    2, APTX_LITE_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_AUDIO_CHANNEL_COUNTS,
                                                       PACS_SOURCE_AUDIO_CHANNEL_COUNT,
                                                    5, APTX_LITE_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,
                                                       (PACS_APTX_LITE_SUPPORTED_OCTETS_MIN & 0xff), PACS_APTX_LITE_SUPPORTED_OCTETS_MIN >> 8,(PACS_APTX_LITE_SUPPORTED_OCTETS_MAX & 0xff), PACS_APTX_LITE_SUPPORTED_OCTETS_MAX >> 8
                                                    };

static const GattPacsServerVSPacRecord vs_aptx_lite_sink_pacs[] =
{
    {
        .codecId                            = PACS_VENDOR_CODEC_ID,
        .companyId                          = PACS_VENDOR_COMPANY_ID_QCOM,
        .vendorCodecId                      = 0x0002, /* PACS_RECORD_VENDOR_CODEC_APTX_LITE */
        .vsConfigLen                        = ARRAY_DIM(vs_codec_specific_caps_aptx_lite_snk),
        .vsConfig                           = vs_codec_specific_caps_aptx_lite_snk,
        .metadataLength                     = ARRAY_DIM(metadata_audio_aptx_lite),
        .metadata                           = metadata_audio_aptx_lite,
    }
};

static const GattPacsServerVSPacRecord vs_aptx_lite_src_pacs[] =
{
    {
        .codecId                            = PACS_VENDOR_CODEC_ID,
        .companyId                          = PACS_VENDOR_COMPANY_ID_QCOM,
        .vendorCodecId                      = 0x0002, /* PACS_RECORD_VENDOR_CODEC_APTX_LITE*/
        .vsConfigLen                        = ARRAY_DIM(vs_codec_specific_caps_aptx_lite_src),
        .vsConfig                           = vs_codec_specific_caps_aptx_lite_src,
        .metadataLength                     = ARRAY_DIM(metadata_audio_aptx_lite),
        .metadata                           = metadata_audio_aptx_lite,
    }
};

#endif /* INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE */


#ifdef INCLUDE_LE_AUDIO_UNICAST
/* PACS record for LE unicast source */
static const GattPacsServerRecordType default_source_pacs_vs_lc3[] =
{
#ifndef DISABLE_LE_AUDIO_VOICE
    {
        /* Source PAC for Conversation and Live context 16_2, 32_1, 32_2 */
        .codecId                            = PACS_LC3_CODEC_ID,
        .companyId                          = 0x0000,
        .vendorCodecId                      = 0x0000,
        .supportedSamplingFrequencies       = PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_VOICE_SOURCE,
        .supportedFrameDuration             = PACS_SUPPORTED_FRAME_DURATION_FOR_VOICE_SOURCE,
        .audioChannelCounts                 = PACS_SOURCE_AUDIO_CHANNEL_COUNT,
        .minSupportedOctetsPerCodecFrame    = PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_VOICE_SOURCE,
        .maxSupportedOctetsPerCodecFrame    = PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_VOICE_SOURCE,
        .supportedMaxCodecFramePerSdu       = PACS_SOURCE_MAX_CODEC_FRAMES_PER_SDU,
        .metadataLength                     = ARRAY_DIM(metadata_voice_vs_lc3),
        .metadata                           = metadata_voice_vs_lc3
    },
#endif /* !DISABLE_LE_AUDIO_VOICE */
#ifdef INCLUDE_LE_STEREO_RECORDING
    {
        /* Source PAC for unicast stereo recording 48_2_2, 48_1 */
        .codecId                            = PACS_LC3_CODEC_ID,
        .companyId                          = 0x0000,
        .vendorCodecId                      = 0x0000,
        .supportedSamplingFrequencies       = PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_STEREO_RECORDING_SOURCE,
        .supportedFrameDuration             = PACS_SUPPORTED_FRAME_DURATION_FOR_STEREO_RECORDING_SOURCE,
        .audioChannelCounts                 = PACS_SOURCE_AUDIO_CHANNEL_COUNT,
        .minSupportedOctetsPerCodecFrame    = PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_STEREO_RECORDING_SOURCE,
        .maxSupportedOctetsPerCodecFrame    = PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_STEREO_RECORDING_SOURCE,
        .supportedMaxCodecFramePerSdu       = PACS_SOURCE_MAX_CODEC_FRAMES_PER_SDU,
        .metadataLength                     = ARRAY_DIM(wmap_metadata_vs_lc3),
        .metadata                           = wmap_metadata_vs_lc3
    },
#endif
#ifdef INCLUDE_LE_GAMING_MODE
    {
        /* Source PAC for unicast Gaming VBC 16_2, 16_1 */
        .codecId                            = PACS_LC3_CODEC_ID,
        .companyId                          = 0x0000,
        .vendorCodecId                      = 0x0000,
        .supportedSamplingFrequencies       = PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_GAMING_SOURCE,
        .supportedFrameDuration             = PACS_SUPPORTED_FRAME_DURATION_FOR_GAMING_SOURCE,
        .audioChannelCounts                 = PACS_SOURCE_AUDIO_CHANNEL_COUNT,
        .minSupportedOctetsPerCodecFrame    = PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SOURCE,
        .maxSupportedOctetsPerCodecFrame    = PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SOURCE,
        .supportedMaxCodecFramePerSdu       = PACS_SOURCE_MAX_CODEC_FRAMES_PER_SDU,
        .metadataLength                     = ARRAY_DIM(gmap_metadata_vs_lc3),
        .metadata                           = gmap_metadata_vs_lc3
    },
#endif /* INCLUDE_LE_GAMING_MODE */
};

/* Minimum presentation delay table for codecs LC3, Aptx R3 and Aptx Lite*/
static const BapServerCodecPdMin default_codec_pd_min[] =
{
#ifdef INCLUDE_LE_GAMING_MODE
    {
        /* PD entry for Gaming mode without VBC audio context */
        .targetLatency              = TARGET_LATENCY_TARGET_LOWER,
        .samplingFreq               = PACS_SAMPLING_FREQUENCY_48KHZ,
        .frameDuration              = PACS_SUPPORTED_FRAME_DURATION_FOR_GAMING_SINK,
        .sinkLc3PdMin               = BAP_PD_MIN_FOR_GAMING_SINK,
        .sourceLc3PdMin             = BAP_PD_MIN_FOR_GAMING_SOURCE,
        .sinkVsAptxPdMin            = BAP_PD_MIN_FOR_GAMING_APTX_LITE_SINK,
        .sourceVsAptxPdMin          = 0,
    },
    {
        /* PD entry for Gaming mode with VBC audio context */
        .targetLatency              = TARGET_LATENCY_TARGET_LOWER,
        .samplingFreq               = PACS_SAMPLING_FREQUENCY_16KHZ | PACS_SAMPLING_FREQUENCY_48KHZ,
        .frameDuration              = PACS_SUPPORTED_FRAME_DURATION_FOR_GAMING_SINK,
        .sinkLc3PdMin               = BAP_PD_MIN_FOR_GAMING_WITH_VBC_WB_SINK,
        .sourceLc3PdMin             = BAP_PD_MIN_FOR_GAMING_WITH_VBC_WB_SOURCE,
        .sinkVsAptxPdMin            = BAP_PD_MIN_FOR_GAMING_WITH_VBC_APTX_LITE_SINK,
        .sourceVsAptxPdMin          = BAP_PD_MIN_FOR_GAMING_WITH_VBC_APTX_LITE_SOURCE,
    },
#ifdef ENABLE_LEA_GAMING_MODE_VBC_32KHZ
    {
        /* PD entry for Gaming mode with VBC audio context */
        .targetLatency              = TARGET_LATENCY_TARGET_LOWER,
        .samplingFreq               = PACS_SAMPLING_FREQUENCY_32KHZ | PACS_SAMPLING_FREQUENCY_48KHZ,
        .frameDuration              = PACS_SUPPORTED_FRAME_DURATION_FOR_GAMING_SINK,
        .sinkLc3PdMin               = BAP_PD_MIN_FOR_GAMING_WITH_VBC_SWB_SINK,
        .sourceLc3PdMin             = BAP_PD_MIN_FOR_GAMING_WITH_VBC_SWB_SOURCE,
        .sinkVsAptxPdMin            = 0,
        .sourceVsAptxPdMin          = 0,
    },
#endif /* ENABLE_LEA_GAMING_MODE_VBC_32KHZ */
#endif /* INCLUDE_LE_GAMING_MODE */
    {
        /* PD entry for Conversation audio context */
        .targetLatency              = TARGET_LATENCY_TARGET_BALANCE_AND_RELIABLE,
        .samplingFreq               = PACS_SAMPLING_FREQUENCY_16KHZ | PACS_SAMPLING_FREQUENCY_24KHZ |
                                      PACS_SAMPLING_FREQUENCY_32KHZ,
        .frameDuration              = PACS_SUPPORTED_FRAME_DURATION_7P5MS | PACS_SUPPORTED_FRAME_DURATION_10MS,
        .sinkLc3PdMin               = BAP_PD_MIN_FOR_VOICE_SINK,
        .sourceLc3PdMin             = BAP_PD_MIN_FOR_VOICE_SOURCE,
        .sinkVsAptxPdMin            = 0,
        .sourceVsAptxPdMin          = 0,
    },
    {
        /* PD entry for Music and Live audio context */
        .targetLatency              = TARGET_LATENCY_TARGET_HIGHER_RELIABILITY,
        .samplingFreq               = PACS_SAMPLING_FREQUENCY_16KHZ | PACS_SAMPLING_FREQUENCY_24KHZ |
                                      PACS_SAMPLING_FREQUENCY_32KHZ | PACS_SAMPLING_FREQUENCY_48KHZ,
        .frameDuration              = PACS_SUPPORTED_FRAME_DURATION_7P5MS | PACS_SUPPORTED_FRAME_DURATION_10MS,
        .sinkLc3PdMin               = BAP_PD_MIN_FOR_MUSIC_SINK,
        .sourceLc3PdMin             = BAP_PD_MIN_FOR_STEREO_RECORDING_SOURCE,
        .sinkVsAptxPdMin            = BAP_PD_MIN_FOR_APTX_SINK,
        .sourceVsAptxPdMin          = 0,
    },
};
#endif /* INCLUDE_LE_AUDIO_UNICAST */

static ServiceHandle pacs_instance = PACS_INVALID_HANDLE;
le_bap_pacs_records_t *sink_records = NULL;
le_bap_pacs_records_t *source_records = NULL;


static void leBapPacsUtilities_OnGattConnect(gatt_cid_t cid);
static void leBapPacsUtilities_OnGattDisconnect(gatt_cid_t cid);
static void leBapPacsUtilities_OnEncryptionChanged(gatt_cid_t, bool encryption);

static const gatt_connect_observer_callback_t le_connect_callbacks =
{
    .OnConnection = leBapPacsUtilities_OnGattConnect,
    .OnDisconnection = leBapPacsUtilities_OnGattDisconnect,
    .OnEncryptionChanged = leBapPacsUtilities_OnEncryptionChanged
};


static ServiceHandle leBapPacsUtilities_GetPacsInstance(void)
{
    PanicZero(pacs_instance);
    return pacs_instance;
}

static pdd_size_t leBapPacsUtilities_GetDeviceDataLength(device_t device)
{
    void * config = NULL;
    size_t config_size = 0;

    if(Device_GetProperty(device, device_property_le_audio_pacs_config, &config, &config_size) == FALSE)
    {
        config_size = 0;
    }

    return config_size;
}

static void leBapPacsUtilities_SerialisetDeviceData(device_t device, void *buf, pdd_size_t offset)
{
    void * config = NULL;
    size_t config_size = 0;

    UNUSED(offset);

    if(Device_GetProperty(device, device_property_le_audio_pacs_config, &config, &config_size))
    {
        memcpy(buf, config, config_size);
    }
}

static void leBapPacsUtilities_DeserialisetDeviceData(device_t device, void *buf, pdd_size_t data_length, pdd_size_t offset)
{
    UNUSED(offset);
    Device_SetProperty(device, device_property_le_audio_pacs_config, buf, data_length);
}

static void leBapPacsUtilities_RegisterAsPersistentDeviceDataUser(void)
{
    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        PDDU_ID_PACS,
        leBapPacsUtilities_GetDeviceDataLength,
        leBapPacsUtilities_SerialisetDeviceData,
        leBapPacsUtilities_DeserialisetDeviceData);
}

void * LeBapPacsUtilities_RetrieveClientConfig(gatt_cid_t cid)
{
    device_t device = GattConnect_GetBtDevice(cid);
    void * device_config = NULL;

    if(device)
    {
        size_t size;
        PACS_UTILITIES_LOG("LeBapPacsUtilities_RetrieveClientConfig retrieving config device=0x%p", device);
        if (!Device_GetProperty(device, device_property_le_audio_pacs_config, &device_config, &size))
        {
            device_config = NULL;
        }
    }

    return device_config;
}

void LeBapPacsUtilities_StoreClientConfig(gatt_cid_t cid, void * config, uint8 size)
{
    device_t device = GattConnect_GetBtDevice(cid);

    if(device)
    {
        PACS_UTILITIES_LOG("LeBapPacsUtilities_StoreClientConfig storing config device=0x%p", device);
        Device_SetProperty(device, device_property_le_audio_pacs_config, config, size);
    }
}

static void leBapPacsUtilities_OnGattConnect(gatt_cid_t cid)
{
    UNUSED(cid);
}

static void leBapPacsUtilities_OnGattDisconnect(gatt_cid_t cid)
{
    PACS_UTILITIES_LOG("leBapPacsUtilities_OnGattDisconnect cid 0x%4x", cid);

    BapPacsConfig *config = BapServerRemovePacsConfig(pacs_instance, cid);

    if (config)
    {
        LeBapPacsUtilities_StoreClientConfig(cid, (void *)config, sizeof(*config));
        free(config);
    }
}

static void leBapPacsUtilities_OnEncryptionChanged(gatt_cid_t cid, bool encrypted)
{
    if (encrypted && !GattConnect_IsDeviceTypeOfPeer(cid))
    {
        PACS_UTILITIES_LOG("leBapPacsUtilities_OnEncryptionChanged cid 0x%4x encrypted %d", cid, encrypted);
        BapPacsConfig *config = NULL;

        config = (BapPacsConfig *)LeBapPacsUtilities_RetrieveClientConfig(cid);

        bapStatus status = BapServerAddPacsConfig(pacs_instance, cid, config);
        if (status != BAP_SERVER_STATUS_SUCCESS)
        {
            Panic();
        }
    }
}

static uint32 leBapPacsUtilities_GetAudioLocationByDeviceSide(bool for_sink)
{
    uint32 audio_locations = 0;

    switch (Multidevice_GetSide())
    {
        case multidevice_side_left:
            audio_locations = PACS_AUDIO_LOCATION_LEFT;
        break;

        case multidevice_side_right:
            audio_locations = PACS_AUDIO_LOCATION_RIGHT;
        break;

        case multidevice_side_both:
            audio_locations = PACS_AUDIO_LOCATION_LEFT;
            if (for_sink)
            {
                audio_locations |= PACS_AUDIO_LOCATION_RIGHT;
            }
        break;

        default:
        break;
    }

    return audio_locations;
}

static uint32 leBapPacsUtilities_GetAudioLocationByDeviceType(bool for_sink)
{
    uint32 audio_locations = 0;
    multidevice_type_t type = Multidevice_GetType();

#ifdef INCLUDE_LE_AUDIO_STEREO_CONFIG
    if (!for_sink && type == multidevice_type_pair)
    {
        /* Pair device are in Stereo sink configuration */
        audio_locations = PACS_AUDIO_LOCATION_LEFT;
        return audio_locations;
    }
#endif

    switch (type)
    {
        case multidevice_type_pair:
#ifdef ENABLE_LEA_CIS_DELEGATION
            audio_locations = PACS_AUDIO_LOCATION_LEFT | PACS_AUDIO_LOCATION_RIGHT;
        break;
#endif

        case multidevice_type_single:
            audio_locations = leBapPacsUtilities_GetAudioLocationByDeviceSide(for_sink);
        break;

        default:
        break;
    }

    PACS_UTILITIES_LOG("leBapPacsUtilities_GetAudioLocationByDeviceType for_sink: %d, audio_locations: 0x%x", for_sink, audio_locations);
    return audio_locations;
}

#ifdef INCLUDE_LE_AUDIO_UNICAST
static const GattPacsServerRecordType* leBapPacsUtilities_GetSourcePacs(uint16 *no_of_source_pacs)
{
    PACS_UTILITIES_LOG("leBapPacsUtilities_GetSourcePacs()");

    if (LeBapPacsUtilities_Lc3EpcLicenseCheck())
    {
         PACS_UTILITIES_LOG("leBapPacsUtilities_GetSourcePacs() VS LC3 License");
        *no_of_source_pacs = ARRAY_DIM(default_source_pacs_vs_lc3);
        return default_source_pacs_vs_lc3;
    }

    *no_of_source_pacs = ARRAY_DIM(default_source_pacs);
    return default_source_pacs;
}
#endif

static const GattPacsServerRecordType* leBapPacsUtilities_GetSinkPacs(uint16 *no_of_sink_pacs)
{
    PACS_UTILITIES_LOG("leBapPacsUtilities_GetSinkPacs()");

    if (LeBapPacsUtilities_Lc3EpcLicenseCheck())
    {
        PACS_UTILITIES_LOG("leBapPacsUtilities_GetSinkPacs() VS LC3 License");

        *no_of_sink_pacs = ARRAY_DIM(default_sink_pacs_vs_lc3);
        return default_sink_pacs_vs_lc3;
    }

    *no_of_sink_pacs = ARRAY_DIM(default_sink_pacs);
    return default_sink_pacs;
}


#if defined (INCLUDE_LE_APTX_ADAPTIVE) || defined (INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE)
static const GattPacsServerVSPacRecord* leBapPacsUtilities_GetSinkPacsVS(uint16 *no_of_sink_pacs, uint8 vs_codec)
{
    PACS_UTILITIES_LOG("leBapPacsUtilities_GetSinkPacsVS()");

#ifdef INCLUDE_LE_APTX_ADAPTIVE
    if (vs_codec == KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE && FeatureVerifyLicense(APTX_ADAPTIVE_LOSSLESS_DECODE))
    {
        PACS_UTILITIES_LOG("leBapPacsUtilities_GetSinkPacsVS() VS aptX");

        *no_of_sink_pacs = ARRAY_DIM(default_sink_pacs_vs_aptx);
        return default_sink_pacs_vs_aptx;
    }
#endif

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
    if (vs_codec == KYMERA_LE_AUDIO_CODEC_APTX_LITE && (FeatureVerifyLicense(APTX_ADAPTIVE_DECODE) || FeatureVerifyLicense(APTX_ADAPTIVE_MONO_DECODE)))
    {
        PACS_UTILITIES_LOG("leBapPacsUtilities_GetSinkPacsVS() VS APTX LITE");

        *no_of_sink_pacs = ARRAY_DIM(vs_aptx_lite_sink_pacs);
        return vs_aptx_lite_sink_pacs;
    }
#endif

    *no_of_sink_pacs = 0;
    return NULL;
}

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
static const GattPacsServerVSPacRecord* leBapPacsUtilities_GetSrcPacsVS(uint16 *no_of_src_pacs, uint8 vs_codec)
{
    PACS_UTILITIES_LOG("leBapPacsUtilities_GetSrcPacsVS()");

    if (vs_codec == KYMERA_LE_AUDIO_CODEC_APTX_LITE)
    {
        PACS_UTILITIES_LOG("leBapPacsUtilities_GetSrcPacsVS() VS APTX LITE");

        *no_of_src_pacs = ARRAY_DIM(vs_aptx_lite_src_pacs);
        return vs_aptx_lite_src_pacs;
    }

    *no_of_src_pacs = 0;
    return NULL;
}
#endif /* INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE */

static void leBapPacsUtilities_AddVsPacs(uint8 vs_codec, bool is_source)
{
    uint8 scan;
    uint16 no_of_pacs_vs;
    const GattPacsServerVSPacRecord *pacs_record_vs = NULL;

    if (!is_source)
    {
        pacs_record_vs = leBapPacsUtilities_GetSinkPacsVS(&no_of_pacs_vs, vs_codec);

        for (scan = 0; scan < no_of_pacs_vs; scan++)
        {
            LeBapPacsUtilities_AddPacRecord(PACS_DIRECTION_AUDIO_SINK, &pacs_record_vs[scan], TRUE);
        }
    }
#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
    else
    {
        pacs_record_vs = leBapPacsUtilities_GetSrcPacsVS(&no_of_pacs_vs, vs_codec);

        for (scan = 0; scan < no_of_pacs_vs; scan++)
        {
            LeBapPacsUtilities_AddPacRecord(PACS_DIRECTION_AUDIO_SOURCE, &pacs_record_vs[scan], TRUE);
        }
    }
#endif /* INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE */
}
#endif /* defined (INCLUDE_LE_APTX_ADAPTIVE) || defined (INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE) */



static void leBapPacsUtilities_SetupDefaultPacs(void)
{
    uint8 scan;
    ServiceHandle pacs_handle = leBapPacsUtilities_GetPacsInstance();
    uint16 no_of_pacs;
    const GattPacsServerRecordType *pacs_record = NULL;

#ifdef INCLUDE_LE_AUDIO_UNICAST
    pacs_record = leBapPacsUtilities_GetSourcePacs(&no_of_pacs);

    PACS_UTILITIES_LOG("leBapPacsUtilities_SetupDefaultPacs(): Initialize source PAC records");

    for (scan = 0; scan < no_of_pacs; scan++)
    {
        LeBapPacsUtilities_AddPacRecord(PACS_DIRECTION_AUDIO_SOURCE, &pacs_record[scan], FALSE);
    }

    PanicFalse(BapServerAddPacAudioLocation(pacs_handle, PACS_DIRECTION_AUDIO_SOURCE, leBapPacsUtilities_GetAudioLocationByDeviceType(FALSE)));
    PACS_UTILITIES_LOG("leBapPacsUtilities_SetupPacs source location=0x%x",
                       BapServerGetPacAudioLocation(pacs_handle, PACS_DIRECTION_AUDIO_SOURCE));

    PanicFalse(BapServerAddPacAudioContexts(pacs_handle, PACS_DIRECTION_AUDIO_SOURCE, SOURCE_SUPPORTED_AUDIO_CONTEXTS, 
                                            PACS_SUPPORTED_AUDIO_CONTEXTS));
    PanicFalse(BapServerAddPacAudioContexts(pacs_handle, PACS_DIRECTION_AUDIO_SOURCE, SOURCE_AVAILABLE_AUDIO_CONTEXTS,
                                            PACS_AVAILABLE_AUDIO_CONTEXTS));
    PACS_UTILITIES_LOG("leBapPacsUtilities_SetupPacs source supported context=0x%x",SOURCE_SUPPORTED_AUDIO_CONTEXTS);

    PACS_UTILITIES_LOG("leBapPacsUtilities_SetupPacs source availability=0x%x",
                       BapServerGetPacAvailableContexts(pacs_handle, PACS_DIRECTION_AUDIO_SOURCE));

    PanicFalse(BapServerUnicastRegisterCodecPdMin(pacs_handle, default_codec_pd_min, ARRAY_DIM(default_codec_pd_min)));

#endif /* INCLUDE_LE_AUDIO_UNICAST */

    PACS_UTILITIES_LOG("leBapPacsUtilities_SetupDefaultPacs(): Initialize Sink PAC records");

    pacs_record = leBapPacsUtilities_GetSinkPacs(&no_of_pacs);

    for (scan = 0; scan < no_of_pacs; scan++)
    {
        LeBapPacsUtilities_AddPacRecord(PACS_DIRECTION_AUDIO_SINK, &pacs_record[scan], FALSE);
    }

#ifdef INCLUDE_LE_APTX_ADAPTIVE
    leBapPacsUtilities_AddVsPacs(KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE, FALSE);
#endif

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
    leBapPacsUtilities_AddVsPacs(KYMERA_LE_AUDIO_CODEC_APTX_LITE, FALSE);
    leBapPacsUtilities_AddVsPacs(KYMERA_LE_AUDIO_CODEC_APTX_LITE, TRUE);
#endif

    PanicFalse(BapServerAddPacAudioLocation(pacs_handle, PACS_DIRECTION_AUDIO_SINK, leBapPacsUtilities_GetAudioLocationByDeviceType(TRUE)));
    PACS_UTILITIES_LOG("leBapPacsUtilities_SetupPacs sink location=0x%x",
                       BapServerGetPacAudioLocation(pacs_handle, PACS_DIRECTION_AUDIO_SINK));

    PanicFalse(BapServerAddPacAudioContexts(pacs_handle, PACS_DIRECTION_AUDIO_SINK, SINK_SUPPORTED_AUDIO_CONTEXTS,
                                            PACS_SUPPORTED_AUDIO_CONTEXTS));
    PanicFalse(BapServerAddPacAudioContexts(pacs_handle, PACS_DIRECTION_AUDIO_SINK, SINK_AVAILABLE_AUDIO_CONTEXTS,
                                            PACS_AVAILABLE_AUDIO_CONTEXTS));
    PACS_UTILITIES_LOG("leBapPacsUtilities_SetupPacs Sink supported context=0x%x",SINK_SUPPORTED_AUDIO_CONTEXTS);
    PACS_UTILITIES_LOG("leBapPacsUtilities_SetupPacs sink availability=0x%x",
                       BapServerGetPacAvailableContexts(pacs_handle, PACS_DIRECTION_AUDIO_SINK));

}

void LeBapPacsSetBapHandle(ServiceHandle bap_handle)
{
    pacs_instance = bap_handle;
}

void LeBapPacsUtilities_Init(void)
{
    PACS_UTILITIES_LOG("LeBapPacsUtilities_Init");
    if (sink_records == NULL)
    {
        /* LeBapPacsUtilities_Init shall be called after LeBapScanDelegator_Init */ 
        PanicFalse(pacs_instance != PACS_INVALID_HANDLE);

        leBapPacsUtilities_RegisterAsPersistentDeviceDataUser();

        GattConnect_RegisterObserver(&le_connect_callbacks);

        leBapPacsUtilities_SetupDefaultPacs();
    }
}

void LeBapPacsUtilities_AddPacRecord(PacsServerDirectionType sink_or_source, const void *additional_pac, bool vendor_specific)
{
    ServiceHandle pacs_handle = leBapPacsUtilities_GetPacsInstance();
    le_bap_pacs_records_t *pac_records = NULL;

    /* Check if we are modifying the Sink or Source PAC record structure. Panic if we can't tell. */
    if (sink_or_source == PACS_DIRECTION_AUDIO_SINK)
    {
        pac_records = sink_records;
    }
    else if (sink_or_source == PACS_DIRECTION_AUDIO_SOURCE)
    {
        pac_records = source_records;
    }
    else
    {
        Panic();
    }

    /* At this point the 'pac_records' pointer must be valid. Next we determine if we are allocating or re-allocating memory. */
    if (pac_records == NULL)
    {
        pac_records = (le_bap_pacs_records_t *) PanicNull(calloc(1, (sizeof(le_bap_pacs_records_t) + sizeof(uint16))));
    }
    else
    {
        pac_records = (le_bap_pacs_records_t *) PanicNull(realloc(pac_records, (sizeof(le_bap_pacs_records_t) +
                                                                  (sizeof(uint16) * (pac_records->handle_count + 1)))));
    }

    if (vendor_specific == FALSE)
    {
        pac_records->handles[pac_records->handle_count] =
                BapServerAddPacRecord(pacs_handle, (PacsDirectionType)sink_or_source, (BapServerPacsRecord *)additional_pac);
    }
#if defined (INCLUDE_LE_APTX_ADAPTIVE) || defined (INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE)
    else
    {
        pac_records->handles[pac_records->handle_count] =
                BapServerAddVSPacRecord(pacs_handle, (PacsDirectionType)sink_or_source, (BapServerVSPacsRecord *)additional_pac);
    }
#endif
    if (pac_records->handles[pac_records->handle_count] < PACS_RECORD_INVALID_PARAMETERS)
    {
        PACS_UTILITIES_LOG("LeBapPacsUtilities_AddSinkPacRecord: Registered Records=0x%x New Handle=0x%x",
                           pac_records->handle_count,
                           pac_records->handles[pac_records->handle_count]);
        pac_records->handle_count++;
    }
    else
    {
        PACS_UTILITIES_LOG("LeBapPacsUtilities_AddSinkPacRecord: Failed 0x%04x", pac_records->handles[pac_records->handle_count]);
        Panic();
    }

    /* Assign local PAC record pointer to static PAC struct. */
    if (sink_or_source == PACS_DIRECTION_AUDIO_SINK)
    {
        sink_records = pac_records;
    }
    else if (sink_or_source == PACS_DIRECTION_AUDIO_SOURCE)
    {
        source_records = pac_records;
    }
    else
    {
        Panic();
    }
}

const le_bap_pacs_records_t* LeBapPacsUtilities_GetPacHandles(bool for_sink)
{
    return for_sink ? sink_records : source_records;
}

uint16 LeBapPacsUtilities_GetSinkAudioContextAvailability(void)
{
    return BapServerGetPacAvailableContexts(leBapPacsUtilities_GetPacsInstance(), PACS_DIRECTION_AUDIO_SINK);
}

void LeBapPacsUtililties_SetSinkAudioContextAvailability(uint16 audio_contexts)
{

    PACS_UTILITIES_LOG("LeBapPacsUtililties_SetSinkAudioContextAvailability audio_contexts=0x%x", audio_contexts);

    if(BapServerAddPacAudioContexts(leBapPacsUtilities_GetPacsInstance(), PACS_DIRECTION_AUDIO_SINK,
                              audio_contexts, PACS_AVAILABLE_AUDIO_CONTEXTS) == FALSE)
    {
        PACS_UTILITIES_LOG("LeBapPacsUtililties_SetSinkAudioContextAvailability failed");
        Panic();
    }
}

bool LeBapPacsUtilities_IsSinkAudioContextAvailable(PacsAudioContextType audio_context)
{
    return (audio_context && (LeBapPacsUtilities_GetSinkAudioContextAvailability() & audio_context) == audio_context);
}

static inline bool leBapPacsUtilities_IsVendorSpecificCodecMatch(uint8 coding_format, uint16 company_id,
                                                                 uint16 vendor_specific_codec_id,
                                                                 const GattPacsServerRecordType * pac_record)
{
    return (coding_format == PACS_VENDOR_CODEC_ID
            && coding_format == pac_record->codecId
            && company_id == pac_record->companyId
            && vendor_specific_codec_id == pac_record->vendorCodecId);
}

static inline bool leBapPacsUtilities_IsStandardCodecMatch(uint8 coding_format, uint16 company_id,
                                                           uint16 vendor_specific_codec_id,
                                                           const GattPacsServerRecordType * pac_record)
{
    return (coding_format != PACS_VENDOR_CODEC_ID
            && coding_format == pac_record->codecId
            && company_id == 0x00
            && vendor_specific_codec_id == 0x00);
}


bool LeBapPacsUtilities_IsCodecIdAndSpecificCapabilitiesSupportedBySink(uint8 coding_format, 
                                                                        uint16 company_id, 
                                                                        uint16 vendor_specific_codec_id,
                                                                        PacsSamplingFrequencyType sampling_frequency,
                                                                        PacsFrameDurationType frame_duration,
                                                                        uint16 octets_per_frame)
{
    bool is_supported = FALSE;
    for(int i = 0; ((i < sink_records->handle_count) && (is_supported == FALSE)); i++)
    {
        PACS_UTILITIES_LOG("LeBapPacsUtilities_IsCodecIdAndSpecificCapabilitiesSupportedBySink checking handle[%d]=0x%x",
                           i, sink_records->handles[i]);

        const BapServerPacsRecord * pac_record = BapServerGetPacRecord( leBapPacsUtilities_GetPacsInstance(), sink_records->handles[i]);
        if(pac_record)
        {
            if(coding_format == pac_record->codecId
                && company_id == pac_record->companyId
                && vendor_specific_codec_id == pac_record->vendorCodecId)
            {
                if ((sampling_frequency & pac_record->supportedSamplingFrequencies) &&
                    (frame_duration & pac_record->supportedFrameDuration) &&
                    (octets_per_frame >= pac_record->minSupportedOctetsPerCodecFrame) &&
                    (octets_per_frame <= pac_record->maxSupportedOctetsPerCodecFrame))
                {
                    is_supported = TRUE;
                    break;
                }
            }
        }
        else
        {
            PACS_UTILITIES_LOG("LeBapPacsUtilities_IsCodecIdAndSpecificCapabilitiesSupportedBySink no record found handle[%d]=0x%x",
                               i, sink_records->handles[i]);
        }
        
    }

    return is_supported;
}

uint16 LeBapPacsUtilities_GetSourceAudioContextAvailability(void)
{
    return BapServerGetPacAvailableContexts(leBapPacsUtilities_GetPacsInstance(), PACS_DIRECTION_AUDIO_SOURCE);
}

void LeBapPacsUtililties_SetSourceAudioContextAvailability(uint16 audio_contexts)
{
    PACS_UTILITIES_LOG("LeBapPacsUtililties_SetSourceAudioContextAvailability audio_contexts=0x%x", audio_contexts);

    if(BapServerAddPacAudioContexts(leBapPacsUtilities_GetPacsInstance(), PACS_DIRECTION_AUDIO_SOURCE,
                              audio_contexts, PACS_AVAILABLE_AUDIO_CONTEXTS) == FALSE)
    {
        PACS_UTILITIES_LOG("LeBapPacsUtililties_SetSourceAudioContextAvailability failed");
        Panic();
    }
}

bool LeBapPacsUtilities_IsSourceAudioContextAvailable(PacsAudioContextType audio_context)
{
    return ((LeBapPacsUtilities_GetSourceAudioContextAvailability() & audio_context) == audio_context);
}

uint32 LeBapPacsUtilities_GetSinkAudioLocation(void)
{
    return BapServerGetPacAudioLocation(leBapPacsUtilities_GetPacsInstance(), PACS_DIRECTION_AUDIO_SINK);
}

void LeBapPacsUtilities_ClaimSinkAudioContext(AudioContextType audio_context)
{
    PACS_UTILITIES_LOG("LeBapPacsUtilities_ClaimSinkAudioContext audio_context=0x%x", audio_context);

    if (!BapServerRemovePacAudioContexts(leBapPacsUtilities_GetPacsInstance(),
                                         PACS_DIRECTION_AUDIO_SINK,
                                         audio_context,
                                         PACS_AVAILABLE_AUDIO_CONTEXTS))
    {
        PACS_UTILITIES_LOG("LeBapPacsUtilities_ClaimSinkAudioContext failed");
        Panic();
    }
}

void LeBapPacsUtilities_ClaimSourceAudioContext(AudioContextType audio_context)
{
    PACS_UTILITIES_LOG("LeBapPacsUtilities_ClaimSourceAudioContext audio_context=0x%x", audio_context);

    if (!BapServerRemovePacAudioContexts(leBapPacsUtilities_GetPacsInstance(),
                                         PACS_DIRECTION_AUDIO_SOURCE,
                                         audio_context,
                                         PACS_AVAILABLE_AUDIO_CONTEXTS))
    {
        PACS_UTILITIES_LOG("LeBapPacsUtilities_ClaimSourceAudioContext failed");
        Panic();
    }
}

void LeBapPacsUtilities_RestoreSinkAudioContext(AudioContextType audio_context)
{
    PACS_UTILITIES_LOG("LeBapPacsUtilities_RestoreSinkAudioContext audio_context=0x%x", audio_context);

    if (!BapServerAddPacAudioContexts(leBapPacsUtilities_GetPacsInstance(),
                                        PACS_DIRECTION_AUDIO_SINK,
                                        audio_context,
                                        PACS_AVAILABLE_AUDIO_CONTEXTS))
    {
        PACS_UTILITIES_LOG("LeBapPacsUtilities_RestoreSinkAudioContext failed");
        Panic();
    }
}

void LeBapPacsUtilities_RestoreSourceAudioContext(AudioContextType audio_context)
{
    PACS_UTILITIES_LOG("LeBapPacsUtilities_RestoreSourceAudioContext audio_context=0x%x", audio_context);

    if (!BapServerAddPacAudioContexts(leBapPacsUtilities_GetPacsInstance(),
                                      PACS_DIRECTION_AUDIO_SOURCE,
                                      audio_context,
                                      PACS_AVAILABLE_AUDIO_CONTEXTS))
    {
        PACS_UTILITIES_LOG("LeBapPacsUtilities_RestoreSourceAudioContext failed");
        Panic();
    }
}

bool LeBapPacsUtilities_Lc3EpcLicenseCheck(void)
{
    return FeatureVerifyLicense(LC3_EPC_FEATURE_ID);
}

static uint16 leBapPacsUtilities_Lc3GetNumberOfAudioChannels(uint16 audio_channel_mask)
{
    uint16 channel_count = 0;

    while (audio_channel_mask > 0)
    {
        channel_count++;
        audio_channel_mask &= (audio_channel_mask - 1);
    }

    return channel_count;
}

uint16 LeBapPacsUtilities_Lc3GetFrameLength(uint16 max_sdu_size, uint16 blocks_per_sdu, uint16 audio_channel_mask)
{ 
    uint16 frame_length = max_sdu_size;

    if (audio_channel_mask != 0 && blocks_per_sdu != 0)
    {
        frame_length /= leBapPacsUtilities_Lc3GetNumberOfAudioChannels(audio_channel_mask) * blocks_per_sdu;
    }

    return frame_length;
}

PacsSamplingFrequencyType LeBapPacsUtilities_GetPacsSamplingFreqBitMaskFromFreq(uint32 sampling_frequency)
{
    PacsSamplingFrequencyType pacs_frequency;

    switch (sampling_frequency)
    {
        case 8000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_8KHZ;
        break;

        case 16000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_16KHZ;
        break;

        case 24000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_24KHZ;
        break;

        case 32000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_32KHZ;
        break;

        case 44100:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_44_1KHZ;
        break;

        case 48000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_48KHZ;
        break;
        case 96000:
            pacs_frequency = PACS_SAMPLING_FREQUENCY_96KHZ;
        break;

        default:
            pacs_frequency = (PacsSamplingFrequencyType) 0;
        break;
    }

    return pacs_frequency;
}

const GattPacsServerRecordType* LeBapPacsUtilities_GetPacs(bool is_source, uint16 *pacs_count)
{
    const GattPacsServerRecordType* pacs_record = NULL;

    if (is_source)
    {
#ifdef INCLUDE_LE_AUDIO_UNICAST
        pacs_record = leBapPacsUtilities_GetSourcePacs(pacs_count);
#else
        *pacs_count = 0;
#endif
    }
    else
    {
        pacs_record = leBapPacsUtilities_GetSinkPacs(pacs_count);
    }

    return pacs_record;
}


AudioContextType LeBapPacsUtilities_GetPreferredAudioContext(const uint8 *pac_metadata , uint8 pac_metadata_len)
{
    AudioContextType audiocontext = 0;
    const uint8 *metadata, *metadata_end;

    for (metadata = pac_metadata, metadata_end = pac_metadata + pac_metadata_len;
         metadata < metadata_end; metadata += metadata[0])
    {
        if (metadata[1] == PACS_PREFERRED_AUDIO_CONTEXTS_TYPE)
        {
            audiocontext = ((uint16) metadata[3] << 8) | metadata[2];
            break;
        }
    }

    return audiocontext;
}

#if (defined (INCLUDE_LE_APTX_ADAPTIVE) || defined (INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE)) && defined (USE_SYNERGY)
const GattPacsServerVSPacRecord* LeBapPacsUtilities_GetPacsVS(bool is_source, uint16 *pacs_count, uint8 vs_codec)
{
    const GattPacsServerVSPacRecord* pacs_record = NULL;
    *pacs_count = 0;

    if (!is_source)
    {
        pacs_record = leBapPacsUtilities_GetSinkPacsVS(pacs_count, vs_codec);
    }
#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
    else
    {
        pacs_record = leBapPacsUtilities_GetSrcPacsVS(pacs_count, vs_codec);
    }
#endif /* INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE */
    return pacs_record;
}

PacsSamplingFrequencyType LeBapPacsUtilities_GetSampleRateFromVSPac(const GattPacsServerVSPacRecord *pacs_record)
{
    PacsSamplingFrequencyType supported_rates = 0;

    BapServerLtvUtilitiesFindLtvValue((uint8*)pacs_record->vsConfig, pacs_record->vsConfigLen, SAMPLING_FREQUENCY_TYPE, (uint8*)&supported_rates, 2);

    return supported_rates;
}
#endif /* INCLUDE_LE_APTX_ADAPTIVE */
