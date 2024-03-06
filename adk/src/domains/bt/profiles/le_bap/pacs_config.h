/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_bap
    \brief      Common PACS configuration settings defines for LE Audio
    @{
*/

#ifndef PACS_CONFIG_H_
#define PACS_CONFIG_H_

#ifdef USE_SYNERGY
#include "bap_server_lib.h"
#else
#include "bap_server.h"
#endif

#ifdef INCLUDE_LE_AUDIO_STEREO_CONFIG

#ifdef ENABLE_LE_AUDIO_CSIP
#define PACS_SINK_AUDIO_CHANNEL_COUNT                           (PACS_AUDIO_CHANNEL_1)
#else /* ENABLE_LE_AUDIO_CSIP */
#define PACS_SINK_AUDIO_CHANNEL_COUNT                           (PACS_AUDIO_CHANNELS_2 | PACS_AUDIO_CHANNEL_1)
#endif /* ENABLE_LE_AUDIO_CSIP */

#define PACS_SOURCE_AUDIO_CHANNEL_COUNT                         (PACS_AUDIO_CHANNEL_1)
#define PACS_GAMING_MAX_CODEC_FRAMES_PER_SDU                    (4)
#define LC3_EPC_FEATURE_ID                                      (LC3_EPC_HEADSET)

#else /* INCLUDE_LE_AUDIO_STEREO_CONFIG */

#define PACS_SINK_AUDIO_CHANNEL_COUNT                           (PACS_AUDIO_CHANNEL_1)
#define PACS_SOURCE_AUDIO_CHANNEL_COUNT                         (PACS_AUDIO_CHANNEL_1)
#define PACS_GAMING_MAX_CODEC_FRAMES_PER_SDU                    (2)
#define LC3_EPC_FEATURE_ID                                      (LC3_EPC_EARBUD)

#endif /* INCLUDE_LE_AUDIO_STEREO_CONFIG */

#ifdef INCLUDE_FRAME_DURATION_7P5MS
#define SUPPORTED_FRAME_DURATION_7P5MS                          (PACS_SUPPORTED_FRAME_DURATION_7P5MS)
#else
#define SUPPORTED_FRAME_DURATION_7P5MS                          (0x00)
#endif

#ifndef DISABLE_LE_AUDIO_MEDIA

/*! PACS parameter for media sink */
#define PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_MEDIA_SINK        (PACS_SAMPLING_FREQUENCY_48KHZ | PACS_SAMPLING_FREQUENCY_32KHZ | \
                                                                 PACS_SAMPLING_FREQUENCY_24KHZ | PACS_SAMPLING_FREQUENCY_16KHZ)
#define PACS_SUPPORTED_FRAME_DURATION_FOR_MEDIA_SINK            (PACS_SUPPORTED_FRAME_DURATION_10MS | SUPPORTED_FRAME_DURATION_7P5MS | \
                                                                 PACS_PREFERRED_FRAME_DURATION_10MS)
#define PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_MEDIA_SINK         (30)
#define PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_MEDIA_SINK         (155)

#define SUPPORT_MEDIA_CONTEXTS                                  (AUDIO_CONTEXT_TYPE_MEDIA)

#else /* DISABLE_LE_AUDIO_MEDIA */

#define SUPPORT_MEDIA_CONTEXTS                                  (AUDIO_CONTEXT_TYPE_UNKNOWN)

#endif /* !DISABLE_LE_AUDIO_MEDIA */

#define ALL_MEDIA_AUDIO_CONTEXTS                                (AUDIO_CONTEXT_TYPE_MEDIA |              \
                                                                 AUDIO_CONTEXT_TYPE_SOUND_EFFECTS |     \
                                                                 AUDIO_CONTEXT_TYPE_NOTIFICATIONS |     \
                                                                 AUDIO_CONTEXT_TYPE_ALERTS |            \
                                                                 AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM     \
                                                                )

#define AVAILABLE_AUDIO_CONTEXTS                                (SUPPORT_MEDIA_CONTEXTS)

#define PACS_SINK_MAX_CODEC_FRAMES_PER_SDU                      (2)
#define PACS_SOURCE_MAX_CODEC_FRAMES_PER_SDU                    (2)

