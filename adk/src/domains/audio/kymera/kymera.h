/*!
\copyright  Copyright (c) 2017-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file		kymera.h
\defgroup   kymera Kymera
\ingroup    audio_domain
\brief      The Kymera Manager API

*/

#ifndef KYMERA_H
#define KYMERA_H

#include <chain.h>
#include <transform.h>
#include <hfp.h>
#include <a2dp.h>
#include <anc.h>
#include <audio_plugin_common.h>
#include <kymera_config.h>
#include <rtime.h>
#include "va_audio_types.h"
#include "aec_leakthrough.h"
#include "ringtone/ringtone_if.h"
#include "file/file_if.h"
#include "usb_audio.h"
#include "kymera_aec.h"
#include "kymera_output_common_chain.h"

#include <kymera_adaptation_audio_protected.h>
#include <kymera_adaptation_voice_protected.h>

/*! @{ */

/*! Microphone: There are no microphone*/
#define NO_MIC (0)

/*! \brief List of all supported callbacks. */
typedef struct
{
    bool (*GetA2dpParametersPrediction)(uint32 *rate, uint8 *seid);
} kymera_callback_configs_t;



typedef struct
{
    /*! Pointer to the definition of tone being played. */
    const ringtone_note *tone;
} KYMERA_NOTIFICATION_TONE_STARTED_T;

typedef struct
{
    /*! File index of the voice prompt being played. */
    FILE_INDEX id;
} KYMERA_NOTIFICATION_PROMPT_STARTED_T;

typedef struct
{
    /*! Number of the first band being changed */
    uint8 start_band;
    /*! Number of the last band being changed */
    uint8 end_band;
} KYMERA_NOTIFCATION_USER_EQ_BANDS_UPDATED_T;

typedef struct
{
    /* The info variable is  interpreted according to message that it is delievered to clients.
            event_id  for  AANC_EVENT_CLEAR
            gain value for KYMERA_AANC_EVENT_ED_INACTIVE_GAIN_UNCHANGED
            flags recevied for KYMERA_AANC_EVENT_ED_ACTIVE
            NA    when KYMERA_AANC_EVENT_QUIET_MODE
            Category for Noise ID
        */
    uint16 info;
} kymera_aanc_event_msg_t;

typedef kymera_aanc_event_msg_t KYMERA_AANC_CLEAR_IND_T;
typedef kymera_aanc_event_msg_t KYMERA_AANC_ED_ACTIVE_TRIGGER_IND_T;
typedef kymera_aanc_event_msg_t KYMERA_AANC_ED_INACTIVE_TRIGGER_IND_T;
typedef kymera_aanc_event_msg_t KYMERA_AANC_QUIET_MODE_TRIGGER_IND_T;
typedef kymera_aanc_event_msg_t KYMERA_AANC_EVENT_IND_T;
typedef kymera_aanc_event_msg_t KYMERA_AANC_NOISE_ID_IND_T;


/*! \brief Events that Kymera send to its registered clients */
typedef enum kymera_messages
{
    /*! A tone have started */
    KYMERA_NOTIFICATION_TONE_STARTED = KYMERA_MESSAGE_BASE,
    /*! A voice prompt have started */
    KYMERA_NOTIFICATION_PROMPT_STARTED,
    /*! Latency reconfiguration has completed */
    KYMERA_LATENCY_MANAGER_RECONFIG_COMPLETE_IND,
    /*! Latency reconfiguration has failed */
    KYMERA_LATENCY_MANAGER_RECONFIG_FAILED_IND,
    /*! EQ available notification */
    KYMERA_NOTIFICATION_EQ_AVAILABLE,
    /*! EQ unavailable notification */
    KYMERA_NOTIFICATION_EQ_UNAVAILABLE,
    /*! EQ bands updated notification */
    KYMERA_NOTIFCATION_USER_EQ_BANDS_UPDATED,

    KYMERA_AANC_ED_ACTIVE_TRIGGER_IND,
    KYMERA_AANC_ED_INACTIVE_TRIGGER_IND,
    KYMERA_AANC_QUIET_MODE_TRIGGER_IND,
    KYMERA_AANC_ED_ACTIVE_CLEAR_IND,
    KYMERA_AANC_ED_INACTIVE_CLEAR_IND,
    KYMERA_AANC_QUIET_MODE_CLEAR_IND,
    KYMERA_AANC_NOISE_ID_IND,
    KYMERA_LOW_LATENCY_STATE_CHANGED_IND,

    KYMERA_AANC_BAD_ENVIRONMENT_TRIGGER_IND,
    KYMERA_AANC_BAD_ENVIRONMENT_CLEAR_IND,
    KYMERA_EFT_GOOD_FIT_IND,
    KYMERA_EFT_BAD_FIT_IND,
    KYMERA_PROMPT_END_IND,
    KYMERA_WIND_STAGE1_DETECTED,
    KYMERA_WIND_STAGE2_DETECTED,
    KYMERA_WIND_STAGE1_RELEASED,
    KYMERA_WIND_STAGE2_RELEASED,
    KYMERA_HIGH_BANDWIDTH_STATE_CHANGED_IND,
    KYMERA_STREAM_MODIFIER_CHANGED_IND,

    /*! Capability disable complete message. */
    KYMERA_ANC_COMMON_CAPABILITY_DISABLE_COMPLETE,
    
    /*! Capability enable complete message. */
    KYMERA_ANC_COMMON_CAPABILITY_ENABLE_COMPLETE,

    /*! Capability mode change trigger message. */
    KYMERA_ANC_COMMON_CAPABILITY_MODE_CHANGE_TRIGGER,

#ifdef INCLUDE_CVC_DEMO
    KYMERA_NOTIFICATION_CVC_SEND_MODE_CHANGED,
#endif

    /*! Indication that aptX Adaptive is streaming */
    KYMERA_APTX_ADAPTIVE_STREAMING_IND,

/*! This must be the final message */
    KYMERA_MESSAGE_END
} kymera_msg_t;

typedef enum
{
    NO_SCO,
    SCO_NB,
    SCO_WB,
    SCO_SWB,
#ifdef INCLUDE_SWB_LC3
    SCO_SWB_LC3,
#endif
    SCO_UWB
} appKymeraScoMode;

/*! \brief Sample rates used to configure SCO */
typedef struct
{
    /*! The rate to use for the chain input (microphone output) */
    uint32_t mic;
    /*! The rate to use for the use case (SCO SEND/SCO RECEIVE sample rate) */
    uint32_t use_case;
} appKymeraScoSampleRates;

typedef struct
{
    appKymeraScoMode mode;
    uint8 mic_cfg;
    const chain_config_t *chain;
    appKymeraScoSampleRates rates;
} appKymeraScoChainInfo;

