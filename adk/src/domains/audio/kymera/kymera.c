/*!
\copyright  Copyright (c) 2017-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera.c
\brief      Kymera Manager
*/

#include "kymera.h"
#include "kymera_a2dp.h"
#include "kymera_a2dp_source.h"
#include "kymera_wired_analog.h"
#include "kymera_dsp_clock.h"
#include "kymera_external_amp.h"
#include "kymera_config.h"
#include "kymera_sco_private.h"
#include "kymera_anc_common.h"
#include "kymera_aec.h"
#include "kymera_loopback_audio.h"
#include "kymera_music_processing.h"
#include "kymera_lock.h"
#include "kymera_state.h"
#include "kymera_op_msg.h"
#include "kymera_internal_msg_ids.h"
#include "kymera_output.h"
#include "kymera_anc.h"
#include "kymera_broadcast_concurrency.h"
#include "av.h"
#include "a2dp_profile.h"
#include "usb_common.h"
#include "power_manager.h"
#include "kymera_latency_manager.h"
#include "kymera_dynamic_latency.h"
#include "mirror_profile.h"
#include "anc_state_manager.h"
#include <vmal.h>
#include "kymera_usb_audio.h"
#include "kymera_usb_voice.h"
#include "kymera_usb_sco.h"
#include "kymera_va.h"
#include "kymera_fit_test.h"
#include "kymera_tones_prompts.h"
#include "kymera_leakthrough.h"
#include "kymera_output_ultra_quiet_dac.h"
#include "kymera_setup.h"
#include "kymera_statistics.h"
#include "kymera_msg.h"
#include "kymera_debug_utils.h"

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
#include "kymera_le_audio.h"
#endif

#if defined(INCLUDE_LE_AUDIO_UNICAST)
#include "kymera_le_voice.h"
#include "kymera_le_mic_chain.h"
#endif

#ifdef INCLUDE_LE_AUDIO_USB_SOURCE
#include "kymera_usb_le_audio.h"
#endif

#include <task_list.h>
#include <logging.h>
#include <ps.h>
#include <rtime.h>
#include "wind_detect.h"

#ifdef ENABLE_SELF_SPEECH
#include "self_speech.h"
#endif

#include <opmsg_prim.h>


/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_ENUM(app_kymera_internal_message_ids)
LOGGING_PRESERVE_MESSAGE_ENUM(kymera_messages)
LOGGING_PRESERVE_MESSAGE_TYPE(kymera_msg_t)

#ifndef HOSTED_TEST_ENVIRONMENT

/*! There is checking that the messages assigned by this module do
not overrun into the next module's message ID allocation */
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(KYMERA, KYMERA_MESSAGE_END)

#endif

/* Constants for CVC Operator Set Control Mode values */
#define CONTROL_MODE_FULL_PROCESSING          2
#define CONTROL_MODE_CVC_RCV_PASSTHROUGH_MIC1 3
#define CONTROL_MODE_CVC_SND_PASSTHROUGH_MIC1 4

#define IS_LEA_MEDIA_SENDER_AUDIO_SOURCE_VALID(audio_source) (audio_source < KYMERA_AUDIO_SOURCES_MAX)
#define IS_LEA_MEDIA_SENDER_AUDIO_SOURCE_AVAILABLE(audio_source) (audio_source != KYMERA_AUDIO_SOURCE_NONE)

/*!< State data for the DSP configuration */
kymeraTaskData  app_kymera;

/*! Macro for creating messages */
#define MAKE_KYMERA_MESSAGE(TYPE) \
    TYPE##_T *message = PanicUnlessNew(TYPE##_T);

/*! \brief The KYMERA_INTERNAL_SCO_MIC_MUTE message content. */
typedef struct
{
    /*! TRUE to enable mute, FALSE to disable mute. */
    bool mute;
} KYMERA_INTERNAL_SCO_MIC_MUTE_T;

/*! \brief The KYMERA_INTERNAL_SCO_SET_VOL message content. */
typedef struct
{
    /*! The volume to set. */
    int16 volume_in_db;
} KYMERA_INTERNAL_SCO_SET_VOL_T;

/*! \brief The KYMERA_INTERNAL_QHS_LEVEL_CHANGED message content. */
typedef struct
{
    /*! The qhs_level to set. */
    uint16 qhs_level;
} KYMERA_INTERNAL_QHS_LEVEL_CHANGED_T;

#if defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER)
/*! \brief The KYMERA_INTERNAL_LEA_MEDIA_BROADCAST_REQ message content */
typedef struct
{
    /*! TRUE to enable broadacst, FALSE to disable broadcast */
    bool broadcast_enable;
    /*! Media sender audio source */
    appKymeraLeAudioMediaSenderSourceType audio_source;
} KYMERA_INTERNAL_LEA_MEDIA_BROADCAST_REQ_T;
#endif /* defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER) */

/*! \brief The KYMERA_INTERNAL_LE_VOICE_MIC_MUTE message content. */
typedef KYMERA_INTERNAL_SCO_MIC_MUTE_T KYMERA_INTERNAL_LE_VOICE_MIC_MUTE_T;

static const appKymeraScoChainInfo *appKymeraScoChainTable = NULL;

static const capability_bundle_config_t *bundle_config = NULL;

static const kymera_chain_configs_t *chain_configs = NULL;

static const kymera_callback_configs_t *callback_configs = NULL;

#if defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER)
static Kymera_LeaMediaBroadcastRequestCallback LeaMediaBroadcastRequestHandler[KYMERA_AUDIO_SOURCES_MAX] = {NULL};
#endif /* defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER) */

#if defined(INCLUDE_LE_AUDIO_UNICAST)
static void appKymeraHandleInternalLeaMicSynchronised(void);
#endif

#define GetKymeraClients() TaskList_GetBaseTaskList(&KymeraGetTaskData()->client_tasks)

static void kymera_FrameworkEnabled(bool exiting_lp_mode)
{
    UNUSED(exiting_lp_mode);
    DEBUG_LOG("kymera_FrameworkEnabled");
    appPowerPerformanceProfileRelinquish();
    appKymeraResetCurrentDspClockConfig();
}

static const vmal_callbacks_t vmal_callbacks =
{
    .TurnedFrameworkOn = kymera_FrameworkEnabled,
};

static const vmal_registry_per_observer_t vmal_registration =
{
    .callbacks = &vmal_callbacks,
};

static void appKymeraScoStartHelper(Sink audio_sink, const appKymeraScoChainInfo *info, uint8 wesco,
                                    int16 volume_in_db, uint8 pre_start_delay, bool conditionally,
                                    bool synchronised_start, Kymera_ScoStartedHandler started_handler)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_SCO_START);
    PanicNull(audio_sink);

    message->audio_sink = audio_sink;
    message->wesco      = wesco;
    message->volume_in_db     = volume_in_db;
    message->pre_start_delay = pre_start_delay;
    message->sco_info   = info;
    message->synchronised_start = synchronised_start;
    message->started_handler = started_handler;
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_SCO_START, message, conditionally ? &theKymera->lock : NULL);
}

static const appKymeraScoChainInfo *appKymeraScoFindChain(const appKymeraScoChainInfo *info, appKymeraScoMode mode, uint8 mic_cfg)
{
    while (info->mode != NO_SCO)
    {
        if ((info->mode == mode) && (info->mic_cfg == mic_cfg))
        {
            return info;
        }

        info++;
    }
    return NULL;
}

