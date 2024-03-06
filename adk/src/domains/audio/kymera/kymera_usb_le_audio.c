/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_usb_le_audio.c
\brief      Kymera USB to ISO Driver, setsup below chain configurations
            USB to ISO Chain (USB RX -> Re-Sampler -> Rate Adjust ->LC3-ENC -> ISO Sink)
            ISO to USB Chain (ISO Src -> LC3 DEC -> Re-Sampler -> USB TX)
*/

#ifdef INCLUDE_LE_AUDIO_USB_SOURCE

#include "kymera_usb_le_audio.h"
#include "kymera_volume.h"
#include "kymera_data.h"
#include "kymera_setup.h"
#include "kymera_dsp_clock.h"
#include "kymera_tones_prompts.h"
#include "kymera_state.h"
#include "power_manager.h"
#include <operators.h>

#define ISO_USB_LE_FRAME_SIZE                   (2) /* 16 Bits*/

#define MIXER_GAIN_RAMP_SAMPLES                 24000u
#define USB_SINGLE_MIC_CHANNEL                  1
#define USB_DUAL_MIC_CHANNELS                   2

#ifdef USE_DUAL_MICROPHONE_FOR_LEA_USB_SOURCE
#define USB_NUMBER_OF_MIC_CHANNEL_COUNT         USB_DUAL_MIC_CHANNELS
#else
#define USB_NUMBER_OF_MIC_CHANNEL_COUNT         USB_SINGLE_MIC_CHANNEL
#endif

#define IS_QHS_LEVEL_VALID(qhs_level)           ((qhs_level > 0) && (qhs_level <= 6))

#define INVALID_ISO_HANDLE                      (0xFFFFu)

#define LC3_DECODE_SCO_ISO_BUFFER_SIZE          (1920u)

#define kymeraUsbLeAudio_FromAirInputStereo(iso_handle) (iso_handle != INVALID_ISO_HANDLE)

static const appKymeraUsbIsoChainTable *usb_to_iso_chain_config_map = NULL;
static const appKymeraIsoUsbChainTable *iso_to_usb_chain_config_map = NULL;

static kymera_chain_handle_t vbc_to_usb_chain = NULL;

static chain_operator_role_t kymeraUsbLeAudioGetEncOperatorRole(uint16 codec_type, bool right_channel) 
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

static chain_operator_role_t kymeraUsbLeAudioGetDecOperatorRole(uint16 codec_type, bool right_channel) 
{
    if (codec_type == KYMERA_LE_AUDIO_CODEC_LC3)
    {
        return right_channel ? OPR_LC3_DECODE_SCO_ISO_RIGHT : OPR_LC3_DECODE_SCO_ISO;
    }
    else if (codec_type == KYMERA_LE_AUDIO_CODEC_APTX_LITE)
    {
        return right_channel ? OPR_APTX_LITE_DECODE_SCO_ISO_RIGHT : OPR_APTX_LITE_DECODE_SCO_ISO;
    }
    else
    {
        Panic();
    }

    return 0;
}

static kymera_chain_handle_t kymeraUsbLeAudio_GetChain(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    return theKymera->chain_input_handle;
}

static Sink kymeraUsbLeAudio_GetToAirSink(uint16 iso_handle)
{
    DEBUG_LOG("kymeraUsbLeAudio_GetToAirSink Iso_handle %x", iso_handle);
    return PanicNull(StreamIsoSink(iso_handle));
}

static const chain_config_t* kymeraUsbLeAudio_FindToAirConfigTable(uint16 to_air_stream_type, uint16 codec_type)
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

    DEBUG_LOG("kymeraUsbLeAudio_FindToAirConfigTable for codec_type enum:appKymeraLeAudioCodec:%d, to_air_stream_type enum:appKymeraLeStreamType:%d, chain_type enum:appKymeraLeAudioEncoderConfigType:%d",
              codec_type, to_air_stream_type, chain_type);

    for (index = 0; index < usb_to_iso_chain_config_map->table_length; index++)
    {
        if (usb_to_iso_chain_config_map->chain_table[index].codec_type == codec_type &&
            usb_to_iso_chain_config_map->chain_table[index].chain_type == chain_type)
        {
            return usb_to_iso_chain_config_map->chain_table[index].chain;
        }
    }

    Panic();
    return NULL;
}