/*! \brief The prompt file format */
typedef enum prompt_format
{
    PROMPT_FORMAT_PCM,
    PROMPT_FORMAT_SBC,
    PROMPT_FORMAT_AAC,
} promptFormat;

/*! \brief Defines the different audio sources supported for LEA broadcast
 *  \note Any new audio source to be added in between "KYMERA_AUDIO_SOURCE_NONE" & "KYMERA_AUDIO_SOURCES_MAX" in this enum
 */
typedef enum
{
    KYMERA_AUDIO_SOURCE_NONE = -1,
    KYMERA_AUDIO_SOURCE_A2DP,
    KYMERA_AUDIO_SOURCE_LE_AUDIO_UNICAST,
    KYMERA_AUDIO_SOURCES_MAX
} appKymeraLeAudioMediaSenderSourceType;

/*! \brief Defines the different codecs used for le audio */
typedef enum
{
    KYMERA_LE_AUDIO_CODEC_LC3,
    KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE,
    KYMERA_LE_AUDIO_CODEC_APTX_LITE
} appKymeraLeAudioCodec;

/*! \brief Defines the type of LE audio decoder chain configuration */
typedef enum
{
    KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO,         /* Single mono decoder to mono output chain configuration */
    KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_DUAL_MONO,    /* Single mono decoder split into two mono output chain configuration */
    KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_STEREO,       /* Single decoder (joint) to stereo output chain configuration. */
    KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_DUAL_DECODER_TO_STEREO,         /* Dual decoder to stereo output chain configuration. */
    KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_MAX
} appKymeraLeAudioDecoderConfigType;

/*! \brief Defines the type of LE audio encoder chain configuration */
typedef enum
{
    KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_SINGLE_MONO_CIS,      /* Stereo input to single mono CIS encoder chain configuration */
    KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_DUAL_MONO_CIS,        /* Stereo input to dual mono CIS encoder chain configuration */
    KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_SINGLE_STEREO_CIS,    /* Stereo input to single stereo CIS encoder chain configuration */
    KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_MAX
} appKymeraLeAudioEncoderConfigType;

typedef enum
{
    KYMERA_LE_STREAM_MONO,              /* Single BIS/CIS carrying mono, left only or right only data */
    KYMERA_LE_STREAM_STEREO_USE_LEFT,   /* Single BIS/CIS carrying stereo but use only LEFT */
    KYMERA_LE_STREAM_STEREO_USE_RIGHT,  /* Single BIS/CIS carrying stereo but use only RIGHT */
    KYMERA_LE_STREAM_STEREO_USE_BOTH,   /* Single BIS/CIS carrying stereo and use both */
    KYMERA_LE_STREAM_DUAL_MONO,         /* DUAL CIS carrying stereo and use both channel */
    KYMERA_LE_STREAM_MAX
} appKymeraLeStreamType;

typedef enum
{
    KYMERA_POWER_ACTIVATION_MODE_ASYNC,
    KYMERA_POWER_ACTIVATION_MODE_IMMEDIATE,
} appKymeraPowerActivationMode;

typedef enum
{
    KYMERA_AUDIO_STANDALONE_STEREO,
    KYMERA_AUDIO_MIRROR_MONO_LEFT,
    KYMERA_AUDIO_MIRROR_MONO_RIGHT,
    KYMERA_AUDIO_MIRROR_STEREO
} appKymeraAudioType;

typedef struct
{
    uint8 mic_cfg;
    const chain_config_t *chain;
    uint16 rate;
    appKymeraLeAudioCodec codec_type;
    bool is_voice_back_channel;
} appKymeraLeMicChainInfo;

typedef struct
{
    const appKymeraLeMicChainInfo *chain_table;
    unsigned table_length;
} appKymeraLeMicChainTable;

/*! \brief Parameters used to determine the VA encode chain config to use */
typedef struct
{
    va_audio_codec_t encoder;
} kymera_va_encode_chain_params_t;

typedef struct
{
    kymera_va_encode_chain_params_t chain_params;
    const chain_config_t *chain_config;
} appKymeraVaEncodeChainInfo;

typedef struct
{
    const appKymeraVaEncodeChainInfo *chain_table;
    unsigned table_length;
} appKymeraVaEncodeChainTable;

/*! \brief Parameters used to determine the VA WuW chain config to use */
typedef struct
{
    va_wuw_engine_t wuw_engine;
} kymera_va_wuw_chain_params_t;

typedef struct
{
    kymera_va_wuw_chain_params_t chain_params;
    const chain_config_t *chain_config;
} appKymeraVaWuwChainInfo;

typedef struct
{
    const appKymeraVaWuwChainInfo *chain_table;
    unsigned table_length;
} appKymeraVaWuwChainTable;

/*! \brief Parameters used to determine the VA mic chain config to use */
typedef struct
{
    unsigned wake_up_word_detection:1;
    unsigned clear_voice_capture:1;
    uint8 number_of_mics;
} kymera_va_mic_chain_params_t;

typedef struct
{
    kymera_va_mic_chain_params_t chain_params;
    const chain_config_t *chain_config;
} appKymeraVaMicChainInfo;

typedef struct
{
    const appKymeraVaMicChainInfo *chain_table;
    unsigned table_length;
} appKymeraVaMicChainTable;

typedef void (* KymeraVoiceCaptureStarted)(Source capture_source);

/*! \brief Response to a Wake-Up-Word detected indication */
typedef struct
{
    bool start_capture;
    KymeraVoiceCaptureStarted capture_callback;
    va_audio_wuw_capture_params_t capture_params;
} kymera_wuw_detected_response_t;

typedef kymera_wuw_detected_response_t (* KymeraWakeUpWordDetected)(const va_audio_wuw_detection_info_t *wuw_info);

typedef struct
{
    const chain_config_t *chain_config;
    appKymeraLeAudioCodec codec_type;
    appKymeraLeAudioDecoderConfigType decoder_config_type;
} appKymeraLeAudioChainInfo;

typedef struct
{
    const appKymeraLeAudioChainInfo *chain_table;
    unsigned table_length;
} appKymeraLeAudioChainTable;

typedef struct
{
    uint8 is_stereo_config;
    uint8 mic_count;
    uint16 sample_rate;
    const chain_config_t *chain_config;
} appKymeraLeVoiceChainInfo;

typedef struct
{
    const appKymeraLeVoiceChainInfo *chain_table;
    unsigned table_length;
} appKymeraLeVoiceChainTable;

typedef struct
{
    appKymeraLeAudioEncoderConfigType chain_type;
    const chain_config_t *chain;
    appKymeraLeAudioCodec codec_type;
} appKymeraUsbIsoLeAudioChainInfo;