static void kymera_dsp_msg_handler(MessageFromOperator *op_msg)
{
    uint16 event_id;
    kymera_op_unsolicited_message_ids_t msg_id;


    msg_id = op_msg->message[KYMERA_OP_MSG_WORD_MSG_ID];
    event_id = op_msg->message[KYMERA_OP_MSG_WORD_EVENT_ID];

    DEBUG_LOG("KYMERA_OP_UNSOLICITED_MSG_ID: enum:kymera_op_unsolicited_message_ids_t:%d, EVENT_ID:%d", msg_id, event_id);

    switch (msg_id)
    {
        case KYMERA_OP_MSG_ID_TONE_END:
        {
            DEBUG_LOG("KYMERA_OP_MSG_ID_TONE_END");

            PanicFalse(op_msg->len == KYMERA_OP_MSG_LEN);

            appKymeraTonePromptStop();
            Kymera_LatencyManagerHandleToneEnd();
        }
        break;
        
        case KYMERA_OP_MSG_ID_AANC_EVENT_TRIGGER:
        {
            DEBUG_LOG("KYMERA_OP_MSG_ID_AANC_EVENT_TRIGGER Event: enum:kymera_aanc_op_event_ids_t:%d",event_id);
            if (event_id ==  KYMERA_AANC_EVENT_ED_ACTIVE )
            {
                appKymera_MsgRegisteredClients(KYMERA_AANC_ED_ACTIVE_TRIGGER_IND,
                                               op_msg->message[KYMERA_OP_MSG_WORD_PAYLOAD_0]);
            }
            else if (event_id ==  KYMERA_AANC_EVENT_ED_INACTIVE_GAIN_UNCHANGED )
            {
                appKymera_MsgRegisteredClients(KYMERA_AANC_ED_INACTIVE_TRIGGER_IND,
                                               op_msg->message[KYMERA_OP_MSG_WORD_PAYLOAD_0]);
            }
            else if (event_id ==  KYMERA_AANC_EVENT_QUIET_MODE )
            {
                appKymera_MsgRegisteredClients(KYMERA_AANC_QUIET_MODE_TRIGGER_IND,
                                               KYMERA_OP_MSG_WORD_PAYLOAD_NA);
            }
            else if (event_id ==  KYMERA_AANC_EVENT_BAD_ENVIRONMENT )
            {
                appKymera_MsgRegisteredClients(KYMERA_AANC_BAD_ENVIRONMENT_TRIGGER_IND,
                                               op_msg->message[KYMERA_OP_MSG_WORD_PAYLOAD_0]);
            }
            else
            {
                /* ignore */
            }
        }
        break;

        case KYMERA_OP_MSG_ID_AANC_EVENT_CLEAR:
        {
            DEBUG_LOG("KYMERA_OP_MSG_ID_AANC_EVENT_CLEAR Event: enum:kymera_aanc_op_event_ids_t:%d", event_id);

            if (event_id ==  KYMERA_AANC_EVENT_ED_ACTIVE )
            {
                appKymera_MsgRegisteredClients(KYMERA_AANC_ED_ACTIVE_CLEAR_IND,
                                               op_msg->message[KYMERA_OP_MSG_WORD_PAYLOAD_0]);
            }
            else if (event_id ==  KYMERA_AANC_EVENT_ED_INACTIVE_GAIN_UNCHANGED )
            {
                appKymera_MsgRegisteredClients(KYMERA_AANC_ED_INACTIVE_CLEAR_IND,
                                               op_msg->message[KYMERA_OP_MSG_WORD_PAYLOAD_0]);
            }
            else if (event_id ==  KYMERA_AANC_EVENT_QUIET_MODE )
            {
                appKymera_MsgRegisteredClients(KYMERA_AANC_QUIET_MODE_CLEAR_IND,
                                               KYMERA_OP_MSG_WORD_PAYLOAD_NA);
            }
            else if (event_id ==  KYMERA_AANC_EVENT_BAD_ENVIRONMENT )
            {
                appKymera_MsgRegisteredClients(KYMERA_AANC_BAD_ENVIRONMENT_CLEAR_IND,
                                               op_msg->message[KYMERA_OP_MSG_WORD_PAYLOAD_0]);
            }
            else
            {
                /* ignore */
            }
        }
        break;

        case KYMERA_OP_MSG_ID_FIT_TEST:
        {
            if(event_id == KYMERA_FIT_TEST_EVENT_ID)
            {
                if(op_msg->message[KYMERA_OP_MSG_WORD_PAYLOAD_0] == KYMERA_FIT_TEST_RESULT_GOOD)
                {
                    appKymera_MsgRegisteredClients(KYMERA_EFT_GOOD_FIT_IND, KYMERA_OP_MSG_WORD_PAYLOAD_NA);
                    DEBUG_LOG_ALWAYS("kymera_dsp_msg_handler: Good Fit!!");
                }
                else if(op_msg->message[KYMERA_OP_MSG_WORD_PAYLOAD_0] == KYMERA_FIT_TEST_RESULT_BAD)
                {
                    appKymera_MsgRegisteredClients(KYMERA_EFT_BAD_FIT_IND, KYMERA_OP_MSG_WORD_PAYLOAD_NA);
                    DEBUG_LOG_ALWAYS("kymera_dsp_msg_handler: Bad Fit!!");
                }
                else if(op_msg->message[KYMERA_OP_MSG_WORD_PAYLOAD_0] == KYMERA_FIT_TEST_DATA_AVAILABLE)
                {
#ifdef ENABLE_CONTINUOUS_EARBUD_FIT_TEST
                    KymeraFitTest_ContinuousGetCapturedBins(0,0); /* Ref 0 .. 1.75kHz */
                    KymeraFitTest_ContinuousGetCapturedBins(0,1); /* Ref 2 .. 3.75kHz */
                    KymeraFitTest_ContinuousGetCapturedBins(0,2); /* Ref 4 .. 5.75kHz */
                    KymeraFitTest_ContinuousGetCapturedBins(0,3); /* Ref 6 .. 7.75kHz */
                    KymeraFitTest_ContinuousGetCapturedBins(1,0); /* Mic 0 .. 1.75kHz */
                    KymeraFitTest_ContinuousGetCapturedBins(1,1); /* Mic 2 .. 3.75kHz */
                    KymeraFitTest_ContinuousGetCapturedBins(1,2); /* Mic 4 .. 5.75kHz */
                    KymeraFitTest_ContinuousGetCapturedBins(1,3); /* Mic 6 .. 7.75kHz */
#else
                    DEBUG_LOG_ALWAYS("kymera_dsp_msg_handler: Data available");
#endif
                }
            }
        }
        break;
    case KYMERA_OP_MSG_ID_AHM_EVENT:
        {            
            DEBUG_LOG_ALWAYS("KYMERA_OP_MSG_ID_AHM_EVENT ");
            /*AHM*/
            if (op_msg->message[KYMERA_AHM_EVENT_ID_INDEX] ==  KYMERA_AHM_EVENT_ID_FF_RAMP )
            {            
                if(op_msg->message[KYMERA_AHM_EVENT_TYPE_INDEX] == KYMERA_AHM_EVENT_TYPE_TRIGGER)
                {
                    /* FF Ramp started; do nothing */                    
                    DEBUG_LOG_ALWAYS("AHM FF RAMP TRIGGER!!");
                }
                else /* KYMERA_AHM_EVENT_TYPE_CLEAR */
                {
                    /* FF Ramp finished */                    
                    DEBUG_LOG_ALWAYS("AHM FF RAMP CLEAR!!");
                    KymeraAncCommon_RampCompleteAction();
                }
            }
            else if (op_msg->message[KYMERA_AHM_EVENT_ID_INDEX] ==  KYMERA_AHM_EVENT_ID_FB_RAMP )
            {            
                if(op_msg->message[KYMERA_AHM_EVENT_TYPE_INDEX] == KYMERA_AHM_EVENT_TYPE_TRIGGER)
                {
                    /* FB Ramp started; do nothing */
                    DEBUG_LOG_ALWAYS("AHM FB RAMP TRIGGER!!");
                }
                else /* KYMERA_AHM_EVENT_TYPE_CLEAR */
                {                
                    /* FB Ramp finished */
                    DEBUG_LOG_ALWAYS("AHM FB RAMP CLEAR!!");
                }
            }            
            /*WIND DETECT*/
            else if (op_msg->message[KYMERA_AHM_EVENT_ID_INDEX] ==  KYMERA_WIND_DETECT_ID_1MIC)
            {            
                if(op_msg->message[KYMERA_AHM_EVENT_TYPE_INDEX] == KYMERA_AHM_EVENT_TYPE_TRIGGER)
                {
                    DEBUG_LOG_ALWAYS("1MIC WIND ATTACK!, intensity: enum:wind_detect_intensity_t:%d", op_msg->message[KYMERA_AHM_EVENT_PAYLOAD_A_INDEX]);
                    appKymera_MsgRegisteredClients(KYMERA_WIND_STAGE1_DETECTED, op_msg->message[KYMERA_AHM_EVENT_PAYLOAD_A_INDEX]);
                }
                else
                {
                    DEBUG_LOG_ALWAYS("1MIC WIND RELEASE!!");
                    appKymera_MsgRegisteredClients(KYMERA_WIND_STAGE1_RELEASED,KYMERA_OP_MSG_WORD_PAYLOAD_NA);
                }
            }            
            else if (op_msg->message[KYMERA_AHM_EVENT_ID_INDEX] ==  KYMERA_WIND_DETECT_ID_2MIC)
            {            
                if(op_msg->message[KYMERA_AHM_EVENT_TYPE_INDEX] == KYMERA_AHM_EVENT_TYPE_TRIGGER)
                {
                    DEBUG_LOG_ALWAYS("2MIC WIND ATTACK!, intensity: enum:wind_detect_intensity_t:%d", op_msg->message[KYMERA_AHM_EVENT_PAYLOAD_A_INDEX]);
                    appKymera_MsgRegisteredClients(KYMERA_WIND_STAGE2_DETECTED, op_msg->message[KYMERA_AHM_EVENT_PAYLOAD_A_INDEX]);
                }
                else
                {
                    DEBUG_LOG_ALWAYS("2MIC WIND RELEASE!!");
                    appKymera_MsgRegisteredClients(KYMERA_WIND_STAGE2_RELEASED,KYMERA_OP_MSG_WORD_PAYLOAD_NA);
                }
            }
            /*ATR VAD SELF SPEECH*/
            else if (op_msg->message[KYMERA_AHM_EVENT_ID_INDEX] ==  KYMERA_ATR_VAD_ID_1MIC)
            {            
                if(op_msg->message[KYMERA_AHM_EVENT_TYPE_INDEX] == KYMERA_AHM_EVENT_TYPE_TRIGGER)
                {
                    DEBUG_LOG_ALWAYS("1MIC ATR VAD SELF SPEECH TRIGGER!");
#if defined (ENABLE_AUTO_AMBIENT)
                    AncAutoAmbient_Trigger();
#endif
                }
                else
                {
                    DEBUG_LOG_ALWAYS("1MIC ATR VAD SELF SPEECH CLEAR!");   
#if defined (ENABLE_AUTO_AMBIENT)
                    AncAutoAmbient_Release();
#endif
                }
            }            
            else if (op_msg->message[KYMERA_AHM_EVENT_ID_INDEX] ==  KYMERA_ATR_VAD_ID_2MIC)
            {            
                if(op_msg->message[KYMERA_AHM_EVENT_TYPE_INDEX] == KYMERA_AHM_EVENT_TYPE_TRIGGER)
                {
                    DEBUG_LOG_ALWAYS("2MIC ATR VAD SELF SPEECH TRIGGER!");
                }
                else
                {
                    DEBUG_LOG_ALWAYS("2MIC ATR VAD SELF SPEECH CLEAR!");
                }
            }
            /*NOISE ID*/
            else if (op_msg->message[KYMERA_AHM_EVENT_ID_INDEX] ==  KYMERA_NOISE_ID)
            {            
                if (op_msg->message[KYMERA_AHM_EVENT_TYPE_INDEX] == KYMERA_NOISE_ID_CATEGORY_0)
                {                
                   DEBUG_LOG_ALWAYS("NOISE_ID_CATEGORY 0!");
                   appKymera_MsgRegisteredClients(KYMERA_AANC_NOISE_ID_IND, ANC_NOISE_ID_CATEGORY_A);
                }
                else if (op_msg->message[KYMERA_AHM_EVENT_TYPE_INDEX] == KYMERA_NOISE_ID_CATEGORY_1)
                {                
                   DEBUG_LOG_ALWAYS("NOISE_ID_CATEGORY 1!");
                   appKymera_MsgRegisteredClients(KYMERA_AANC_NOISE_ID_IND, ANC_NOISE_ID_CATEGORY_B);
                }
                else
                {
                    DEBUG_LOG_ALWAYS("NOISE ID CATEGORY INVALID!");
                }
            }            
            else if (op_msg->message[KYMERA_AHM_EVENT_ID_INDEX] ==  KYMERA_AHM_EVENT_ID_TRANSITION)
            {
                if(op_msg->message[KYMERA_AHM_EVENT_TYPE_INDEX] == KYMERA_AHM_EVENT_TYPE_CLEAR)
                {
#ifdef ENABLE_ANC_FAST_MODE_SWITCH
                    DEBUG_LOG_ALWAYS("AHM Transition complete!");
                    KymeraAncCommon_TransitionCompleteAction();
#endif
                }
            }
            else
            {
                /* ignore */
            }
        }
        break;

        default:
        break;
    }
}