static const chain_config_t* kymeraUsbLeAudio_FindFromAirConfigTable(bool is_dual_cis, bool is_pts_mode, uint16 codec_type)
{
    uint8 index;
    appKymeraLeAudioDecoderConfigType chain_type;

    if (is_dual_cis)
    {
        chain_type = KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_DUAL_DECODER_TO_STEREO;
    }
    else
    {
#ifdef USE_DUAL_MICROPHONE_FOR_LEA_USB_SOURCE
        chain_type = is_pts_mode ? KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO :
                                   KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_STEREO;
#else
        UNUSED(is_pts_mode);
        chain_type = KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO;
#endif
    }

    DEBUG_LOG("kymeraUsbLeAudio_FindFromAirConfigTable for codec_type enum:appKymeraLeAudioCodec:%d, chain_type enum:appKymeraLeAudioDecoderConfigType:%d",
              codec_type, chain_type);

    for (index = 0; index < iso_to_usb_chain_config_map->table_length; index++)
    {
        if (iso_to_usb_chain_config_map->chain_table[index].codec_type  == codec_type &&
            iso_to_usb_chain_config_map->chain_table[index].chain_type == chain_type)
        {
            return iso_to_usb_chain_config_map->chain_table[index].chain;
        }
    }

    Panic();
    return NULL;
}

static kymera_chain_handle_t kymeraUsbLeAudio_CreateUsbToIsoChain(uint16 stream_type, uint8 codec_type)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    const chain_config_t *config = kymeraUsbLeAudio_FindToAirConfigTable(stream_type, codec_type);

    DEBUG_LOG("kymeraUsbLeAudio_CreateUsbToIsoChain()");
    theKymera->chain_input_handle = PanicNull(ChainCreate(config));
    /* Configure DSP power mode appropriately for USB chain */
    appKymeraConfigureDspPowerMode();

    return theKymera->chain_input_handle;
}

static kymera_chain_handle_t kymeraUsbLeAudio_CreateIsoToUsbChain(bool is_dual_cis, bool is_pts_mode, uint16 codec_type)
{
    const chain_config_t *config = kymeraUsbLeAudio_FindFromAirConfigTable(is_dual_cis, is_pts_mode, codec_type);
    kymera_chain_handle_t iso_to_usb_chain_handle = PanicNull(ChainCreate(config));

    return iso_to_usb_chain_handle;
}

