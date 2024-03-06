/*******************************************************************************
 * Copyright (c) 2009 - 2020 Qualcomm Technologies International, Ltd.
*******************************************************************************/

/**
 * \file  stream_anc.c
 * \ingroup stream
 *
 * Control the ANC audio HW <br>
 * This file contains the stream ANC operator shim functions. <br>
 *
 *  \note This file contains the operator access functions for Active Noise Cancellation (ANC).
 *  It consists of several shim functions that provide an interface between instances of
 *  (downloaded) Kymera capabilities and the ANC HAL layer (without which access would not
 *  be possible).
 *
 */

#if !defined(HAVE_SIDE_TONE_HARDWARE)
    #error "HAVE_SIDE_TONE_HARDWARE must be defined "
           "when INSTALL_UNINTERRUPTABLE_ANC is defined"
#endif /* !defined(HAVE_SIDE_TONE_HARDWARE) */

/*******************************************************************************
Include Files
*/


#include "hal_audio.h"
#include "patch/patch.h"
#include "stream/stream_for_override.h"
#include "stream/stream_for_opmgr.h"
#include "stream/stream_for_ops.h"
#include "stream/stream_for_adaptors.h"
#include "stream/stream_for_anc.h"
#include "stream/stream_endpoint.h"
#include "audio_hwm_anc.h"
#if defined(INSTALL_CLK_MGR)
#include "clk_mgr/clk_mgr.h"
#endif

/*******************************************************************************
 * Private macros/consts
 */

#ifdef STREAM_ANC_ENABLE_L5_DBG_MSG

#define STREAM_ANC_L5_DBG_MSG(x)                  L5_DBG_MSG(x)
#define STREAM_ANC_L5_DBG_MSG1(x, a)              L5_DBG_MSG1(x, a)
#define STREAM_ANC_L5_DBG_MSG2(x, a, b)           L5_DBG_MSG2(x, a, b)
#define STREAM_ANC_L5_DBG_MSG3(x, a, b, c)        L5_DBG_MSG3(x, a, b, c)
#define STREAM_ANC_L5_DBG_MSG4(x, a, b, c, d)     L5_DBG_MSG4(x, a, b, c, d)
#define STREAM_ANC_L5_DBG_MSG5(x, a, b, c, d, e)  L5_DBG_MSG5(x, a, b, c, d, e)

#else  /* STREAM_ANC_ENABLE_L5_DBG_MSG */

#define STREAM_ANC_L5_DBG_MSG(x)                  ((void)0)
#define STREAM_ANC_L5_DBG_MSG1(x, a)              ((void)0)
#define STREAM_ANC_L5_DBG_MSG2(x, a, b)           ((void)0)
#define STREAM_ANC_L5_DBG_MSG3(x, a, b, c)        ((void)0)
#define STREAM_ANC_L5_DBG_MSG4(x, a, b, c, d)     ((void)0)
#define STREAM_ANC_L5_DBG_MSG5(x, a, b, c, d, e)  ((void)0)

#endif /* STREAM_ANC_ENABLE_L5_DBG_MSG */

/* Macros for eANC (parallal ANC) */
#define ANC_INSTANCE_MASK_POSITION         (16)
#define ANC_PARAMETER_VALUE_MASK           (0xFFFF)
#define VALUE_TO_ANC_INSTANCE_MASK(value)  ((value & (HWM_ANC_INSTANCE_ANC01_MASK << ANC_INSTANCE_MASK_POSITION)) >> ANC_INSTANCE_MASK_POSITION)
#define INSTANCE_MASK_LSB                  (0x01)
/*******************************************************************************
Private Function Declarations
*/
#ifndef INSTALL_ANC_V2P0
#if defined(INSTALL_CLK_MGR) && !defined(UNIT_TEST_BUILD)
static inline bool kcodec_is_clocked(void)
{
    return clk_mgr_kcodec_is_clocked();
}
#else
#define kcodec_is_clocked() TRUE
#endif
#endif /* INSTALL_ANC_V2P0 */

/*******************************************************************************
Private Function Definitions
*/

/**
 * \brief Derive an ANC filter ID from the source configure key.
 *
 * \param key ANC instance ID (e.g. ANC0, ANC1).
 *
 * \return path_id Path ID that is derived from the key.
 */
static STREAM_ANC_PATH key_to_id(STREAM_CONFIG_KEY key);

/* Variable to store the anc license check status.*/ 
bool anc_license_check = FALSE;

/*ANC enable status variable captures STATUS_UNKNOWN_COMMAND, STATUS_CMD_PENDING, STATUS_CMD_FAILED and STATUS_OK statuses*/
STATUS_KYMERA stream_anc_enable_status = STATUS_UNKNOWN_COMMAND;
/**
 * \brief dummy response callback function.
 *
 * \param dummy_con_id dummy con_id in place of Kymera con_id
 *        dummy_status stores the status of the call
 *
 *        anc_license_check to be TRUE if status is ok. 
 *        stream_anc_enable_status holds the status.
 */
static bool stream_anc_dummy_callback(CONNECTION_LINK dummy_con_id,
                                      STATUS_KYMERA dummy_status)
{
    patch_fn_shared(stream_anc);

    if(dummy_status == STATUS_OK)
    {
       anc_license_check = TRUE;
    }
    stream_anc_enable_status = dummy_status;

    STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_dummy_callback(): "
                           "ANC enabling, anc_license_check=0x%x, stream_anc_enable_status=0x%x",
                           anc_license_check, stream_anc_enable_status);

    return TRUE;
}
/*******************************************************************************
Public Function Definitions
*/

/**
 * \brief Configure an ANC IIR filter (sets foreground and background IIR coefficients)
 *
 * \param instance   ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id    ANC input path ID (e.g. FFA, FFB, FB).
 * \param num_coeffs Number of coefficients.
 * \param coeffs     Pointer to an array of IIR coefficients.
 *
 * \return TRUE if successful
 */
bool stream_anc_set_anc_iir_coeffs(STREAM_ANC_INSTANCE instance,
                                   STREAM_ANC_PATH path_id,
                                   unsigned num_coeffs,
                                   const STREAM_ANC_IIR_COEFF_TYPE *coeffs)
{
    patch_fn_shared(stream_anc);
    return audio_hwm_anc_set_anc_iir_background_filter(instance,
                                                       path_id,
                                                       (uint16) num_coeffs,
                                                       (const HWM_ANC_IIR_COEFF_TYPE *)coeffs);
}

/**
 * \brief Configure an ANC IIR filter (sets foreground IIR coefficients). 
 *
 * \param instance   ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id    ANC input path ID (e.g. FFA, FFB, FB).
 * \param num_coeffs Number of coefficients.
 * \param coeffs     Pointer to an array of IIR coefficients.
 *
 * \return TRUE if successful

 * NOTE: Need to seperately update background coeffs when using this API
 */
