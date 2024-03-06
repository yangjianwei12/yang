/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle LE mic chain

*/

#ifdef INCLUDE_LE_AUDIO_UNICAST

#include "kymera_le_mic_chain.h"
#include "kymera_le_audio.h"
#include "kymera_dsp_clock.h"
#include "kymera_mic_if.h"
#include "kymera_le_mic_chain.h"
#include "kymera_chain_roles.h"
#include "kymera_config.h"
#include "kymera.h"
#include "kymera_le_common.h"
#include "kymera_setup.h"
#include "kymera_leakthrough.h"
#include "kymera_state.h"
#include <logging.h>

/*! Code assertion that can be checked at run time. This will cause a panic. */
#define assert(x)   PanicFalse(x)

#define STEREO_RECORDING_SAMPLE_RATE   48000

/* @TODO Only 1 mic is supported until CVC is included */
#define MAX_NUM_OF_MICS_SUPPORTED (1)

/*! \brief LE Mic chain information */
typedef struct
{
    /*! The encoder for the LE mic chain */
    appKymeraLeAudioCodec codec_type;
    /* Sink associated with the ISO handle */
    Sink to_air_sink;
    /*! The encoder and mic sample rate for the LE mic chain */
    unsigned sample_rate;
    /*! Does mic chain require CVC such as for gaming mode */
    bool use_cvc;
    /*! Does mic chain require mic sync */
    bool mic_sync;
}kymera_le_mic_chain_data_t;

kymera_le_mic_chain_data_t le_mic_chain_data;

static const appKymeraLeMicChainTable *chain_config_map = NULL;
static kymera_chain_handle_t le_mic_chain = NULL;
#ifdef INCLUDE_LE_STEREO_RECORDING
static kymera_chain_handle_t le_mic_output_chain = NULL;
#endif

static const output_registry_entry_t output_info =
{
    .user = output_user_le_srec,
#ifdef INCLUDE_STEREO
    .connection = output_connection_stereo,
#else
    .connection = output_connection_mono,
#endif
};

static kymera_chain_handle_t kymera_GetLeMicChain(void);
static void kymera_ConfigureEncoder(const le_microphone_config_t *microphone_params);
static void kymera_ConfigureBasicPass(void);
static void kymera_ConfigureRateAdjust(const le_microphone_config_t *microphone_params);
static const chain_config_t * kymera_GetChainConfig(uint16 sample_rate, appKymeraLeAudioCodec encoder_type);
static Sink kymera_GetChainInput(void);

static bool kymeraLeMic_GetConnectionParameters(uint16 *mic_ids, Sink *mic_sinks, uint8 *num_of_mics, uint32 *sample_rate, Sink *aec_ref_sink);
static bool kymeraLeMic_DisconnectIndication(const mic_change_info_t *info);
static mic_user_state_t kymeraLeMic_GetUserState(void);
static bool kymeraLeMic_GetAecRefUsage(void);
static uint32 kymeraLeMic_GetMandatoryTtpDelay(void);
static bool kymeraLeMic_IsMicOnlyCapture(void);
static bool kymeraLeMic_IsStereoMicRecording(void);

static const mic_callbacks_t kymeraLeMic_Callbacks =
{
    .MicGetConnectionParameters = kymeraLeMic_GetConnectionParameters,
    .MicDisconnectIndication = kymeraLeMic_DisconnectIndication,
    .MicGetUserState = kymeraLeMic_GetUserState,
    .MicGetAecRefUsage = kymeraLeMic_GetAecRefUsage,
    .MicGetMandatoryTtpDelay = kymeraLeMic_GetMandatoryTtpDelay,
};

static uint32 kymeraLeMic_ttpDelay = 0;

static const mic_registry_per_user_t kymeraLeMic_Registry =
{
    .user = mic_user_le_mic,
    .callbacks = &kymeraLeMic_Callbacks,
    .permanent.mandatory_mic_ids = NULL,
    .permanent.num_of_mandatory_mics = 0,
};

