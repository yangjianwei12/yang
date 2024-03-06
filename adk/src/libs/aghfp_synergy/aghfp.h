/****************************************************************************
Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.

*/

/*!
@file	aghfp.h
@brief	Header file for the Audio Gateway Hands Free Profile library.

		This file documents the Audio Gateway Hands Free Profile library
		APIs.
		
*/

#ifndef AGHFP_H_
#define AGHFP_H_

#include <csr_bt_hfg_prim.h>
#include <library.h>
#include <bdaddr_.h>
#include <connection.h>
#include <message.h>


/* The maximum number of codecs the HFP can handle. */
#define AGHFP_MAX_NUM_CODECS	(5)

/* HF indicators as defined by HF Assigned number */
#define AGHFP_MAX_HF_INDICATORS	(2)

/*!  Aghfp Flag Defines

	These flags can be or'd together and used as the supported_features passed
    into AghfpInit. These values correspond to those defined for use with the
    +BRSF AT command in the HFP spec.
*/
typedef enum
{
	aghfp_three_way_calling = 0x1, 		/*!	@brief Device supports three way call control. */
	aghfp_nrec_function = 0x2,			/*!	@brief Device implements noise reduction / echo cancellation. */
	aghfp_voice_recognition = 0x4,		/*!	@brief Device supports voice recognition. */
	aghfp_inband_ring = 0x8,			/*! @brief Device can send inband ring tones. */
	aghfp_attach_phone_number = 0x10,	/*!	@brief Device can attach a phone number to a voice tag. */
	aghfp_incoming_call_reject = 0x20,	/*! @brief Device implements incoming call reject. */
	aghfp_enhanced_call_status = 0x40,  /*! @brief Setting this flag indicates that this device implements enhanced call status. */
	aghfp_enhanced_call_control = 0x80, /*! @brief Setting this flag indicates that this device implements enhanced call control. */
	aghfp_extended_error_codes = 0x100, /*! @brief Setting this flag indicates that this device implements extended error result codes. */
	aghfp_codec_negotiation = 0x200,    /*! @brief Setting this flag indicates that this supports codec negotiation. */
	aghfp_hf_indicators = 0x400,        /*!brief Setting this flag indicates that this supports hf indicators support. */
	aghfp_esco_s4_supported = 0x800,    /*! @brief Setting this flag indicates that this supports eSCO S4 audio settings */
	aghfp_enhanced_voice_recognition_status = 0x1000,     /*! @brief Device supports Enhanced Voice Recognition Status */
	aghfp_voice_recognition_text = 0x2000                 /*! @brief Device supports Voice Recognition Text */
} aghfp_supported_features;

/*! @brief This enum defines mask values to represent indicators sent to the HF */
typedef enum
{
    aghfp_service_indicator           = 0x01,
    aghfp_call_indicator              = 0x02,
    aghfp_call_setup_indicator        = 0x04,
    aghfp_call_held_indicator         = 0x08,
    aghfp_signal_strength_indicator   = 0x10,
    aghfp_roaming_status_indicator    = 0x20,
    aghfp_battery_charge_indicator    = 0x40,
    aghfp_all_indicators              = 0x7f
} aghfp_indicator_mask;

/*!	@brief The supported profiles. */
typedef enum
{
	aghfp_headset_profile,						/*!< As defined in part K6 of the Bluetooth specification. */
	aghfp_handsfree_profile,					/*!< As defined in version 1.0 of the handsfree profile specification. */
	aghfp_handsfree_15_profile,					/*!< As defined in version 1.5 of the handsfree profile specification. */
    aghfp_handsfree_16_profile,					/*!< As defined in version 1.6 of the handsfree profile specification. */
    aghfp_handsfree_17_profile,					/*!< As defined in version 1.7 of the handsfree profile specification. */
    aghfp_handsfree_18_profile					/*!< As defined in version 1.8 of the handsfree profile specification. */
} aghfp_profile;


/*! @brief Generic aghfp status. */
typedef enum
{
	aghfp_success,								/*!< Success. */
	aghfp_fail									/*!< Failure. */
} aghfp_lib_status;


/*!	@brief Return status for the AGHFP_INIT_CFM message */
typedef enum
{
	aghfp_init_success,							/*!< Successful initialisation. */
	aghfp_init_rfc_chan_fail,					/*!< Unsuccessful due to RFCOMM channel registration failure. */
	aghfp_init_sdp_reg_fail						/*!< Unsuccessful due to a service record registration failure. */
} aghfp_init_status;


