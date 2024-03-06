/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   ml_example ML Loopback Example Application
\brief      Machine learning loopback example application

This example application is based on the audio/ML chain:

MIC -> Example ML Capability -> SPK

To change ML model used modify model_file in the ml_config structure.
However, overall structure is constrained to described above, so new model needs to fit in it.

*/

#include <os.h>
#include "logging.h"
#include "ml_loopback_setup_audio.h"
#include "ml_loopback.h"

/* Configuration of the machine learning usecase */
static const ml_config_t ml_config = {
        /* usecase-id, should be same as what is mentioned in the model file */
        /* For the example model, basic_model.keai, it is 1 */
        .use_case_id = 1,
        /* Default capability ID of the ML template - Change to the actual ID */
        /* of the created template */
        .capability_id = 0xC800,
        /* Model access methods */
        .access_method = MODEL_COPY_AND_AUTO_UNLOAD,
        /* Default operator bundle name: Change this to the actual capability bundle */
        /* created from the template */
        .operator_bundle = "download_ml.dkcs",
        /* Download ML Engine Lib capability bundle - this is supplied as a prebuilt dkcs   */
        /* and needs to be downloaded before using any machine learning capability. This    */
        /* capability needs to be instantiated only once even if there are multiple machine */
        /* learning capabilities in the usecase */
        .dnld_ml_engine_lib_bundle = "download_ml_engine_lib.edkcs",
        /* Model file name */
        .model_file = "basic_model.keai",
        /* Batch Reset count: Resets the persistent tensors after this count if not zero */
        .batch_reset_count = 0
};

/*! \brief Application entry point

    This function is the entry point for the application.

    \returns Nothing. Only exits by powering down.
*/
int main(void)
{
    OsInit();

    DEBUG_LOG_ALWAYS("Starting ML loopback example");

    /* setup the input and output endpoints */
    audio_endpoint_t audio_endpoint = Ml_SetupAudio();

    /* create the kymera chain and start the chain */
    Ml_Start(audio_endpoint, &ml_config);

    DEBUG_LOG_ALWAYS("ML loopback example is running");

    /* Start the message scheduler loop */
    MessageLoop();

    /* We should never get here, keep compiler happy */
    return 0;
}
