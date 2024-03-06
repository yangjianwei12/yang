/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file   usb_source_hid.c
    \ingroup    usb_source
    \brief  USB source - HID control commands
*/

#include "usb_audio_fd.h"
#include "usb_source.h"
#include "usb_source_hid.h"
#include "logging.h"
#include "audio_sources.h"
#include "voice_sources.h"
#include <panic.h>
#include <telephony_messages.h>
#include "ui.h"
#include "focus_voice_source.h"
#include <device_list.h>

/* Status of each event is tri-state and will consume 2 bits */
#define USB_SOURCE_HID_STATUS_POS(event)   (event * USB_SOURCE_HID_STATUS_SIZE)

/* Timer is active when it need to send a USB event after a delay.*/
#define USB_SOURCE_TWC_OPERATION_TIMEOUT_MS     2000
/* Delay has been determined experimentally. After resuming call from hold,
 * MS Teams on Windows is not responding to call terminate request, if send earlier */
#define USB_SOURCE_END_CALL_ON_RESUME_DELAY_MS  1000
/* The delay is experimental, not supported by MS Teams.
 * After active call termination, may need to wait for short duration before answering incoming call */
#define USB_SOURCE_ANSWER_CALL_TERMINATION_DELAY_MS          100

usb_source_info_t usb_source_info;

static usb_source_hid_interface_t *usb_source_hid_interface = NULL;

static void usbSource_MsgHandler(Task task, MessageId id, Message message);

static usb_source_hid_event_status_t usbSource_GetHidEventStatus(usb_source_rx_hid_event_t event)
{
    PanicFalse(event < USB_SOURCE_RX_HID_EVT_COUNT);
    return (usb_source_hid_event_status_t)((usb_source_info.hid_event_status_info >> USB_SOURCE_HID_STATUS_POS(event)) & USB_SOURCE_HID_STATUS_MASK);
}

static void usbSource_SetHidEventStatus(usb_source_rx_hid_event_t event, usb_source_hid_event_status_t event_status)
{
    PanicFalse(event < USB_SOURCE_RX_HID_EVT_COUNT);
    usb_source_info.hid_event_status_info = (usb_source_info.hid_event_status_info & ~(USB_SOURCE_HID_STATUS_MASK << USB_SOURCE_HID_STATUS_POS(event))) |
                            ((event_status & USB_SOURCE_HID_STATUS_MASK) << USB_SOURCE_HID_STATUS_POS(event));
    DEBUG_LOG_VERBOSE("usbSource_SetHidEventStatus: hid_event_status_info 0x%X", usb_source_info.hid_event_status_info);
}

