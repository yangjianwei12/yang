/*!
\copyright  Copyright (c) 2017 - 2021  Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_prompts.c
\brief      Kymera tones / prompts
*/
#include "kymera_tones_prompts.h"
#include "kymera_dsp_clock.h"
#include "kymera_state.h"
#include "kymera_data.h"
#include "kymera_setup.h"
#include "kymera_lock.h"
#include "kymera_internal_msg_ids.h"
#include "kymera_config.h"
#include "kymera_aec.h"
#include "kymera_output_if.h"
#include "kymera_msg.h"
#include "kymera_op_msg.h"
#include "kymera_fit_test.h"
#include "system_clock.h"
#include "timestamp_event.h"
#include <operators.h>

#define BUFFER_SIZE_FACTOR 4

#define PREPARE_FOR_PROMPT_TIMEOUT (1000)

/* Indicates the buffer size required for SBC-prompts / tone-generator */
#define PROMPT_TONE_OUTPUT_SIZE_SBC (256)

/* Buffer size to use for the decoded output of AAC prompts (in addition to any other buffers used for prompts ).
 * Has been determined experimentally for prompts of a sampling rate of up to 48KHz and bitrate of up to 128Kbps.
 */
#define AAC_PROMPT_DECODED_BUFFER_SIZE (2560)

/* Buffer size to use for the decoded output of SBC prompts (in addition to any other buffers used for prompts ).
 */
#define SBC_PROMPT_DECODED_BUFFER_SIZE (900)

#define PROMPT_TASK ((Task)&prompt_handler)

static kymera_chain_handle_t kymera_GetTonePromptChain(void);

static void kymera_SetupPromptSource(Source source);
static void kymera_ClosePromptSource(Source source);

static bool kymera_OutputDisconnectRequest(void);
static void kymera_PrepareForOutputChainDisconnect(void);
static void kymera_CompleteOutputChainDisconnect(void);

static const output_callbacks_t output_callbacks =
{
   .OutputDisconnectRequest = kymera_OutputDisconnectRequest,
   .OutputDisconnectPrepare = kymera_PrepareForOutputChainDisconnect,
   .OutputDisconnectComplete = kymera_CompleteOutputChainDisconnect,
};

static const output_registry_entry_t output_info =
{
    .user = output_user_prompt,
    .connection = output_connection_aux,
    .assume_chain_compatibility = TRUE,
    .prefer_chain_config_from_user = output_user_a2dp,
    .callbacks = &output_callbacks,
};

static void kymera_PromptMsgHandler(Task task, MessageId id, Message msg);
static const TaskData prompt_handler = {kymera_PromptMsgHandler};

static enum
{
    kymera_tone_idle,
    kymera_tone_ready_tone,
    kymera_tone_ready_prompt,
    kymera_tone_playing
} kymera_tone_state = kymera_tone_idle;

static void kymera_StartPromptSource(FILE_INDEX prompt)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    theKymera->prompt_source = PanicNull(StreamFileSource(prompt));
    kymera_SetupPromptSource(theKymera->prompt_source);
    PanicFalse(ChainConnectInput(kymera_GetTonePromptChain(), theKymera->prompt_source, EPR_PROMPT_IN));
    KymeraOutput_SetAuxVolume(KYMERA_CONFIG_PROMPT_VOLUME);
}

static void kymera_StopPromptSource(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    KymeraOutput_SetAuxVolume(0);
    if (theKymera->prompt_source)
    {
        kymera_ClosePromptSource(theKymera->prompt_source);
        theKymera->prompt_source = NULL;
    }
}

static void kymera_ReplayPromptSource(FILE_INDEX prompt)
{
    kymera_StopPromptSource();
    kymera_StartPromptSource(prompt);
}

static void kymera_PromptMsgHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);
    UNUSED(msg);

    if (id == MESSAGE_STREAM_DISCONNECT)
    {
        DEBUG_LOG("kymera_PromptMsgHandler: MESSAGE_STREAM_DISCONNECT");

        /* To support looping Fit Test prompt in EFT use cases */
        if(KymeraFitTest_PromptReplayRequired())
        {
            FILE_INDEX prompt = KymeraGetTaskData()->prompt;
            kymera_ReplayPromptSource(prompt);
            return;
        }

        appKymeraTonePromptStop();
        appKymera_MsgRegisteredClients(KYMERA_PROMPT_END_IND, KYMERA_OP_MSG_WORD_PAYLOAD_NA);
    }
}

