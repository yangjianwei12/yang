/*!
\copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera LE Voice chain handling
*/

#include "kymera_le_voice.h"
#include "kymera_state.h"
#include "kymera_config.h"
#include "kymera_dsp_clock.h"
#include "kymera_tones_prompts.h"
#include "kymera_aec.h"
#include "kymera_mic_if.h"
#include "kymera_output_if.h"
#include "kymera_data.h"
#include "kymera_leakthrough.h"
#include "kymera_le_common.h"
#include "kymera_le_mic_chain.h"
#include "av.h"
#include <vmal.h>
#include <anc_state_manager.h>
#include <chain.h>

#ifdef INCLUDE_LE_AUDIO_UNICAST

/*! Code assertion that can be checked at run time. This will cause a panic. */
#define assert(x)   PanicFalse(x)

/*
    Parameters for LE voice for the initial testing.
*/
#define LC3_DECODE_SCO_ISO_PACKET_LEN   40
#define LC3_DECODE_SCO_ISO_BUFFER_SIZE  640

#define LC3_ENCODE_SCO_ISO_PACKET_LEN   40

#ifdef INCLUDE_STEREO
#define MAX_NUM_OF_LE_VOICE_MICS_SUPPORTED (1)
#else
#define MAX_NUM_OF_LE_VOICE_MICS_SUPPORTED (2)
#endif

#define DECODE_ONLY_LEFT   0
#define DECODE_ONLY_RIGHT  1

/*! List of possible audio paths for LE voice call. */
typedef enum
{
    le_voice_mic_only_path,
    le_voice_speaker_only_path,
    le_voice_mic_speaker_path,
} le_voice_path_t;

static unsigned le_voice_sample_rate;

static bool use_split_stereo_chain;

static const output_registry_entry_t output_info_mono =
{
    .user = output_user_le_voice_mono,
    .connection = output_connection_mono,
};

static const output_registry_entry_t output_info_stereo =
{
    .user = output_user_le_voice_stereo,
    .connection = output_connection_stereo,
};

static const appKymeraLeVoiceChainTable *chain_config_map = NULL;
static kymera_chain_handle_t le_voice_chain = NULL;

static inline kymera_chain_handle_t KymeraLeVoice_GetChain(void)
{
    return le_voice_chain;
}

static uint8 kymeraLeVoice_getNumberOfLeVoiceMics(void);
static uint32 kymeraLeVoice_getTtpLatency(uint32 ttp_latency_us, uint16 sample_rate);
static bool kymeraLeVoice_MicGetConnectionParameters(uint16 *mic_ids, Sink *mic_sinks, uint8 *num_of_mics, uint32 *sample_rate, Sink *aec_ref_sink);
static bool kymeraLeVoice_MicDisconnectIndication(const mic_change_info_t *info);
static mic_user_state_t kymeraLeVoice_GetUserState(void);
static bool kymeraLeVoice_GetAecRefUsage(void);
static uint32 kymeraLeVoice_GetMandatoryTtpDelay(void);

static const mic_callbacks_t kymeraLeVoice_Callbacks =
{
    .MicGetConnectionParameters = kymeraLeVoice_MicGetConnectionParameters,
    .MicDisconnectIndication = kymeraLeVoice_MicDisconnectIndication,
    .MicGetUserState = kymeraLeVoice_GetUserState,
    .MicGetAecRefUsage = kymeraLeVoice_GetAecRefUsage,
    .MicGetMandatoryTtpDelay = kymeraLeVoice_GetMandatoryTtpDelay,
};

static uint32 kymeraLeVoice_ttpDelay = 0;

static const mic_registry_per_user_t kymeraLeVoice_MicRegistry =
{
    .user = mic_user_le_voice,
    .callbacks = &kymeraLeVoice_Callbacks,
    .permanent.mandatory_mic_ids = NULL,
    .permanent.num_of_mandatory_mics = 0,
};