static void kymeraUsbLeAudio_ConfigureEncoder(le_media_config_t *to_air_params, unsigned operator_role)
{
    Operator op = ChainGetOperatorByRole(kymeraUsbLeAudio_GetChain(), operator_role);

    DEBUG_LOG("kymeraUsbLeAudio_ConfigureEncoder() Codec Type enum:appKymeraLeAudioCodec:%d , stream_type %d", to_air_params->codec_type, to_air_params->stream_type);

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

static void kymeraUsbLeAudio_ConfigureDecoder(const le_microphone_config_t *from_air_params, unsigned operator_role)
{
    Operator op = ChainGetOperatorByRole(vbc_to_usb_chain, operator_role);

    if (operator_role == OPR_LC3_DECODE_SCO_ISO)
    {
        OperatorsLc3DecoderScoIsoSetPacketLength(op, from_air_params->frame_length);
        OperatorsLc3DecoderScoIsoSetFrameDuration(op, from_air_params->frame_duration);
    }
    else if (operator_role == OPR_APTX_LITE_DECODE_SCO_ISO || operator_role == OPR_APTX_LITE_DECODE_SCO_ISO_RIGHT)
    {
        OperatorsAptxLiteDecoderScoIsoSetFrameDuration(op, from_air_params->frame_duration);
    }

    DEBUG_LOG_INFO("kymeraUsbLeAudio_ConfigureDecoder(), ");
    DEBUG_LOG_INFO("DEC sample rate %d, TTP latency %dms, codec type %d", 
                    from_air_params->sample_rate, from_air_params->presentation_delay, from_air_params->codec_type);

    if (from_air_params->codec_frame_blocks_per_sdu > 1)
    {
        OperatorsLc3DecoderScoIsoSetBlocksPerSdu(op, from_air_params->codec_frame_blocks_per_sdu);
    }

    OperatorsStandardSetSampleRate(op, from_air_params->sample_rate);
    OperatorsStandardSetTimeToPlayLatency(op, from_air_params->presentation_delay);
    OperatorsStandardSetBufferSize(op, LC3_DECODE_SCO_ISO_BUFFER_SIZE);  /* TODO what would be the buffer size here? */
}

static void kymeraUsbLeAudio_ConfigureUsbToIsoChain(KYMERA_INTERNAL_USB_LE_AUDIO_START_T *usb_le_audio)
{
    usb_config_t config;
    Operator usb_audio_rx_op = ChainGetOperatorByRole(kymeraUsbLeAudio_GetChain(), OPR_USB_AUDIO_RX);
    Operator resampler_op = ChainGetOperatorByRole(kymeraUsbLeAudio_GetChain(), OPR_SPEAKER_RESAMPLER);
    Operator rate_adjust_op = ChainGetOperatorByRole(kymeraUsbLeAudio_GetChain(), OPR_RATE_ADJUST);

    DEBUG_LOG_INFO("kymeraUsbLeAudio_ConfigureUsbToIsoChain: USB host sample rate %d, To Air sample rate %d",
                                usb_le_audio->spkr_sample_rate, usb_le_audio->to_air_params.sample_rate);
    DEBUG_LOG_INFO("USB RX min_latency_us %d, max_latency_us %d", usb_le_audio->min_latency_us, usb_le_audio->max_latency_us);
    DEBUG_LOG_INFO("USB RX target_latency_us %d", usb_le_audio->target_latency_us);

    /* Configure USB RX operator */
    config.sample_rate = usb_le_audio->spkr_sample_rate;
    config.sample_size = usb_le_audio->spkr_frame_size;
    config.number_of_channels = usb_le_audio->spkr_channels;
    OperatorsConfigureUsbAudio(usb_audio_rx_op, config);
    OperatorsStandardSetTimeToPlayLatency(usb_audio_rx_op, usb_le_audio->target_latency_us);     /* TODO Anything to be done here for PRESENTATION_DELAY received ? */
    OperatorsStandardSetLatencyLimits(usb_audio_rx_op,usb_le_audio->min_latency_us, usb_le_audio->max_latency_us);
    OperatorsStandardSetBufferSizeWithFormat(usb_audio_rx_op, TTP_BUFFER_SIZE, operator_data_format_pcm);

    /* Configure Resampler operator */
    OperatorsResamplerSetConversionRate(resampler_op, usb_le_audio->spkr_sample_rate, usb_le_audio->to_air_params.sample_rate);

    /* Configure Rate Adjust  operator */
    OperatorsStandardSetSampleRate(rate_adjust_op, usb_le_audio->to_air_params.sample_rate);

    SinkConfigure(kymeraUsbLeAudio_GetToAirSink(usb_le_audio->to_air_params.source_iso_handle), STREAM_RM_USE_RATE_ADJUST_OPERATOR, rate_adjust_op);

    /* Configure Encoder operators */
    kymeraUsbLeAudio_ConfigureEncoder(&usb_le_audio->to_air_params, kymeraUsbLeAudioGetEncOperatorRole(usb_le_audio->to_air_params.codec_type, FALSE));

    if (usb_le_audio->to_air_params.stream_type == KYMERA_LE_STREAM_DUAL_MONO)
    {
        Operator rate_adjust_op_right = ChainGetOperatorByRole(kymeraUsbLeAudio_GetChain(), OPR_RATE_ADJUST_RIGHT);
        OperatorsStandardSetSampleRate(rate_adjust_op_right, usb_le_audio->to_air_params.sample_rate);
        SinkConfigure(kymeraUsbLeAudio_GetToAirSink(usb_le_audio->to_air_params.source_iso_handle_right), STREAM_RM_USE_RATE_ADJUST_OPERATOR, rate_adjust_op_right);
        kymeraUsbLeAudio_ConfigureEncoder(&usb_le_audio->to_air_params, kymeraUsbLeAudioGetEncOperatorRole(usb_le_audio->to_air_params.codec_type, TRUE));
    }

    if (usb_le_audio->to_air_params.stream_type == KYMERA_LE_STREAM_MONO)
    {
        DEBUG_LOG_INFO("Configure Mixer for KYMERA_LE_STREAM_MONO");
        Operator mixer_op = ChainGetOperatorByRole(kymeraUsbLeAudio_GetChain(), OPR_LEFT_RIGHT_MIXER);
        DEBUG_LOG_INFO("kymeraUsbLeAudio_ConfigureUsbToIsoChain: resampler_op %x, mixer_op %x", resampler_op, mixer_op);
        OperatorsConfigureMixer(mixer_op, usb_le_audio->spkr_sample_rate, 1, GAIN_HALF, GAIN_HALF, GAIN_MIN, 1, 1, 0);
        OperatorsMixerSetNumberOfSamplesToRamp(mixer_op, MIXER_GAIN_RAMP_SAMPLES);
    }

    /* Configure DSP power mode appropriately for USB chain */
    appKymeraConfigureDspPowerMode();
}

#ifdef USE_DUAL_MICROPHONE_FOR_LEA_USB_SOURCE
static void kymeraUsbLeAudio_ConfigureSplitter(void)
{
    Operator op = ChainGetOperatorByRole(vbc_to_usb_chain, OPR_LEA_USB_SPLT_ISO_RX);
    DEBUG_LOG_INFO("kymeraUsbLeAudio_ConfigureSplitter()");
    if(op)
    {
        OperatorsSplitterSetWorkingMode(op, splitter_mode_clone_input);
        OperatorsSplitterEnableSecondOutput(op, TRUE);
        OperatorsSplitterSetDataFormat(op, operator_data_format_pcm);
    }
}
#endif

static void kymeraUsbLeAudio_ConfigureIsoToUsbChain(KYMERA_INTERNAL_USB_LE_AUDIO_START_T *usb_le_audio)
{
    Operator resampler_op = ChainGetOperatorByRole(vbc_to_usb_chain, OPR_SPEAKER_RESAMPLER);
    Operator usb_audio_tx_op = ChainGetOperatorByRole(vbc_to_usb_chain, OPR_USB_AUDIO_TX);
    usb_config_t config;

    DEBUG_LOG_INFO("kymeraUsbLeAudio_ConfigureIsoToUsbChain()");
    DEBUG_LOG_INFO("USB host Mic sample rate %d, From Air sample rate %d",
                   usb_le_audio->mic_sample_rate, usb_le_audio->from_air_params.sample_rate);
    DEBUG_LOG_INFO("USB TX min_latency_us %d, max_latency_us %d, target_latency_us %d",
                   usb_le_audio->min_latency_us, usb_le_audio->max_latency_us, usb_le_audio->target_latency_us);

    /* Configure USB TX operator */
    config.sample_rate = usb_le_audio->mic_sample_rate;
    config.sample_size = ISO_USB_LE_FRAME_SIZE;

    config.number_of_channels = usb_le_audio->pts_mode &&
        usb_le_audio->from_air_params.source_iso_handle_right == INVALID_ISO_HANDLE ? USB_SINGLE_MIC_CHANNEL
                                                                                    : USB_NUMBER_OF_MIC_CHANNEL_COUNT;

    OperatorsConfigureUsbAudio(usb_audio_tx_op, config);
    OperatorsStandardSetBufferSizeWithFormat(usb_audio_tx_op, TTP_BUFFER_SIZE, operator_data_format_pcm);

    /* Configure Resampler operator */
    OperatorsResamplerSetConversionRate(resampler_op, usb_le_audio->from_air_params.sample_rate, usb_le_audio->mic_sample_rate);

#ifdef USE_DUAL_MICROPHONE_FOR_LEA_USB_SOURCE
    if(!kymeraUsbLeAudio_FromAirInputStereo(usb_le_audio->from_air_params.source_iso_handle_right))
    {
        /* Configure splitter in case of Mono VBC */
        kymeraUsbLeAudio_ConfigureSplitter();
    }
#endif

    /* Configure Decoder operators */
    kymeraUsbLeAudio_ConfigureDecoder(&usb_le_audio->from_air_params, kymeraUsbLeAudioGetDecOperatorRole(usb_le_audio->from_air_params.codec_type, FALSE));

    if (kymeraUsbLeAudio_FromAirInputStereo(usb_le_audio->from_air_params.source_iso_handle_right))
    {
        kymeraUsbLeAudio_ConfigureDecoder(&usb_le_audio->from_air_params, kymeraUsbLeAudioGetDecOperatorRole(usb_le_audio->from_air_params.codec_type, TRUE));
    }
}

static void kymeraUsbLeAudio_SetupToAirChain(KYMERA_INTERNAL_USB_LE_AUDIO_START_T *usb_le_audio)
{
    /* Create To Air chain or USB to ISO Sink chain */
    kymera_chain_handle_t usb_to_air_chain = kymeraUsbLeAudio_CreateUsbToIsoChain(usb_le_audio->to_air_params.stream_type,
                                                                                  usb_le_audio->to_air_params.codec_type);
    Sink usb_ep_snk = ChainGetInput(usb_to_air_chain, EPR_USB_FROM_HOST);
    Source iso_ep_src_left = ChainGetOutput(usb_to_air_chain, EPR_ISO_TO_AIR_LEFT);


    DEBUG_LOG_INFO("kymeraUsbLeAudio_SetupToAirChain()");

    /* Configure To Air (USB to ISO) chain specific operators */
    kymeraUsbLeAudio_ConfigureUsbToIsoChain(usb_le_audio);

    /* Cleanup the endpoints before setting up the connections */
    StreamDisconnect(iso_ep_src_left, NULL);
    StreamDisconnect(NULL, usb_ep_snk);
    StreamDisconnect(usb_le_audio->spkr_src, NULL);

    /* Connect chain inputs and outputs to endpoints */
    StreamConnect(usb_le_audio->spkr_src, usb_ep_snk);
    StreamConnect(iso_ep_src_left, kymeraUsbLeAudio_GetToAirSink(usb_le_audio->to_air_params.source_iso_handle));

    if (usb_le_audio->to_air_params.stream_type == KYMERA_LE_STREAM_DUAL_MONO)
    {
        Source iso_ep_src_right = ChainGetOutput(usb_to_air_chain, EPR_ISO_TO_AIR_RIGHT);

        StreamDisconnect(iso_ep_src_right, NULL);
        StreamConnect(iso_ep_src_right, kymeraUsbLeAudio_GetToAirSink(usb_le_audio->to_air_params.source_iso_handle_right));
    }

    ChainConnect(usb_to_air_chain);
    ChainStart(usb_to_air_chain);
}

static void kymeraUsbLeAudio_SetupFromAirChain(KYMERA_INTERNAL_USB_LE_AUDIO_START_T *usb_le_audio)
{
    Sink iso_ep_snk_left;
    Sink iso_ep_snk_right;
    Source iso_source_left;
    Source iso_source_right;
    Source usb_ep_src;
    bool is_dual_cis = kymeraUsbLeAudio_FromAirInputStereo(usb_le_audio->from_air_params.source_iso_handle_right);

    vbc_to_usb_chain = PanicNull(kymeraUsbLeAudio_CreateIsoToUsbChain(is_dual_cis, usb_le_audio->pts_mode, usb_le_audio->from_air_params.codec_type));

    iso_ep_snk_left = ChainGetInput(vbc_to_usb_chain, EPR_ISO_FROM_AIR_LEFT);
    usb_ep_src = ChainGetOutput(vbc_to_usb_chain, EPR_USB_TO_HOST);

    iso_source_left = PanicNull(StreamIsoSource(usb_le_audio->from_air_params.source_iso_handle));

    /* Configure From Air (ISO to USB) chain specific operators */
    kymeraUsbLeAudio_ConfigureIsoToUsbChain(usb_le_audio);

    /* Connect chain inputs and outputs to endpoints */
    StreamConnect(iso_source_left, iso_ep_snk_left);
    StreamConnect(usb_ep_src, usb_le_audio->mic_sink);

    if (is_dual_cis)
    {
        iso_ep_snk_right = ChainGetInput(vbc_to_usb_chain, EPR_ISO_FROM_AIR_RIGHT);
        iso_source_right = PanicNull(StreamIsoSource(usb_le_audio->from_air_params.source_iso_handle_right));
        StreamConnect(iso_source_right, iso_ep_snk_right);
        /* Synchronise ISO sources in case of stereo streams */
        PanicFalse(SourceSynchronise(iso_source_left, iso_source_right));
    }

    ChainConnect(vbc_to_usb_chain);
    ChainStart(vbc_to_usb_chain);
}

static void kymeraUsbLeAudio_StopVbc(void)
{
    if (vbc_to_usb_chain != NULL)
    {
        ChainStop(vbc_to_usb_chain);

        Sink iso_ep_snk_left = ChainGetInput(vbc_to_usb_chain, EPR_ISO_FROM_AIR_LEFT);
        Sink iso_ep_snk_right = ChainGetInput(vbc_to_usb_chain, EPR_ISO_FROM_AIR_RIGHT);
        Source usb_ep_src = ChainGetOutput(vbc_to_usb_chain, EPR_USB_TO_HOST);

        StreamDisconnect(usb_ep_src, NULL);
        StreamDisconnect(NULL, iso_ep_snk_left);
        StreamDisconnect(NULL, iso_ep_snk_right);
        ChainDestroy(vbc_to_usb_chain);
        vbc_to_usb_chain = NULL;
    }
    else
    {
        DEBUG_LOG_INFO("kymeraUsbLeAudio_StopVbc, not stopping -VBC chain is not running");
    }
}

void KymeraUsbLeAudio_Start(KYMERA_INTERNAL_USB_LE_AUDIO_START_T *usb_le_audio)
{
    DEBUG_LOG_INFO("KymeraUsbLeAudio_Start");

    /* If there is a tone still playing at this point,
     * it must be an interruptible tone, so cut it off */
    appKymeraTonePromptStop();

    /* Can't start voice chain if we're not idle */
    PanicFalse(appKymeraGetState() == KYMERA_STATE_IDLE);

    appKymeraSetState(KYMERA_STATE_USB_LE_AUDIO_ACTIVE);

    /* USB audio requires higher clock speeds, so request a switch to the "performance" power profile */
    appPowerPerformanceProfileRequest();

    /* If there is only "from_air" data, no need to setup the "to air" chain
     * Note: This check is added as this was causing a panic when running 
     *       PTS Qual runs which tests only voice mono audio 
     */
    if (usb_le_audio->to_air_params.codec_frame_blocks_per_sdu != 0)
    {
        /* Create and setup output chain i.e. To Air chain */
        kymeraUsbLeAudio_SetupToAirChain(usb_le_audio);

        if (usb_le_audio->to_air_params.codec_type == KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE)
        {
            DEBUG_LOG_INFO("Calling Kymera_UsbLeAudioApplyQhsRate");
            Kymera_UsbLeAudioApplyQhsRate(usb_le_audio->qhs_level);
        }
    }

    /* Create From Air chain or ISO to USB chain when VBC is enabled */
    if (usb_le_audio->vbc_enabled)
    {
        kymeraUsbLeAudio_SetupFromAirChain(usb_le_audio);
    }
}

void KymeraUsbLeAudio_Stop(KYMERA_INTERNAL_USB_LE_AUDIO_STOP_T *usb_le_audio_stop)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    kymera_chain_handle_t usb_to_air_chain = kymeraUsbLeAudio_GetChain();
    Sink usb_ep_snk;
    Source iso_ep_src_left;
    Source iso_ep_src_right;

    DEBUG_LOG_INFO("KymeraUsbLeAudio_Stop() ");

    if (appKymeraGetState() != KYMERA_STATE_USB_LE_AUDIO_ACTIVE)
    {
        if (usb_to_air_chain == NULL)
        {
            /* Attempting to stop a USB Voice chain when not ACTIVE.*/
            DEBUG_LOG_INFO("KymeraUsbLeAudio_Stop, not stopping - already idle");
        }
        else
        {
            DEBUG_LOG_WARN("KymeraUsbLeAudio_Stop, state %d, usb_to_air_chain %lu ",
                            appKymeraGetState(), usb_to_air_chain);
            Panic();
        }

        PanicZero(usb_le_audio_stop->kymera_stopped_handler);
        usb_le_audio_stop->kymera_stopped_handler(usb_le_audio_stop->spkr_src);
        return;
    }

    appKymeraTonePromptStop();
    kymeraUsbLeAudio_StopVbc();
    ChainStop(usb_to_air_chain);

   usb_ep_snk = ChainGetInput(usb_to_air_chain, EPR_USB_FROM_HOST);
   iso_ep_src_left = ChainGetOutput(usb_to_air_chain, EPR_ISO_TO_AIR_LEFT);
   iso_ep_src_right = ChainGetOutput(usb_to_air_chain, EPR_ISO_TO_AIR_RIGHT);

    StreamDisconnect(NULL, usb_le_audio_stop->mic_sink);
    StreamDisconnect(usb_le_audio_stop->spkr_src, NULL);
    StreamConnectDispose(usb_le_audio_stop->spkr_src);

    StreamDisconnect(NULL, usb_ep_snk);

    StreamDisconnect(iso_ep_src_left, NULL);
    StreamDisconnect(iso_ep_src_right, NULL);

    ChainDestroy(usb_to_air_chain);

    theKymera->chain_input_handle = NULL;

    /* No longer need to be in high performance power profile */
    appPowerPerformanceProfileRelinquish();

    /* Update state variables */
    appKymeraSetState(KYMERA_STATE_IDLE);

    PanicZero(usb_le_audio_stop->kymera_stopped_handler);
    usb_le_audio_stop->kymera_stopped_handler(usb_le_audio_stop->spkr_src);
}