bool stream_anc_set_anc_iir_foreground_coeffs(STREAM_ANC_INSTANCE instance,
                                   STREAM_ANC_PATH path_id,
                                   unsigned num_coeffs,
                                   const STREAM_ANC_IIR_COEFF_TYPE *coeffs)
{
    patch_fn_shared(stream_anc);
    return audio_hwm_anc_set_anc_iir_foreground_filter(instance,
                                                       path_id,
                                                       (uint16) num_coeffs,
                                                       (const HWM_ANC_IIR_COEFF_TYPE *)coeffs);
}


/**
 * \brief Get ANC IIR filter (gets the coefficients)
 *
 * \param instance   ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id    ANC input path ID (e.g. FFA, FFB, FB).
 * \param num_coeffs Number of coefficients.
 * \param coeffs     Pointer to an array of IIR coefficients.
 *
 * \return TRUE if successful
 */
bool stream_anc_get_anc_iir_coeffs(STREAM_ANC_INSTANCE instance,
                                   STREAM_ANC_PATH path_id,
                                   unsigned num_coeffs,
                                   STREAM_ANC_IIR_COEFF_TYPE *coeffs)
{
    patch_fn_shared(stream_anc);
    return audio_hwm_anc_get_anc_iir_filter(instance,
                                            path_id,
                                            (uint16) num_coeffs,
                                            (HWM_ANC_IIR_COEFF_TYPE *) coeffs);
}

/**
 * \brief Select the currently active IIR coefficient set
 *
 * \param anc_instance ANC instance mask (e.g. ANC0, ANC1, ANC01).
 * \param coeff_set coefficient set 0: Foreground, 1: background.
 *
 * \note  Coefficients for the FFA, FFB and FB IIR filters are banked
 *        (LPF shift coefficients are not banked)
 */
void stream_anc_select_active_iir_coeffs(STREAM_ANC_INSTANCE_MASK anc_instance_mask, STREAM_ANC_BANK coeff_set)
{
    audio_hwm_anc_select_active_iir_coeffs(anc_instance_mask, coeff_set);
}



/**
 * \brief Copy the foreground coefficient set to the background coefficient set
 *
 * \param anc_instance ANC instance mask (e.g. ANC0, ANC1, ANC01).
 *
 * \note  Coefficients for the FFA, FFB and FB IIR filters are banked
 *        (LPF shift coefficients are not banked)
 */
void stream_anc_update_background_iir_coeffs(STREAM_ANC_INSTANCE_MASK anc_instance_mask)
{
    audio_hwm_anc_update_background_iir_coeffs(anc_instance_mask);
}


/**
 * \brief Configure an ANC LPF filter (sets the LPF coefficients)
 *
 * \param instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id  ANC input path ID (e.g. FFA, FFB, FB).
 * \param shift1   Coefficient 1 expressed as a shift.
 * \param shift2   Coefficient 2 expressed as a shift.
 *
 * \return TRUE if successful
 */
bool stream_anc_set_anc_lpf_coeffs(STREAM_ANC_INSTANCE instance,
                                   STREAM_ANC_PATH path_id,
                                   uint16 shift1, uint16 shift2)
{
    patch_fn_shared(stream_anc);

#ifdef INSTALL_ANC_V2P0
    if(!audio_hwm_anc_is_clocked())
#else
    if(!kcodec_is_clocked())
#endif
    {
        return FALSE;
    }

    return audio_hwm_anc_set_anc_lpf_filter(instance, path_id, shift1, shift2);
}

#if !defined(UNIT_TEST_BUILD)

#define STREAM_ANC_CONTROL_EN_0_FBTUNEOUT0_SEL_MASK 0x0001
#define STREAM_ANC_CONTROL_EN_0_FBTUNEOUT1_SEL_MASK 0x0002
#define STREAM_ANC_CONTROL_EN_1_FBTUNEOUT0_SEL_MASK 0x0004
#define STREAM_ANC_CONTROL_EN_1_FBTUNEOUT1_SEL_MASK 0x0008
#define STREAM_ANC_CONTROL_EN_2_FBTUNEOUT0_SEL_MASK 0x0010
#define STREAM_ANC_CONTROL_EN_2_FBTUNEOUT1_SEL_MASK 0x0020
#define STREAM_ANC_CONTROL_EN_3_FBTUNEOUT0_SEL_MASK 0x0040
#define STREAM_ANC_CONTROL_EN_3_FBTUNEOUT1_SEL_MASK 0x0080

#define DEC_CHAIN_0 0
#define DEC_CHAIN_1 1
#define DEC_CHAIN_2 2
#define DEC_CHAIN_3 3

/**
 * \brief Configure ANC tuning options
 *
 * \param instance ANC instance ID (e.g. ANC0, ANC1).
 * \param chain    Which chain must be tuned.
 *
 */
void stream_anc_set_anc_tune(STREAM_ANC_INSTANCE instance,
                             unsigned chain)
{
    uint32 enable;
    uint32 select;

    patch_fn_shared(stream_anc);

    switch (chain)
    {
        case DEC_CHAIN_3:
            select = STREAM_ANC_CONTROL_EN_3_FBTUNEOUT0_SEL_MASK |
                     STREAM_ANC_CONTROL_EN_3_FBTUNEOUT1_SEL_MASK;
            enable = STREAM_ANC_CONTROL_EN_3_FBTUNEOUT1_SEL_MASK;
            break;
        case DEC_CHAIN_2:
            select = STREAM_ANC_CONTROL_EN_2_FBTUNEOUT0_SEL_MASK |
                     STREAM_ANC_CONTROL_EN_2_FBTUNEOUT1_SEL_MASK;
            enable = STREAM_ANC_CONTROL_EN_2_FBTUNEOUT0_SEL_MASK;
            break;
        case DEC_CHAIN_1:
            select = STREAM_ANC_CONTROL_EN_1_FBTUNEOUT0_SEL_MASK |
                     STREAM_ANC_CONTROL_EN_1_FBTUNEOUT1_SEL_MASK;
            enable = STREAM_ANC_CONTROL_EN_1_FBTUNEOUT1_SEL_MASK;
           break;
        case DEC_CHAIN_0:
            select = STREAM_ANC_CONTROL_EN_0_FBTUNEOUT0_SEL_MASK |
                     STREAM_ANC_CONTROL_EN_0_FBTUNEOUT1_SEL_MASK;
            enable = STREAM_ANC_CONTROL_EN_0_FBTUNEOUT0_SEL_MASK;
           break;
        default:
            select = 0;
            enable = 0;
            break;
    }

    enable = enable << 16;
    select = select << 16;
    hal_audio_set_anc_control(instance, enable, select);
}

/**
 * \brief Copy the foreground gains to the background gains
 *
 * \param anc_instance_mask ANC instance mask (e.g. ANC0, ANC1, ANC01).
 *
 * \note  Gains for the FFA, FFB, and FB LPF are shadowed
 *        (but gain shifts are not)
 */
