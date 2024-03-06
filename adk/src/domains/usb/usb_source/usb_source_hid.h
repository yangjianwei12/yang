/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       usb_source_hid.h
    \addtogroup usb_source
    \brief      USB source - HID control header
    @{
*/

#ifndef USB_SOURCE_HID_H
#define USB_SOURCE_HID_H

#include "usb_source.h"

/*! \brief USB Source internal message IDs */
enum usb_source_internal_message_ids
{
    USB_SOURCE_INTERNAL_TERMINATE_CALL,                 /*!< Terminate call */
    USB_SOURCE_INTERNAL_ANSWER_CALL,                    /*!< Answer call */
    USB_SOURCE_INTERNAL_RESUME_CALL,                    /*!< Resume Call */

    USB_SOURCE_INTERNAL_RESET_OPERATION,                /*!< Reset operation initiated */
};

/*! \brief USB Source internal command locks */
typedef enum
{
    /* Terminate call once call is resumed (HOLD_EVT status = FALSE && OFF_HOOK_EVT status = TRUE)*/
    USB_SOURCE_LOCK_END_CALL_ON_RESUME          = 0x01,
    /* Answer incoming call once active call is terminated (OFF_HOOK_EVT status = FALSE)*/
    USB_SOURCE_LOCK_ANSWER_CALL_ON_TERMINATION  = 0x02,
    /* Resume call on hold once active call is terminated (OFF_HOOK_EVT status = FALSE)*/
    USB_SOURCE_LOCK_RESUME_CALL_ON_TERMINATION  = 0x04,
    USB_SOURCE_LOCK_MASK = 0xFF
}usb_source_command_lock_t;

typedef struct
{
    TaskData task;
    /* Size/datatype of hid_event_status_info(uint8) depends on USB_SOURCE_RX_HID_EVT_COUNT
     * and USB_SOURCE_HID_STATUS_SIZE.*/
    uint8 hid_event_status_info;
    uint8 command_lock;
} usb_source_info_t;

extern usb_source_info_t usb_source_info;

/*! Returns the USB Source task */
#define UsbSource_GetTask()                 (&usb_source_info.task)

#define UsbSource_SetCommandLock(lock)      usb_source_info.command_lock |= (lock)
#define UsbSource_ClearCommandLock(lock)    usb_source_info.command_lock &= ~(lock)
#define UsbSource_ClearAllCommandLock()     usb_source_info.command_lock = 0
#define UsbSource_IsCommandLockSet(lock)    ((usb_source_info.command_lock & (lock)) == (lock))
#define UsbSource_IsAnyCommandLockSet()    (usb_source_info.command_lock  != 0)

/*! \brief Send HID Play event  */
void UsbSource_Play(audio_source_t source);

/*! \brief Send HID Pause event  */
void UsbSource_Pause(audio_source_t source);

/*! \brief Send HID PlayPause event  */
void UsbSource_PlayPause(audio_source_t source);

/*! \brief Send HID Stop event  */
void UsbSource_Stop(audio_source_t source);

/*! \brief Send HID Forward event  */
void UsbSource_Forward(audio_source_t source);

/*! \brief Send HID Back event  */
void UsbSource_Back(audio_source_t source);

/*! \brief Send HID Fast Forward event  */
void UsbSource_FastForward(audio_source_t source, bool state);

/*! \brief Send HID Fast Rewind event  */
void UsbSource_FastRewind(audio_source_t source, bool state);

/*! \brief Send HID Audio Volume Up event  */
void UsbSource_AudioVolumeUp(audio_source_t source);

/*! \brief Send HID Audio Volume Down event  */
void UsbSource_AudioVolumeDown(audio_source_t source);

/*! \brief Send HID Audio Speaker Mute event  */
void UsbSource_AudioSpeakerMute(audio_source_t source, mute_state_t state);

/*! \brief Stub function for SetAbsolute command which is not supported by USB HID */
void UsbSource_AudioVolumeSetAbsolute(audio_source_t source, volume_t volume);

/*! \brief Send arbitrary HID event */
bool UsbSource_SendEvent(usb_source_control_event_t event);

/*! \brief Send HID Incomming Call Accept event  */
void UsbSource_IncomingCallAccept(voice_source_t source);

/*! \brief Send HID Incomming Call Reject event  */
void UsbSource_IncomingCallReject(voice_source_t source);

/*! \brief Send HID Ongoing Call Terminate event  */
void UsbSource_OngoingCallTerminate(voice_source_t source);

/*! \brief Handle TWC control action  */
void UsbSource_TwcControl(voice_source_t source, voice_source_twc_control_t action);

/*! \brief Send HID Toggle Microphone Mute event  */
void UsbSource_ToggleMicrophoneMute(voice_source_t source);

/*! \brief Send HID Voice Volume Up event  */
void UsbSource_VoiceVolumeUp(voice_source_t source);

/*! \brief Send HID Voice Volume Down event  */
void UsbSource_VoiceVolumeDown(voice_source_t source);

/*! \brief Stub function for SetAbsolute command which is not supported by USB HID */
void UsbSource_VoiceVolumeSetAbsolute(voice_source_t source, volume_t volume);

/*! \brief Send HID Voice Speaker Mute event  */
void UsbSource_VoiceSpeakerMute(voice_source_t source, mute_state_t state);

/*! \brief Send arbitrary HID report */
bool UsbSource_SendReport(const uint8 *report, uint16 size);

/*! \brief Reset USB HID received event status */
void UsbSource_ResetHidEventStatus(void);

/*! \brief Get USB HID off-hook event status */
usb_source_hid_event_status_t UsbSource_GetHidOffHookStatus(void);

/*! \brief Get USB HID ring event status */
usb_source_hid_event_status_t UsbSource_GetHidRingStatus(void);

/*! \brief Get USB HID mute event status */
usb_source_hid_event_status_t UsbSource_GetHidMuteStatus(void);

/*! \brief Get USB HID hold event status */
usb_source_hid_event_status_t UsbSource_GetHidHoldStatus(void);

#endif /* USB_SOURCE_HID_H */

/*! @} */