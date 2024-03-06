/****************************************************************************
Copyright (c) 2004 - 2021 Qualcomm Technologies International, Ltd.

FILE NAME
    hfp.h

DESCRIPTION
    Header file for the HFP profile library.
*/

/*!
\file
\defgroup hfp hfp
\ingroup vm_libs

\brief  Header file for the Hands Free Profile library.

\section hfp_intro INTRODUCTION
        This file documents the Hands Free Profile library API.
*/

/** @{ */

#ifndef HFP_H_
#define HFP_H_

#include <library.h>
#include <bdaddr_.h>
#include <connection.h>
#include <message.h>
#include <handover_if.h>

/*!  Hfp Supported Features Flag Defines

    These flags can be or'd together and used as the supported_features field
    of an hfp_init_params structure.
*/

/*!
    @brief Setting this flag indicates that this device implements noise
    reduction / echo cancellation.
*/
#define HFP_NREC_FUNCTION           (1)
/*!
    @brief Setting this flag indicates that this device supports three way call
    control.
*/
#define HFP_THREE_WAY_CALLING       (1<<1)
/*!
    @brief Setting this flag indicates that this device can display calling
    line identification information.
*/
#define HFP_CLI_PRESENTATION        (1<<2)
/*!
    @brief Setting this flag indicates that this device can enable voice
    recognition on the AG.
*/
#define HFP_VOICE_RECOGNITION       (1<<3)
/*!
    @brief Setting this flag indicates that the AG can control this device's
    local volume.
*/
#define HFP_REMOTE_VOL_CONTROL        (1<<4)
/*!
    @brief Setting this flag indicates that this device can request a list of
    current calls from the AG and also receive call status indicators from the AG.
*/
#define HFP_ENHANCED_CALL_STATUS    (1<<5)
/*!
    @brief Setting this flag indicates that this device can use the extended
    three-way calling features of the AG.
*/
#define HFP_ENHANCED_CALL_CONTROL   (1<<6)
/*!
    @brief Setting this flag indicates that this device supports codec
    negotiation.
*/
#define HFP_CODEC_NEGOTIATION       (1<<7)
/*!
   @brief Setting this flag indicates that this device supports HF indicators.
*/
#define HFP_HF_INDICATORS           (1<<8)
/*!
    @brief Setting this flag indicates that this device supports eSCO S4
    (and T2) Settings. If WBS is disabled this flag is also set if S4 is supported.
*/
#define HFP_ESCO_S4_SUPPORTED       (1<<9)


/*!
    @brief Bitmask of all features supported by the HFP library
*/
#define HFP_SUPPORTED_FEATURES (HFP_NREC_FUNCTION | HFP_THREE_WAY_CALLING | \
                                HFP_CLI_PRESENTATION | HFP_VOICE_RECOGNITION | \
                                HFP_REMOTE_VOL_CONTROL | HFP_ENHANCED_CALL_STATUS | \
                                HFP_CODEC_NEGOTIATION |HFP_HF_INDICATORS | HFP_ESCO_S4_SUPPORTED)

/*! The maximum length of arrays in messages to the application */
#define HFP_MAX_ARRAY_LEN          (32)

/*!
    @brief The supported profiles.
*/
typedef enum
{
    /*! The supported profile has not been set for this profile instance. */
    hfp_no_profile              = 0,
    /*! As defined in part K6 of the Bluetooth specification.*/
    hfp_headset_profile         = 1<<0,
    /*! As defined in version 1.5 of the handsfree profile specification.*/
    hfp_handsfree_profile       = 1<<1,
    /*! As defined in version 1.7 of the handsfree profile specification, S4 mandatory indicating HFP 1.7*/
    hfp_handsfree_107_profile = 1<<2,

    FORCE_ENUM_TO_MIN_16BIT(hfp)
} hfp_profile;

/** Bitmasks for all HSP/HFP **/
#define hfp_headset_all           (hfp_headset_profile)
#define hfp_handsfree_all         (hfp_handsfree_profile | hfp_handsfree_107_profile)
#define hfp_handsfree_and_headset (hfp_headset_all | hfp_handsfree_all)

