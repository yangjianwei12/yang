/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \ingroup endpoints
 * \file stream_for_anc.h
 *
 * Temporary header file to make it easier to restructure the relationship
 * between audio and stream as far as ANC is concerned.
 */
#ifndef STREAM_FOR_ANC_H
#define STREAM_FOR_ANC_H

/* For pointer to ENDPOINT */
#include "stream_common.h"
/* For STATUS_KYMERA */
#include "status_prim.h"
/* For STREAM_ANC_INSTANCE and STREAM_ANC_PATH. */
#include "stream_type_alias.h"
/* Dummy CONNECTION ID to use in stream_anc_enable function. */
#define DUMMY_CON_ID 0

#ifdef INSTALL_UNINTERRUPTABLE_ANC
/**
 * \brief stream_audio_anc_get_primary_instance_id function
 *
 * \param ep endpoint pointer to the endpoint
 *
 * \NOTE ANC HW instance that is first associated with the endpoint becomes 
 *       primary ANC instance of that Endpoint
 */
extern STREAM_ANC_INSTANCE stream_audio_anc_get_primary_instance_id(ENDPOINT *ep);

/**
 * \brief stream_audio_anc_get_secondary_instance_id function
 *
 * \param ep endpoint pointer to the endpoint
 * \NOTE ANC HW instance becomes secondary ANC instance for an endpoint if it is the second ANC instance 
 *       getting associated with the endpoint.
 */
extern STREAM_ANC_INSTANCE stream_audio_anc_get_secondary_instance_id(ENDPOINT *ep);

/**
 * \brief stream_audio_anc_set_primary_instance_id function
 *
 * \param ep                Pointer to the endpoint
 * \param instance_id       ANC Instance ID to associate with the endpoint
 *
 * \NOTE ANC HW instance that is first associated with the endpoint becomes is used to 
 *       configure primary ANC instance of that Endpoint. Primary ANC instance (default ANC instance) of
 *       the endpoint ANC hardware when ANC instance mask is not provided from the application.
 *       Any ANC instance can be configured as primary/secondary
 */
extern void stream_audio_anc_set_primary_instance_id(ENDPOINT *ep,
                                             STREAM_ANC_INSTANCE instance_id);

/**
 * \brief stream_audio_anc_set_secondary_instance_id function
 *
 * \param ep                Pointer to the endpoint
 * \param instance_id       ANC Instance ID to associate with the endpoint
 */
extern void stream_audio_anc_set_secondary_instance_id(ENDPOINT *ep,
                                             STREAM_ANC_INSTANCE instance_id);

/**
 * \brief stream_audio_anc_get_primary_input_path_id function
 *
 * \param ep Pointer to the endpoint
 *
 * \NOTE ANC input path that is first associated with the endpoint becomes 
 *       primary input path of that Endpoint
 */
extern STREAM_ANC_PATH stream_audio_anc_get_primary_input_path_id(ENDPOINT *ep);

/**
 * \brief stream_audio_anc_get_secondary_input_path_id function
 *
 * \param ep Pointer to the endpoint
 *
 * \NOTE ANC input path becomes secondary input path for an endpoint if it is the second ANC input path 
 *       getting associated with the endpoint.
 */
extern STREAM_ANC_PATH stream_audio_anc_get_secondary_input_path_id(ENDPOINT *ep);


/**
 * \brief stream_audio_anc_set_primary_input_path_id function
 *
 * \param ep       Pointer to the endpoint
 * \param path_id  ANC Input ID to associate with the endpoint
 *
 * \NOTE ANC input path that is first associated with the endpoint becomes 
 *       primary input path of that Endpoint
 *       any ANC input path can be configured as primary/secondary
 */
extern void stream_audio_anc_set_primary_input_path_id(ENDPOINT *ep,
                                               STREAM_ANC_PATH path_id);

/**
 * \brief stream_audio_anc_set_secondary_input_path_id function
 *
 * \param ep       Pointer to the endpoint
 * \param path_id  Secondary ANC Input ID to associate with the endpoint
 *
 * \NOTE ANC input path becomes secondary input path for an endpoint if it is the second ANC input path 
 *       getting associated with the endpoint.
 */
extern void stream_audio_anc_set_secondary_input_path_id(ENDPOINT *ep,
                                               STREAM_ANC_PATH path_id);


#ifdef INSTALL_ANC_STICKY_ENDPOINTS
/**
 * \brief stream_audio_anc_get_close_pending function
 *
 * \param ep Pointer to the endpoint
 */
extern bool stream_audio_anc_get_close_pending(ENDPOINT *ep);

