/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle LE Audio chain

*/

#include "kymera_le_audio.h"
#include "kymera_dsp_clock.h"
#include "kymera_state.h"
#include "kymera_internal_msg_ids.h"
#include "kymera_source_sync.h"
#include "kymera_music_processing.h"
#include "kymera_output_if.h"
#include "kymera_tones_prompts.h"
#include "kymera_leakthrough.h"
#include "kymera_data.h"
#include "kymera_le_common.h"
#include "kymera_volume.h"
#include "operator.h"

/*! Code assertion that can be checked at run time. This will cause a panic. */
#define assert(x)   PanicFalse(x)

#ifdef INCLUDE_LE_AUDIO_UNICAST
#include "kymera_le_mic_chain.h"
#endif

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)

#define LC3_DECODE_SCO_ISO_BUFFER_SIZE  1920
#define DECODE_ONLY_LEFT   0
#define DECODE_ONLY_RIGHT  1

/* Extra amount of data (in uS) to predecode at the start of an aptX adaptive stream in addition to the frame duration */
#define ADDITIONAL_PREDECODE_DURATION  2000

#define KYMERA_NUMBER_OF_QSS_PARAMS_TO_QUERY 0x3
#define MIXER_GAIN_RAMP_SAMPLES 24000

/*! \brief LE From Air chain information */
typedef struct
{
    le_media_config_t from_air_params;
}kymera_le_from_air_chain_data_t;

kymera_le_from_air_chain_data_t le_from_air_chain_data;


static const output_registry_entry_t output_info =
{
    .user = output_user_le_audio,
#ifdef INCLUDE_STEREO
    .connection = output_connection_stereo,
#else
    .connection = output_connection_mono,
#endif
};

static const appKymeraLeAudioChainTable *chain_config_map = NULL;
static const appKymeraLeAudioFromAirVoiceChainTable *from_air_cvc_chain_config_map = NULL;

static void kymera_FromAirChainDataReset(void);

static void kymeraLeAudio_StartMediaChain(uint16 source_iso_handle, uint16 source_iso_handle_right);

static const chain_config_t *kymeraLeAudio_FindFromAirCvcChainConfigTable(uint16 sample_rate);

static const chain_config_t* kymeraLeAudio_FindChainFromConfigTable(const le_media_config_t *media_params)
{
    uint8 index;
    appKymeraLeAudioDecoderConfigType decoder_config_type;
    bool is_stereo_device = Multidevice_IsDeviceStereo();

    assert(chain_config_map);

    switch (media_params->stream_type)
    {
        default:
        case KYMERA_LE_STREAM_MONO:
            decoder_config_type = is_stereo_device ? KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_DUAL_MONO
                                                   : KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO;
        break;

        case KYMERA_LE_STREAM_STEREO_USE_LEFT:
        case KYMERA_LE_STREAM_STEREO_USE_RIGHT:
            decoder_config_type = is_stereo_device ? KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_DUAL_MONO
#ifdef INCLUDE_CIS_MIRRORING
                                                   : KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_STEREO;
#else
                                                   : KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO;
#endif
        break;

        case KYMERA_LE_STREAM_STEREO_USE_BOTH:
            decoder_config_type = KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_STEREO;
        break;

        case KYMERA_LE_STREAM_DUAL_MONO:
            decoder_config_type = is_stereo_device ? KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_DUAL_DECODER_TO_STEREO
                                                   : KYMERA_LE_AUDIO_DECODER_CONFIG_TYPE_SINGLE_DECODER_TO_MONO;
        break;
    }

    DEBUG_LOG("kymeraLeAudio_FindChainFromConfigTable for codec_type: %d, decoder_config: %d", media_params->codec_type, decoder_config_type);
    for (index = 0; index < chain_config_map->table_length; index++)
    {
        if (chain_config_map->chain_table[index].codec_type  == media_params->codec_type &&
            chain_config_map->chain_table[index].decoder_config_type == decoder_config_type)
        {
            return chain_config_map->chain_table[index].chain_config;
        }
    }

    Panic();
    return NULL;
}

