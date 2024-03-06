/*!
\copyright  Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle the messages generated by Voice Assistant related public APIs

*/

#ifdef INCLUDE_VOICE_UI
#include "kymera.h"
#include "kymera_va.h"
#include "kymera_va_common.h"
#include "kymera_va_wuw_chain.h"
#include "kymera_data.h"
#include <logging.h>

typedef enum
{
    KYMERA_INTERNAL_VOICE_CAPTURE_START,
    KYMERA_INTERNAL_VOICE_CAPTURE_STOP,
    KYMERA_INTERNAL_WAKE_UP_WORD_DETECTION_START,
    KYMERA_INTERNAL_WAKE_UP_WORD_DETECTION_STOP,
    KYMERA_INTERNAL_WAKE_UP_WORD_RESPONSE
} internal_message_ids;

typedef struct
{
    KymeraVoiceCaptureStarted callback;
    va_audio_voice_capture_params_t params;
} KYMERA_INTERNAL_VOICE_CAPTURE_START_T;

typedef struct
{
    KymeraWakeUpWordDetected callback;
    va_audio_wuw_detection_params_t params;
} KYMERA_INTERNAL_WAKE_UP_WORD_DETECTION_START_T;

typedef struct
{
    kymera_wuw_detected_response_t response;
} KYMERA_INTERNAL_WAKE_UP_WORD_RESPONSE_T;

typedef struct
{
    uint16 detection_confidence;
    uint16 start_ts_msb;
    uint16 start_ts_lsb;
    uint16 end_ts_msb;
    uint16 end_ts_lsb;
    uint16 metadata_length;
} graph_manager_wuw_detection_info_t;

typedef struct
{
    uint16 number_of_channels;
    graph_manager_wuw_detection_info_t channel[1];
} graph_manager_wuw_detection_message_t;

static void kymera_VaMessageHandler(Task task, MessageId id, Message msg);
static void kymera_WakeUpWordDetectionHandler(Task task, MessageId id, Message msg);
static const TaskData msg_handler = { kymera_VaMessageHandler };
static const TaskData wuw_detection_handler = { kymera_WakeUpWordDetectionHandler };

static KymeraWakeUpWordDetected wuw_detected_callback = NULL;

static void kymera_VaMessageHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);
    switch (id)
    {
        case KYMERA_INTERNAL_VOICE_CAPTURE_START:
        {
            const KYMERA_INTERNAL_VOICE_CAPTURE_START_T *req = msg;
            PanicFalse(Kymera_PrepareVaLiveCapture(&req->params));
            req->callback(PanicNull(Kymera_StartVaLiveCapture(&req->params)));
        }
        break;

        case KYMERA_INTERNAL_VOICE_CAPTURE_STOP:
            PanicFalse(Kymera_StopVaCapture());
        break;

        case KYMERA_INTERNAL_WAKE_UP_WORD_DETECTION_START:
        {
            const KYMERA_INTERNAL_WAKE_UP_WORD_DETECTION_START_T *req = msg;
            PanicFalse(Kymera_PrepareVaWuwDetection((Task) &wuw_detection_handler, &req->params));
            PanicFalse(Kymera_StartVaWuwDetection((Task) &wuw_detection_handler, &req->params));
            wuw_detected_callback = req->callback;
        }
        break;

        case KYMERA_INTERNAL_WAKE_UP_WORD_DETECTION_STOP:
            PanicFalse(Kymera_StopVaWuwDetection());
            wuw_detected_callback = NULL;
        break;

        case KYMERA_INTERNAL_WAKE_UP_WORD_RESPONSE:
        {
            const KYMERA_INTERNAL_WAKE_UP_WORD_RESPONSE_T *req = msg;

            if (req->response.start_capture)
            {
                PanicFalse(req->response.capture_callback != NULL);
                req->response.capture_callback(PanicNull(Kymera_StartVaWuwCapture(&req->response.capture_params)));
            }
            else
            {
                PanicFalse(Kymera_IgnoreDetectedVaWuw());
            }
        }
        break;
    }
}

static void kymera_LogWuwDetectedMsg(const MessageFromOperator *wuw_msg)
{
    switch(wuw_msg->message[0])
    {
        case 0xFF:
            DEBUG_LOG_INFO("kymera_LogWuwDetectedMsg: Detected in low power mode");
            break;
        case 0x00:
            DEBUG_LOG_INFO("kymera_LogWuwDetectedMsg: Detected in active mode");
            break;
        default:
            DEBUG_LOG_WARN("kymera_LogWuwDetectedMsg: Unknown mode");
            break;
    }
}

