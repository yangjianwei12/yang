/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       usb_dongle_setup_audio.c
\brief      Module configure audio chains for USB dongle application
*/

#include "kymera.h"
#include "kymera_setup.h"
#include "wired_audio_source.h"

#include "usb_dongle_setup_audio.h"
#include "usb_dongle_cap_ids.h"
#include <chain_input_wired_sbc_encode.h>

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
#ifdef INCLUDE_APTX_ADAPTIVE_22
#include <chain_input_wired_aptx_adaptive_r3_encode.h>
#include <chain_input_usb_aptx_adaptive_r3_encode.h>
#endif /* INCLUDE_APTX_ADAPTIVE_22 */
#include <chain_input_wired_aptx_adaptive_encode.h>
#include <chain_input_usb_aptx_adaptive_encode.h>
#include <chain_input_wired_aptxhd_encode.h>
#include <chain_input_usb_aptxhd_encode.h>
#include <chain_input_wired_aptx_classic_encode.h>
#include <chain_input_usb_aptx_classic_encode.h>
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#include <chain_input_usb_sbc_encode.h>
#include <chain_input_usb_sco_nb.h>
#include <chain_input_usb_sco_nb.h>
#include <chain_input_sco_usb_nb.h>
#include <chain_input_usb_sco_wb.h>
#include <chain_input_sco_usb_wb.h>
#include <chain_input_sco_usb_swb.h>
#include <chain_input_usb_sco_swb.h>
#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
#include "kymera_usb_le_audio.h"
#include <chain_input_usb_iso.h>
#include <chain_input_usb_iso_mono.h>
#include <chain_input_usb_iso_joint_stereo.h>
#include <chain_input_iso_usb.h>
#include <chain_input_iso_usb_mono.h>
#include <chain_input_iso_usb_mono_to_stereo.h>
#ifdef INCLUDE_LE_AUDIO_ANALOG_SOURCE
#include "kymera_analog_le_audio.h"
#include <chain_input_wired_iso.h>
#include <chain_input_wired_iso_mono.h>
#include <chain_input_wired_iso_joint_stereo.h>
#endif
#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
#include <chain_input_usb_iso_aptx_lite.h>
#include <chain_input_usb_iso_joint_stereo_aptx_lite.h>
#include <chain_input_iso_usb_aptx_lite.h>
#include <chain_input_iso_usb_mono_aptx_lite.h>
#include <chain_input_iso_usb_mono_to_stereo_aptx_lite.h>
#ifdef INCLUDE_LE_AUDIO_ANALOG_SOURCE
#include <chain_input_wired_iso_aptx_lite.h>
#include <chain_input_wired_iso_joint_stereo_aptx_lite.h>
#endif
#ifdef INCLUDE_LE_APTX_ADAPTIVE
#include <chain_input_usb_iso_aptx_adaptive.h>
#ifdef INCLUDE_LE_AUDIO_ANALOG_SOURCE
#include <chain_input_wired_iso_aptx_adaptive.h>
#endif
#endif

#endif /* INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE */

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