void stream_anc_update_background_gains(STREAM_ANC_INSTANCE_MASK anc_instance_mask)
{
    patch_fn_shared(stream_anc);
    hal_update_background_gains(anc_instance_mask);
}

/**
 * \brief Retrieve the ANC enable/disable state
 *
 * \param anc_enable_0_ptr Pointer to result (bitfield controlling instance ANC0 signal paths).
 * \param anc_enable_1_ptr Pointer to result (bitfield controlling instance ANC1 signal paths).
 *
 */
void stream_get_anc_enable(uint16 *anc_enable_0_ptr, uint16 *anc_enable_1_ptr)
{
    /* Call the HAL layer to determine the ANC enable/disable state */
    hal_get_audio_anc_stream_anc_enable(anc_enable_0_ptr, anc_enable_1_ptr);
}

#if defined(INSTALL_ANC_CLIP_THRESHOLD)
/**
 * \brief Configure the clipping/threshold detection ANC output level
 *
 * \param anc_instance ANC instance
 * \param level threshold level to configure
 *
 * \return none
 */
void stream_anc_set_clip_level(STREAM_ANC_INSTANCE anc_instance,
                               uint32 level)
{
    hal_audio_anc_set_clip_level(anc_instance, level);
}

/**
 * \brief Enable/disable the ANC output threshold detector
 *
 * \param anc_instance ANC instance
 * \param callback NULL: disable ANC threshold detection
 *                 non-NUll: pointer to function to be called
 *                           on exceeding ANC detection threshold
 *
 * \return none
 */
void stream_anc_detect_enable(STREAM_ANC_INSTANCE anc_instance,
                              void (*callback)(void))
{
    hal_audio_anc_detect_enable(anc_instance, callback);
}
#endif /* defined(INSTALL_ANC_CLIP_THRESHOLD) */

#endif /* UNIT_TEST_BUILD */
/**
 * \brief Wrapper to enable/disable ANC with license check.
 *
 * \param con_id Feature ID (passed as first parameter to the callback)
 * \param stream_anc_instance_number The number of ANC instances/blocks can be used.
 * \param anc_enable Bitfield controlling instance ANC0 and ANC1 signal paths.
 *                   ANC enable Array of 1 used to control instance ANC0 in Earbud software since mono channel.
 *                   Array of 2 used to control instance ANC0 and ANC1 in Headphone software since stereo channel.
 *
 * \param resp_callback callback function pointer for sending the response.
 *
 * \note: Calls the secure ANC interface and supplies a user callback of
 * the form:
 *
 * bool stream_anc_dummy_callback(unsigned dummy_con_id, unsigned dummy_status)
 *
 * This can be used to determine the completion status of the command.
 */
void stream_anc_enable_from_adaptor(CONNECTION_LINK con_id,
                                    STREAM_ANC_INSTANCE stream_anc_instance_number,
                                    uint16 *anc_enable,
                                    bool (*resp_callback)(CONNECTION_LINK con_id,
                                                          STATUS_KYMERA status))
{
    /* Call the existing ANC enable wrapper */
    audio_hwm_anc_stream_anc_enable_wrapper(con_id, anc_enable[0], anc_enable[1], resp_callback);
}

/**
 * \brief Wrapper to enable/disable ANC with license check from anc tuning capability.
 *
 * \param con_id Feature ID (passed as first parameter to the callback)
 * \param stream_anc_instance_number The number of ANC instances/blocks can be used.
 * \param anc_enable Bitfield controlling instance ANC0 and ANC1 signal paths.
 *                   ANC enable Array of 1 used to control instance ANC0 in Earbud software since mono channel.
 *                   Array of 2 used to control instance ANC0 and ANC1 in Headphone software since stereo channel.
 *
 * \param resp_callback callback function pointer for sending the response.
 *
 * \note: Calls the secure ANC interface and supplies a user callback of
 * the form:
 *
 * bool stream_anc_dummy_callback(unsigned dummy_con_id, unsigned dummy_status)
 *
 * This can be used to determine the completion status of the command.
 */
void stream_anc_enable(STREAM_ANC_INSTANCE stream_anc_instance_number,
                       uint16 *anc_enable,
                       bool (*resp_callback)(void))
{
    patch_fn_shared(stream_anc);

    /*License check flag set to FALSE before check*/
    anc_license_check = FALSE;

    /*To provide anc enable pending status to anc tuning capablity*/
    stream_anc_enable_status = STATUS_CMD_PENDING;

    STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_enable(): "
                           "ANC enabling, anc_license_check=0x%x, stream_anc_enable_status=0x%x",
                           anc_license_check, stream_anc_enable_status);
    stream_anc_enable_op_callback(stream_anc_instance_number, anc_enable, resp_callback);
}

/**
 * \brief Wrapper to check the pending status by anc tuning capability.
 */
bool stream_anc_enable_pending_status(void)
{
     /* Check the pending status */
    if(stream_anc_enable_status == STATUS_CMD_PENDING){
       return TRUE;
    }
    return FALSE;
}

/**
 * \brief Operator callback for stream ANC enable.
 *
 * \param stream_anc_instance_number Number of ANC instances to be enabled.
 * \param anc_enable Array to result (bitfield controlling instance ANC0, ANC1 signal paths).
 * \param resp_callback ANC operator callback after the license check.
 *
 */
void stream_anc_enable_op_callback(STREAM_ANC_INSTANCE stream_anc_instance_number,
                                   uint16 *anc_enable,
                                   bool (*resp_callback)(void))
{
    patch_fn_shared(stream_anc);

    /* Call the existing ANC enable wrapper */
    audio_hwm_anc_stream_anc_enable_wrapper(DUMMY_CON_ID, anc_enable[0], anc_enable[1], stream_anc_dummy_callback);

    STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_enable_op_callback(): "
                           "ANC enabling, anc_license_check=0x%x, stream_anc_enable_status=0x%x",
                           anc_license_check, stream_anc_enable_status);

    /* Operator callback after the license check */
    if(anc_license_check)
    {
       resp_callback();
    }else if((anc_enable[0] == 0) && (anc_enable[1] == 0) && !(stream_anc_enable_status == STATUS_CMD_FAILED))
    {
       resp_callback();
    }
}

/**
 * \brief Configure the ANC path gains (background gains).
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (e.g. FFA, FFB, FB).
 * \param gain Gain applied to filter (8 bit linear gain value).
 *
 */
bool stream_anc_set_anc_fine_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 gain)
{
    return audio_hwm_anc_source_config_background_gain(anc_instance, path_id, gain);
}

/**
 * \brief Configure the ANC path gains. (foreground gains)
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (e.g. FFA, FFB, FB).
 * \param gain Gain applied to filter (8 bit linear gain value).
 *
 */
bool stream_anc_set_anc_foreground_fine_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 gain)
{
    return audio_hwm_anc_source_config_foreground_gain(anc_instance, path_id, gain);
}

