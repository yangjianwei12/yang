/*!
\copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       headset_setup_audio.c
\brief      Module Conifgure Audio chains for headset application
*/

#include "kymera.h"
#include "source_prediction.h"
#include "kymera_setup.h"
#include "wired_audio_source.h"
#include "cap_id_prim.h"

#include "headset_cap_ids.h"
#include "headset_setup_audio.h"
#include "headset_product_config.h"

#include "chain_sco_nb.h"
#include "chain_sco_wb.h"
#include "chain_sco_swb.h"
#ifdef INCLUDE_SWB_LC3
#include "chain_sco_swb_lc3.h"
#include "chain_sco_swb_lc3_2mic.h"
#endif
#include "chain_sco_nb_2mic.h"
#include "chain_sco_wb_2mic.h"
#include "chain_sco_swb_2mic.h"
#include "chain_sco_nb_2mic_binaural.h"
#include "chain_sco_wb_2mic_binaural.h"
#include "chain_sco_swb_2mic_binaural.h"
#include "chain_output_volume_stereo.h"
#include "chain_output_volume_mono.h"
#include "chain_output_volume_common.h"
#include "chain_output_volume_stereo_le.h"
#include "chain_output_volume_mono_le.h"
#include "chain_prompt_sbc.h"
#include "chain_prompt_aac.h"
#include "chain_prompt_pcm.h"
#include "chain_tone_gen.h"
#include "chain_aec.h"
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
#include "chain_anc.h"

#include "chain_input_sbc_stereo.h"
#include "chain_input_aptx_stereo.h"
#include "chain_input_aptxhd_stereo.h"
#include "chain_input_aptx_adaptive_stereo.h"
#include "chain_input_aptx_adaptive_stereo_q2q.h"
#ifdef INCLUDE_APTX_ADAPTIVE_22
#include "chain_input_aptx_adaptive_r3_stereo_q2q.h"
#include "chain_input_aptx_adaptive_r3_stereo.h"
#endif
#ifdef ENABLE_TWM_SPEAKER
#include "chain_input_aac_stereo_mix.h"
#include "chain_forwarding_input_aptx_right.h"
#include "chain_forwarding_input_aptx_left.h"
#include "chain_input_aptx_adaptive_r3_mono.h"
#include "chain_input_aptx_adaptive_r3_mono_q2q.h"
#include "chain_input_aptx_adaptive_stereo_mix.h"
#include "chain_input_aptx_adaptive_stereo_mix_q2q.h"
#include "chain_input_aptx_stereo_mix.h"
#include "chain_input_sbc_stereo_mix.h"
#include "chain_input_aptx_split_stereo.h"
#endif /* ENABLE_TWM_SPEAKER */
#include "chain_input_aac_stereo.h"
#include "chain_input_wired_analog_stereo.h"
#include "chain_input_usb_stereo.h"
#include "chain_usb_voice_rx_mono.h"
#include "chain_usb_voice_rx_stereo.h"
#include "chain_usb_voice_wb.h"
#include "chain_usb_voice_wb_2mic.h"
#include "chain_usb_voice_wb_2mic_binaural.h"
#include "chain_usb_voice_nb.h"
#include "chain_usb_voice_nb_2mic.h"
#include "chain_usb_voice_nb_2mic_binaural.h"

#if (defined(INCLUDE_MIRRORING) && defined (ENABLE_TWM_SPEAKER))
#define SPEAKER_MONO_STEREO
#endif

#ifdef ENABLE_LE_AUDIO_CSIP
#define SPEAKER_MONO
#endif