void appKymeraPromptPlay(FILE_INDEX prompt, promptFormat format, uint32 rate, rtime_t ttp,
                         bool interruptible, uint16 *client_lock, uint16 client_lock_mask)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("appKymeraPromptPlay, queue prompt %d, int %u", prompt, interruptible);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_TONE_PROMPT_PLAY);
    message->tone = NULL;
    message->prompt = prompt;
    message->prompt_format = format;
    message->rate = rate;
    message->time_to_play = ttp;
    message->interruptible = interruptible;
    message->client_lock = client_lock;
    message->client_lock_mask = client_lock_mask;

    MessageCancelFirst(&theKymera->task, KYMERA_INTERNAL_PREPARE_FOR_PROMPT_TIMEOUT);
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_TONE_PROMPT_PLAY, message, &theKymera->lock);
    theKymera->tone_count++;
}

void appKymeraTonePlay(const ringtone_note *tone, rtime_t ttp, bool interruptible, bool is_loud,
                       uint16 *client_lock, uint16 client_lock_mask)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("appKymeraTonePlay, queue tone %p, int %u", tone, interruptible);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_TONE_PROMPT_PLAY);
    message->tone = tone;
    message->is_loud = is_loud;
    message->prompt = FILE_NONE;
    message->rate = KYMERA_TONE_GEN_RATE;
    message->time_to_play = ttp;
    message->interruptible = interruptible;
    message->client_lock = client_lock;
    message->client_lock_mask = client_lock_mask;

    MessageCancelFirst(&theKymera->task, KYMERA_INTERNAL_PREPARE_FOR_PROMPT_TIMEOUT);
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_TONE_PROMPT_PLAY, message, &theKymera->lock);
    theKymera->tone_count++;
}

void appKymeraTonePromptCancel(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("appKymeraTonePromptCancel");

    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_TONE_PROMPT_STOP, NULL, &theKymera->lock);
}

void appKymeraCancelA2dpStart(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    MessageCancelAll(&theKymera->task, KYMERA_INTERNAL_A2DP_START);
    appKymeraClearA2dpStartingLock(theKymera);
}

void appKymeraA2dpStart(uint16 *client_lock, uint16 client_lock_mask,
                        const a2dp_codec_settings *codec_settings,
                        uint32 max_bitrate,
                        int16 volume_in_db, uint8 master_pre_start_delay,
                        uint8 q2q_mode, aptx_adaptive_ttp_latencies_t nq2q_ttp)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraA2dpStart, seid %u, lock %u, busy_lock %u, q2q %u, features 0x%x", codec_settings->seid, theKymera->lock, theKymera->busy_lock, q2q_mode, codec_settings->codecData.aptx_ad_params.features);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_A2DP_START);
    message->lock = client_lock;
    message->lock_mask = client_lock_mask;
    message->codec_settings = *codec_settings;
    message->volume_in_db = volume_in_db;
    message->master_pre_start_delay = master_pre_start_delay;
    message->q2q_mode = q2q_mode;
    message->nq2q_ttp = nq2q_ttp;
    message->max_bitrate = max_bitrate;
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_A2DP_START,
                             message,
                             &theKymera->lock);
}

void appKymeraA2dpStop(uint8 seid, Source source)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    MessageId mid = appA2dpIsSeidSource(seid) ? KYMERA_INTERNAL_A2DP_STOP_FORWARDING :
                                                KYMERA_INTERNAL_A2DP_STOP;
    DEBUG_LOG("appKymeraA2dpStop, seid %u", seid);

    /*Cancel any pending KYMERA_INTERNAL_A2DP_AUDIO_SYNCHRONISED message.
      A2DP might have been stopped while Audio Synchronization is still incomplete,
      in which case this timed message needs to be cancelled.
     */
    MessageCancelAll(&theKymera->task, KYMERA_INTERNAL_A2DP_AUDIO_SYNCHRONISED);

    /*Cancel any pending KYMERA_INTERNAL_A2DP_START message.*/
    appKymeraCancelA2dpStart();

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_A2DP_STOP);
    message->seid = seid;
    message->source = source;
    MessageSendConditionally(&theKymera->task, mid, message, &theKymera->lock);
}

void appKymeraA2dpSetVolume(int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraA2dpSetVolume, volume %d", volume_in_db);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_A2DP_SET_VOL);
    message->volume_in_db = volume_in_db;
    MessageCancelFirst(&theKymera->task, KYMERA_INTERNAL_A2DP_SET_VOL);
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_A2DP_SET_VOL, message, &theKymera->lock);
}

bool appKymeraScoStart(Sink audio_sink, appKymeraScoMode mode, uint8 wesco, int16 volume_in_db, uint8 pre_start_delay,
                       bool synchronised_start, Kymera_ScoStartedHandler handler)
{
    uint8 mic_cfg = appConfigVoiceGetNumberOfMics();
    const appKymeraScoChainInfo *info = appKymeraScoFindChain(appKymeraScoChainTable, mode, mic_cfg);
    if (!info)
        info = appKymeraScoFindChain(appKymeraScoChainTable,
                                     mode, mic_cfg);

    if (info)
    {
        DEBUG_LOG("appKymeraScoStart, queue sink 0x%x", audio_sink);

        if (audio_sink)
        {
            DEBUG_LOG("appKymeraScoStart, queue sink 0x%x", audio_sink);
            appKymeraScoStartHelper(audio_sink, info, wesco, volume_in_db, pre_start_delay, TRUE,
                                    synchronised_start, handler);
            return TRUE;
        }
        else
        {
            DEBUG_LOG("appKymeraScoStart, invalid sink");
            return FALSE;
        }
    }
    else
    {
        DEBUG_LOG("appKymeraScoStart, failed to find suitable SCO chain");
        return FALSE;
    }
}

void appKymeraScoStop(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraScoStop");

    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_SCO_STOP, NULL, &theKymera->lock);
}

void appKymeraScoSetVolume(int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("appKymeraScoSetVolume msg, vol %d", volume_in_db);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_SCO_SET_VOL);
    message->volume_in_db = volume_in_db;
    MessageCancelFirst(&theKymera->task, KYMERA_INTERNAL_SCO_SET_VOL);
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_SCO_SET_VOL, message, &theKymera->lock);
}

void appKymeraScoMicMute(bool mute)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("appKymeraScoMicMute msg, mute %u", mute);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_SCO_MIC_MUTE);
    message->mute = mute;
    MessageSend(&theKymera->task, KYMERA_INTERNAL_SCO_MIC_MUTE, message);
}

void appKymeraUpdateQhsLevel(uint16 qhs_level)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("appKymeraUpdateQhsLevel msg, qhs_level %d", qhs_level);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_QHS_LEVEL_CHANGED);
    message->qhs_level = qhs_level;
    MessageSend(&theKymera->task, KYMERA_INTERNAL_QHS_LEVEL_CHANGED, message);
}

void appKymeraProspectiveDspPowerOn(appKymeraPowerActivationMode mode)
{
    DEBUG_LOG_FN_ENTRY("appKymeraProspectiveDspPowerOn enum:appKymeraPowerActivationMode:%d", mode);

    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraProspectiveDspPowerOn handled_async_prospective_dsp_power_on %u handled_sync_prospective_dsp_power_on %u", theKymera->handled_async_prospective_dsp_power_on, theKymera->handled_sync_prospective_dsp_power_on);

    switch(mode)
    {
        case KYMERA_POWER_ACTIVATION_MODE_ASYNC:
        {
            if(!theKymera->handled_async_prospective_dsp_power_on && !theKymera->handled_sync_prospective_dsp_power_on)
            {
                DEBUG_LOG("appKymeraProspectiveDspPowerOn executing asynchronous power on");
                appPowerPerformanceProfileRequest();

                if(VmalOperatorFrameworkEnableMainProcessorAsync())
                {
                    appPowerPerformanceProfileRelinquish();
                }

                theKymera->handled_async_prospective_dsp_power_on = TRUE;
            }
        }
        break;

        case KYMERA_POWER_ACTIVATION_MODE_IMMEDIATE:
        {
            if(!theKymera->handled_sync_prospective_dsp_power_on)
            {
                DEBUG_LOG("appKymeraProspectiveDspPowerOn executing synchronous power on");
                appPowerPerformanceProfileRequest();
                VmalOperatorFrameworkEnableMainProcessor();
                appPowerPerformanceProfileRelinquish();
                theKymera->handled_sync_prospective_dsp_power_on = TRUE;
            }
        }
        break;

        default:
        {
            Panic();
        }
        break;
    }

    MessageCancelFirst(KymeraGetTask(), KYMERA_INTERNAL_PROSPECTIVE_POWER_OFF);
    MessageSendLater(KymeraGetTask(), KYMERA_INTERNAL_PROSPECTIVE_POWER_OFF, NULL,
                     appConfigProspectiveAudioOffTimeout());
}

