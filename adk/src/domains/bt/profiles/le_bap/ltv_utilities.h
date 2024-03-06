/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_bap
    \brief      Utility functions for extracting information from LTV (length, type, value) data
    @{
*/

#ifndef LTV_UTILITIES_H_
#define LTV_UTILITIES_H_

#ifdef USE_SYNERGY
#include "bap_server_lib.h"
#else
#include "bap_server.h"
#endif

#define BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS                      0x01
#define BAP_METADATA_LTV_TYPE_STREAMING_AUDIO_CONTEXTS                      0x02
#define BAP_METADATA_LTV_TYPE_CCID_LIST                                     0x05

/* LC3 VS LTV Metadata defines, Refer 4.5.2.3.1 VS Metadata Type 0x10: Supported Features LTV for LC3 of QBCE SPEC */

#define BAP_METADATA_LTV_TYPE_VENDOR_SPECIFIC_CONTEXTS                     0xFF
#define VS_METADATA_COMPANY_ID_QUALCOMM                                    0x000A
#define VS_METADATA_LC3_COMPANY_ID_QUALCOMM_SIZE                           0x02
#define VS_METADATA_LC3_VALUE_LENGTH                                       0x0A
#define VS_METADATA_LTV_SIZE_LC3                                           (VS_METADATA_LENGTH_LC3 + VS_METADATA_LC3_COMPANY_ID_QUALCOMM_SIZE + 2)
#define VS_METADATA_LENGTH_LC3                                             (VS_METADATA_LC3_VALUE_LENGTH + 1)
#define VS_METADATA_TYPE_LC3                                               0x10
#define LC3_CODEC_VERSION_ID                                               0x01
#define LC3_ENCODER_VERSION_ID                                             0x01

#define LC3_DECODER_VERSION_STANDARD  0x00 // Don't advertise advanced decoder
#define LC3_DECODER_VERSION_EPC       0x01 // LC3 decoder with EPC
#define LC3_DECODER_VERSION_FT        0x02 // LC3 decoder with Flush Timeout updates

#define LC3_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY                        0x01
#define LC3_CODEC_CONFIG_LTV_LEN_SAMPLING_FREQUENCY                         0x03

#define LC3_CODEC_CONFIG_LTV_TYPE_FRAME_DURATION                            0x02
#define LC3_CODEC_CONFIG_LTV_LEN_FRAME_DURATION                             0x02

#define LC3_CODEC_CONFIG_LTV_TYPE_AUDIO_CHANNEL_ALLOCATION                  0x03
#define LC3_CODEC_CONFIG_LTV_LEN_AUDIO_CHANNEL_ALLOCATION                   0x02

#define LC3_CODEC_CONFIG_LTV_TYPE_OCTETS_PER_FRAME_CODEC                    0x04
#define LC3_CODEC_CONFIG_LTV_LEN_OCTETS_PER_FRAME_CODEC                     0x05

#define LC3_CODEC_CONFIG_LTV_TYPE_CODEC_FRAME_BLOCKS_PER_SDU                0x05
#define LC3_CODEC_CONFIG_LTV_LEN_CODEC_FRAME_BLOCKS_PER_SDU                 0x02

#define LC3_CODEC_CONFIG_SPECIFIC_CAPABILTIES_LENGTH                        \
    (1 + LC3_CODEC_CONFIG_LTV_LEN_SAMPLING_FREQUENCY +                      \
     1 + LC3_CODEC_CONFIG_LTV_LEN_FRAME_DURATION +                          \
     1 + LC3_CODEC_CONFIG_LTV_LEN_AUDIO_CHANNEL_ALLOCATION +                \
     1 + LC3_CODEC_CONFIG_LTV_LEN_OCTETS_PER_FRAME_CODEC +                  \
     1 + LC3_CODEC_CONFIG_LTV_LEN_CODEC_FRAME_BLOCKS_PER_SDU)

/* aptx supported features LTV struct values */
#define APTX_LTV_MIN_FRAME_DURATION_15MS                                    0x3c  // 15ms (resolution .25ms)
#define APTX_LTV_MIN_FRAME_DURATION_10MS                                    0x28
#define APTX_LTV_MIN_FRAME_DURATION                                         APTX_LTV_MIN_FRAME_DURATION_10MS
#define APTX_LTV_FEATURE_SCALE_TO_LOSSLESS                                  0x01

#define VS_METADATA_LTV_SIZE_APTX                                          (VS_METADATA_LENGTH_APTX+VS_METADATA_LC3_COMPANY_ID_QUALCOMM_SIZE)
#define VS_METADATA_LENGTH_APTX                                            0x0B
#define VS_METADATA_TYPE_APTX                                              0x11
#define APTX_ENCODER_VERSION_ID                                            0x00
#define APTX_DECODER_VERSION_ID                                            0x02
#define APTX_ADAPTIVE_VS_CODEC_ID                                          0x01

#define APTX_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_SAMPLING_FREQUENCIES      0x01
#define APTX_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_AUDIO_CHANNEL_COUNTS      0x03
#define APTX_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME    0x04

#define APTX_CODEC_CONFIG_LTV_TYPE_SELECTED_OCTETS_PER_CODEC_FRAME           0x04
#define APTX_CODEC_CONFIG_LTV_TYPE_SELECTED_SAMPLING_FREQUENCY               0x81
#define APTX_CODEC_CONFIG_LTV_TYPE_SELECTED_CHANNEL_ALLOCATION               0x83

/* bit[0]: 0 = Headset, 1 = EB , bit[1]: 0 = mono VBC, 1= stereo VBC */
#ifdef INCLUDE_STEREO
#define DEVICE_TYPE_INFO                                                     0x00
#else
#define DEVICE_TYPE_INFO                                                     0x01
#endif

#define VS_METADATA_LENGTH_APTX_LITE                                               0x0B
#define VS_METADATA_TYPE_APTX_LITE                                                 0x12

#define APTX_LITE_VS_CODEC_ID                                                      0x0002
#define APTX_LITE_ENCODER_VERSION_ID                                               0x00
#define APTX_LITE_DECODER_VERSION_ID                                               0x00
#define APTX_LITE_DEVICE_TYPE_INFO                                                 DEVICE_TYPE_INFO

#define APTX_LITE_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_SAMPLING_FREQUENCIES       0x01
#define APTX_LITE_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_AUDIO_CHANNEL_COUNTS       0x03
#define APTX_LITE_CODEC_CAPABILITIES_LTV_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME     0x04


/*! \brief LE Audio context types as defined in BAPS_Assigned_Numbers_v12 */
typedef enum
{
    audio_context_type_unknown              = 0x0000,
    audio_context_type_unspecified          = 0x0001,
    audio_context_type_conversational       = 0x0002,
    audio_context_type_media                = 0x0004,
    audio_context_type_game                 = 0x0008,
    audio_context_type_instructional        = 0x0010,
    audio_context_type_voice_assistant      = 0x0020,
    audio_context_type_live                 = 0x0040,
    audio_context_type_sound_effects        = 0x0080,
    audio_context_type_notifications        = 0x0100,
    audio_context_type_ringtone             = 0x0200,
    audio_context_type_alerts               = 0x0400,
    audio_context_type_emergency_alarm      = 0x0800,
} audio_context_t;

/*! \brief Utility function to get the LC3 version from VS metadata

    \param metadata Metadata field in ASE operation
    \param metadata_length length of the Metadata in ASE opeation

    \returns uint8 lc3 version if VS LC3 present otherwise 0 
 */

uint8 LtvUtilities_FindLc3EncoderVersionFromVsMetadata(uint8 * metadata, uint8 metadata_length);

/*! \brief Find Content Control Id from the metadata

    \param metadata Metadata field in ASE operation
    \param metadata_length length of the Metadata in ASE opeation

    \returns uin8* ccid list including type and length if present, otherwise NULL.
*/
uint8* LtvUtilities_FindContentControlIdList(uint8 * metadata, uint8 metadata_length, uint8 *ccid_list_length);

#endif /* LTV_UTILITIES_H_ */
/*! @} */