static void UsbSource_HandleUsbHidEvent(usb_source_rx_hid_event_t event, bool is_active)
{
    usb_source_hid_event_status_t event_status = (is_active ? USB_SOURCE_HID_STATUS_ACTIVE :
                                                              USB_SOURCE_HID_STATUS_INACTIVE);
    usb_source_hid_event_status_t present_status = usbSource_GetHidEventStatus(event);
    voice_source_t focused_source = voice_source_none;

    if(present_status != event_status)
    {
        DEBUG_LOG_DEBUG("UsbSource_HandleUsbHidEvent: Event enum:usb_source_rx_hid_event_t:%d"
                       " status 0x%X", event, is_active);

        usbSource_SetHidEventStatus(event, event_status);
        if((present_status == USB_SOURCE_HID_STATUS_UNDEFINED) &&
                (event_status == USB_SOURCE_HID_STATUS_INACTIVE))
        {
            return;
        }

        switch(event)
        {
            case USB_SOURCE_RX_HID_MUTE_EVT:
                if (is_active)
                {
                    Telephony_NotifyMicrophoneMuted(voice_source_usb);
                }
                else
                {
                    Telephony_NotifyMicrophoneUnmuted(voice_source_usb);
                }
                break;
            case USB_SOURCE_RX_HID_OFF_HOOK_EVT:
                if (is_active)
                {
                    if(UsbSource_IsCommandLockSet(USB_SOURCE_LOCK_END_CALL_ON_RESUME))
                    {
                        MessageCancelAll(UsbSource_GetTask(), USB_SOURCE_INTERNAL_TERMINATE_CALL);
                        MessageSendLater(UsbSource_GetTask(), USB_SOURCE_INTERNAL_TERMINATE_CALL, NULL, USB_SOURCE_END_CALL_ON_RESUME_DELAY_MS);
                    }
                    else
                    {
                        /* Updating MRU index of usb_device */
                        DeviceList_DeviceWasUsed(UsbAudio_GetDevice());

                        Telephony_NotifyCallActive(voice_source_usb);
                    }
                }
                else
                {
                    if(UsbSource_IsCommandLockSet(USB_SOURCE_LOCK_ANSWER_CALL_ON_TERMINATION))
                    {
                        MessageCancelAll(UsbSource_GetTask(), USB_SOURCE_INTERNAL_ANSWER_CALL);
                        MessageSendLater(UsbSource_GetTask(), USB_SOURCE_INTERNAL_ANSWER_CALL, NULL, USB_SOURCE_END_CALL_ON_RESUME_DELAY_MS);
                    }
                    else if(UsbSource_GetHidHoldStatus() != USB_SOURCE_HID_STATUS_ACTIVE)
                    {
                        Telephony_NotifyCallEnded(voice_source_usb);
                    }
                }
                break;
            case USB_SOURCE_RX_HID_HOLD_EVT:
                if (is_active)
                {
                    Telephony_NotifyHoldActive(voice_source_usb);
                }
                else
                {
                    if(!UsbSource_IsCommandLockSet(USB_SOURCE_LOCK_END_CALL_ON_RESUME))
                    {
                        Telephony_NotifyHoldInactive(voice_source_usb);
                        if(UsbSource_GetHidOffHookStatus() != USB_SOURCE_HID_STATUS_ACTIVE)
                        {
                            Telephony_NotifyCallEnded(voice_source_usb);
                        }
                    }
                }
                break;
            case USB_SOURCE_RX_HID_RING_EVT:
                if (is_active)
                {
                    /* Updating MRU index of usb_device */
                    DeviceList_DeviceWasUsed(UsbAudio_GetDevice());

                    Telephony_NotifyCallIncomingOutOfBandRingtone(voice_source_usb);
                }
                else
                {
                    Telephony_NotifyCallIncomingEnded(voice_source_usb);
                }
                break;
            default:
                DEBUG_LOG_ERROR("UsbSource_HandleUsbHidEvent : UNSUPPORTED EVENT");
                Panic();
        }

        Focus_GetVoiceSourceForContext(ui_provider_telephony, &focused_source);
        if (focused_source == voice_source_usb)
        {
            Ui_InformContextChange(ui_provider_telephony, VoiceSources_GetSourceContext(voice_source_usb));
        }
        else
        {
            DEBUG_LOG_VERBOSE("UsbSource_HandleUsbHidEvent didn't push context for unfocused enum:voice_source_t:%d", voice_source_usb);
        }
    }
}

static void usbSource_SetTwcCommandLock(usb_source_command_lock_t lock)
{
    UsbSource_SetCommandLock(lock);
    MessageCancelAll(UsbSource_GetTask(), USB_SOURCE_INTERNAL_RESET_OPERATION);
    MessageSendLater(UsbSource_GetTask(), USB_SOURCE_INTERNAL_RESET_OPERATION, NULL, USB_SOURCE_TWC_OPERATION_TIMEOUT_MS);
}

static void usbSource_ClearTwcCommandLock(usb_source_command_lock_t lock)
{
    UsbSource_ClearCommandLock(lock);
    if(!UsbSource_IsAnyCommandLockSet())
    {
        MessageCancelAll(UsbSource_GetTask(), USB_SOURCE_INTERNAL_RESET_OPERATION);
    }
}