/**
 * \brief stream_audio_anc_set_close_pending function
 *
 * \param ep              Pointer to the endpoint
 * \param close_pending   To close the endpoint
 */
extern void stream_audio_anc_set_close_pending(ENDPOINT *ep,
                                               bool close_pending);
#endif /* INSTALL_ANC_STICKY_ENDPOINTS */

#ifdef INSTALL_AUDIO_MODULE
/**
 * \brief Perform ANC configuration via stream source configure.
 *
 * \param endpoint Pointer to the endpoint.
 * \param key      Key identifying the entity to configure.
 * \param value    Value to configure.
 *
 * \return TRUE if successful, FALSE if error.
 */
extern bool stream_anc_source_configure(ENDPOINT *endpoint,
                                        STREAM_CONFIG_KEY key,
                                        uint32 value);

/**
 * \brief Perform ANC configuration via stream sink configure.
 *
 * \param endpoint Pointer to the endpoint.
 * \param key      Key identifying the entity to configure.
 * \param value    Value to configure.
 *
 * \return TRUE if successful, FALSE if error.
 */
extern bool stream_anc_sink_configure(ENDPOINT *endpoint,
                                      STREAM_CONFIG_KEY key,
                                      uint32 value);
#endif /* INSTALL_AUDIO_MODULE */

#endif /* INSTALL_UNINTERRUPTABLE_ANC */

/**
 * \brief
 *
 * \param ep    Pointer to endpoint to configure
 * \param path
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_input(ENDPOINT *ep, STREAM_ANC_PATH path);

/**
 * \brief
 *
 * \param ep       Pointer to endpoint to configure
 * \param instance
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_instance(ENDPOINT *ep,
                                          STREAM_ANC_INSTANCE instance);

/**
 * \brief
 *
 * \param ep       Pointer to endpoint to configure
 * \param bitfield
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_control(ENDPOINT *ep, uint32 bitfield);

/**
 * \brief
 *
 * \param ep    Pointer to endpoint to configure
 * \param path
 * \param value
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_gain(ENDPOINT *ep, STREAM_ANC_PATH path,
                                      unsigned value);

/**
 * \brief
 *
 * \param ep    Pointer to endpoint to configure
 * \param path
 * \param value
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_gain_shift(ENDPOINT *ep,
                                            STREAM_ANC_PATH path,
                                            unsigned value);

/**
 * \brief
 *
 * \param ep    Pointer to endpoint to configure
 * \param path
 * \param value
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_dc_filter_enable(ENDPOINT *ep,
                                                  STREAM_ANC_PATH path,
                                                  unsigned value);

/**
 * \brief
 *
 * \param ep    Pointer to endpoint to configure
 * \param path
 * \param value
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_dc_filter_shift(ENDPOINT *ep,
                                                 STREAM_ANC_PATH path,
                                                 unsigned value);

/**
 * \brief
 *
 * \param ep    Pointer to endpoint to configure
 * \param path
 * \param value
 *
 * \return TRUE if successful
 */
extern bool stream_anc_configure_dmic_x2_enable(ENDPOINT *ep,
                                                STREAM_ANC_PATH path,
                                                unsigned value);
/**
 * \brief Returns the number of coefficient of the filter
 *        in a specified path of an ANC block.
 *
 * \param path  path to query
 *
 * \return number coefficients
 */
extern unsigned stream_anc_get_filters_coeff_number(STREAM_ANC_PATH path);

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
extern bool stream_anc_config(STREAM_ANC_INSTANCE instance_id,
                       STREAM_ANC_PATH path_id,
                       STREAM_CONFIG_KEY key,
                       uint32 value);


/**
 * \brief Configure an ANC IIR filter (sets foreground and background coefficients).
 *
 * \param instance     ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id      ANC input path ID (e.g. FFA, FFB, FB).
 * \param num_coeffs   Number of coefficients.
 * \param coeffs       Pointer to an array of IIR coefficients.
 *
 * \return TRUE if successful.
 */
extern bool stream_anc_set_anc_iir_coeffs(STREAM_ANC_INSTANCE instance,
                                          STREAM_ANC_PATH path_id,
                                          unsigned num_coeffs,
                                          const STREAM_ANC_IIR_COEFF_TYPE *coeffs);

/**
 * \brief Configure an ANC IIR filter (sets the foreground coefficients).
 *
 * \param instance     ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id      ANC input path ID (e.g. FFA, FFB, FB).
 * \param num_coeffs   Number of coefficients.
 * \param coeffs       Pointer to an array of IIR coefficients.
 *
 * \return TRUE if successful.
 */
