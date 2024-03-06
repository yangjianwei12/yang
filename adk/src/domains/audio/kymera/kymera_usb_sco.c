/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_usb_sco.c
\brief      Kymera USB to SCO Driver
*/

#include "kymera_usb_sco.h"
#include "kymera_dsp_clock.h"
#include "kymera_state.h"
#include "kymera_data.h"
#include "kymera_setup.h"
#include "kymera_tones_prompts.h"
#include "kymera_volume.h"
#include "kymera_output_chain_config.h"
#include "kymera_buffer_utils.h"
#include "kymera_source_sync.h"
#include "power_manager.h"

#define USB_VOICE_CHANNEL_MONO                 (1)
#define USB_VOICE_CHANNEL_STEREO               (2)

#define BPT_GAIN_MIN    (-120)
#define BPT_GAIN_FULL   (0)

#define MIXER_GAIN_RAMP_SAMPLES 24000

#define SCO_USB_VOICE_FRAME_SIZE            (2) /* 16 Bits */

#define SCO_NBS_SAMPLE_RATE 8000
#define SCO_WBS_SAMPLE_RATE 16000
#define SCO_SWBS_SAMPLE_RATE 32000

/*
 * Source Sync downstream buffer size calculation
 * This is the sum of the buffers downstream from the Source Sync operator.
 * NOTE: If the audio chain is altered, or the buffer values are changed,
 * then these values will need to be updated accordingly.
 */

/* downstream buffer sizes based on a chain with:
 * [Source Sync] --> buffer 1 --> [Encoder] --> buffer 2 --> [SCO Sink]
 * [NOTE] If the chain is modified, update the _BUFSZ values below
 */
#define NBS_SOSY_TO_ENCODER_BUFSZ    128
#define WBS_SOSY_TO_ENCODER_BUFSZ    256
#define SWBS_SOSY_TO_ENCODER_BUFSZ   1024

#define NBS_ENCODER_TO_ENDPOINT_BUFSZ   256
#define WBS_ENCODER_TO_ENDPOINT_BUFSZ   256
#define SWBS_ENCODER_TO_ENDPOINT_BUFSZ  256

#define SOSY_DOWNSTREAM_BUFFER_NBS  (NBS_SOSY_TO_ENCODER_BUFSZ + NBS_ENCODER_TO_ENDPOINT_BUFSZ)
#define SOSY_DOWNSTREAM_BUFFER_WBS  (WBS_SOSY_TO_ENCODER_BUFSZ + WBS_ENCODER_TO_ENDPOINT_BUFSZ)
#define SOSY_DOWNSTREAM_BUFFER_SWBS (SWBS_SOSY_TO_ENCODER_BUFSZ + SWBS_ENCODER_TO_ENDPOINT_BUFSZ)

/* Source Sync operator MAX_LATENCY settings in kick periods.
 * Note: kick period value is in usec/kick, so need to multiply by US_PER_SEC */
#define SOSY_MAX_LATENCY_NBS_KP    (US_PER_SEC * SOSY_DOWNSTREAM_BUFFER_NBS / SCO_NBS_SAMPLE_RATE / KICK_PERIOD_VOICE)
#define SOSY_MAX_LATENCY_WBS_KP    (US_PER_SEC * SOSY_DOWNSTREAM_BUFFER_WBS / SCO_WBS_SAMPLE_RATE / KICK_PERIOD_VOICE)
#define SOSY_MAX_LATENCY_SWBS_KP   (US_PER_SEC * SOSY_DOWNSTREAM_BUFFER_SWBS / SCO_SWBS_SAMPLE_RATE / KICK_PERIOD_VOICE)

static kymera_chain_handle_t sco_to_usb_voice_chain = NULL;

static kymera_chain_handle_t kymeraUsbScoVoice_GetChain(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    /* Create input chain */
    return theKymera->chain_input_handle;
}

static kymera_chain_handle_t kymeraUsbScoVoice_CreateUsbToScoChain(uint32_t sample_rate)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    const chain_config_t *config = NULL;

    switch (sample_rate)
    {
        case SCO_NBS_SAMPLE_RATE:
           config = Kymera_GetChainConfigs()->chain_usb_voice_nb_config;
        break;
        case SCO_WBS_SAMPLE_RATE:
            config = Kymera_GetChainConfigs()->chain_usb_voice_wb_config;
        break;
        case SCO_SWBS_SAMPLE_RATE:
            config = Kymera_GetChainConfigs()->chain_usb_voice_swb_config;
        break;
    default:
        DEBUG_LOG_ERROR("USB Voice: Invalid sample rate %d", sample_rate);
        Panic();
    }

    theKymera->chain_input_handle = PanicNull(ChainCreate(config));

    /* Configure DSP power mode appropriately for USB chain */
    appKymeraConfigureDspPowerMode();

    return theKymera->chain_input_handle;
}