static const capability_bundle_t capability_bundle[] =
{
#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
#ifdef INCLUDE_APTX_ADAPTIVE
    {
        "download_aptx_adaptive_encode.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#ifdef INCLUDE_APTX_ADAPTIVE_22
    {
        "download_aptx_adaptive_r3_encode.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif /* INCLUDE_APTX_ADAPTIVE_22 */
#endif /* INCLUDE_APTX_ADAPTIVE */
#ifdef INCLUDE_APTX_HD
    {
        "download_aptxhd_encode.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif /* INCLUDE_APTX_HD */
    {
        "download_aptx_encode.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
    {
        "download_swbs.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
#ifdef DOWNLOAD_LC3_ENCODE_SCO_ISO
    {
        "download_lc3_encode_sco_iso.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_LC3_DECODE_SCO_ISO
    {
        "download_lc3_decode_sco_iso.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif

#ifdef DOWNLOAD_APTX_LITE_ENCODE_SCO_ISO
    {
        "download_aptx_lite_encode_sco_iso.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_APTX_LITE_DECODE_SCO_ISO
    {
        "download_aptx_lite_decode_sco_iso.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_APTX_ADAPTIVE_ENCODE_SCO_ISO
    {
        "download_aptx_adaptive_encode_sco_iso.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

    {
        0, 0
    }
};

static const capability_bundle_config_t bundle_config = {capability_bundle, ARRAY_DIM(capability_bundle) - 1};

static const kymera_chain_configs_t chain_configs = {
#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
    .chain_input_wired_sbc_encode_config = &chain_input_wired_sbc_encode_config,
    .chain_input_wired_aptx_adaptive_encode_config = &chain_input_wired_aptx_adaptive_encode_config,
    .chain_input_wired_aptxhd_encode_config = &chain_input_wired_aptxhd_encode_config,
    .chain_input_wired_aptx_classic_encode_config = &chain_input_wired_aptx_classic_encode_config,
#ifdef INCLUDE_APTX_ADAPTIVE_22
    .chain_input_wired_aptx_adaptive_r3_encode_config = &chain_input_wired_aptx_adaptive_r3_encode_config,
    .chain_input_usb_aptx_adaptive_r3_encode_config = &chain_input_usb_aptx_adaptive_r3_encode_config,
#endif /* INCLUDE_APTX_ADAPTIVE_22 */
    .chain_input_usb_sbc_encode_config = &chain_input_usb_sbc_encode_config,
    .chain_input_usb_aptx_adaptive_encode_config = &chain_input_usb_aptx_adaptive_encode_config,
    .chain_input_usb_aptxhd_encode_config = &chain_input_usb_aptxhd_encode_config,
    .chain_input_usb_aptx_classic_encode_config = &chain_input_usb_aptx_classic_encode_config,
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */
    .chain_sco_nb_config = &chain_input_sco_usb_nb_config,
    .chain_usb_voice_nb_config = &chain_input_usb_sco_nb_config,
    .chain_sco_wb_config = &chain_input_sco_usb_wb_config,
    .chain_usb_voice_wb_config = &chain_input_usb_sco_wb_config,
    .chain_sco_swb_config = &chain_input_sco_usb_swb_config,
    .chain_usb_voice_swb_config = &chain_input_usb_sco_swb_config,
};

const appKymeraScoChainInfo kymera_sco_chain_table[] =
{
    /* sco_mode   mic_cfg          chain                        rate */
    { SCO_NB,     0,      &chain_input_usb_sbc_encode_config,  8000 },
    {NO_SCO}
};

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
const appKymeraUsbIsoLeAudioChainInfo kymera_usb_iso_chain_info[] =
{
    /*                              type                                                chain                                             codec                 */
    { KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_SINGLE_MONO_CIS,    &chain_input_usb_iso_mono_config,                       KYMERA_LE_AUDIO_CODEC_LC3 },
    { KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_DUAL_MONO_CIS,      &chain_input_usb_iso_config,                            KYMERA_LE_AUDIO_CODEC_LC3 },
    { KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_SINGLE_STEREO_CIS,  &chain_input_usb_iso_joint_stereo_config,               KYMERA_LE_AUDIO_CODEC_LC3 },

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
    { KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_DUAL_MONO_CIS,      &chain_input_usb_iso_aptx_lite_config,                  KYMERA_LE_AUDIO_CODEC_APTX_LITE },
    { KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_SINGLE_STEREO_CIS,  &chain_input_usb_iso_joint_stereo_aptx_lite_config,     KYMERA_LE_AUDIO_CODEC_APTX_LITE },
#endif /* INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE */

#ifdef INCLUDE_LE_APTX_ADAPTIVE
    { KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_DUAL_MONO_CIS,      &chain_input_usb_iso_aptx_adaptive_config,              KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE },
#endif /* INCLUDE_LE_APTX_ADAPTIVE */

};

static const appKymeraUsbIsoChainTable to_air_iso_chain_table =
{
    .chain_table = kymera_usb_iso_chain_info,
    .table_length = ARRAY_DIM(kymera_usb_iso_chain_info)
};

const appKymeraIsoUsbLeAudioChainInfo kymera_iso_usb_chain_info[] =
{
    /* Stereo                           chain                               codec                       */
    { KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_DUAL_DECODER_TO_STEREO,       &chain_input_iso_usb_config,                            KYMERA_LE_AUDIO_CODEC_LC3 },
    { KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_STEREO,     &chain_input_iso_usb_mono_to_stereo_config,             KYMERA_LE_AUDIO_CODEC_LC3 },
    { KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO,       &chain_input_iso_usb_mono_config,                       KYMERA_LE_AUDIO_CODEC_LC3 },

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
    { KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_DUAL_DECODER_TO_STEREO,       &chain_input_iso_usb_aptx_lite_config,                  KYMERA_LE_AUDIO_CODEC_APTX_LITE },
    { KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_STEREO,     &chain_input_iso_usb_mono_to_stereo_aptx_lite_config,   KYMERA_LE_AUDIO_CODEC_APTX_LITE },
    { KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO,       &chain_input_iso_usb_mono_aptx_lite_config,             KYMERA_LE_AUDIO_CODEC_APTX_LITE },
#endif /* INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE */
};

static const appKymeraIsoUsbChainTable from_air_iso_chain_table =
{
    .chain_table = kymera_iso_usb_chain_info,
    .table_length = ARRAY_DIM(kymera_iso_usb_chain_info)
};

#ifdef INCLUDE_LE_AUDIO_ANALOG_SOURCE
const appKymeraAnalogIsoLeAudioChainInfo kymera_analog_iso_chain_info[] =
{
    /*                              type                                                chain                                             codec                 */
    { KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_SINGLE_MONO_CIS,    &chain_input_wired_iso_mono_config,             KYMERA_LE_AUDIO_CODEC_LC3 },
    { KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_DUAL_MONO_CIS,      &chain_input_wired_iso_config,                  KYMERA_LE_AUDIO_CODEC_LC3 },
    { KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_SINGLE_STEREO_CIS,  &chain_input_wired_iso_joint_stereo_config,     KYMERA_LE_AUDIO_CODEC_LC3 },

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
    { KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_DUAL_MONO_CIS,      &chain_input_wired_iso_aptx_lite_config,                KYMERA_LE_AUDIO_CODEC_APTX_LITE },
    { KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_SINGLE_STEREO_CIS,  &chain_input_wired_iso_joint_stereo_aptx_lite_config,   KYMERA_LE_AUDIO_CODEC_APTX_LITE },
#endif /* INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE */

#ifdef INCLUDE_LE_APTX_ADAPTIVE
    { KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_DUAL_MONO_CIS,      &chain_input_wired_iso_aptx_adaptive_config,            KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE },
#endif /* INCLUDE_LE_APTX_ADAPTIVE */

};

static const appKymeraAnalogIsoChainTable analog_to_air_iso_chain_table =
{
    .chain_table = kymera_analog_iso_chain_info,
    .table_length = ARRAY_DIM(kymera_analog_iso_chain_info)
};
#endif /* INCLUDE_LE_AUDIO_ANALOG_SOURCE */

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

const wired_audio_config_t wired_audio_config = 
{
    .rate = 48000,
    .min_latency = 10,/*! in milli-seconds */
    .max_latency = 40,/*! in milli-seconds */
    .target_latency = 30 /*! in milli-seconds */
};

bool usb_dongleSetupAudio(void)
{
    Kymera_SetChainConfigs(&chain_configs);
    WiredAudioSource_Configure(&wired_audio_config);
    Kymera_SetScoChainTable(kymera_sco_chain_table);
#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
    Kymera_UsbLeAudioSetToAirChainTable(&to_air_iso_chain_table);
    Kymera_UsbLeAudioSetFromAirChainTable(&from_air_iso_chain_table);
#ifdef INCLUDE_LE_AUDIO_ANALOG_SOURCE
    KymeraAnalogLeAudio_SetToAirChainTable(&analog_to_air_iso_chain_table);
#endif
#endif
    return TRUE;
}

void usb_dongleSetBundlesConfig(void)
{
    Kymera_SetBundleConfig(&bundle_config);
}