extern bool stream_anc_set_anc_iir_foreground_coeffs(STREAM_ANC_INSTANCE instance,
                                          STREAM_ANC_PATH path_id,
                                          unsigned num_coeffs,
                                          const STREAM_ANC_IIR_COEFF_TYPE *coeffs);


/**
 * \brief Get ANC IIR filter (gets the coefficients).
 *
 * \param instance     ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id      ANC input path ID (e.g. FFA, FFB, FB).
 * \param num_coeffs   Number of coefficients.
 * \param coeffs       Pointer to an array of IIR coefficients.
 *
 * \return TRUE if successful.
 */
extern bool stream_anc_get_anc_iir_coeffs(STREAM_ANC_INSTANCE instance,
                                          STREAM_ANC_PATH path_id,
                                          unsigned num_coeffs,
                                          STREAM_ANC_IIR_COEFF_TYPE *coeffs);

/**
 * \brief update coefficients from foreground to background.
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 */

extern void stream_anc_update_background_iir_coeffs(STREAM_ANC_INSTANCE_MASK anc_instance_mask);

/**
 * \brief Configure ANC activee iir coefficients.
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param coeff_set    Whether banks are being used as foreground or background
 */

extern void stream_anc_select_active_iir_coeffs(STREAM_ANC_INSTANCE_MASK anc_instance_mask,
                                                STREAM_ANC_BANK coeff_set);

/**
 * \brief Configure ANC tuning options
 *
 * \param instance ANC instance ID (e.g. ANC0, ANC1).
 * \param chain    Which chain must be tuned.
 */
extern void stream_anc_set_anc_tune(STREAM_ANC_INSTANCE instance,
                                    unsigned chain);

/**
 * \brief Wrapper to enable/disable ANC with license check from anc tuning capability.
 *
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
extern void stream_anc_enable(STREAM_ANC_INSTANCE stream_anc_instance_number,
                              uint16 *anc_enable,
                              bool (*resp_callback)(void));

extern bool stream_anc_enable_pending_status(void);
/**
 * \brief Configure an ANC LPF filter (sets the LPF coefficients)
 *
 * \param instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id      ANC input path ID (e.g. FFA, FFB, FB).
 * \param shift1       Coefficient 1 expressed as a shift.
 * \param shift2       Coefficient 2 expressed as a shift.
 *
 * \return TRUE if successful
 */
extern bool stream_anc_set_anc_lpf_coeffs(STREAM_ANC_INSTANCE instance,
                                          STREAM_ANC_PATH path_id,
                                          uint16 shift1,
                                          uint16 shift2);

/**
 * \brief Copy the foreground gains to the background gains
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1, ANC01).
 *
 * \note Gains for the FFA, FFB, and FB LPF are shadowed
 *       (but gain shifts are not)
 */
extern void stream_anc_update_background_gains(
    STREAM_ANC_INSTANCE_MASK anc_instance_mask);

/**
 * \brief Retrieve the ANC enable/disable state
 *
 * \param anc_enable_0_ptr Pointer to result (bitfield controlling
 *                         instance ANC0 signal paths).
 * \param anc_enable_1_ptr Pointer to result (bitfield controlling
 *                         instance ANC1 signal paths).
 *
 */
extern void stream_get_anc_enable(uint16 *anc_enable_0_ptr,
                                  uint16 *anc_enable_1_ptr);

#if defined(INSTALL_ANC_CLIP_THRESHOLD)

/**
 * \brief Configure the clipping/threshold detection ANC output level
 *
 * \param anc_instance ANC instance
 * \param level        Threshold level to configure
 *
 * \return none
 */
extern void stream_anc_set_clip_level(STREAM_ANC_INSTANCE anc_instance,
                                      uint32 level);

/**
 * \brief Enable/disable the ANC output threshold detector
 *
 * \param anc_instance ANC instance
 * \param callback NULL:     disable ANC threshold detection
 *                 non-NUll: pointer to function to be called
 *                           on exceeding ANC detection threshold
 *
 * \return none
 */
extern void stream_anc_detect_enable(STREAM_ANC_INSTANCE anc_instance,
                                     void (*callback)(void));

#endif /* defined(INSTALL_ANC_CLIP_THRESHOLD) */

/**
 * \brief Configure the ANC path gains. (background gains)
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (e.g. FFA, FFB, FB).
 * \param gain Gain applied to filter (8 bit linear gain value).
 *
 */
extern bool stream_anc_set_anc_fine_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 gain);

/**
 * \brief Configure the ANC path gains. (foreground gains)
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (e.g. FFA, FFB, FB).
 * \param gain Gain applied to filter (8 bit linear gain value).
 *
 */