/**
 * \brief Configure the ANC path gain shift (gain exponent).
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (e.g. FFA, FFB, FB).
 * \param shift Gain shift applied to filter (4 bit shift value).
 *
 */
bool stream_anc_set_anc_coarse_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 shift)
{
    return audio_hwm_anc_source_config_gain_shift(anc_instance, path_id, shift);
}

#ifdef INSTALL_ANC_V2P0
/**
 * \brief Configure the ANC FFa/b path to the Rx mixer gain.
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (FFA, FFB).
 * \param gain Gain applied to filter (8 bit linear gain value).
 *
 */
bool stream_anc_set_anc_rx_mix_foreground_fine_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 gain)
{
    bool result;
    patch_fn_shared(stream_anc);
    if(path_id == STREAM_ANC_PATH_FFA_ID)
    {
        result = audio_hwm_anc_source_config_rx_mix_ffa_foreground_gain(anc_instance, gain);
    }
    else if(path_id == STREAM_ANC_PATH_FFB_ID)
    {
        result = audio_hwm_anc_source_config_rx_mix_ffb_foreground_gain(anc_instance, gain);
    }
    else
    {
        /* RxMix is applicable only for FFa & FFb paths. */
        result = FALSE;
    }
    return result;
}

/**
 * \brief Gain Shift Value for ANC FF path A/B to the Rx Mixer.
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (FFA, FFB).
 * \param shift Gain shift applied to filter (4 bit shift value).
 *
 */
bool stream_anc_set_anc_rx_mix_coarse_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 shift)
{
    bool result;
    patch_fn_shared(stream_anc);
    if(path_id == STREAM_ANC_PATH_FFA_ID)
    {
        result = audio_hwm_anc_source_config_rx_mix_ffa_gain_shift(anc_instance, shift);
    }
    else if(path_id == STREAM_ANC_PATH_FFB_ID)
    {
        result = audio_hwm_anc_source_config_rx_mix_ffb_gain_shift(anc_instance, shift);
    }
    else
    {
        /* RxMix is applicable only for FFa & FFb paths. */
        result = FALSE;
    }
    return result;
}
#endif /* INSTALL_ANC_V2P0 */

/**
 * \brief Get ANC path gains.
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (e.g. FFA, FFB, FB).
 * \param gain pointer to Gain
 *
 */
bool stream_anc_get_anc_fine_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 *gain)
{
    return audio_hwm_anc_get_anc_gain(anc_instance, path_id, gain);
}

/**
 * \brief Get ANC path gain shift (gain exponent).
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (e.g. FFA, FFB, FB).
 * \param shift pointer to Gain shift
 *
 */
bool stream_anc_get_anc_coarse_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 *shift)
{
    return audio_hwm_anc_get_anc_gain_shift(anc_instance, path_id, shift);
}

/**
 * \brief Place-holder function for patching purposes.
 *
 * \param ptr Pointer to function parameters (to be used as required)
 *
 */
void stream_anc_user1(void *ptr)
{
    patch_fn_shared(stream_anc);
}

/**
 * \brief Place-holder function for patching purposes.
 *
 * \param ptr Pointer to function parameters (to be used as required)
 *
 */
void stream_anc_user2(void *ptr)
{
    patch_fn_shared(stream_anc);
}

bool stream_anc_configure_input(ENDPOINT *ep, STREAM_ANC_PATH path)
{
    STREAM_CONFIG_KEY key = STREAM_CONFIG_KEY_STREAM_ANC_INPUT;
    return stream_configure_connected_to_endpoint(ep, key, (uint32) path);
}

bool stream_anc_configure_instance(ENDPOINT *ep, STREAM_ANC_INSTANCE instance)
{
    STREAM_CONFIG_KEY key = STREAM_CONFIG_KEY_STREAM_ANC_INSTANCE;
    return stream_configure_connected_to_endpoint(ep, key, (uint32) instance);
}

/* TODO: proper bitfield description. */
bool stream_anc_configure_control(ENDPOINT *ep, uint32 bitfield)
{
    STREAM_CONFIG_KEY key = STREAM_CONFIG_KEY_STREAM_ANC_CONTROL;
    return stream_configure_connected_to_endpoint(ep, key, (uint32) bitfield);
}

bool stream_anc_configure_gain(ENDPOINT *ep,
                               STREAM_ANC_PATH path,
                               unsigned value)
{
    STREAM_CONFIG_KEY key;
    patch_fn_shared(stream_anc);

    switch (path)
    {
        case STREAM_ANC_PATH_FFA_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN;
            break;
        case STREAM_ANC_PATH_FFB_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN;
            break;
        case STREAM_ANC_PATH_FB_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN;
            break;
        default:
            return FALSE;
    }
    return stream_configure_connected_to_endpoint(ep, key, (uint32) value);
}

bool stream_anc_configure_gain_shift(ENDPOINT *ep,
                                     STREAM_ANC_PATH path,
                                     unsigned value)
{
    STREAM_CONFIG_KEY key;
    patch_fn_shared(stream_anc);

    switch (path)
    {
        case STREAM_ANC_PATH_FFA_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN_SHIFT;
            break;
        case STREAM_ANC_PATH_FFB_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN_SHIFT;
            break;
        case STREAM_ANC_PATH_FB_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN_SHIFT;
            break;
        default:
            return FALSE;
    }
    return stream_configure_connected_to_endpoint(ep, key, (uint32) value);
}

bool stream_anc_configure_dc_filter_enable(ENDPOINT *ep,
                                           STREAM_ANC_PATH path,
                                           unsigned value)
{
    STREAM_CONFIG_KEY key;
    patch_fn_shared(stream_anc);

    switch (path)
    {
        case STREAM_ANC_PATH_FFA_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_ENABLE;
            break;
        case STREAM_ANC_PATH_FFB_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_ENABLE;
            break;
        case STREAM_ANC_PATH_SM_LPF_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_ENABLE;
            break;
        default:
            return FALSE;
    }
    return stream_configure_connected_to_endpoint(ep, key, (uint32) value);
}

bool stream_anc_configure_dc_filter_shift(ENDPOINT *ep,
                                          STREAM_ANC_PATH path,
                                          unsigned value)
{
    STREAM_CONFIG_KEY key;
    patch_fn_shared(stream_anc);

    switch (path)
    {
        case STREAM_ANC_PATH_FFA_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_SHIFT;
            break;
        case STREAM_ANC_PATH_FFB_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_SHIFT;
            break;
        case STREAM_ANC_PATH_SM_LPF_ID:
            key = STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_SHIFT;
            break;
        default:
            return FALSE;
    }
    return stream_configure_connected_to_endpoint(ep, key, (uint32) value);
}

