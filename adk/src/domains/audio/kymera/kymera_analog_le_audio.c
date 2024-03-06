/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version
\file       kymera_analog_le_audio.c
\brief      Kymera Wired (Analog) to ISO Driver. Sample configuration:
            Line-In to ISO Chain (Line-In -> Re-Sampler -> Rate Adjust ->LC3-ENC -> ISO Sink)
*/

#ifdef INCLUDE_LE_AUDIO_ANALOG_SOURCE

#include "kymera.h"
#include "kymera_analog_le_audio.h"
#include "kymera_volume.h"
#include "kymera_data.h"
#include "kymera_setup.h"
#include "kymera_dsp_clock.h"
#include "kymera_tones_prompts.h"
#include "kymera_state.h"
#include "power_manager.h"
#include <operators.h>

#define MIXER_GAIN_RAMP_SAMPLES                 24000u

static const appKymeraAnalogIsoChainTable *analog_to_iso_chain_config_map = NULL;

static chain_operator_role_t kymeraAnalogLeAudioGetEncOperatorRole(uint16 codec_type, bool right_channel)
{
    chain_operator_role_t operator_role = OPR_LC3_ENCODE_SCO_ISO;

    switch (codec_type)
    {
        case KYMERA_LE_AUDIO_CODEC_LC3:
        {
            operator_role = right_channel ? OPR_LC3_ENCODE_SCO_ISO_RIGHT : OPR_LC3_ENCODE_SCO_ISO;
        }
        break;

        case KYMERA_LE_AUDIO_CODEC_APTX_LITE :
        {
            operator_role = right_channel ? OPR_APTX_LITE_ENCODE_SCO_ISO_RIGHT : OPR_APTX_LITE_ENCODE_SCO_ISO;
        }
        break;

        case KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE :
        {
            operator_role = right_channel ? OPR_APTX_ADAPTIVE_ENCODE_SCO_ISO_RIGHT : OPR_APTX_ADAPTIVE_ENCODE_SCO_ISO;
        }
        break;

        default:
            Panic();
        break;
    }

    return operator_role;
}

static kymera_chain_handle_t kymeraAnalogLeAudio_GetChain(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    return theKymera->chain_input_handle;
}

static Sink kymeraAnalogLeAudio_GetToAirSink(uint16 iso_handle)
{
    DEBUG_LOG("kymeraAnalogLeAudio_GetToAirSink Iso_handle %x", iso_handle);
    return PanicNull(StreamIsoSink(iso_handle));
}

static const chain_config_t* kymeraAnalogLeAudio_FindToAirConfigTable(uint16 to_air_stream_type, uint16 codec_type)
{
    uint8 index;
    appKymeraLeAudioEncoderConfigType chain_type;

    switch (to_air_stream_type)
    {
        case KYMERA_LE_STREAM_MONO:
        case KYMERA_LE_STREAM_STEREO_USE_LEFT:
        case KYMERA_LE_STREAM_STEREO_USE_RIGHT:
            chain_type = KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_SINGLE_MONO_CIS;
            break;

        case KYMERA_LE_STREAM_STEREO_USE_BOTH:
            chain_type = KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_SINGLE_STEREO_CIS;
            break;

        case KYMERA_LE_STREAM_DUAL_MONO:
            chain_type = KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_DUAL_MONO_CIS;
            break;

        default:
            chain_type = KYMERA_LE_AUDIO_ENCODER_CONFIG_TYPE_STEREO_TO_SINGLE_MONO_CIS;
            Panic();
            break;
    }

    DEBUG_LOG("kymeraAnalogLeAudio_FindToAirConfigTable for codec_type enum:appKymeraLeAudioCodec:%d, to_air_stream_type enum:appKymeraLeStreamType:%d, chain_type enum:appKymeraLeAudioEncoderConfigType:%d",
              codec_type, to_air_stream_type, chain_type);

    for (index = 0; index < analog_to_iso_chain_config_map->table_length; index++)
    {
        if (analog_to_iso_chain_config_map->chain_table[index].codec_type == codec_type &&
            analog_to_iso_chain_config_map->chain_table[index].chain_type == chain_type)
        {
            return analog_to_iso_chain_config_map->chain_table[index].chain;
        }
    }

    Panic();
    return NULL;
}

static kymera_chain_handle_t kymeraAnalogLeAudio_CreateAnalogToIsoChain(uint16 stream_type, uint8 codec_type)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    const chain_config_t *config = kymeraAnalogLeAudio_FindToAirConfigTable(stream_type, codec_type);

    DEBUG_LOG("kymeraAnalogLeAudio_CreateAnalogToIsoChain");
    theKymera->chain_input_handle = PanicNull(ChainCreate(config));

    /* Configure DSP power mode appropriately */
    appKymeraConfigureDspPowerMode();

    return theKymera->chain_input_handle;
}