/*!
    @brief WB-Speech codec bit masks.
*/
typedef enum
{
    /*! No Codec. */
    hfp_wbs_codec_mask_none         = 0x00,
    /*! CVSD Codec. CVSD support is mandatory. */
    hfp_wbs_codec_mask_cvsd         = 0x01,
    /*! SBC Codec. SBC support is mandatory. */
    hfp_wbs_codec_mask_msbc         = 0x02,

    FORCE_ENUM_TO_MIN_16BIT(hfp_wbs_codec_mask)
} hfp_wbs_codec_mask;

/*!
    @brief HF Indicators bit masks.
*/
typedef enum
{
    /*! No HF indicator. */
    hfp_indicator_mask_none           = 0x00,
    /*! Enhanced safety indicator. */
    hfp_enhanced_safety_mask          = 1 << 0,
    /*! Battery level indicator. */
    hfp_battery_level_mask            = 1 << 1,

    FORCE_ENUM_TO_MIN_16BIT(hfp_hf_indicators_mask)
} hfp_hf_indicators_mask;

/*!
    @brief Possible values indicators can take for optional_indicators
*/
typedef enum
{
    hfp_indicator_off    = 0,
    hfp_indicator_on     = 1,
    hfp_indicator_ignore = 2,

    FORCE_ENUM_TO_MIN_16BIT(hfp_indicator_status)
} hfp_indicator_status;

/*!
    @brief Possible values indicators can take for hf_indicators
*/
typedef enum
{
    hfp_hf_indicator_off    = 0,
    hfp_hf_indicator_on     = 1
} hfp_hf_indicator_state;


/*! @brief  Assigned numbers for HF indicators as per
  https://www.bluetooth.org/en-us/specification/assigned-numbers/hands-free-profile
*/
typedef enum
{
    hf_indicators_invalid  = 0x00,
    hf_enhanced_safety     = 0x01,
    hf_battery_level       = 0x02
} hfp_indicators_assigned_id;


/*!
    @brief Possible cvsd audio settings allowed with secure connection in addition to mandatory safe settings 'S1'
*/
typedef enum
{
    hfp_audio_setting_none,
    hfp_audio_setting_s4,
    hfp_audio_setting_s3
} hfp_secure_audio_settings;

/*!
    @brief Structure containing separate values for each supported indicator
*/
typedef struct
{
    unsigned service:2;
    unsigned signal_strength:2;
    unsigned roaming_status:2;
    unsigned battery_charge:2;
    unsigned unused:8;
} hfp_optional_indicators;

/*!
    @brief Possible CSR2CSR codecs
*/
typedef enum
{
    /*! No Codec. */
    hfp_csr_codec_mask_none         = 0x00,
    /*! CVSD Codec. */
    hfp_csr_codec_mask_cvsd         = 0x01,
    /*! Auristream Codec - 2 bits per sample. */
    hfp_csr_codec_mask_adpcm_2bps   = 0x02,
    /*! Auristream Codec - 4 bits per sample. */
    hfp_csr_codec_mask_adpcm_4bps   = 0x04,

    FORCE_ENUM_TO_MIN_16BIT(hfp_csr_codec_mask)
} hfp_csr_codec_mask;

/*!
    @brief Possible CSR2CSR bandwidths (to use with above codecs)
*/
typedef enum
{
    /*! No reported bandwidths supported*/
    hfp_csr_bandwidth_none  = 0x00,
    /*! 8kHz. */
    hfp_csr_bandwidth_8kHz  = 0x01,
    /*! 16kHz. */
    hfp_csr_bandwidth_16kHz = 0x02,

    FORCE_ENUM_TO_MIN_16BIT(hfp_csr_bandwidth_mask)
} hfp_csr_bandwidth_mask;