static kymera_chain_handle_t kymeraUsbScoVoice_CreateScoToUsbChain(uint32_t sample_rate)
{
    const chain_config_t *config = NULL;
    kymera_chain_handle_t sco_to_usb_handle;

    switch (sample_rate)
    {
        case SCO_NBS_SAMPLE_RATE:
           config = Kymera_GetChainConfigs()->chain_sco_nb_config;
        break;
        case SCO_WBS_SAMPLE_RATE:
            config = Kymera_GetChainConfigs()->chain_sco_wb_config;
        break;
        case SCO_SWBS_SAMPLE_RATE:
            config = Kymera_GetChainConfigs()->chain_sco_swb_config;
        break;
    default:
        DEBUG_LOG_ERROR("USB Voice: Invalid sample rate %d", sample_rate);
        Panic();
    }

    sco_to_usb_handle = PanicNull(ChainCreate(config));

    return sco_to_usb_handle;
}

static void kymeraUsbScoVoice_ConfigureUsbToScoChain(KYMERA_INTERNAL_USB_SCO_VOICE_START_T *usb_voice)
{
    usb_config_t config;
    Operator usb_audio_rx_op = ChainGetOperatorByRole(kymeraUsbScoVoice_GetChain(), OPR_USB_AUDIO_RX);
    Operator resampler_op = ChainGetOperatorByRole(kymeraUsbScoVoice_GetChain(), OPR_SPEAKER_RESAMPLER);
    Operator rate_adjust_op = ChainGetOperatorByRole(kymeraUsbScoVoice_GetChain(), OPR_RATE_ADJUST);

    OperatorsResamplerSetConversionRate(resampler_op, usb_voice->spkr_sample_rate, usb_voice->sco_sample_rate);

    OperatorsStandardSetSampleRate(rate_adjust_op, usb_voice->sco_sample_rate);


    if(usb_voice->spkr_channels == USB_VOICE_CHANNEL_STEREO)
    {
        DEBUG_LOG_INFO("Kymera USB SCO USB_VOICE_CHANNEL_STEREO");
        Operator mixer_op = ChainGetOperatorByRole(kymeraUsbScoVoice_GetChain(), OPR_LEFT_RIGHT_MIXER);
        DEBUG_LOG_INFO("kymeraUsbScoVoice_ConfigureUsbToScoChain: resampler_op %x, mixer_op %x", resampler_op, mixer_op);
        OperatorsConfigureMixer(mixer_op, usb_voice->spkr_sample_rate, 1, GAIN_HALF, GAIN_HALF, GAIN_MIN, 1, 1, 0);
        OperatorsMixerSetNumberOfSamplesToRamp(mixer_op, MIXER_GAIN_RAMP_SAMPLES);
    }

    config.sample_rate = usb_voice->spkr_sample_rate;
    config.sample_size = usb_voice->spkr_frame_size;
    config.number_of_channels = usb_voice->spkr_channels;

    OperatorsConfigureUsbAudio(usb_audio_rx_op, config);

    OperatorsStandardSetLatencyLimits(usb_audio_rx_op,
                                      MS_TO_US(usb_voice->min_latency_ms),
                                      MS_TO_US(usb_voice->max_latency_ms));

    OperatorsStandardSetTimeToPlayLatency(usb_audio_rx_op, MS_TO_US(usb_voice->target_latency_ms));
    OperatorsStandardSetBufferSizeWithFormat(usb_audio_rx_op, TTP_BUFFER_SIZE,
                                                     operator_data_format_pcm);

    if ( ChainGetOperatorByRole(kymeraUsbScoVoice_GetChain(), OPR_SOURCE_SYNC) )
    {
        DEBUG_LOG_INFO("kymeraUsbScoVoice_ConfigureUsbToScoChain: Source Sync operator available" );

        /* configure the Source Sync operator to stream silence if the USB source isn't providing audio */
        kymera_output_chain_config source_sync_config =
        {
            .rate = usb_voice->sco_sample_rate,
            .source_sync_max_period = appKymeraGetFastKickSourceSyncPeriod(TRUE),
            .source_sync_min_period = appKymeraGetFastKickSourceSyncPeriod(FALSE),
            .source_sync_kick_back_threshold = APTX_ADAPTIVE_CODEC_BLOCK_SIZE,
            .kick_period = KICK_PERIOD_VOICE,
            .source_sync_input_buffer_size_samples = APTX_ADAPTIVE_CODEC_BLOCK_SIZE +
                                                    US_TO_BUFFER_SIZE_MONO_PCM(KICK_PERIOD_VOICE, usb_voice->sco_sample_rate)
                                                    + 1,
            .source_sync_output_buffer_size_samples = US_TO_BUFFER_SIZE_MONO_PCM((5.0 * KICK_PERIOD_VOICE) / 2.0, usb_voice->sco_sample_rate),
            .chain_type = output_chain_mono,
            .set_source_sync_min_period = TRUE,
            .set_source_sync_max_period = TRUE,
            .set_source_sync_kick_back_threshold = TRUE,
            .chain_include_aec = 0,
        };

        /* set the MAX_LATENCY based on the HFP codec type which we can deduce from the sample rate */
        switch (usb_voice->sco_sample_rate)
        {
            case SCO_NBS_SAMPLE_RATE:
                source_sync_config.source_sync_max_latency = MILLISECONDS_Q6_26(SOSY_MAX_LATENCY_NBS_KP);
            break;
            case SCO_WBS_SAMPLE_RATE:
                source_sync_config.source_sync_max_latency = MILLISECONDS_Q6_26(SOSY_MAX_LATENCY_WBS_KP);
            break;
            case SCO_SWBS_SAMPLE_RATE:
                source_sync_config.source_sync_max_latency = MILLISECONDS_Q6_26(SOSY_MAX_LATENCY_SWBS_KP);
            break;
            default:
                DEBUG_LOG_ERROR("USB Voice: Invalid sample rate %d", usb_voice->sco_sample_rate);
                Panic();
        }

        DEBUG_LOG_V_VERBOSE("kymeraUsbScoVoice_ConfigureUsbToScoChain: configure the Source Sync operator. ");
        DEBUG_LOG_V_VERBOSE("  .rate = %lu", source_sync_config.rate );
        DEBUG_LOG_V_VERBOSE("  .source_sync_max_period = %lu, .source_sync_min_period = %lu",
                            source_sync_config.source_sync_max_period>>26,
                            source_sync_config.source_sync_min_period>>26 );
        DEBUG_LOG_V_VERBOSE("  .source_sync_max_latency = %lu kick periods", source_sync_config.source_sync_max_latency>>26 );
        DEBUG_LOG_V_VERBOSE("  .source_sync_kick_back_threshold = %lu, .kick_period = %u",
                            source_sync_config.source_sync_kick_back_threshold,
                            source_sync_config.kick_period);
        DEBUG_LOG_V_VERBOSE("  .source_sync_input_buffer_size_samples = %u, .source_sync_output_buffer_size_samples = %u",
                            source_sync_config.source_sync_input_buffer_size_samples,
                            source_sync_config.source_sync_output_buffer_size_samples );
        DEBUG_LOG_V_VERBOSE("  .chain_type = enum:output_chain_t:%d", source_sync_config.chain_type );
        DEBUG_LOG_V_VERBOSE("  .set_source_sync_min_period = %u, .set_source_sync_max_period = %u",
                            source_sync_config.set_source_sync_min_period,
                            source_sync_config.set_source_sync_max_period );
        DEBUG_LOG_V_VERBOSE("  .set_source_sync_kick_back_threshold = %u",
                            source_sync_config.set_source_sync_kick_back_threshold );
        DEBUG_LOG_V_VERBOSE("  .chain_include_aec = %u", source_sync_config.chain_include_aec );
        appKymeraConfigureSourceSync(KymeraGetTaskData()->chain_input_handle,
                                     &source_sync_config,
                                     TRUE, FALSE);
    }   /* Source Sync Operator configuration */

    SinkConfigure(usb_voice->sco_sink, STREAM_RM_USE_RATE_ADJUST_OPERATOR, rate_adjust_op);
}