static void kymera_LeMicPopulateConnectParams(uint16 *mic_ids, Sink *mic_sinks, uint8 num_mics, Sink *aec_ref_sink)
{
    PanicZero(mic_ids);
    PanicZero(mic_sinks);
    PanicZero(aec_ref_sink);
    PanicFalse(num_mics <= MAX_NUM_OF_MICS_SUPPORTED);

    DEBUG_LOG("kymera_LeMicPopulateConnectParams: Number of Mics %d", num_mics);

    /* @TODO only 1 mic is supported until CVC is added to LE mic chain */
    mic_ids[0] = appConfigMicVoice();
    mic_sinks[0] = kymera_GetChainInput();

    if(num_mics > 1)
    {
        mic_ids[1] = MICROPHONE_NONE;
        mic_sinks[1] = NULL;
    }

    if (Kymera_IsVoiceBackChannelEnabled() || kymeraLeMic_IsMicOnlyCapture())
    {
        aec_ref_sink[0] = ChainGetInput(le_mic_chain, EPR_RATE_ADJUST_REF_IN);
    }
    else
    {
        aec_ref_sink[0] = NULL;
    }
}

static bool kymeraLeMic_DisconnectIndication(const mic_change_info_t *info)
{
    UNUSED(info);
    DEBUG_LOG_ERROR("kymeraLeMic_DisconnectIndication: LE Mic shouldn't have to get disconnected");
    Panic();
    return TRUE;
}

static bool kymeraLeMic_GetConnectionParameters(uint16 *mic_ids, Sink *mic_sinks, uint8 *num_of_mics, uint32 *sample_rate, Sink *aec_ref_sink)
{
    DEBUG_LOG("kymeraLeMic_GetConnectionParameters");

    *sample_rate = Kymera_LeMicSampleRate();
    *num_of_mics = 1 ; /*appConfigVoiceGetNumberOfMics(); @TODO only 1 mic is supported for le mic chain */
    assert(*num_of_mics <= MAX_NUM_OF_MICS_SUPPORTED);
    kymera_LeMicPopulateConnectParams(mic_ids, mic_sinks, *num_of_mics, aec_ref_sink);
    return TRUE;
}

static mic_user_state_t kymeraLeMic_GetUserState(void)
{
    return mic_user_state_non_interruptible;
}

static bool kymeraLeMic_GetAecRefUsage(void)
{
    /* Use AEC reference for Voice Back channel and Voice recording */
    if (kymeraLeMic_IsMicOnlyCapture() || Kymera_IsVoiceBackChannelEnabled())
    {
        return TRUE;
    }

    return FALSE;
}

static uint32 kymeraLeMic_GetMandatoryTtpDelay(void)
{
    return kymeraLeMic_ttpDelay;
}

static kymera_chain_handle_t kymera_GetLeMicChain(void)
{
    return le_mic_chain;
}

static const chain_config_t * kymera_GetChainConfig(uint16 sample_rate, appKymeraLeAudioCodec encoder_type)
{
    unsigned index;
    bool is_voice_back_channel = Kymera_IsVoiceBackChannelEnabled();

    /* @TODO The LE mic chain currently only supports 1 mic. Add 2 mic and 3 mic support as well*/
    uint8 mic = 1;

    assert(chain_config_map);

    for (index = 0; index < chain_config_map->table_length; index++)
    {
        if(chain_config_map->chain_table[index].mic_cfg == mic && chain_config_map->chain_table[index].rate == sample_rate && chain_config_map->chain_table[index].codec_type == encoder_type
            && chain_config_map->chain_table[index].is_voice_back_channel == is_voice_back_channel)
        {
            DEBUG_LOG("kymera_GetChainConfig :sample_rate %x, mic_cfg %x, encoder_type %x , is_voice_back_channel %d", sample_rate, mic, encoder_type, is_voice_back_channel);
            return chain_config_map->chain_table[index].chain;
        }
    }

    return NULL;
}