static void usbSource_ClearAllTwcCommandLock(void)
{
    UsbSource_ClearAllCommandLock();
    MessageCancelAll(UsbSource_GetTask(), USB_SOURCE_INTERNAL_ANSWER_CALL);
    MessageCancelAll(UsbSource_GetTask(), USB_SOURCE_INTERNAL_TERMINATE_CALL);
}

void UsbSource_ResetHidEventStatus(void)
{
    DEBUG_LOG_VERBOSE("UsbSource_ResetHidEventStatus");
    /* Setting all the bits to 1 which will indicate STATUS_UNDEFINED*/
    usb_source_info.hid_event_status_info = ~(usb_source_info.hid_event_status_info & 0);

    usbSource_ClearAllTwcCommandLock();

    if(UsbSource_GetTask()->handler == NULL)
    {
        UsbSource_GetTask()->handler = usbSource_MsgHandler;
    }
}

usb_source_hid_event_status_t UsbSource_GetHidOffHookStatus(void)
{
    return usbSource_GetHidEventStatus(USB_SOURCE_RX_HID_OFF_HOOK_EVT);
}

usb_source_hid_event_status_t UsbSource_GetHidRingStatus(void)
{
    return usbSource_GetHidEventStatus(USB_SOURCE_RX_HID_RING_EVT);
}

usb_source_hid_event_status_t UsbSource_GetHidMuteStatus(void)
{
    return usbSource_GetHidEventStatus(USB_SOURCE_RX_HID_MUTE_EVT);
}

usb_source_hid_event_status_t UsbSource_GetHidHoldStatus(void)
{
    return usbSource_GetHidEventStatus(USB_SOURCE_RX_HID_HOLD_EVT);
}

void UsbSource_RegisterHid(usb_source_hid_interface_t *hid_interface)
{
    usb_source_hid_interface = hid_interface;

    /* Ensuring size of hid_event_status_info is sufficient for all supported USB HID events*/
    PanicFalse((sizeof(usb_source_info.hid_event_status_info) * 8) >= (USB_SOURCE_RX_HID_EVT_COUNT * USB_SOURCE_HID_STATUS_SIZE));

    UsbSource_ResetHidEventStatus();
    if(usb_source_hid_interface->register_handler)
    {
        usb_source_hid_interface->register_handler(UsbSource_HandleUsbHidEvent);
    }

    UsbSource_GetTask()->handler = usbSource_MsgHandler;
}

void UsbSource_UnregisterHid(void)
{
    UsbSource_GetTask()->handler = NULL;
    UsbSource_ResetHidEventStatus();
    if(usb_source_hid_interface->unregister_handler)
    {
        usb_source_hid_interface->unregister_handler();
    }
    usb_source_hid_interface = NULL;
}

bool UsbSource_SendEvent(usb_source_control_event_t event)
{
    if ((UsbSource_IsAudioSupported() || UsbSource_IsVoiceSupported()) &&
            usb_source_hid_interface && usb_source_hid_interface->send_event)
    {
        return usb_source_hid_interface->send_event(event) == USB_RESULT_OK;
    }
    return FALSE;
}

bool UsbSource_SendReport(const uint8 *report, uint16 size)
{
    if (usb_source_hid_interface &&
            usb_source_hid_interface->send_report)
    {
        return usb_source_hid_interface->send_report(report, size) == USB_RESULT_OK;
    }
    return FALSE;
}

void UsbSource_Play(audio_source_t source)
{
    if (source == audio_source_usb)
    {
        UsbSource_SendEvent(USB_SOURCE_PLAY);
    }
}

void UsbSource_Pause(audio_source_t source)
{
    if (source == audio_source_usb)
    {
        UsbSource_SendEvent(USB_SOURCE_PAUSE);
    }
}