/*!	@brief Return status for the AGHFP_SLC_CONNECT_CFM message */
typedef enum
{
	aghfp_connect_success,						/*!< Successful connection. */
	aghfp_connect_sdp_fail,						/*!< Unsuccessful due to a service search failure. */
	aghfp_connect_slc_failed,					/*!< Unsuccessful due to a service level connection failure. */
	aghfp_connect_failed_busy,					/*!< Unsuccessful due to service level connection already established. */
    aghfp_connect_failed,						/*!< Unsuccessful due to RFCOMM connection failing to be established. */
    aghfp_connect_server_channel_not_registered,/*!< Unsuccessful due to attempt to connect to unallocated server channel. */
    aghfp_connect_timeout,						/*!< Unsuccessful due to connection attempt timing out. */
    aghfp_connect_rejected,						/*!< Unsuccessful due to remote device rejecting connection. */
    aghfp_connect_normal_disconnect,			/*!< Unsuccessful due to remote device terminating the connection. */
    aghfp_connect_abnormal_disconnect,			/*!< Unsuccessful due to an abnormal disconnect while establishing an rfcomm connection. */
    aghfp_connect_sdp_fail_no_connection,   	/*!< Service search failed as connection to remote device couldn't be made. */
    aghfp_connect_sdp_fail_no_data,       	    /*!< Service search failed as remote device didn't return the requested data. */
    aghfp_connect_security_reject			    /*!< Unsuccessful due to rejected security. */
} aghfp_connect_status;


/*!	@brief Return status for the AGHFP_SLC_DISCONNECT_IND message */
typedef enum
{
	aghfp_disconnect_success,					/*!< Successful disconnection. */
	aghfp_disconnect_link_loss,					/*!< Unsuccessful due to abnormal link loss. */
	aghfp_disconnect_no_slc,					/*!< Unsuccessful due to no current connection. */
	aghfp_disconnect_timeout,					/*!< Unsuccessful due to RFCOMM connection attempt timeout. */
	aghfp_disconnect_error						/*!< Unsuccessful due to RFCOMM connection attempt error. */
} aghfp_disconnect_status;

/*!
	@brief Return status for the AGHFP_CALL_MGR_CREATE_CFM message.
*/
typedef enum
{
    /*! Successful call creation.*/
    aghfp_call_create_success,
    /*! Unsuccessful due to failure indication from firmware.*/
    aghfp_call_create_failure,
    /*! Unsuccessful due to a call create/terminate already being attempted.*/
    aghfp_call_create_in_progress,
    /*! Unsuccessful due to call terminate being issued.*/
    aghfp_call_create_aborted,
    /*! Unsuccessful due to slc being removed.*/
    aghfp_call_create_slc_removed,
    /*! Unsuccessful due to audio already existsing.*/
    aghfp_call_create_have_audio,
    /*! Unsuccessful due to one or more parameters specified being invalid.*/
    aghfp_call_create_invalid_params,
    /*! Unsuccessful due to audio handler currently opening/closing an audio channel.*/
    aghfp_call_create_audio_handler_active,
    /*! Unsuccessful due to library being in incorrect state.*/
    aghfp_call_create_error
} aghfp_call_create_status;


/*!
	@brief Return status for the AGHFP_CALL_MGR_TERMINATE_IND message.
*/
typedef enum
{
    /*! Successful call termination.*/
    aghfp_call_terminate_success,
    /*! Unsuccessful due to failure indication from firmware.*/
    aghfp_call_terminate_failure,
    /*! Unsuccessful due to library being in incorrect state.*/
    aghfp_call_terminate_error
} aghfp_call_terminate_status;


/*!
	@brief Return status for the AGHFP_AUDIO_CONNECT_CFM message.
*/
typedef enum
{
    /*! Successful audio connection.*/
    aghfp_audio_connect_success,
    /*! Unsuccessful due to negotiation failure.*/
    aghfp_audio_connect_failure,
    /*! Unsuccessful due to audio already being with device.*/
    aghfp_audio_connect_have_audio,
    /*! Unsuccessful due to an audio connect already being attempted.*/
    aghfp_audio_connect_in_progress,
    /*! Unsuccessful due to one or more parameters specified being invalid.*/
    aghfp_audio_connect_invalid_params,
    /*! Unsuccessful due to Call Manager setting up/shutting down a call (and hence audio).*/
    aghfp_audio_connect_call_manager_active,
    /*! Unsuccessful due to library being in incorrect state.*/
    aghfp_audio_connect_error,
    /*! Unsuccessful due to a Wide Band Speech Error. */
    aghfp_audio_connect_wbs_fail,
    /*! Unsuccessful due to connection attempt timing out.*/
    aghfp_audio_connect_timeout
} aghfp_audio_connect_status;