/* Handle KYMERA_INTERNAL_PROSPECTIVE_POWER_OFF - switch off DSP again */
static void appKymeraHandleProspectivePowerOff(void)
{
    DEBUG_LOG_FN_ENTRY("appKymeraHandleProspectivePowerOff");
    kymeraTaskData *theKymera = KymeraGetTaskData();

    if(theKymera->handled_async_prospective_dsp_power_on)
    {
        OperatorsFrameworkDisable();
        theKymera->handled_async_prospective_dsp_power_on = FALSE;
    }

    if(theKymera->handled_sync_prospective_dsp_power_on)
    {
        OperatorsFrameworkDisable();
        theKymera->handled_sync_prospective_dsp_power_on = FALSE;
    }
}

static void appKymeraHandleInternalScoAudioSynchronised(void)
{
    if (appKymeraGetState() == KYMERA_STATE_SCO_ACTIVE)
    {
        DEBUG_LOG("appKymeraHandleInternalScoAudioSynchronised");
        KymeraOutput_MuteMainChannel(FALSE);
    }
}

static void kymera_msg_handler(Task task, MessageId id, Message msg)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    UNUSED(task);

    switch (id)
    {
        case MESSAGE_FROM_OPERATOR:
            kymera_dsp_msg_handler((MessageFromOperator *)msg);
        break;

        case MESSAGE_SOURCE_EMPTY:
        break;

        case MESSAGE_STREAM_DISCONNECT:
        {
            DEBUG_LOG("appKymera MESSAGE_STREAM_DISCONNECT");
        }
        break;

        case KYMERA_INTERNAL_A2DP_START:
        {
            const KYMERA_INTERNAL_A2DP_START_T *m = (const KYMERA_INTERNAL_A2DP_START_T *)msg;
            uint8 seid = m->codec_settings.seid;

            /* If there is no pre-start delay, or during the pre-start delay, the
            start can be cancelled if there is a stop on the message queue */
            MessageId mid = appA2dpIsSeidSource(seid) ? KYMERA_INTERNAL_A2DP_STOP_FORWARDING :
                                                        KYMERA_INTERNAL_A2DP_STOP;
            if (MessageCancelFirst(&theKymera->task, mid))
            {
                /* A stop on the queue was cancelled, clear the starter's lock
                and stop starting */
                DEBUG_LOG("appKymera not starting due to queued stop, seid=%u", seid);
                if (m->lock)
                {
                    *m->lock &= ~m->lock_mask;
                }
                /* Also clear kymera's lock, since no longer starting */
                appKymeraClearA2dpStartingLock(theKymera);
                break;
            }
            if (m->master_pre_start_delay)
            {
                /* Send another message before starting kymera. */
                MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_A2DP_START);
                *message = *m;
                --message->master_pre_start_delay;
                MessageSend(&theKymera->task, id, message);
                appKymeraSetA2dpStartingLock(theKymera);
                break;
            }
        }
        // fallthrough (no message cancelled, zero master_pre_start_delay)
        case KYMERA_INTERNAL_A2DP_STARTING:
        {
#if defined(INCLUDE_MIRRORING) || defined(INCLUDE_STEREO)
            const KYMERA_INTERNAL_A2DP_START_T *m = (const KYMERA_INTERNAL_A2DP_START_T *)msg;
            if (Kymera_A2dpHandleInternalStart(m))
            {
                KymeraDebugUtil_StartAudioDspMonitor();
                /* Start complete, clear locks. */
                appKymeraClearA2dpStartingLock(theKymera);
            }
            else
            {
                /* Start incomplete, send another message. */
                MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_A2DP_START);
                *message = *m;
                MessageSend(&theKymera->task, KYMERA_INTERNAL_A2DP_STARTING, message);
                appKymeraSetA2dpStartingLock(theKymera);
            }
#endif
        }
        break;

        case KYMERA_INTERNAL_A2DP_STOP:
            KymeraDebugUtil_StopAudioDspMonitor();
            /* Flow Through */
        case KYMERA_INTERNAL_A2DP_STOP_FORWARDING:
#if defined(INCLUDE_MIRRORING) || defined(INCLUDE_STEREO)
            Kymera_A2dpHandleInternalStop(msg);
#endif
        break;

        case KYMERA_INTERNAL_A2DP_SET_VOL:
        {
#if defined(INCLUDE_MIRRORING) || defined(INCLUDE_STEREO)
            KYMERA_INTERNAL_A2DP_SET_VOL_T *m = (KYMERA_INTERNAL_A2DP_SET_VOL_T *)msg;
            Kymera_A2dpHandleInternalSetVolume(m->volume_in_db);
#endif
        }
        break;

        case KYMERA_INTERNAL_SCO_START:
        {
            const KYMERA_INTERNAL_SCO_START_T *m = (const KYMERA_INTERNAL_SCO_START_T *)msg;

            if (m->pre_start_delay)
            {
                /* Resends are sent unconditonally, but the lock is set blocking
                   other new messages */
                appKymeraSetScoStartingLock(KymeraGetTaskData());
                appKymeraScoStartHelper(m->audio_sink, m->sco_info, m->wesco, m->volume_in_db,
                                        m->pre_start_delay - 1, FALSE,
                                        m->synchronised_start, m->started_handler);
            }
            else
            {
                Kymera_ScoHandleInternalStart(m);
                KymeraDebugUtil_StartAudioDspMonitor();
                appKymeraClearScoStartingLock(KymeraGetTaskData());
            }
        }
        break;

        case KYMERA_INTERNAL_SCO_SET_VOL:
        {
            KYMERA_INTERNAL_SCO_SET_VOL_T *m = (KYMERA_INTERNAL_SCO_SET_VOL_T *)msg;
            Kymera_ScoHandleInternalSetVolume(m->volume_in_db);
        }
        break;

        case KYMERA_INTERNAL_SCO_MIC_MUTE:
        {
            KYMERA_INTERNAL_SCO_MIC_MUTE_T *m = (KYMERA_INTERNAL_SCO_MIC_MUTE_T *)msg;
            Kymera_ScoHandleInternalMicMute(m->mute);
        }
        break;

        case KYMERA_INTERNAL_SCO_STOP:
            KymeraDebugUtil_StopAudioDspMonitor();
            Kymera_ScoHandleInternalStop();
        break;

        case KYMERA_INTERNAL_TONE_PROMPT_PLAY:
            appKymeraHandleInternalTonePromptPlay(msg);
        break;

        case KYMERA_INTERNAL_TONE_PROMPT_STOP:
        case KYMERA_INTERNAL_PREPARE_FOR_PROMPT_TIMEOUT:
            appKymeraTonePromptStop();
        break;

        case KYMERA_INTERNAL_ANC_TUNING_START:
            KymeraAnc_TuningCreateChain((const KYMERA_INTERNAL_ANC_TUNING_START_T *)msg);
        break;

        case KYMERA_INTERNAL_ANC_TUNING_STOP:
            KymeraAnc_TuningDestroyChain((const KYMERA_INTERNAL_ANC_TUNING_STOP_T *)msg);
        break;

        case KYMERA_INTERNAL_ADAPTIVE_ANC_TUNING_START:
            KymeraAncCommon_CreateAdaptiveAncTuningChain((const KYMERA_INTERNAL_ADAPTIVE_ANC_TUNING_START_T *)msg);
        break;

        case KYMERA_INTERNAL_ADAPTIVE_ANC_TUNING_STOP:
            KymeraAncCommon_DestroyAdaptiveAncTuningChain((const KYMERA_INTERNAL_ADAPTIVE_ANC_TUNING_STOP_T *)msg);
        break;

        case KYMERA_INTERNAL_AANC_ENABLE:
        case KYMERA_INTERNAL_MIC_CONNECTION_TIMEOUT_ANC:
        {
            KymeraAncCommon_AncEnable((const KYMERA_INTERNAL_AANC_ENABLE_T *)msg);

            /*Inform ANC SM that capabilities are started*/
            MessageSend(AncStateManager_GetTask(), KYMERA_ANC_COMMON_CAPABILITY_ENABLE_COMPLETE, NULL);
        }
        break;
        
#ifdef ENABLE_SELF_SPEECH
        case KYMERA_INTERNAL_SELF_SPEECH_ENABLE_TIMEOUT:            
            Self_Speech_Enable();
            break;
#endif

#ifdef ENABLE_SELF_SPEECH
        case KYMERA_INTERNAL_SELF_SPEECH_DISABLE_TIMEOUT:            
            Self_Speech_Disable();
            break;
#endif

        case KYMERA_INTERNAL_PROSPECTIVE_POWER_OFF:
            appKymeraHandleProspectivePowerOff();
        break;

        case KYMERA_INTERNAL_AUDIO_SS_DISABLE:
            DEBUG_LOG("appKymera KYMERA_INTERNAL_AUDIO_SS_DISABLE");
            OperatorsFrameworkDisable();
            break;
#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
        case KYMERA_INTERNAL_LE_AUDIO_START:
        {
            const KYMERA_INTERNAL_LE_AUDIO_START_T *m = (const KYMERA_INTERNAL_LE_AUDIO_START_T *)msg;
            kymeraLeAudio_Start(m);
            KymeraDebugUtil_StartAudioDspMonitor();
            appKymeraClearLeStartingLock(KymeraGetTaskData());
        }
        break;
        
        case KYMERA_INTERNAL_LE_AUDIO_STOP:
        {
            KymeraDebugUtil_StopAudioDspMonitor();
            kymeraLeAudio_Stop();
        }
        break;
        
        case KYMERA_INTERNAL_LE_AUDIO_SET_VOLUME:
        {
            KYMERA_INTERNAL_LE_AUDIO_SET_VOLUME_T *m = (KYMERA_INTERNAL_LE_AUDIO_SET_VOLUME_T *)msg;
            kymeraLeAudio_SetVolume(m->volume_in_db);
        }
        break;

#ifdef INCLUDE_MIRRORING
        case KYMERA_INTERNAL_LE_AUDIO_UNMUTE:
        {
            KymeraLeAudio_HandleAudioUnmuteInd((const KYMERA_INTERNAL_LE_AUDIO_UNMUTE_T *)msg);
        }
        break;

        case KYMERA_INTERNAL_LE_AUDIO_MUTE:
        {
            KymeraLeAudio_ConfigureStartSync(KymeraGetTaskData(), TRUE);
        }
        break;