static void kymera_LeMicChainDataInit(const le_microphone_config_t *params, bool use_cvc)
{
    DEBUG_LOG("kymera_LeMicChainDataInit");
    memset(&le_mic_chain_data, 0, sizeof(kymera_le_mic_chain_data_t));
    /* Set the codec type LE Mic chain */
    le_mic_chain_data.codec_type = params->codec_type;
    le_mic_chain_data.sample_rate = params->sample_rate;
    /* Retrieve the sink for the associated ISO handle */
    le_mic_chain_data.to_air_sink = StreamIsoSink(params->source_iso_handle);
    le_mic_chain_data.use_cvc = use_cvc;
    le_mic_chain_data.mic_sync = params->mic_sync;
    DEBUG_LOG("kymera_LeMicChainDataInit: codec_type enum:appKymeraLeAudioCodec: %d, sample_rate %d, sink = 0x%p, is_voice_back_channel = %d, mic_sync : %d",
              le_mic_chain_data.codec_type, le_mic_chain_data.sample_rate, le_mic_chain_data.to_air_sink, le_mic_chain_data.use_cvc, le_mic_chain_data.mic_sync );
}

static void kymera_LeMicChainDataReset(void)
{
    DEBUG_LOG("kymera_LeMicChainDataReset");
    memset(&le_mic_chain_data, 0, sizeof(kymera_le_mic_chain_data_t));

    /* Reset TTP delay value set by client in mic user registry */
    kymeraLeMic_ttpDelay = 0;
}

static void kymera_CreateChain(const le_microphone_config_t *params)
{
    const chain_config_t *chain_config = kymera_GetChainConfig(params->sample_rate,params->codec_type);

    if (kymeraLeMic_IsStereoMicRecording() || kymeraLeMic_IsMicOnlyCapture())
    {
        chain_config = Kymera_GetChainConfigs()->chain_lc3_iso_mic_capture_config_cvc;
#ifdef INCLUDE_LE_STEREO_RECORDING
        /* Mic only capture chain without CVC for Stereo recording and with CVC for Voice Recording */
        chain_config = params->mic_sync != FALSE ?
                       Kymera_GetChainConfigs()->chain_lc3_iso_mic_capture_config :
                       Kymera_GetChainConfigs()->chain_lc3_iso_mic_capture_config_cvc;

        /* Setup an output dummy chain in case of stereo recording for implicit leakthrough ANC */
        const chain_config_t * dummy_output_config = Kymera_GetChainConfigs()->chain_lc3_iso_dummy_output_config;
        le_mic_output_chain = PanicNull(ChainCreate(dummy_output_config));
#endif
    }

    if(chain_config)
    {
        DEBUG_LOG("kymera_CreateChain");
        le_mic_chain = PanicNull(ChainCreate(chain_config));
    }
    else
    {
        DEBUG_LOG("kymera_CreateChain: No compatible LE Mic chain !");
        Panic();
    }
}

static void kymera_ConfigureChain(const le_microphone_config_t *params)
{
    PanicFalse(le_mic_chain != NULL);
    PanicFalse(params != NULL);

    DEBUG_LOG("kymera_ConfigureChain() LE Mic Chain");

    kymera_ConfigureEncoder(params);

    /* Use Basic Pass for stereo recording */
    if (kymeraLeMic_IsStereoMicRecording())
    {
        kymera_ConfigureBasicPass();
    }
    else
    {
        Kymera_SetOperatorUcid(le_mic_chain, OPR_CVC_SEND, UCID_CVC_SEND);
    }

    kymera_ConfigureRateAdjust(params);
}

static Source kymera_GetChainOutput(void)
{
    PanicNull(le_mic_chain);
    return ChainGetOutput(le_mic_chain, EPR_ISO_TO_AIR_LEFT);
}