/*!
    @brief CSR features 
*/
typedef struct
{
    unsigned                caller_name:1;
    unsigned                raw_text:1;
    unsigned                sms:1;
    unsigned                batt_level:1;
    unsigned                pwr_source:1;
    unsigned                codecs:3;
    unsigned                codec_bandwidths:2;
    unsigned                unused:6;
} hfp_csr_features;

/*!
    @brief Configuration parameters passed into the hfp profile library in
    order for an Hfp profile instance to be created and initialised.

    The library client (usually the application) must specify the profile to be
    supported. In the case of hands free it must also specify the set of
    supported features for that profile instance. Optionally, the client may
    also supply a service record. If the service_record pointer is set to null
    the default service record provided by the hfp library is used. If however
    the pointer is non null, this client supplied service record is used. The
    hfp library will still insert the correct rfcomm channel number into the
    service record but it will perform no other error checking on the record
    provided.  The client provided service record may be fairly large so the
    hfp library will not attempt to take a local copy of the data. For the
    moment the hfp library assumes that the service record pointer is in const
    or that the client will not free the pointer for the lifetime of the
    application.
    
    @param supported_profile This indicates which profiles the library should
    support. This can be headset profile, one of the hansfree profiles, or
    both headset profile and one of the handsfree profiles. 
*/
typedef struct
{
    /*! The profiles supported.                                         */
    hfp_profile               supported_profile;
    /*! See supported features flags.                                   */
    uint16                    supported_features;
    /*! User configurable information regarding the HF's codecs.        */
    hfp_wbs_codec_mask        supported_wbs_codecs;
    /*! Optional indicators the HF may not wish to receive              */
    hfp_optional_indicators   optional_indicators;
    /* pack bool parameters all into one word                           */
    unsigned                  unused:13;
    /*! Disable Noise Reduction/Echo Cancellation on connection         */
    unsigned                  disable_nrec:1;
    /*! Enable sending of extended error codes by AG's supporting them  */
    unsigned                  extended_errors:1;
    /*! Whether multiple connections to one profile are supported       */
    unsigned                  multipoint:1;
    /*! The time in minutes to attempt to reconnect for on link loss    */
    unsigned                  link_loss_time:8;
    /*! The time in seconds between link loss reconnect attempts        */
    unsigned                  link_loss_interval:8;
    /*! The CSR2CSR features to enable if supported by the remote AG    */
    hfp_csr_features          csr_features;
    /*! HF indicators feature HF supports */
    hfp_hf_indicators_mask  hf_indicators;
    /*! Qualcomm Codec Extension Mode IDs supported by HF */
    codec_mode_bit_type       hf_codec_modes;
} hfp_init_params;

/*!
    @brief Connection parameters for setting up an eSCO/SCO connection
*/
typedef struct
{
   uint32            bandwidth;
   uint16            max_latency;
   sync_air_coding   voice_settings;
   sync_retx_effort  retx_effort;
} hfp_audio_params;