static void kymeraLeAudio_CreateInputChain(const le_media_config_t *media_params, bool use_cvc)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    const chain_config_t *chain_config = use_cvc? kymeraLeAudio_FindFromAirCvcChainConfigTable(media_params->sample_rate) :
                                                  kymeraLeAudio_FindChainFromConfigTable(media_params);

    /* Create chain and return handle */
    PanicNotNull(theKymera->chain_input_handle);
    theKymera->chain_input_handle = ChainCreate(chain_config);
}

static void kymeraLeDecoderForStreamType(Operator op, appKymeraLeStreamType stream_type)
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

            if (Multidevice_IsDeviceStereo())
            {
                OperatorsLc3DecoderScoIsoSetMonoDecode(op,
                    stream_type == KYMERA_LE_STREAM_STEREO_USE_LEFT ? DECODE_ONLY_LEFT : DECODE_ONLY_RIGHT);
            }
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

static void appKymeraLeAudioConfigureDecoderOperatorCommon(Operator op, const le_media_config_t *media_params)
{
    OperatorsStandardSetSampleRate(op, media_params->sample_rate);
    OperatorsStandardSetTimeToPlayLatency(op, media_params->presentation_delay);
    OperatorsStandardSetBufferSize(op, LC3_DECODE_SCO_ISO_BUFFER_SIZE);
}

static void appKymeraLeAudioConfigureLc3Decoder(Operator op, const le_media_config_t *media_params)
{
    OperatorsLc3DecoderScoIsoSetPacketLength(op, media_params->frame_length);
    OperatorsLc3DecoderScoIsoSetFrameDuration(op, media_params->frame_duration);

    DEBUG_LOG("appKymeraLeAudioConfigureInputChain op_id %x", op);
    kymeraLeDecoderForStreamType(op, media_params->stream_type);

    if (media_params->codec_frame_blocks_per_sdu > 1)
    {
        OperatorsLc3DecoderScoIsoSetBlocksPerSdu(op, media_params->codec_frame_blocks_per_sdu);
    }

    appKymeraLeAudioConfigureDecoderOperatorCommon(op, media_params);
}

static void appKymeraLeAudioConfigureLeftRightMixer(const le_media_config_t *media_params)
{
    Operator mixer_op;

    if (GET_OP_FROM_CHAIN(mixer_op, KymeraGetTaskData()->chain_input_handle, OPR_LEFT_RIGHT_MIXER))
    {
            uint32 gain_left = GAIN_FULL;
            uint32 gain_right = GAIN_MIN;

            if (media_params->stream_type == KYMERA_LE_STREAM_STEREO_USE_RIGHT)
            {
                gain_left = GAIN_MIN;
                gain_right = GAIN_FULL;
            }

            OperatorsConfigureMixer(mixer_op, media_params->sample_rate, 1, gain_left, gain_right, GAIN_MIN, 1, 1, 0);
            OperatorsMixerSetNumberOfSamplesToRamp(mixer_op, MIXER_GAIN_RAMP_SAMPLES);
    }
}

static void appKymeraLeAudioConfigureInputChain(const le_media_config_t *media_params)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    
    /* Codec  specific configuration */
    switch (media_params->codec_type)
    {
        case KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE:
        {/* aptX adaptive specific initialisation */
            Operator op = ChainGetOperatorByRole(theKymera->chain_input_handle, OPR_APTX_ADAPTIVE_DECODE_SCO_ISO);
            appKymeraLeAudioConfigureDecoderOperatorCommon(op, media_params);
            OperatorsAptxAdaptiveDecoderScoIsoSetFrameDuration(op, media_params->frame_duration);
            /* Set the aptX adaptive predecode duration to be an ISO interval plus 2ms */
            OperatorsAptxAdaptiveDecoderScoIsoSetPredecodeDuration(op, (media_params->frame_duration + ADDITIONAL_PREDECODE_DURATION));
        }
        break;

        case KYMERA_LE_AUDIO_CODEC_APTX_LITE:
        {
            Operator op = ChainGetOperatorByRole(theKymera->chain_input_handle, OPR_APTX_LITE_DECODE_SCO_ISO);
            appKymeraLeAudioConfigureDecoderOperatorCommon(op,media_params);
            OperatorsAptxLiteDecoderScoIsoSetFrameDuration(op, media_params->frame_duration);
        }
        break;

        case KYMERA_LE_AUDIO_CODEC_LC3:
        default:
        {
            Operator op = ChainGetOperatorByRole(theKymera->chain_input_handle, OPR_LC3_DECODE_SCO_ISO);

            appKymeraLeAudioConfigureLc3Decoder(op, media_params);
            appKymeraLeAudioConfigureLeftRightMixer(media_params);
        }

        if(media_params->source_iso_handle_right != LE_AUDIO_INVALID_ISO_HANDLE)
        {
            Operator op_right = ChainGetOperatorByRole(theKymera->chain_input_handle, OPR_LC3_DECODE_SCO_ISO_RIGHT);
            DEBUG_LOG("appKymeraLeAudioConfigureInputChain source_iso_handle_right %d", media_params->source_iso_handle_right);
            appKymeraLeAudioConfigureLc3Decoder(op_right, media_params);
        }
        break;
    }
}