static Sink kymera_GetChainInput(void)
{
    PanicNull(le_mic_chain);

    if (Kymera_IsVoiceBackChannelEnabled() || kymeraLeMic_IsMicOnlyCapture())
    {
        return ChainGetInput(le_mic_chain, EPR_RATE_ADJUST_IN1);
    }
    
    return ChainGetInput(le_mic_chain, EPR_MIC_MIC1_IN);
}

static void kymera_ConnectChain(void)
{
    /* Get sources and sinks for chain endpoints */
    Source chain_output = kymera_GetChainOutput();

    DEBUG_LOG("kymera_ConnectChain() LE Mic Chain");
    /* Connect Mic chain output to to air sink */
    StreamConnect(chain_output, le_mic_chain_data.to_air_sink );
    ChainConnect(le_mic_chain);
}

static void kymera_ConfigureEncoder(const le_microphone_config_t *microphone_params)
{
    DEBUG_LOG("kymera_ConfigureEncoder() LE Mic Chain: Codec type %d", microphone_params->codec_type);

    switch (microphone_params->codec_type)
    {
        case KYMERA_LE_AUDIO_CODEC_APTX_LITE:
        {
            Operator op = ChainGetOperatorByRole(le_mic_chain, OPR_APTX_LITE_ENCODE_SCO_ISO);
            OperatorsAptxLiteEncoderScoIsoSetEncodingParams(op, microphone_params->sample_rate, APTX_LITE_ISO_CHANNEL_MODE_MONO,
                                                       microphone_params->frame_duration);
        }
        break;

        default:
        case KYMERA_LE_AUDIO_CODEC_LC3:
        {
            Operator op = ChainGetOperatorByRole(le_mic_chain, OPR_LC3_ENCODE_SCO_ISO);
            
            OperatorsLc3EncoderScoIsoSetPacketLength(op, microphone_params->frame_length);
            OperatorsStandardSetSampleRate(op, microphone_params->sample_rate);
            OperatorsLc3EncoderScoIsoSetFrameDuration(op,microphone_params->frame_duration);
            OperatorsLc3EncoderScoIsoSetBlocksPerSdu(op,microphone_params->codec_frame_blocks_per_sdu);
            if (!Kymera_IsVoiceBackChannelEnabled())
            {
                OperatorsLc3EncoderScoIsoSetErrorResilience(op, lc3_epc_low);
            }
        }
        break;
            
    }
}

static void kymera_CreateOutputPath(uint32 sample_rate, bool mic_sync)
{
#ifndef INCLUDE_STEREO
    /* Create an appropriate Output chain for leakthrough path */
    kymera_output_chain_config output_config;
    KymeraOutput_SetDefaultOutputChainConfig(&output_config, sample_rate, KICK_PERIOD_VOICE, 0);
    output_config.chain_include_aec = TRUE;

    if (!Kymera_IsVoiceBackChannelEnabled())
    {
#ifdef INCLUDE_LE_STEREO_RECORDING
        PanicFalse(Kymera_OutputPrepare(output_user_le_srec, &output_config));
        /* Connect dummy output chain in case of stereo recording for implicit ANC leakthrough */
        output_source_t dummy_source = {0};
        dummy_source.mono = ChainGetOutput(le_mic_output_chain, EPR_SOURCE_ENCODE_OUT);
        PanicFalse(Kymera_OutputConnect(output_user_le_srec, &dummy_source));
        if (mic_sync)
        {
            /* Start Mic chain muted until both MICs are syncronized */
            KymeraLeAudioVoice_SetMicMuteState(TRUE);
        }
#else
        UNUSED(mic_sync);
#endif
    }
    KymeraOutput_ChainStart();
    Kymera_LeakthroughSetAecUseCase(aec_usecase_enable_leakthrough);
#else
    UNUSED(sample_rate);
    UNUSED(mic_sync);
#endif
}