/*!
     @brief Generic hfp status.
*/
typedef enum
{
    hfp_success,                        /*!< Success.*/
    hfp_fail,                           /*!< Failure.*/
    hfp_ag_failure,                     /*!< Failure - AG failure.*/
    hfp_no_connection_to_phone,         /*!< Failure - No connection to phone.*/
    hfp_operation_not_allowed,          /*!< Failure - Operation not allowed.*/
    hfp_operation_not_supported,        /*!< Failure - Operation not supported.*/
    hfp_ph_sim_pin_required,            /*!< Failure - PH-SIM PIN required.*/
    hfp_sim_not_inserted,               /*!< Failure - SIM not inserted.*/
    hfp_sim_pin_required,               /*!< Failure - SIM PIN required.*/
    hfp_sim_puk_required,               /*!< Failure - SIM PUK required.*/
    hfp_sim_failure,                    /*!< Failure - SIM failure.*/
    hfp_sim_busy,                       /*!< Failure - SIM busy.*/
    hfp_incorrect_password,             /*!< Failure - Incorrect password.*/
    hfp_sim_pin2_required,              /*!< Failure - SIM PIN2 required.*/
    hfp_sim_puk2_required,              /*!< Failure - SIM PUK2 required.*/
    hfp_memory_full,                    /*!< Failure - Memory full.*/
    hfp_invalid_index,                  /*!< Failure - Invalid index.*/
    hfp_memory_failure,                 /*!< Failure - Memory failure.*/
    hfp_text_string_too_long,           /*!< Failure - Text string too long.*/
    hfp_invalid_chars_in_text_string,   /*!< Failure - Invalid characters in text string.*/
    hfp_dial_string_too_long,           /*!< Failure - Dial string too long.*/
    hfp_invalid_chars_in_dial_string,   /*!< Failure - Invalid characters in dial string.*/
    hfp_no_network_service,             /*!< Failure - No network service.*/
    hfp_network_timeout,                /*!< Failure - Network timeout.*/
    hfp_network_not_allowed,            /*!< Failure - Network not allowed, emergency calls only.*/

    hfp_csr_not_inited,                 /*!< Failure - CSR extensions have not been initialised. */
    hfp_csr_no_slc,                     /*!< Failure - CSR extension request failed due to no SLC being present. */
    hfp_csr_invalid_params,             /*!< Failure - CSR extention request failed due to being supplied with invalid parameters from the application. */
    hfp_csr_invalid_ag_params,          /*!< Failure - CSR extention request failed due to being supplied with invalid parameters from the AG. */
    hfp_csr_mod_ind_no_mem,             /*!< Failure - CSR extension modify indicators failed due to not being able to allocate
                                             memory for AT Command generation.  A request with fewer indictors may succeed */
    hfp_timeout                         /*!< Failure - Timed out waiting for AG response */
} hfp_lib_status;


/*!
    @brief Return status for the HFP_INIT_CFM message
*/
typedef enum
{
    /*! Successful initialisation.*/
    hfp_init_success,
    /*! Unsuccessful as HFP is already initialised. HFP may still be used. */
    hfp_init_reinit_fail,    /* Errors after this are fatal, and hfp is de-initialised */

    /*! Unsuccessful due to RFCOMM channel registration failure. HFP Init failed.*/
    hfp_init_rfc_chan_fail,
    /*! Unsuccessful due to a service record registration failure. HFP Init failed.*/
    hfp_init_sdp_reg_fail
} hfp_init_status;


/*!
    @brief Return status for the HFP_SLC_CONNECT_CFM message
*/
typedef enum
{
    /*! Successful connection.*/
    hfp_connect_success,
    /*! Unsuccessful due to a service search failure.*/
    hfp_connect_sdp_fail,
    /*! Unsuccessful due to a service level connection failure.*/
    hfp_connect_slc_failed,
    /*! Unsuccessful as max supported connections have been reached.*/
    hfp_connect_failed_busy,
    /*! Unsuccessful due to RFCOMM connection failing to be established.*/
    hfp_connect_failed,
    /*! Unsuccessful due to attempt to connect to unallocated server channel.*/
    hfp_connect_server_channel_not_registered,
    /*! Unsuccessful due to connection attempt timing out.*/
    hfp_connect_timeout,
    /*! Unsuccessful due to remote device rejecting connection.*/
    hfp_connect_rejected,
    /*! Unsuccessful due to security failure*/
    hfp_connect_rejected_security,
    /*! Connect attempt aborted in favour of incoming connection from same device */
    hfp_connect_failed_crossover
} hfp_connect_status;

/*!
    @brief Return status for the HFP_SLC_LINK_LOSS_IND message
*/
typedef enum
{
    hfp_link_loss_none,
    hfp_link_loss_recovery,
    hfp_link_loss_timeout,
    hfp_link_loss_abort
} hfp_link_loss_status;


