/*!
\copyright  Copyright (c) 2017 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_sco.c
\brief      Kymera SCO
*/

#include "kymera_sco_private.h"
#include "kymera_state.h"
#include "kymera_config.h"
#include "kymera_dsp_clock.h"
#include "kymera_tones_prompts.h"
#include "kymera_aec.h"
#include "kymera_leakthrough.h"
#include "kymera_output.h"
#include "av.h"
#include "kymera_mic_if.h"
#include "kymera_output_if.h"
#include "kymera_data.h"
#include "kymera_setup.h"
#include "kymera_anc_common.h"
#include "kymera_chain_utils.h"

#ifdef INCLUDE_CVC_DEMO
#include "cap_id_prim.h"
#endif

#ifdef INCLUDE_HYBRID_CVC
#include "kymera_ml.h"
#endif

#include <vmal.h>
#include <anc_state_manager.h>
#include <timestamp_event.h>
#include <opmsg_prim.h>

#ifdef INCLUDE_SWB_LC3
#include <operators.h>
#endif
#define AWBSDEC_SET_BITPOOL_VALUE    0x0003
#define AWBSENC_SET_BITPOOL_VALUE    0x0001

#ifdef ENABLE_ADAPTIVE_ANC
#define AEC_TX_BUFFER_SIZE_MS 45
#else
#define AEC_TX_BUFFER_SIZE_MS 15
#endif

#ifdef INCLUDE_SWB_LC3
#define SCO_SWB_LC3_FRAME_DURATION_IN_US       7500
#define SCO_SWB_LC3_PACKET_LENGTH_IN_BYTES     58
#define SCO_SWB_LC3_TTP_LATENCY_IN_US          20000
#define SCO_SWB_LC3_MANDATORY_TTP_DELAY_IN_US  30000

#define SCO_ISO_ENDPOINT_FOR_LC3 1
#define SCO_ISO_ENDPOINT_FOR_ISO 0
#endif

typedef struct set_bitpool_msg_s
{
    uint16 id;
    uint16 bitpool;
}set_bitpool_msg_t;

#ifdef INCLUDE_CVC_DEMO
typedef struct
{
    uint8 mic_config;
    kymera_cvc_mode_t mode;
    uint8 passthrough_mic;
    uint8 mode_of_operation;

} cvc_send_settings_t;

static cvc_send_settings_t cvc_send_settings = {0};

static void kymera_ScoSetCvc3MicSettings(void);
#endif /* INCLUDE_CVC_DEMO */

static kymera_chain_handle_t the_sco_chain;
static enum
{
    kymera_sco_idle,
    kymera_sco_prepared,
    kymera_sco_starting,
    kymera_sco_started,
} kymera_sco_state = kymera_sco_idle;

static bool kymera_ScoMicGetConnectionParameters(uint16 *mic_ids, Sink *mic_sinks, uint8 *num_of_mics, uint32 *sample_rate, Sink *aec_ref_sink);
static bool kymera_ScoMicDisconnectIndication(const mic_change_info_t *info);
static mic_user_state_t kymera_ScoMicGetScoState(void);
static bool kymera_ScoMicGetAecRefUsage(void);
static uint32 kymera_ScoMicGetMandatoryTtpDelay(void);

static void kymera_ConfigureScoReceive(Operator sco_receive, const void *params);
static void kymera_ConfigureScoSendResampler(Operator resampler, const void *params);
static void kymera_ConfigurePcmMicGain(Operator pcm_mic_gain, const void *params);
static void kymera_ConfigureCvcSend(Operator sco_send, const void *params);

static const mic_callbacks_t kymera_MicScoCallbacks =
{
    .MicGetConnectionParameters = kymera_ScoMicGetConnectionParameters,
    .MicDisconnectIndication = kymera_ScoMicDisconnectIndication,
    .MicGetUserState = kymera_ScoMicGetScoState,
    .MicGetAecRefUsage = kymera_ScoMicGetAecRefUsage,
    .MicGetMandatoryTtpDelay = kymera_ScoMicGetMandatoryTtpDelay,
};

static const mic_registry_per_user_t kymera_MicScoRegistry =
{
    .user = mic_user_sco,
    .callbacks = &kymera_MicScoCallbacks,
    .permanent.mandatory_mic_ids = NULL,
    .permanent.num_of_mandatory_mics = 0,
};

static const output_registry_entry_t output_info =
{
    .user = output_user_sco,
    .connection = output_connection_mono,
};

static const operator_config_map_t operator_config_map[] =
{
    {OPR_SCO_RECEIVE,  kymera_ConfigureScoReceive},
    {OPR_SCO_SEND_RESAMPLER, kymera_ConfigureScoSendResampler},
    {OPR_PCM_MIC_GAIN, kymera_ConfigurePcmMicGain},
    {OPR_CVC_SEND,     kymera_ConfigureCvcSend},
};