static kymera_chain_handle_t kymera_GetTonePromptChain(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    return theKymera->chain_tone_handle;
}

static void kymera_SetupPromptSource(Source source)
{
    MessageStreamTaskFromSource(source, PROMPT_TASK);
}

static void kymera_ClosePromptSource(Source source)
{
    if (source)
    {
        MessageStreamTaskFromSource(source, NULL);
        StreamDisconnect(source, NULL);
        SourceClose(source);
    }
}

static void kymera_PrepareOutputChain(uint16 sample_rate)
{
    kymera_output_chain_config config = {0};
    KymeraOutput_SetDefaultOutputChainConfig(&config, sample_rate, KICK_PERIOD_TONES, 0);

    appKymeraBoostDspClockToMax();

    PanicFalse(Kymera_OutputPrepare(output_user_prompt, &config));
}

static const chain_config_t * kymera_GetPromptChainConfig(promptFormat prompt_format)
{
    const chain_config_t *config = NULL;

    switch(prompt_format)
    {
        case PROMPT_FORMAT_SBC:
            config = Kymera_GetChainConfigs()->chain_prompt_sbc_config;
        break;
        case PROMPT_FORMAT_PCM:
            config = Kymera_GetChainConfigs()->chain_prompt_pcm_config;
        break;
        case PROMPT_FORMAT_AAC:
            config = Kymera_GetChainConfigs()->chain_prompt_aac_config;
        break;
    }

    return config;
}

static const chain_config_t * kymera_GetToneChainConfig(void)
{
    return Kymera_GetChainConfigs()->chain_tone_gen_config;
}

static void kymera_ConfigureTonePromptInputBuffer(kymera_chain_handle_t chain, unsigned buffer_size)
{
    Operator op = ChainGetOperatorByRole(chain, OPR_TONE_PROMPT_PCM_BUFFER);
    operator_data_format_t data_format = operator_data_format_pcm;

    if (op == INVALID_OPERATOR)
    {
        op = ChainGetOperatorByRole(chain, OPR_TONE_PROMPT_ENCODED_BUFFER);
        data_format = operator_data_format_encoded;
        PanicFalse(op != INVALID_OPERATOR); // There must be either a PCM or an Encoded buffer
    }

    DEBUG_LOG("kymera_ConfigureTonePromptInputBuffer: buffer_op %u, buffer_size %u, data_format enum:operator_data_format_t:%d",
              op, buffer_size, data_format);
    OperatorsSetPassthroughDataFormat(op, data_format);
    OperatorsStandardSetBufferSizeWithFormat(op, buffer_size, data_format);
}

static void kymera_CreateChain(const chain_config_t *config)
{
    PanicFalse(kymera_GetTonePromptChain() == NULL);
    kymeraTaskData *theKymera = KymeraGetTaskData();
    kymera_chain_handle_t chain = ChainCreate(config);

    unsigned aux_buffer_size = Kymera_OutputGetMainVolumeBufferSize();
    // The buffer size must be positive
    PanicZero(aux_buffer_size);
    aux_buffer_size += PROMPT_TONE_OUTPUT_SIZE_SBC;

    kymera_ConfigureTonePromptInputBuffer(chain, aux_buffer_size);
    theKymera->chain_tone_handle = chain;
}

static void kymera_CreatePromptChain(promptFormat prompt_format)
{
    const chain_config_t *config = kymera_GetPromptChainConfig(prompt_format);

    /* NULL is a valid config for a PCM prompt */
    if (config)
    {
        Operator op;
        unsigned buffer_size;

        kymera_CreateChain(config);
        switch (prompt_format)
        {
            case PROMPT_FORMAT_AAC:
                buffer_size = AAC_PROMPT_DECODED_BUFFER_SIZE;
            break;

            case PROMPT_FORMAT_SBC:
                buffer_size = SBC_PROMPT_DECODED_BUFFER_SIZE;
            break;

            default:
                buffer_size = 0;
            break;
        }

        if (buffer_size != 0 && GET_OP_FROM_CHAIN(op, kymera_GetTonePromptChain(), OPR_TONE_PROMPT_DECODED_BUFFER))
        {
            OperatorsStandardSetBufferSize(op, buffer_size);
        }

        ChainConnect(kymera_GetTonePromptChain());
    }

    kymera_tone_state = kymera_tone_ready_prompt;
}

