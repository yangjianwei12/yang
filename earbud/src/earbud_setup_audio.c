/*!
\copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Pre and post init audio setup.

*/

#include "earbud_cap_ids.h"

#include "chain_aptx_ad_tws_plus_decoder.h"

#include "chain_input_aac_stereo_mix.h"
#include "chain_input_sbc_stereo_mix.h"
#include "chain_input_aptx_stereo_mix.h"
#include "chain_forwarding_input_aptx_left.h"
#include "chain_forwarding_input_aptx_right.h"

#include "chain_input_aptx_adaptive_stereo_mix.h"
#include "chain_input_aptx_adaptive_stereo_mix_q2q.h"

#ifdef INCLUDE_APTX_ADAPTIVE_22
#include "chain_input_aptx_adaptive_r3_mono.h"
#include "chain_input_aptx_adaptive_r3_mono_q2q.h"
#include "chain_input_aptx_adaptive_r3_stereo_mix_q2q.h"
#endif

#include "chain_aec.h"
#if defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
#include "chain_output_volume_mono_geq.h"
#ifdef INCLUDE_KYMERA_COMPANDER
#include "chain_output_volume_mono_compander_geq.h"
#endif
#else
#include "chain_output_volume_mono.h"
#ifdef INCLUDE_KYMERA_COMPANDER
#include "chain_output_volume_mono_compander.h"
#endif
#endif

#include "chain_output_volume_common.h"

#include "chain_tone_gen.h"
#include "chain_prompt_sbc.h"
#include "chain_prompt_pcm.h"
#include "chain_prompt_aac.h"

#include "chain_sco_nb.h"
#include "chain_sco_wb.h"
#include "chain_sco_swb.h"
#ifdef INCLUDE_SWB_LC3
#include "chain_sco_swb_lc3.h"
#include "chain_sco_swb_lc3_2mic.h"
#endif
#include "chain_sco_nb_2mic.h"
#include "chain_sco_nb_2mic_downsampled.h"
#include "chain_sco_wb_2mic.h"
#include "chain_sco_swb_2mic.h"
#include "chain_sco_nb_3mic.h"
#include "chain_sco_nb_3mic_downsampled.h"
#include "chain_sco_wb_3mic.h"
#include "chain_sco_swb_3mic.h"
#include "chain_sco_nb_downsampled.h"
#ifdef INCLUDE_LIS25BA_ACCELEROMETER
#include "chain_sco_nb_3mic_pcm.h"
#include "chain_sco_nb_3mic_pcm_downsampled.h"
#include "chain_sco_wb_3mic_pcm.h"
#include "chain_sco_swb_3mic_pcm.h"
#endif

#include "chain_va_encode_msbc.h"
#include "chain_va_encode_opus.h"
#include "chain_va_encode_sbc.h"
#include "chain_va_mic_1mic.h"
#include "chain_va_mic_1mic_cvc.h"
#include "chain_va_mic_1mic_cvc_no_vad_wuw.h"
#include "chain_va_mic_1mic_cvc_wuw.h"
#include "chain_va_mic_2mic_cvc.h"
#include "chain_va_mic_2mic_cvc_wuw.h"
#include "chain_va_wuw_qva.h"
#include "chain_va_wuw_gva.h"
#include "chain_va_wuw_apva.h"

#include "chain_music_processing.h"
#ifdef INCLUDE_MUSIC_PROCESSING
        #include "chain_music_processing_user_eq.h"
#endif
#include "chain_anc.h"
#include "chain_aanc.h"
#include "chain_aancv2.h"
#include "chain_wind_detect.h"
#include "chain_self_speech_detect.h"
#include "chain_noise_id.h"
#include "chain_ahm.h"
#include "chain_hcgr.h"
#include "chain_adaptive_ambient.h"
#include "chain_anc_compander.h"
#include "chain_aanc_fbc.h"
#include "chain_aanc_fbpath_fbc.h"
#include "chain_aanc_splitter_mic_ref_path.h"
#include "chain_aah.h"
#include "chain_anc_client_basic_passthrough.h"

#include "chain_mic_resampler.h"