static void kymeraAnalogLeAudio_ConfigureEncoder(le_media_config_t *to_air_params, unsigned operator_role)
{
    Operator op = ChainGetOperatorByRole(kymeraAnalogLeAudio_GetChain(), operator_role);

    DEBUG_LOG("kymeraAnalogLeAudio_ConfigureEncoder, codec type: enum:appKymeraLeAudioCodec:%d, stream type: enum:appKymeraLeStreamType:%d",
                                                    to_air_params->codec_type, to_air_params->stream_type);

    switch (to_air_params->codec_type)
    {
        case KYMERA_LE_AUDIO_CODEC_LC3:
        {
            OperatorsLc3EncoderScoIsoSetPacketLength(op, to_air_params->frame_length);
            OperatorsStandardSetSampleRate(op, to_air_params->sample_rate);
            OperatorsLc3EncoderScoIsoSetFrameDuration(op, to_air_params->frame_duration);
            OperatorsLc3EncoderScoIsoSetBlocksPerSdu(op, to_air_params->codec_frame_blocks_per_sdu);
            OperatorsLc3EncoderScoIsoSetErrorResilience(op, lc3_epc_low);

            if (to_air_params->stream_type == KYMERA_LE_STREAM_STEREO_USE_BOTH)
            {
                OperatorsLc3EncoderScoIsoSetNumOfChannels(op, 2);
            }
        }
        break;

        case KYMERA_LE_AUDIO_CODEC_APTX_LITE:
        {
            uint16 channel_mode = (to_air_params->stream_type == KYMERA_LE_STREAM_STEREO_USE_BOTH ? APTX_LITE_ISO_CHANNEL_MODE_JOINT_STEREO : APTX_LITE_ISO_CHANNEL_MODE_MONO);

            OperatorsAptxLiteEncoderScoIsoSetEncodingParams(op, to_air_params->sample_rate, channel_mode, to_air_params->frame_duration);
        }
        break;

        case KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE:
        {
            OperatorsAptxLiteEncoderScoIsoSetEncodingParams(op, to_air_params->sample_rate, APTX_ADAPTIVE_ISO_CHANNEL_MODE_MONO, to_air_params->frame_duration);
            OperatorsAptxAdaptiveTestModeScoIsoEncodingParams(op,3,300);
        }
        break;

        default:
            /* Codec not supported */
            Panic();
        break;
    }
}

static Source kymeraAnalogLeAudio_GetAnalogSource(audio_channel channel, uint8 inst, uint32 rate)
{
#define SAMPLE_SIZE 24
    Source source;
    analogue_input_params params = {
        .pre_amp = FALSE,
        .gain = 0x09, /* for line-in set to 0dB */
        .instance = 0, /* Place holder */
        .enable_24_bit_resolution = (SAMPLE_SIZE == 24)
        };

    DEBUG_LOG_VERBOSE("kymeraAnalogLeAudio_GetAnalogSource, channel: enum:audio_channel:%u,"
                                                        " instance: enum:audio_instance:%u,"
                                                        " rate: %u, size: %u", channel, inst, rate, SAMPLE_SIZE);
    params.instance = inst;
    source = AudioPluginAnalogueInputSetup(channel, params, rate);
    PanicFalse(SourceConfigure(source, STREAM_AUDIO_SAMPLE_SIZE, SAMPLE_SIZE));

    return source;
}