/*!
    @brief Return status for the HFP_SLC_DISCONNECT_IND message
*/
typedef enum
{
    /*! Successful disconnection.*/
    hfp_disconnect_success,
    /*! Unsuccessful due to abnormal link loss.*/
    hfp_disconnect_link_loss,
    /*! Unsuccessful due to no current connection.*/
    hfp_disconnect_no_slc,
    /*! Disconnection has occurred but in an abnormal way */
    hfp_disconnect_abnormally,
    /*! Unsuccessful due to RFCOMM connection attempt error.*/
    hfp_disconnect_error,
    /*! RFCOMM disconnected as transferred to TWS Peer.*/
    hfp_disconnect_transferred
} hfp_disconnect_status;

/*!
    @brief Link priority is used to identify different links to
    AG devices using the order in which the devices were connected.
*/
typedef enum
{
    /*! Invalid Link. */
    hfp_invalid_link,
    /*! The link that was connected first. */
    hfp_primary_link,
    /*! The link that was connected second. */
    hfp_secondary_link
} hfp_link_priority;


/*!
    @brief Return status for the HFP_AUDIO_CONNECT_CFM message.
*/
typedef enum
{
    /*! Successful audio connection.*/
    hfp_audio_connect_success,
    /*! Unsuccessful due to negotiation failure.*/
    hfp_audio_connect_failure,
    /*! Unsuccessful due to audio already being with device.*/
    hfp_audio_connect_have_audio,
    /*! Unsuccessful due to an audio connect already being attempted.*/
    hfp_audio_connect_in_progress,
    /*! Unsuccessful due to one or more parameters specified being invalid.*/
    hfp_audio_connect_invalid_params,
    /*! Unsuccessful due to library being in incorrect state.*/
    hfp_audio_connect_error,
    /*! Unsuccessful due to AG being unable to begin codec negotiation.*/
    hfp_audio_connect_codec_neg_fail,
    /*! Unsuccessful due to unexpected error during codec negotiation.*/
    hfp_audio_connect_codec_neg_error,
    /*! Successful audio connection but no HFP link is present*/
    hfp_audio_connect_no_hfp_link
} hfp_audio_connect_status;


/*!
    @brief Return status for the HFP_AUDIO_DISCONNECT_IND message.
*/
typedef enum
{
    /*! Successful audio disconnection.*/
    hfp_audio_disconnect_success,
    /*! Unsuccessful due to failure indication from firmware.*/
    hfp_audio_disconnect_failure,
    /*! Unsuccessful due to audio being with AG.*/
    hfp_audio_disconnect_no_audio,
    /*! Unsuccessful due to an audio disconnect already being attempted.*/
    hfp_audio_disconnect_in_progress,
    /*! Unsuccessful due to library being in incorrect state.*/
    hfp_audio_disconnect_error,
    /*! Disconnected due to the connection being transferred.*/
    hfp_audio_disconnect_transferred
} hfp_audio_disconnect_status;


/*!
    @brief Transfer direction for audio connection.
*/
typedef enum
{
    /*! Transfer the audio to the HFP device.*/
    hfp_audio_to_hfp,
    /*! Transfer the audio to the audio gateway.*/
    hfp_audio_to_ag,
    /*! Toggle direction of current audio.*/
    hfp_audio_transfer
} hfp_audio_transfer_direction;

/*!
    @brief Actions that can be carried out using HfpCallHoldActionRequest.
*/
typedef enum
{
    /*! CHLD=0 */
    hfp_chld_release_held_reject_waiting = 0,
    /*! CHLD=1 */
    hfp_chld_release_active_accept_other = 1,
    /*! CHLD=2 */
    hfp_chld_hold_active_accept_other    = 2,
    /*! CHLD=3 */
    hfp_chld_add_held_to_multiparty      = 3,
    /*! CHLD=4 */
    hfp_chld_join_calls_and_hang_up      = 4
} hfp_chld_action;

/*!
    @brief The call state of a connection. Maps directly onto the call SM.
*/
typedef enum
{
    hfp_call_state_idle,
    hfp_call_state_incoming,
    hfp_call_state_incoming_held,
    hfp_call_state_outgoing,
    hfp_call_state_active,
    hfp_call_state_twc_incoming,    /* TWC: Three way call */
    hfp_call_state_twc_outgoing,
    hfp_call_state_held_active,
    hfp_call_state_held_remaining,
    hfp_call_state_multiparty
} hfp_call_state;

