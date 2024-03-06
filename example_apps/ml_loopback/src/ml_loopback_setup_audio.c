/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Setup of the audio hardware
*/

#include "ml_loopback_setup_audio.h"

#include <vmal.h>

#include <app/audio/audio_if.h>
#include <stream.h>
#include <panic.h>

/*! Sample rate to use for the DAC */
#define DAC_SAMPLE_RATE         16000
/*! Codec gain for input and output */
#define CODEC_GAIN              9

audio_endpoint_t Ml_SetupAudio(void)
{
    audio_endpoint_t audio_endpoint;

    VmalOperatorFrameworkEnableMainProcessor();

    /* Get the output endpoints */
    audio_endpoint.sink_ep  = PanicNull(StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));
    /* Get the input endpoints */
    audio_endpoint.source_ep = PanicNull(StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));

    /* Configure the sample rate and gain for the input endpoint */
    PanicFalse(SourceConfigure(audio_endpoint.source_ep,  STREAM_CODEC_INPUT_RATE, DAC_SAMPLE_RATE));
    PanicFalse(SourceConfigure(audio_endpoint.source_ep,  STREAM_CODEC_INPUT_GAIN, CODEC_GAIN));

    /* Configure the sample rate and gain for the output endpoint */
    PanicFalse(SinkConfigure(audio_endpoint.sink_ep,  STREAM_CODEC_OUTPUT_RATE, DAC_SAMPLE_RATE));
    PanicFalse(SinkConfigure(audio_endpoint.sink_ep,  STREAM_CODEC_OUTPUT_GAIN, CODEC_GAIN));

    return audio_endpoint;
}

