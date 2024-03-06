/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    ml_example
\brief      Setup of the audio hardware

*/

#ifndef ML_LOOPBACK_SETUP_AUDIO_H
#define ML_LOOPBACK_SETUP_AUDIO_H

#include <source.h>
#include <sink.h>

typedef struct AUDIO_ENDPOINT
{
    Source source_ep;
    Sink sink_ep;
}audio_endpoint_t;

/*! \brief Non ML audio setup

    It configures the input ADC and output DAC.

    \return audio_endpoint_t setup

*/
audio_endpoint_t Ml_SetupAudio(void);

#endif // ML_LOOPBACK_SETUP_AUDIO_H