#endif /* INCLUDE_MIRRORING */
#endif /* INCLUDE_LE_AUDIO_BROADCAST || INCLUDE_LE_AUDIO_UNICAST */
#ifdef INCLUDE_LE_AUDIO_UNICAST
        case KYMERA_INTERNAL_LE_VOICE_START:
        {
            const KYMERA_INTERNAL_LE_VOICE_START_T *m = (const KYMERA_INTERNAL_LE_VOICE_START_T *)msg;
            KymeraLeVoice_HandleInternalStart(m);
            KymeraDebugUtil_StartAudioDspMonitor();
            appKymeraClearLeStartingLock(KymeraGetTaskData());
        }
        break;

        case KYMERA_INTERNAL_LE_VOICE_STOP:
        {
            KymeraDebugUtil_StopAudioDspMonitor();
            KymeraLeVoice_HandleInternalStop();
        }
        break;

        case KYMERA_INTERNAL_LE_VOICE_SET_VOLUME:
        {
            KYMERA_INTERNAL_LE_VOICE_SET_VOLUME_T *m = (KYMERA_INTERNAL_LE_VOICE_SET_VOLUME_T *)msg;
            KymeraLeVoice_HandleInternalSetVolume(m->volume_in_db);
        }
        break;

        case KYMERA_INTERNAL_LE_VOICE_MIC_MUTE:
        {
            KYMERA_INTERNAL_LE_VOICE_MIC_MUTE_T *m = (KYMERA_INTERNAL_LE_VOICE_MIC_MUTE_T *)msg;
            kymeraLeVoice_HandleInternalMicMute(m->mute);
        }
        break;

        case KYMERA_INTERNAL_LEA_MIC_SYNCHRONISED:
            appKymeraHandleInternalLeaMicSynchronised();
        break;

#endif
#ifdef INCLUDE_MIRRORING
        case MESSAGE_SINK_AUDIO_SYNCHRONISED:
        case MESSAGE_SOURCE_AUDIO_SYNCHRONISED:
            appKymeraA2dpHandleAudioSyncStreamInd(id, msg);
        break;

        case KYMERA_INTERNAL_A2DP_DATA_SYNC_IND_TIMEOUT:
            appKymeraA2dpHandleDataSyncIndTimeout();
        break;

        case KYMERA_INTERNAL_A2DP_MESSAGE_MORE_DATA_TIMEOUT:
            appKymeraA2dpHandleMessageMoreDataTimeout();
        break;

        case KYMERA_INTERNAL_A2DP_AUDIO_SYNCHRONISED:
             appKymeraA2dpHandleAudioSynchronisedInd();
        break;

        case MESSAGE_MORE_DATA:
            appKymeraA2dpHandleMessageMoreData((const MessageMoreData *)msg);
        break;

        case MIRROR_PROFILE_A2DP_STREAM_ACTIVE_IND:
            Kymera_LatencyManagerHandleMirrorA2dpStreamActive();
        break;

        case MIRROR_PROFILE_A2DP_STREAM_INACTIVE_IND:
            Kymera_LatencyManagerHandleMirrorA2dpStreamInactive();
        break;

#endif /* INCLUDE_MIRRORING */

        case KYMERA_INTERNAL_SCO_AUDIO_SYNCHRONISED:
            appKymeraHandleInternalScoAudioSynchronised();
        break;

        case KYMERA_INTERNAL_LATENCY_MANAGER_MUTE:
            Kymera_LatencyManagerHandleMute();
        break;

        case KYMERA_INTERNAL_LATENCY_CHECK_TIMEOUT:
            Kymera_DynamicLatencyHandleLatencyTimeout();
		break;

        case KYMERA_INTERNAL_LATENCY_RECONFIGURE:
            Kymera_LatencyManagerHandleLatencyReconfigure((const KYMERA_INTERNAL_LATENCY_RECONFIGURE_T *)msg);
        break;

        case KYMERA_INTERNAL_LATENCY_MANAGER_MUTE_COMPLETE:
            Kymera_LatencyManagerHandleMuteComplete();
        break;

        case KYMERA_INTERNAL_AEC_LEAKTHROUGH_CREATE_STANDALONE_CHAIN:
            OperatorsFrameworkEnable();
            /* fall through */
        case KYMERA_INTERNAL_MIC_CONNECTION_TIMEOUT_LEAKTHROUGH:
            Kymera_CreateLeakthroughChain();
        break;
        case KYMERA_INTERNAL_AEC_LEAKTHROUGH_DESTROY_STANDALONE_CHAIN:
        {
            Kymera_DestroyLeakthroughChain();
            MessageSendLater(KymeraGetTask(), KYMERA_INTERNAL_PROSPECTIVE_POWER_OFF, NULL,
                             appConfigProspectiveAudioOffTimeout());
        }
        break;
        case KYMERA_INTERNAL_AEC_LEAKTHROUGH_SIDETONE_ENABLE:
        {
            Kymera_LeakthroughSetupSTGain();
        }
        break;
        case KYMERA_INTERNAL_AEC_LEAKTHROUGH_SIDETONE_GAIN_RAMPUP:
        {
            Kymera_LeakthroughStepupSTGain();
        }
        break;
        case KYMERA_INTERNAL_WIRED_ANALOG_AUDIO_START:
        {
            const KYMERA_INTERNAL_WIRED_ANALOG_AUDIO_START_T *m = (const KYMERA_INTERNAL_WIRED_ANALOG_AUDIO_START_T *)msg;
            /* Call the function in Kymera_Wired_Analog_Audio to start the audio chain */
            KymeraWiredAnalog_StartPlayingAudio(m);
        }
        break;
        case KYMERA_INTERNAL_WIRED_ANALOG_AUDIO_STOP:
        {
            /*Call the function in Kymera_Wired_Analog_Audio to stop the audio chain */
            KymeraWiredAnalog_StopPlayingAudio();
        }
        break;
        case KYMERA_INTERNAL_WIRED_AUDIO_SET_VOL:
        {
            KYMERA_INTERNAL_WIRED_AUDIO_SET_VOL_T *m = (KYMERA_INTERNAL_WIRED_AUDIO_SET_VOL_T *)msg;
            KymeraWiredAnalog_SetVolume(m->volume_in_db);
        }
        break;
        case KYMERA_INTERNAL_USB_AUDIO_START:
        {
            KymeraUsbAudio_Start((KYMERA_INTERNAL_USB_AUDIO_START_T *)msg);
        }
        break;
        case KYMERA_INTERNAL_USB_AUDIO_STOP:
        {
            KymeraUsbAudio_Stop((KYMERA_INTERNAL_USB_AUDIO_STOP_T *)msg);
        }
        break;
        case KYMERA_INTERNAL_USB_AUDIO_SET_VOL:
        {
            KYMERA_INTERNAL_USB_AUDIO_SET_VOL_T *m = (KYMERA_INTERNAL_USB_AUDIO_SET_VOL_T *)msg;
            KymeraUsbAudio_SetVolume(m->volume_in_db);
        }
        break;
        case KYMERA_INTERNAL_USB_VOICE_START:
        {
            KymeraUsbVoice_Start((KYMERA_INTERNAL_USB_VOICE_START_T *)msg);
        }
        break;
        case KYMERA_INTERNAL_USB_VOICE_STOP:
        {
            KymeraUsbVoice_Stop((KYMERA_INTERNAL_USB_VOICE_STOP_T *)msg);
        }
        break;
        case KYMERA_INTERNAL_USB_VOICE_SET_VOL:
        {
            KYMERA_INTERNAL_USB_VOICE_SET_VOL_T *m = (KYMERA_INTERNAL_USB_VOICE_SET_VOL_T *)msg;
            KymeraUsbVoice_SetVolume(m->volume_in_db);
        }
        break;
        case KYMERA_INTERNAL_USB_VOICE_MIC_MUTE:
        {
            KYMERA_INTERNAL_USB_VOICE_MIC_MUTE_T *m = (KYMERA_INTERNAL_USB_VOICE_MIC_MUTE_T *)msg;
            KymeraUsbVoice_MicMute(m->mute);
        }
        break;
        case KYMERA_INTERNAL_LOW_LATENCY_STREAM_CHECK:
        {
            Kymera_LatencyManagerHandleLLStreamCheck();
        }
        break;
#if defined(INCLUDE_MUSIC_PROCESSING)
        case KYMERA_INTERNAL_USER_EQ_SELECT_EQ_BANK:
        {
            DEBUG_LOG("KYMERA_INTERNAL_USER_EQ_SELECT_EQ_BANK");
            KYMERA_INTERNAL_USER_EQ_SELECT_EQ_BANK_T *m = (KYMERA_INTERNAL_USER_EQ_SELECT_EQ_BANK_T *)msg;
            Kymera_SelectEqBankNow(m->preset);
        }
        break;
        case KYMERA_INTERNAL_USER_EQ_SET_USER_GAINS:
        {
            DEBUG_LOG("KYMERA_INTERNAL_USER_EQ_SET_USER_GAINS");
            KYMERA_INTERNAL_USER_EQ_SET_USER_GAINS_T *m = (KYMERA_INTERNAL_USER_EQ_SET_USER_GAINS_T *)msg;
            Kymera_SetUserEqBandsNow(m->start_band, m->end_band, m->gain);
            free(m->gain);

            KYMERA_NOTIFCATION_USER_EQ_BANDS_UPDATED_T *message = PanicUnlessMalloc(sizeof(KYMERA_NOTIFCATION_USER_EQ_BANDS_UPDATED_T));
            message->start_band = m->start_band;
            message->end_band = m->end_band;
            TaskList_MessageSendWithSize(theKymera->listeners, KYMERA_NOTIFCATION_USER_EQ_BANDS_UPDATED, message, sizeof(KYMERA_NOTIFCATION_USER_EQ_BANDS_UPDATED_T));
        }
        break;
        case KYMERA_INTERNAL_USER_EQ_APPLY_GAINS:
        {
            if(KymeraGetTaskData()->eq.selected_eq_bank == EQ_BANK_USER)
            {
                Kymera_ApplyGains(0, KymeraGetTaskData()->eq.user.number_of_bands - 1);
            }
        }
        break;