typedef struct
{
    appKymeraLeAudioDecoderConfigType chain_type;
    const chain_config_t *chain;
    appKymeraLeAudioCodec codec_type;
} appKymeraIsoUsbLeAudioChainInfo;

typedef struct
{
    const appKymeraUsbIsoLeAudioChainInfo *chain_table;
    unsigned table_length;
} appKymeraUsbIsoChainTable;

typedef struct
{
    const appKymeraIsoUsbLeAudioChainInfo *chain_table;
    unsigned table_length;
} appKymeraIsoUsbChainTable;

typedef struct
{
    uint8 is_stereo_config;
    uint16 sample_rate;
    const chain_config_t *chain_config;
} appKymeraLeAudioFromAirVoiceChainInfo;

typedef struct
{
    const appKymeraLeAudioFromAirVoiceChainInfo *chain_table;
    unsigned table_length;
} appKymeraLeAudioFromAirVoiceChainTable;

typedef appKymeraUsbIsoLeAudioChainInfo appKymeraAnalogIsoLeAudioChainInfo;

typedef struct
{
    const appKymeraAnalogIsoLeAudioChainInfo *chain_table;
    unsigned table_length;
} appKymeraAnalogIsoChainTable;

#define MAX_NUMBER_SUPPORTED_MICS 4

typedef struct
{
    uint16 filter_type;
    uint16 cut_off_freq;
    int16  gain;
    uint16 q;
} kymera_eq_paramter_set_t;

typedef struct
{
    uint8 number_of_bands;
    kymera_eq_paramter_set_t *params;
} kymera_user_eq_bank_t;

typedef struct
{
    usb_voice_mode_t mode;
    uint8 mic_cfg;
    const chain_config_t *chain;
    uint32_t rate;
} appKymeraUsbVoiceChainInfo;

/*! Structure defining a single hardware output */
typedef struct
{
    /*! The type of output hardware */
    audio_hardware hardware;
    /*! The specific hardware instance used for this output */
    audio_instance instance;
    /*! The specific channel used for the output */
    audio_channel  channel;
} appKymeraHardwareOutput;

/*!Structure that defines Adaptive ANC connection parameters */
typedef struct
{
    bool in_ear;                          /*! to provide in-ear / out-ear status to adaptive anc capability */
    audio_anc_path_id control_path;       /*! to decide if FFa path becomes control or FFb */
    adaptive_anc_hw_channel_t hw_channel; /*! Hadware instance to select */
    anc_mode_t current_mode;              /*Current ANC mode*/
}KYMERA_INTERNAL_AANC_ENABLE_T;

typedef uint16 kymera_client_lock_t;

/*! \brief Start streaming A2DP audio.
    \param client_lock If not NULL, bits set in client_lock_mask will be cleared
           in client_lock when A2DP is started, or if an A2DP stop is requested,
           before A2DP has started.
    \param client_lock_mask A mask of bits to clear in the client_lock.
    \param codec_settings The A2DP codec settings.
    \param max_bitrate The max bitrate for the input stream (in bps). Ignored if zero.
    \param volume_in_db The start volume.
    \param master_pre_start_delay This function always sends an internal message
    to request the module start kymera. The internal message is sent conditionally
    on the completion of other activities, e.g. a tone. The caller may request
    that the internal message is sent master_pre_start_delay additional times before the
    start of kymera commences. The intention of this is to allow the caller to
    delay the starting of kymera (with its long, blocking functions) to match
    the message pipeline of some concurrent message sequence the caller doesn't
    want to be blocked by the starting of kymera. This delay is only applied
    when starting the 'master' (a non-TWS sink SEID).
    \param q2q_mode The source device is a Qualcomm device that supports Q2Q mode
*/
void appKymeraA2dpStart(kymera_client_lock_t *client_lock, uint16 client_lock_mask,
                        const a2dp_codec_settings *codec_settings,
                        uint32 max_bitrate,
                        int16 volume_in_db, uint8 master_pre_start_delay,
                        uint8 q2q_mode, aptx_adaptive_ttp_latencies_t nq2q_ttp);

/*! \brief Stop streaming A2DP audio.
    \param seid The stream endpoint ID to stop.
    \param source The source associatied with the seid.
*/
void appKymeraA2dpStop(uint8 seid, Source source);

/*! \brief Set the A2DP streaming volume.
    \param volume_in_db.
*/
void appKymeraA2dpSetVolume(int16 volume_in_db);

/*! Callback function type for informing caller that SCO chain has started */
typedef void (*Kymera_ScoStartedHandler)(void);

/*! \brief Start SCO audio.
    \param audio_sink The SCO audio sink.
    \param codec WB-Speech codec bit masks.
    \param wesco The link Wesco.
    \param volume_in_db The starting volume.
    \param pre_start_delay This function always sends an internal message
    to request the module start SCO. The internal message is sent conditionally
    on the completion of other activities, e.g. a tone. The caller may request
    that the internal message is sent pre_start_delay additional times before
    starting kymera. The intention of this is to allow the caller to
    delay the start of kymera (with its long, blocking functions) to match
    the message pipeline of some concurrent message sequence the caller doesn't
    want to be blocked by the starting of kymera.
    \param synchronised_start If TRUE, the chain will be started muted.
    #Kymera_ScheduleScoSyncUnmute should be called to define the time
    at which the audio output will be unmuted. Internally, the module guards
    against the user not calling #Kymera_ScheduleScoSyncUnmute by scheduling
    an unmute appConfigScoSyncUnmuteTimeoutMs after the chain is started.
    \param handler Function pointer called when the SCO chain has been started.
*/
bool appKymeraScoStart(Sink audio_sink, appKymeraScoMode mode, uint8 wesco,
                       int16 volume_in_db, uint8 pre_start_delay,
                       bool synchronised_start, Kymera_ScoStartedHandler handler);


/*! \brief Stop SCO audio.
*/
void appKymeraScoStop(void);

/*! \brief Set SCO volume.

    \param[in] volume_in_db.
 */
void appKymeraScoSetVolume(int16 volume_in_db);

/*! \brief Enable or disable MIC muting.

    \param[in] mute TRUE to mute MIC, FALSE to unmute MIC.
 */
void appKymeraScoMicMute(bool mute);

/*! \brief Get the SCO CVC voice quality.
    \return The voice quality.
 */
uint8 appKymeraScoVoiceQuality(void);