bool stream_anc_configure_dmic_x2_enable(ENDPOINT *ep,
                                         STREAM_ANC_PATH path,
                                         unsigned value)
{
    uint32 ff_dmic_x2_mask;
    patch_fn_shared(stream_anc);

    switch (path)
    {
        case STREAM_ANC_PATH_FFA_ID:
            ff_dmic_x2_mask = 
             (STREAM_ANC_CONTROL_DMIC_X2_A_SEL_MASK \
                << STREAM_ANC_CONTROL_ACCESS_SELECT_ENABLES_SHIFT);

            if(value == 1)
            {
               ff_dmic_x2_mask |= STREAM_ANC_CONTROL_DMIC_X2_A_SEL_MASK;
            }

            break;

        case STREAM_ANC_PATH_FFB_ID:
            ff_dmic_x2_mask = 
             (STREAM_ANC_CONTROL_DMIC_X2_B_SEL_MASK \
                << STREAM_ANC_CONTROL_ACCESS_SELECT_ENABLES_SHIFT);

            if(value == 1)
            {
               ff_dmic_x2_mask |= STREAM_ANC_CONTROL_DMIC_X2_B_SEL_MASK;
            }

            break;
        case STREAM_ANC_PATH_SM_LPF_ID:
        default:
            return FALSE;
    }
    return stream_configure_connected_to_endpoint(ep,
                            STREAM_CONFIG_KEY_STREAM_ANC_CONTROL,
                            ff_dmic_x2_mask);
}

unsigned stream_anc_get_filters_coeff_number(STREAM_ANC_PATH path)
{
    patch_fn_shared(stream_anc);

    switch (path)
    {
        case STREAM_ANC_PATH_FFA_ID:
            return STREAM_ANC_IIR_FILTER_FFA_NUM_COEFFS;
        case STREAM_ANC_PATH_FFB_ID:
            return STREAM_ANC_IIR_FILTER_FFB_NUM_COEFFS;
        case STREAM_ANC_PATH_SM_LPF_ID:
            return STREAM_ANC_IIR_FILTER_FB_NUM_COEFFS;
        default:
            return 0;
    }
}

/**
 * \brief Configures ANC HW for given instance and path
 *
 * \param instance_id ANC Instance to configure
 *
 * \param path_id ANC filter path to configure
 *
 * \param key Stream key of ANC parameter
 *
 * \param value Value to configure
 *
 * \return Returns TRUE if success.
 */
bool stream_anc_config(STREAM_ANC_INSTANCE instance_id,
                       STREAM_ANC_PATH path_id,
                       STREAM_CONFIG_KEY key,
                       uint32 value)
{
    bool result = FALSE;

    patch_fn_shared(stream_anc);

    switch(key)
    {
        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_ENABLE:
        {
            uint16 val = (uint16)value;
            result = audio_hwm_anc_source_config_filter_enable(instance_id, path_id, val);
            break;
        }

        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_SHIFT:
        {
            uint16 val = (uint16)value;
            result = audio_hwm_anc_source_config_filter_shift(instance_id, path_id, val);
            break;
        }

        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN:
        case STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN:
        {
            uint16 val = (uint16)value;
            result = audio_hwm_anc_source_config_background_gain(instance_id, path_id, val);
            break;
        }

        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN_SHIFT:
        {
            uint16 val = (uint16)value;
            result = audio_hwm_anc_source_config_gain_shift(instance_id, path_id, val);
            break;
        }

        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_ADAPT_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_ADAPT_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_FB_ADAPT_ENABLE:
        {
            uint16 val = (uint16)value;
            result = audio_hwm_anc_source_config_adapt_enable(instance_id, path_id, val);
            break;
        }

#ifdef INSTALL_ANC_V2P0
        case STREAM_CONFIG_KEY_STREAM_ANC_RX_MIX_FFA_GAIN:
        {
            uint16 val = (uint16)value;
            result = audio_hwm_anc_source_config_rx_mix_ffa_background_gain(instance_id, val);
            break;
        }
        case STREAM_CONFIG_KEY_STREAM_ANC_RX_MIX_FFB_GAIN:
        {
            uint16 val = (uint16)value;
            result = audio_hwm_anc_source_config_rx_mix_ffb_background_gain(instance_id, val);
            break;
        }

        case STREAM_CONFIG_KEY_STREAM_ANC_RX_MIX_FFA_SHIFT:
        {
            uint16 val = (uint16)value;
            result = audio_hwm_anc_source_config_rx_mix_ffa_gain_shift(instance_id, val);
            break;
        }
        case STREAM_CONFIG_KEY_STREAM_ANC_RX_MIX_FFB_SHIFT:
        {
            uint16 val = (uint16)value;
            result = audio_hwm_anc_source_config_rx_mix_ffb_gain_shift(instance_id, val);
            break;
        }
#endif
        default:
        {
            STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                "Invalid key, key=0x%x, value=0x%x",
                key, value);

            /* Error invalid key */
            return FALSE;
        }
    }
     return result;
}

/**
 * \brief Perform ANC configuration via stream source configure.
 *
 * \param endpoint Pointer to the endpoint.
 * \param key      Key identifying the entity to configure.
 * \param value    Value to configure.
 *
 * \return TRUE if successful, FALSE if error.
 */
bool stream_anc_source_configure(ENDPOINT *endpoint,
                                 STREAM_CONFIG_KEY key,
                                 uint32 value)
{
    STREAM_ANC_INSTANCE instance_id;
    STREAM_ANC_PATH path_id;
    uint32 anc_instance_mask = 0;
    bool result;
    patch_fn_shared(stream_anc);

    STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
        "Configuring ANC, key=0x%x, value=0x%x",
        key, value);

    /* Check the endpoint pointer */
    if (endpoint == NULL)
    {
        STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_source_configure(): "
            "Endpoint pointer is NULL");

        return FALSE;
    }

    /* ID of instance being configured (or set) */
    instance_id = stream_audio_anc_get_primary_instance_id(endpoint);

    /* Check that this a valid instance (unless the instance is being set) */
    if ((audio_hwm_anc_is_anc_instance_valid(instance_id) == FALSE) &&
       (key != STREAM_CONFIG_KEY_STREAM_ANC_INSTANCE))
    {
        STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
            "Endpoint has an unset/invalid ANC instance ID, key=0x%x, instance_id=0x%x",
            key, instance_id);

        return FALSE;
    }

    path_id = key_to_id(key);

    if(key == STREAM_CONFIG_KEY_STREAM_ANC_INSTANCE)
    {
        /* Set the ANC instance associated with the endpoint */

        STREAM_ANC_INSTANCE stream_anc_instance_id = (STREAM_ANC_INSTANCE)value;

        if (audio_hwm_anc_is_anc_instance_valid_or_none(stream_anc_instance_id) == FALSE)
        {
            STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                "Invalid ANC instance ID, key=0x%x, value=0x%x",
                key, value);

            return FALSE;
        }

        /* Disable ANC endpoints */
        if (audio_hwm_anc_source_interface_disable(stream_anc_instance_id) == FALSE)
        {
            STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_source_configure(): "
                "failed to disable ANC endpoints");

            return FALSE;
        }