static void kymeraLeVoice_SetTtpDelay(const le_microphone_config_t *le_mic_params);
static uint8 kymeraLeVoice_getNumberOfLeVoiceMics(void)
{
    uint8 num_of_le_voice_mics = appConfigVoiceGetNumberOfMics();

    if (num_of_le_voice_mics > MAX_NUM_OF_LE_VOICE_MICS_SUPPORTED)
    {
        num_of_le_voice_mics = MAX_NUM_OF_LE_VOICE_MICS_SUPPORTED;
    }

    return num_of_le_voice_mics;
}

static void kymeraLeVoice_SetTtpDelay(const le_microphone_config_t *le_mic_params)
{
    kymeraLeVoice_ttpDelay = kymeraLeVoice_getTtpLatency(le_mic_params->presentation_delay,
                                                         le_mic_params->sample_rate);
}

static void KymeraLeVoice_SetSampleRate(unsigned sample_rate)
{
    le_voice_sample_rate = sample_rate;
}

static unsigned KymeraLeVoice_GetSampleRate(void)
{
    return le_voice_sample_rate;
}

static void kymeraLeVoice_PopulateConnectParams(uint16 *mic_ids, Sink *mic_sinks, uint8 num_mics, Sink *aec_ref_sink)
{
    kymera_chain_handle_t chain = le_voice_chain;
    PanicZero(mic_ids);
    PanicZero(mic_sinks);
    PanicZero(aec_ref_sink);
    PanicFalse(num_mics <= MAX_NUM_OF_LE_VOICE_MICS_SUPPORTED);

    mic_ids[0] = appConfigMicVoice();
    mic_sinks[0] = ChainGetInput(chain, EPR_RATE_ADJUST_IN1);
    if(num_mics > 1)
    {
        mic_ids[1] = appConfigMicExternal();
        mic_sinks[1] = ChainGetInput(chain, EPR_RATE_ADJUST_IN2);
    }
    if(num_mics > 2)
    {
#if defined(INCLUDE_BCM)
        mic_ids[2] = appConfigMicInternalBCM();
#elif defined(INCLUDE_LIS25BA_ACCELEROMETER)
        mic_ids[2] = appConfigMicInternalPCM();
#else
        mic_ids[2] = appConfigMicInternal();
#endif
        mic_sinks[2] = ChainGetInput(chain, EPR_RATE_ADJUST_IN3);
    }
    aec_ref_sink[0] = ChainGetInput(chain, EPR_RATE_ADJUST_REF_IN);
}

static bool kymeraLeVoice_MicDisconnectIndication(const mic_change_info_t *info)
{
    UNUSED(info);
    DEBUG_LOG_ERROR("kymeraLeVoice_MicDisconnectIndication: LE Voice shouldn't have to get disconnected");
    Panic();
    return TRUE;
}

static bool kymeraLeVoice_MicGetConnectionParameters(uint16 *mic_ids, Sink *mic_sinks, uint8 *num_of_mics, uint32 *sample_rate, Sink *aec_ref_sink)
{
    DEBUG_LOG("kymeraLeVoice_MicGetConnectionParameters");

    *sample_rate = KymeraLeVoice_GetSampleRate();
    *num_of_mics = kymeraLeVoice_getNumberOfLeVoiceMics();
    assert(*num_of_mics <= MAX_NUM_OF_LE_VOICE_MICS_SUPPORTED);
    kymeraLeVoice_PopulateConnectParams(mic_ids, mic_sinks, *num_of_mics, aec_ref_sink);
    return TRUE;
}

static mic_user_state_t kymeraLeVoice_GetUserState(void)
{
    return mic_user_state_non_interruptible;
}

static bool kymeraLeVoice_GetAecRefUsage(void)
{
    return TRUE;
}

static uint32 kymeraLeVoice_GetMandatoryTtpDelay(void)
{
    return kymeraLeVoice_ttpDelay;
}