#include "chain_va_graph_manager.h"
#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
#include "chain_lc3_iso_mono_decoder.h"
#if defined(INCLUDE_CIS_MIRRORING) && defined(INCLUDE_LE_AUDIO_STEREO_CONFIG)
#include "chain_lc3_iso_stereo_decoder.h"
#endif
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST
#include "chain_lc3_iso_mono_voice_wb.h"
#include "chain_lc3_iso_mono_voice_uwb.h"
#include "chain_lc3_iso_mono_voice_swb.h"
#include "chain_lc3_iso_mono_nb_1mic.h"
#include "chain_lc3_iso_mono_wb_1mic_cvc.h"
#include "chain_lc3_iso_mono_swb_1mic_cvc.h"
#include "chain_lc3_iso_mono_fb_1mic_cvc.h"

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
#include "chain_aptx_lite_iso_mono_decoder.h"
#include "chain_aptx_lite_iso_mono_wb_1mic_cvc.h"
#endif /* INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE */


/* From Air Chains for LE Voice */
#include "chain_lc3_iso_to_dac_voice_wb.h"
#include "chain_lc3_iso_to_dac_voice_swb.h"

#include "chain_lc3_iso_mono_voice_wb_2mic.h"
#include "chain_lc3_iso_mono_voice_uwb_2mic.h"
#include "chain_lc3_iso_mono_voice_swb_2mic.h"

#include "chain_lc3_iso_mono_voice_wb_3mic.h"
#include "chain_lc3_iso_mono_voice_uwb_3mic.h"
#include "chain_lc3_iso_mono_voice_swb_3mic.h"
#include "chain_lc3_iso_dummy_output.h"
#include "chain_output_volume_mono_le.h"
#include "chain_lc3_iso_mono_voice_speaker_only.h"

#ifdef INCLUDE_KYMERA_COMPANDER
#include "chain_output_volume_mono_compander_le.h"
#endif
#ifdef INCLUDE_LE_APTX_ADAPTIVE
#include "chain_aptx_adaptive_iso_mono_decoder.h"
#endif
#endif

#include "chain_fit_test_mic_path.h"

#ifdef INCLUDE_AUDIO_ML_ENGINE
#include "chain_ml_engine.h"
#endif

#include "earbud_setup_audio.h"
#include "source_prediction.h"
#include "kymera.h"
#include "cap_id_prim.h"
#include <kymera_setup.h>

#ifdef INCLUDE_DECODERS_ON_P1
    #define DOWNLOAD_TYPE_DECODER capability_load_to_p0_use_on_both
#else
    #define DOWNLOAD_TYPE_DECODER capability_load_to_p0_use_on_p0_only
#endif