static void kymera_ConfigureBasicPass(void)
{
    Operator op = ChainGetOperatorByRole(le_mic_chain, OPR_BASIC_PASS);

    OperatorsStandardSetUCID(op, UCID_PASS_LE);
    OperatorsSetPassthroughDataFormat(op, operator_data_format_pcm);
}

static void kymera_ConfigureRateAdjust(const le_microphone_config_t *microphone_params)
{
    Operator op = ChainGetOperatorByRole(le_mic_chain, OPR_RATE_ADJUST);
    Sink voice_sink = StreamIsoSink(microphone_params->source_iso_handle);

    OperatorsStandardSetSampleRate(op, microphone_params->sample_rate);
    SinkConfigure(voice_sink, STREAM_RM_USE_RATE_ADJUST_OPERATOR, op);
}

static void kymera_DisconnectChain(void)
{
    Kymera_MicDisconnect(mic_user_le_mic);
    StreamDisconnect(kymera_GetChainOutput(), NULL);
    /* Reset the AEC ref usecase*/
    Kymera_SetAecUseCase(aec_usecase_default);
}

static void kymera_DestroyChain(void)
{
    PanicNull(le_mic_chain);
    DEBUG_LOG("kymera_DestroyChain, LE mic chain");
    kymera_DisconnectChain();
    kymera_LeMicChainDataReset();
    ChainDestroy(le_mic_chain);
#ifdef INCLUDE_LE_STEREO_RECORDING
    ChainDestroy(le_mic_output_chain);
    le_mic_output_chain = NULL;
#endif
    le_mic_chain = NULL;
}

static void kymera_AdjustTtpDelay(const le_microphone_config_t *microphone_params, bool is_voice_back_channel)
{
    /* CVC algorithmic delay compensation for Gaming mode VBC */
    uint32 reduction_us = (is_voice_back_channel) ? LE_AUDIO_WB_CVC_ALGORITHMIC_DELAY_US : 0ul;

    if (microphone_params->presentation_delay >= reduction_us)
    {
        kymeraLeMic_ttpDelay = microphone_params->presentation_delay - reduction_us;
    }
    else
    {
        kymeraLeMic_ttpDelay = 0;
    }
}

Operator Kymera_GetLeMicCvcSend(void)
{
    if (appKymeraGetState() != KYMERA_STATE_LE_AUDIO_ACTIVE)
    {
        return INVALID_OPERATOR;
    }

    return ChainGetOperatorByRole(kymera_GetLeMicChain(), OPR_CVC_SEND);
}

unsigned Kymera_LeMicSampleRate(void)
{
    return le_mic_chain_data.sample_rate;
}

void Kymera_SetLeMicChainTable(const appKymeraLeMicChainTable *chain_table)
{
    chain_config_map = chain_table;
}