void Kymera_SetLeVoiceChainTable(const appKymeraLeVoiceChainTable *chain_table)
{
    chain_config_map = chain_table;
}

static kymera_chain_handle_t KymeraLeVoice_CreateChain(const chain_config_t *chain_config)
{
    DEBUG_LOG("KymeraLeVoice_CreateChain");

    /* Create chain and return handle */
    PanicNotNull(le_voice_chain);
    le_voice_chain = ChainCreate(chain_config);

    /* Configure DSP power mode appropriately for LE voice chain */
    appKymeraConfigureDspPowerMode();

    return le_voice_chain;
}

/*! \brief Adjust TTP_LATENCY for CVC_RECEIVE/CVC_SEND algorithmic delay
 *
 * \param ttp_latency_us TTP latency requested, in microseconds
 * \param sample_rate Sample rate of the LE voice chain
 *
 * \return adjusted TTP latency rate based on sampling frequency.
*/
static uint32 kymeraLeVoice_getTtpLatency(uint32 ttp_latency_us, uint16 sample_rate)
{
    /* cVc's frame size for 24/48kHz (UWB/FB) is 5ms and 16/32kHz (WB/SWB) is 7.5ms */
    uint32 reduction_us = sample_rate == 24000 || sample_rate == 48000 ? LE_AUDIO_UWB_CVC_ALGORITHMIC_DELAY_US : LE_AUDIO_WB_CVC_ALGORITHMIC_DELAY_US;

    if (ttp_latency_us >= reduction_us)
    {
        ttp_latency_us -= reduction_us;
    }

    return ttp_latency_us;
}

static void kymeraLeVoice_ConfigureLc3Decoder(Operator op, const le_speaker_config_t *le_speaker_params)
{
    DEBUG_LOG(" kymeraLeVoice_ConfigureLc3Decoder enum:appKymeraLeStreamType:%d, blocks per sdu %d", le_speaker_params->stream_type, le_speaker_params->codec_frame_blocks_per_sdu);

    OperatorsLc3DecoderScoIsoSetFrameDuration(op, le_speaker_params->frame_duration);
    OperatorsStandardSetBufferSize(op, LC3_DECODE_SCO_ISO_BUFFER_SIZE);

    OperatorsLc3DecoderScoIsoSetPacketLength(op, le_speaker_params->frame_length);
    OperatorsStandardSetSampleRate(op, le_speaker_params->sample_rate);
    OperatorsStandardSetTimeToPlayLatency(op, le_speaker_params->presentation_delay);

    if (le_speaker_params->codec_frame_blocks_per_sdu > 1)
    {
        OperatorsLc3DecoderScoIsoSetBlocksPerSdu(op, le_speaker_params->codec_frame_blocks_per_sdu);
    }

    switch (le_speaker_params->stream_type)
    {
        case KYMERA_LE_STREAM_MONO:
        case KYMERA_LE_STREAM_DUAL_MONO:
            /* No action */
            break;

        case KYMERA_LE_STREAM_STEREO_USE_RIGHT:
            {
                /* In case of Stereo stream render only right data */
                OperatorsLc3DecoderScoIsoSetNumOfChannels(op, 2);
                OperatorsLc3DecoderScoIsoSetMonoDecode(op, DECODE_ONLY_RIGHT);
            }
            break;

        case KYMERA_LE_STREAM_STEREO_USE_LEFT:
        case KYMERA_LE_STREAM_STEREO_USE_BOTH:
            {
                /* In case of Stereo stream render only left data as in the case of SCO */
                OperatorsLc3DecoderScoIsoSetNumOfChannels(op, 2);
                OperatorsLc3DecoderScoIsoSetMonoDecode(op, DECODE_ONLY_LEFT);
            }
            break;

        default:
            break;
    }
}

