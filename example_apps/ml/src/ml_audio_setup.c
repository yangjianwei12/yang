/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Setup of the audio hardware
*/

#include "ml_audio_setup.h"

#include <vmal.h>

#include <app/audio/audio_if.h>
#include <stream.h>
#include <panic.h>

Source Ml_SetupAudio(int mic_gain)
{
    Source input;

    VmalOperatorFrameworkEnableMainProcessor();
    VmalOperatorFrameworkEnableSecondProcessor();

    input = PanicNull(StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A));
    SourceConfigure(input, STREAM_CODEC_INPUT_RATE, 16000);
    SourceConfigure(input, STREAM_CODEC_INPUT_GAIN, mic_gain);
    MicbiasConfigure(MIC_BIAS_0, MIC_BIAS_ENABLE, (uint16)MIC_BIAS_FORCE_ON);

    return input;
}
