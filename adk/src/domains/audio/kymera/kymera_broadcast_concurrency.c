/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera broadcast concurrency support for A2DP, LE Unicast, etc
*/

#if defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER)

#include "kymera_broadcast_concurrency.h"
#include "kymera_setup.h"


/* Splitter buffer size */
#define LEA_SPLT_ISO_TX_BUFFER_SIZE     4096

static chain_operator_role_t appKymeraAudioGetEncOperatorRole(uint16 codec_type, bool right_channel)
{
    if (codec_type == KYMERA_LE_AUDIO_CODEC_LC3)
    {
        return right_channel ? OPR_LC3_ENCODE_SCO_ISO_RIGHT : OPR_LC3_ENCODE_SCO_ISO;
    }
    else
    {
        Panic();
    }
    return 0;
}

static void appKymeraConfigureLc3Encoder(kymera_chain_handle_t chain, le_media_config_t *lea_broadcast_params, unsigned operator_role)
{
    PanicNull(lea_broadcast_params);
    Operator op = ChainGetOperatorByRole(chain, operator_role);

    DEBUG_LOG("appKymeraConfigureLc3Encoder, Sample rate = %lu, frame length = %lu, frame duration = %lu, stream type = %lu, presentation delay = %lu, codec type = %d, codec frame blocks per sdu = %d",
              lea_broadcast_params->sample_rate,
              lea_broadcast_params->frame_length,
              lea_broadcast_params->frame_duration,
              lea_broadcast_params->stream_type,
              lea_broadcast_params->presentation_delay,
              lea_broadcast_params->codec_type,
              lea_broadcast_params->codec_frame_blocks_per_sdu);

    if(op)
    {
        switch (lea_broadcast_params->codec_type)
        {
            case KYMERA_LE_AUDIO_CODEC_LC3:
            {
                OperatorsLc3EncoderScoIsoSetPacketLength(op, lea_broadcast_params->frame_length);
                OperatorsStandardSetSampleRate(op, lea_broadcast_params->sample_rate);
                OperatorsLc3EncoderScoIsoSetFrameDuration(op, lea_broadcast_params->frame_duration);
                OperatorsLc3EncoderScoIsoSetBlocksPerSdu(op, lea_broadcast_params->codec_frame_blocks_per_sdu);

                if (lea_broadcast_params->stream_type == KYMERA_LE_STREAM_STEREO_USE_BOTH)
                {
                    OperatorsLc3EncoderScoIsoSetNumOfChannels(op, 2);
                }
            }
            break;

            default:
                /* Codec not supported */
                Panic();
            break;
        }
    }
}

#ifdef ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT
/* Since LC3 decoder needs to wait for additional time for audio sync, need additional buffer */
#define LC3_DECODE_SCO_ISO_BUFFER_SIZE 4096
#define DECODE_ONLY_LEFT   0
#define DECODE_ONLY_RIGHT  1

static void appKymeraDecoderForStreamType(Operator op, appKymeraLeStreamType stream_type)
{
    switch (stream_type)
    {
        case KYMERA_LE_STREAM_MONO:
            /* No action */
            break;

        case KYMERA_LE_STREAM_STEREO_USE_LEFT:
        case KYMERA_LE_STREAM_STEREO_USE_RIGHT:
            /* Set number of channels before configuring the Mono Decoder */
            OperatorsLc3DecoderScoIsoSetNumOfChannels(op, 2);
            OperatorsLc3DecoderScoIsoSetMonoDecode(op,
                stream_type == KYMERA_LE_STREAM_STEREO_USE_LEFT ? DECODE_ONLY_LEFT : DECODE_ONLY_RIGHT);
            break;

        case KYMERA_LE_STREAM_STEREO_USE_BOTH:
            OperatorsLc3DecoderScoIsoSetNumOfChannels(op, 2);
            break;

        case KYMERA_LE_STREAM_DUAL_MONO:
            /* No action */
            break;

        default:
            break;
    }
}