static void kymeraLeVoice_ConfigureDecoder(kymera_chain_handle_t chain_handle, const le_speaker_config_t *le_speaker_params)
{
    Operator op = ChainGetOperatorByRole(chain_handle, OPR_LC3_DECODE_SCO_ISO);

    kymeraLeVoice_ConfigureLc3Decoder(op, le_speaker_params);

    if(le_speaker_params->iso_handle_right != LE_AUDIO_INVALID_ISO_HANDLE)
    {
        op = ChainGetOperatorByRole(chain_handle, OPR_LC3_DECODE_SCO_ISO_RIGHT);
        kymeraLeVoice_ConfigureLc3Decoder(op, le_speaker_params);
    }
}

static void kymeraLeVoice_ConfigureEncoder(kymera_chain_handle_t chain_handle, const le_microphone_config_t *le_mic_params)
{
    Operator op = ChainGetOperatorByRole(chain_handle, OPR_LC3_ENCODE_SCO_ISO);
    OperatorsLc3EncoderScoIsoSetPacketLength(op, le_mic_params->frame_length);
    OperatorsStandardSetSampleRate(op, le_mic_params->sample_rate);
    OperatorsLc3EncoderScoIsoSetFrameDuration(op,le_mic_params->frame_duration);

    if (le_mic_params->codec_version == 1)
    {
        OperatorsLc3EncoderScoIsoSetErrorResilience(op, lc3_epc_low);
    }
}

/*! \brief configure the LC3 encoder and decoder operators in Voice chain */
static void kymeraLeVoice_ConfigureChain(const KYMERA_INTERNAL_LE_VOICE_START_T *start_params, le_voice_path_t voice_path)
{
    kymera_chain_handle_t chain = le_voice_chain;
    Operator op = ChainGetOperatorByRole(chain, OPR_RATE_ADJUST);

    PanicNull((void *)chain);

    DEBUG_LOG("kymeraLeVoice_ConfigureChain voice path : enum:le_voice_path_t:%d", voice_path);

    /* Configure LC3 DECODER */
    kymeraLeVoice_ConfigureDecoder(chain, &start_params->speaker_params);

    if (voice_path != le_voice_speaker_only_path)
    {
        /* Configure LC3 ENCODER */
        kymeraLeVoice_ConfigureEncoder(chain, &start_params->microphone_params);
        OperatorsStandardSetSampleRate(op, start_params->microphone_params.sample_rate);
    }

    /* Configure CVC RECEIVE
        full_processing = 0x2
        pass_through_left = 0x04
    */

    /* Configure CVC SEND
        full_processing = 0x2
        pass_through = 0x3
    */

    Kymera_SetVoiceUcids(chain);
}

static void kymeraLeVoice_ConfigureEndpoints(Sink voice_sink)
{
    kymera_chain_handle_t chain = le_voice_chain;
    PanicNull((void *)chain);

    Operator op = ChainGetOperatorByRole(chain, OPR_RATE_ADJUST);
    DEBUG_LOG("kymeraLeVoice_ConfigureEndpoints op_id=%d sink=%d", op, voice_sink);
    SinkConfigure(voice_sink, STREAM_RM_USE_RATE_ADJUST_OPERATOR, op);
}

static const chain_config_t *kymeraLeVoice_FindChainFromConfigTable(const appKymeraLeVoiceChainTable *chain_config, uint16 sample_rate, le_voice_path_t voice_path)
{
    unsigned index;
    uint8 mic = (voice_path == le_voice_speaker_only_path) ? 0: kymeraLeVoice_getNumberOfLeVoiceMics();

    assert(chain_config);

    DEBUG_LOG("kymeraLeVoice_FindChainFromConfigTable chain_table %p chain_length %u, voice_path %d",
              chain_config->chain_table, chain_config->table_length, voice_path);

    for (index = 0; index < chain_config->table_length; index++)
    {
        /* Choose matching chain from voice chain table, however in the absence of microphone path we use non CVC
           lc3 decoder chain to render speaker path disregarding sample rate matching */
        if(chain_config->chain_table[index].is_stereo_config == use_split_stereo_chain && 
            chain_config->chain_table[index].mic_count == mic && (mic == 0 || chain_config->chain_table[index].sample_rate == sample_rate))
        {
            return chain_config->chain_table[index].chain_config;
        }
    }

    DEBUG_LOG("kymeraLeVoice_FindChainFromConfigTable: No compatible chain configuration found!");
    Panic();

    return NULL;
}

