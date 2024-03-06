/****************************************************************************
 * Copyright (c) 2011 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file stream_type_alias.h
 * \ingroup stream
 *
 * Public header file for stream types.
 * Currently just aliases ACCMD types.
 */

#ifndef STREAM_TYPE_ALIAS_H
#define STREAM_TYPE_ALIAS_H

/****************************************************************************
Include Files
*/

#if defined(CHIP_BASE_HYDRA)
#include "accmd_prim.h"
typedef ACCMD_CONFIG_KEY STREAM_CONFIG_KEY;
#endif

/****************************************************************************
Public Type Declarations
*/

/**
 * ACCMD types are hidden from stream by using typedef aliases
 */

#if defined(CHIP_BASE_HYDRA)
#define STREAM_CONFIG_KEY_STREAM_AUDIO_DISABLE_ENDPOINT_PROCESSING \
         ACCMD_CONFIG_KEY_STREAM_AUDIO_DISABLE_ENDPOINT_PROCESSING
#define STREAM_CONFIG_KEY_STREAM_AUDIO_SAMPLE_PERIOD_DEVIATION \
         ACCMD_CONFIG_KEY_STREAM_AUDIO_SAMPLE_PERIOD_DEVIATION
#define STREAM_CONFIG_KEY_STREAM_AUDIO_SINK_DELAY \
         ACCMD_CONFIG_KEY_STREAM_AUDIO_SINK_DELAY
#define STREAM_CONFIG_KEY_STREAM_AUDIO_SOURCE_METADATA_ENABLE \
         ACCMD_CONFIG_KEY_STREAM_AUDIO_SOURCE_METADATA_ENABLE
#define STREAM_CONFIG_KEY_STREAM_CODEC_INPUT_GAIN \
         ACCMD_CONFIG_KEY_STREAM_CODEC_INPUT_GAIN
#define STREAM_CONFIG_KEY_STREAM_CODEC_OUTPUT_GAIN \
         ACCMD_CONFIG_KEY_STREAM_CODEC_OUTPUT_GAIN
#define STREAM_CONFIG_KEY_STREAM_DIGITAL_MIC_INPUT_GAIN \
         ACCMD_CONFIG_KEY_STREAM_DIGITAL_MIC_INPUT_GAIN
#define STREAM_CONFIG_KEY_STREAM_RM_ENABLE_DEFERRED_KICK \
         ACCMD_CONFIG_KEY_STREAM_RM_ENABLE_DEFERRED_KICK
#define STREAM_CONFIG_KEY_STREAM_RM_ENABLE_HW_ADJUST \
         ACCMD_CONFIG_KEY_STREAM_RM_ENABLE_HW_ADJUST
#define STREAM_CONFIG_KEY_STREAM_RM_ENABLE_SW_ADJUST \
         ACCMD_CONFIG_KEY_STREAM_RM_ENABLE_SW_ADJUST
#define STREAM_CONFIG_KEY_STREAM_RM_USE_RATE_ADJUST_OPERATOR \
         ACCMD_CONFIG_KEY_STREAM_RM_USE_RATE_ADJUST_OPERATOR
#define STREAM_CONFIG_KEY_STREAM_SPDIF_OUTPUT_RATE \
         ACCMD_CONFIG_KEY_STREAM_SPDIF_OUTPUT_RATE
#define STREAM_CONFIG_KEY_STREAM_AUDIO_SAMPLE_SIZE \
         ACCMD_CONFIG_KEY_STREAM_AUDIO_SAMPLE_SIZE
#define STREAM_CONFIG_KEY_STREAM_SPDIF_SET_EP_FORMAT \
         ACCMD_CONFIG_KEY_STREAM_SPDIF_SET_EP_FORMAT
#define STREAM_CONFIG_KEY_STREAM_CODEC_AOV_MODE_ON \
        ACCMD_CONFIG_KEY_STREAM_CODEC_AOV_MODE_ON
#define STREAM_CONFIG_KEY_STREAM_DIGITAL_MIC_AOV_MODE_ON \
        ACCMD_CONFIG_KEY_STREAM_DIGITAL_MIC_AOV_MODE_ON
#endif /* CHIP_BASE_HYDRA */

/**
 * ACCMD enum members are hidden from stream by using aliases
 */

#if defined(HAVE_ANC_HARDWARE)

#ifdef INSTALL_ANC_V2P0
#define STREAM_ANC_IIR_FILTER_FB_NUM_COEFFS \
         ACCMD_ANC_IIR_FILTER32_FB_NUM_COEFFS
#define STREAM_ANC_IIR_FILTER_FFA_NUM_COEFFS \
         ACCMD_ANC_IIR_FILTER32_FFA_NUM_COEFFS
