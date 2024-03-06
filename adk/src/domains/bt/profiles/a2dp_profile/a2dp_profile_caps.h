/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief
*/

#ifndef A2DP_PROFILE_CAPS_H_
#define A2DP_PROFILE_CAPS_H_

#ifdef USE_SYNERGY
#include "a2dp_profile.h"
#endif /* USE_SYNERGY */

/*
 * Common macro definitions
 */
#define SPLIT_IN_2_OCTETS(x) ((x) >> 8)  & 0xFF, (x) & 0xFF
#define SPLIT_IN_4_OCTETS(x) (x >> 24) & 0xFF, (x >> 16) & 0xFF, (x >> 8) & 0xFF, x & 0xFF

#define DECODE_RESOURCE_ID (1)      /*!< Resource ID for endpoint definition, indicating decoding. That is, incoming audio */
#define ENCODE_RESOURCE_ID (2)      /*!< Resource ID for endpoint definition, indicating encoding. That is, outgoing audio */

/*! The maximum AAC bitrate */
#define AAC_BITRATE 264630



/*@{ \name SBC configuration bit fields*/
/*! [Octet 0] Support for 16kHz Sampling frequency */
#define SBC_SAMPLING_FREQ_16000        128
/*! [Octet 0] Support for 32kHz Sampling frequency */
#define SBC_SAMPLING_FREQ_32000         64
/*! [Octet 0] Support for 44.1kHz Sampling frequency */
#define SBC_SAMPLING_FREQ_44100         32
/*! [Octet 0] Support for 48kHz Sampling frequency */
#define SBC_SAMPLING_FREQ_48000         16
/*! [Octet 0] Support for Mono channel mode */
#define SBC_CHANNEL_MODE_MONO            8
/*! [Octet 0] Support for Dualchannel mode */
#define SBC_CHANNEL_MODE_DUAL_CHAN       4
/*! [Octet 0] Support for Stereo channel mode */
#define SBC_CHANNEL_MODE_STEREO          2
/*! [Octet 0] Support for Joint Stereo channel mode */
#define SBC_CHANNEL_MODE_JOINT_STEREO    1

/*! [Octet 1] Support for a block length of 4 */
#define SBC_BLOCK_LENGTH_4             128
/*! [Octet 1] Support for a block length of 8 */
#define SBC_BLOCK_LENGTH_8              64
/*! [Octet 1] Support for a block length of 12 */
#define SBC_BLOCK_LENGTH_12             32
/*! [Octet 1] Support for a block length of 16 */
#define SBC_BLOCK_LENGTH_16             16
/*! [Octet 1] Support for 4 subbands */
#define SBC_SUBBANDS_4                   8
/*! [Octet 1] Support for 8 subbands */
#define SBC_SUBBANDS_8                   4
/*! [Octet 1] Support for SNR allocation */
#define SBC_ALLOCATION_SNR               2
/*! [Octet 1] Support for Loudness allocation */
#define SBC_ALLOCATION_LOUDNESS          1

/*! [Octet 2] Minimum bitpool supported */
#define SBC_BITPOOL_MIN                  2
/*! [Octet 2] Maximum bitpool supported */
#define SBC_BITPOOL_MAX                250
/*! [Octet 2] Maximum bitpool for Medium quality */
#define SBC_BITPOOL_MEDIUM_QUALITY      35
/*! [Octet 2] Maximum bitpool for High quality */
#define SBC_BITPOOL_HIGH_QUALITY        53


/*@} */


/*@{ \name AAC/AAC+ configuration bit fields*/
#ifndef DISABLE_A2DP_1P4_ERROR_CODE
/*! [Octet 0] Support for MPEG-D DRC */
#define AAC_MPEGD_DRC           (1<<0)
#endif
/*! [Octet 0] Support for MPEG-2 AAC LC */
#define AAC_MPEG2_AAC_LC        (1<<7)
/*! [Octet 0] Support for MPEG-4 AAC LC */
#define AAC_MPEG4_AAC_LC        (1<<6)
/*! [Octet 0] Support for MPEG-4 AAC LTP */
#define AAC_MPEG4_AAC_LTP       (1<<5)
/*! [Octet 0] Support for MPEG-4 AAC Scalable */
#define AAC_MPEG4_AAC_SCALE     (1<<4)