static le_voice_path_t kymeraLeVoice_GetVoicePath(const KYMERA_INTERNAL_LE_VOICE_START_T * start_params)
{
    le_voice_path_t voice_path;

    if (start_params->microphone_present && start_params->speaker_present)
    {
        voice_path = le_voice_mic_speaker_path;
    }
    else if (start_params->microphone_present)
    {
        voice_path = le_voice_mic_only_path;
    }
    else
    {
        voice_path = le_voice_speaker_only_path;
    }

    DEBUG_LOG("kymeraLeVoice_GetVoicePath: enum:le_voice_path_t:%d",voice_path);

    return voice_path;
}

static void kymeraLeVoice_SetupToAirChain(const KYMERA_INTERNAL_LE_VOICE_START_T *start_params, le_voice_path_t voice_path)
{
    DEBUG_LOG("kymeraLeVoice_SetupToAirChain: codec_type %d, codec version %d, frame_duration %d, sample_rate %d, frame_length %d, presentation_delay %d",
             start_params->microphone_params.codec_type, start_params->microphone_params.codec_version, start_params->microphone_params.frame_duration, start_params->microphone_params.sample_rate,
             start_params->microphone_params.frame_length, start_params->microphone_params.presentation_delay);

    if (voice_path == le_voice_mic_only_path)
    {
        Kymera_CreateLeMicChain(&start_params->microphone_params, FALSE);
    }
}