static void kymeraLeAudio_CreateAndConfigureOutputChain(uint32 rate,
                                                   int16 volume_in_db , bool is_voice_back_channel, bool use_src_sync)
{
    kymera_output_chain_config config = {0};

    config.rate = rate;
    config.kick_period = KICK_PERIOD_LE_AUDIO;
    if (use_src_sync)
        config.chain_type = output_info.connection == output_connection_mono ? output_chain_mono : output_chain_stereo;
    else
        config.chain_type = output_info.connection == output_connection_mono ? output_chain_mono_le : output_chain_stereo_le;

    if (is_voice_back_channel)
    {
        config.chain_include_aec = TRUE;
    }

    /* By default or for source sync version <=3.3 the output buffer needs to
       be able to hold at least SS_MAX_PERIOD worth  of audio (default = 2 *
       Kp), but be less than SS_MAX_LATENCY (5 * Kp). The recommendation is 2 Kp
       more than SS_MAX_PERIOD, so 4 * Kp. */
    appKymeraSetSourceSyncConfigOutputBufferSize(&config, 4, 0);

    PanicFalse(Kymera_OutputPrepare(output_user_le_audio, &config));
    KymeraOutput_SetMainVolume(volume_in_db);
}

static void kymeraLeAudio_JoinChains(kymeraTaskData *theKymera, bool use_cvc)
{
    output_source_t output = {0};

    if (output_info.connection == output_connection_stereo)
    {
        output.stereo.left = ChainGetOutput(theKymera->chain_input_handle, EPR_SOURCE_DECODED_PCM);
        output.stereo.right = ChainGetOutput(theKymera->chain_input_handle, EPR_SOURCE_DECODED_PCM_RIGHT);
        if(Kymera_IsMusicProcessingPresent() && !use_cvc)
        {
            PanicFalse(ChainConnectInput(theKymera->chain_music_processing_handle, output.stereo.left, EPR_MUSIC_PROCESSING_IN_L));
            PanicFalse(ChainConnectInput(theKymera->chain_music_processing_handle, output.stereo.right, EPR_MUSIC_PROCESSING_IN_R));
            output.stereo.left = ChainGetOutput(theKymera->chain_music_processing_handle, EPR_MUSIC_PROCESSING_OUT_L);
            output.stereo.right = ChainGetOutput(theKymera->chain_music_processing_handle, EPR_MUSIC_PROCESSING_OUT_R);
        }
    }
    else
    {
        output.mono = ChainGetOutput(theKymera->chain_input_handle, EPR_SOURCE_DECODED_PCM);
        if(Kymera_IsMusicProcessingPresent() && !use_cvc)
        {
            PanicFalse(ChainConnectInput(theKymera->chain_music_processing_handle, output.mono, EPR_MUSIC_PROCESSING_IN_L));
            output.mono = ChainGetOutput(theKymera->chain_music_processing_handle, EPR_MUSIC_PROCESSING_OUT_L);
        }
    }

    PanicFalse(Kymera_OutputConnect(output_user_le_audio, &output));
}

#ifdef INCLUDE_MIRRORING
static void kymeraLeAudio_AudioSyncMute(bool mute_enable)
{
    Operator op;

    if (GET_OP_FROM_CHAIN(op, KymeraGetTaskData()->chain_input_handle, OPR_BASIC_PASS))
    {
        OperatorsSetPassthroughGain(op, mute_enable ? VOLUME_MUTE_IN_DB : 0);
    }
}