static void appKymeraConfigureLc3Decoder(Operator op, const le_media_config_t *lea_broadcast_params)
{
    uint32 audio_sync_ttp = 0;
    OperatorsLc3DecoderScoIsoSetPacketLength(op, lea_broadcast_params->frame_length);
    OperatorsLc3DecoderScoIsoSetFrameDuration(op, lea_broadcast_params->frame_duration);

    DEBUG_LOG("appKymeraConfigureLc3Decoder op_id %x", op);
    appKymeraDecoderForStreamType(op, lea_broadcast_params->stream_type);

    if (lea_broadcast_params->codec_frame_blocks_per_sdu > 1)
    {
        OperatorsLc3DecoderScoIsoSetBlocksPerSdu(op, lea_broadcast_params->codec_frame_blocks_per_sdu);
    }

    OperatorsStandardSetSampleRate(op, lea_broadcast_params->sample_rate);
    /* This value is equal to BIG transport latency + presentation delay on the receiver */
    audio_sync_ttp = lea_broadcast_params->transport_latency_big + lea_broadcast_params->presentation_delay;
    DEBUG_LOG("appKymeraConfigureDecoder Audio Sync TTP %x", audio_sync_ttp);
    OperatorsStandardSetTimeToPlayLatency(op, audio_sync_ttp);
    OperatorsStandardSetBufferSize(op, LC3_DECODE_SCO_ISO_BUFFER_SIZE);
}

void KymeraBroadcastConcurrency_ConfigureTranscodingChain(uint32 rate)
{
    /* Configure the necessary operators if transcoding is done at broadcaster */
    kymeraTaskData *theKymera = KymeraGetTaskData();
    Operator resampler_op = ChainGetOperatorByRole(theKymera->chain_input_handle, OPR_SPEAKER_RESAMPLER);
    if(resampler_op)
    {
        DEBUG_LOG("KymeraBroadcastConcurrency_ConfigureTranscodingChain, Spk Sample rate: %x, Iso Sample rate %x", rate, theKymera->lea_broadcast_params->sample_rate);
        OperatorsResamplerSetConversionRate(resampler_op, rate, theKymera->lea_broadcast_params->sample_rate);
    }

    /* Configure LC3 Encoder & Decoder */
    appKymeraConfigureLc3Encoder(theKymera->chain_input_handle, theKymera->lea_broadcast_params, appKymeraAudioGetEncOperatorRole(theKymera->lea_broadcast_params->codec_type, FALSE));
    appKymeraConfigureLc3Decoder(ChainGetOperatorByRole(theKymera->chain_input_handle, OPR_LC3_DECODE_SCO_ISO), theKymera->lea_broadcast_params);
    if (theKymera->lea_broadcast_params->stream_type == KYMERA_LE_STREAM_DUAL_MONO)
    {
        appKymeraConfigureLc3Encoder(theKymera->chain_input_handle, theKymera->lea_broadcast_params, appKymeraAudioGetEncOperatorRole(theKymera->lea_broadcast_params->codec_type, TRUE));
        appKymeraConfigureLc3Decoder(ChainGetOperatorByRole(theKymera->chain_input_handle, OPR_LC3_DECODE_SCO_ISO_RIGHT), theKymera->lea_broadcast_params);
    }
}
#else
void KymeraBroadcastConcurrency_ConfigureTTPLatencyBuffer(uint32 rate)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    PanicNull(theKymera->lea_broadcast_params);
    Operator op_ttp = ChainGetOperatorByRole(theKymera->chain_input_handle, OPR_LEA_CONC_LATENCY_BUFFER);
    uint32 audio_sync_ttp = theKymera->lea_broadcast_params->transport_latency_big + theKymera->lea_broadcast_params->presentation_delay;
    DEBUG_LOG("appKymeraConfigureLatencyBuffer op = %lu, rate = %lu, ttp delay = %lu", op_ttp, rate, audio_sync_ttp);

    if(op_ttp)
    {
        OperatorsStandardSetSampleRate(op_ttp, rate);
        OperatorsSetPassthroughDataFormat(op_ttp, operator_data_format_pcm);

        /* Configure the TTP passthrough operator. */
        OperatorsStandardSetTimeToPlayLatency(op_ttp, audio_sync_ttp);
        OperatorsStandardSetBufferSizeWithFormat(op_ttp, TTP_BUFFER_SIZE, operator_data_format_pcm);
    }
}
#endif /* ENABLE_LE_AUDIO_TRANSCODE_BROADCAST_SUPPORT */

void KymeraBroadcastConcurrency_ConfigureBroadcastSplitter(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    Operator op_splitter = ChainGetOperatorByRole(theKymera->chain_input_handle, OPR_LEA_SPLT_ISO_TX);
    if(op_splitter)
    {
        OperatorsStandardSetBufferSize(op_splitter, LEA_SPLT_ISO_TX_BUFFER_SIZE);
        OperatorsSplitterSetDataFormat(op_splitter, operator_data_format_pcm);
    }
}