static void kymeraLeVoice_SetupFromAirChain(const KYMERA_INTERNAL_LE_VOICE_START_T *start_params, le_voice_path_t voice_path)
{
    const chain_config_t *chain_config = NULL;
    kymeraTaskData *theKymera = KymeraGetTaskData();

    if (voice_path == le_voice_mic_only_path)
    {
        return;
    }

    use_split_stereo_chain = (start_params->speaker_params.iso_handle_right != LE_AUDIO_INVALID_ISO_HANDLE);
    chain_config = kymeraLeVoice_FindChainFromConfigTable(chain_config_map, start_params->speaker_params.sample_rate , voice_path);

    DEBUG_LOG("kymeraLeVoice_SetupFromAirChain, single audio location/mono/stereo ISO 0x%x, right ISO 0x%x, sink 0x%x, vol %d, enum:appKymeraState:%d",
              start_params->speaker_params.iso_handle, start_params->speaker_params.iso_handle_right, start_params->microphone_params.source_iso_handle, start_params->volume_in_db, appKymeraGetState());

    DEBUG_LOG("codec_type %d, codec version %d, frame_duration %d, sample_rate %d, frame_length %d, presentation_delay %d",
             start_params->speaker_params.codec_type, start_params->speaker_params.codec_version, start_params->speaker_params.frame_duration, start_params->speaker_params.sample_rate,
             start_params->speaker_params.frame_length, start_params->speaker_params.presentation_delay);

    /* Create appropriate LE Voice chain */
    PanicNull(KymeraLeVoice_CreateChain(chain_config));

    if (voice_path != le_voice_speaker_only_path)
    {
        /* Connect to Mic interface */
        if (!Kymera_MicConnect(mic_user_le_voice))
        {
            DEBUG_LOG_ERROR("KymeraLeVoice_HandleInternalStart: Mic connection was not successful. LE Voice should always be prepared.");
            Panic();
        }
    }

    /* Configure LC3 Encoder and Decoder */
    kymeraLeVoice_ConfigureChain(start_params, voice_path);

    /* Select output user depending on CIS configuration */
    output_users_t output_user_le_voice = use_split_stereo_chain ? output_user_le_voice_stereo : output_user_le_voice_mono;

    /* Create an appropriate Output chain */
    kymera_output_chain_config output_config;
    KymeraOutput_SetDefaultOutputChainConfig(&output_config, start_params->speaker_params.sample_rate, KICK_PERIOD_VOICE, 0);

    output_config.chain_type = use_split_stereo_chain? output_chain_stereo : output_chain_mono;
    output_config.chain_include_aec = TRUE;
    PanicFalse(Kymera_OutputPrepare(output_user_le_voice, &output_config));

    /* Get sources and sinks for chain endpoints */
    Source to_air_ep = ChainGetOutput(le_voice_chain, EPR_ISO_TO_AIR_LEFT);
    Sink from_air_ep_left = ChainGetInput(le_voice_chain, EPR_ISO_FROM_AIR_LEFT);

    /* Connect ISO to chain ISO endpoints */
    Sink to_air_iso_sink = StreamIsoSink(start_params->microphone_params.source_iso_handle);
    Source from_air_iso_source = StreamIsoSource(start_params->speaker_params.iso_handle);

    if (voice_path != le_voice_speaker_only_path)
    {
        kymeraLeVoice_ConfigureEndpoints(to_air_iso_sink);
        StreamConnect(to_air_ep, to_air_iso_sink);
    }

    StreamConnect(from_air_iso_source, from_air_ep_left);

    if (use_split_stereo_chain)
    {
        Sink from_air_ep_right = ChainGetInput(le_voice_chain, EPR_ISO_FROM_AIR_RIGHT);
        Source from_air_iso_source_right = StreamIsoSource(start_params->speaker_params.iso_handle_right);
        StreamConnect(from_air_iso_source_right, from_air_ep_right);
    }

    if (theKymera->chain_config_callbacks && theKymera->chain_config_callbacks->ConfigureLeVoiceChain && voice_path == le_voice_mic_speaker_path)
    {
        kymera_le_voice_config_params_t params = {0};
        params.codec_type = start_params->speaker_params.codec_type;
        params.sample_rate = start_params->speaker_params.sample_rate;
        params.frame_length = start_params->speaker_params.frame_length;
        params.frame_duration_us = start_params->speaker_params.frame_duration;
        params.presentation_delay_us = start_params->speaker_params.presentation_delay;
        theKymera->chain_config_callbacks->ConfigureLeVoiceChain(le_voice_chain, &params);
    }

    /* Connect chain */
    ChainConnect(le_voice_chain);
}