#endif /* INCLUDE_MUSIC_PROCESSING */

#if defined(INCLUDE_CVC_DEMO)
        case KYMERA_INTERNAL_CVC_3MIC_POLL_MODE_OF_OPERATION:
            Kymera_ScoPollCvcSend3MicModeOfOperation();
        break;
#endif

        /* USB Audio mute status update */
        case KYMERA_INTERNAL_USB_AUDIO_MUTE:
        {
            KYMERA_INTERNAL_USB_AUDIO_MUTE_T *m = (KYMERA_INTERNAL_USB_AUDIO_MUTE_T *)msg;
            switch (appKymeraGetState())
            {
                case KYMERA_STATE_USB_AUDIO_ACTIVE:
                    KymeraUsbAudio_Mute(m->mute);
                break;

                default: break;
            }
        }
        break;

#if defined(INCLUDE_LE_AUDIO_USB_SOURCE)
        case KYMERA_INTERNAL_QHS_LEVEL_CHANGED:
        {
            KYMERA_INTERNAL_QHS_LEVEL_CHANGED_T *m = (KYMERA_INTERNAL_QHS_LEVEL_CHANGED_T *)msg;

            switch (appKymeraGetState())
            {
                case KYMERA_STATE_USB_LE_AUDIO_ACTIVE:
                    Kymera_UsbLeAudioApplyQhsRate(m->qhs_level);
                break;

                case KYMERA_STATE_WIRED_LE_AUDIO_ACTIVE:
                    //Kymera_UsbLeAudioApplyQhsRate(m->qhs_level);
                break;

                default:
                    break;
            }
        }
        break;
#endif

#if defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER)
        /* LEA Media Broadast request message(enable/disable) */
        case KYMERA_INTERNAL_LEA_MEDIA_BROADCAST_REQ:
        {
            KYMERA_INTERNAL_LEA_MEDIA_BROADCAST_REQ_T *m = (KYMERA_INTERNAL_LEA_MEDIA_BROADCAST_REQ_T *)msg;
            /* Set LEA broadcasting status and enable/disable second outputs of the splitter which provide inputs to the
             * To-Air ISO chain accordingly by invoking the callback of the corresponding audio source if available */
            KymeraBroadcastConcurrency_SetLeAudioBroadcastingStatus(m->broadcast_enable);
            if (IS_LEA_MEDIA_SENDER_AUDIO_SOURCE_VALID(m->audio_source))
            {
                if (IS_LEA_MEDIA_SENDER_AUDIO_SOURCE_AVAILABLE(m->audio_source) && LeaMediaBroadcastRequestHandler[m->audio_source])
                {
                    /* Invoke Lea broadcast request handler */
                    LeaMediaBroadcastRequestHandler[m->audio_source](m->broadcast_enable);
                }
            }
        }
        break;
#endif /* (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && (ENABLE_SIMPLE_SPEAKER) */

        default:
            break;
    }
}

bool appKymeraInit(Task init_task)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    memset(theKymera, 0, sizeof(*theKymera));
    theKymera->task.handler = kymera_msg_handler;
    theKymera->state = KYMERA_STATE_IDLE;
    theKymera->output_rate = 0;
    theKymera->lock = theKymera->busy_lock = 0;
    theKymera->a2dp_seid = AV_SEID_INVALID;
    theKymera->tone_count = 0;
    theKymera->split_tx_mode = 0;
    theKymera->q2q_mode = 0;
    theKymera->enable_left_right_mix = TRUE;
    theKymera->aptx_adaptive_ttp_ssrc = NULL;
    theKymera->listeners = TaskList_Create();
    appKymeraExternalAmpSetup();
#if defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER)
    theKymera->is_audio_broadcasting = FALSE;
#endif

    if (bundle_config && bundle_config->number_of_capability_bundles > 0)
    {
        DEBUG_LOG("appKymeraInit number of bundles %d", bundle_config->number_of_capability_bundles);
        ChainSetDownloadableCapabilityBundleConfig(bundle_config);
    }
    else
    {
        DEBUG_LOG("appKymeraInit bundle config not valid");
    }

    VmalRegisterAudioFrameworkObserver(&vmal_registration);

    Microphones_Init();

#ifdef INCLUDE_ANC_PASSTHROUGH_SUPPORT_CHAIN
    theKymera->anc_passthough_operator = INVALID_OPERATOR;
#endif

#ifdef ENABLE_AEC_LEAKTHROUGH
    Kymera_LeakthroughInit();
#endif

#ifdef INCLUDE_ULTRA_QUIET_DAC_MODE
    Kymera_UltraQuietDacInit();
#endif
    theKymera->client_tasks = TaskList_Create();

    Kymera_ScoInit();
    KymeraUsbVoice_Init();
    KymeraAnc_Init();
    KymeraAncCommon_Init();
#ifdef INCLUDE_VOICE_UI
    Kymera_VaInit();
#endif
#if defined(INCLUDE_MIRRORING) || defined(INCLUDE_STEREO)
    Kymera_A2dpInit();
#endif
#if defined(INCLUDE_A2DP_USB_SOURCE) || defined(INCLUDE_A2DP_ANALOG_SOURCE)
    kymeraA2dpSource_Init();
#endif
    appKymeraTonePromptInit();
    KymeraWiredAnalog_Init();
    appKymeraLoopbackInit();

#if !defined(INCLUDE_A2DP_USB_SOURCE)
    KymeraUsbAudio_Init();
#endif

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
    kymeraLeAudio_Init();
#endif

#ifdef INCLUDE_LE_AUDIO_UNICAST
    KymeraLeVoice_Init();
#endif

    Kymera_LatencyManagerInit(FALSE, 0);
    Kymera_DynamicLatencyInit();
    MirrorProfile_ClientRegister(&theKymera->task);


    Kymera_InitMusicProcessing();

#if defined(ENABLE_EARBUD_FIT_TEST) || defined(ENABLE_CONTINUOUS_EARBUD_FIT_TEST)
    KymeraFitTest_Init();
#endif

    KymeraStatistics_Init();

    KymeraOutput_Init();

    UNUSED(init_task);
    return TRUE;
}

bool Kymera_IsIdle(void)
{
    return !appKymeraIsBusy();
}

void Kymera_ClientRegister(Task client_task)
{
    DEBUG_LOG("Kymera_ClientRegister");
    kymeraTaskData *kymera_sm = KymeraGetTaskData();
    TaskList_AddTask(kymera_sm->client_tasks, client_task);
}

void Kymera_ClientUnregister(Task client_task)
{
    DEBUG_LOG("Kymera_ClientRegister");
    kymeraTaskData *kymera_sm = KymeraGetTaskData();
    TaskList_RemoveTask(kymera_sm->client_tasks, client_task);
}


void Kymera_SetBundleConfig(const capability_bundle_config_t *config)
{
    bundle_config = config;
}

void Kymera_SetChainConfigs(const kymera_chain_configs_t *configs)
{
    chain_configs = configs;
}

const kymera_chain_configs_t *Kymera_GetChainConfigs(void)
{
    PanicNull((void *)chain_configs);
    return chain_configs;
}

void Kymera_SetScoChainTable(const appKymeraScoChainInfo *info)
{
    appKymeraScoChainTable = info;
}

void Kymera_RegisterNotificationListener(Task task)
{
    kymeraTaskData * theKymera = KymeraGetTaskData();
    TaskList_AddTask(theKymera->listeners, task);
}

void Kymera_StartWiredAnalogAudio(int16 volume_in_db, uint32 rate, uint32 min_latency, uint32 max_latency, uint32 target_latency)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_WIRED_ANALOG_AUDIO_START);
    DEBUG_LOG("Kymera_StartWiredAnalogAudio");

    message->rate      = rate;
    message->volume_in_db     = volume_in_db;
    message->min_latency = min_latency;
    message->max_latency = max_latency;
    message->target_latency = target_latency;
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_WIRED_ANALOG_AUDIO_START, message, &theKymera->lock);
}

void Kymera_StopWiredAnalogAudio(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("Kymera_StopWiredAnalogAudio");
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_WIRED_ANALOG_AUDIO_STOP, NULL, &theKymera->lock);
}

void appKymeraWiredAudioSetVolume(int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraWiredAudioSetVolume, volume %d", volume_in_db);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_WIRED_AUDIO_SET_VOL);
    message->volume_in_db = volume_in_db;
    MessageCancelFirst(&theKymera->task, KYMERA_INTERNAL_WIRED_AUDIO_SET_VOL);
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_WIRED_AUDIO_SET_VOL, message, &theKymera->lock);
}

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
void Kymera_LeAudioStart(bool media_present, bool microphone_present, bool reconfig,
                        int16 volume_in_db, const le_media_config_t *media, 
                        const le_microphone_config_t *microphone)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("Kymera_LeAudioStart, lock %u, busy_lock %u, Reconfig %d", theKymera->lock, theKymera->busy_lock, reconfig);

    if (reconfig)
    {
        /* Reconfiguration of LE audio graph needed, hence stop current graph and reload with new params */
        Kymera_LeAudioStop();
    }

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_LE_AUDIO_START);
    message->media_present = media_present;
    message->microphone_present = microphone_present;
    message->reconfig = reconfig;
    message->volume_in_db = volume_in_db;
    message->media_params = *media;
    message->microphone_params = *microphone;
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_START,
                             message,
                             &theKymera->lock);
}

void Kymera_LeAudioStop(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("Kymera_LeAudioStop");

    /* Cancel any pending KYMERA_INTERNAL_LE_AUDIO_UNMUTE messages
       because LE audio may have been stopped before the synchronised start
       has finished. */
    MessageCancelAll(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_UNMUTE);

    if (MessagePendingFirst(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_START, NULL))
    {
        MessageCancelFirst(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_START);
    }
    else
    {
        MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_STOP, NULL, &theKymera->lock);
    }
}