static void kymera_ConfigureScoReceive(Operator sco_receive, const void *params)
{
    /*! \todo Need to decide ahead of time if we need any latency.
        Simple enough to do if we are legacy or not. Less clear if
        legacy but no peer connection */
    sco_chain_op_params_t * sco_chain_op_params = (sco_chain_op_params_t *)params;

    /* Enable Time To Play if supported */
    if (appConfigScoChainTTP(sco_chain_op_params->wesco) != 0)
    {
        OperatorsStandardSetTimeToPlayLatency(sco_receive, appConfigScoChainTTP(sco_chain_op_params->wesco));
        OperatorsStandardSetBufferSize(sco_receive, appConfigScoBufferSize(sco_chain_op_params->rates.use_case));
    }
}

static void kymera_ConfigureScoSendResampler(Operator resampler, const void *params)
{
    sco_chain_op_params_t * sco_chain_op_params = (sco_chain_op_params_t *)params;
    OperatorsConfigureResampler(resampler, sco_chain_op_params->rates.mic, sco_chain_op_params->rates.use_case);
}

static void kymera_ConfigurePcmMicGain(Operator pcm_mic_gain, const void *params)
{
    UNUSED(params);
    OperatorsSetPassthroughDataFormat(pcm_mic_gain, operator_data_format_pcm);
}

static void kymera_ConfigureCvcSend(Operator cvc_send, const void *params)
{
    UNUSED(params);
#ifdef INCLUDE_HYBRID_CVC
    kymera_ml_config_t ml_config = {
        .use_case_id = 1,
        .access_method = model_direct_access,
        .batch_reset_count = 0,
        .model_file = "cvc_noise_sup.keai"
    };
    Kymera_MlActivate(cvc_send, &ml_config);
#else
    UNUSED(cvc_send);
#endif
}

static void kymera_DestroyScoChain(void)
{
#ifdef INCLUDE_HYBRID_CVC
    Operator cvc_send = PanicZero(ChainGetOperatorByRole(the_sco_chain, OPR_CVC_SEND));
    Kymera_MlDeactivate(cvc_send);   
#endif

    ChainDestroy(the_sco_chain);
    the_sco_chain = NULL;

#ifdef INCLUDE_HYBRID_CVC
    Kymera_MlEngineDestroy();
#endif

    /* Update state variables */
    appKymeraSetState(KYMERA_STATE_IDLE);
    kymera_sco_state = kymera_sco_idle;

    Kymera_LeakthroughResumeChainIfSuspended();
}

static kymera_chain_handle_t kymera_GetScoChain(void)
{
    return the_sco_chain;
}

#ifdef INCLUDE_SWB_LC3
static void kymera_ConfigureDecoder(kymera_chain_handle_t chain_handle)
{
    Operator lc3_decode_op = ChainGetOperatorByRole(chain_handle, OPR_LC3_DECODE_SCO_ISO);
    DEBUG_LOG_FN_ENTRY("kymera_ConfigureLc3Decoder");

    /* 
    Setting Frame Duration and Packet Length as defined by the specification 
    +--------------------------------------------------+---------------+
    | Parameter                                        |  Value        |
    +--------------------------------------------------+---------------+
    | Channel mode                                     | Mono          |
    | Sampling rate                                    | 32 kHz        |
    | Frame duration                                   | 7.5 ms        |
    | Byte count per codec frame(excluding H2 header)  | 58            |
    | Resulting bit rate (excluding H2 header)         | 61867 bits/s  |
    +--------------------------------------------------+---------------+
    */
    OperatorsLc3DecoderScoIsoSetFrameDuration(lc3_decode_op, SCO_SWB_LC3_FRAME_DURATION_IN_US);
    OperatorsLc3DecoderScoIsoSetPacketLength(lc3_decode_op, SCO_SWB_LC3_PACKET_LENGTH_IN_BYTES);

    /* 32kHz/25 = 1280 */
    OperatorsStandardSetSampleRate(lc3_decode_op, (KymeraGetTaskData()->sco_info->rates.use_case));
    /* 18ms to 20ms for audio @120MHz */
    OperatorsStandardSetTimeToPlayLatency(lc3_decode_op, SCO_SWB_LC3_TTP_LATENCY_IN_US);

    /* Configuring the SCO_ISO endpoint to configure as LC3 input connected to it. */
    OperatorsLc3DecoderScoIsoIdConnectScoEndpoint(lc3_decode_op, SCO_ISO_ENDPOINT_FOR_LC3);
}

static void kymera_ConfigureEncoder(kymera_chain_handle_t chain_handle)
{
    DEBUG_LOG_FN_ENTRY("kymera_ConfigureEncoder");
    Operator lc3_encode_op = ChainGetOperatorByRole(chain_handle, OPR_LC3_ENCODE_SCO_ISO);
    OperatorsLc3EncoderScoIsoSetFrameDuration(lc3_encode_op, SCO_SWB_LC3_FRAME_DURATION_IN_US);
    OperatorsStandardSetSampleRate(lc3_encode_op, (KymeraGetTaskData()->sco_info->rates.use_case));
    OperatorsLc3EncoderScoIsoSetPacketLength(lc3_encode_op, SCO_SWB_LC3_PACKET_LENGTH_IN_BYTES);

    /* Configuring the SCO_ISO endpoint to configure as LC3 input connected to it. */
    OperatorsLc3EncoderScoIsoIdConnectScoEndpoint(lc3_encode_op, SCO_ISO_ENDPOINT_FOR_LC3);
}
#endif