/*!
    @brief Values of the subscriber number service.
*/
typedef enum
{
    /*! Asynchronous modem.*/
    hfp_service_async_modem,
    /*! Synchronous modem.*/
    hfp_service_sync_modem,
    /*! PAD access (synchronous).*/
    hfp_service_pad_access,
    /*! Packet access (asynchronous).*/
    hfp_service_packet_access,
    /*! Voice.*/
    hfp_service_voice,
    /*! Fax.*/
    hfp_service_fax
} hfp_subscriber_service;


/*!
    @brief Call direction used in HFP_CURRENT_CALLS_IND message.
*/
typedef enum
{
    /*! Call from AG to network.*/
    hfp_call_mobile_originated,
    /*! Call from network to AG.*/
    hfp_call_mobile_terminated
} hfp_call_direction;


/*!
    @brief Call status used in HFP_CURRENT_CALLS_IND message.
*/
typedef enum
{
    /*! Call is currently active.*/
    hfp_call_active,
    /*! Call is currently held.*/
    hfp_call_held,
    /*! Call is being dialled - mobile originated only.*/
    hfp_call_dialling,
    /*! Call is alerting - mobile originated only.*/
    hfp_call_alerting,
    /*! Call is incoming - mobile terminated only.*/
    hfp_call_incoming,
    /*! Call is waiting - mobile terminated only.*/
    hfp_call_waiting
} hfp_call_status;


/*!
    @brief Call mode used in HFP_CURRENT_CALLS_IND message.
*/
typedef enum
{
    /*! Voice call.*/
    hfp_call_voice,
    /*! Data call.*/
    hfp_call_data,
    /*! FAX call.*/
    hfp_call_fax
} hfp_call_mode;


/*!
    @brief Call multiparty status used in HFP_CURRENT_CALLS_IND message.
*/
typedef enum
{
    /*! Call is not multiparty.*/
    hfp_not_multiparty_call,
    /*! Call is multiparty.*/
    hfp_multiparty_call
} hfp_call_multiparty;


/*!
    @brief Response and hold actions that can be used in HfpResponseHoldActionRequest.
*/
typedef enum
{
    /*! Place an incoming call on hold.*/
    hfp_hold_incoming_call,
    /*! Accept a previously held incoming call.*/
    hfp_accept_held_incoming_call,
    /*! Reject a previously held incoming call.*/
    hfp_reject_held_incoming_call
} hfp_btrh_action;


/*!
    @brief Used to identify type of number specified in HFP_SUBSCRIBER_NUMBER_IND,
           HFP_CALLER_ID_IND, HFP_CALL_WAITING_IND and HFP_CURRENT_CALLS_IND.
*/
typedef enum
{
    /*! Type of number is unknown.*/
    hfp_number_unknown,
    /*! Number is an international number.*/
    hfp_number_international,
    /*! Number is a national number.*/
    hfp_number_national,
    /*! Number is a network specific number.*/
    hfp_number_network,
    /*! Number is a dedicated access, short code.*/
    hfp_number_dedicated
} hfp_number_type;

/*!
    @brief Power source status.
*/
typedef enum
{
    /*! Device in using battery power. */
    hfp_csr_pwr_rep_battery = 1,
    /*! Device is using an external power source. */
    hfp_csr_pwr_rep_external = 2
} hfp_csr_power_status_report;

extern const hfp_audio_params default_s4_esco_audio_params;
extern const hfp_audio_params default_esco_audio_params;
extern const hfp_audio_params default_sco_audio_params;


/*!
 * \brief hfp_handover_if
 *
 *        Struct containing interface function pointers for marshalling and handover
 *        operations.  See handover_if library documentation for more information.
 */
extern const handover_interface hfp_handover_if;


#endif /* HFP_H_ */

/** @} */