void Kymera_LeAudioSetVolume(int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("Kymera_LeAudioSetVolume msg, vol %d", volume_in_db);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_LE_AUDIO_SET_VOLUME);
    message->volume_in_db = volume_in_db;
    MessageCancelFirst(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_SET_VOLUME);
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_SET_VOLUME, message, &theKymera->lock);
}

void Kymera_LeAudioUnmute(rtime_t unmute_time)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_LE_AUDIO_UNMUTE);
    message->unmute_time = unmute_time;
    MessageCancelAll(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_UNMUTE);
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_LE_AUDIO_UNMUTE, message, &theKymera->lock);

    DEBUG_LOG("Kymera_LeAudioUnmute in %dms", RtimeTimeToMsDelay(message->unmute_time));
}
#endif

#if defined(INCLUDE_LE_AUDIO_UNICAST)
void Kymera_LeVoiceStart(bool speaker_present, bool microphone_present, bool reconfig,
                        int16 volume_in_db, const le_speaker_config_t *speaker,
                        const le_microphone_config_t *microphone)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("Kymera_LeVoiceStart, lock %u, busy_lock %u, Reconfig %d", theKymera->lock, theKymera->busy_lock, reconfig);

    if (reconfig)
    {
        /* Reconfiguration of LE audio graph needed, hence stop current graph and reload with new params */
        Kymera_StopLeMicChain();
        Kymera_LeVoiceStop();
    }

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_LE_VOICE_START);
    message->speaker_present = speaker_present;
    message->microphone_present = microphone_present;
    message->reconfig = reconfig;
    message->volume_in_db = volume_in_db;
    message->speaker_params = *speaker;
    message->microphone_params = *microphone;
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_LE_VOICE_START,
                             message,
                             &theKymera->lock);
}

void Kymera_LeVoiceStop(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("Kymera_LeVoiceStop");

    if (MessagePendingFirst(&theKymera->task, KYMERA_INTERNAL_LE_VOICE_START, NULL))
    {
        /* we should not be in LE voice routing state */
        PanicFalse(appKymeraGetState() != KYMERA_STATE_LE_VOICE_ACTIVE);

        MessageCancelFirst(&theKymera->task, KYMERA_INTERNAL_LE_VOICE_START);
    }
    else
    {
        MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_LE_VOICE_STOP, NULL, &theKymera->lock);
    }
}

void Kymera_LeVoiceSetVolume(int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("Kymera_LeVoiceSetVolume msg, vol %d", volume_in_db);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_LE_VOICE_SET_VOLUME);
    message->volume_in_db = volume_in_db;
    MessageCancelFirst(&theKymera->task, KYMERA_INTERNAL_LE_VOICE_SET_VOLUME);
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_LE_VOICE_SET_VOLUME, message, &theKymera->lock);
}

void Kymera_LeVoiceMicMute(bool mute)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    DEBUG_LOG("Kymera_LeVoiceMicMute msg, mute %u", mute);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_LE_VOICE_MIC_MUTE);
    message->mute = mute;
    MessageSend(&theKymera->task, KYMERA_INTERNAL_LE_VOICE_MIC_MUTE, message);
}

void Kymera_ScheduleLeaMicSyncUnmute(Delay delay)
{
    DEBUG_LOG("Kymera_ScheduleLeaMicSyncUnmute, unmute in %dms", delay);
    MessageCancelFirst(KymeraGetTask(), KYMERA_INTERNAL_LEA_MIC_SYNCHRONISED);
    MessageSendLater(KymeraGetTask(), KYMERA_INTERNAL_LEA_MIC_SYNCHRONISED,
                     NULL, delay);
}

static void appKymeraHandleInternalLeaMicSynchronised(void)
{
    if (appKymeraGetState() == KYMERA_STATE_LE_AUDIO_ACTIVE)
    {
        DEBUG_LOG("appKymeraHandleInternalLeaMicSynchronised");
        KymeraLeAudioVoice_SetMicMuteState(FALSE);
    }
}

#endif

void appKymeraUsbAudioStart(uint8 channels, uint8 frame_size, Source src,
                            int16 volume_in_db, bool mute_status, uint32 rate,
                            uint32 min_latency, uint32 max_latency, uint32 target_latency)
{
    DEBUG_LOG("appKymeraUsbAudioStart");
    kymeraTaskData *theKymera = KymeraGetTaskData();

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_USB_AUDIO_START);
    message->channels = channels;
    message->frame_size = frame_size;
    message->sample_freq = rate;
    message->spkr_src = src;
    message->volume_in_db = volume_in_db;
    message->mute_status = mute_status;
    message->min_latency_ms = min_latency;
    message->max_latency_ms = max_latency;
    message->target_latency_ms = target_latency;

    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_USB_AUDIO_START, message, &theKymera->lock);
}

void appKymeraUsbAudioStop(Source usb_src,
                           void (*kymera_stopped_handler)(Source source))
{
    DEBUG_LOG("appKymeraUsbAudioStop");
    kymeraTaskData *theKymera = KymeraGetTaskData();

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_USB_AUDIO_STOP);
    message->source = usb_src;
    message->kymera_stopped_handler = kymera_stopped_handler;

    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_USB_AUDIO_STOP, message, &theKymera->lock);
}

void appKymeraUsbAudioSetVolume(int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG("appKymeraUsbAudioSetVolume, volume %d", volume_in_db);

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_USB_AUDIO_SET_VOL);
    message->volume_in_db = volume_in_db;
    MessageCancelFirst(&theKymera->task, KYMERA_INTERNAL_USB_AUDIO_SET_VOL);
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_USB_AUDIO_SET_VOL, message, &theKymera->lock);
}

void appKymeraUsbVoiceStart(usb_voice_mode_t mode, uint8 spkr_channels, uint8 spkr_frame_size,
                            uint32 spkr_sample_rate, uint32 mic_sample_rate, Source spkr_src,
                            Sink mic_sink, int16 volume_in_db, uint32 min_latency, uint32 max_latency,
                            uint32 target_latency, void (*kymera_stopped_handler)(Source source))
{
    DEBUG_LOG("appKymeraUsbVoiceStart");
    kymeraTaskData *theKymera = KymeraGetTaskData();

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_USB_VOICE_START);
    message->mode = mode;
    message->spkr_channels = spkr_channels;
    message->spkr_frame_size = spkr_frame_size;
    message->spkr_sample_rate = spkr_sample_rate;
    message->mic_sample_rate = mic_sample_rate;
    message->spkr_src = spkr_src;
    message->mic_sink = mic_sink;
    message->volume = volume_in_db;
    message->min_latency_ms = min_latency;
    message->max_latency_ms = max_latency;
    message->target_latency_ms = target_latency;
    message->kymera_stopped_handler = kymera_stopped_handler;

    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_USB_VOICE_START, message, &theKymera->lock);
}

void appKymeraUsbVoiceStop(Source spkr_src, Sink mic_sink,
                           void (*kymera_stopped_handler)(Source source))
{
    DEBUG_LOG("appKymeraUsbVoiceStop");
    kymeraTaskData *theKymera = KymeraGetTaskData();

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_USB_VOICE_STOP);
    message->spkr_src = spkr_src;
    message->mic_sink = mic_sink;
    message->kymera_stopped_handler = kymera_stopped_handler;

    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_USB_VOICE_STOP, message, &theKymera->lock);
}

void appKymeraUsbVoiceSetVolume(int16 volume_in_db)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_USB_VOICE_SET_VOL);
    message->volume_in_db = volume_in_db;
    MessageCancelFirst(&theKymera->task, KYMERA_INTERNAL_USB_VOICE_SET_VOL);
    MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_USB_VOICE_SET_VOL, message, &theKymera->lock);
}

void appKymeraUsbVoiceMicMute(bool mute)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_USB_VOICE_MIC_MUTE);
    message->mute = mute;
    MessageSend(&theKymera->task, KYMERA_INTERNAL_USB_VOICE_MIC_MUTE, message);
}

Transform Kymera_GetA2dpMediaStreamTransform(void)
{
    Transform trans = (Transform)0;

#ifdef INCLUDE_MIRRORING
    kymeraTaskData *theKymera = KymeraGetTaskData();

    if (Kymera_A2dpIsStreaming())
    {
        trans = theKymera->hashu.hash;
    }
#endif

    return trans;
}

void Kymera_EnableAdaptiveAnc(bool in_ear, audio_anc_path_id path, adaptive_anc_hw_channel_t hw_channel, anc_mode_t mode)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_AANC_ENABLE);

    message->in_ear         = in_ear;
    message->control_path   = path;
    message->hw_channel     = hw_channel;
    message->current_mode   = mode;
    MessageSend(&theKymera->task, KYMERA_INTERNAL_AANC_ENABLE, message);
}

bool Kymera_AdaptiveAncIsNoiseLevelBelowQuietModeThreshold(void)
{
    return KymeraAncCommon_AdaptiveAncIsNoiseLevelBelowQmThreshold();
}

void Kymera_SetA2dpOutputParams(a2dp_codec_settings * codec_settings)
{
    kymeraTaskData *the_kymera = KymeraGetTaskData();
    the_kymera->a2dp_output_params = codec_settings;
}

void Kymera_ClearA2dpOutputParams(void)
{
    kymeraTaskData *the_kymera = KymeraGetTaskData();
    the_kymera->a2dp_output_params = NULL;
}

bool Kymera_IsA2dpOutputPresent(void)
{
    kymeraTaskData *the_kymera = KymeraGetTaskData();
    return the_kymera->a2dp_output_params ? TRUE : FALSE;
}


bool Kymera_IsA2dpSynchronisationNotInProgress(void)
{
#ifdef INCLUDE_MIRRORING
    kymeraTaskData *theKymera = KymeraGetTaskData();

    if (Kymera_A2dpIsStreaming() && (theKymera->sync_info.state != KYMERA_AUDIO_SYNC_STATE_COMPLETE))
    {
        DEBUG_LOG("Kymera_IsA2dpSynchronisationNotInProgress: audio sync incomplete");
        return FALSE;
    }
#endif /* INCLUDE_MIRRORING */
    return TRUE;
}