static void kymera_ConfigureScoChain(uint16 wesco)
{
    DEBUG_LOG_INFO("kymera_ConfigureScoChain");

    kymera_chain_handle_t sco_chain = kymera_GetScoChain();
    kymeraTaskData *theKymera = KymeraGetTaskData();
    PanicNull((void *)theKymera->sco_info);

    sco_chain_op_params_t sco_op_params = {0};
    sco_op_params.rates = theKymera->sco_info->rates;
    sco_op_params.wesco = wesco;
#ifdef INCLUDE_SWB_LC3
    const appKymeraScoMode mode = theKymera->sco_info->mode;

    /* configure */
    if (mode == SCO_SWB_LC3)
    {
        /* Adjust TTP_LATENCY for CVC_RECEIVE/CVC_SEND algorithmic delay */
        /* cVc's frame size for 16/32kHz (WB/SWB) is 7.5ms */

        /* Configure LC3 DECODE */
        kymera_ConfigureDecoder(sco_chain);

        /* Configure LC3 ENCODE */
        kymera_ConfigureEncoder(sco_chain);
    }
#endif

    if (sco_op_params.rates.mic != sco_op_params.rates.use_case)
    {
        // Resampling SCO SEND is necessary
        PanicFalse(ChainGetOperatorByRole(sco_chain, OPR_SCO_SEND_RESAMPLER) != INVALID_OPERATOR);
    }
    Kymera_ConfigureChain(sco_chain, operator_config_map, ARRAY_DIM(operator_config_map), &sco_op_params);

    Kymera_SetVoiceUcids(sco_chain);

    if(theKymera->chain_config_callbacks && theKymera->chain_config_callbacks->ConfigureScoInputChain)
    {
        kymera_sco_config_params_t params = {0};
        params.rates = theKymera->sco_info->rates;
        params.mode = theKymera->sco_info->mode;
        params.wesco = wesco;
        theKymera->chain_config_callbacks->ConfigureScoInputChain(sco_chain, &params);
    }
}

static void kymera_PopulateScoConnectParams(uint16 *mic_ids, Sink *mic_sinks, uint8 num_mics, Sink *aec_ref_sink)
{
    kymera_chain_handle_t sco_chain = kymera_GetScoChain();
#ifdef INCLUDE_SWB_LC3
    kymeraTaskData *theKymera = KymeraGetTaskData();
    PanicNull((void *)theKymera->sco_info);
    appKymeraScoMode mode = theKymera->sco_info->mode;
#endif
    PanicZero(mic_ids);
    PanicZero(mic_sinks);
    PanicZero(aec_ref_sink);
    PanicFalse(num_mics <= 3);

    mic_ids[0] = appConfigMicVoice();
#ifdef INCLUDE_SWB_LC3
    if (mode == SCO_SWB_LC3)
    {
        mic_sinks[0] = ChainGetInput(sco_chain, EPR_RATE_ADJUST_IN1);
    }
    else 
#endif
    {
        mic_sinks[0] = ChainGetInput(sco_chain, EPR_CVC_SEND_IN1);
    }

    if(num_mics > 1)
    {
        mic_ids[1] = appConfigMicExternal();
#ifdef INCLUDE_SWB_LC3
        if (mode == SCO_SWB_LC3)
        {
            mic_sinks[1] = ChainGetInput(sco_chain, EPR_RATE_ADJUST_IN2);
        }
        else
#endif
        {
            mic_sinks[1] = ChainGetInput(sco_chain, EPR_CVC_SEND_IN2);
        }
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
#ifdef INCLUDE_SWB_LC3
        if (mode == SCO_SWB_LC3)
        {
            mic_sinks[2] = ChainGetInput(sco_chain, EPR_RATE_ADJUST_IN3);
        }
        else
#endif
        {
            mic_sinks[2] = ChainGetInput(sco_chain, EPR_CVC_SEND_IN3);
        }
    }
#ifdef INCLUDE_SWB_LC3
    if (mode == SCO_SWB_LC3)
    {
        aec_ref_sink[0] = ChainGetInput(sco_chain, EPR_RATE_ADJUST_REF_IN);
    }
    else
#endif
    {
        aec_ref_sink[0] = ChainGetInput(sco_chain, EPR_CVC_SEND_REF_IN);
    }
}

static bool kymera_ScoMicDisconnectIndication(const mic_change_info_t *info)
{
    UNUSED(info);
    DEBUG_LOG_ERROR("kymera_ScoMicDisconnectIndication: SCO shouldn't have to get disconnected");
    Panic();
    return TRUE;
}