void UsbSource_PlayPause(audio_source_t source)
{
    if (source == audio_source_usb)
    {
        UsbSource_SendEvent(USB_SOURCE_PLAY_PAUSE);
    }
}

void UsbSource_Stop(audio_source_t source)
{
    if (source == audio_source_usb)
    {
        UsbSource_SendEvent(USB_SOURCE_STOP);
    }
}

void UsbSource_Forward(audio_source_t source)
{
    if (source == audio_source_usb)
    {
        UsbSource_SendEvent(USB_SOURCE_NEXT_TRACK);
    }
}

void UsbSource_Back(audio_source_t source)
{
    if (source == audio_source_usb)
    {
        UsbSource_SendEvent(USB_SOURCE_PREVIOUS_TRACK);
    }
}

void UsbSource_FastForward(audio_source_t source, bool state)
{
    if (source == audio_source_usb)
    {
        UsbSource_SendEvent(state ?
                                USB_SOURCE_FFWD_ON:
                                USB_SOURCE_FFWD_OFF);
    }
}

void UsbSource_FastRewind(audio_source_t source, bool state)
{
    if (source == audio_source_usb)
    {
        UsbSource_SendEvent(state ?
                                USB_SOURCE_REW_ON:
                                USB_SOURCE_REW_OFF);
    }
}

void UsbSource_AudioVolumeUp(audio_source_t source)
{
    if (source == audio_source_usb)
    {
        UsbSource_SendEvent(USB_SOURCE_VOL_UP);
    }
}

void UsbSource_AudioVolumeDown(audio_source_t source)
{
    if (source == audio_source_usb)
    {
        UsbSource_SendEvent(USB_SOURCE_VOL_DOWN);
    }
}

void UsbSource_AudioSpeakerMute(audio_source_t source, mute_state_t state)
{
    UNUSED(state);
    if (source == audio_source_usb)
    {
        UsbSource_SendEvent(USB_SOURCE_MUTE);
    }
}

void UsbSource_AudioVolumeSetAbsolute(audio_source_t source, volume_t volume)
{
    if (source == audio_source_usb)
    {
        volume_t current_volume = AudioSources_GetVolume(source);
        if (volume.value > current_volume.value)
        {
            UsbSource_AudioVolumeUp(source);
        }
        else if (volume.value < current_volume.value)
        {
            UsbSource_AudioVolumeDown(source);
        }
    }
}

void UsbSource_IncomingCallAccept(voice_source_t source)
{
    if (source == voice_source_usb)
    {
        if(UsbSource_GetHidRingStatus() == USB_SOURCE_HID_STATUS_ACTIVE)
        {
            /* This implementation works with hosts which support HOOK SWITCH usage of USB HID. */
            UsbSource_SendEvent(USB_SOURCE_HOOK_SWITCH_ANSWER);
            Telephony_NotifyCallAnswered(voice_source_usb);
        }
        else if(UsbSource_GetHidRingStatus() == USB_SOURCE_HID_STATUS_UNDEFINED)
        {
            /* This implementation works with Android hosts which does not support HOOK SWITCH, but
             * supports PLAY_PAUSE to accept call. */
            UsbSource_SendEvent(USB_SOURCE_PLAY_PAUSE);
        }
        else
        {
            DEBUG_LOG_WARN("UsbSource: No Incoming Call - HidRingStatus: "
                           "enum:usb_source_hid_event_status_t:%d ", UsbSource_GetHidRingStatus());
        }
    }
}