void KymeraBroadcastConcurrency_CreateToAirChain(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    const chain_config_t *config = NULL;
    PanicNull(theKymera);
    PanicNull(theKymera->lea_broadcast_params);

    if (Kymera_IsAudioBroadcasting())
    {
        DEBUG_LOG("KymeraBroadcastConcurrency_CreateToAirChain");
        switch (theKymera->lea_broadcast_params->stream_type)
        {
            case KYMERA_LE_STREAM_STEREO_USE_BOTH:
                config = Kymera_GetChainConfigs()->chain_lc3_joint_stereo_to_iso_config;
                break;

            case KYMERA_LE_STREAM_DUAL_MONO:
                config = Kymera_GetChainConfigs()->chain_lc3_stereo_to_iso_config;
                break;

            default:
                config = Kymera_GetChainConfigs()->chain_lc3_stereo_to_iso_config;
                break;
        }

        /* Create To Air chain */
        theKymera->chain_to_air_handle = PanicNull(ChainCreate(config));
    }
}

void KymeraBroadcastConcurrency_ConfigureToAirChain(uint32 rate)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    source_defined_params_t source_params_lea;
    le_audio_connect_parameters_t *lea_params_data;
    PanicNull(theKymera);
    PanicNull(theKymera->lea_broadcast_params);

    Operator resampler_op = ChainGetOperatorByRole(theKymera->chain_to_air_handle, OPR_SPEAKER_RESAMPLER);
    Operator rate_adjust_op = ChainGetOperatorByRole(theKymera->chain_to_air_handle, OPR_RATE_ADJUST);

    /* if we are broadcasting, then we can get the lea audio configs */
    if(Kymera_IsAudioBroadcasting())
    {
        PanicFalse(AudioSources_GetConnectParameters(audio_source_le_audio_broadcast_sender, &source_params_lea));
        PanicNull(source_params_lea.data);
        lea_params_data = source_params_lea.data;
        le_media_config_t to_air_params = lea_params_data->media;
        /* Release the connect parameters */
        AudioSources_ReleaseConnectParameters(audio_source_le_audio_broadcast, &source_params_lea);

        /* Configure To Air chain specific operators */
        /* Configure Resampler operator */
        if(resampler_op)
        {
            DEBUG_LOG("KymeraBroadcastConcurrency_ConfigureToAirChain, Spk Sample rate: %x, Iso Sample rate %x", rate, to_air_params.sample_rate);
            OperatorsResamplerSetConversionRate(resampler_op, rate, to_air_params.sample_rate);
        }
        /* Configure Rate Adjust operator */
        if(rate_adjust_op)
        {
            OperatorsStandardSetSampleRate(rate_adjust_op, to_air_params.sample_rate);
            SinkConfigure(StreamIsoSink(to_air_params.source_iso_handle), STREAM_RM_USE_RATE_ADJUST_OPERATOR, rate_adjust_op);
        }

        /* Configure Encoder operators */
        appKymeraConfigureLc3Encoder(theKymera->chain_to_air_handle, &to_air_params, appKymeraAudioGetEncOperatorRole(to_air_params.codec_type, FALSE));

        if (to_air_params.stream_type == KYMERA_LE_STREAM_DUAL_MONO)
        {
            Operator rate_adjust_op_right = ChainGetOperatorByRole(theKymera->chain_to_air_handle, OPR_RATE_ADJUST_RIGHT);
            if(rate_adjust_op_right)
            {
                OperatorsStandardSetSampleRate(rate_adjust_op_right, to_air_params.sample_rate);
                SinkConfigure(StreamIsoSink(to_air_params.source_iso_handle_right), STREAM_RM_USE_RATE_ADJUST_OPERATOR, rate_adjust_op_right);
            }
            appKymeraConfigureLc3Encoder(theKymera->chain_to_air_handle, &to_air_params, appKymeraAudioGetEncOperatorRole(theKymera->lea_broadcast_params->codec_type, TRUE));
        }
        ChainConnect(theKymera->chain_to_air_handle);
    }
}