static bool kymera_ScoMicGetConnectionParameters(uint16 *mic_ids, Sink *mic_sinks, uint8 *num_of_mics, uint32 *sample_rate, Sink *aec_ref_sink)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("kymera_ScoMicGetConnectionParameters");

    *sample_rate = theKymera->sco_info->rates.mic;
    *num_of_mics = appConfigVoiceGetNumberOfMics();
    kymera_PopulateScoConnectParams(mic_ids, mic_sinks, theKymera->sco_info->mic_cfg, aec_ref_sink);
    return TRUE;
}

static mic_user_state_t kymera_ScoMicGetScoState(void)
{
    return mic_user_state_non_interruptible;
}

static bool kymera_ScoMicGetAecRefUsage(void)
{
    return TRUE;
}

static uint32 kymera_ScoMicGetMandatoryTtpDelay(void)
{
    uint32 kymera_MicScoTtpDelay = 0;

#ifdef INCLUDE_SWB_LC3
    kymeraTaskData *theKymera = KymeraGetTaskData();
    if(theKymera->sco_info && theKymera->sco_info->mode == SCO_SWB_LC3)
    {
        kymera_MicScoTtpDelay = SCO_SWB_LC3_MANDATORY_TTP_DELAY_IN_US;
    }
#endif
    DEBUG_LOG("kymera_SetMicScoTtpDelay %d us",kymera_MicScoTtpDelay);
    return kymera_MicScoTtpDelay;
}

static kymera_chain_handle_t kymera_ScoCreateChain(const appKymeraScoChainInfo *info)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraCreateScoChain, mode %u, mic_cfg %u, mic rate %u, use_case rate %u",
               info->mode, info->mic_cfg, info->rates.mic, info->rates.use_case);


#ifdef INCLUDE_HYBRID_CVC
    Kymera_MlEngineCreate();
#endif

    theKymera->sco_info = info;
    return ChainCreate(info->chain);
}

static void kymera_ScoDestroy(void)
{
    DEBUG_LOG("kymera_ScoDestroy: SCO state %d", kymera_sco_state);

    if (kymera_sco_state == kymera_sco_idle)
    {
        if (!kymera_GetScoChain())
        {
            /*
             * Attempting to stop a SCO chain when already idle.
             * This happens when the user attempts to stop SCO, following a failed attempt to start the SCO chain.
             * In this case, there is nothing to do, since the failed start attempt has already stopped SCO putting it back to idle already.
             */
            DEBUG_LOG("kymera_ScoDestroy: Already idle");
            return;
        }
        else
        {
            Panic();
        }
    }

    Source sco_ep_src = ChainGetOutput(kymera_GetScoChain(), EPR_SCO_TO_AIR);
    Sink sco_ep_snk = ChainGetInput(kymera_GetScoChain(), EPR_SCO_FROM_AIR);

    Kymera_LeakthroughEnableSidetonePath(FALSE);

    /* A tone still playing at this point must be interruptible */
    appKymeraTonePromptStop();

    ChainStop(kymera_GetScoChain());

    /* Disconnect SCO from chain SCO endpoints */
    StreamDisconnect(sco_ep_src, NULL);
    StreamDisconnect(NULL, sco_ep_snk);

    Kymera_MicDisconnect(mic_user_sco);
    Kymera_OutputDisconnect(output_user_sco);
    kymera_DestroyScoChain();

    appKymeraConfigureDspPowerMode();
}

static bool kymera_ScoAttemptStartChains(int16 volume_in_db)
{
    PanicFalse(kymera_sco_state == kymera_sco_prepared);
    PanicNull(kymera_GetScoChain());

    kymera_sco_state = kymera_sco_starting;
    appKymeraBoostDspClockToMax();

    /* Connect to Mic interface */
    if (!Kymera_MicConnect(mic_user_sco))
    {
        DEBUG_LOG_PANIC("Kymera_ScoHandlePrepareReq: Mic connection was not successful. Sco should always be prepared.");
    }
    KymeraOutput_ChainStart();

	appKymeraConfigureDspPowerMode();

    /* The chain can fail to start if the SCO source disconnects whilst kymera
    is queuing the SCO start request or starting the chain. If the attempt fails,
    ChainStartAttempt will stop (but not destroy) any operators it started in the chain. */
    if (ChainStartAttempt(kymera_GetScoChain()))
    {
        TimestampEvent(TIMESTAMP_EVENT_SCO_MIC_STREAM_STARTED);

        Kymera_ScoHandleInternalSetVolume(volume_in_db);

#ifndef FORCE_CVC_PASSTHROUGH
        if (KymeraGetTaskData()->enable_cvc_passthrough)
#endif
        {
            Kymera_SetCvcPassthroughMode(KYMERA_CVC_RECEIVE_PASSTHROUGH | KYMERA_CVC_SEND_PASSTHROUGH, 0);
        }
#ifdef INCLUDE_CVC_DEMO
        kymera_ScoSetCvc3MicSettings();
#endif

        Kymera_LeakthroughSetAecUseCase(aec_usecase_enable_leakthrough);
        kymera_sco_state = kymera_sco_started;
        return TRUE;
    }

    DEBUG_LOG("kymera_ScoAttemptStartChains: Could not start chain");
    /*
     * This needs to be done here, since between the failed attempt to start and the subsequent stop (if message is used)
     * a tone may need to be played - it would not be possible to play a tone in a stopped SCO chain.
     */
    kymera_ScoDestroy();
    return FALSE;
}