#define STREAM_ANC_IIR_FILTER_FFB_NUM_COEFFS \
         ACCMD_ANC_IIR_FILTER32_FFB_NUM_COEFFS
#define STREAM_ANC_IIR_FILTER_OCTETS_PER_COEFF \
         ACCMD_ANC_IIR_FILTER32_OCTETS_PER_COEFF
#define STREAM_ANC_IIR_FILTER_COEFF_MSW_SHIFT \
         ACCMD_ANC_IIR_FILTER32_COEFF_MSW_SHIFT
#else
#define STREAM_ANC_IIR_FILTER_FB_NUM_COEFFS \
         ACCMD_ANC_IIR_FILTER_FB_NUM_COEFFS
#define STREAM_ANC_IIR_FILTER_FFA_NUM_COEFFS \
         ACCMD_ANC_IIR_FILTER_FFA_NUM_COEFFS
#define STREAM_ANC_IIR_FILTER_FFB_NUM_COEFFS \
         ACCMD_ANC_IIR_FILTER_FFB_NUM_COEFFS
#define STREAM_ANC_IIR_FILTER_OCTETS_PER_COEFF \
         ACCMD_ANC_IIR_FILTER_OCTETS_PER_COEFF
#endif

#define STREAM_ANC_CONTROL_ACCESS_SELECT_ENABLES_SHIFT \
         ACCMD_ANC_CONTROL_ACCESS_SELECT_ENABLES_SHIFT

#define STREAM_CONFIG_KEY_STREAM_ANC_INSTANCE \
         ACCMD_CONFIG_KEY_STREAM_ANC_INSTANCE
#define STREAM_CONFIG_KEY_STREAM_ANC_INPUT \
         ACCMD_CONFIG_KEY_STREAM_ANC_INPUT
#define STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_ENABLE \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_ENABLE
#define STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_ENABLE \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_ENABLE
#define STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_ENABLE \
         ACCMD_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_ENABLE
#define STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_SHIFT \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_SHIFT
#define STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_SHIFT \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_SHIFT
#define STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_SHIFT \
         ACCMD_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_SHIFT
#define STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFA_GAIN
#define STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFB_GAIN
#define STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN \
         ACCMD_CONFIG_KEY_STREAM_ANC_FB_GAIN
#define STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN_SHIFT \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFA_GAIN_SHIFT
#define STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN_SHIFT \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFB_GAIN_SHIFT
#define STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN_SHIFT \
         ACCMD_CONFIG_KEY_STREAM_ANC_FB_GAIN_SHIFT
#define STREAM_CONFIG_KEY_STREAM_ANC_FFA_ADAPT_ENABLE \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFA_ADAPT_ENABLE
#define STREAM_CONFIG_KEY_STREAM_ANC_FFB_ADAPT_ENABLE \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFB_ADAPT_ENABLE
#define STREAM_CONFIG_KEY_STREAM_ANC_FB_ADAPT_ENABLE \
         ACCMD_CONFIG_KEY_STREAM_ANC_FB_ADAPT_ENABLE
#define STREAM_CONFIG_KEY_STREAM_ANC_CONTROL \
         ACCMD_CONFIG_KEY_STREAM_ANC_CONTROL
#define STREAM_CONFIG_KEY_STREAM_ANC_FFA_IIR_COEFFS \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFA_IIR_COEFFS
#define STREAM_CONFIG_KEY_STREAM_ANC_FFB_IIR_COEFFS \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFB_IIR_COEFFS
#define STREAM_CONFIG_KEY_STREAM_ANC_FB_IIR_COEFFS \
         ACCMD_CONFIG_KEY_STREAM_ANC_FB_IIR_COEFFS
#define STREAM_CONFIG_KEY_STREAM_ANC_FFA_LPF_COEFFS \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFA_LPF_COEFFS
#define STREAM_CONFIG_KEY_STREAM_ANC_FFB_LPF_COEFFS \
         ACCMD_CONFIG_KEY_STREAM_ANC_FFB_LPF_COEFFS
#define STREAM_CONFIG_KEY_STREAM_ANC_FB_LPF_COEFFS \
         ACCMD_CONFIG_KEY_STREAM_ANC_FB_LPF_COEFFS

#ifdef INSTALL_ANC_V2P0
#define STREAM_CONFIG_KEY_STREAM_ANC_CONTROL_1 \
         ACCMD_CONFIG_KEY_STREAM_ANC_CONTROL_1
#endif

#define STREAM_ANC_CONTROL_FB_TUNE_DSM_EN_MASK \
         ACCMD_ANC_CONTROL_FB_TUNE_DSM_EN_MASK