/*! [Octet 1] Support for 8kHz sampling frequency */
#define AAC_SAMPLE_8000         (1<<7)
/*! [Octet 1] Support for 11025Hz sampling frequency */
#define AAC_SAMPLE_11025        (1<<6)
/*! [Octet 1] Support for 12kHz sampling frequency */
#define AAC_SAMPLE_12000        (1<<5)
/*! [Octet 1] Support for 16kHz sampling frequency */
#define AAC_SAMPLE_16000        (1<<4)
/*! [Octet 1] Support for 22050Hz sampling frequency */
#define AAC_SAMPLE_22050        (1<<3)
/*! [Octet 1] Support for 24kHz sampling frequency */
#define AAC_SAMPLE_24000        (1<<2)
/*! [Octet 1] Support for 32kHz sampling frequency */
#define AAC_SAMPLE_32000        (1<<1)
/*! [Octet 1] Support for 44.1kHz sampling frequency */
#define AAC_SAMPLE_44100        (1<<0)
/*! [Octet 2] Support for 48kHz sampling frequency */
#define AAC_SAMPLE_48000        (1<<7)
/*! [Octet 2] Support for 64kHz sampling frequency */
#define AAC_SAMPLE_64000        (1<<6)
/*! [Octet 2] Support for 88.2kHz sampling frequency */
#define AAC_SAMPLE_88200        (1<<5)
/*! [Octet 2] Support for 96kHz sampling frequency */
#define AAC_SAMPLE_96000        (1<<4)
/*! [Octet 2] Support for using 1 channel */
#define AAC_CHANNEL_1           (1<<3)
/*! [Octet 2] Support for using 2 channels */
#define AAC_CHANNEL_2           (1<<2)

/*! [Octet 3] Support for Variable Bit Rate */
#define AAC_VBR                 (1<<7)

/*! Most significant word of the AAC bitrate */
#define AAC_BITRATE_MSW UINT32_MSW(AAC_BITRATE)
/*! Least significant word of the AAC bitrate */
#define AAC_BITRATE_LSW UINT32_LSW(AAC_BITRATE)

/*! AAC bitrate [Octet 3]  */
#define AAC_BITRATE_3       UINT16_LSO(AAC_BITRATE_MSW)

/*! AAC bitrate [Octet 4] */
#define AAC_BITRATE_4       UINT16_MSO(AAC_BITRATE_LSW)

/*! AAC bitrate [Octet 5] */
#define AAC_BITRATE_5       UINT16_LSO(AAC_BITRATE_LSW)

/*@} */

/*@{ \name Common APTX HD configuration bit fields */
#define APTX_HD_RESERVED_BYTE         0x00
/*@} */

/*@{ \name Common APTX configuration bit fields */
#define APTX_SAMPLING_FREQ_44100         32
#define APTX_SAMPLING_FREQ_48000         16
#define APTX_CHANNEL_MODE_STEREO          2
#define APTX_CHANNEL_MODE_MONO            8

/*@} */

#ifdef USE_SYNERGY
/*! The APT Vendor ID.*/
#define A2DP_APT_VENDOR_ID                 (0x4f000000)
/*! The CSR apt-X Codec ID.*/
#define A2DP_CSR_APTX_CODEC_ID             (0x0100)
/*! The QTI aptX LL Codec ID. */
#define A2DP_QTI_APTX_LL_CODEC_ID          (0x0200)
/*! The CSR aptX ACL Sprint Codec ID.*/
#define A2DP_CSR_APTX_ACL_SPRINT_CODEC_ID  (0x0200)
/*! The QTI Vendor ID */
#define A2DP_QTI_VENDOR_ID                 (0xd7000000U)
/*! The QTI aptX-HD Codec ID.*/
#define A2DP_QTI_APTXHD_CODEC_ID           (0x2400)
/*! The QTI aptX Adaptive Codec ID.*/
#define A2DP_QTI_APTX_AD_CODEC_ID          (0xAD00)
/*! brief The QTI aptX TWS+ Codec ID.*/
#define A2DP_QTI_APTX_TWS_PLUS_CODEC_ID    (0x2500)
/*! The QTI SBC TWS+ Codec ID.*/
#define A2DP_QTI_SBC_TWS_PLUS_CODEC_ID     (0x2600)
/*! The CSR Vendor ID.*/
#define A2DP_CSR_VENDOR_ID                 (0x0a000000)
/*! The CSR Faststream Codec ID.*/
#define A2DP_CSR_FASTSTREAM_CODEC_ID            (0x0100)
/*! The CSR True Wireless Stereo v3 Codec ID for AAC.*/
#define A2DP_CSR_TWS_AAC_CODEC_ID          (0x0401)
/*! The CSR True Wireless Stereo v3 Codec ID for SBC.*/
#define A2DP_CSR_TWS_SBC_CODEC_ID               (0x0301)
/*!  The CSR True Wireless Stereo v3 Codec ID for AAC.*/
#define A2DP_CSR_TWS_AAC_CODEC_ID               (0x0401)
/*! The CSR True Wireless Stereo v3 Codec ID for MP3.*/
#define A2DP_CSR_TWS_MP3_CODEC_ID               (0x0501)
/*! The CSR True Wireless Stereo v3 Codec ID for AptX.*/
#define A2DP_CSR_TWS_APTX_CODEC_ID               (0x0601)
/*! The CSR True Wireless Stereo v3 Codec ID for aptX Adaptive.*/
#define A2DP_CSR_TWS_APTX_AD_CODEC_ID            (0x0701)

/*!
    @brief Used to indicate an invalid AV Sync delay
*/
#define A2DP_INVALID_AV_SYNC_DELAY (0xFFFF)
#endif /* USE_SYNERGY */

void appAvUpdateSbcCapabilities(uint8 *caps, uint32 sample_rate);

#endif /* A2DP_PROFILE_CAPS_H_ */