void Kymera_ScoHandleInternalStop(void)
{
    kymera_ScoDestroy();
    if (KymeraAncCommon_AdaptiveAncIsEnabled())
    {
        appKymeraSetState(KYMERA_STATE_ADAPTIVE_ANC_STARTED);
        appKymeraConfigureDspPowerMode();
    }
    MessageCancelFirst(KymeraGetTask(), KYMERA_INTERNAL_SCO_AUDIO_SYNCHRONISED);
}

void Kymera_ScoHandleInternalSetVolume(int16 volume_in_db)
{
    DEBUG_LOG("Kymera_ScoHandleInternalSetVolume: vol %d", volume_in_db);
    if (kymera_sco_state != kymera_sco_idle)
    {
        KymeraOutput_SetMainVolume(volume_in_db);
    }
}

void Kymera_ScoHandleInternalMicMute(bool mute)
{
    DEBUG_LOG("Kymera_ScoHandleInternalMicMute: mute %u", mute);
    if (kymera_sco_state != kymera_sco_idle)
    {
        Operator cvc_send_op;
        if (GET_OP_FROM_CHAIN(cvc_send_op, kymera_GetScoChain(), OPR_CVC_SEND))
        {
            OperatorsStandardSetControl(cvc_send_op, cvc_send_mute_control, mute);
        }
        else
        {
            // Fall-back when CVC send is not present, mute via AEC REF operator
            Operator aec_op = Kymera_GetAecOperator();
            if (aec_op != INVALID_OPERATOR)
            {
                OperatorsAecMuteMicOutput(aec_op, mute);
            }
        }
    }
}

uint8 appKymeraScoVoiceQuality(void)
{
    uint8 quality = appConfigVoiceQualityWorst();

    if (appConfigVoiceQualityMeasurementEnabled())
    {
        Operator cvc_send_op;
        if (GET_OP_FROM_CHAIN(cvc_send_op, kymera_GetScoChain(), OPR_CVC_SEND))
        {
            uint16 rx_msg[2], tx_msg = OPMSG_COMMON_GET_VOICE_QUALITY;
            PanicFalse(OperatorMessage(cvc_send_op, &tx_msg, 1, rx_msg, 2));
            quality = MIN(appConfigVoiceQualityBest() , rx_msg[1]);
            quality = MAX(appConfigVoiceQualityWorst(), quality);
        }
    }
    else
    {
        quality = appConfigVoiceQualityWhenDisabled();
    }

    DEBUG_LOG("appKymeraScoVoiceQuality %u", quality);

    return quality;
}

void Kymera_ScoHandlePrepareReq(Sink sco_snk, const appKymeraScoChainInfo *info, uint8 wesco, bool synchronised_start)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("Kymera_ScoHandlePrepareReq: sink 0x%x, mode enum:appKymeraScoMode:%d, wesco %u, state %u", sco_snk, info->mode, wesco, appKymeraGetState());

    /* If there is a tone still playing at this point,
     * it must be an interruptible tone, so cut it off */
    appKymeraTonePromptStop();

    Kymera_LeakthroughStopChainIfRunning();

    PanicFalse(kymera_sco_state == kymera_sco_idle);
    PanicNotNull(kymera_GetScoChain());

    /* Change kymera state now (even if only temporarily) */
    appKymeraSetState(KYMERA_STATE_SCO_ACTIVE);

    appKymeraBoostDspClockToMax();
    the_sco_chain = kymera_ScoCreateChain(info);

#ifdef INCLUDE_SWB_LC3
    if(theKymera->sco_info && theKymera->sco_info->mode == SCO_SWB_LC3)
    {
        PanicFalse(Kymera_AecSetTtpDelayBeforeConnection(kymera_ScoMicGetMandatoryTtpDelay()));
    }
#endif
    /* Configure chain specific operators */
    kymera_ConfigureScoChain(wesco);

    /* Create an appropriate Output chain */
    kymera_output_chain_config output_config;
    KymeraOutput_SetDefaultOutputChainConfig(&output_config, theKymera->sco_info->rates.use_case, KICK_PERIOD_VOICE, 0);

    output_config.chain_type = output_chain_mono;
    output_config.chain_include_aec = TRUE;
    PanicFalse(Kymera_OutputPrepare(output_user_sco, &output_config));

    /* Get sources and sinks for chain endpoints */
    Source sco_ep_src = ChainGetOutput(the_sco_chain, EPR_SCO_TO_AIR);
    Sink sco_ep_snk = ChainGetInput(the_sco_chain, EPR_SCO_FROM_AIR);

    /* Connect SCO to chain SCO endpoints */
    /* Get SCO source from SCO sink */
    Source sco_src = StreamSourceFromSink(sco_snk);
 