static void kymeraLeVoice_StartFromAirChain(const KYMERA_INTERNAL_LE_VOICE_START_T *start_params, le_voice_path_t voice_path)
{
    kymera_chain_handle_t chain = NULL;
    /* Select output user depending on CIS configuration */
    output_users_t output_user_le_voice = use_split_stereo_chain ? output_user_le_voice_stereo : output_user_le_voice_mono;
    /* Connect to the Output chain */
    output_source_t sources;

    if (voice_path == le_voice_mic_only_path)
    {
        return;
    }

    DEBUG_LOG("kymeraLeVoice_StartFromAirChain");

    chain = PanicNull(le_voice_chain);

    if (use_split_stereo_chain)
    {
        sources.stereo.left = ChainGetOutput(chain, EPR_SCO_SPEAKER);
        sources.stereo.right = ChainGetOutput(chain, EPR_SCO_SPEAKER_RIGHT);
    }
    else
    {
        sources.mono = ChainGetOutput(chain, EPR_SCO_SPEAKER);
    }

    PanicFalse(Kymera_OutputConnect(output_user_le_voice, &sources));
    KymeraOutput_ChainStart();

    /* The chain can fail to start if the ISO source disconnects whilst kymera
       is queuing the ISO start request or starting the chain. If the attempt
       fails, ChainStartAttempt will stop (but not destroy) any operators it
       started in the chain. */
    if (ChainStartAttempt(chain))
    {
        KymeraLeVoice_HandleInternalSetVolume(start_params->volume_in_db);
        KymeraLeAudioVoice_SetMicMuteState((bool)start_params->microphone_params.mic_mute_state);
#ifndef FORCE_CVC_PASSTHROUGH
        kymeraTaskData *theKymera = KymeraGetTaskData();
        if(theKymera->enable_cvc_passthrough)
#endif
        {
            Kymera_SetCvcPassthroughMode(KYMERA_CVC_RECEIVE_PASSTHROUGH | KYMERA_CVC_SEND_PASSTHROUGH, 0);
        }

        if(AecLeakthrough_IsLeakthroughEnabled())
        {
           Kymera_LeakthroughSetAecUseCase(aec_usecase_enable_leakthrough);
        }
    }
    else
    {
        DEBUG_LOG("KymeraLeVoice_HandleInternalStart, could not start chain");
        /* Stop/destroy the chain, returning state to KYMERA_STATE_IDLE.
           This needs to be done here, since between the failed attempt to start
           and the subsequent stop (when appKymeraScoStop() is called), a tone
           may need to be played - it would not be possible to play a tone in a
           stopped SCO chain. The state needs to be KYMERA_STATE_LE_VOICE_ACTIVE for
           KymeraLeVoice_HandleInternalStop() to stop/destroy the chain. */
        KymeraLeVoice_HandleInternalStop();
    }
}

static void kymeraLeVoice_StartToAirChain(le_voice_path_t voice_path)
{
    if (voice_path == le_voice_mic_only_path)
    {
        DEBUG_LOG("kymeraLeVoice_StartToAirChain");
        Kymera_StartLeMicChain();
        /* Ensure to unmute the mice */
        KymeraLeAudioVoice_SetMicMuteState(FALSE);
    }
}

void KymeraLeVoice_HandleInternalStart(const KYMERA_INTERNAL_LE_VOICE_START_T * start_params)
{
    le_voice_path_t voice_path = kymeraLeVoice_GetVoicePath(start_params);

    /* If there is a tone still playing at this point,
     * it must be an interruptible tone, so cut it off */
    appKymeraTonePromptStop();

    if(appKymeraGetState() == KYMERA_STATE_STANDALONE_LEAKTHROUGH)
    {
        Kymera_LeakthroughStopChainIfRunning();
        appKymeraSetState(KYMERA_STATE_IDLE);
    }

    /* Can't start voice chain if we're not idle */
    PanicFalse(appKymeraGetState() == KYMERA_STATE_IDLE || appKymeraGetState() == KYMERA_STATE_ADAPTIVE_ANC_STARTED);

    /* LE Voice chain must be destroyed if we get here */
    PanicNotNull(le_voice_chain);

    /* Move to LE Voice active state now, what ever happens we end up in this state
      (even if it's temporary) */
    appKymeraSetState(KYMERA_STATE_LE_VOICE_ACTIVE);

    /* Momentarily boosting DSP clock for faster chain creation and setup */
    appKymeraBoostDspClockToMax();

    kymeraLeVoice_SetTtpDelay(&start_params->microphone_params);

    KymeraLeVoice_SetSampleRate(start_params->speaker_params.sample_rate);

#ifdef INCLUDE_SWB_LC3
    PanicFalse(Kymera_AecSetTtpDelayBeforeConnection(kymeraLeVoice_ttpDelay));
#endif

    kymeraLeVoice_SetupToAirChain(start_params, voice_path);

    kymeraLeVoice_SetupFromAirChain(start_params, voice_path);

    kymeraLeVoice_StartFromAirChain(start_params, voice_path);

    kymeraLeVoice_StartToAirChain(voice_path);
}

