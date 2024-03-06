/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       wired_audio_media_control_interface.c
    \ingroup    wired_source
    \brief      wired audio media control interface definitions
*/

#if defined(INCLUDE_WIRED_ANALOG_AUDIO) || defined(INCLUDE_A2DP_ANALOG_SOURCE) || defined(INCLUDE_LE_AUDIO_ANALOG_SOURCE)

#include "wired_audio_media_control_interface.h"
#include "kymera_adaptation_audio_protected.h"
#include "wired_audio_private.h"
#include "audio_sources.h"

#include <stdlib.h>
#include <panic.h>
#include "logging.h"
#include "wired_audio_media_control_interface.h"

static unsigned wiredAudioSource_Context(audio_source_t source);

static const media_control_interface_t wired_source_media_control_interface =
{
    .Play = NULL,
    .Pause = NULL,
    .PlayPause = NULL,
    .Stop = NULL,
    .Forward = NULL,
    .Back = NULL,
    .FastForward = NULL,
    .FastRewind = NULL,
    .NextGroup = NULL,
    .PreviousGroup = NULL,
    .Shuffle = NULL,
    .Repeat = NULL,
    .Context = wiredAudioSource_Context,
    .Device = WiredAudioSource_GetDevice,
};

static unsigned wiredAudioSource_Context(audio_source_t source)
{
    /* Get the wired audio context */
    return WiredAudioSource_GetContext(source);
}

const media_control_interface_t * WiredAudioSource_GetMediaControlInterface(void)
{
    return &wired_source_media_control_interface;
}

#endif /* INCLUDE_WIRED_ANALOG_AUDIO || INCLUDE_A2DP_ANALOG_SOURCE || INCLUDE_LE_AUDIO_ANALOG_SOURCE */