void KymeraBroadcastConcurrency_JoinToAirChain(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    PanicNull(theKymera);
    source_defined_params_t source_params_lea;
    le_audio_connect_parameters_t *lea_params_data;
    le_media_config_t to_air_params;
    Operator op_splitter = ChainGetOperatorByRole(theKymera->chain_input_handle, OPR_LEA_SPLT_ISO_TX);
    Source iso_ep_src_left = ChainGetOutput(theKymera->chain_to_air_handle, EPR_ISO_TO_AIR_LEFT);

    if (Kymera_IsAudioBroadcasting())
    {
        DEBUG_LOG("KymeraBroadcastConcurrency_JoinToAirChain");
        PanicFalse(AudioSources_GetConnectParameters(audio_source_le_audio_broadcast_sender, &source_params_lea));
        lea_params_data = source_params_lea.data;
        to_air_params = lea_params_data->media;
        /* Release the connect parameters */
        AudioSources_ReleaseConnectParameters(audio_source_le_audio_broadcast, &source_params_lea);
        /* Cleanup the endpoints before setting up the connections */
        StreamDisconnect(iso_ep_src_left, NULL);

        /* Disable the second outputs of the splitter and enable them back after the connection of endpoints */
        OperatorsSplitterEnableSecondOutput(op_splitter, FALSE);
        /* Connect chain inputs and outputs to endpoints */
        PanicFalse(StreamConnect(ChainGetOutput(theKymera->chain_input_handle, EPR_LEA_CONC_SPLT_ISO_LEFT), ChainGetInput(theKymera->chain_to_air_handle, EPR_ISO_FROM_AIR_LEFT)));
        PanicFalse(StreamConnect(ChainGetOutput(theKymera->chain_input_handle, EPR_LEA_CONC_SPLT_ISO_RIGHT), ChainGetInput(theKymera->chain_to_air_handle, EPR_ISO_FROM_AIR_RIGHT)));
        PanicFalse(StreamConnect(iso_ep_src_left, StreamIsoSink(to_air_params.source_iso_handle)));
        OperatorsSplitterEnableSecondOutput(op_splitter, TRUE);

        if (to_air_params.stream_type == KYMERA_LE_STREAM_DUAL_MONO)
        {
            Source iso_ep_src_right = ChainGetOutput(theKymera->chain_to_air_handle, EPR_ISO_TO_AIR_RIGHT);
            StreamDisconnect(iso_ep_src_right, NULL);
            PanicFalse(StreamConnect(iso_ep_src_right, StreamIsoSink(to_air_params.source_iso_handle_right)));
        }
    }
}

void KymeraBroadcastConcurrency_StartToAirChain(void)
{
    if (Kymera_IsAudioBroadcasting())
    {
        /* Start To Air chain */
        DEBUG_LOG("KymeraBroadcastConcurrency_StartToAirChain");
        kymeraTaskData *theKymera = KymeraGetTaskData();
        PanicNull(theKymera);
        ChainStart(theKymera->chain_to_air_handle);
    }
}

void KymeraBroadcastConcurrency_DisconnectToAirChain(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    PanicNull(theKymera);
    source_defined_params_t source_params_lea;
    le_audio_connect_parameters_t *lea_params_data;
    le_media_config_t to_air_params;
    Source iso_ep_src_left = ChainGetOutput(theKymera->chain_input_handle, EPR_ISO_TO_AIR_LEFT);

    DEBUG_LOG("KymeraBroadcastConcurrency_DisconnectToAirChain");
    /* Stop the chain before disconnection */
    ChainStop(theKymera->chain_to_air_handle);

    PanicFalse(AudioSources_GetConnectParameters(audio_source_le_audio_broadcast_sender, &source_params_lea));
    lea_params_data = source_params_lea.data;
    to_air_params = lea_params_data->media;
    /* Release the connect parameters */
    AudioSources_ReleaseConnectParameters(audio_source_le_audio_broadcast, &source_params_lea);
    /* Cleanup the endpoints before setting up the connections */
    StreamDisconnect(iso_ep_src_left, NULL);
    StreamDisconnect(ChainGetOutput(theKymera->chain_input_handle, EPR_LEA_CONC_SPLT_ISO_LEFT), NULL);
    StreamDisconnect(ChainGetOutput(theKymera->chain_input_handle, EPR_LEA_CONC_SPLT_ISO_RIGHT), NULL);
    if (to_air_params.stream_type == KYMERA_LE_STREAM_DUAL_MONO)
    {
        Source iso_ep_src_right = ChainGetOutput(theKymera->chain_input_handle, EPR_ISO_TO_AIR_RIGHT);
        StreamDisconnect(iso_ep_src_right, NULL);
    }
}

void KymeraBroadcastConcurrency_DestroyToAirChain(void)
{
    /* Destroy To Air chain */
    DEBUG_LOG("KymeraBroadcastConcurrency_DestroyToAirChain");
    kymeraTaskData *theKymera = KymeraGetTaskData();
    PanicNull(theKymera);
    ChainDestroy(theKymera->chain_to_air_handle);
    theKymera->chain_to_air_handle = NULL;
}

void KymeraBroadcastConcurrency_SetLeAudioBroadcastingStatus(bool is_broadcasting_enabled)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    PanicNull(theKymera);
    theKymera->is_audio_broadcasting = is_broadcasting_enabled;
}

bool KymeraBroadcastConcurrency_GetLeAudioBroadcastingStatus(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    PanicNull(theKymera);
    return theKymera->is_audio_broadcasting;
}

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) */