/*!
	@brief Return status for the AGHFP_AUDIO_DISCONNECT_IND message.
*/
typedef enum
{
    /*! Successful audio disconnection.*/
    aghfp_audio_disconnect_success,
    /*! Unsuccessful due to failure indication from firmware.*/
    aghfp_audio_disconnect_failure,
    /*! Unsuccessful due to audio being with AG.*/
    aghfp_audio_disconnect_no_audio,
    /*! Unsuccessful due to an audio disconnect already being attempted.*/
    aghfp_audio_disconnect_in_progress,
    /*! Unsuccessful due to Call Manager setting up/shutting down a call (and hence audio).*/
    aghfp_audio_disconnect_call_manager_active,
    /*! Unsuccessful due to library being in incorrect state.*/
    aghfp_audio_disconnect_error
} aghfp_audio_disconnect_status;


/*!
	@brief Transfer direction for audio connection.
*/
typedef enum
{
	/*! Transfer the audio to the HFP device.*/
	aghfp_audio_to_hfp,
	/*! Transfer the audio to the audio gateway.*/
	aghfp_audio_to_ag,
	/*! Toggle direction of current audio.*/
	aghfp_audio_transfer
} aghfp_audio_transfer_direction;


/*! @brief Service indicator parameter. Used in the function AghfpSendServiceIndicator */
typedef enum
{
	aghfp_service_none,							/*!< Implies no service. No Home/Roam network available. */
	aghfp_service_present						/*!< Implies present of service. Home/Roam network available. */
} aghfp_service_availability;


/*! @brief Call indicator parameter. Used in the function AghfpSendCallIndicator */
typedef enum
{
	aghfp_call_none,							/*!< Means no call active. */
	aghfp_call_active							/*!< Means a call is active. */
} aghfp_call_status;


/*! @brief Call setup indicator parameter. Used in the function AghfpSendCallSetupIndicator */
typedef enum
{
	aghfp_call_setup_none,						/*!< Means not currently in call setup. */
	aghfp_call_setup_incoming,					/*!< Means an incoming call process ongoing. */
	aghfp_call_setup_outgoing,					/*!< Means an outgoing call setup is ongoing. */
	aghfp_call_setup_remote_alert				/*!< Means remote part being alerted in an outgoing call. */
} aghfp_call_setup_status;


/*! @brief Call held indicator parameter */
typedef enum
{
    aghfp_call_held_none,
    aghfp_call_held_active,
    aghfp_call_held_remaining
} aghfp_call_held_status;


/*! @brief Roaming status indicator parameter */
typedef enum
{
    aghfp_roam_none,
    aghfp_roam_active
} aghfp_roam_status;


typedef struct
{
    uint16  id;
    uint8   type;
    uint8   service;
    uint16  size_number;
	uint8   *number;
} aghfp_subscriber_info;


typedef enum
{
    aghfp_response_hold_incoming_held,
    aghfp_response_hold_held_accepted,
    aghfp_response_hold_held_rejected
} aghfp_response_hold_state;


typedef enum
{
    aghfp_hold_incoming,
    aghfp_accept_held,
    aghfp_reject_held
} aghfp_response_hold_cmd;


typedef enum
{
    aghfp_call_dir_outgoing,
    aghfp_call_dir_incoming
} aghfp_call_dir;


typedef enum
{
    aghfp_call_state_active,
    aghfp_call_state_held,
    aghfp_call_state_dialling,
    aghfp_call_state_alerting,
    aghfp_call_state_incoming,
    aghfp_call_state_waiting
} aghfp_call_state;


typedef enum
{
    aghfp_call_mode_voice,
    aghfp_call_mode_data,
    aghfp_call_mode_fax
} aghfp_call_mode;


typedef enum
{
    aghfp_call_not_mpty,
    aghfp_call_is_mpty
} aghfp_call_mpty;

/*!
	@brief WB-Speech codec bit masks.
	
	Currently, AMR-WB is unsupported
*/
typedef enum
{
	wbs_codec_invalid	= 0,
	/*! SBC Codec. SBC support is mandatory. */
	wbs_codec_cvsd 	    = (1 << 0),
	/*! SBC Codec. SBC support is mandatory. */
	wbs_codec_msbc 	    = (1 << 1)
} aghfp_wbs_codec;