static void kymera_CreateToneChain(void)
{
    const chain_config_t *config = kymera_GetToneChainConfig();
    PanicNull((void *)config);
    kymera_CreateChain(config);
    ChainConnect(kymera_GetTonePromptChain());
    kymera_tone_state = kymera_tone_ready_tone;
}

static Source kymera_ConfigurePromptChain(const KYMERA_INTERNAL_TONE_PROMPT_PLAY_T *msg)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    theKymera->prompt = msg->prompt;
    theKymera->prompt_source = PanicNull(StreamFileSource(msg->prompt));
    DEBUG_LOG("kymera_ConfigurePromptChain prompt %u", msg->prompt);
    kymera_SetupPromptSource(theKymera->prompt_source);

    if (kymera_GetTonePromptChain())
    {
        if (msg->prompt_format == PROMPT_FORMAT_AAC)
        {
            Operator op = ChainGetOperatorByRole(kymera_GetTonePromptChain(), OPR_AAC_DECODER);
            OperatorsAacDecoderSetFormat(op, aac_frame_format_adts);
        }
        PanicFalse(ChainConnectInput(kymera_GetTonePromptChain(), theKymera->prompt_source, EPR_PROMPT_IN));
        return ChainGetOutput(kymera_GetTonePromptChain(), EPR_TONE_PROMPT_CHAIN_OUT);
    }
    else
    {
        /* No chain (prompt is PCM) so the source is just the file */
        return theKymera->prompt_source;
    }
}

static Source kymera_ConfigureToneChain(const KYMERA_INTERNAL_TONE_PROMPT_PLAY_T *msg)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    Operator op = ChainGetOperatorByRole(kymera_GetTonePromptChain(), OPR_TONE_GEN);
    DEBUG_LOG("kymera_ConfigureToneChain tone gen %u", op);
    OperatorsStandardSetSampleRate(op, msg->rate);
    OperatorsConfigureToneGenerator(op, msg->tone, &theKymera->task);

    return ChainGetOutput(kymera_GetTonePromptChain(), EPR_TONE_PROMPT_CHAIN_OUT);
}

bool appKymeraIsPlayingPrompt(void)
{
    return (kymera_tone_state == kymera_tone_playing);
}

static bool kymera_TonePromptIsReady(void)
{
    return ((kymera_tone_state == kymera_tone_ready_prompt) || (kymera_tone_state == kymera_tone_ready_tone));
}

static bool kymera_IsTheCorrectPromptChainReady(promptFormat format)
{
    Operator sbc_decoder = ChainGetOperatorByRole(kymera_GetTonePromptChain(), OPR_SBC_DECODER);
    Operator aac_decoder = ChainGetOperatorByRole(kymera_GetTonePromptChain(), OPR_AAC_DECODER);
    bool sbc_prompt_ready = ((format == PROMPT_FORMAT_SBC) && sbc_decoder);
    bool aac_prompt_ready = ((format == PROMPT_FORMAT_AAC) && aac_decoder);
    bool pcm_prompt_ready = ((format == PROMPT_FORMAT_PCM) && !(sbc_decoder || aac_decoder));

    return sbc_prompt_ready || aac_prompt_ready || pcm_prompt_ready;
}

static bool kymera_IsTheCorrectTonePromptChainReady(const KYMERA_INTERNAL_TONE_PROMPT_PLAY_T *msg)
{
    bool tone_chain_is_ready = ((kymera_tone_state == kymera_tone_ready_tone) && msg->tone != NULL);
    bool prompt_chain_is_ready = (((kymera_tone_state == kymera_tone_ready_prompt) && msg->prompt != FILE_NONE) &&
                                               kymera_IsTheCorrectPromptChainReady(msg->prompt_format));
    bool correct_chain_is_ready = tone_chain_is_ready || prompt_chain_is_ready;

    DEBUG_LOG("kymera_IsTheCorrectTonePromptChainReady %u, tone ready %u, prompt ready %u",
              correct_chain_is_ready, tone_chain_is_ready, prompt_chain_is_ready);

    return correct_chain_is_ready;
}