/*! \brief Play a tone.
    \param tone The address of the tone to play.
    \param ttp Time to play the audio tone in microseconds.
    \param interruptible If TRUE, the tone may be interrupted by another event
           before it is completed. If FALSE, the tone may not be interrupted by
           another event and will play to completion.
    \param is_loud If TRUE, the tone will be played at the loud tone volume as
           set in KYMERA_CONFIG_LOUD_TONE_VOLUME
    \param client_lock If not NULL, bits set in client_lock_mask will be cleared
           in client_lock when the tone finishes - either on completion, or when
           interrupted.
    \param client_lock_mask A mask of bits to clear in the client_lock.
*/
void appKymeraTonePlay(const ringtone_note *tone, rtime_t ttp, bool interruptible, bool is_loud,
                       kymera_client_lock_t *client_lock, uint16 client_lock_mask);

/*! \brief Play a prompt.
    \param prompt The file index of the prompt to play.
    \param format The prompt file format.
    \param rate The prompt sample rate.
    \param ttp The time to play the audio prompt in microseconds.
    \param interruptible If TRUE, the prompt may be interrupted by another event
           before it is completed. If FALSE, the prompt may not be interrupted by
           another event and will play to completion.
    \param client_lock If not NULL, bits set in client_lock_mask will be cleared
           in client_lock when the prompt finishes - either on completion, or when
           interrupted.
    \param client_lock_mask A mask of bits to clear in the client_lock.
*/
void appKymeraPromptPlay(FILE_INDEX prompt, promptFormat format,
                         uint32 rate, rtime_t ttp, bool interruptible,
                         kymera_client_lock_t *client_lock, uint16 client_lock_mask);

/*! \brief Stop playing an active tone or prompt

    Cancel/stop the currently playing tone or prompt.

    \note This command will only cancel tones and prompts that are allowed
        to be interrupted. This is specified in the interruptible parameter
        used when playing a tone/prompt.

    \note This API should not normally be used. Tones and prompts have a
        limited duration and will end within a reasonable timescale.
        Starting a new tone/prompt will also cancel any currently active tone.
*/
void appKymeraTonePromptCancel(void);


/*! \brief Initialise the kymera module. */
bool appKymeraInit(Task init_task);

/*! \brief Helper function that checks if the Kymera sub-system is idle

    Checking this does not guarantee that a subsequent function call that starts
    kymera activity will succeed.

    \return TRUE if the kymera sub-system was in the idle state at the time
                the function was called, FALSE otherwise.
 */
bool Kymera_IsIdle(void);

/*! \brief Register a Task to receive notifications from Kymera.

    Once registered, #client_task will receive #shadow_profile_msg_t messages.

    \param client_task Task to register to receive shadow_profile notifications.
*/
void Kymera_ClientRegister(Task client_task);

/*! \brief Un-register a Task that is receiving notifications from Kymera.

    If the task is not currently registered then nothing will be changed.

    \param client_task Task to un-register from shadow_profile notifications.
*/
void Kymera_ClientUnregister(Task client_task);



/*! \brief Configure downloadable capabilities bundles.

    This function must be called before appKymeraInit(),
    otherwise no downloadable capabilities will be loaded.

    \param config Pointer to bundle configuration.
*/
void Kymera_SetBundleConfig(const capability_bundle_config_t *config);


/*! \brief Set table used to determine audio chain based on SCO parameters.

    Table set by this function applies to primary TWS device as well as standalone device.

    This function must be called before audio is used.

    \param info SCO audio chains mapping.
*/
void Kymera_SetScoChainTable(const appKymeraScoChainInfo *info);

/*! \brief Set table used to determine audio chain based on VA encode parameters.

    Table set by this function applies to primary TWS device as well as standalone device.

    This function must be called before audio is used.

    \param chain_table VA encode chains mapping.
    */
void Kymera_SetVaEncodeChainTable(const appKymeraVaEncodeChainTable *chain_table);

/*! \brief Set table used to determine audio chain based on VA Mic parameters.

    Table set by this function applies to primary TWS device as well as standalone device.

    This function must be called before audio is used.

    \param chain_table VA mic chains mapping.
    */
void Kymera_SetVaMicChainTable(const appKymeraVaMicChainTable *chain_table);

/*! \brief Set table used to determine audio chain based on VA Wake Up Word parameters.

    Table set by this function applies to primary TWS device as well as standalone device.

    This function must be called before audio is used.

    \param chain_table Wake Up Word chains mapping.
    */
void Kymera_SetVaWuwChainTable(const appKymeraVaWuwChainTable *chain_table);

/*! \brief Set the left/right mixer mode
    \param stereo_lr_mix If TRUE, a 50/50 mix of the left and right stereo channels
    will be output by the mixer to the local device, otherwise, the left/right-ness
    of the earbud will be used to output 100% l/r.
*/
void appKymeraSetStereoLeftRightMix(bool stereo_lr_mix);

/*! \brief Enable or disable an external amplifier.
    \param enable TRUE to enable, FALSE to disable.
*/
void appKymeraExternalAmpControl(bool enable);

/*! Connect parameters for ANC tuning  */
typedef struct
{
    uint32 usb_rate;
    Source spkr_src;
    Sink mic_sink;
    uint8 spkr_channels;
    uint8 mic_channels;
    uint8 frame_size;
} anc_tuning_connect_parameters_t;

/*! Disconnect parameters for ANC tuning  */
typedef struct
{
    Source spkr_src;
    Sink mic_sink;
    void (*kymera_stopped_handler)(Source source);
} anc_tuning_disconnect_parameters_t;

/*! \brief Start Anc tuning procedure.
           Note that Device has to be enumerated as USB audio device before calling this API.
    \param anc tuning connect parameters
    \return void
*/
void KymeraAnc_EnterTuning(const anc_tuning_connect_parameters_t * param);

/*! \brief Stop ANC tuning procedure.
    \param anc tuning disconnect parameters
    \return void
*/
void KymeraAnc_ExitTuning(const anc_tuning_disconnect_parameters_t * param);

/*! \brief Cancel any pending KYMERA_INTERNAL_A2DP_START message.
    \param void
    \return void
*/
void appKymeraCancelA2dpStart(void);

/*! \brief Check if tone is playing.

    \return TRUE if tone is playing.
*/
bool appKymeraIsTonePlaying(void);

/*! \brief Prospectively start the DSP (if not already started).
 *  After a period, the DSP will be automatically stopped again if no activity
 *  is started
 *  \param Power activation mode: immediate or asynchronous */
void appKymeraProspectiveDspPowerOn(appKymeraPowerActivationMode mode);

#ifdef INCLUDE_MIRRORING
/*! \brief Set the primary/secondary synchronised start time
 *  \param clock Local clock synchronisation time instant
 */
void appKymeraA2dpSetSyncStartTime(uint32 clock);

/*! \brief Set the primary unmute time during a synchronised unmute.
 *  \param unmute_time Local clock synchronised unmute instant
 */