void KymeraLeAudio_ConfigureStartSync(kymeraTaskData *theKymera, bool start_muted)
{
    DEBUG_LOG("kymeraLeAudio_ConfigureStartSync start_muted %d", start_muted);

    if (start_muted)
    {
        /* Mute audio output before starting the input chain to ensure that
         * audio chains consume audio data and play silence on the output
         * until the application receives sink synchronised indication.
         * The source sync gain is used instead of the volume control as this
         * gain only affects the input stream - the aux/main volume can be set
         * without affecting the source sync's mute of the input.
         */
        kymeraLeAudio_AudioSyncMute(TRUE);
        theKymera->sync_info.state = KYMERA_AUDIO_SYNC_STATE_IN_PROGRESS;
        theKymera->sync_info.mode = KYMERA_AUDIO_SYNC_START_PRIMARY_SYNC_UNMUTE;
    }
    else
    {
        /* Audio sync is not required in these modes. */
        theKymera->sync_info.state = KYMERA_AUDIO_SYNC_STATE_COMPLETE;
    }
}

static void kymeraLeAudio_CompleteStartSync(kymeraTaskData *theKymera)
{
    if (   (theKymera->state != KYMERA_STATE_LE_AUDIO_ACTIVE)
        || (theKymera->sync_info.state == KYMERA_AUDIO_SYNC_STATE_COMPLETE))
    {
        return;
    }

    switch(theKymera->sync_info.mode)
    {
    case KYMERA_AUDIO_SYNC_START_SECONDARY_SYNC_UNMUTE:
    case KYMERA_AUDIO_SYNC_START_PRIMARY_SYNC_UNMUTE:
        {
            /* Now that LE audio is synchronised, unmute the source sync output */
            kymeraLeAudio_AudioSyncMute(FALSE);
            theKymera->sync_info.state = KYMERA_AUDIO_SYNC_STATE_COMPLETE;
        }
        break;

    default:
        Panic();
        break;
    }
}

void Kymera_LeAudioSyncMute(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("Kymera_LeAudioSyncMute lock %u", theKymera->lock);

    /* Cancel any pending mute messages */
    MessageCancelAll(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_UNMUTE);

    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_MUTE, NULL, &theKymera->lock);
}

void KymeraLeAudio_HandleAudioUnmuteInd(const KYMERA_INTERNAL_LE_AUDIO_UNMUTE_T *unmute_params)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("kymeraLeAudio_HandleAudioUnmuteInd state enum:appKymeraState:%u sync_state enum:appKymeraAudioSyncState:%u sync_mode enum:appKymeraAudioSyncStartMode:%u",
              theKymera->state, theKymera->sync_info.state, theKymera->sync_info.mode);

    /* Cancel any pending messages */
    MessageCancelAll(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_UNMUTE);

    Delay delay = RtimeTimeToMsDelay(unmute_params->unmute_time);
    if (D_IMMEDIATE == delay)
    {
        /* Un-mute should happen 'now', but only if the LE audio chain is active. */
        if (theKymera->state == KYMERA_STATE_LE_AUDIO_ACTIVE)
        {
            kymeraLeAudio_CompleteStartSync(theKymera);
        }
        else
        {
            /* LE audio chain is not active but if there is a pending start in
               the queue it means this unmute was delivered too early. Re-queue
               it so that if the chain starts muted, it will eventually be
               un-muted - i.e. it won't stay muted forever. */
            if (MessagePendingFirst(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_START, NULL))
            {
                DEBUG_LOG("kymeraLeAudio_HandleAudioUnmuteInd pending conditionally - resend");

                MESSAGE_MAKE(msg, KYMERA_INTERNAL_LE_AUDIO_UNMUTE_T);
                msg->unmute_time = unmute_params->unmute_time;
                MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_UNMUTE, msg, &theKymera->lock);
            }
        }
    }
    else
    {
        DEBUG_LOG("kymeraLeAudio_HandleAudioUnmuteInd pending delay %u - resend", delay);

        MESSAGE_MAKE(msg, KYMERA_INTERNAL_LE_AUDIO_UNMUTE_T);
        msg->unmute_time = unmute_params->unmute_time;
        MessageSendLater(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_UNMUTE, msg, delay);
    }
}
#endif /* INCLUDE_MIRRORING */