static void kymeraUsbScoVoice_ConfigureScoToUsbChain(KYMERA_INTERNAL_USB_SCO_VOICE_START_T *usb_voice)
{
    Operator sco_audio_rx_op = ChainGetOperatorByRole(sco_to_usb_voice_chain, OPR_SCO_RECEIVE);
    Operator resampler_op = ChainGetOperatorByRole(sco_to_usb_voice_chain, OPR_SPEAKER_RESAMPLER);
    Operator usb_audio_tx_op = ChainGetOperatorByRole(sco_to_usb_voice_chain, OPR_USB_AUDIO_TX);
    usb_config_t config;

    config.sample_rate = usb_voice->mic_sample_rate;
    config.sample_size = SCO_USB_VOICE_FRAME_SIZE;
    config.number_of_channels = USB_VOICE_CHANNEL_MONO;

    OperatorsConfigureUsbAudio(usb_audio_tx_op, config);
    OperatorsStandardSetBufferSizeWithFormat(usb_audio_tx_op, TTP_BUFFER_SIZE,
                                                     operator_data_format_pcm);

    Operator bpt_op = ChainGetOperatorByRole(sco_to_usb_voice_chain, OPR_BASIC_PASS);
    OperatorsSetPassthroughDataFormat(bpt_op, operator_data_format_pcm);
    OperatorsSetPassthroughGain(bpt_op, usb_voice->mute_status?BPT_GAIN_MIN:BPT_GAIN_FULL);

    OperatorsResamplerSetConversionRate(resampler_op, usb_voice->sco_sample_rate, usb_voice->mic_sample_rate);
    OperatorsStandardSetTimeToPlayLatency(sco_audio_rx_op, MS_TO_US(usb_voice->target_latency_ms));
    OperatorsStandardSetBufferSize(sco_audio_rx_op, TTP_BUFFER_SIZE);
}