extern bool stream_anc_set_anc_foreground_fine_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 gain);

/**
 * \brief Configure the ANC path gain shift (gain exponent).
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (e.g. FFA, FFB, FB).
 * \param shift Gain shift applied to filter (4 bit shift value).
 *
 */
extern bool stream_anc_set_anc_coarse_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 shift);

#ifdef INSTALL_ANC_V2P0
/**
 * \brief Configure the ANC FFa/b path to the Rx mixer gain.
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (FFA, FFB).
 * \param gain Gain applied to filter (8 bit linear gain value).
 *
 */
extern bool stream_anc_set_anc_rx_mix_foreground_fine_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 gain);

/**
 * \brief Gain Shift Value for ANC FF path A/B to the Rx Mixer.
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (FFA, FFB).
 * \param shift Gain shift applied to filter (4 bit shift value).
 *
 */
extern bool stream_anc_set_anc_rx_mix_coarse_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 shift);
#endif /* INSTALL_ANC_V2P0 */

/**
 * \brief Get ANC path gains.
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (e.g. FFA, FFB, FB).
 * \param gain pointer to Gain
 *
 */
extern bool stream_anc_get_anc_fine_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 *gain);

/**
 * \brief Get ANC path gain shift (gain exponent).
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param path_id ANC input path ID (e.g. FFA, FFB, FB).
 * \param shift pointer to Gain shift applied to filter (4 bit shift value).
 *
 */
extern bool stream_anc_get_anc_coarse_gain(STREAM_ANC_INSTANCE anc_instance, STREAM_ANC_PATH path_id, uint16 *shift);

/**
 * \brief Place-holder function for patching purposes.
 *
 * \param ptr Pointer to function parameters (to be used as required)
 *
 */
extern void stream_anc_user1(void *ptr);

/**
 * \brief Place-holder function for patching purposes.
 *
 * \param ptr Pointer to function parameters (to be used as required)
 *
 */
extern void stream_anc_user2(void *ptr);

/**
 * \brief Enables the Sigma-Delta Modulator on the feedback tuning output.
 *
 * \param endpoint     Pointer to the endpoint.
 *
 * \return TRUE if successful, FALSE if error.
 */
extern bool stream_anc_enable_sdm(ENDPOINT *endpoint);

/** Indicates which filter input to connect to the feedback monitor. */
typedef enum
{
    ANC_FBMON_FFA     = 0,  /*!< Feed Forward Filter (typical outside microphone) */
    ANC_FBMON_FB      = 1,  /*!< Feedback Filter */
#ifdef INSTALL_ANC_V2P0
    ANC_FBMON_ANC_OUT = 2,  /*!< ANC Output */
#endif
} ANC_FBMON_SRC;

#ifdef INSTALL_ANC_V2P0
typedef struct{
    STREAM_CONFIG_KEY key;
    uint16* data;
}ANC_STREAM_CONFIG;

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
extern uint16 stream_anc_set_param(STREAM_ANC_INSTANCE instance,
                             unsigned count,
                             ANC_STREAM_CONFIG *anc_config_list,
                             STATUS_KYMERA* status
                             );
#endif /* INSTALL_ANC_V2P0 */

/**
 * \brief Connect Feedback monitor to an IIR filter input.
 *
 * \param endpoint Pointer to the endpoint.
 * \param source   Which IIR filter input to connect.
 *
 * \return TRUE if successful, FALSE if error.
 */
extern bool stream_anc_connect_feedback_monitor(ENDPOINT *endpoint,
                                                ANC_FBMON_SRC source);

/**
 * \brief Configure ANC miscellaneous hardware options
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param en_and_mask Combined bit field of enables and selection mask for ANC hardware control options.
 * \return TRUE if successful, FALSE if error
 */
extern bool stream_anc_set_anc_control(STREAM_ANC_INSTANCE anc_instance, uint32 en_and_mask);

#ifdef INSTALL_ANC_V2P0
/**
 * \brief Configure ANC miscellaneous hardware options
 *
 * \param anc_instance ANC instance ID (e.g. ANC0, ANC1).
 * \param en_and_mask Combined bit field of enables and selection mask for ANC hardware control options.
 * \return TRUE if successful, FALSE if error
 */
extern bool stream_anc_set_anc_control_1(STREAM_ANC_INSTANCE anc_instance, uint32 en_and_mask);
#endif

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
extern void stream_anc_unpack_32bit_iir_coeffs(const uint16 * coeffs16, uint32 * coeffs32, unsigned num_coeffs);


#endif /* STREAM_FOR_ANC_H */