void kymeraLeAudio_Stop(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("kymeraLeAudio_Stop, state %u", appKymeraGetState());

    switch (appKymeraGetState())
    {
        case KYMERA_STATE_LE_AUDIO_ACTIVE:
        {
            if(theKymera->chain_input_handle != NULL)
            {
                /* A tone still playing at this point must be interruptable */
                appKymeraTonePromptStop();

                /* Stop chains before disconnecting */
                ChainStop(theKymera->chain_input_handle);

                StreamDisconnect(NULL, ChainGetInput(theKymera->chain_input_handle, EPR_ISO_FROM_AIR_LEFT));
                StreamDisconnect(NULL, ChainGetInput(theKymera->chain_input_handle, EPR_ISO_FROM_AIR_RIGHT));

                Kymera_StopMusicProcessingChain();

                Kymera_OutputDisconnect(output_user_le_audio);

                Kymera_DestroyMusicProcessingChain();

                kymera_FromAirChainDataReset();
                /* Destroy chains now that input has been disconnected */
                ChainDestroy(theKymera->chain_input_handle);
                theKymera->chain_input_handle = NULL;
            }

#ifdef INCLUDE_LE_AUDIO_UNICAST
            Kymera_LeMicDisconnectOutputChain();

            /* Destroy mic chains */
            Kymera_StopLeMicChain();
#endif
            appKymeraSetState(KYMERA_STATE_IDLE);

            break;
        }

        case KYMERA_STATE_IDLE:
            break;

        default:
            /* Report, but ignore attempts to stop in invalid states */
            DEBUG_LOG("kymeraLeAudio_Stop, invalid state %u", appKymeraGetState());
            break;
    }

    Kymera_LeakthroughResumeChainIfSuspended();
}

void Kymera_SetLeAudioChainTable(const appKymeraLeAudioChainTable *chain_table)
{
    chain_config_map = chain_table;
}

void Kymera_SetLeFromAirVoiceChainTable(const appKymeraLeAudioFromAirVoiceChainTable *chain_table)
{
    from_air_cvc_chain_config_map = chain_table;
}

static void kymeraLeAudio_PreStartConcurrencyCheck(kymeraTaskData *theKymera)
{
    switch(appKymeraGetState())
    {
        case KYMERA_STATE_IDLE:
        case KYMERA_STATE_ADAPTIVE_ANC_STARTED:
            break;
        default:
            Panic();
            break;
    }
    PanicNotNull(theKymera->chain_input_handle);
}

static void kymeraLeAudio_StartMediaChain(uint16 source_iso_handle, uint16 source_iso_handle_right)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    Source audio_source;
    Source audio_source_right = NULL;

    appKymeraConfigureDspPowerMode();

    if(appKymeraGetState() != KYMERA_STATE_IDLE)
    {
        KymeraOutput_ChainStart();

        if (!le_from_air_chain_data.from_air_params.use_cvc)
        {
            /* Start music processing chain for non voice */
            Kymera_StartMusicProcessingChain();
        }

        audio_source = StreamIsoSource(source_iso_handle);
        DEBUG_LOG("kymeraLeAudio_StartMediaChain audio_source %p, source_iso_handle %d", audio_source, source_iso_handle);

        if(source_iso_handle_right != LE_AUDIO_INVALID_ISO_HANDLE)
        {
            audio_source_right = StreamIsoSource(source_iso_handle_right);

            if ((audio_source != NULL) && (audio_source_right != NULL))
            {
                /* Synchronise ISO source endpoints in stereo configuration before starting the operators connected to the endpoints */
                PanicFalse(SourceSynchronise(audio_source, audio_source_right));
            }

            DEBUG_LOG("kymeraLeAudio_StartMediaChain audio_source_right %p, source_iso_handle_right %d", audio_source_right, source_iso_handle_right);
            if(ChainConnectInput(theKymera->chain_input_handle, audio_source_right, EPR_ISO_FROM_AIR_RIGHT))
            {
                DEBUG_LOG("kymeraLeAudio_StartMediaChain, Split Stereo ISO connection");
            }
        }

        DEBUG_LOG("kymeraLeAudio_StartMediaChain, audio_source %p", audio_source);
        if (ChainConnectInput(theKymera->chain_input_handle, audio_source, EPR_ISO_FROM_AIR_LEFT))
        {
            if (!ChainStartAttempt(theKymera->chain_input_handle))
            {
                DEBUG_LOG("kymeraLeAudio_Start, could not start LE audio input chain");
                /* Stop/destroy the chain, returning state to KYMERA_STATE_IDLE.
                    This needs to be done here, since between the failed attempt to start
                    and the subsequent stop (when kymeraLeAudio_Stop() is called), a tone
                    may need to be played - it would not be possible to play a tone in a
                    stopped ISO chain. The state needs to be KYMERA_STATE_LE_AUDIO_ACTIVE for
                    kymeraLeAudio_Stop() to stop/destroy the chain. */
                kymeraLeAudio_Stop();
            }
        }
    }
}

