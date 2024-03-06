/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       charger_case_setup_audio.c
\brief      Module configure audio chains for charger case application
*/

#include "kymera.h"
#include "kymera_setup.h"
#include "wired_audio_source.h"

#include "charger_case_cap_ids.h"
#include "charger_case_setup_audio.h"
#include <chain_input_wired_sbc_encode.h>
#include <chain_input_wired_aptx_adaptive_encode.h>
#include <chain_input_usb_aptx_adaptive_encode.h>
#include <chain_input_wired_aptx_classic_encode.h>
#include <chain_input_usb_aptx_classic_encode.h>
#include <chain_input_usb_sbc_encode.h>

static const capability_bundle_t capability_bundle[] =
{
    {
        "download_aptx_adaptive_encode.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
    {
        "download_aptx_encode.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
    {
        0, 0
    }
};

static const capability_bundle_config_t bundle_config = {capability_bundle, ARRAY_DIM(capability_bundle) - 1};

static const kymera_chain_configs_t chain_configs = {
    .chain_input_wired_sbc_encode_config = &chain_input_wired_sbc_encode_config,
    .chain_input_wired_aptx_adaptive_encode_config = &chain_input_wired_aptx_adaptive_encode_config,
    .chain_input_wired_aptx_classic_encode_config = &chain_input_wired_aptx_classic_encode_config,
    .chain_input_usb_sbc_encode_config = &chain_input_usb_sbc_encode_config,
    .chain_input_usb_aptx_adaptive_encode_config = &chain_input_usb_aptx_adaptive_encode_config,
    .chain_input_usb_aptx_classic_encode_config = &chain_input_usb_aptx_classic_encode_config,
};

const wired_audio_config_t wired_audio_config = 
{
    .rate = 48000,
    .min_latency = 10,/*! in milli-seconds */
    .max_latency = 40,/*! in milli-seconds */
    .target_latency = 30 /*! in milli-seconds */
};

bool ChargerCase_SetupAudio(void)
{
    Kymera_SetChainConfigs(&chain_configs);
    WiredAudioSource_Configure(&wired_audio_config);
    return TRUE;
}

void ChargerCase_SetBundlesConfig(void)
{
    Kymera_SetBundleConfig(&bundle_config);
}