#ifdef INCLUDE_LE_AUDIO_UNICAST

#ifndef DISABLE_LE_AUDIO_VOICE

/*! PACS parameter for voice sink */
#define PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_VOICE_SINK        (PACS_SAMPLING_FREQUENCY_16KHZ | PACS_SAMPLING_FREQUENCY_32KHZ | \
                                                                 PACS_SAMPLING_FREQUENCY_24KHZ | PACS_SAMPLING_FREQUENCY_48KHZ)
#define PACS_SUPPORTED_FRAME_DURATION_FOR_VOICE_SINK            (SUPPORTED_FRAME_DURATION_7P5MS | PACS_SUPPORTED_FRAME_DURATION_10MS | \
                                                                 PACS_PREFERRED_FRAME_DURATION_10MS)
#define PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_VOICE_SINK         (30)
#define PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_VOICE_SINK         (80)

/*! PACS parameter for voice source */
#define PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_VOICE_SOURCE      (PACS_SAMPLING_FREQUENCY_16KHZ | PACS_SAMPLING_FREQUENCY_24KHZ | \
                                                                 PACS_SAMPLING_FREQUENCY_32KHZ)
#define PACS_SUPPORTED_FRAME_DURATION_FOR_VOICE_SOURCE          (SUPPORTED_FRAME_DURATION_7P5MS | PACS_SUPPORTED_FRAME_DURATION_10MS | \
                                                                 PACS_PREFERRED_FRAME_DURATION_10MS)
#define PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_VOICE_SOURCE       (30)
#define PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_VOICE_SOURCE       (80)

#define SUPPORT_VOICE_CONTEXTS                                  (AUDIO_CONTEXT_TYPE_COVERSATIONAL | AUDIO_CONTEXT_TYPE_RINGTONE)
#define SUPPORT_LIVE_CONTEXTS                                   (AUDIO_CONTEXT_TYPE_LIVE)

#else /* DISABLE_LE_AUDIO_VOICE */

#define SUPPORT_VOICE_CONTEXTS                                  (AUDIO_CONTEXT_TYPE_UNKNOWN)
#define SUPPORT_LIVE_CONTEXTS                                   (AUDIO_CONTEXT_TYPE_UNKNOWN)

#endif /* !DISABLE_LE_AUDIO_VOICE */

#ifdef INCLUDE_LE_GAMING_MODE

/*! PACS parameter for gaming mode sink */
#define PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_GAMING_SINK       (PACS_SAMPLING_FREQUENCY_48KHZ)
#define PACS_SUPPORTED_FRAME_DURATION_FOR_GAMING_SINK           (PACS_SUPPORTED_FRAME_DURATION_7P5MS | PACS_SUPPORTED_FRAME_DURATION_10MS | \
                                                                 PACS_PREFERRED_FRAME_DURATION_7P5MS)
#define PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SINK        (75)
#define PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SINK        (155)

/*! PACS parameter for gaming mode source */
#ifdef ENABLE_LEA_GAMING_MODE_VBC_32KHZ
#define PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_GAMING_SOURCE     (PACS_SAMPLING_FREQUENCY_16KHZ | PACS_SAMPLING_FREQUENCY_32KHZ)
#define PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SOURCE      (30)
#define PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SOURCE      (60)
#else
#define PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_GAMING_SOURCE     (PACS_SAMPLING_FREQUENCY_16KHZ)
#define PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SOURCE      (30)
#define PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_GAMING_SOURCE      (40)
#endif
#define PACS_SUPPORTED_FRAME_DURATION_FOR_GAMING_SOURCE         (PACS_SUPPORTED_FRAME_DURATION_7P5MS | PACS_SUPPORTED_FRAME_DURATION_10MS | \
                                                                 PACS_PREFERRED_FRAME_DURATION_7P5MS)

#define SUPPORT_GAME_CONTEXTS                                   (AUDIO_CONTEXT_TYPE_GAME)

#else /* INCLUDE_LE_GAMING_MODE */

#define SUPPORT_GAME_CONTEXTS                                   (AUDIO_CONTEXT_TYPE_UNKNOWN)