static void kymera_FromAirChainDataReset(void)
{
    DEBUG_LOG("kymera_FromAirChainDataReset");
    memset(&le_from_air_chain_data, 0, sizeof(kymera_le_from_air_chain_data_t));
}

static void kymera_FromAirChainDataInit(const le_media_config_t *from_air_config)
{
    DEBUG_LOG("kymera_FromAirChainDataInit");

    memset(&le_from_air_chain_data, 0, sizeof(kymera_le_from_air_chain_data_t));
    le_from_air_chain_data.from_air_params = *from_air_config;

    DEBUG_LOG("media_params: frame length: %d, Presentation delay: %d, Sample Rate %d, Frame duration %d codec_type %d",
                        le_from_air_chain_data.from_air_params.frame_length,
                        le_from_air_chain_data.from_air_params.presentation_delay,
                        le_from_air_chain_data.from_air_params.sample_rate,
                        le_from_air_chain_data.from_air_params.frame_duration,
                        le_from_air_chain_data.from_air_params.codec_type);

    DEBUG_LOG("media_params: stream_type enum:appKymeraLeStreamType:%u codec_version 0x%x Frame Blocks Per SDU %d source_iso_handle 0x%x, source_iso_handle_right 0x%x, gaming mode %d",
                        le_from_air_chain_data.from_air_params.stream_type,
                        le_from_air_chain_data.from_air_params.codec_version,
                        le_from_air_chain_data.from_air_params.codec_frame_blocks_per_sdu,
                        le_from_air_chain_data.from_air_params.source_iso_handle,
                        le_from_air_chain_data.from_air_params.source_iso_handle_right,
                        le_from_air_chain_data.from_air_params.gaming_mode);

}

static const chain_config_t *kymeraLeAudio_FindFromAirCvcChainConfigTable(uint16 sample_rate)
{
    unsigned index;

    assert(from_air_cvc_chain_config_map);

    DEBUG_LOG("kymeraLeAudio_FindFromAirCvcChainConfigTable chain_table %p chain_length %u",
              from_air_cvc_chain_config_map->chain_table, from_air_cvc_chain_config_map->table_length);

    for (index = 0; index < from_air_cvc_chain_config_map->table_length; index++)
    {
        if(/*from_air_cvc_chain_config_map->chain_table[index].is_stereo_config == use_split_stereo_chain && 
             && */from_air_cvc_chain_config_map->chain_table[index].sample_rate == sample_rate)
        {
            return from_air_cvc_chain_config_map->chain_table[index].chain_config;
        }
    }

    DEBUG_LOG("kymeraLeAudio_FindFromAirCvcChainConfigTable: No compatible chain configuration found!");
    Panic();

    return NULL;
}

static void kymeraLeAudio_CreateFromAirChain(bool use_cvc)
{
    const le_media_config_t *from_air_config = &le_from_air_chain_data.from_air_params;
    kymeraTaskData *theKymera = KymeraGetTaskData();

    kymeraLeAudio_CreateInputChain(from_air_config, use_cvc);
    /* Configure decoder for from air stream */
    appKymeraLeAudioConfigureInputChain(from_air_config);

    if (!use_cvc)
    {
        /* Setup music processing chain */
        Kymera_CreateMusicProcessingChain();
        Kymera_ConfigureMusicProcessing(from_air_config->sample_rate);
    }

    if (theKymera->chain_config_callbacks && theKymera->chain_config_callbacks->ConfigureLeAudioFromAirChain)
    {
        kymera_le_audio_from_air_config_params_t params = {0};
        params.codec_type = from_air_config->codec_type;
        params.sample_rate = from_air_config->sample_rate;
        params.frame_length = from_air_config->frame_length;
        params.frame_duration_us = from_air_config->frame_duration;
        params.presentation_delay_us = from_air_config->presentation_delay;
        theKymera->chain_config_callbacks->ConfigureLeAudioFromAirChain(theKymera->chain_input_handle, &params);
    }

    ChainConnect(theKymera->chain_input_handle);

    kymeraLeAudio_JoinChains(KymeraGetTaskData(), use_cvc);
#ifdef INCLUDE_MIRRORING
    KymeraLeAudio_ConfigureStartSync(KymeraGetTaskData(), from_air_config->start_muted);
#endif
}