#define STREAM_ANC_CONTROL_FBMON_SEL_MASK \
        ACCMD_ANC_CONTROL_FBMON_SEL_MASK
#define STREAM_ANC_CONTROL_DMIC_X2_A_SEL_MASK \
        ACCMD_ANC_CONTROL_DMIC_X2_A_SEL_MASK
#define STREAM_ANC_CONTROL_DMIC_X2_B_SEL_MASK \
        ACCMD_ANC_CONTROL_DMIC_X2_B_SEL_MASK
#define STREAM_ANC_CONTROL_FBMON_SEL_POSITION \
        ACCMD_ANC_CONTROL_FBMON_SEL_POSITION


#ifdef INSTALL_ANC_V2P0
#define STREAM_CONFIG_KEY_STREAM_ANC_RX_MIX_FFA_GAIN \
         ACCMD_CONFIG_KEY_STREAM_ANC_RX_MIX_FFA_GAIN
#define STREAM_CONFIG_KEY_STREAM_ANC_RX_MIX_FFA_SHIFT \
         ACCMD_CONFIG_KEY_STREAM_ANC_RX_MIX_FFA_SHIFT
#define STREAM_CONFIG_KEY_STREAM_ANC_RX_MIX_FFB_GAIN \
         ACCMD_CONFIG_KEY_STREAM_ANC_RX_MIX_FFB_GAIN
#define STREAM_CONFIG_KEY_STREAM_ANC_RX_MIX_FFB_SHIFT \
         ACCMD_CONFIG_KEY_STREAM_ANC_RX_MIX_FFB_SHIFT
#endif

#endif /* HAVE_ANC_HARDWARE */

#ifdef INSTALL_ANC_V2P0
typedef uint32 STREAM_ANC_IIR_COEFF_TYPE;
#else
typedef uint16 STREAM_ANC_IIR_COEFF_TYPE;
#endif

typedef enum
{
    STREAM_ANC_INSTANCE_ANC0_MASK    = (1 << (ACCMD_ANC_INSTANCE_ANC0_ID - 1)),
    STREAM_ANC_INSTANCE_ANC1_MASK    = (1 << (ACCMD_ANC_INSTANCE_ANC1_ID - 1)),
    STREAM_ANC_INSTANCE_ANC01_MASK   = (STREAM_ANC_INSTANCE_ANC0_MASK | STREAM_ANC_INSTANCE_ANC1_MASK)
} STREAM_ANC_INSTANCE_MASK;


#if defined(CHIP_BASE_HYDRA)
typedef ACCMD_INFO_KEY STREAM_INFO_KEY;

#define STREAM_INFO_KEY_AUDIO_SAMPLE_RATE \
         ACCMD_INFO_KEY_AUDIO_SAMPLE_RATE
#define STREAM_INFO_KEY_AUDIO_LOCALLY_CLOCKED \
         ACCMD_INFO_KEY_AUDIO_LOCALLY_CLOCKED
#define STREAM_INFO_KEY_AUDIO_SAMPLE_PERIOD_DEVIATION \
         ACCMD_INFO_KEY_AUDIO_SAMPLE_PERIOD_DEVIATION
#define STREAM_INFO_KEY_ENDPOINT_EXISTS \
         ACCMD_INFO_KEY_ENDPOINT_EXISTS
#define STREAM_INFO_KEY_AUDIO_HW_RM_AVAILABLE \
         ACCMD_INFO_KEY_AUDIO_HW_RM_AVAILABLE

typedef enum
{
    STREAM_AUDIO_SAMPLE_SIZE_13BITS_16CLKS = ACCMD_STREAM_AUDIO_SAMPLE_SIZE_13BITS_16CLKS,
    STREAM_AUDIO_SAMPLE_SIZE_16BITS_16CLKS = ACCMD_STREAM_AUDIO_SAMPLE_SIZE_16BITS_16CLKS,
    STREAM_AUDIO_SAMPLE_SIZE_8BITS_16CLKS = ACCMD_STREAM_AUDIO_SAMPLE_SIZE_8BITS_16CLKS,
    STREAM_AUDIO_SAMPLE_SIZE_8BITS_8CLKS = ACCMD_STREAM_AUDIO_SAMPLE_SIZE_8BITS_8CLKS,
    STREAM_AUDIO_SAMPLE_SIZE_16 = ACCMD_STREAM_AUDIO_SAMPLE_SIZE_16,
    STREAM_AUDIO_SAMPLE_SIZE_24 = ACCMD_STREAM_AUDIO_SAMPLE_SIZE_24
} STREAM_AUDIO_SAMPLE_SIZE;

#else

#error "Adaptor not implemented for this chip"

#endif

#endif /* STREAM_TYPE_ALIAS_H */