static void kymera_SendStartInd(const KYMERA_INTERNAL_TONE_PROMPT_PLAY_T *msg)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    if(msg->tone)
    {
        KYMERA_NOTIFICATION_TONE_STARTED_T* message = PanicNull(malloc(sizeof(KYMERA_NOTIFICATION_TONE_STARTED_T)));
        message->tone = msg->tone;
        TaskList_MessageSend(theKymera->listeners, KYMERA_NOTIFICATION_TONE_STARTED, message);
    }
    else
    {
        KYMERA_NOTIFICATION_PROMPT_STARTED_T* message = PanicNull(malloc(sizeof(KYMERA_NOTIFICATION_PROMPT_STARTED_T)));
        message->id = msg->prompt;
        TaskList_MessageSend(theKymera->listeners, KYMERA_NOTIFICATION_PROMPT_STARTED, message);
    }
}

static Source kymera_PrepareInputChain(const KYMERA_INTERNAL_TONE_PROMPT_PLAY_T *msg)
{
    bool is_tone = (msg->tone != NULL);
    bool is_prompt = (msg->prompt != FILE_NONE);
    Source output = NULL;

    if(is_tone)
    {
        if(!kymera_TonePromptIsReady())
        {
            kymera_CreateToneChain();
        }
        output = kymera_ConfigureToneChain(msg);
    }
    else if(is_prompt)
    {
        if(!kymera_TonePromptIsReady())
        {
            kymera_CreatePromptChain(msg->prompt_format);
        }
        output = kymera_ConfigurePromptChain(msg);
    }

    if(KymeraGetTaskData()->chain_config_callbacks && KymeraGetTaskData()->chain_config_callbacks->ConfigureTonePromptChain)
    {
        kymera_tone_prompt_config_params_t params = {0};
        params.sample_rate = msg->rate;
        params.is_tone = is_tone;
        params.prompt_format = msg->prompt_format;

        KymeraGetTaskData()->chain_config_callbacks->ConfigureTonePromptChain(KymeraGetTaskData()->chain_tone_handle, &params);
    }

    return output;
}

void appKymeraHandleInternalTonePromptPlay(const KYMERA_INTERNAL_TONE_PROMPT_PLAY_T *msg)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    output_source_t output = {0};
    int16 volume_db = (msg->tone != NULL) ? KYMERA_CONFIG_TONE_VOLUME : KYMERA_CONFIG_PROMPT_VOLUME;

    if (msg->tone != NULL && msg->is_loud)
    {
        volume_db = KYMERA_CONFIG_LOUD_TONE_VOLUME;
        KymeraOutput_DisableSoundLimiter();
    }

    DEBUG_LOG("appKymeraHandleInternalTonePromptPlay, prompt %x, tone %p, ttp %d, int %u, lock 0x%x, mask 0x%x",
                msg->prompt, msg->tone, msg->time_to_play, msg->interruptible, msg->client_lock, msg->client_lock_mask);

    kymera_SendStartInd(msg);

    /* If there is a tone still playing at this point, it must be an interruptable tone, so cut it off */
    if(appKymeraIsPlayingPrompt() || (!kymera_IsTheCorrectTonePromptChainReady(msg) && kymera_TonePromptIsReady()))
    {
        appKymeraTonePromptStop();
    }

    kymera_PrepareOutputChain(msg->rate);
    KymeraOutput_ChainStart();
    output.aux = kymera_PrepareInputChain(msg);
    PanicFalse(Kymera_OutputConnect(output_user_prompt, &output));
    KymeraOutput_SetAuxVolume(volume_db);

    if (KymeraOutput_SetAuxTtp(msg->time_to_play))
    {
        rtime_t now = SystemClockGetTimerTime();
        rtime_t delta = rtime_sub(msg->time_to_play, now);
        DEBUG_LOG("appKymeraHandleInternalTonePromptPlay now=%u, ttp=%u, left=%d", now, msg->time_to_play, delta);
        uint16 delta_in_ms = (uint16)(delta/1000);
        TimestampEvent_Offset(TIMESTAMP_EVENT_PROMPT_PLAY, delta_in_ms);
    }

    /* Start tone */
    if (theKymera->chain_tone_handle)
    {
        ChainStart(theKymera->chain_tone_handle);
    }

    kymera_tone_state = kymera_tone_playing;
    /* May need to exit low power mode to play tone simultaneously */
    appKymeraConfigureDspPowerMode();

    if (!msg->interruptible)
    {
        appKymeraSetToneLock(theKymera);
    }
    theKymera->tone_client_lock = msg->client_lock;
    theKymera->tone_client_lock_mask = msg->client_lock_mask;
}