void appKymeraA2dpSetSyncUnmuteTime(rtime_t unmute_time);

/*!
 * \brief kymera_a2dp_mirror_handover_if
 *
 *        Struct containing interface function pointers for marshalling and handover
 *        operations.  See handover_if library documentation for more information.
 */
extern const handover_interface kymera_a2dp_mirror_handover_if;
#endif /* INCLUDE_MIRRORING */

/*! \brief Unmute of the main output of the SCO chain after a delay.
 *  \param delay The delay after which to unmute.
 */
void Kymera_ScheduleScoSyncUnmute(Delay delay);

/*! \brief Connects passthrough operator to dac in order to mitigate tonal noise
    observed in QCC512x devices*/
void KymeraAnc_ConnectPassthroughSupportChainToDac(void);

/*! \brief Disconnects passthrough operator from dac used in conjunction with
     KymeraAncConnectPassthroughSupportChainToDac(void) */
void KymeraAnc_DisconnectPassthroughSupportChainFromDac(void);

/*! \brief Creates passthrough operator support chain used for mitigation of tonal
    noise observed with QCC512x devices*/
void KymeraAnc_CreatePassthroughSupportChain(void);

/*! \brief Destroys passthrough operator support chain which could have been used with
    Qcc512x devices for mitigating tonal noise*/
void KymeraAnc_DestroyPassthroughSupportChain(void);

/*! \brief Start voice capture.
    \param callback Called when capture has started.
    \param params Parameters based on which the voice capture will be configured.
*/
void Kymera_StartVoiceCapture(KymeraVoiceCaptureStarted callback, const va_audio_voice_capture_params_t *params);

/*! \brief Stop voice capture.
*/
void Kymera_StopVoiceCapture(void);

/*! \brief Start Wake-Up-Word detection.
    \param callback Called when Wake-Up-Word is detected, a voice capture can be started based on the return.
    \param params Parameters based on which the Wake-Up-Word detection will be configured.
*/
void Kymera_StartWakeUpWordDetection(KymeraWakeUpWordDetected callback, const va_audio_wuw_detection_params_t *params);

/*! \brief Stop Wake-Up-Word detection.
*/
void Kymera_StopWakeUpWordDetection(void);

/*! \brief Get the version number of the WuW engine operator.
    \param wuw_engine The ID of the engine.
    \param version The version number to be populated.
*/
void Kymera_GetWakeUpWordEngineVersion(va_wuw_engine_t wuw_engine, va_audio_wuw_engine_version_t *version);

/*! \brief Store the WuW engine with the largest PM allocation requirements.
*/
void Kymera_StoreLargestWuwEngine(void);

/*! \brief Updates the DSP clock to the fastest possible speed in case of
 * A2DP and SCO before enabling the ANC and changing the mode and revert back to the previous
 * clock speed later to reduce the higher latency in peer sync
 */
void KymeraAnc_UpdateDspClock(bool boost);

/*! \brief Register for notification

    Registered task will receive KYMERA_NOTIFICATION_* messages.

    As it is intended to be used by test system it supports only one client.
    That is to minimise memory use.
    It can be extended to support arbitrary number of clients when necessary.

    \param task Listener task
*/
void Kymera_RegisterNotificationListener(Task task);

/*! Kymera API references for software leak-through */
/*! \brief Enables the leakthrough */
#ifdef ENABLE_AEC_LEAKTHROUGH
void Kymera_EnableLeakthrough(void);
#else
#define Kymera_EnableLeakthrough() ((void)(0))
#endif

/*! \brief Disable the leakthrough */
#ifdef ENABLE_AEC_LEAKTHROUGH
void Kymera_DisableLeakthrough(void);
#else
#define Kymera_DisableLeakthrough() ((void)(0))
#endif

/*! \brief Notify leakthough of a change in leakthrough mode */
#ifdef ENABLE_AEC_LEAKTHROUGH
void Kymera_LeakthroughUpdateMode(leakthrough_mode_t mode);
#else
#define Kymera_LeakthroughUpdateMode(x) ((void)(0))
#endif

/*! \brief Update leakthrough for AEC use case */
#ifdef ENABLE_AEC_LEAKTHROUGH
void Kymera_LeakthroughSetAecUseCase(aec_usecase_t usecase);
#else
#define Kymera_LeakthroughSetAecUseCase(x) ((void)(0))
#endif

/*! \brief Tries to enable the Adaptive ANC chain */
void Kymera_EnableAdaptiveAnc(bool in_ear, audio_anc_path_id path, adaptive_anc_hw_channel_t hw_channel, anc_mode_t mode);


/*! \brief Identify if noise level is below Quiet Mode threshold
    \param void
    \return TRUE if noise level is below threshold, otherwise FALSE
*/
bool Kymera_AdaptiveAncIsNoiseLevelBelowQuietModeThreshold(void);

/*! \brief Set up loop back from Mic input to DAC when kymera is ON

    \param mic_number select the mic to loop back
    \param sample_rate set the sampling rate for both input and output chain
 */
void appKymeraCreateLoopBackAudioChain(uint16 mic_number, uint32 sample_rate);

/*! \brief Destroy loop back from Mic input to DAC chain

    \param mic_number select the mic to loop back

 */
void appKymeraDestroyLoopbackAudioChain(uint16 mic_number);

/*! \brief Starts the wired analog audio chain
      \param volume_in_db Volume for wired analog
      \param rate sample rate at which wired analog audio needs to be played
      \param min_latency minimum latency identified for wired audio
      \param max_latency maximum latency identified for wired audio
      \param target_latency fixed latency identified for wired audio
 */
void Kymera_StartWiredAnalogAudio(int16 volume_in_db, uint32 rate, uint32 min_latency, uint32 max_latency, uint32 target_latency);

/*! \brief Stop the wired analog audio chain
 */
void Kymera_StopWiredAnalogAudio(void);

/*! \brief Set volume for Wired Audio chain.
    \param volume_in_db Volume to be set.
*/
void appKymeraWiredAudioSetVolume(int16 volume_in_db);

/*! \brief Set table used to determine audio chain based on LE Audio parameters.

    This function must be called before audio is used.

    \param info LE audio chains mapping.
*/
void Kymera_SetLeAudioChainTable(const appKymeraLeAudioChainTable *chain_table);

void Kymera_SetLeFromAirVoiceChainTable(const appKymeraLeAudioFromAirVoiceChainTable *chain_table);

/*! \brief Start streaming LE audio.

    \param media_present speaker/music configuration present.
    \param microphone_present microphone configuration present
    \param reconfig This is a reconfiguration request
    \param volume_in_db The start volume.
    \param le_media_config_t media configuration
    \param le_microphone_config_t microphone configuration
*/
void Kymera_LeAudioStart(bool media_present, bool microphone_present, bool reconfig,
                        int16 volume_in_db, const le_media_config_t *media,
                        const le_microphone_config_t *microphone);