#endif /* INCLUDE_LE_GAMING_MODE */

#ifdef INCLUDE_LE_STEREO_RECORDING

/*! PACS parameter for stereo recording source */
#define PACS_SUPPORTED_SAMPLING_FREQUENCY_FOR_STEREO_RECORDING_SOURCE   (PACS_SAMPLING_FREQUENCY_48KHZ)
#define PACS_SUPPORTED_FRAME_DURATION_FOR_STEREO_RECORDING_SOURCE       (PACS_SUPPORTED_FRAME_DURATION_7P5MS | PACS_SUPPORTED_FRAME_DURATION_10MS | \
                                                                         PACS_PREFERRED_FRAME_DURATION_10MS)
#define PACS_MIN_OCTECTS_PER_CODEC_FRAME_FOR_STEREO_RECORDING_SOURCE    (75)
#define PACS_MAX_OCTECTS_PER_CODEC_FRAME_FOR_STEREO_RECORDING_SOURCE    (155)

#endif /* INCLUDE_LE_STEREO_RECORDING */

/* The from air latency has been optmiized for QCC518x chip series */
#if defined(__QCC518X__) || defined(__QCC308X__)
#define LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT                  (2000)
#define LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT_AND_HDR          (LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT + 2000)
#else
#define LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT                  (0)
#define LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT_AND_HDR          (LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT + 500)
#endif

/*! Presentation Delay for various usecases */
#ifdef INCLUDE_LE_AUDIO_STEREO_CONFIG

#define BAP_PD_MIN_FOR_GAMING_SINK                              (14000 - LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT)
#define BAP_PD_MIN_FOR_GAMING_WITH_VBC_WB_SINK                  (14000 - LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT)
#define BAP_PD_MIN_FOR_GAMING_WITH_VBC_WB_SOURCE                (40000)
#define BAP_PD_MIN_FOR_GAMING_WITH_VBC_SWB_SINK                 (16000 - LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT)
#define BAP_PD_MIN_FOR_GAMING_WITH_VBC_SWB_SOURCE               (40000)

#else /* INCLUDE_LE_AUDIO_STEREO_CONFIG */

#define BAP_PD_MIN_FOR_GAMING_SINK                              (10000 - LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT)
#define BAP_PD_MIN_FOR_GAMING_WITH_VBC_WB_SINK                  (10000 - LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT)
#define BAP_PD_MIN_FOR_GAMING_WITH_VBC_SWB_SINK                 (10000 - LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT)
#define BAP_PD_MIN_FOR_GAMING_WITH_VBC_SWB_SOURCE               (40000)
#define BAP_PD_MIN_FOR_GAMING_WITH_VBC_WB_SOURCE                (40000)

#endif /* INCLUDE_LE_AUDIO_STEREO_CONFIG */

#define BAP_PD_MIN_FOR_GAMING_SOURCE                            (0)
#define BAP_PD_MIN_FOR_VOICE_SINK                               (25000)
#define BAP_PD_MIN_FOR_VOICE_SOURCE                             (40000)
#define BAP_PD_MIN_FOR_MUSIC_SINK                               (25000)
#define BAP_PD_MIN_FOR_STEREO_RECORDING_SOURCE                  (37500)
#define BAP_PD_MIN_FOR_APTX_SINK                                (30000)

#define BAP_PD_MIN_FOR_GAMING_APTX_LITE_SINK                    (9400 - LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT_AND_HDR)
#define BAP_PD_MIN_FOR_GAMING_WITH_VBC_APTX_LITE_SINK           (10400 - LE_AUDIO_REDUCE_PD_DUE_TO_BT_TRANSPORT_AND_HDR)
#define BAP_PD_MIN_FOR_GAMING_WITH_VBC_APTX_LITE_SOURCE         (33000)