static void kymeraAnalogLeAudio_ConfigureAnalogToIsoChain(KYMERA_INTERNAL_ANALOG_LE_AUDIO_START_T *wired_le_audio)
{
    Operator ttp_pass_op = ChainGetOperatorByRole(kymeraAnalogLeAudio_GetChain(), OPR_LATENCY_BUFFER);
    Operator resampler_op = ChainGetOperatorByRole(kymeraAnalogLeAudio_GetChain(), OPR_SPEAKER_RESAMPLER);

    DEBUG_LOG_INFO("kymeraAnalogLeAudio_ConfigureAnalogToIsoChain: Wired sample rate %d, To Air sample rate %d",
                                wired_le_audio->sample_rate, wired_le_audio->to_air_params.sample_rate);
    DEBUG_LOG_INFO("\tWired min_latency_us %d, max_latency_us %d", wired_le_audio->min_latency_us, wired_le_audio->max_latency_us);
    DEBUG_LOG_INFO("\tWired target_latency_us %d", wired_le_audio->target_latency_us);

    OperatorsStandardSetSampleRate(ttp_pass_op, wired_le_audio->sample_rate);
    OperatorsSetPassthroughDataFormat(ttp_pass_op, operator_data_format_pcm);

    /* Configure the TTP passthrough operator.
     * TODO: Anything to be done here for LEA PRESENTATION_DELAY received? */
    OperatorsStandardSetLatencyLimits(ttp_pass_op, wired_le_audio->min_latency_us, wired_le_audio->max_latency_us);
    OperatorsStandardSetTimeToPlayLatency(ttp_pass_op, wired_le_audio->target_latency_us);
    OperatorsStandardSetBufferSizeWithFormat(ttp_pass_op, TTP_BUFFER_SIZE, operator_data_format_pcm);

    /* Configure Resampler operator */
    OperatorsResamplerSetConversionRate(resampler_op, wired_le_audio->sample_rate, wired_le_audio->to_air_params.sample_rate);

    /* Configure Encoder operators */
    kymeraAnalogLeAudio_ConfigureEncoder(&wired_le_audio->to_air_params, kymeraAnalogLeAudioGetEncOperatorRole(wired_le_audio->to_air_params.codec_type, FALSE));

    if (wired_le_audio->to_air_params.stream_type == KYMERA_LE_STREAM_DUAL_MONO)
    {
        kymeraAnalogLeAudio_ConfigureEncoder(&wired_le_audio->to_air_params, kymeraAnalogLeAudioGetEncOperatorRole(wired_le_audio->to_air_params.codec_type, TRUE));
    }

    if (wired_le_audio->to_air_params.stream_type == KYMERA_LE_STREAM_MONO)
    {
        DEBUG_LOG_INFO("kymeraAnalogLeAudio_ConfigureAnalogToIsoChain, configure Mixer for mono stream");
        Operator mixer_op = ChainGetOperatorByRole(kymeraAnalogLeAudio_GetChain(), OPR_LEFT_RIGHT_MIXER);
        DEBUG_LOG_INFO("kymeraAnalogLeAudio_ConfigureAnalogToIsoChain: resampler_op %x, mixer_op %x", resampler_op, mixer_op);
        OperatorsConfigureMixer(mixer_op, wired_le_audio->sample_rate, 1, GAIN_HALF, GAIN_HALF, GAIN_MIN, 1, 1, 0);
        OperatorsMixerSetNumberOfSamplesToRamp(mixer_op, MIXER_GAIN_RAMP_SAMPLES);
    }

    /* Configure DSP power mode appropriately for Wired chain */
    appKymeraConfigureDspPowerMode();
}

static void kymeraAnalogLeAudio_ConnectLineIn(KYMERA_INTERNAL_ANALOG_LE_AUDIO_START_T *wired_le_audio)
{
    DEBUG_LOG_FN_ENTRY("kymeraAnalogLeAudio_ConnectLineIn");

    Source line_in_l = kymeraAnalogLeAudio_GetAnalogSource(appConfigLeftAudioChannel(),
                                                           appConfigLeftAudioInstance(), wired_le_audio->sample_rate);
    Source line_in_r = kymeraAnalogLeAudio_GetAnalogSource(appConfigRightAudioChannel(),
                                                           appConfigRightAudioInstance(), wired_le_audio->sample_rate);

    /* If Stereo, then synchronize */
    if (line_in_r)
    {
        SourceSynchronise(line_in_l, line_in_r);
    }

    ChainConnectInput(KymeraGetTaskData()->chain_input_handle, line_in_l, EPR_WIRED_STEREO_INPUT_L);

    if (line_in_r)
    {
        ChainConnectInput(KymeraGetTaskData()->chain_input_handle, line_in_r, EPR_WIRED_STEREO_INPUT_R);
    }
}

static void kymeraAnalogLeAudio_DisconnectLineIn(void)
{
    DEBUG_LOG_FN_ENTRY("kymeraAnalogLeAudio_DisconnectLineIn");

    kymeraTaskData *theKymera = KymeraGetTaskData();

    Sink to_ttp_l = ChainGetInput(theKymera->chain_input_handle, EPR_WIRED_STEREO_INPUT_L);
    Sink to_ttp_r = ChainGetInput(theKymera->chain_input_handle, EPR_WIRED_STEREO_INPUT_R);

    DEBUG_LOG_V_VERBOSE("kymeraAnalogLeAudio_DisconnectLineIn, l-sink(%p), r-sink(%p)", to_ttp_l, to_ttp_r);

    StreamDisconnect(NULL, to_ttp_l);
    StreamDisconnect(NULL, to_ttp_r);
}