void KymeraLeVoice_HandleInternalStop(void)
{
    DEBUG_LOG("KymeraLeVoice_HandleInternalStop, state %u", appKymeraGetState());

    if (appKymeraGetState() != KYMERA_STATE_LE_VOICE_ACTIVE)
    {
        if (le_voice_chain == NULL)
        {
            /* Attempting to stop a LE Voice chain when not ACTIVE. This happens
               when the user calls Kymera_LeVoiceStop() following a failed
               attempt to start the LE Voice chain - see ChainStartAttempt() in
               KymeraLeVoice_HandleInternalStart().
               In this case, there is nothing to do, since the failed start
               attempt cleans up by calling this function in state
               KYMERA_STATE_LE_VOICE_ACTIVE */
            DEBUG_LOG("KymeraLeVoice_HandleInternalStop, not stopping - already idle");
            return;
        }
        else
        {
            Panic();
        }
    }

    if (le_voice_chain != NULL)
    {
        Source iso_ep_src = ChainGetOutput(le_voice_chain, EPR_ISO_TO_AIR_LEFT);
        Sink iso_ep_snk = ChainGetInput(le_voice_chain, EPR_ISO_FROM_AIR_LEFT);
        Sink iso_ep_snk_right = ChainGetInput(le_voice_chain, EPR_ISO_FROM_AIR_RIGHT);

        /* Disable AEC_REF sidetone path */
        Kymera_LeakthroughEnableSidetonePath(FALSE);

        /* A tone still playing at this point must be interruptable */
        appKymeraTonePromptStop();

        /* Stop chains */
        ChainStop(le_voice_chain);

        /* Disconnect SCO from chain SCO endpoints */
        StreamDisconnect(iso_ep_src, NULL);
        StreamDisconnect(NULL, iso_ep_snk);
        StreamDisconnect(NULL, iso_ep_snk_right);

        Kymera_MicDisconnect(mic_user_le_voice);

        output_users_t output_user_le_voice = use_split_stereo_chain ? output_user_le_voice_stereo : output_user_le_voice_mono;
        Kymera_OutputDisconnect(output_user_le_voice);

        /* Reset TTP delay value set by client in mic user registry */
        kymeraLeVoice_ttpDelay = 0;

        /* Destroy chains */
        ChainDestroy(le_voice_chain);
        le_voice_chain = NULL;
    }
    else
    {
        Kymera_LeMicDisconnectOutputChain();
        Kymera_StopLeMicChain();
    }

    /* Update state variables */
    appKymeraSetState(KYMERA_STATE_IDLE);

    Kymera_LeakthroughResumeChainIfSuspended();
}

void KymeraLeVoice_HandleInternalSetVolume(int16 volume_in_db)
{
    DEBUG_LOG("KymeraLeVoice_HandleInternalSetVolume, vol %d", volume_in_db);

    switch (KymeraGetTaskData()->state)
    {
    case KYMERA_STATE_LE_VOICE_ACTIVE:
        KymeraOutput_SetMainVolume(volume_in_db);
        break;

    default:
        break;
    }
}

void kymeraLeVoice_HandleInternalMicMute(bool mute)
{
    DEBUG_LOG("kymeraLeVoice_HandleInternalMicMute, mute %u", mute);

    switch (KymeraGetTaskData()->state)
    {
    case KYMERA_STATE_LE_VOICE_ACTIVE:
        KymeraLeAudioVoice_SetMicMuteState(mute);
        break;

    default:
        break;
    }
}

kymera_chain_handle_t Kymera_LeVoiceGetCvcChain(void)
{
    return le_voice_chain;
}

bool Kymera_IsLeVoiceSplitStereoChain(void)
{
    return use_split_stereo_chain;
}

void KymeraLeVoice_Init(void)
{
    Kymera_OutputRegister(&output_info_mono);
    Kymera_OutputRegister(&output_info_stereo);
    Kymera_MicRegisterUser(&kymeraLeVoice_MicRegistry);
}

#endif