static void kymeraLeAudio_SetupFromAirChain(const KYMERA_INTERNAL_LE_AUDIO_START_T *start_params)
{
    DEBUG_LOG("kymeraLeAudio_SetupFromAirChain, state %u, ", appKymeraGetState());

    if (start_params->media_present)
    {
        bool use_src_sync = FALSE;
        bool use_aec_ref = start_params->microphone_present;

        /* Intiialize LE From Air chain data */
        kymera_FromAirChainDataInit(&start_params->media_params);

        /* Source Sync shall be used only for LE APTX Adaptive version */
        if (le_from_air_chain_data.from_air_params.codec_type == KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE)
        {
            use_src_sync = TRUE;
        }

        /* Create output chain, Keep AEC Ref enabled for all chains */
        kymeraLeAudio_CreateAndConfigureOutputChain(start_params->media_params.sample_rate, start_params->volume_in_db, use_aec_ref, use_src_sync);

        kymeraLeAudio_CreateFromAirChain(le_from_air_chain_data.from_air_params.use_cvc);
    }
}

static void kymeraLeAudio_SetupToAirChain(const KYMERA_INTERNAL_LE_AUDIO_START_T *start_params)
{
#ifdef INCLUDE_LE_AUDIO_UNICAST
    bool is_voice_back_channel = FALSE;

    DEBUG_LOG("kymeraLeAudio_SetupToAirChain, state %u, To Air Path: %d", appKymeraGetState(), start_params->microphone_present);

    if(start_params->microphone_present)
    {
        DEBUG_LOG("kymeraLeAudio_SetupToAirChain microphone_params: Frame Length: %d, Presentation delay: %d, Sample Rate %d, Frame duration %d codec_type %d codec_version 0x%x Frame Blocks Per SDU %d source_iso_handle 0x%x mic_sync:%d",
                            start_params->microphone_params.frame_length,
                            start_params->microphone_params.presentation_delay,
                            start_params->microphone_params.sample_rate,
                            start_params->microphone_params.frame_duration,
                            start_params->microphone_params.codec_type,
                            start_params->microphone_params.codec_version,
                            start_params->microphone_params.codec_frame_blocks_per_sdu,
                            start_params->microphone_params.source_iso_handle,
                            start_params->microphone_params.mic_sync);

        if(start_params->media_present)
        {
            is_voice_back_channel = TRUE;
        }

        Kymera_CreateLeMicChain(&start_params->microphone_params, is_voice_back_channel);
    }
#else
    UNUSED(start_params);
#endif
}

static void kymeraLeAudio_StartFromAirChain(bool from_air_present)
{
    if (from_air_present)
    {
        kymeraLeAudio_StartMediaChain(le_from_air_chain_data.from_air_params.source_iso_handle, le_from_air_chain_data.from_air_params.source_iso_handle_right);
    }
}

static void kymeraLeAudio_StartToAirChain(const KYMERA_INTERNAL_LE_AUDIO_START_T *start_params)
{
    DEBUG_LOG("kymeraLeAudio_StartToAirChain, state %u, To Air path: %d", appKymeraGetState(), start_params->microphone_present);

#ifdef INCLUDE_LE_AUDIO_UNICAST
    if (start_params->microphone_present)
    {
        Kymera_StartLeMicChain();

        if (start_params->microphone_params.mic_sync)
        {
            /* Schedule auto-unmute after timeout if Kymera_ScheduleLeaMicSyncUnmute is not called */
            Kymera_ScheduleLeaMicSyncUnmute(appConfigLeMicSyncUnmuteTimeoutMs());
        }

        if (start_params->microphone_params.started_handler != NULL)
        {
           start_params->microphone_params.started_handler();
        }
    }
#endif
}

