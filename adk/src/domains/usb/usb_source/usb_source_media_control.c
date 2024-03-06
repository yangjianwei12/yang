/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file   usb_source_media_control.c
    \ingroup    usb_source
    \brief  USB source - media control interface
*/

#include "usb_source.h"
#include "usb_source_hid.h"
#include "usb_audio.h"

static device_t usbSource_GetAudioDevice(audio_source_t source);

static const media_control_interface_t usb_source_media_control =
{
    .Play = UsbSource_Play,
    .Pause = UsbSource_Pause,
    .PlayPause = UsbSource_PlayPause,
    .Stop = UsbSource_Pause,
    .Forward = UsbSource_Forward,
    .Back = UsbSource_Back,
    .FastForward = UsbSource_FastForward,
    .FastRewind = UsbSource_FastRewind,
    .NextGroup = NULL,
    .PreviousGroup = NULL,
    .Shuffle = NULL,
    .Repeat = NULL,
    .Context = UsbSource_GetAudioContext,
    .Device = usbSource_GetAudioDevice
};

static const audio_source_volume_control_interface_t usb_source_volume_control =
{
    .VolumeUp = UsbSource_AudioVolumeUp,
    .VolumeDown = UsbSource_AudioVolumeDown,
    .VolumeSetAbsolute = UsbSource_AudioVolumeSetAbsolute,
    .Mute = UsbSource_AudioSpeakerMute
};

static device_t usbSource_GetAudioDevice(audio_source_t source)
{
    UNUSED(source);
    return UsbAudio_GetDevice();
}

void UsbSource_RegisterAudioControl(void)
{
    AudioSources_RegisterMediaControlInterface(audio_source_usb,
                                               &usb_source_media_control);
    AudioSources_RegisterVolumeControl(audio_source_usb,
                                       &usb_source_volume_control);
}

void UsbSource_DeregisterAudioControl(void)
{
    /* not implemented */
}