/*! For CAP the supported audio contexts fields should be set to all of the
    defined audio contexts.

    This does not apply to the available audio contexts fields - they should
    accurately reflect what is currently available.
*/
#define ALL_AUDIO_CONTEXTS (                                        \
                            AUDIO_CONTEXT_TYPE_UNSPECIFIED |        \
                            AUDIO_CONTEXT_TYPE_COVERSATIONAL |      \
                            AUDIO_CONTEXT_TYPE_MEDIA |              \
                            AUDIO_CONTEXT_TYPE_GAME |               \
                            AUDIO_CONTEXT_TYPE_INSTRUCTIONAL |      \
                            AUDIO_CONTEXT_TYPE_VOICE_ASSISTANT |    \
                            AUDIO_CONTEXT_TYPE_LIVE |               \
                            AUDIO_CONTEXT_TYPE_SOUND_EFFECTS |      \
                            AUDIO_CONTEXT_TYPE_NOTIFICATIONS |      \
                            AUDIO_CONTEXT_TYPE_RINGTONE |           \
                            AUDIO_CONTEXT_TYPE_ALERTS |             \
                            AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM      \
                           )

#define AVAILABLE_VOICE_CONTEXTS                                (SUPPORT_VOICE_CONTEXTS | SUPPORT_LIVE_CONTEXTS)

#define SINK_SUPPORTED_AUDIO_CONTEXTS                           (ALL_AUDIO_CONTEXTS)

#define SINK_AVAILABLE_AUDIO_CONTEXTS                           (                                       \
                                                                 AVAILABLE_AUDIO_CONTEXTS |             \
                                                                 SUPPORT_GAME_CONTEXTS |                \
                                                                 SUPPORT_VOICE_CONTEXTS |               \
                                                                 AUDIO_CONTEXT_TYPE_RINGTONE |          \
                                                                 AUDIO_CONTEXT_TYPE_SOUND_EFFECTS |     \
                                                                 AUDIO_CONTEXT_TYPE_NOTIFICATIONS |     \
                                                                 AUDIO_CONTEXT_TYPE_ALERTS |            \
                                                                 AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM |   \
                                                                 AUDIO_CONTEXT_TYPE_UNSPECIFIED         \
                                                                 )

#define SOURCE_SUPPORTED_AUDIO_CONTEXTS                         (ALL_AUDIO_CONTEXTS)
#define SOURCE_AVAILABLE_AUDIO_CONTEXTS                         (AVAILABLE_VOICE_CONTEXTS | SUPPORT_GAME_CONTEXTS | AUDIO_CONTEXT_TYPE_UNSPECIFIED)

#else /* INCLUDE_LE_AUDIO_UNICAST */

#define SINK_SUPPORTED_AUDIO_CONTEXTS                           (AUDIO_CONTEXT_TYPE_UNSPECIFIED | AUDIO_CONTEXT_TYPE_MEDIA)
#define SINK_AVAILABLE_AUDIO_CONTEXTS                           (AVAILABLE_AUDIO_CONTEXTS)
#define SOURCE_SUPPORTED_AUDIO_CONTEXTS                         (AUDIO_CONTEXT_TYPE_UNKNOWN)

#endif /* INCLUDE_LE_AUDIO_UNICAST */

#ifdef ENABLE_LE_AUDIO_FT_UPDATE
#define LC3_DECODER_VERSION_ID_PAC  LC3_DECODER_VERSION_FT
#else
// don't advertise any extra decoder version in the PACs record
#define LC3_DECODER_VERSION_ID_PAC  LC3_DECODER_VERSION_STANDARD
#endif

#ifdef INCLUDE_LE_APTX_ADAPTIVE
#define APTX_SUPPORTED_SAMPLING_FREQS PACS_SAMPLING_FREQUENCY_48KHZ | PACS_SAMPLING_FREQUENCY_96KHZ
#define APTX_SUPPORTED_OCTETS_MIN     120
#define APTX_SUPPORTED_OCTETS_MAX     2000
#endif

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
#define PACS_APTX_LITE_SINK_SUPPORTED_SAMPLING_FREQS        PACS_SAMPLING_FREQUENCY_48KHZ
#define PACS_APTX_LITE_SRC_SUPPORTED_SAMPLING_FREQS         PACS_SAMPLING_FREQUENCY_16KHZ
#define PACS_APTX_LITE_SUPPORTED_OCTETS_MIN                 20
#define PACS_APTX_LITE_SUPPORTED_OCTETS_MAX                 500
#endif

#endif /* PACS_CONFIG_H_ */
/*! @} */