#ifdef INCLUDE_SWB_LC3
    /* configure Rate adjust operator */
    if (theKymera->sco_info->mode == SCO_SWB_LC3)
    {
        /* Configure Rate Adjust */
        Operator op = ChainGetOperatorByRole(the_sco_chain, OPR_RATE_ADJUST);
        OperatorsStandardSetSampleRate(op, theKymera->sco_info->rates.use_case);
        SinkConfigure(sco_snk, STREAM_RM_USE_RATE_ADJUST_OPERATOR, op);
    }
#endif

    StreamConnect(sco_ep_src, sco_snk);
    StreamConnect(sco_src, sco_ep_snk);

    /* Connect chain */
    ChainConnect(the_sco_chain);

    /* Connect to the Output chain */
    output_source_t sources = {.mono = ChainGetOutput(the_sco_chain, EPR_SCO_SPEAKER)};
    PanicFalse(Kymera_OutputConnect(output_user_sco, &sources));
    if(synchronised_start)
    {
        KymeraOutput_MuteMainChannel(TRUE);
    }
    kymera_sco_state = kymera_sco_prepared;
    appKymeraConfigureDspPowerMode();
}

bool Kymera_ScoHandleStartReq(Kymera_ScoStartedHandler started_handler, int16 volume_in_db)
{
    if (kymera_ScoAttemptStartChains(volume_in_db))
    {
        /* Schedule auto-unmute after timeout if Kymera_ScheduleScoSyncUnmute is not called */
        Kymera_ScheduleScoSyncUnmute(appConfigScoSyncUnmuteTimeoutMs());
        if (started_handler)
        {
            started_handler();
        }
        return TRUE;
    }
    return FALSE;
}

void Kymera_ScoHandleInternalStart(const KYMERA_INTERNAL_SCO_START_T *msg)
{
    Kymera_ScoHandlePrepareReq(msg->audio_sink, msg->sco_info, msg->wesco, msg->synchronised_start);
    Kymera_ScoHandleStartReq(msg->started_handler, msg->volume_in_db);
}

void Kymera_ScoInit(void)
{
    Kymera_OutputRegister(&output_info);
    Kymera_MicRegisterUser(&kymera_MicScoRegistry);
#ifdef INCLUDE_CVC_DEMO
    cvc_send_settings.mic_config = 3;
#endif
}

kymera_chain_handle_t Kymera_ScoGetCvcChain(void)
{
    return the_sco_chain;
}

bool Kymera_ScoIsActive(void)
{
    switch(kymera_sco_state)
    {
        case kymera_sco_idle:
        case kymera_sco_prepared:
            return FALSE;
        case kymera_sco_starting:
        case kymera_sco_started:
            return TRUE;
    }
    Panic();
    return FALSE;
}

get_status_data_t* Kymera_GetOperatorStatusDataInScoChain(unsigned operator_role, uint8 number_of_params)
{
    get_status_data_t* get_status = NULL;

    if (kymera_sco_state != kymera_sco_idle)
    {

        Operator op = ChainGetOperatorByRole(the_sco_chain, operator_role);
        get_status = OperatorsCreateGetStatusData(number_of_params);
        OperatorsGetStatus(op, get_status);
    }
    else
    {
        DEBUG_LOG("Kymera_GetOperatorStatusDataInScoChain:Sco Not active yet");
    }
    return get_status;
}

#ifdef INCLUDE_CVC_DEMO

#define CVC_3_MIC_SET_OCCLUDED_MODE       2
#define CVC_3_MIC_SET_EXTERNAL_MIC_MODE   0
#define CVC_3_MIC_BYP_NPC_MASK          0x8

#define CVC_3_MIC_NUM_STATUS_VAR         29
#define CVC_3_MIC_STATUS_BITFLAG         24
#define CVC_3_MIC_THREE_MIC_FLAG_MASK   0x8