static void kymeraAnalogLeAudio_SetupToAirChain(KYMERA_INTERNAL_ANALOG_LE_AUDIO_START_T *wired_le_audio)
{
    /* Create To Air chain or Wired to ISO Sink chain */
    kymera_chain_handle_t wired_to_air_chain = kymeraAnalogLeAudio_CreateAnalogToIsoChain(wired_le_audio->to_air_params.stream_type,
                                                                                  wired_le_audio->to_air_params.codec_type);
    Source iso_ep_src_left = ChainGetOutput(wired_to_air_chain, EPR_ISO_TO_AIR_LEFT);


    DEBUG_LOG_INFO("kymeraAnalogLeAudio_SetupToAirChain");

    /* Configure To Air (Wired to ISO) chain specific operators */
    kymeraAnalogLeAudio_ConfigureAnalogToIsoChain(wired_le_audio);

    /* Connect chain inputs and outputs to endpoints */
    kymeraAnalogLeAudio_ConnectLineIn(wired_le_audio);
    StreamConnect(iso_ep_src_left, kymeraAnalogLeAudio_GetToAirSink(wired_le_audio->to_air_params.source_iso_handle));

    if (wired_le_audio->to_air_params.stream_type == KYMERA_LE_STREAM_DUAL_MONO)
    {
        Source iso_ep_src_right = ChainGetOutput(wired_to_air_chain, EPR_ISO_TO_AIR_RIGHT);
        StreamConnect(iso_ep_src_right, kymeraAnalogLeAudio_GetToAirSink(wired_le_audio->to_air_params.source_iso_handle_right));
    }

    ChainConnect(wired_to_air_chain);
    ChainStart(wired_to_air_chain);
}

void KymeraAnalogLeAudio_Start(KYMERA_INTERNAL_ANALOG_LE_AUDIO_START_T *wired_le_audio)
{
    DEBUG_LOG_INFO("kymeraAnalogLeAudio_Start");

    /* If there is a tone still playing at this point,
     * it must be an interruptible tone, so cut it off */
    appKymeraTonePromptStop();

    /* Can't start audio chain if we're not idle */
    PanicFalse(appKymeraGetState() == KYMERA_STATE_IDLE);

    appKymeraSetState(KYMERA_STATE_WIRED_LE_AUDIO_ACTIVE);

    /* Wired audio requires higher clock speeds, so request a switch to the "performance" power profile */
    appPowerPerformanceProfileRequest();

    /* Panic if there is no "to_air" data.
     * Note: This check is inherited from kymeraUsbLeAudio_Start().
     */
    PanicZero(wired_le_audio->to_air_params.codec_frame_blocks_per_sdu);

    /* Create and setup output chain i.e. To Air chain */
    kymeraAnalogLeAudio_SetupToAirChain(wired_le_audio);
}

void KymeraAnalogLeAudio_Stop(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    kymera_chain_handle_t wired_to_air_chain = kymeraAnalogLeAudio_GetChain();
    Source iso_ep_src_left;
    Source iso_ep_src_right;

    DEBUG_LOG_INFO("kymeraAnalogLeAudio_Stop");

    if (appKymeraGetState() != KYMERA_STATE_WIRED_LE_AUDIO_ACTIVE)
    {
        if (wired_to_air_chain == NULL)
        {
            /* Attempting to stop the chain when it's not ACTIVE.*/
            DEBUG_LOG_INFO("kymeraAnalogLeAudio_Stop, not stopping - already idle");
        }
        else
        {
            DEBUG_LOG_WARN("kymeraAnalogLeAudio_Stop, state %d, wired_to_air_chain %lu ",
                            appKymeraGetState(), wired_to_air_chain);
            Panic();
        }

        return;
    }

    ChainStop(wired_to_air_chain);

    /* Disconnect line input */
    kymeraAnalogLeAudio_DisconnectLineIn();

    /* Disconnect ISO output */
    iso_ep_src_left = ChainGetOutput(wired_to_air_chain, EPR_ISO_TO_AIR_LEFT);
    iso_ep_src_right = ChainGetOutput(wired_to_air_chain, EPR_ISO_TO_AIR_RIGHT);

    StreamDisconnect(iso_ep_src_left, NULL);
    StreamDisconnect(iso_ep_src_right, NULL);

    ChainDestroy(wired_to_air_chain);

    theKymera->chain_input_handle = NULL;

    /* No longer need to be in high performance power profile */
    appPowerPerformanceProfileRelinquish();

    /* Update state variables */
    appKymeraSetState(KYMERA_STATE_IDLE);
}

void KymeraAnalogLeAudio_SetToAirChainTable(const appKymeraAnalogIsoChainTable *chain_table)
{
    analog_to_iso_chain_config_map = chain_table;
}

#endif /* INCLUDE_LE_AUDIO_ANALOG_SOURCE */