#ifdef INSTALL_ANC_STICKY_ENDPOINTS
        /* Close the endpoint if a closure by the ANC is pending */
        if (stream_audio_anc_get_close_pending(endpoint))
        {
            STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_source_configure(): "
                "closing source endpoint released by ANC");

            /* Close the endpoint now ANC has finished with it */
            if (!stream_close_endpoint(endpoint))
            {
                STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_source_configure(): "
                    "failed to close source endpoint released by ANC");
                return FALSE;
            }

            STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_source_configure(): "
                "source endpoint released by ANC is now closed");

            /* No longer waiting for ANC to close the endpoint */
            stream_audio_anc_set_close_pending(endpoint, FALSE);
        }
#endif /* INSTALL_ANC_STICKY_ENDPOINTS */

        if(stream_anc_instance_id == STREAM_ANC_INSTANCE_NONE_ID)
        {
            /* Disassociate both primary and secondary ANC instances with the endpoint */
            stream_audio_anc_set_primary_instance_id(endpoint, stream_anc_instance_id);
            stream_audio_anc_set_secondary_instance_id(endpoint, stream_anc_instance_id);
        }else
        {
            /* Check if primary instance is associated with EP */
            if(stream_audio_anc_get_primary_instance_id(endpoint) != STREAM_ANC_INSTANCE_NONE_ID)
            {
                /* We are here because primary ANC instance is already associated with the EP. 
                   If we have a new ANC instance (different from primary) configure it as secondary ANC instance */
                if(stream_audio_anc_get_primary_instance_id(endpoint) != stream_anc_instance_id)
                {
                    /* We have a new ANC instance, configure it as secondary */
                    stream_audio_anc_set_secondary_instance_id(endpoint, stream_anc_instance_id);
                }
            }else
            {
                /* We are here as EP is not associated with primary ANC instance (and also secondary). 
                   Associate this ANC instance with EP as primary ANC instance */
                stream_audio_anc_set_primary_instance_id(endpoint, stream_anc_instance_id);
            }
        }

        return TRUE;
    }

    /* Set ANC misc. controls */
    if(key == STREAM_CONFIG_KEY_STREAM_ANC_CONTROL)
    {
        result = audio_hwm_anc_source_set_anc_control(instance_id, value);
        if (!result)
        {
            STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                "ANC CONTROL config FAIL, key=0x%x, value=0x%x",
                key, value);
        }

        return result;
    }

#ifdef INSTALL_ANC_V2P0
    /* Set ANC misc. controls_1 */
    if(key == STREAM_CONFIG_KEY_STREAM_ANC_CONTROL_1)
    {
        result = audio_hwm_anc_source_set_anc_control_1(instance_id, value);
        if (!result)
        {
            STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                "ANC CONTROL_1 config FAIL, key=0x%x, value=0x%x",
                key, value);
        }

        return result;
    }
#endif

    anc_instance_mask = VALUE_TO_ANC_INSTANCE_MASK(value);
    /* Remove ANC mask from value */
    value = value & ANC_PARAMETER_VALUE_MASK;

    if(!anc_instance_mask)
    {
        /* We did not receive any instance data in value. Generate a mask using ANC instance (primary) associated with the endpoint */
        anc_instance_mask = ANC_INSTANCE_ID_TO_MASK(instance_id);
    }

    /* Initialize the instance_id to ANC0 and loop through the mask. */
    instance_id = STREAM_ANC_INSTANCE_ANC0_ID; 
    while(anc_instance_mask)
    {
        if(anc_instance_mask & INSTANCE_MASK_LSB)
        {
            /* Configure ANC instance according to the key */
            if (key == STREAM_CONFIG_KEY_STREAM_ANC_INPUT)
            {
                /* Set the ANC input path associated with the endpoint */

                STREAM_ANC_PATH stream_anc_path_id;
                STREAM_DEVICE hardware_type;
                audio_instance device_instance;
                audio_channel channel_number;

                /* Get the device type */
                hardware_type = (STREAM_DEVICE) stream_get_device_type(endpoint);

                /* Get the device instance */
                device_instance = (audio_instance) get_hardware_instance(endpoint);

                /* Get the channel number (note: zeroth channel is channel number 1) */
                channel_number = (audio_channel) (get_hardware_channel(endpoint) + 1);

                /* Check the input endpoint type */
                switch (hardware_type)
                {
                    case STREAM_DEVICE_CODEC:
                    case STREAM_DEVICE_DIGITAL_MIC:
                        break;

                    default:
                        /* Can only support AMIC and DMIC ANC inputs */
                        STREAM_ANC_L5_DBG_MSG3("stream_anc: stream_anc_source_configure(): "
                            "Invalid ANC input endpoint type, key=0x%x, value=0x%x, hardware_type=0x%x",
                            key, value, hardware_type);

                        return FALSE;
                }

                STREAM_ANC_L5_DBG_MSG3("stream_anc: stream_anc_source_configure(): "
                    "STREAM_ANC_INPUT, hardware_type=0x%x, device_instance=0x%x, channel_number=0x%x",
                    hardware_type, device_instance, channel_number);

                /* Associate or disassociate the endpoint and the ANC input path */
                stream_anc_path_id = (STREAM_ANC_PATH)value;
                if(stream_anc_path_id == STREAM_ANC_PATH_NONE_ID)
                {
                    /* Disassociate both primary and secondary ANC paths with the endpoint */
                    stream_audio_anc_set_primary_input_path_id(endpoint, stream_anc_path_id);
                    stream_audio_anc_set_secondary_input_path_id(endpoint, stream_anc_path_id);
                }else
                {
                    /* Check if primary path is associated with EP */
                    if(stream_audio_anc_get_primary_input_path_id(endpoint) != STREAM_ANC_PATH_NONE_ID)
                    {
                        /* We are here because primary ANC path is already associated with the EP. 
                           If we have a new ANC path (different from primary) configure it as secondary ANC path */
                        if(stream_audio_anc_get_primary_input_path_id(endpoint) != stream_anc_path_id)
                        {
                            /* We have a new ANC path, configure it as secondary */
                            stream_audio_anc_set_secondary_input_path_id(endpoint, stream_anc_path_id);
                        }
                    }else
                    {
                        /* We are here as EP is not associated with primary ANC path (and also secondary). 
                           Associate this ANC path with EP as primary ANC path */
                        stream_audio_anc_set_primary_input_path_id(endpoint, stream_anc_path_id);
                    }
                }

                result = audio_hwm_anc_interface_update(instance_id,
                                                        stream_anc_path_id,
                                                        hardware_type,
                                                        device_instance,
                                                        channel_number);
                if (result == FALSE)
                {
                    STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                        "Invalid ANC input path, key=0x%x, value=0x%x",
                        key, value);
                }
            }
           else
            {
                result = stream_anc_config(instance_id, path_id, key, value);
                
            }
            
            /* Return immediately if something went wrong*/
            if (!result)
            {
                STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_source_configure(): "
                            "ANC Config FAIL, key=0x%x, value=0x%x",
                            key, value);
                return FALSE;
            }
        }

        /* Update the mask and instance_id to next ANC instance */
        anc_instance_mask = anc_instance_mask >> 1;
        instance_id++;
    }

    return TRUE;
}
/**
 * \brief Perform ANC configuration via stream sink configure.
 *
 * \param endpoint Pointer to the endpoint.
 * \param key      Key identifying the entity to configure.
 * \param value    Value to configure.
 *
 * \return TRUE if successful, FALSE if error.
 */