static bool kymera_Is3MicCvcInScoChain(kymera_chain_handle_t chain_containing_cvc)
{
    bool cvc_3mic_found = FALSE;
    if(chain_containing_cvc != NULL)
    {
        cvc_3mic_found = ChainCheckCapabilityId(chain_containing_cvc, (capability_id_t)CAP_ID_DOWNLOAD_CVCEB3MIC_MONO_IE_NB) |
                         ChainCheckCapabilityId(chain_containing_cvc, (capability_id_t)CAP_ID_DOWNLOAD_CVCEB3MIC_MONO_IE_WB) |
                         ChainCheckCapabilityId(chain_containing_cvc, (capability_id_t)CAP_ID_DOWNLOAD_CVCEB3MIC_MONO_IE_SWB) |
                         ChainCheckCapabilityId(chain_containing_cvc, (capability_id_t)CAP_ID_CVCEB3MIC_MONO_IE_NB) |
                         ChainCheckCapabilityId(chain_containing_cvc, (capability_id_t)CAP_ID_CVCEB3MIC_MONO_IE_WB) |
                         ChainCheckCapabilityId(chain_containing_cvc, (capability_id_t)CAP_ID_DOWNLOAD_CVCHS3MIC_MONO_SEND_FB) |
                         ChainCheckCapabilityId(chain_containing_cvc, (capability_id_t)CAP_ID_CVCHS3MIC_MONO_SEND_FB);
#ifdef INCLUDE_HYBRID_CVC
#define CAP_ID_CVC_3MIC_WB  0x40C7
#define CAP_ID_CVC_3MIC_SWB 0x40C8
        cvc_3mic_found |= ChainCheckCapabilityId(chain_containing_cvc, (capability_id_t)CAP_ID_CVC_3MIC_WB) |
                          ChainCheckCapabilityId(chain_containing_cvc, (capability_id_t)CAP_ID_CVC_3MIC_SWB);
#endif
    }
    return cvc_3mic_found;
}

static void kymera_SetCvcSendIntMicModeInChain(kymera_chain_handle_t chain_containing_cvc, uint16 mic_mode)
{
    Operator cvc_op;

    PanicFalse(GET_OP_FROM_CHAIN(cvc_op, chain_containing_cvc, OPR_CVC_SEND));
    OperatorsCvcSendSetIntMicMode(cvc_op, mic_mode);
}

static void kymera_SetCvcSendOmniModeInChain(kymera_chain_handle_t chain_containing_cvc, bool enable)
{
    Operator cvc_op;

    PanicFalse(GET_OP_FROM_CHAIN(cvc_op, chain_containing_cvc, OPR_CVC_SEND));
    if(enable)
    {
        OperatorsCvcSendEnableOmniMode(cvc_op);
    }
    else
    {
        OperatorsCvcSendDisableOmniMode(cvc_op);
    }
}

static void kymera_SetCvcSendBypNpcInChain(kymera_chain_handle_t chain_containing_cvc, bool enable)
{
    Operator cvc_op;
    uint32 dmss_config;
    PanicFalse(GET_OP_FROM_CHAIN(cvc_op, chain_containing_cvc, OPR_CVC_SEND));

    dmss_config = OperatorsCvcSendGetDmssConfig(cvc_op);
    if(enable)
    {
        dmss_config |= CVC_3_MIC_BYP_NPC_MASK;
    }
    else
    {
        dmss_config &= (~CVC_3_MIC_BYP_NPC_MASK);
    }
    OperatorsCvcSendSetDmssConfig(cvc_op, dmss_config);
}

bool Kymera_ScoSetCvcSend3MicMicConfig(uint8 mic_config)
{
    bool setting_changed = FALSE;

    if (cvc_send_settings.mic_config != mic_config)
    {
        setting_changed = TRUE;
    }
    cvc_send_settings.mic_config = mic_config;

    if (kymera_sco_state != kymera_sco_idle)
    {
        kymera_chain_handle_t sco_chain = kymera_GetScoChain();
        if(kymera_Is3MicCvcInScoChain(sco_chain))
        {
            DEBUG_LOG("Kymera_ScoSetCvcSend3MicMicConfig: %dmic cVc", mic_config);
            switch(mic_config)
            {
                case 0:
                    DEBUG_LOG("Kymera_ScoSetCvcSend3MicMicConfig: Passthrough active");
                    break;
                case 1:
                    kymera_SetCvcSendOmniModeInChain(sco_chain, TRUE);
                    kymera_SetCvcSendBypNpcInChain(sco_chain, FALSE);
                    break;
                case 2:
                    kymera_SetCvcSendOmniModeInChain(sco_chain, FALSE);
                    kymera_SetCvcSendBypNpcInChain(sco_chain, TRUE);
                    /* 2Mic mode in combination with HW Leakthrough required */
                    kymera_SetCvcSendIntMicModeInChain(sco_chain, CVC_3_MIC_SET_EXTERNAL_MIC_MODE);
                    break;
                case 3:
                    kymera_SetCvcSendOmniModeInChain(sco_chain, FALSE);
                    kymera_SetCvcSendBypNpcInChain(sco_chain, FALSE);
                    kymera_SetCvcSendIntMicModeInChain(sco_chain, CVC_3_MIC_SET_OCCLUDED_MODE);
                    break;
                default:
                    DEBUG_LOG("Kymera_ScoSetCvcSend3MicMicConfig: Unknown mic config");
            }
        }
        else
        {
            DEBUG_LOG("Kymera_ScoSetCvcSend3MicMicConfig: No 3Mic cVc found in chain");
        }
    }
    else
    {
        DEBUG_LOG("Kymera_ScoSetCvcSend3MicMicConfig: Storing mic_config %d for next SCO call", mic_config);
    }
    return setting_changed;
}