/*! \brief Stop streaming LE audio.
*/
void Kymera_LeAudioStop(void);

/*! \brief Set the LE audio volume.
    \param volume_in_db.
*/
void Kymera_LeAudioSetVolume(int16 volume_in_db);

/*! \brief Unmute the LE audio output at a given time.

    \param unmute_time Time to unmute the LE audio output.
 */
void Kymera_LeAudioUnmute(rtime_t unmute_time);

/*! \brief Set table used to determine audio chain based on LE Voice parameters.

    This function must be called before LE voice is used.

    \param info LE voice chains mapping.
*/

#ifdef INCLUDE_LE_AUDIO_UNICAST
/*! \brief Set table used to determine mic chain based on LE mic parameters.

    \param info LE mic chains mapping.
*/
void Kymera_SetLeMicChainTable(const  appKymeraLeMicChainTable  *chain_table);

/*! \brief Unmute the LE Mic chain after a delay.
 *
 *  \param delay The delay after which to unmute.
 */

void Kymera_ScheduleLeaMicSyncUnmute(Delay delay);

/*! \brief Mute microphone during LE Audio/Voice usecase.
 *
 *  \param mute Mute microphone if true.
 */
void KymeraLeAudioVoice_SetMicMuteState(bool mute);

#endif

void Kymera_SetLeVoiceChainTable(const appKymeraLeVoiceChainTable *chain_table);

/*! \brief Start streaming LE voice.

    \param speaker_present speaker configuration present.
    \param microphone_present microphone configuration present
    \param reconfig This is a reconfiguration request
    \param volume_in_db The start volume.
    \param le_speaker_config_t speaker configuration
    \param le_microphone_config_t microphone configuration

*/
void Kymera_LeVoiceStart(bool speaker_present, bool microphone_present, bool reconfig,
                        int16 volume_in_db, const le_speaker_config_t *speaker, 
                        const le_microphone_config_t *microphone);

/*! \brief Stop streaming LE voice.
*/
void Kymera_LeVoiceStop(void);

/*! \brief Set the LE voice volume.
    \param volume_in_db.
*/
void Kymera_LeVoiceSetVolume(int16 volume_in_db);

/*! \brief Enable or disable MIC muting.

    \param[in] mute TRUE to mute MIC, FALSE to unmute MIC.
 */
void Kymera_LeVoiceMicMute(bool mute);

/*! \brief Mute to allow a syncronised unmute.

    This function will mute an already playing le broadcast stream.
    It can be un-muted with Kymera_LeAudioUnmute()
*/
void Kymera_LeAudioSyncMute(void);

/*! \brief Create and start USB Audio.
    \param channels USB Audio channels
    \param frame_size 16 bit/24bits.
    \param Sink USB OUT endpoint sink.
    \param volume_in_db Initial volume
    \param mute_status Initial mute state
    \param rate Sample Frequency.
    \param min_latency TTP minimum value in micro-seconds
    \param max_latency TTP max value in micro-seconds
    \param target_latency TTP default value in micro-seconds
*/
void appKymeraUsbAudioStart(uint8 channels, uint8 frame_size,
                            Source src, int16 volume_in_db, bool mute_status,
                            uint32 rate, uint32 min_latency, uint32 max_latency,
                            uint32 target_latency);

/*! \brief Stop and destroy USB Audio chain.
    \param Source USB OUT endpoint source.
    \param kymera_stopped_handler Handler to call when Kymera chain is destroyed.
*/
void appKymeraUsbAudioStop(Source usb_src,
                           void (*kymera_stopped_handler)(Source source));

/*! \brief Set volume for USB Audio chain.
    \param volume_in_db Volume to be set.
*/
void appKymeraUsbAudioSetVolume(int16 volume_in_db);

/*! \brief Create and start USB Voice.
    \param mode Type of mode (NB/WB etc)
    \param spkr_channels number of channels for speaker.
    \param spkr_frame_size number of bits per sample for speaker.
    \param spkr_sample_rate Sample Frequency of Speaker.
    \param mic_sample_rate Sample Frequency of Mic.
    \param spkr_src Speaker Source of USB RX endpoint
    \param mic_sink Sink for mic USB TX endpoint.
    \param volume_in_db Initial volume
    \param min_latency TTP minimum value in micro-seconds
    \param max_latency TTP max value in micro-seconds
    \param target_latency TTP default value in micro-seconds
    \param kymera_stopped_handler Handler to call when Kymera chain is destroyed in the case of chain could not be started.
*/
void appKymeraUsbVoiceStart(usb_voice_mode_t mode, uint8 spkr_channels, uint8 spkr_frame_size,
                            uint32 spkr_sample_rate, uint32 mic_sample_rate, Source spkr_src,
                            Sink mic_sink, int16 volume_in_db, uint32 min_latency, uint32 max_latency,
                            uint32 target_latency, void (*kymera_stopped_handler)(Source source));

/*! \brief Stop and destroy USB Audio chain.
    \param spkr_src USB OUT (from host) endpoint source.
    \param mic_sink USB IN (to host) endpoint sink.
    \param kymera_stopped_handler Handler to call when Kymera chain is destroyed.
*/
void appKymeraUsbVoiceStop(Source spkr_src, Sink mic_sink,
                           void (*kymera_stopped_handler)(Source source));

/*! \brief Set USB Voice volume.

    \param[in] volume_in_db.
 */
void appKymeraUsbVoiceSetVolume(int16 volume_in_db);

/*! \brief Enable or disable MIC muting.

    \param[in] mute TRUE to mute MIC, FALSE to unmute MIC.
*/
void appKymeraUsbVoiceMicMute(bool mute);

/*! \brief Prepare chains required to play prompt.

    \param[in] format The format of the prompt to prepare for
    \param[in] sample_rate The sample rate of the prompt to prepare for

    \return TRUE if chains prepared, otherwise FALSE
 */
bool Kymera_PrepareForPrompt(promptFormat format, uint16 sample_rate);

/*! \brief Check if chains have been prepared for prompt.

    \param[in] format The format of the prompt to play
    \param[in] sample_rate The sample rate of the prompt to play

    \return TRUE if ready for prompt, otherwise FALSE
 */
bool Kymera_IsReadyForPrompt(promptFormat format, uint16 sample_rate);

/*! \brief Get the stream transform connecting the A2DP media source to kymera
    \return The transform, or 0 if the A2DP audio chains are not active.
    \note This function will always return 0 if INCLUDE_MIRRORING is undefined.
*/
Transform Kymera_GetA2dpMediaStreamTransform(void);