static const capability_bundle_t capability_bundle[] =
{
#ifdef DOWNLOAD_SWITCHED_PASSTHROUGH
    {
        "download_switched_passthrough_consumer.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_APTX_CLASSIC_DEMUX
    {
        "download_aptx_demux.edkcs",
        DOWNLOAD_TYPE_DECODER
    },
#endif
#ifdef DOWNLOAD_AEC_REF
    {
#ifdef CORVUS_YD300
        "download_aec_reference.dkcs",
#else
        "download_aec_reference.edkcs",
#endif
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_ADAPTIVE_ANC
    {
        "download_aanc.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_ADAPTIVE_ANCV2
    {
        "download_aanc2.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_WIND_DETECT
    {
        "download_wind_noise_detect.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_ATR_VAD
    {
        "download_atr_vad.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_AHM
    {
        "download_anc_hw_manager.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_ADRC
    {
        "download_anc_compander.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_HCGR
    {
        "download_hcgr.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif

#ifdef DOWNLOAD_AAH
    {
        "download_aah.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif

#ifdef DOWNLOAD_PASSTHROUGH
    {
        "download_passthrough.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif

#ifdef DOWNLOAD_NOISE_ID
    {
        "download_noise_id.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif

#ifdef DOWNLOAD_APTX_ADAPTIVE_DECODE
    {
        "download_aptx_adaptive_decode.edkcs",
        DOWNLOAD_TYPE_DECODER
    },
#endif
#ifdef DOWNLOAD_VOLUME_CONTROL
    {
        "download_volume_control.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_OPUS_CELT_ENCODE
    {
        "download_opus_celt_encode.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_VA_GRAPH_MANAGER
    {
        "download_va_graph_manager.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_CVC_FBC
    {
        "download_cvc_fbc.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_GVA
    {
#if defined(__QCC307X__)
        "download_gva.edkcs",
#else
        "download_gva.dkcs",
#endif
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_APVA
    {
#if defined(__QCC307X__)
        "download_apva.edkcs",
#else
        "download_apva.dkcs",
#endif
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef INCLUDE_AUDIO_ML_ENGINE
    {
        "download_ml_engine_lib.edkcs",
        capability_load_to_p0_use_on_both
    },
#endif
#if defined(DOWNLOAD_CVC_3MIC) || defined(INCLUDE_HYBRID_CVC)
    {
        "download_cvc_send.edkcs",
#if defined(INCLUDE_HYBRID_CVC) || defined(INCLUDE_3MIC_CVC_ON_P1)
        capability_load_to_p0_use_on_both
#else
        capability_load_to_p0_use_on_p0_only
#endif /* defined(INCLUDE_HYBRID_CVC) || defined(INCLUDE_3MIC_CVC_ON_P1) */
    },
#endif /* defined(DOWNLOAD_CVC_3MIC) || defined(INCLUDE_HYBRID_CVC) */
#ifdef DOWNLOAD_OPUS_CELT_ENCODE
    {
        "download_opus_celt_encode.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#if defined(DOWNLOAD_LC3_ENCODE_SCO_ISO) || defined(DOWNLOAD_LC3_ENCODE_SCO_ISO_SWB)
    {
        "download_lc3_encode_sco_iso.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#if defined(DOWNLOAD_LC3_DECODE_SCO_ISO) || defined(DOWNLOAD_LC3_DECODE_SCO_ISO_SWB)
    {
        "download_lc3_decode_sco_iso.edkcs",
#ifdef INCLUDE_RESTRICT_LC3_TO_P0
        capability_load_to_p0_use_on_p0_only
#else
        DOWNLOAD_TYPE_DECODER
#endif
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
#if defined(DOWNLOAD_SWBS_ENC_DEC) || defined(DOWNLOAD_SWBS_DEC) || defined(DOWNLOAD_SWBS_ENC)
    {
        "download_swbs.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_EARBUD_FIT_TEST
    {
        "download_earbud_fit_test.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef INCLUDE_APTX_ADAPTIVE_22
    {
        "download_aptx_adaptive_r3_decode.edkcs",
        DOWNLOAD_TYPE_DECODER
    },
#endif
#ifdef DOWNLOAD_APTX_ADAPTIVE_DECODE_SCO_ISO
    {
        "download_aptx_adaptive_decode_sco_iso.edkcs",
#ifdef INCLUDE_RESTRICT_LC3_TO_P0
        capability_load_to_p0_use_on_p0_only
#else
        DOWNLOAD_TYPE_DECODER
#endif
    },
#endif
#ifdef SPLITTER_DOWNLOAD
    {
        "download_splitter.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_GEQ
    {
        "download_geq.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_RTP_DECODE
    {
        "download_rtp_decode.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
    {
        0, 0
    }
};

static const capability_bundle_config_t bundle_config = {capability_bundle, ARRAY_DIM(capability_bundle) - 1};

static const kymera_chain_configs_t chain_configs = {
        .chain_aptx_ad_tws_plus_decoder_config = &chain_aptx_ad_tws_plus_decoder_config,
#if defined (INCLUDE_DECODERS_ON_P1)
    .chain_input_aac_stereo_mix_config = &chain_input_aac_stereo_mix_config_p1,
    .chain_input_sbc_stereo_mix_config = &chain_input_sbc_stereo_mix_config_p1,
    .chain_input_aptx_stereo_mix_config = &chain_input_aptx_stereo_mix_config_p1,
    .chain_input_aptx_adaptive_stereo_mix_config = &chain_input_aptx_adaptive_stereo_mix_config_p1,
    .chain_input_aptx_adaptive_stereo_mix_q2q_config = &chain_input_aptx_adaptive_stereo_mix_q2q_config_p1,
    .chain_forwarding_input_aptx_left_config = &chain_forwarding_input_aptx_left_config_p1,
    .chain_forwarding_input_aptx_right_config = &chain_forwarding_input_aptx_right_config_p1,
    .chain_prompt_sbc_config = &chain_prompt_sbc_config_p1,
    .chain_prompt_aac_config = &chain_prompt_aac_config_p1,
#ifdef INCLUDE_APTX_ADAPTIVE_22
    .chain_input_aptx_adaptive_r3_mono_q2q_config = &chain_input_aptx_adaptive_r3_mono_q2q_config_p1,
    .chain_input_aptx_adaptive_r3_mono_config = &chain_input_aptx_adaptive_r3_mono_config_p1,
    .chain_input_aptx_adaptive_r3_stereo_mix_q2q_config = &chain_input_aptx_adaptive_r3_stereo_mix_q2q_config_p1,
#endif
#else
    .chain_input_aac_stereo_mix_config = &chain_input_aac_stereo_mix_config_p0,
    .chain_input_sbc_stereo_mix_config = &chain_input_sbc_stereo_mix_config_p0,
    .chain_input_aptx_stereo_mix_config = &chain_input_aptx_stereo_mix_config_p0,
    .chain_forwarding_input_aptx_left_config = &chain_forwarding_input_aptx_left_config_p0,
    .chain_forwarding_input_aptx_right_config = &chain_forwarding_input_aptx_right_config_p0,
    .chain_prompt_sbc_config = &chain_prompt_sbc_config_p0,
    .chain_prompt_aac_config = &chain_prompt_aac_config_p0,
    .chain_input_aptx_adaptive_stereo_mix_config = &chain_input_aptx_adaptive_stereo_mix_config_p0,
    .chain_input_aptx_adaptive_stereo_mix_q2q_config = &chain_input_aptx_adaptive_stereo_mix_q2q_config_p0,
#ifdef INCLUDE_APTX_ADAPTIVE_22
    .chain_input_aptx_adaptive_r3_mono_q2q_config = &chain_input_aptx_adaptive_r3_mono_q2q_config_p0,
    .chain_input_aptx_adaptive_r3_mono_config = &chain_input_aptx_adaptive_r3_mono_config_p0,
    .chain_input_aptx_adaptive_r3_stereo_mix_q2q_config = &chain_input_aptx_adaptive_r3_stereo_mix_q2q_config_p0,
#endif
#endif
    .chain_aec_config = &chain_aec_config,

#if defined (INCLUDE_LE_AUDIO_UNICAST)
    /* LE Audio chains do not use the source sync */
#ifdef INCLUDE_KYMERA_COMPANDER
 #if defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
    .chain_output_volume_mono_le_config = &chain_output_volume_mono_compander_geq_le_config,
 #else
    .chain_output_volume_mono_le_config = &chain_output_volume_mono_compander_le_config,
 #endif
#else
 #if defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
    .chain_output_volume_mono_le_config = &chain_output_volume_mono_geq_le_config,
 #else
    .chain_output_volume_mono_le_config = &chain_output_volume_mono_le_config,
 #endif
#endif
#endif

#if defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
#ifdef INCLUDE_KYMERA_COMPANDER
    .chain_output_volume_mono_config = &chain_output_volume_mono_compander_geq_config,
#else
    .chain_output_volume_mono_config = &chain_output_volume_mono_geq_config,
#endif
#else
#ifdef INCLUDE_KYMERA_COMPANDER
    .chain_output_volume_mono_config = &chain_output_volume_mono_compander_config,
#else
    .chain_output_volume_mono_config = &chain_output_volume_mono_config,
#endif
#endif
    .chain_output_volume_common_config = &chain_output_volume_common_config,
    .chain_tone_gen_config = &chain_tone_gen_config,
    .chain_prompt_pcm_config = &chain_prompt_pcm_config,
    .chain_aanc_config = &chain_aanc_config,
    .chain_anc_config = &chain_anc_config,
    .chain_hcgr_config = &chain_hcgr_config,
    .chain_aancv2_config = &chain_aancv2_config,
    .chain_aanc_fbc_config = &chain_aanc_fbc_config,
    .chain_aanc_fbpath_fbc_config = &chain_aanc_fbpath_fbc_config,
    .chain_aanc_splitter_mic_ref_path_config = &chain_aanc_splitter_mic_ref_path_config,
    .chain_ahm_config = &chain_ahm_config,
    .chain_adaptive_ambient_config = &chain_anc_compander_config,
    .chain_wind_detect_config = &chain_wind_detect_config,
    .chain_self_speech_detect_config = &chain_self_speech_detect_config,
    .chain_noise_id_config = &chain_noise_id_config,
    .chain_aah_config = &chain_aah_config,
    .chain_anc_client_basic_passthrough_config = &chain_anc_client_basic_passthrough_config,
#ifdef INCLUDE_SPEAKER_EQ
#ifdef INCLUDE_MUSIC_PROCESSING
    .chain_music_processing_config_p0 = &chain_music_processing_user_eq_config_p0,
    .chain_music_processing_config_p1 = &chain_music_processing_user_eq_config_p1,
#else
    .chain_music_processing_config_p0 = &chain_music_processing_config_p0,
    .chain_music_processing_config_p1 = &chain_music_processing_config_p1,
#endif
#endif

    .chain_mic_resampler_config = &chain_mic_resampler_config,
    .chain_va_graph_manager_config = &chain_va_graph_manager_config,
    .chain_fit_test_mic_path_config = &chain_fit_test_mic_path_config,
#ifdef INCLUDE_LE_AUDIO_UNICAST
    .chain_lc3_iso_mic_capture_config_cvc = &chain_lc3_iso_mono_swb_1mic_cvc_config,
#endif
#ifdef INCLUDE_LE_STEREO_RECORDING
    .chain_lc3_iso_mic_capture_config = &chain_lc3_iso_mono_nb_1mic_config,
    .chain_lc3_iso_dummy_output_config = &chain_lc3_iso_dummy_output_config,
#endif
#ifdef INCLUDE_AUDIO_ML_ENGINE
    .chain_ml_engine_config = &chain_ml_engine_config_p1,
#endif
};

static const kymera_callback_configs_t callback_configs = {
    .GetA2dpParametersPrediction = &SourcePrediction_GetA2dpParametersPrediction,
};

#ifdef INCLUDE_VOICE_UI
static const appKymeraVaEncodeChainInfo va_encode_chain_info[] =
{
    {{va_audio_codec_sbc}, &chain_va_encode_sbc_config},
    {{va_audio_codec_msbc}, &chain_va_encode_msbc_config},
    {{va_audio_codec_opus}, &chain_va_encode_opus_config}
};

static const appKymeraVaEncodeChainTable va_encode_chain_table =
{
    .chain_table = va_encode_chain_info,
    .table_length = ARRAY_DIM(va_encode_chain_info)
};

static const appKymeraVaMicChainInfo va_mic_chain_info[] =
{
  /*{{  WuW,   CVC, mics}, chain_to_use}*/
#ifdef KYMERA_VA_USE_CHAIN_WITHOUT_CVC
    {{FALSE,  FALSE,   1}, &chain_va_mic_1mic_config},
#else
#ifdef INCLUDE_WUW
#ifdef KYMERA_VA_USE_CHAIN_WITHOUT_VAD
    {{ TRUE,  TRUE,    1}, &chain_va_mic_1mic_cvc_no_vad_wuw_config},
#else
    {{ TRUE,  TRUE,    1}, &chain_va_mic_1mic_cvc_wuw_config},
    {{ TRUE,  TRUE,    2}, &chain_va_mic_2mic_cvc_wuw_config},
#endif /* KYMERA_VA_USE_CHAIN_WITHOUT_VAD */
#endif /* INCLUDE_WUW */
    {{FALSE,  TRUE,    1}, &chain_va_mic_1mic_cvc_config},
    {{FALSE,  TRUE,    2}, &chain_va_mic_2mic_cvc_config}
#endif /* KYMERA_VA_USE_CHAIN_WITHOUT_CVC */
};

static const appKymeraVaMicChainTable va_mic_chain_table =
{
    .chain_table = va_mic_chain_info,
    .table_length = ARRAY_DIM(va_mic_chain_info)
};

#ifdef INCLUDE_WUW
static const appKymeraVaWuwChainInfo va_wuw_chain_info[] =
{
    {{va_wuw_engine_qva}, &chain_va_wuw_qva_config},
#ifdef INCLUDE_GAA_WUW
    {{va_wuw_engine_gva}, &chain_va_wuw_gva_config},
#endif /* INCLUDE_GAA_WUW */
#ifdef INCLUDE_AMA_WUW
    {{va_wuw_engine_apva}, &chain_va_wuw_apva_config}
#endif /* INCLUDE_AMA_WUW */
};

static const appKymeraVaWuwChainTable va_wuw_chain_table =
{
    .chain_table = va_wuw_chain_info,
    .table_length = ARRAY_DIM(va_wuw_chain_info)
};
#endif /* INCLUDE_WUW */
#endif /* INCLUDE_VOICE_UI */

#ifdef INCLUDE_HYBRID_CVC
const appKymeraScoChainInfo kymera_sco_chain_table[] =
{
  /* sco_mode mic_cfg   chain                                          mic rate, use_case rate */
  { SCO_NB,   1,        &chain_sco_nb_downsampled_config_p1,          {16000,     8000}},
  { SCO_WB,   1,        &chain_sco_wb_config_p1,                      {16000,    16000}},
  { SCO_SWB,  1,        &chain_sco_swb_config_p1,                     {32000,    32000}},

  { SCO_NB,   2,        &chain_sco_nb_2mic_downsampled_config_p1,     {16000,     8000}},
  { SCO_WB,   2,        &chain_sco_wb_2mic_config_p1,                 {16000,    16000}},
  { SCO_SWB,  2,        &chain_sco_swb_2mic_config_p1,                {32000,    32000}},

#ifdef INCLUDE_LIS25BA_ACCELEROMETER
  { SCO_NB,   3,        &chain_sco_nb_3mic_pcm_downsampled_config_p1, {16000,     8000}},
  { SCO_WB,   3,        &chain_sco_wb_3mic_pcm_config_p1,             {16000,    16000}},
  { SCO_SWB,  3,        &chain_sco_swb_3mic_pcm_config_p1,            {32000,    32000}},
#else /* INCLUDE_LIS25BA_ACCELEROMETER */
  { SCO_NB,   3,        &chain_sco_nb_3mic_downsampled_config_p1,     {16000,     8000}},
  { SCO_WB,   3,        &chain_sco_wb_3mic_config_p1,                 {16000,    16000}},
  { SCO_SWB,  3,        &chain_sco_swb_3mic_config_p1,                {32000,    32000}},
#endif /* INCLUDE_LIS25BA_ACCELEROMETER */
  { NO_SCO }
};
#else /* INCLUDE_HYBRID_CVC */
const appKymeraScoChainInfo kymera_sco_chain_table[] =
{
  /* sco_mode mic_cfg   chain                               mic rate, use_case rate */
  { SCO_NB,   1,        &chain_sco_nb_config_p0,           { 8000,     8000}},
  { SCO_WB,   1,        &chain_sco_wb_config_p0,           {16000,    16000}},
  { SCO_SWB,  1,        &chain_sco_swb_config_p0,          {32000,    32000}},
#ifdef INCLUDE_SWB_LC3
  { SCO_SWB_LC3,  1,    &chain_sco_swb_lc3_config,         {32000,    32000}},
#endif
  { SCO_NB,   2,        &chain_sco_nb_2mic_config_p0,      { 8000,     8000}},
  { SCO_WB,   2,        &chain_sco_wb_2mic_config_p0,      {16000,    16000}},
  { SCO_SWB,  2,        &chain_sco_swb_2mic_config_p0,     {32000,    32000}},
#ifdef INCLUDE_SWB_LC3
  { SCO_SWB_LC3,  2,    &chain_sco_swb_lc3_2mic_config,    {32000,    32000}},
#endif

#ifdef INCLUDE_LIS25BA_ACCELEROMETER
#ifdef INCLUDE_3MIC_CVC_ON_P1
  { SCO_NB,   3,        &chain_sco_nb_3mic_pcm_config_p1,  { 8000,     8000}},
  { SCO_WB,   3,        &chain_sco_wb_3mic_pcm_config_p1,  {16000,    16000}},
  { SCO_SWB,  3,        &chain_sco_swb_3mic_pcm_config_p1, {32000,    32000}},
#else /* INCLUDE_3MIC_CVC_ON_P1 */
  { SCO_NB,   3,        &chain_sco_nb_3mic_pcm_config_p0,  { 8000,     8000}},
  { SCO_WB,   3,        &chain_sco_wb_3mic_pcm_config_p0,  {16000,    16000}},
  { SCO_SWB,  3,        &chain_sco_swb_3mic_pcm_config_p0, {32000,    32000}},
#endif /* INCLUDE_3MIC_CVC_ON_P1 */
#else /* INCLUDE_LIS25BA_ACCELEROMETER */
#ifdef INCLUDE_3MIC_CVC_ON_P1
  { SCO_NB,   3,        &chain_sco_nb_3mic_config_p1,      { 8000,     8000}},
  { SCO_WB,   3,        &chain_sco_wb_3mic_config_p1,      {16000,    16000}},
  { SCO_SWB,  3,        &chain_sco_swb_3mic_config_p1,     {32000,    32000}},
#else /* INCLUDE_3MIC_CVC_ON_P1 */
  { SCO_NB,   3,        &chain_sco_nb_3mic_config_p0,      { 8000,     8000}},
  { SCO_WB,   3,        &chain_sco_wb_3mic_config_p0,      {16000,    16000}},
  { SCO_SWB,  3,        &chain_sco_swb_3mic_config_p0,     {32000,    32000}},
#endif /* INCLUDE_3MIC_CVC_ON_P1 */
#endif /* INCLUDE_LIS25BA_ACCELEROMETER */
  { NO_SCO }
};
#endif /* INCLUDE_HYBRID_CVC */

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
const appKymeraLeAudioChainInfo le_audio_chain_info[] =
{
#if defined (INCLUDE_DECODERS_ON_P1) && !defined(INCLUDE_RESTRICT_LC3_TO_P0)
    { &chain_lc3_iso_mono_decoder_config_p1 , KYMERA_LE_AUDIO_CODEC_LC3, KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO },
#ifdef INCLUDE_LE_APTX_ADAPTIVE
    { &chain_aptx_adaptive_iso_mono_decoder_config_p1, KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE, KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO },
#endif
#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
  { &chain_aptx_lite_iso_mono_decoder_config_p1, KYMERA_LE_AUDIO_CODEC_APTX_LITE, KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO },
#endif
#else
    { &chain_lc3_iso_mono_decoder_config_p0, KYMERA_LE_AUDIO_CODEC_LC3, KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO },
#ifdef INCLUDE_LE_APTX_ADAPTIVE
    { &chain_aptx_adaptive_iso_mono_decoder_config_p0, KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE, KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO },
#endif
#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
  { &chain_aptx_lite_iso_mono_decoder_config_p0, KYMERA_LE_AUDIO_CODEC_APTX_LITE, KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO },
#endif
#if defined(INCLUDE_LE_AUDIO_STEREO_CONFIG) && defined(INCLUDE_CIS_MIRRORING)
    { &chain_lc3_iso_stereo_decoder_config_p0, KYMERA_LE_AUDIO_CODEC_LC3, KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_STEREO },
#endif
#endif
};

static const appKymeraLeAudioChainTable le_audio_chain_table =
{
    .chain_table = le_audio_chain_info,
    .table_length = ARRAY_DIM(le_audio_chain_info)
};
#endif /* INCLUDE_LE_AUDIO_UNICAST || INCLUDE_LE_AUDIO_BROADCAST */

#if defined(INCLUDE_LE_AUDIO_UNICAST)
const appKymeraLeVoiceChainInfo le_voice_chain_info[] =
{
  /* stereo_config, mic_cfg , rate ,  chain*/
  { FALSE, 1, 16000, &chain_lc3_iso_mono_voice_wb_config },
  { FALSE, 1, 24000, &chain_lc3_iso_mono_voice_uwb_config},
  { FALSE, 1, 32000, &chain_lc3_iso_mono_voice_swb_config},

  { FALSE, 2, 16000, &chain_lc3_iso_mono_voice_wb_2mic_config},
  { FALSE, 2, 24000, &chain_lc3_iso_mono_voice_uwb_2mic_config},
  { FALSE, 2, 32000, &chain_lc3_iso_mono_voice_swb_2mic_config},

#ifdef INCLUDE_3MIC_CVC_ON_P1
  { FALSE, 3, 16000, &chain_lc3_iso_mono_voice_wb_3mic_config_p1  },
  { FALSE, 3, 24000, &chain_lc3_iso_mono_voice_uwb_3mic_config_p1 },
  { FALSE, 3, 32000, &chain_lc3_iso_mono_voice_swb_3mic_config_p1 },
#else /* INCLUDE_3MIC_CVC_ON_P1 */
  { FALSE, 3, 16000, &chain_lc3_iso_mono_voice_wb_3mic_config_p0  },
  { FALSE, 3, 24000, &chain_lc3_iso_mono_voice_uwb_3mic_config_p0 },
  { FALSE, 3, 32000, &chain_lc3_iso_mono_voice_swb_3mic_config_p0 },
#endif /* INCLUDE_3MIC_CVC_ON_P1 */

  { FALSE, 0, 16000, &chain_lc3_iso_mono_voice_speaker_only_config},

};

static const appKymeraLeVoiceChainTable le_voice_chain_table =
{
    .chain_table = le_voice_chain_info,
    .table_length = ARRAY_DIM(le_voice_chain_info)
};

const appKymeraLeAudioFromAirVoiceChainInfo le_from_air_voice_chain_info[] =
{
    /* stereo_config, rate ,  chain*/
    {FALSE, 16000, &chain_lc3_iso_to_dac_voice_wb_config },
/*    {FALSE, 24000, &chain_lc3_iso_to_dac_voice_uwb_config }*/
    {FALSE, 32000, &chain_lc3_iso_to_dac_voice_swb_config }

};

static const appKymeraLeAudioFromAirVoiceChainTable le_from_air_voice_chain_table =
{
    .chain_table = le_from_air_voice_chain_info,
    .table_length = ARRAY_DIM(le_from_air_voice_chain_info)
};

const appKymeraLeMicChainInfo le_mic_chain_info[] =
{
    /* mic_cfg ,  chain  , rate , codec , is_voice_back_channel */
    /* The below chain is a VBC chain in Gaming mode */
    { 1, &chain_lc3_iso_mono_wb_1mic_cvc_config,  16000, KYMERA_LE_AUDIO_CODEC_LC3, TRUE},

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
  { 1, &chain_aptx_lite_iso_mono_wb_1mic_cvc_config,  16000, KYMERA_LE_AUDIO_CODEC_APTX_LITE, TRUE},
#endif

  { 1, &chain_lc3_iso_mono_swb_1mic_cvc_config, 32000, KYMERA_LE_AUDIO_CODEC_LC3, TRUE},
  { 1, &chain_lc3_iso_mono_fb_1mic_cvc_config, 48000, KYMERA_LE_AUDIO_CODEC_LC3, TRUE}

};

static const appKymeraLeMicChainTable le_mic_chain_table =
{
    .chain_table = le_mic_chain_info,
    .table_length = ARRAY_DIM(le_mic_chain_info)
};

#endif /* INCLUDE_LE_AUDIO_UNICAST */

const audio_output_config_t audio_hw_output_config =
{

#ifdef ENHANCED_ANC_USE_2ND_DAC_ENDPOINT
    .mapping = {
        {audio_output_type_dac, audio_output_hardware_instance_0, audio_output_channel_a },
        {audio_output_type_dac, audio_output_hardware_instance_0, audio_output_channel_b }
    },
#else
    .mapping = {
        {audio_output_type_dac, audio_output_hardware_instance_0, audio_output_channel_a }
    },
#endif
    .gain_type = {audio_output_gain_none, audio_output_gain_none},
    .output_resolution_mode = audio_output_24_bit,
    .fixed_hw_gain = 0
};

static const kymera_mic_config_t kymera_mic_config =
{
#ifdef SPLITTER_DOWNLOAD
    .cap_id_splitter = (capability_id_t)CAP_ID_DOWNLOAD_SPLITTER
#else
    .cap_id_splitter = (capability_id_t)CAP_ID_SPLITTER
#endif
};

static void appSetupAudio_SetKymeraChains(void)
{
    Kymera_SetChainConfigs(&chain_configs);
    Kymera_SetScoChainTable(kymera_sco_chain_table);
#ifdef INCLUDE_VOICE_UI
    Kymera_SetVaMicChainTable(&va_mic_chain_table);
    Kymera_SetVaEncodeChainTable(&va_encode_chain_table);
#endif
#ifdef INCLUDE_WUW
    Kymera_SetVaWuwChainTable(&va_wuw_chain_table);
#endif /* INCLUDE_WUW */
#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
    Kymera_SetLeAudioChainTable(&le_audio_chain_table);
#endif
#if defined(INCLUDE_LE_AUDIO_UNICAST)
    Kymera_SetLeVoiceChainTable(&le_voice_chain_table);
    Kymera_SetLeFromAirVoiceChainTable(&le_from_air_voice_chain_table);
    Kymera_SetLeMicChainTable(&le_mic_chain_table);
#endif
}

bool AppSetupAudio_InitAudio(Task init_task)
{
    bool status;
    Kymera_SetBundleConfig(&bundle_config);
    status = appKymeraInit(init_task);
    if (status)
    {
        appSetupAudio_SetKymeraChains();
        Kymera_MicInit(&kymera_mic_config);
        Kymera_SetCallbackConfigs(&callback_configs);
#ifdef INCLUDE_WUW
        Kymera_StoreLargestWuwEngine();
#endif
        AudioOutputInit(&audio_hw_output_config);
    }
    return status;
}