void Kymera_CreateLeMicChain(const le_microphone_config_t *microphone_params, bool use_cvc)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    PanicFalse(microphone_params != NULL);
    /* Intiialize LE Mic chain data */
    kymera_LeMicChainDataInit(microphone_params, use_cvc);
    /* Create LE mic chain */
    kymera_CreateChain(microphone_params);

    kymera_AdjustTtpDelay(microphone_params, use_cvc);

    /* Connect to Mic interface */
    if (!Kymera_MicConnect(mic_user_le_mic))
    {
        DEBUG_LOG_ERROR("Kymera_CreateLeMicChain: Mic connection was not successful. LE Voice should always be prepared.");
        Panic();
    }

    /* Configuring LE mic chain */
    kymera_ConfigureChain(microphone_params);

    if (theKymera->chain_config_callbacks && theKymera->chain_config_callbacks->ConfigureLeAudioToAirChain)
    {
        kymera_le_audio_to_air_config_params_t params = {0};
        params.codec_type = microphone_params->codec_type;
        params.sample_rate = microphone_params->sample_rate;
        params.frame_length = microphone_params->frame_length;
        params.frame_duration_us = microphone_params->frame_duration;
        params.presentation_delay_us = microphone_params->presentation_delay;
        theKymera->chain_config_callbacks->ConfigureLeAudioToAirChain(le_mic_chain, &params);
    }

    kymera_ConnectChain();

    if (!use_cvc)
    {
        /* Configure speaker path for leakthrough only for stereo recording mode */
        kymera_CreateOutputPath(microphone_params->sample_rate, microphone_params->mic_sync);
    }
    else
    {
#ifndef FORCE_CVC_PASSTHROUGH
        if(theKymera->enable_cvc_passthrough)
#endif
        {
            Kymera_SetCvcPassthroughMode(KYMERA_CVC_SEND_PASSTHROUGH, 0);
        }

        if (microphone_params->mic_sync)
        {
            /* Start Mic chain muted until both MICs are syncronized for Gaming mode VBC */
            KymeraLeAudioVoice_SetMicMuteState(TRUE);
        }
    }
}

void Kymera_StartLeMicChain(void)
{
    appKymeraConfigureDspPowerMode();

    /* The chain can fail to start if the ISO source disconnects whilst kymera
       is queuing the ISO start request or starting the chain. If the attempt
       fails, ChainStartAttempt will stop (but not destroy) any operators it
       started in the chain. */
    if (!ChainStartAttempt(le_mic_chain))
    {
        DEBUG_LOG("Kymera_StartLeMicChain, could not start chain");
        /* Stop/destroy the chain, returning state to KYMERA_STATE_IDLE.
           This needs to be done here, since between the failed attempt to start
           and the subsequent stop (when kymeraLeAudio_Stop() is called), a tone
           may need to be played - it would not be possible to play a tone in a
           stopped ISO chain. The state needs to be KYMERA_STATE_LE_AUDIO_ACTIVE for
           kymeraLeAudio_Stop() to stop/destroy the chain. */
        kymeraLeAudio_Stop();
    }
}

void Kymera_StopLeMicChain(void)
{
    kymera_chain_handle_t mic_chain = kymera_GetLeMicChain();

    if(!mic_chain)
    {
        DEBUG_LOG("Kymera_StopLeMicChain, not stopping - already idle/uncreated");
        return;
    }
    /* Disable AEC_REF sidetone path */
    Kymera_LeakthroughEnableSidetonePath(FALSE);

    /* Stop Mic chain */
    ChainStop(mic_chain);
    kymera_DestroyChain();
}

void Kymera_LeMicChainInit(void)
{
    Kymera_OutputRegister(&output_info);
    Kymera_MicRegisterUser(&kymeraLeMic_Registry);
}

void Kymera_LeMicDisconnectOutputChain(void)
{
    /* Disconnect output chain if voice back channel is disabled */
    if (!Kymera_IsVoiceBackChannelEnabled())
    {
        Kymera_OutputDisconnect(output_user_le_srec);
    }
}

kymera_chain_handle_t Kymera_LeAudioGetCvcChain(void)
{
    return le_mic_chain;
}

bool Kymera_IsVoiceBackChannelEnabled(void)
{
    return le_mic_chain_data.use_cvc;
}

static bool kymeraLeMic_IsStereoMicRecording(void)
{
    return (le_mic_chain_data.use_cvc == FALSE && le_mic_chain_data.mic_sync == TRUE);
}

static bool kymeraLeMic_IsMicOnlyCapture(void)
{
    return (le_mic_chain_data.use_cvc == FALSE && le_mic_chain_data.mic_sync == FALSE);
}

bool Kymera_LeMicIsCodecTypeAptxLite(void)
{
    return (le_mic_chain_data.codec_type == KYMERA_LE_AUDIO_CODEC_APTX_LITE);
}

#endif