static bool kymera_OutputDisconnectRequest(void)
{
    // If idle it shouldn't be prepared/connected with the output chain
    PanicFalse(kymera_tone_state != kymera_tone_idle);
    return TRUE;
}

static void kymera_PrepareForOutputChainDisconnect(void)
{
    if (appKymeraIsPlayingPrompt())
    {
        kymeraTaskData *theKymera = KymeraGetTaskData();
        KymeraOutput_SetAuxVolume(0);

        if (theKymera->prompt_source)
        {
            kymera_ClosePromptSource(theKymera->prompt_source);
            theKymera->prompt_source = NULL;
            theKymera->prompt = FILE_NONE;
        }

        if (theKymera->chain_tone_handle)
        {
            ChainStop(theKymera->chain_tone_handle);
        }
    }
}

static void kymera_CompleteOutputChainDisconnect(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    if(theKymera->chain_tone_handle)
    {
        ChainDestroy(theKymera->chain_tone_handle);
        theKymera->chain_tone_handle = NULL;
    }

    appKymeraClearToneLock(theKymera);

    if(appKymeraIsPlayingPrompt())
    {
        PanicZero(theKymera->tone_count);
        theKymera->tone_count--;
    }

    kymera_tone_state = kymera_tone_idle;

     /* Return to low power mode (if applicable) */
    appKymeraConfigureDspPowerMode();

    /* Tone now stopped, clear the client's lock */
    if (theKymera->tone_client_lock)
    {
        *theKymera->tone_client_lock &= ~theKymera->tone_client_lock_mask;
        theKymera->tone_client_lock = 0;
        theKymera->tone_client_lock_mask = 0;
    }
}

void appKymeraTonePromptStop(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
   
    /* Exit if there isn't a tone or prompt playing */
    if ((!theKymera->chain_tone_handle) && (!theKymera->prompt_source) && (!kymera_TonePromptIsReady()))
    {
        return;
    }

    DEBUG_LOG("appKymeraTonePromptStop, state %u", appKymeraGetState());

    kymera_PrepareForOutputChainDisconnect();
    /* Keep framework enabled until after disconnect completion and DSP clock update */
    OperatorsFrameworkEnable();
    Kymera_OutputDisconnect(output_user_prompt);
    kymera_CompleteOutputChainDisconnect();
    OperatorsFrameworkDisable();
    KymeraOutput_EnableSoundLimiter();
}

bool Kymera_PrepareForPrompt(promptFormat format, uint16 sample_rate)
{
    bool prepared = FALSE;

    if(kymera_tone_state == kymera_tone_idle)
    {
        kymeraTaskData *theKymera = KymeraGetTaskData();
        kymera_PrepareOutputChain(sample_rate);
        kymera_CreatePromptChain(format);
        MessageSendLater(&theKymera->task, KYMERA_INTERNAL_PREPARE_FOR_PROMPT_TIMEOUT, NULL, PREPARE_FOR_PROMPT_TIMEOUT);
        prepared = TRUE;
    }

    DEBUG_LOG("Kymera_PrepareForPrompt prepared %u, format %u rate %u", prepared, format, sample_rate);

    return prepared;
}

bool Kymera_IsReadyForPrompt(promptFormat format, uint16 sample_rate)
{
    bool is_ready_for_prompt = ((kymera_tone_state == kymera_tone_ready_prompt) && kymera_IsTheCorrectPromptChainReady(format));
    DEBUG_LOG("Kymera_IsReadyForPrompt %u, format %u rate %u", is_ready_for_prompt, format, sample_rate);
    return is_ready_for_prompt;
}

void Kymera_PromptLoadDownloadableCaps(void)
{
    kymera_output_chain_config config = {0};
    KymeraOutput_SetDefaultOutputChainConfig(&config, 0, 0, 0);
    KymeraOutput_LoadDownloadableCaps(config.chain_type);
    ChainLoadDownloadableCapsFromChainConfig(Kymera_GetChainConfigs()->chain_prompt_sbc_config);
}

void Kymera_PromptUnloadDownloadableCaps(void)
{
    kymera_output_chain_config config = {0};
    KymeraOutput_SetDefaultOutputChainConfig(&config, 0, 0, 0);
    KymeraOutput_UnloadDownloadableCaps(config.chain_type);
    ChainUnloadDownloadableCapsFromChainConfig(Kymera_GetChainConfigs()->chain_prompt_sbc_config);
}

void appKymeraTonePromptInit(void)
{
    Kymera_OutputRegister(&output_info);
}