/*!
    @brief HF Indicators bit masks.
*/
typedef enum
{
    /*! No HF indicator. */
    aghfp_indicator_mask_none           = 0,
    /*! Enhanced safety indicator. */
    aghfp_enhanced_safety_mask       = (1 << 0),
    /*! Battery level indicator. */
    aghfp_battery_level_mask         = (1 << 1)
} aghfp_hf_indicators_mask;

/*! @brief  Assigned numbers for HF indicators as per
  https://www.bluetooth.org/en-us/specification/assigned-numbers/hands-free-profile
*/
typedef enum
{
    aghfp_hf_indicators_invalid  = 0x00,
    aghfp_enhanced_safety        = 0x01,
    aghfp_battery_level          = 0x02
} aghfp_hf_indicators_assigned_id;

/*!
    @brief Possible values indicators can take for hf_indicators
*/
typedef enum
{
    aghfp_hf_indicator_off    = 0,
    aghfp_hf_indicator_on     = 1
} aghfp_hf_indicator_state;
/*!
   @brief HF indicators supported by AG
*/
typedef struct
{
    aghfp_hf_indicators_mask   active_hf_indicators; /* HF Indicators supported by AG*/
    uint16 hf_indicators_state; /* bit mask of hf indicators state */
}aghfp_hf_indicator_info;

typedef enum
{
	aghfp_codec_negotiation_type_none,
	aghfp_codec_negotiation_type_wbs,
	aghfp_codec_negotiation_type_csr
} aghfp_codec_negotiation_type;

typedef struct
{
    uint16              idx;
    aghfp_call_dir      dir;
    aghfp_call_state    status;
    aghfp_call_mode     mode;
    aghfp_call_mpty     mpty;
    uint8               type;
    uint16              size_number;
    uint8              *number;
} aghfp_call_info;


typedef enum
{
	aghfp_call_type_incoming,		/* Create new incoming call to HS/HF */
	aghfp_call_type_outgoing,		/* Create new outgoing call to HS/HF */
	aghfp_call_type_transfer		/* Route existing outgoing call to HS/HF */
} aghfp_call_type;


/*!
	@brief Structure defining user definable information regarding the AG's codecs.

*/
typedef struct
{
	uint8		num_codecs;
	uint16		ag_codecs;
	aghfp_wbs_codec		codec_ids[AGHFP_MAX_NUM_CODECS];
} aghfp_codecs_info;

/* Structure to hold one CSR feature as published in the AT+CSRFN command. */
typedef struct
{
    uint16          indicator;
    uint16          value;
} aghfp_csr_feature_indicator;

typedef CsrBtHfgAudioLinkParameterListConfig    aghfp_audio_params;
typedef CsrBtHfgAudioDisconnectInd              AGHFP_AUDIO_DISCONNECT_IND_T  ;
typedef CsrBtHfgRejectInd                       AGHFP_CALL_HANG_UP_IND_T ;
typedef CsrBtHfgAnswerInd                       AGHFP_ANSWER_IND_T ;
typedef CsrBtHfgDisconnectInd                   AGHFP_SLC_DISCONNECT_IND_T ;
typedef CsrBtHfgAudioConnectInd                 AGHFP_AUDIO_CONNECT_IND_T;
typedef CsrBtHfgNoiseEchoInd                    AGHFP_NREC_SETUP_IND_T;
typedef CsrBtHfgAudioConnectCfm                 AGHFP_AUDIO_CONNECT_CFM_T;
typedef CsrBtHfgServiceConnectInd               AGHFP_SLC_CONNECT_IND_T;
typedef CsrBtHfgDialInd                         AGHFP_DIAL_IND_T;
typedef CsrBtHfgDialInd                         AGHFP_MEMORY_DIAL_IND_T;
typedef CsrBtHfgDialInd                         AGHFP_LAST_NUMBER_REDIAL_IND_T ;
typedef CsrBtHfgOperatorInd                     AGHFP_NETWORK_OPERATOR_IND_T;
typedef CsrBtHfgSubscriberNumberInd             AGHFP_SUBSCRIBER_NUMBER_IND_T;
typedef CsrBtHfgCallListInd                     AGHFP_CURRENT_CALLS_IND_T;
typedef CsrBtHfgCallHandlingInd                 AGHFP_RESPONSE_HOLD_STATUS_REQUEST_IND_T;
typedef CsrBtHfgMicGainInd                      AGHFP_SYNC_MICROPHONE_GAIN_IND_T;
typedef CsrBtHfgSpeakerGainInd                  AGHFP_SYNC_SPEAKER_VOLUME_IND_T;
typedef CsrBtHfgManualIndicatorInd              AGHFP_CALL_INDICATIONS_STATUS_REQUEST_IND_T;


#endif /* AGHFP_H_ */