void UsbSource_IncomingCallReject(voice_source_t source)
{
    if (source == voice_source_usb)
    {
        if(UsbSource_GetHidRingStatus() == USB_SOURCE_HID_STATUS_ACTIVE)
        {
            /* Version 4.0 of the "Microsoft Teams Devices General Specification" specifies
             * "Button 1" for Teams compatibility. The Jabra developer documentation shows that
             * a Button is required for correct operation for a call reject. */
            UsbSource_SendEvent(USB_SOURCE_BUTTON_ONE);
            Telephony_NotifyCallRejected(voice_source_usb);
        }
        else if(UsbSource_GetHidRingStatus() == USB_SOURCE_HID_STATUS_UNDEFINED)
        {
            DEBUG_LOG_WARN("UsbSource: Host does not support; HidRingStatus: "
                           "enum:usb_source_hid_event_status_t:%d ", UsbSource_GetHidRingStatus());
        }
        else
        {
            DEBUG_LOG_WARN("UsbSource: No Incoming Call - HidRingStatus: "
                           "enum:usb_source_hid_event_status_t:%d ", UsbSource_GetHidRingStatus());
        }
    }
}

void UsbSource_OngoingCallTerminate(voice_source_t source)
{
    if (source == voice_source_usb)
    {
        if(UsbSource_GetHidOffHookStatus() == USB_SOURCE_HID_STATUS_ACTIVE)
        {
            /* This implementation works with hosts which support HOOK SWITCH usage of USB HID. */
            UsbSource_SendEvent(USB_SOURCE_HOOK_SWITCH_TERMINATE);
            Telephony_NotifyCallTerminated(voice_source_usb);
        }
        else if(UsbSource_GetHidOffHookStatus() == USB_SOURCE_HID_STATUS_UNDEFINED)
        {
            /* This implementation works with Android hosts which does not support HOOK SWITCH, but
             * supports PLAY_PAUSE to terminate call. */
            UsbSource_SendEvent(USB_SOURCE_PLAY_PAUSE);
        }
        else
        {
            DEBUG_LOG_WARN("UsbSource: No Ongoing Call - HidOffHookStatus:enum:usb_source_hid_event_status_t:%d ",
                           UsbSource_GetHidOffHookStatus());
        }
    }
}

void UsbSource_TwcControl(voice_source_t source, voice_source_twc_control_t action)
{
    DEBUG_LOG_DEBUG("UsbSource_TwcControl enum:voice_source_t:%d, enum:voice_source_twc_control_t:%d, "
                    "hid_event_status_info:%d", source, action, usb_source_info.hid_event_status_info);

    if (source == voice_source_usb)
    {
        if(UsbSource_GetHidOffHookStatus() != USB_SOURCE_HID_STATUS_UNDEFINED)
        {
            switch(action)
            {
                case voice_source_release_held_reject_waiting:
                    if(UsbSource_GetHidHoldStatus() == USB_SOURCE_HID_STATUS_ACTIVE)
                    {
                        /* Need to wait till call resumes before we can terminate the call */
                        usbSource_SetTwcCommandLock(USB_SOURCE_LOCK_END_CALL_ON_RESUME);
                        UsbSource_SendEvent(USB_SOURCE_FLASH);
                        /* If there were any active call, that will be put on hold for terminating other call
                         * and shall need to resume it back post termination */
                    }
                    if(UsbSource_GetHidRingStatus() == USB_SOURCE_HID_STATUS_ACTIVE)
                    {
                        UsbSource_SendEvent(USB_SOURCE_BUTTON_ONE);
                    }
                    break;

                case voice_source_release_active_accept_other:
                    if(UsbSource_GetHidOffHookStatus() == USB_SOURCE_HID_STATUS_ACTIVE)
                    {
                        if(UsbSource_GetHidRingStatus() == USB_SOURCE_HID_STATUS_ACTIVE)
                        {
                            /* Need to wait till present call termination before resuming held call */
                            usbSource_SetTwcCommandLock(USB_SOURCE_LOCK_ANSWER_CALL_ON_TERMINATION);
                        }
                        UsbSource_SendEvent(USB_SOURCE_HOOK_SWITCH_TERMINATE);
                    }
                    else if(UsbSource_GetHidHoldStatus() == USB_SOURCE_HID_STATUS_ACTIVE)
                    {
                        UsbSource_SendEvent(USB_SOURCE_FLASH);
                    }
                    else if(UsbSource_GetHidRingStatus() == USB_SOURCE_HID_STATUS_ACTIVE)
                    {
                        UsbSource_SendEvent(USB_SOURCE_HOOK_SWITCH_ANSWER);
                    }
                    break;

                case voice_source_hold_active_accept_other:
                    if((UsbSource_GetHidOffHookStatus() == USB_SOURCE_HID_STATUS_ACTIVE) ||
                            (UsbSource_GetHidHoldStatus() == USB_SOURCE_HID_STATUS_ACTIVE))
                    {
                        UsbSource_SendEvent(USB_SOURCE_FLASH);
                    }
                    break;

                case voice_source_add_held_to_multiparty:
                case voice_source_join_calls_and_hang_up:
                    DEBUG_LOG_WARN("UsbSource: Action Not supported -  enum:voice_source_twc_control_t:%d ", action);
                    break;

                default:
                    Panic();
                    break;
            }
        }
    }
}
void UsbSource_ToggleMicrophoneMute(voice_source_t source)
{
    if (source == voice_source_usb)
    {
        UsbSource_SendEvent(USB_SOURCE_PHONE_MUTE);
    }
}