#ifdef INCLUDE_MUSIC_PROCESSING
#include "chain_music_processing_user_eq.h"
#if defined(SPEAKER_MONO_STEREO) || defined (SPEAKER_MONO)
#include "chain_music_processing_user_eq_mono.h"
#endif
#else
#include "chain_music_processing.h"
#if defined(SPEAKER_MONO_STEREO) || defined (SPEAKER_MONO)
#include "chain_music_processing_mono.h"
#endif
#endif /* INCLUDE_MUSIC_PROCESSING */

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
#include "chain_lc3_iso_stereo_decoder.h"
#include "chain_lc3_iso_split_stereo_decoder.h"
#include <chain_lc3_iso_mono_to_stereo_decoder.h>
#ifdef SPEAKER_MONO
#include "chain_lc3_iso_mono_decoder.h"
#endif /* SPEAKER_MONO */
#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
#include "chain_aptx_lite_iso_stereo_decoder.h"
#include "chain_aptx_lite_iso_mono_wb_1mic_cvc.h"
#endif /* INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE */
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST
#include "chain_lc3_iso_voice_wb.h"
#include "chain_lc3_iso_voice_uwb.h"
#include "chain_lc3_iso_voice_swb.h"
#include "chain_lc3_iso_stereo_voice_wb.h"
#include "chain_lc3_iso_stereo_voice_uwb.h"
#include "chain_lc3_iso_stereo_voice_swb.h"
#include "chain_lc3_iso_mono_wb_1mic_cvc.h"
#include "chain_lc3_iso_stereo_voice_speaker_only.h"
#include "chain_lc3_iso_mono_voice_speaker_only.h"
#include "chain_lc3_iso_mono_1mic.h"
#include "chain_lc3_iso_dummy_output.h"
#include "chain_lc3_iso_mono_swb_1mic_cvc.h"

/* From Air Chains for LE Voice */
#include "chain_lc3_iso_to_dac_voice_wb_mono.h"

#endif

#include "chain_mic_resampler.h"

#include "chain_va_graph_manager.h"

#ifdef ENABLE_SIMPLE_SPEAKER
#include "chain_spk_sco_nb.h"
#include "chain_spk_sco_wb.h"
#include "chain_spk_sco_swb.h"
#include "chain_spk_usb_voice_nb.h"
#include "chain_spk_usb_voice_wb.h"
#if defined(KYMERA_SCO_USE_2MIC) || defined(KYMERA_SCO_USE_2MIC_BINAURAL)
      #error KYMERA_SCO_USE_2MIC, KYMERA_SCO_USE_2MIC_BINAURAL is not supported in Simple Speaker build
#endif
#endif  /* ENABLE_SIMPLE_SPEAKER */

#ifdef INCLUDE_GAMING_HEADSET_ADDON
#include "chain_output_mixing_common.h"
#endif

#if defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER)
#ifdef ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT
#include "chain_input_aac_transcode_iso_conc_stereo_p1.h"
#include "chain_input_sbc_transcode_iso_conc_stereo_p1.h"
#include "chain_input_aac_transcode_iso_conc_joint_stereo_p1.h"
#include "chain_input_sbc_transcode_iso_conc_joint_stereo_p1.h"
#include "chain_lc3_stereo_to_iso_encoders_p1.h"
#include "chain_lc3_joint_stereo_to_iso_encoders_p1.h"
#else
#include "chain_input_aac_iso_conc_stereo.h"
#include "chain_input_sbc_iso_conc_stereo.h"
#include "chain_lc3_stereo_to_iso.h"
#include "chain_lc3_joint_stereo_to_iso.h"
#endif /* ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT */
#endif /* (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && (ENABLE_SIMPLE_SPEAKER) */

#ifdef INCLUDE_DECODERS_ON_P1
#define DOWNLOAD_TYPE_DECODER capability_load_to_p0_use_on_both
#else
#define DOWNLOAD_TYPE_DECODER capability_load_to_p0_use_on_p0_only
#endif