void Kymera_ScoGetCvcPassthroughMode(kymera_cvc_mode_t *mode, uint8 *passthrough_mic)
{
    *mode = cvc_send_settings.mode;
    *passthrough_mic = cvc_send_settings.passthrough_mic;
    DEBUG_LOG("Kymera_ScoGetCvcPassthroughMode: mode enum:kymera_cvc_mode_t:%d passthrough_mic %d", *mode, *passthrough_mic);
}

void Kymera_ScoGetCvcSend3MicMicConfig(uint8 *mic_config)
{
    *mic_config = cvc_send_settings.mic_config;
    DEBUG_LOG("Kymera_ScoGetCvcSend3MicMicConfig: mic_config %d", *mic_config);
}

void Kymera_ScoGetCvcSend3MicModeOfOperation(uint8 *mode_of_operation)
{
    *mode_of_operation = cvc_send_settings.mode_of_operation;
    DEBUG_LOG("Kymera_ScoGetCvcSend3MicModeOfOperation: mode_of_operation %d", *mode_of_operation);
}

static get_status_data_t* kymera_ScoGetCvcSend3MicStatusData(kymera_chain_handle_t chain_containing_cvc)
{
    Operator cvc_op;
    PanicFalse(GET_OP_FROM_CHAIN(cvc_op, chain_containing_cvc, OPR_CVC_SEND));
    get_status_data_t* get_status = OperatorsCreateGetStatusData(CVC_3_MIC_NUM_STATUS_VAR);
    OperatorsGetStatus(cvc_op, get_status);
    return get_status;
}

static void kymera_ScoGetCvcSend3MicStatusFlagsInChain(kymera_chain_handle_t chain_containing_cvc, uint32 *status_bitflag)
{
    if(chain_containing_cvc != NULL)
    {
        get_status_data_t* get_status = kymera_ScoGetCvcSend3MicStatusData(chain_containing_cvc);
        *status_bitflag = (uint32)(get_status->value[CVC_3_MIC_STATUS_BITFLAG]);
        free(get_status);
    }
}

void Kymera_ScoPollCvcSend3MicModeOfOperation(void)
{
    kymera_chain_handle_t sco_chain = kymera_GetScoChain();
    if(kymera_Is3MicCvcInScoChain(sco_chain))
    {
        uint32 status_bitflag = 0;
        kymera_ScoGetCvcSend3MicStatusFlagsInChain(sco_chain, &status_bitflag);
        uint8 mode_of_operation = status_bitflag & CVC_3_MIC_THREE_MIC_FLAG_MASK;
        if(mode_of_operation != cvc_send_settings.mode_of_operation)
        {
            /* Mode change detected -> send GAIA notification */
            DEBUG_LOG("Kymera_ScoPollCvcSend3MicModeOfOperation: Changed to %d", mode_of_operation);
            cvc_send_settings.mode_of_operation = mode_of_operation;
            kymeraTaskData *theKymera = KymeraGetTaskData();
            TaskList_MessageSendId(theKymera->listeners, KYMERA_NOTIFICATION_CVC_SEND_MODE_CHANGED);
        }
        MessageSendLater(KymeraGetTask(), KYMERA_INTERNAL_CVC_3MIC_POLL_MODE_OF_OPERATION,
                         NULL, POLL_SETTINGS_MS);
    }
}

static void kymera_ScoSetCvc3MicSettings(void)
{
    kymera_chain_handle_t sco_chain = kymera_GetScoChain();
    if(kymera_Is3MicCvcInScoChain(sco_chain))
    {
        if(cvc_send_settings.mode != KYMERA_CVC_NOTHING_SET)
        {
            Kymera_ScoSetCvcSend3MicMicConfig(cvc_send_settings.mic_config);
            Kymera_SetCvcPassthroughMode(cvc_send_settings.mode, cvc_send_settings.passthrough_mic);
        }
        else
        {
            DEBUG_LOG("kymera_ScoSetCvc3MicSettings: No valid settings found for 3Mic cVc");
        }
        /* Start polling for 3Mic cVc mode of operation */
        Kymera_ScoPollCvcSend3MicModeOfOperation();
    }
    else
    {
        DEBUG_LOG("kymera_ScoSetCvc3MicSettings: No 3Mic cVc capability found");
    }
}

bool Kymera_UpdateScoCvcSendSetting(kymera_cvc_mode_t mode, uint8 passthrough_mic)
{
    bool setting_changed = FALSE;
    if((cvc_send_settings.mode != mode) || (cvc_send_settings.passthrough_mic != passthrough_mic))
    {
        setting_changed = TRUE;
    }
    cvc_send_settings.mode = mode;
    cvc_send_settings.passthrough_mic = passthrough_mic;
    return setting_changed;
}

#endif /* INCLUDE_CVC_DEMO */