static va_audio_wuw_detection_info_t kymera_GetWuwDetectionInfo(const MessageFromOperator *op_msg)
{
    va_audio_wuw_detection_info_t info = {0};
    const graph_manager_wuw_detection_message_t *message = (const graph_manager_wuw_detection_message_t *) &op_msg->message[3];

    PanicFalse(message->number_of_channels == 1);
    info.detection_confidence = message->channel[0].detection_confidence;
    info.start_timestamp = UINT32_BUILD(message->channel[0].start_ts_msb, message->channel[0].start_ts_lsb);
    info.end_timestamp   = UINT32_BUILD(message->channel[0].end_ts_msb, message->channel[0].end_ts_lsb);
#ifdef INCLUDE_WUW_METADATA
    info.metadata.data_length = message->channel[0].metadata_length;
#else
    info.metadata.data_length = 0;
#endif
    return info;
}

static void kymera_WakeUpWordDetectionHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);
    UNUSED(id);

    kymera_LogWuwDetectedMsg(msg);

    if (Kymera_VaWuwDetected() == FALSE)
    {
        DEBUG_LOG_WARN("kymera_WakeUpWordDetectionHandler: WuW detected message was ignored");
        return;
    }

    if (MessagesPendingForTask((Task) &msg_handler, NULL))
    {
        DEBUG_LOG_WARN("kymera_WakeUpWordDetectionHandler: Ignore WuW detected due to other pending client requests");
        PanicFalse(Kymera_IgnoreDetectedVaWuw());
    }
    else
    {
        MESSAGE_MAKE(message, KYMERA_INTERNAL_WAKE_UP_WORD_RESPONSE_T);
        va_audio_wuw_detection_info_t wuw_info = kymera_GetWuwDetectionInfo(msg);
        kymeraTaskData *theKymera = KymeraGetTaskData();
        DEBUG_LOG("kymera_WakeUpWordDetectionHandler: Get client response, metadata size %d", wuw_info.metadata.data_length);
        if(wuw_info.metadata.data_length > 0)
        {
            wuw_info.metadata.data = Kymera_VaWuwChainGetMetadata(wuw_info.metadata.data_length);
        }

        PanicFalse(wuw_detected_callback != NULL);
        message->response = wuw_detected_callback(&wuw_info);
        MessageSendConditionally((Task) &msg_handler, KYMERA_INTERNAL_WAKE_UP_WORD_RESPONSE, message, &theKymera->lock);

        if(wuw_info.metadata.data)
        {
            free(wuw_info.metadata.data);
        }
    }
}

void Kymera_StartVoiceCapture(KymeraVoiceCaptureStarted callback, const va_audio_voice_capture_params_t *params)
{
    MESSAGE_MAKE(message, KYMERA_INTERNAL_VOICE_CAPTURE_START_T);
    kymeraTaskData *theKymera = KymeraGetTaskData();
    PanicFalse(callback && params);
    message->callback = callback;
    message->params = *params;
    MessageSendConditionally((Task) &msg_handler, KYMERA_INTERNAL_VOICE_CAPTURE_START, message, &theKymera->lock);
}

void Kymera_StopVoiceCapture(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    MessageSendConditionally((Task) &msg_handler, KYMERA_INTERNAL_VOICE_CAPTURE_STOP, NULL, &theKymera->lock);
}

void Kymera_StartWakeUpWordDetection(KymeraWakeUpWordDetected callback, const va_audio_wuw_detection_params_t *params)
{
    MESSAGE_MAKE(message, KYMERA_INTERNAL_WAKE_UP_WORD_DETECTION_START_T);
    kymeraTaskData *theKymera = KymeraGetTaskData();
    PanicFalse(callback && params);
    message->callback = callback;
    message->params = *params;
    MessageSendConditionally((Task) &msg_handler, KYMERA_INTERNAL_WAKE_UP_WORD_DETECTION_START, message, &theKymera->lock);
}

void Kymera_StopWakeUpWordDetection(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    MessageSendConditionally((Task) &msg_handler, KYMERA_INTERNAL_WAKE_UP_WORD_DETECTION_STOP, NULL, &theKymera->lock);
}

#endif /*#ifdef INCLUDE_VOICE_UI */