bool stream_anc_sink_configure(ENDPOINT *endpoint,
                               STREAM_CONFIG_KEY key,
                               uint32 value)
{
    STREAM_ANC_INSTANCE instance_id;
    STREAM_ANC_INSTANCE stream_anc_instance_id;
    audio_instance device_instance;
    audio_channel channel_number;
    bool result;
    patch_fn_shared(stream_anc);

    STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_sink_configure(): Configuring ANC, key=0x%x, value=0x%x",
        key, value);

    /* Check the endpoint pointer */
    if (endpoint == NULL)
    {
        STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_sink_configure(): Endpoint pointer is NULL");

        return FALSE;
    }

    /* ID of instance being configured (or set) */
    instance_id = stream_audio_anc_get_primary_instance_id(endpoint);

    /* Check that this a valid instance (unless the instance is being set) */
    if ((audio_hwm_anc_is_anc_instance_valid(instance_id) == FALSE) &&
       (key != STREAM_CONFIG_KEY_STREAM_ANC_INSTANCE))
    {
        STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_sink_configure(): "
            "Endpoint has an unset/invalid ANC instance ID, key=0x%x, instance_id=0x%x",
            key, instance_id);

        return FALSE;
    }

    if (key != STREAM_CONFIG_KEY_STREAM_ANC_INSTANCE)
    {
        STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_sink_configure(): "
            "Invalid key, key=0x%x, value=0x%x",
            key, value);

        return FALSE;
    }

    /* Configure sink according to the key */

    /* Set the ANC instance associated with the endpoint */
    stream_anc_instance_id = (STREAM_ANC_INSTANCE)value;

    if (audio_hwm_anc_is_anc_instance_valid_or_none(stream_anc_instance_id) == FALSE)
    {
        STREAM_ANC_L5_DBG_MSG2("stream_anc: stream_anc_sink_configure(): "
            "Invalid ANC instance ID, key=0x%x, value=0x%x",
            key, value);

        return FALSE;
    }

    /* Get the device instance */
    device_instance = (audio_instance) get_hardware_instance(endpoint);

    /* Get the channel number (note: zeroth channel is channel number 1) */
    channel_number = (audio_channel) (get_hardware_channel(endpoint) + 1);

    result = audio_hwm_anc_sink_interface_update(stream_anc_instance_id,
                                                 device_instance,
                                                 channel_number);
    if (result == FALSE)
    {
        STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_sink_configure(): "
            "failed to disable ANC endpoints");

        return FALSE;
    }

#ifdef INSTALL_ANC_STICKY_ENDPOINTS
    /* Close the endpoint if a closure by the ANC is pending */
    if (stream_audio_anc_get_close_pending(endpoint))
    {
        STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_sink_configure(): "
            "closing sink endpoint released by ANC");

        /* Close the endpoint now ANC has finished with it */
        if (!stream_close_endpoint(endpoint))
        {
            STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_sink_configure(): "
                "failed to close sink endpoint released by ANC");

            return FALSE;
        }

        STREAM_ANC_L5_DBG_MSG("stream_anc: stream_anc_sink_configure(): "
            "sink endpoint released by ANC is now closed");

        /* No longer waiting for ANC to close the endpoint */
        stream_audio_anc_set_close_pending(endpoint, FALSE);
    }
#endif /* INSTALL_ANC_STICKY_ENDPOINTS */

    /* Associate/dissociate the sink endpoint with the ANC instance ID */
    stream_audio_anc_set_primary_instance_id(endpoint, stream_anc_instance_id);

    return TRUE;
}

/**
 * \brief Enables the Sigma-Delta Modulator on the feedback tuning output.
 *        TODO: This probably should be folded into anc_tuning_set_monitor.
 *
 * \param endpoint Pointer to the endpoint.
 *
 * \return TRUE if successful, FALSE if error.
 */
bool stream_anc_enable_sdm(ENDPOINT *endpoint)
{
    uint32 en_and_mask;
    patch_fn_shared(stream_anc);
    en_and_mask = (STREAM_ANC_CONTROL_FB_TUNE_DSM_EN_MASK <<
                   STREAM_ANC_CONTROL_ACCESS_SELECT_ENABLES_SHIFT);
    en_and_mask |= STREAM_ANC_CONTROL_FB_TUNE_DSM_EN_MASK;
    return stream_anc_configure_control(endpoint, en_and_mask);
}


/**
 * \brief Configure ANC miscellaneous hardware options
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param en_and_mask Combined bit field of enables and selection mask for ANC hardware control options.
 * \return TRUE if successful, FALSE if error
 */
bool stream_anc_set_anc_control(STREAM_ANC_INSTANCE anc_instance, uint32 en_and_mask)
{
    return audio_hwm_anc_source_set_anc_control(anc_instance, en_and_mask);
}

#ifdef INSTALL_ANC_V2P0
/**
 * \brief Configure ANC miscellaneous hardware options
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param en_and_mask Combined bit field of enables and selection mask for ANC hardware control options.
 * \return TRUE if successful, FALSE if error
 */
bool stream_anc_set_anc_control_1(STREAM_ANC_INSTANCE anc_instance, uint32 en_and_mask)
{
    return audio_hwm_anc_source_set_anc_control_1(anc_instance, en_and_mask);
}
#endif /* INSTALL_ANC_V2P0 */

/**
 * \brief Connect Feedback monitor to an IIR filter input.
 *
 * \param endpoint Pointer to the endpoint.
 * \param source   Which IIR filter input to connect.
 *
 * \return TRUE if successful, FALSE if error.
 */
bool stream_anc_connect_feedback_monitor(ENDPOINT *endpoint,
                                         ANC_FBMON_SRC source)
{
    uint32 bit_enable;
    uint32 bit_select;
    uint32 en_and_mask;
    patch_fn_shared(stream_anc);

    bit_enable = (source << STREAM_ANC_CONTROL_FBMON_SEL_POSITION) & STREAM_ANC_CONTROL_FBMON_SEL_MASK;
    bit_select = STREAM_ANC_CONTROL_FBMON_SEL_MASK;
    en_and_mask = (bit_select << STREAM_ANC_CONTROL_ACCESS_SELECT_ENABLES_SHIFT) | bit_enable;

    return stream_anc_configure_control(endpoint, en_and_mask);
}