void Kymera_UsbLeAudioSetToAirChainTable(const appKymeraUsbIsoChainTable *chain_table)
{
    usb_to_iso_chain_config_map = chain_table;
}

void Kymera_UsbLeAudioSetFromAirChainTable(const appKymeraIsoUsbChainTable *chain_table)
{
    iso_to_usb_chain_config_map = chain_table;
}

void Kymera_UsbLeAudioApplyQhsRate(uint16 qhs_level)
{
    Operator aptx_adaptive_op_left = ChainGetOperatorByRole(kymeraUsbLeAudio_GetChain(), OPR_APTX_ADAPTIVE_ENCODE_SCO_ISO);
    Operator aptx_adaptive_op_right = ChainGetOperatorByRole(kymeraUsbLeAudio_GetChain(), OPR_APTX_ADAPTIVE_ENCODE_SCO_ISO_RIGHT);

    if(aptx_adaptive_op_left && aptx_adaptive_op_right && IS_QHS_LEVEL_VALID(qhs_level))
    {
        DEBUG_LOG_INFO("Kymera_UsbLeAudioApplyQhsRate,QHS level %d", qhs_level);

        OperatorsAptxAdaptiveEncoderScoIsoSetQhsLevel(aptx_adaptive_op_left, qhs_level);
        OperatorsAptxAdaptiveEncoderScoIsoSetQhsLevel(aptx_adaptive_op_right, qhs_level);
    }
}

#endif /* INCLUDE_LE_AUDIO_USB_SOURCE */