void UsbSource_VoiceVolumeUp(voice_source_t source)
{
    if (source == voice_source_usb)
    {
        UsbSource_SendEvent(USB_SOURCE_VOL_UP);
    }
}

void UsbSource_VoiceVolumeDown(voice_source_t source)
{
    if (source == voice_source_usb)
    {
        UsbSource_SendEvent(USB_SOURCE_VOL_DOWN);
    }
}

void UsbSource_VoiceVolumeSetAbsolute(voice_source_t source, volume_t volume)
{
    if (source == voice_source_usb)
    {
        volume_t current_volume = VoiceSources_GetVolume(source);
        if (volume.value > current_volume.value)
        {
            UsbSource_VoiceVolumeUp(source);
        }
        else if (volume.value < current_volume.value)
        {
            UsbSource_VoiceVolumeDown(source);
        }
    }
}

void UsbSource_VoiceSpeakerMute(voice_source_t source, mute_state_t state)
{
    UNUSED(state);
    if (source == voice_source_usb)
    {
        UsbSource_SendEvent(USB_SOURCE_MUTE);
    }
}

static void usbSource_MsgHandler(Task task, MessageId id, Message message)
{
    DEBUG_LOG_INFO("usbSource_MsgHandler MessageId: 0x%x", id);
    UNUSED(task);
    UNUSED(message);

    switch (id)
    {
        case USB_SOURCE_INTERNAL_TERMINATE_CALL:
            usbSource_ClearTwcCommandLock(USB_SOURCE_LOCK_END_CALL_ON_RESUME);
            if(UsbSource_GetHidOffHookStatus() == USB_SOURCE_HID_STATUS_ACTIVE)
            {
                UsbSource_SendEvent(USB_SOURCE_HOOK_SWITCH_TERMINATE);
            }
            break;

        case USB_SOURCE_INTERNAL_ANSWER_CALL:
            usbSource_ClearTwcCommandLock(USB_SOURCE_LOCK_ANSWER_CALL_ON_TERMINATION);
            if(UsbSource_GetHidRingStatus() == USB_SOURCE_HID_STATUS_ACTIVE)
            {
                UsbSource_SendEvent(USB_SOURCE_HOOK_SWITCH_ANSWER);
            }
            break;

        case USB_SOURCE_INTERNAL_RESET_OPERATION:
            if(UsbSource_IsAnyCommandLockSet())
            {
                DEBUG_LOG_WARN("usbSource_MsgHandler RESET_OPERATION: 0x%x", usb_source_info.command_lock);
                usbSource_ClearAllTwcCommandLock();
            }
            break;

        default:
            break;
    }

}