void kymeraLeAudio_Start(const KYMERA_INTERNAL_LE_AUDIO_START_T * start_params)
{
    DEBUG_LOG("kymeraLeAudio_Start, state %u, From Air Chain : %d, To Air Chain : %d", 
                appKymeraGetState(), start_params->media_present, start_params->microphone_present);

    /* Momentarily boosting DSP clock for faster chain creation and setup */
    appKymeraBoostDspClockToMax();

    /* If there is a tone still playing at this point,
     * it must be an interruptible tone, so cut it off */
    appKymeraTonePromptStop();

    if(appKymeraGetState() == KYMERA_STATE_STANDALONE_LEAKTHROUGH)
    {
        Kymera_LeakthroughStopChainIfRunning();
        appKymeraSetState(KYMERA_STATE_IDLE);
    }

    kymeraLeAudio_PreStartConcurrencyCheck(KymeraGetTaskData());

    /* Move to LE audio active state now, what ever happens we end up in this state
      (even if it's temporary) */
    appKymeraSetState(KYMERA_STATE_LE_AUDIO_ACTIVE);

    /* Mic In -> LC3/AptxR3 Encode -> ISO stream */
    kymeraLeAudio_SetupToAirChain(start_params);

    /* ISO stream -> LC3/AptxR3 Decode -> DAC out */
    kymeraLeAudio_SetupFromAirChain(start_params);

    kymeraLeAudio_StartFromAirChain(start_params->media_present);

    kymeraLeAudio_StartToAirChain(start_params);

    if (KymeraLeAudio_IsAptxAdaptiveStreaming())
    {
        TaskList_MessageSendId(KymeraGetTaskData()->listeners, KYMERA_APTX_ADAPTIVE_STREAMING_IND);
    }
}

void kymeraLeAudio_SetVolume(int16 volume_in_db)
{
    DEBUG_LOG("kymeraLeAudio_SetVolume, vol %d", volume_in_db);

    switch (KymeraGetTaskData()->state)
    {
        case KYMERA_STATE_LE_AUDIO_ACTIVE:
        {
            KymeraOutput_SetMainVolume(volume_in_db);
        }
        break;
        default:
            break;
    }
}

bool KymeraLeAudio_IsAptxAdaptiveStreaming(void)
{
    bool le_aptx_adpative_streaming = FALSE;

    if (KymeraGetTaskData()->state == KYMERA_STATE_LE_AUDIO_ACTIVE && 
        le_from_air_chain_data.from_air_params.codec_type == KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE)
    {
        le_aptx_adpative_streaming = TRUE;
    }

    DEBUG_LOG("KymeraLeAudio_IsAptxAdaptiveStreaming, %d", le_aptx_adpative_streaming);

    return le_aptx_adpative_streaming;
}

bool KymeraLeAudio_GetAptxAdaptiveLossLessInfo(uint32 *lossless_data)
{
    bool status = FALSE;
    Operator operator;
    get_status_data_t* get_status = NULL;

    if (KymeraLeAudio_IsAptxAdaptiveStreaming())
    {
        operator = ChainGetOperatorByRole(KymeraGetTaskData()->chain_input_handle,
                                          OPR_APTX_ADAPTIVE_DECODE_SCO_ISO);
        get_status = OperatorsCreateGetStatusData(KYMERA_NUMBER_OF_QSS_PARAMS_TO_QUERY);
        OperatorsGetStatus(operator, get_status);

        if (get_status->result == obpm_ok)
        {
            *lossless_data = (uint8) get_status->value[0] << 24 |
                             (uint8) get_status->value[1] << 16 |
                             (uint16) get_status->value[2];
        }

        DEBUG_LOG("KymeraLeAudio_GetAptxAdaptiveLossLessInfo status: enum:obpm_result_state_t:%u, lossless_support: 0x%x, lossless_mode: 0x%x, bit_rate:0x%x",
                  get_status->result, get_status->value[0], get_status->value[1], get_status->value[2]);

        status = TRUE;
        free(get_status);
    }

    return status;
}

void kymeraLeAudio_Init(void)
{
    Kymera_OutputRegister(&output_info);
#ifdef INCLUDE_LE_AUDIO_UNICAST
    Kymera_LeMicChainInit();
#endif
}

#endif
