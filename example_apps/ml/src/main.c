/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   ml_example_svad ML SVAD Example Application
\brief      Machine learning example application based on SVAD

This example application is based on the audio/ML chain:

MIC -> ML_EXAMPLE_SVAD

ML_EXAMPLE_SVAD capability gets audio samples from the microphone, runs the ML model and send messages to the
application task.

*/

#include "ml_audio_setup.h"
#include "ml.h"

#include <logging.h>
#include <message.h>
#include <os.h>

#define BCM_MIC_GAIN 20

/* Configuration of the machine learning example SVAD usecase */
static const ml_config_t ml_config = {
        .use_case_id = 1,
        .capability_id = 0x40C1,
        .access_method = model_copy_and_auto_unload,
        .model_file = "self_vad_ces.keai",
        .batch_reset_count = 0
};

/*! \brief Print ML chain output */
static void ml_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    MessageFromOperator *ml_msg = (MessageFromOperator *)message;

    DEBUG_LOG_ALWAYS("Message len %d", ml_msg->len);
    DEBUG_LOG_ALWAYS("MESSAGE_FROM_OPERATOR:[%d,%d,%d,%d,%d]",
            ml_msg->message[0], ml_msg->message[1], ml_msg->message[2], ml_msg->message[3],
            ml_msg->message[4]);
    uint16 vad_status = ml_msg->message[4];
    DEBUG_LOG_ALWAYS("event id: %d, vad status: %d", id, vad_status);
}

TaskData ml_taskdata = {.handler = ml_MessageHandler};

int main(void)
{
    OsInit();

    DEBUG_LOG_ALWAYS("Starting ML example");

    Source input = Ml_SetupAudio(BCM_MIC_GAIN);

    Ml_Start(input, (Task)&ml_taskdata, &ml_config);

    DEBUG_LOG_ALWAYS("ML example is running");

    /* Start the message scheduler loop */
    MessageLoop();

    /* We should never get here, keep compiler happy */
    return 0;
}