void Kymera_SetCallbackConfigs(const kymera_callback_configs_t *configs)
{
    DEBUG_LOG("Kymera_SetCallbackConfigs");
    callback_configs = configs;
}

const kymera_callback_configs_t *Kymera_GetCallbackConfigs(void)
{
    return callback_configs;
}

bool Kymera_IsQ2qModeEnabled(void)
{
    return KymeraGetTaskData()->q2q_mode;
}

bool appKymeraIsTonePlaying(void)
{
    return (KymeraGetTaskData()->tone_count > 0);
}

void Kymera_RegisterConfigCallbacks(kymera_chain_config_callbacks_t *callbacks)
{
    KymeraGetTaskData()->chain_config_callbacks = callbacks;
}

void Kymera_ScheduleScoSyncUnmute(Delay delay)
{
    DEBUG_LOG("Kymera_ScheduleScoSyncUnmute, unmute in %dms", delay);
    MessageCancelFirst(KymeraGetTask(), KYMERA_INTERNAL_SCO_AUDIO_SYNCHRONISED);
    MessageSendLater(KymeraGetTask(), KYMERA_INTERNAL_SCO_AUDIO_SYNCHRONISED,
                     NULL, delay);
}

static void kymera_SetCvcSendPassthroughInChain(kymera_chain_handle_t chain_handle, kymera_cvc_mode_t mode, uint8 passthrough_mic)
{
    if (chain_handle != NULL)
    {
        Operator cvc_snd_op;

        DEBUG_LOG("kymera_SetCvcSendPassthroughInChain: state enum:appKymeraState:%d mode enum:kymera_cvc_mode_t:%d passthrough mic %d",
                   appKymeraGetState(), mode, passthrough_mic);

        if (mode & KYMERA_CVC_SEND_PASSTHROUGH)
        {
            PanicFalse(GET_OP_FROM_CHAIN(cvc_snd_op, chain_handle, OPR_CVC_SEND));
            OperatorsStandardSetControl(cvc_snd_op, OPMSG_CONTROL_MODE_ID,
                                        CONTROL_MODE_CVC_SND_PASSTHROUGH_MIC1 + passthrough_mic);
        }
        else if (mode & KYMERA_CVC_SEND_FULL_PROCESSING)
        {
            PanicFalse(GET_OP_FROM_CHAIN(cvc_snd_op, chain_handle, OPR_CVC_SEND));
            OperatorsStandardSetControl(cvc_snd_op, OPMSG_CONTROL_MODE_ID, CONTROL_MODE_FULL_PROCESSING);
        }
    }
}

static void kymera_SetCvcRcvPassthroughInChain(kymera_chain_handle_t chain_handle, kymera_cvc_mode_t mode, uint8 passthrough_mic)
{
    if (chain_handle != NULL)
    {
        Operator cvc_rcv_op;

        DEBUG_LOG("kymera_SetCvcRcvPassthroughInChain: state enum:appKymeraState:%d mode enum:kymera_cvc_mode_t:%d passthrough mic %d",
                   appKymeraGetState(), mode, passthrough_mic);

        if (mode & KYMERA_CVC_RECEIVE_PASSTHROUGH)
        {
            PanicFalse(GET_OP_FROM_CHAIN(cvc_rcv_op, chain_handle, OPR_CVC_RECEIVE));
            OperatorsStandardSetControl(cvc_rcv_op, OPMSG_CONTROL_MODE_ID,
                                        CONTROL_MODE_CVC_RCV_PASSTHROUGH_MIC1 + passthrough_mic);
        }
        else if (mode & KYMERA_CVC_RECEIVE_FULL_PROCESSING)
        {
            PanicFalse(GET_OP_FROM_CHAIN(cvc_rcv_op, chain_handle, OPR_CVC_RECEIVE));
            OperatorsStandardSetControl(cvc_rcv_op, OPMSG_CONTROL_MODE_ID, CONTROL_MODE_FULL_PROCESSING);
        }
    }
}

bool Kymera_SetCvcPassthroughMode(kymera_cvc_mode_t mode, uint8 passthrough_mic)
{
    bool setting_changed = FALSE;
    kymera_chain_handle_t send_chain_handle = NULL;
    kymera_chain_handle_t recv_chain_handle = NULL;

    DEBUG_LOG("Kymera_SetCvcPassthroughMode: mode enum:kymera_cvc_mode_t:%d passthrough_mic %d", mode, passthrough_mic);

#ifdef INCLUDE_CVC_DEMO
    setting_changed = Kymera_UpdateScoCvcSendSetting(mode, passthrough_mic);
#endif

    if (mode == (KYMERA_CVC_RECEIVE_PASSTHROUGH | KYMERA_CVC_SEND_PASSTHROUGH))
    {
        KymeraGetTaskData()->enable_cvc_passthrough = 1;
    }

    switch(appKymeraGetState())
    {
#ifdef INCLUDE_USB_DEVICE
        case KYMERA_STATE_USB_VOICE_ACTIVE:
            send_chain_handle = recv_chain_handle = KymeraUsbVoice_GetCvcChain();
            break;
#endif
        case KYMERA_STATE_SCO_ACTIVE:
            send_chain_handle = recv_chain_handle = Kymera_ScoGetCvcChain();
            break;

#ifdef INCLUDE_LE_AUDIO_UNICAST
        case KYMERA_STATE_LE_AUDIO_ACTIVE:
            send_chain_handle = Kymera_LeAudioGetCvcChain();
            break;

        case KYMERA_STATE_LE_VOICE_ACTIVE:
            send_chain_handle = recv_chain_handle = Kymera_LeVoiceGetCvcChain();
            break;
#endif

        default:
            break;
    }

    kymera_SetCvcSendPassthroughInChain(send_chain_handle, mode, passthrough_mic);
    kymera_SetCvcRcvPassthroughInChain(recv_chain_handle, mode, passthrough_mic);

    return setting_changed;
}

void appKymeraUsbAudioMute(bool mute)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();

    MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_USB_AUDIO_MUTE);
    message->mute = mute;
    MessageSend(&theKymera->task, KYMERA_INTERNAL_USB_AUDIO_MUTE, message);
}

#ifdef ENABLE_TWM_SPEAKER
void Kymera_SetAudioType(appKymeraAudioType audio_type, bool is_toggle_party_mode)
{

    kymeraTaskData *theKymera = KymeraGetTaskData();
    PanicNull(theKymera);
    DEBUG_LOG("Kymera_SetAudioType: current audio type enum:appKymeraAudioType:%d and requested enum:appKymeraAudioType:%d and IsTogglePartyMode %d", theKymera->audio_type, audio_type, is_toggle_party_mode);
    if(theKymera->audio_type != audio_type)
    {
        Kymera_A2dpHandleSetAudioType(audio_type, is_toggle_party_mode);
    }
}
#endif

#if defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER)

static appKymeraLeAudioMediaSenderSourceType appKymeraGetLeaMediaSenderAudioSource(audio_source_t audio_source_routed)
{
    appKymeraLeAudioMediaSenderSourceType media_sender_source_type;
    DEBUG_LOG("appKymeraGetLeaMediaSenderAudioSource, Routed audio source is enum:audio_source_t:%d", audio_source_routed);
    switch(audio_source_routed)
    {
        case audio_source_a2dp_1:
        case audio_source_a2dp_2:
        {
            media_sender_source_type = KYMERA_AUDIO_SOURCE_A2DP;
        }
        break;

        case audio_source_le_audio_unicast_1:
        case audio_source_le_audio_unicast_2:
        {
            media_sender_source_type = KYMERA_AUDIO_SOURCE_LE_AUDIO_UNICAST;
        }
        break;

        default:
        {
            DEBUG_LOG("Other audio sources are not supported currently and if routed the broadcast request to be ignored");
            media_sender_source_type = KYMERA_AUDIO_SOURCE_NONE;
        }
        break;
    }
    return media_sender_source_type;
}

void Kymera_RegisterLeaMediaBroadcastRequestCallback(appKymeraLeAudioMediaSenderSourceType audio_source, Kymera_LeaMediaBroadcastRequestCallback callback)
{
    DEBUG_LOG("Kymera_RegisterLeaMediaBroadcastRequestCallback");
    if (IS_LEA_MEDIA_SENDER_AUDIO_SOURCE_VALID(audio_source) && IS_LEA_MEDIA_SENDER_AUDIO_SOURCE_AVAILABLE(audio_source))
    {
        LeaMediaBroadcastRequestHandler[audio_source] = callback;
    }
}

void Kymera_EnableLeaAudioBroadcasting(bool enable)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    DEBUG_LOG_VERBOSE("Kymera_EnableLeaAudioBroadcasting, enable = ", enable);    

    /* Enable broadcasting only if it was not broadcasting previously and
     * Disable only if it was broadcasting previously */
    if (Kymera_IsAudioBroadcasting() != enable)
    {
        /* Send kymera internal message to handle broadcast request accordingly */
        MAKE_KYMERA_MESSAGE(KYMERA_INTERNAL_LEA_MEDIA_BROADCAST_REQ);
        message->broadcast_enable = enable;
        message->audio_source = appKymeraGetLeaMediaSenderAudioSource(AudioSources_GetRoutedSource());
        MessageSendConditionally(&theKymera->task, KYMERA_INTERNAL_LEA_MEDIA_BROADCAST_REQ, message, &theKymera->lock);
    }
}

void Kymera_SetLeaBroadcastParams(le_media_config_t * lea_broadcast_params)
{
    kymeraTaskData *the_kymera = KymeraGetTaskData();
    PanicNull(the_kymera);
    PanicNull(lea_broadcast_params);
    the_kymera->lea_broadcast_params = lea_broadcast_params;
}

void Kymera_ClearLeaBroadcastParams(void)
{
    kymeraTaskData *the_kymera = KymeraGetTaskData();
    PanicNull(the_kymera);
    the_kymera->lea_broadcast_params = NULL;
}

#endif /* (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER) */