static void kymeraUsbScoVoice_DestroyChain(KYMERA_INTERNAL_USB_SCO_VOICE_STOP_T *usb_voice)
{
    DEBUG_LOG("kymeraUsbScoVoice_DestroyChain");

    kymeraTaskData *theKymera = KymeraGetTaskData();
    kymera_chain_handle_t usb_to_sco_voice_chain = kymeraUsbScoVoice_GetChain();

    Sink usb_ep_snk = ChainGetInput(usb_to_sco_voice_chain, EPR_USB_FROM_HOST);
    Source sco_ep_src = ChainGetOutput(usb_to_sco_voice_chain, EPR_SCO_TO_AIR);

    Sink sco_ep_snk = ChainGetInput(sco_to_usb_voice_chain, EPR_SCO_FROM_AIR);
    Source usb_ep_src = ChainGetOutput(sco_to_usb_voice_chain, EPR_USB_TO_HOST);

    Source sco_source = StreamSourceFromSink(usb_voice->sco_sink);

    ChainStop(usb_to_sco_voice_chain);
    ChainStop(sco_to_usb_voice_chain);

    StreamDisconnect(usb_voice->spkr_src, NULL);
    StreamConnectDispose(usb_voice->spkr_src);
    StreamDisconnect(NULL, usb_ep_snk);

    StreamDisconnect(sco_ep_src, NULL);
    StreamDisconnect(NULL, usb_voice->sco_sink);

    StreamDisconnect(sco_source, NULL);
    StreamDisconnect(NULL, sco_ep_snk);

    StreamDisconnect(usb_ep_src, NULL);
    StreamDisconnect(NULL, usb_voice->mic_sink);

    ChainDestroy(usb_to_sco_voice_chain);
    ChainDestroy(sco_to_usb_voice_chain);

    theKymera->chain_input_handle = NULL;
    sco_to_usb_voice_chain = NULL;

    /* No longer need to be in high performance power profile */
    appPowerPerformanceProfileRelinquish();

    /* Update state variables */
    appKymeraSetState(KYMERA_STATE_IDLE);
}