static const capability_bundle_t capability_bundle[] =
{
#ifdef INCLUDE_GAMING_HEADSET_ADDON
    {
        "download_convert_from_audio.dkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_AEC_REF
    {
        "download_aec_reference.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_VOLUME_CONTROL
    {
        "download_volume_control.edkcs",
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
#ifdef DOWNLOAD_OPUS_CELT_ENCODE
    {
        "download_opus_celt_encode.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_APTX_ADAPTIVE_DECODE
    {
        "download_aptx_adaptive_decode.edkcs",
        DOWNLOAD_TYPE_DECODER
    },
#endif
#ifdef DOWNLOAD_SWITCHED_PASSTHROUGH
    {
        "download_switched_passthrough_consumer.edkcs",
        capability_bundle_available_p0
    },
#endif
#ifdef DOWNLOAD_SWBS
    {
        "download_swbs.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#if defined(DOWNLOAD_LC3_ENCODE_SCO_ISO) || defined(DOWNLOAD_LC3_ENCODE_SCO_ISO_SWB)
#if defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER) && defined (ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT)
    {
        "download_lc3_encode_sco_iso.edkcs",
        capability_load_to_p0_use_on_both
    },
#else
    {
        "download_lc3_encode_sco_iso.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif /* defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER) && defined (ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT) */
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
#ifdef SPLITTER_DOWNLOAD
    {
        "download_splitter.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#endif
#ifdef INCLUDE_AUDIO_ML_ENGINE
    {
        "download_ml_engine_lib.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef DOWNLOAD_USB_AUDIO
    {
        "download_usb_audio.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#ifdef INCLUDE_APTX_ADAPTIVE_22
    {
        "download_aptx_adaptive_r3_decode.edkcs",
        DOWNLOAD_TYPE_DECODER
    },
#endif
#ifdef DOWNLOAD_RTP_DECODE
    {
        "download_rtp_decode.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif
#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) && (!defined (ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT))
    {
        "download_passthrough.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) && !defined (ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT) */
    {
        0, 0
    }
};

static const capability_bundle_config_t bundle_config = {capability_bundle, ARRAY_DIM(capability_bundle) - 1};

static const kymera_chain_configs_t chain_configs = {
    .chain_output_volume_mono_config = &chain_output_volume_mono_config,
    .chain_output_volume_stereo_config = &chain_output_volume_stereo_config,
#ifdef INCLUDE_GAMING_HEADSET_ADDON
    .chain_output_volume_common_config = &chain_output_mixing_common_config,
#else
    .chain_output_volume_common_config = &chain_output_volume_common_config,
#endif
    .chain_output_volume_mono_le_config = &chain_output_volume_mono_le_config,
    .chain_output_volume_stereo_le_config = &chain_output_volume_stereo_le_config,
    .chain_tone_gen_config = &chain_tone_gen_config,
    .chain_aec_config = &chain_aec_config,
    .chain_prompt_pcm_config = &chain_prompt_pcm_config,
    .chain_anc_config = &chain_anc_config,
#ifdef INCLUDE_DECODERS_ON_P1
#if defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER)
#ifdef ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT
    /* When transcoding is enabled while broadcasting, p1 configuration(SBC/AAC decoder configured to run on p1)
     * is used by default to enable 2nd processor */
    .chain_input_sbc_stereo_config = &chain_input_sbc_transcode_iso_conc_stereo_p1_config_p1,
    .chain_input_aac_stereo_config = &chain_input_aac_transcode_iso_conc_stereo_p1_config_p1,
    .chain_input_sbc_joint_stereo_config = &chain_input_sbc_transcode_iso_conc_joint_stereo_p1_config_p1,
    .chain_input_aac_joint_stereo_config = &chain_input_aac_transcode_iso_conc_joint_stereo_p1_config_p1,
#else
    .chain_input_sbc_stereo_config = &chain_input_sbc_iso_conc_stereo_config_p1,
    .chain_input_aac_stereo_config = &chain_input_aac_iso_conc_stereo_config_p1,
#endif /* ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT */
#else
    .chain_input_sbc_stereo_config = &chain_input_sbc_stereo_config_p1,
    .chain_input_aac_stereo_config = &chain_input_aac_stereo_config_p1,
#endif /* defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER) */
    .chain_input_aptx_stereo_config = &chain_input_aptx_stereo_config_p1,
    .chain_input_aptxhd_stereo_config = &chain_input_aptxhd_stereo_config_p1,

#ifdef ENABLE_TWM_SPEAKER
    .chain_input_aac_stereo_mix_config = &chain_input_aac_stereo_mix_config_p1,
    .chain_forwarding_input_aptx_left_config = &chain_forwarding_input_aptx_left_config_p1,
    .chain_forwarding_input_aptx_right_config = &chain_forwarding_input_aptx_right_config_p1,
    .chain_input_sbc_stereo_mix_config = &chain_input_sbc_stereo_mix_config_p1,
    .chain_input_aptx_stereo_mix_config = &chain_input_aptx_stereo_mix_config_p1,
    .chain_input_aptx_adaptive_stereo_mix_config = &chain_input_aptx_adaptive_stereo_mix_config_p1,
    .chain_input_aptx_adaptive_stereo_mix_q2q_config = &chain_input_aptx_adaptive_stereo_mix_q2q_config_p1,
    .chain_input_aptx_split_stereo_config = &chain_input_aptx_split_stereo_config_p1,
#ifdef INCLUDE_APTX_ADAPTIVE_22
    .chain_input_aptx_adaptive_r3_mono_q2q_config = &chain_input_aptx_adaptive_r3_mono_q2q_config_p1,
    .chain_input_aptx_adaptive_r3_mono_config = &chain_input_aptx_adaptive_r3_mono_config_p1,
#endif /* INCLUDE_APTX_ADAPTIVE_22 */
#endif /* ENABLE_TWM_SPEAKER */

    .chain_input_aptx_adaptive_stereo_config = &chain_input_aptx_adaptive_stereo_config_p1,
    .chain_input_aptx_adaptive_stereo_q2q_config = &chain_input_aptx_adaptive_stereo_q2q_config_p1,
#ifdef INCLUDE_APTX_ADAPTIVE_22
    .chain_input_aptx_adaptive_r3_stereo_q2q_config = &chain_input_aptx_adaptive_r3_stereo_q2q_config_p1,
    .chain_input_aptx_adaptive_r3_stereo_config = &chain_input_aptx_adaptive_r3_stereo_config_p1,
#endif
    .chain_prompt_sbc_config = &chain_prompt_sbc_config_p1,
    .chain_prompt_aac_config = &chain_prompt_aac_config_p1,

#else /* INCLUDE_DECODERS_ON_P1 */
#if defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER)
#ifdef ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT
    /* When transcoding is enabled while broadcasting, p1 configuration(SBC/AAC decoder configured to run on p1)
     * is used by default to enable 2nd processor */
    .chain_input_sbc_stereo_config = &chain_input_sbc_transcode_iso_conc_stereo_p1_config_p1,
    .chain_input_aac_stereo_config = &chain_input_aac_transcode_iso_conc_stereo_p1_config_p1,
    .chain_input_sbc_joint_stereo_config = &chain_input_sbc_transcode_iso_conc_joint_stereo_p1_config_p1,
    .chain_input_aac_joint_stereo_config = &chain_input_aac_transcode_iso_conc_joint_stereo_p1_config_p1,
#else
    .chain_input_sbc_stereo_config = &chain_input_sbc_iso_conc_stereo_config_p0,
    .chain_input_aac_stereo_config = &chain_input_aac_iso_conc_stereo_config_p0,
#endif /* ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT */
#else
    .chain_input_sbc_stereo_config = &chain_input_sbc_stereo_config_p0,
    .chain_input_aac_stereo_config = &chain_input_aac_stereo_config_p0,
#endif /* defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER) */
    .chain_input_aptx_stereo_config = &chain_input_aptx_stereo_config_p0,
    .chain_input_aptxhd_stereo_config = &chain_input_aptxhd_stereo_config_p0,

#ifdef ENABLE_TWM_SPEAKER
    .chain_input_aac_stereo_mix_config = &chain_input_aac_stereo_mix_config_p0,
    .chain_forwarding_input_aptx_left_config = &chain_forwarding_input_aptx_left_config_p0,
    .chain_forwarding_input_aptx_right_config = &chain_forwarding_input_aptx_right_config_p0,
    .chain_input_sbc_stereo_mix_config = &chain_input_sbc_stereo_mix_config_p0,
    .chain_input_aptx_stereo_mix_config = &chain_input_aptx_stereo_mix_config_p0,
    .chain_input_aptx_adaptive_stereo_mix_config = &chain_input_aptx_adaptive_stereo_mix_config_p0,
    .chain_input_aptx_adaptive_stereo_mix_q2q_config = &chain_input_aptx_adaptive_stereo_mix_q2q_config_p0,
    .chain_input_aptx_split_stereo_config = &chain_input_aptx_split_stereo_config_p0,
#ifdef INCLUDE_APTX_ADAPTIVE_22
    .chain_input_aptx_adaptive_r3_mono_q2q_config = &chain_input_aptx_adaptive_r3_mono_q2q_config_p0,
    .chain_input_aptx_adaptive_r3_mono_config = &chain_input_aptx_adaptive_r3_mono_config_p0,
#endif /* INCLUDE_APTX_ADAPTIVE_22 */
#endif /* ENABLE_TWM_SPEAKER */

    .chain_prompt_sbc_config = &chain_prompt_sbc_config_p0,
    .chain_prompt_aac_config = &chain_prompt_aac_config_p0,
    .chain_input_aptx_adaptive_stereo_config = &chain_input_aptx_adaptive_stereo_config_p0,
    .chain_input_aptx_adaptive_stereo_q2q_config = &chain_input_aptx_adaptive_stereo_q2q_config_p0,
#ifdef INCLUDE_APTX_ADAPTIVE_22
    .chain_input_aptx_adaptive_r3_stereo_q2q_config = &chain_input_aptx_adaptive_r3_stereo_q2q_config_p0,
    .chain_input_aptx_adaptive_r3_stereo_config = &chain_input_aptx_adaptive_r3_stereo_config_p0,
#endif

#endif /* INCLUDE_DECODERS_ON_P1 */

#ifdef INCLUDE_SPEAKER_EQ
#ifdef INCLUDE_MUSIC_PROCESSING

#ifdef SPEAKER_MONO_STEREO
    .chain_music_processing_mono_config_p0 = &chain_music_processing_user_eq_mono_config_p0,
    .chain_music_processing_mono_config_p1 = &chain_music_processing_user_eq_mono_config_p1,
#endif /* SPEAKER_MONO_STEREO */

#ifdef SPEAKER_MONO
    .chain_music_processing_config_p0 = &chain_music_processing_user_eq_mono_config_p0,
    .chain_music_processing_config_p1 = &chain_music_processing_user_eq_mono_config_p1,
#else /* SPEAKER_MONO */
    .chain_music_processing_config_p0 = &chain_music_processing_user_eq_config_p0,
    .chain_music_processing_config_p1 = &chain_music_processing_user_eq_config_p1,
#endif/* SPEAKER_MONO */

#else /* INCLUDE_MUSIC_PROCESSING */

#ifdef SPEAKER_MONO_STEREO
    .chain_music_processing_mono_config_p0 = &chain_music_processing_mono_config_p0,
    .chain_music_processing_mono_config_p1 = &chain_music_processing_mono_config_p1,
#endif /* SPEAKER_MONO_STEREO */

#ifdef SPEAKER_MONO
    .chain_music_processing_config_p0 = &chain_music_processing_mono_config_p0,
    .chain_music_processing_config_p1 = &chain_music_processing_mono_config_p1,
#else /* SPEAKER_MONO */
    .chain_music_processing_config_p0 = &chain_music_processing_config_p0,
    .chain_music_processing_config_p1 = &chain_music_processing_config_p1,
#endif/* SPEAKER_MONO */

#endif /* INCLUDE_MUSIC_PROCESSING */
#endif /* INCLUDE_SPEAKER_EQ */

    .chain_input_wired_analog_stereo_config = &chain_input_wired_analog_stereo_config,
    .chain_input_usb_stereo_config = &chain_input_usb_stereo_config,
    .chain_usb_voice_rx_mono_config = &chain_usb_voice_rx_mono_config,
    .chain_usb_voice_rx_stereo_config = &chain_usb_voice_rx_stereo_config,
#ifdef ENABLE_SIMPLE_SPEAKER
    .chain_usb_voice_nb_config = &chain_spk_usb_voice_nb_config,
    .chain_usb_voice_wb_config = &chain_spk_usb_voice_wb_config,
#else
    .chain_usb_voice_nb_config = &chain_usb_voice_nb_config,
    .chain_usb_voice_wb_config = &chain_usb_voice_wb_config,
#endif /* ENABLE_SIMPLE_SPEAKER */
    .chain_usb_voice_wb_2mic_config = &chain_usb_voice_wb_2mic_config,
    .chain_usb_voice_wb_2mic_binaural_config = &chain_usb_voice_wb_2mic_binaural_config,
    .chain_usb_voice_nb_2mic_config = &chain_usb_voice_nb_2mic_config,
    .chain_usb_voice_nb_2mic_binaural_config = &chain_usb_voice_nb_2mic_binaural_config,
    .chain_mic_resampler_config = &chain_mic_resampler_config,
    .chain_va_graph_manager_config = &chain_va_graph_manager_config,
#ifdef INCLUDE_LE_AUDIO_UNICAST
    .chain_lc3_iso_mic_capture_config_cvc = &chain_lc3_iso_mono_swb_1mic_cvc_config,
    .chain_lc3_iso_mic_capture_config = &chain_lc3_iso_mono_1mic_config,
    .chain_lc3_iso_dummy_output_config = &chain_lc3_iso_dummy_output_config,
#endif
#if defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER)
#ifdef ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT
    .chain_lc3_stereo_to_iso_config = &chain_lc3_stereo_to_iso_encoders_p1_config_p1,
    .chain_lc3_joint_stereo_to_iso_config = &chain_lc3_joint_stereo_to_iso_encoders_p1_config_p1,
#else
    .chain_lc3_stereo_to_iso_config = &chain_lc3_stereo_to_iso_config,
    .chain_lc3_joint_stereo_to_iso_config = &chain_lc3_joint_stereo_to_iso_config,
#endif /* ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT */
#endif /* defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER) */

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
#endif

static const kymera_callback_configs_t callback_configs = {
    .GetA2dpParametersPrediction = &SourcePrediction_GetA2dpParametersPrediction,
};

#ifdef INCLUDE_VOICE_UI
static const appKymeraVaMicChainTable va_mic_chain_table =
{
    .chain_table = va_mic_chain_info,
    .table_length = ARRAY_DIM(va_mic_chain_info)
};
#endif

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

const appKymeraScoChainInfo kymera_sco_chain_table[] =
{
#ifdef KYMERA_SCO_USE_2MIC
#ifdef KYMERA_SCO_USE_2MIC_BINAURAL

    /* 2-mic cVc Binaural configurations
     sco_mode   mic_cfg   chain                             mic rate, use_case rate */
    { SCO_NB,   2,    &chain_sco_nb_2mic_binaural_config,  { 8000,     8000}},
    { SCO_WB,   2,    &chain_sco_wb_2mic_binaural_config,  {16000,    16000}},
    { SCO_SWB,  2,    &chain_sco_swb_2mic_binaural_config, {32000,    32000}},
#else  /* KYMERA_SCO_USE_2MIC_BINAURAL */

    /* 2-mic cVc Endfire configurations
     sco_mode  mic_cfg   chain                          mic rate, use_case rate */
    { SCO_NB,  2,       &chain_sco_nb_2mic_config_p0,  { 8000,     8000}},
    { SCO_WB,  2,       &chain_sco_wb_2mic_config_p0,  {16000,    16000}},
    { SCO_SWB, 2,       &chain_sco_swb_2mic_config_p0, {32000,    32000}},
#ifdef INCLUDE_SWB_LC3
    { SCO_SWB_LC3,  2,    &chain_sco_swb_lc3_2mic_config,  {32000,    32000}},
#endif
#endif /* KYMERA_SCO_USE_2MIC_BINAURAL */
#else   /* KYMERA_SCO_USE_2MIC */

    /* 1-mic CVC configurations
    sco_mode   mic_cfg   chain                     mic rate, use_case rate */
#ifdef ENABLE_SIMPLE_SPEAKER
    { SCO_NB,  1,      &chain_spk_sco_nb_config,  { 8000,     8000}},
    { SCO_WB,  1,      &chain_spk_sco_wb_config,  {16000,    16000}},
    { SCO_SWB, 1,      &chain_spk_sco_swb_config, {32000,    32000}},
#else
    { SCO_NB,  1,      &chain_sco_nb_config_p0,   { 8000,     8000}},
    { SCO_WB,  1,      &chain_sco_wb_config_p0,   {16000,    16000}},
    { SCO_SWB, 1,      &chain_sco_swb_config_p0,  {32000,    32000}},
#ifdef INCLUDE_SWB_LC3
    { SCO_SWB_LC3,  1, &chain_sco_swb_lc3_config, {32000,    32000}},
#endif
#endif /* ENABLE_SIMPLE_SPEAKER */
#endif /* KYMERA_SCO_USE_2MIC */

    { NO_SCO }
};

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
const appKymeraLeAudioChainInfo le_audio_chain_info[] =
{
  { &chain_lc3_iso_stereo_decoder_config_p0, KYMERA_LE_AUDIO_CODEC_LC3, KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_STEREO },
  { &chain_lc3_iso_split_stereo_decoder_config_p0, KYMERA_LE_AUDIO_CODEC_LC3, KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_DUAL_DECODER_TO_STEREO },
  { &chain_lc3_iso_mono_to_stereo_decoder_config_p0, KYMERA_LE_AUDIO_CODEC_LC3, KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_DUAL_MONO },
#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
  { &chain_aptx_lite_iso_stereo_decoder_config_p0, KYMERA_LE_AUDIO_CODEC_APTX_LITE, KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_STEREO },
#endif
#ifdef SPEAKER_MONO
  { &chain_lc3_iso_mono_decoder_config_p0, KYMERA_LE_AUDIO_CODEC_LC3, KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO },
#endif
};

static const appKymeraLeAudioChainTable le_audio_chain_table =
{
    .chain_table = le_audio_chain_info,
    .table_length = ARRAY_DIM(le_audio_chain_info)
};
#endif  /* INCLUDE_LE_AUDIO_UNICAST || INCLUDE_LE_AUDIO_BROADCAST */

const wired_audio_config_t wired_audio_config =
{
    .rate = 48000,
    .min_latency = 10,/*! in milli-seconds */
    .max_latency = 40,/*! in milli-seconds */
    .target_latency = 30 /*! in milli-seconds */
};

const audio_output_config_t audio_hw_output_config =
{
#ifdef ENABLE_I2S_OUTPUT
    .mapping = {
        {audio_output_type_i2s, audio_output_hardware_instance_0, audio_output_channel_a },
        {audio_output_type_i2s, audio_output_hardware_instance_0, audio_output_channel_b }
    },
    .gain_type = {audio_output_gain_digital, audio_output_gain_digital},
#ifdef ENABLE_I2S_OUTPUT_24BIT
    .output_resolution_mode = audio_output_24_bit,
#else
    .output_resolution_mode = audio_output_16_bit,
#endif
    .fixed_hw_gain = -1440 /* -24dB */
#else
    .mapping = {
        {audio_output_type_dac, audio_output_hardware_instance_0, audio_output_channel_a },
        {audio_output_type_dac, audio_output_hardware_instance_0, audio_output_channel_b }
    },
    .gain_type = {audio_output_gain_none, audio_output_gain_none},
    .output_resolution_mode = audio_output_24_bit,
    .fixed_hw_gain = 0
#endif
};

static const kymera_mic_config_t kymera_mic_config =
{
#ifdef SPLITTER_DOWNLOAD
    .cap_id_splitter = (capability_id_t)CAP_ID_DOWNLOAD_SPLITTER
#else
    .cap_id_splitter = (capability_id_t)CAP_ID_SPLITTER
#endif
};

#ifdef INCLUDE_LE_AUDIO_UNICAST
const appKymeraLeVoiceChainInfo le_voice_chain_info[] =
{
    /* stereo_config, mic_cfg , rate ,  chain*/
    { FALSE, 1, 16000, &chain_lc3_iso_voice_wb_config},
    { FALSE, 1, 24000, &chain_lc3_iso_voice_uwb_config},
    { FALSE, 1, 32000, &chain_lc3_iso_voice_swb_config},
    { TRUE, 1, 16000, &chain_lc3_iso_stereo_voice_wb_config},
    { TRUE, 1, 24000, &chain_lc3_iso_stereo_voice_uwb_config},
    { TRUE, 1, 32000, &chain_lc3_iso_stereo_voice_swb_config},
    { TRUE, 0, 16000, &chain_lc3_iso_stereo_voice_speaker_only_config},
    { FALSE, 0, 16000, &chain_lc3_iso_mono_voice_speaker_only_config}
};

static const appKymeraLeVoiceChainTable le_voice_chain_table =
{
    .chain_table = le_voice_chain_info,
    .table_length = ARRAY_DIM(le_voice_chain_info)
};

const appKymeraLeMicChainInfo le_mic_chain_info[] =
{
  /* mic_cfg ,  chain  , rate , codec , is_voice_back_channel */
  /* The below chain is a VBC chain in Gaming mode */
  { 1, &chain_lc3_iso_mono_wb_1mic_cvc_config,  16000, KYMERA_LE_AUDIO_CODEC_LC3, TRUE},
#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
  { 1, &chain_aptx_lite_iso_mono_wb_1mic_cvc_config,  16000, KYMERA_LE_AUDIO_CODEC_APTX_LITE, TRUE},
#endif
  { 1, &chain_lc3_iso_mono_swb_1mic_cvc_config, 32000, KYMERA_LE_AUDIO_CODEC_LC3, TRUE}
};

static const appKymeraLeMicChainTable le_mic_chain_table =
{
    .chain_table = le_mic_chain_info,
    .table_length = ARRAY_DIM(le_mic_chain_info)
};

#endif /* INCLUDE_LE_AUDIO_UNICAST */


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
    Kymera_SetLeMicChainTable(&le_mic_chain_table);
#endif /* INCLUDE_LE_AUDIO_UNICAST */
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
        WiredAudioSource_Configure(&wired_audio_config);
        AudioOutputInit(&audio_hw_output_config);
    }
    return status;
}