/**
 * \brief Derive an ANC filter ID from the source configure key.
 *
 * \param key ANC instance ID (e.g. ANC0, ANC1).
 *
 * \return path_id Path ID that is derived from the key.
 */
static STREAM_ANC_PATH key_to_id(STREAM_CONFIG_KEY key)
{
    STREAM_ANC_PATH path_id;
    patch_fn_shared(stream_anc);

    /* Act according to the key */
    switch (key)
    {
        /* Configure the ANC DC filter/SM LPF */
        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_DC_FILTER_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_GAIN_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_ADAPT_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_IIR_COEFFS:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFA_LPF_COEFFS:
            path_id = STREAM_ANC_PATH_FFA_ID;
            break;

        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_DC_FILTER_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_GAIN_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_ADAPT_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_IIR_COEFFS:
        case STREAM_CONFIG_KEY_STREAM_ANC_FFB_LPF_COEFFS:
            path_id = STREAM_ANC_PATH_FFB_ID;
            break;

        case STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN:
        case STREAM_CONFIG_KEY_STREAM_ANC_FB_GAIN_SHIFT:
        case STREAM_CONFIG_KEY_STREAM_ANC_FB_ADAPT_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_FB_IIR_COEFFS:
        case STREAM_CONFIG_KEY_STREAM_ANC_FB_LPF_COEFFS:
            path_id = STREAM_ANC_PATH_FB_ID;
            break;

        case STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_ENABLE:
        case STREAM_CONFIG_KEY_STREAM_ANC_SM_LPF_FILTER_SHIFT:
            path_id = STREAM_ANC_PATH_SM_LPF_ID;
            break;

        default:
            path_id = STREAM_ANC_PATH_NONE_ID;
            break;
    }

    return path_id;
}

#ifdef INSTALL_ANC_V2P0
/**
 * \brief Unpack 32-bit ANC IIR coeffs from packed 16-bit array.
 *
 * \param coeffs16 Pointer to packed array of coeffs (Input)
 *
 * \param coeffs32 Pointer to un-packed array of 32-bit coeffs (Ouput)
 *
 * \param num_coeffs Number of IIR coeffs
 *
 * \Note The array coeffs16 is expected have packed data of 32-bit coeffs with LSW first, followed by MSW for each coeff.
 */
void stream_anc_unpack_32bit_iir_coeffs(const uint16 * coeffs16, uint32 * coeffs32, unsigned num_coeffs)
{
    uint32 coeff32_lsw;
    uint32 coeff32_msw;
    uint16 idx;

    patch_fn_shared(stream_anc);

    for(idx = 0; idx < num_coeffs; idx++)
    {
        /* coeffs16[] has 32-bit coeffs packed as 16-bit LSW followed by 16-bit MSW */
        coeff32_lsw = (uint32)coeffs16[2*idx];
        coeff32_msw = ((uint32)coeffs16[2*idx + 1]) << STREAM_ANC_IIR_FILTER_COEFF_MSW_SHIFT;
        coeffs32[idx] = coeff32_msw | coeff32_lsw;
    }
}

/**
 * \brief Sets all ANC HW configuration for given instance in anc_config_list
 *
 * \param instance ANC instance to configure.
 *
 * \param total_cmds Number of ANC stream keys to configure
 *
 * \param anc_config_list Pointer to list of ANC keys and parameters
 *
 * \param status Status of command execution.
 *
 * \return If success, returns number of ANC parameters configured. Else, returns failed ANC stream key.
 *
 */
uint16 stream_anc_set_param(STREAM_ANC_INSTANCE instance,
                             unsigned total_cmds,
                             ANC_STREAM_CONFIG *anc_config_list,
                             STATUS_KYMERA* status
                             )
{
    unsigned index =0;
    bool result = FALSE;
    STREAM_ANC_PATH path_id;
    unsigned data;

    patch_fn_shared(stream_anc);

    for(index = 0; index < total_cmds; index++)
    {
        L2_DBG_MSG2("ANC config key = 0x%x, value_lsw = %d ",anc_config_list[index].key,*(anc_config_list[index].data));
        path_id = key_to_id(anc_config_list[index].key);
        switch(anc_config_list[index].key)
        {
            case STREAM_CONFIG_KEY_STREAM_ANC_FFA_IIR_COEFFS:
            case STREAM_CONFIG_KEY_STREAM_ANC_FFB_IIR_COEFFS:
            case STREAM_CONFIG_KEY_STREAM_ANC_FB_IIR_COEFFS:
            {

                HWM_ANC_IIR_COEFF_TYPE coeffs32[STREAM_ANC_IIR_FILTER_FFA_NUM_COEFFS];

                stream_anc_unpack_32bit_iir_coeffs(anc_config_list[index].data, coeffs32, STREAM_ANC_IIR_FILTER_FFA_NUM_COEFFS);
                result = audio_hwm_anc_set_anc_iir_background_filter(instance,
                                                                      path_id,
                                                                      (uint16) STREAM_ANC_IIR_FILTER_FFA_NUM_COEFFS,
                                                                      (const HWM_ANC_IIR_COEFF_TYPE *)coeffs32);
                break;
            }
            case STREAM_CONFIG_KEY_STREAM_ANC_FFA_LPF_COEFFS:
            case STREAM_CONFIG_KEY_STREAM_ANC_FFB_LPF_COEFFS:
            case STREAM_CONFIG_KEY_STREAM_ANC_FB_LPF_COEFFS:
            {
                uint16 shift1;
                uint16 shift2;

                shift1 = anc_config_list[index].data[0];
                shift2 = anc_config_list[index].data[1];

                result = stream_anc_set_anc_lpf_coeffs(instance, path_id, shift1, shift2);
                break;
            }
            case STREAM_CONFIG_KEY_STREAM_ANC_CONTROL:
            {
                data = ((uint32)anc_config_list[index].data[0] |
                       ((uint32)anc_config_list[index].data[1] << 16));
                result = audio_hwm_anc_source_set_anc_control(instance, data);
                break;
            }
            case STREAM_CONFIG_KEY_STREAM_ANC_CONTROL_1:
            {
                data = ((uint32)anc_config_list[index].data[0] |
                       (((uint32)anc_config_list[index].data[1]) << 16));
                result = audio_hwm_anc_source_set_anc_control_1(instance, data);
                break;
            }
            default:
            {
                data = (uint32)anc_config_list[index].data[0];
                result = stream_anc_config(instance, path_id, anc_config_list[index].key, data);
            }

        }
        if(result == FALSE)
        {
            L2_DBG_MSG3("Failed to configure 0x%x, instance = %d, path_if = %d ",anc_config_list[index].key, instance, path_id);
            *status = STATUS_CMD_FAILED;
            return anc_config_list[index].key;
        }
    }
    *status = STATUS_OK;
    return (uint16)total_cmds;
}
#endif /* INSTALL_ANC_V2P0 */