void Kymera_SetA2dpOutputParams(a2dp_codec_settings * codec_settings);
void Kymera_ClearA2dpOutputParams(void);
bool Kymera_IsA2dpOutputPresent(void);

#ifndef INCLUDE_MIRRORING
/*! \brief Get the source application target latency value
    \return The target latecy.
*/
unsigned appKymeraGetCurrentLatency(void);

/*! \brief Set the target latency using the VM_TRANSFORM_PACKETISE_TTP_DELAY message.

    \param[in] target_latency - value to use for target latency.
*/
void appKymeraSetTargetLatency(uint16_t target_latency);
#endif

/*! \brief get the status of audio synchronization state
    \return TRUE if audio synchronization is completed, otherwise FALSE
*/
bool Kymera_IsA2dpSynchronisationNotInProgress(void);

/* When A2DP stars it is using eq_bank so select bank in EQ and select corresponding UCID.
   EQ presists its paramters.
   Kymera_SetUserEqBands() call is only required to change paramters,
   it is not required for initial setup of EQ as previously stored values will be used.*/

/* Returns numeber of User EQ bands.
   It doesn't require EQ to be running. */
/* Numebr of bands is #defined in kymera_config.h */
uint8 Kymera_GetNumberOfEqBands(void);

uint8 Kymera_GetNumberOfEqBanks(void);

/* Returns currently selected User EQ bank, applies to presets as well as user bank.
   It doesn't require EQ to be running */
uint8 Kymera_GetSelectedEqBank(void);

bool Kymera_SelectEqBank(uint32 delay_ms, uint8 bank);

/* Set gains for range of bands.
   It automatically switchies to user eq bank if other bank is selected.
   It requires EQ to be running.

   Returns FALSE if EQ is not running*/
bool Kymera_SetUserEqBands(uint32 delay_ms, uint8 start_band, uint8 end_band, int16 * gains);

void Kymera_GetEqBandInformation(uint8 band, kymera_eq_paramter_set_t *param_set);

/* Sends a message containg complete set of user EQ paramters.
   It requires EQ to be running.

   Returns FALSE if EQ is not running*/
bool Kymera_RequestUserEqParams(Task task);

/* Writes selected parts (like EQ bank) of kymera state to a ps key */
void Kymera_PersistState(void);

/* Checks if user eq is active */
uint8 Kymera_UserEqActive(void);

void Kymera_GetEqParams(uint8 band);
bool Kymera_ApplyGains(uint8 start_band, uint8 end_band);

/*! \brief Populate array of available presets

    It will populate presets array with ids of defined presets.
    It will scan audio ps keys to find which presets are defined,
    that takes time.

    Note that this functions ignores preset 'flat' UCID 1 and 'user eq' UCID 63.
    Only presets located between above are taken into account.

    When presets == NULL then it just returns number of presets.

    \param presets Array to be populated. In needs to be big enough to hold all preset ids.

    \return Number of presets defined.
*/
uint8 Kymera_PopulatePresets(uint8 *presets);


/*! \brief Populate all callback configurations for kymera.

    \param callback configs struct.
*/
void Kymera_SetCallbackConfigs(const kymera_callback_configs_t *configs);

/*! \brief Get a pointer to the callback configuration.

    \return callback configs struct.
*/
const kymera_callback_configs_t *Kymera_GetCallbackConfigs(void);

/*! \brief Check if Q2Q mode is enabled.

    \return TRUE if Q2Q mode is enabled.
*/
bool Kymera_IsQ2qModeEnabled(void);

typedef enum
{
    KYMERA_CVC_NOTHING_SET = 0,
    KYMERA_CVC_RECEIVE_FULL_PROCESSING = (1 << 0),
    KYMERA_CVC_RECEIVE_PASSTHROUGH = (1 << 1),
    KYMERA_CVC_SEND_FULL_PROCESSING = (1 << 2),
    KYMERA_CVC_SEND_PASSTHROUGH = (1 << 3),
} kymera_cvc_mode_t;

/*! \brief Sets CVC Send and/or CVC Receive mode to Pass-Through or Full Processing
 *  \param mode full processing or pass-through mode for cvc send and cvc receive operator
 *         passthrough_mic microphone to pass through in case of pass-through mode
 *  \return TRUE when settings have changed
 */
bool Kymera_SetCvcPassthroughMode(kymera_cvc_mode_t mode, uint8 passthrough_mic);

#define Kymera_ScoSetCvcPassthroughMode(mode, passthrough_mic) \
    Kymera_SetCvcPassthroughMode(mode, passthrough_mic)

/*! \brief Reads give operator status data in the sco chain.
 *  \param Sco chain operator roles ( e.g. OPR_SCO_RECEIVE).
 *         Number of parameters in the operator status data.
 *  \return operator status data
 */
get_status_data_t* Kymera_GetOperatorStatusDataInScoChain(unsigned operator_role, uint8 number_of_params);

#ifdef INCLUDE_CVC_DEMO
/*! \brief Gets SCO CVC Send and CVC Receive Passthrough mode. */
void Kymera_ScoGetCvcPassthroughMode(kymera_cvc_mode_t *mode, uint8 *passthrough_mic);

/*! \brief Sets the microphone configuration in 3Mic CVC Send:
 *  \param mic_mode 1: 1-mic config using the omni mode setting
 *                  2: 2-mic config. Use this setting in combination with HW leakthrough
 *                  3: 3-mic config. Normal operation
 *  \return TRUE when settings have changed
 */
bool Kymera_ScoSetCvcSend3MicMicConfig(uint8 mic_mode);

/*! \brief Gets the microphone configuration from 3Mic CVC Send */
void Kymera_ScoGetCvcSend3MicMicConfig(uint8 *mic_config);

/*! \brief Gets the 3Mic CVC Send mode of operation (2Mic or 3Mic mode) */
void Kymera_ScoGetCvcSend3MicModeOfOperation(uint8 *mode_of_operation);

/*! \brief Polls the 3Mic CVC Send mode of operation (2Mic or 3Mic mode) */
void Kymera_ScoPollCvcSend3MicModeOfOperation(void);
#endif /*INCLUDE_CVC_DEMO*/

#if defined(INCLUDE_A2DP_USB_SOURCE) || defined(INCLUDE_A2DP_ANALOG_SOURCE)
/*! \brief Control the flow of media packets to the remote A2DP sink.

    Newly created A2DP Source chains will default to disposing of all media
    packets produced by the encoder, until streaming is explicitly enabled by
    calling this function. This is to allow the start of the media stream to be
    synchronised with the receipt of an AVDTP Start Response from the sink, to
    ensure that we don't begin sending any media packets over the air until our
    AVDTP Start Request has actually been acknowledged and accepted.

    \param enable TRUE to stop disposing packets and start streaming them to the sink.
                  FALSE to stop streaming to the sink and resume disposing of packets.

    \return TRUE if packet streaming was started or stopped as requested, FALSE otherwise.
*/
bool Kymera_A2dpSourceAllowMediaStreaming(bool enable);