void KymeraUsbScoVoice_Start(KYMERA_INTERNAL_USB_SCO_VOICE_START_T *usb_sco_voice)
{
    DEBUG_LOG("KymeraUsbScoVoice_Start");

    /* If there is a tone still playing at this point,
     * it must be an interruptible tone, so cut it off */
    appKymeraTonePromptStop();

    /* Can't start voice chain if we're not idle */
    PanicFalse(appKymeraGetState() == KYMERA_STATE_IDLE);

    Source sco_source = StreamSourceFromSink(usb_sco_voice->sco_sink);

    if (!sco_source)
    {
        /* SCO disconnected before we had chance to set up the chain. */
        DEBUG_LOG("KymeraUsbScoVoice_Start, no SCO source available");
        return;
    }

    appKymeraSetState(KYMERA_STATE_USB_SCO_VOICE_ACTIVE);

    /* USB audio requires higher clock speeds, so request a switch to the "performance" power profile */
    appPowerPerformanceProfileRequest();

    /* Create appropriate USB chain */
    kymera_chain_handle_t usb_to_sco_voice_chain = PanicNull(kymeraUsbScoVoice_CreateUsbToScoChain(usb_sco_voice->sco_sample_rate));

    sco_to_usb_voice_chain = PanicNull(kymeraUsbScoVoice_CreateScoToUsbChain(usb_sco_voice->sco_sample_rate));

    Sink usb_ep_snk = ChainGetInput(usb_to_sco_voice_chain, EPR_USB_FROM_HOST);
    Source sco_ep_src = ChainGetOutput(usb_to_sco_voice_chain, EPR_SCO_TO_AIR);

    Sink sco_ep_snk = ChainGetInput(sco_to_usb_voice_chain, EPR_SCO_FROM_AIR);
    Source usb_ep_src = ChainGetOutput(sco_to_usb_voice_chain, EPR_USB_TO_HOST);

    /* Configure chain specific operators */
    kymeraUsbScoVoice_ConfigureUsbToScoChain(usb_sco_voice);
    kymeraUsbScoVoice_ConfigureScoToUsbChain(usb_sco_voice);

    StreamDisconnect(sco_ep_src, NULL);
    StreamDisconnect(NULL, usb_ep_snk);

    StreamDisconnect(usb_sco_voice->spkr_src, NULL);
    StreamDisconnect(NULL, usb_sco_voice->sco_sink);

    StreamConnect(usb_sco_voice->spkr_src, usb_ep_snk);
    StreamConnect(sco_ep_src, usb_sco_voice->sco_sink);

    StreamConnect(sco_source, sco_ep_snk);
    StreamConnect(usb_ep_src, usb_sco_voice->mic_sink);

    ChainConnect(sco_to_usb_voice_chain);
    ChainConnect(usb_to_sco_voice_chain);

    if (!ChainStartAttempt(sco_to_usb_voice_chain) ||
        !ChainStartAttempt(usb_to_sco_voice_chain))
    {
        /* SCO likely disconnected during chain setup, tear down chains. */
        DEBUG_LOG("KymeraUsbScoVoice_Start, failed to start chains");

        KYMERA_INTERNAL_USB_SCO_VOICE_STOP_T usb_sco_stop;
        usb_sco_stop.spkr_src = usb_sco_voice->spkr_src;
        usb_sco_stop.mic_sink = usb_sco_voice->mic_sink;
        usb_sco_stop.sco_sink = usb_sco_voice->sco_sink;

        kymeraUsbScoVoice_DestroyChain(&usb_sco_stop);
    }
}

void KymeraUsbScoVoice_Stop(KYMERA_INTERNAL_USB_SCO_VOICE_STOP_T *usb_sco_stop)
{
    DEBUG_LOG("KymeraUsbScoVoice_Stop");

    kymera_chain_handle_t usb_to_sco_voice_chain = kymeraUsbScoVoice_GetChain();

    if (appKymeraGetState() == KYMERA_STATE_USB_SCO_VOICE_ACTIVE)
    {
        appKymeraTonePromptStop();
        kymeraUsbScoVoice_DestroyChain(usb_sco_stop);
    }
    else if (usb_to_sco_voice_chain)
    {
        /* Should never get here - chain should be destroyed if USB SCO voice not active. */
        DEBUG_LOG_WARN("KymeraUsbScoVoice_Stop, state enum:appKymeraState:%d, usb_voice_chain %lu",
                        appKymeraGetState(), usb_to_sco_voice_chain);
        Panic();
    }
    else
    {
        /* Attempted to stop voice chain when already stopped. This is ok, and
           can happen legitimately, for example if the chain failed to start we
           would have already torn it down. Proceed to call the stopped handler. */
        DEBUG_LOG_INFO("KymeraUsbScoVoice_Stop, not stopping, already idle");
    }

    PanicZero(usb_sco_stop->kymera_stopped_handler);
    usb_sco_stop->kymera_stopped_handler(usb_sco_stop->spkr_src);
}

void KymeraUsbScoVoice_MicMute(bool mute)
{
    DEBUG_LOG_INFO("KymeraUsbScoVoice_MicMute, mute %u", mute);
    Operator bpt_op = ChainGetOperatorByRole(sco_to_usb_voice_chain, OPR_BASIC_PASS);
    OperatorsSetPassthroughGain(bpt_op, mute?BPT_GAIN_MIN:BPT_GAIN_FULL);
}