/*! \brief Enable/disable automatic RF signal parameter updates to encoder.

    RF signal parameter updates are enabled by default, and are automatically
    sent periodically to the aptX Adaptive encoder once an aptX Adaptive chain
    is started. This function allows overriding that behaviour (i.e. disabling
    automatic updates, then perhaps enabling them again later if desired).

    \param enable FALSE to disable automatic updates, TRUE to enable them again.
*/
void Kymera_AptxAdEncoderEnableRfSignalUpdates(bool enable);

/*! \brief Manually send a single set of RF signal parameters to the encoder.

    Send the specified set of RF signal params to the aptX Adaptive encoder, if
    running. It is recommended to disable automatic updates first, if it is
    desired for the given values to persist (otherwise they will be very quickly
    overwritten). If the encoder isn't currently running, then the params are
    stored and sent as the "initial set" next time the encoder starts.

    \param[in] params Pointer to set of RF parameters to send to the encoder.

    \returns TRUE if params sent to encoder, FALSE if encoder wasn't running
             (params stored for next encoder start, in that case).
*/
bool Kymera_AptxAdEncoderSendRfSignalParams(const aptxad_96K_encoder_rf_signal_params_t *params);

/*! \brief Retrieve the current most recent set of RF signal parameters.

    Fill the provided structure with the current stored set of RF signal params.
    This will either be the latest live values last sent to the encoder, if the
    encoder is running. Or the set of values to be sent next time the encoder
    starts, if it's not currently running.

    \param[out] params Pointer to structure where signal values will be written.
*/
void Kymera_AptxAdEncoderGetRfSignalParams(aptxad_96K_encoder_rf_signal_params_t *params);

/*! \brief Set the desired quality mode for the aptX Adaptive encoder.

    The aptX Adaptive encoder can operate in one of two different quality modes
    depending on user selection, optimising for either Low Latency (LL) or High
    Quality (HQ). This function sets the mode that will be used the next time
    the encoder starts.

    \param quality_mode The new desired aptX Adaptive quality mode.

    \note If the encoder is currently running, then the chain must be stopped
          and restarted before any mode change can take effect. Even then, it
          may not be possible to honour the preference, for example if LL mode
          is requested when the negotiated A2DP sample rate is 96kHz. It is
          therefore recommended to renegotiate the current A2DP sample rate
          immediately after calling this function, before restarting the chain
          (if required), which will avoid the problem entirely.
*/
void Kymera_AptxAdEncoderSetDesiredQualityMode(aptxad_quality_mode_t quality_mode);

/*! \brief Get the desired quality mode for the aptX Adaptive encoder.

    The aptX Adaptive encoder can operate in one of two different quality modes
    depending on user selection, optimising for either Low Latency (LL) or High
    Quality (HQ). This function returns the mode that will be used the next time
    the encoder starts. This may not be the same as the current mode in use by
    the encoder, for example if the mode was changed while the encoder was
    running and it hasn't been restarted yet. It will however always match the
    last value set by Kymera_AptxAdEncoderSetDesiredQualityMode().

    \returns The encoder's current preferred quality mode selection.
*/
aptxad_quality_mode_t Kymera_AptxAdEncoderGetDesiredQualityMode(void);

/*! \brief Get the current quality mode the aptX Adaptive encoder is running in.

    If the aptX Adaptive encoder is currently running, then this function will
    return the actual (live) quality mode in use by the encoder. If not, then it
    will return the mode that will be chosen next time the encoder starts, i.e.
    the same value as Kymera_AptxAdEncoderGetDesiredQualityMode(). These values
    can differ, for example if the mode was changed while the encoder was
    running and it hasn't been restarted yet.

    \returns The current live quality mode in use by the aptX Adaptive encoder,
             if running. If not, then the mode to be used next time it starts.
*/
aptxad_quality_mode_t Kymera_AptxAdEncoderGetActiveQualityMode(void);


#endif /* INCLUDE_A2DP_USB_SOURCE || INCLUDE_A2DP_ANALOG_SOURCE */

/*! \brief Set the USB Audio mute.

    \param[in] mute TRUE to mute USB Audio, FALSE to unmute USB audio.
*/
void appKymeraUsbAudioMute(bool mute);

#ifdef ENABLE_TWM_SPEAKER
/*! \brief Set the Audio type.

    \param[in] audio_type Stereo/mono based on the toggle input of party mode and TWM-Standalone mode.
    \param[in] is_toggle_party_mode TRUE if input is Toggle party mode and FALSE if input is Toggle TWM-Standalone mode.
*/
void Kymera_SetAudioType(appKymeraAudioType audio_type, bool is_toggle_party_mode);
#endif

/*! \brief Callback function pointer for handling LEA media broadcast request based on the audio source routed.
 *         Specific callback function for each audio source(which supports broadcast) has to be registered and
 *         invoked accordingly.

    \param[in] enable TRUE for broadcast enable & FALSE for broadcast disable.
*/
typedef void (*Kymera_LeaMediaBroadcastRequestCallback) (bool enable);

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER)

/*! \brief Register LEA media broadcast request callback which would be invoked when enable/disable broadcast
 * request is obtained.

    \param audio_source LEA media sender audio source.
    \param callback callbak function to be invoked.
*/
void Kymera_RegisterLeaMediaBroadcastRequestCallback(appKymeraLeAudioMediaSenderSourceType audio_source, Kymera_LeaMediaBroadcastRequestCallback callback);

/*! \brief Enable/Disable audio broadcasting.

    \param[in] enable TRUE/FALSE to enable or disable audio broadcasting respectively.
*/
void Kymera_EnableLeaAudioBroadcasting(bool enable);

/*! \brief Set the default LEA broadcast audio config parameters which shall be used in concurrency use-case.

    \param[in] lea_broadcast_params LEA broadcast params to be set.
*/
void Kymera_SetLeaBroadcastParams(le_media_config_t * lea_broadcast_params);

/*! \brief Clear the a2dp LEA broadcast parameters.
*/
void Kymera_ClearLeaBroadcastParams(void);

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE && ENABLE_SIMPLE_SPEAKER */

/*! \brief Update the QHS Level change to Audio SS

    \param[in] qhs_level QHS level change.
*/
void appKymeraUpdateQhsLevel(uint16 qhs_level);

/*!@} */

#endif /* KYMERA_H */
