/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Volume observer that syncs between AVRCP/LE audio and USB
*/

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) || defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)

#include "usb_dongle_logging.h"
#include "usb_dongle_volume_observer.h"
#include "usb_dongle_config.h"
#include "audio_sources.h"
#include <usb_device.h>
#include <usb_audio.h>
#include <volume_utils.h>
#include <volume_service.h>
#include <volume_messages.h>
#include <telephony_messages.h>

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
#include <avrcp.h>
#include <aghfp.h>
#include <av.h>
#include <aghfp_profile.h>
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
#include "le_audio_client.h"
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#define USB_DONGLE_VO_USB_VOLUME_CHANGE_DELAY_MS            20      /*!< delay between sending USB Volume inc/dec commands */
#define USB_DONGLE_VO_SINK_VOLUME_CHANGE_DELAY_MS           200     /*!< delay between sending AVRCP/LE audio Volume updates commands */

/* USB volume (in percentage) below which the volume down HID command is not allowed to send
   if the target volume is non-zero for windows host */
#define USB_DONGLE_VO_LE_USB_VOLUME_CUTOFF_FOR_VOLUME_DOWN  7

/* if the USB host becomes unresponsive, we need to stop sending volume messages */
#define USB_VOLUME_CHANGE_COUNT_MAX                         20

/* Burst sent to classify Host OS */
#define BURST_SIZE                                          4

#define USB_DONGLE_VO_LE_INVALID_LE_AUDIO_VOLUME            0xFFFF

/* As per Volume Control Service document, Table 3.8 */
#define LE_AUDIO_VOLUME_MIN_STEPS                           0
#define LE_AUDIO_VOLUME_MAX_STEPS                           255
#define LE_AUDIO_VOLUME_NUM_STEPS                           ((LE_AUDIO_VOLUME_MAX_STEPS - LE_AUDIO_VOLUME_MIN_STEPS) + 1)

static void usbDongle_NotifyAudioRoutingChange(audio_source_t source, source_routing_change_t change);
static void usbDongle_NotifyVolumeChange(audio_source_t source, event_origin_t origin, volume_t volume);
static void usbDongle_HandleUpdateUsbVolume(void);
static void usbDongle_NotifyVoiceVolumeChange(voice_source_t source, event_origin_t origin, volume_t volume);
static void usbDongle_NotifyVoiceMuteChange(voice_source_t source, event_origin_t origin, bool mute_state);
static void usbDongle_SendHidVolumeMessage(void);
static void usbDongle_UpdateVolumeChangeDirection(volume_t usb_volume, int16 new_usb_volume);

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
static void usbDongle_QueueAvrcpVolumeUpdate(uint8 volume);
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
static void usbDongle_LeaVolumeQueueLeAudioVolumeUpdate(uint8  volume);
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

enum vo_internal_message_ids
{
    VO_INTERNAL_UPDATE_USB_VOLUME,      /*!< Message used to create a delay between each USB volume inc/dec */
    VO_INTERNAL_UPDATE_AVRCP_VOLUME,    /*!< Message used to create a delay between each AVRCP absolute volume */
    VO_INTERNAL_UPDATE_LE_AUDIO_VOLUME  /*!< Message used to create a delay between each LE Audio absolute volume */
};

typedef enum vo_internal_state
{
    VO_INTERNAL_STATE_IDLE,                 /*!< No volume changes in this state */
    VO_INTERNAL_STATE_USB_VOLUME_SYNC,      /*!< Syncing the USB volume */
    VO_INTERNAL_STATE_SINK_VOLUME_SYNC      /*!< Syncing the Sink volume */
} vo_internal_state_t;

typedef enum host_os_type
{
    HOST_OS_TYPE_WINDOWS_MAC,        /*!< Host OS is Windows or MacOS */
    HOST_OS_TYPE_LINUX_ANDROID,      /*!< Host OS is Linux or Android */
    HOST_OS_TYPE_UNIDENTIFIED        /*!< Host OS has not been identified */
} host_os_type_t;

typedef enum
{
    request_type_host_to_sink,
    request_type_sink_to_host
} request_type_t;

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
typedef struct
{
    bool                    avrcp_connected;        /*!< AVRCP is connected */
    bool                    aghfp_connected;        /*!< AGHFP is connected */
    bool                    hold_active;            /*!< TRUE = Call is on HOLD, FALSE = Call is NOT on HOLD */
    bool                    pre_hold_mute_state;    /*!< State of mute before the call went on hold so that state can be reverted to */

    uint8                   avrcp_volume_target;    /*!< value to set on AVRCP */

    avInstanceTaskData      *av_instance;
    aghfpInstanceTaskData   *aghfp_instance;
} bredr_volume_t;
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/* data for handling LE Audio volume updates */
typedef struct
{
    bool            audio_connected;
    bool            audio_vbc_active;
    uint8           volume_target;              /*!< value to set on LE Audio */
    uint16          last_sync_sink_volume;      /*!< The last volume level that is synchronized to the sink.
                                                  Size uint16 is chosen so that its value can be cleared to
                                                  an invalid value outside of LE audio range (0-255) */
    ServiceHandle   audio_group_handle;
} le_audio_volume_t;
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/* data for handling USB volume updates */
typedef struct
{
    bool            up;             /*!< TRUE = increase volume, FALSE = decrement volume */
    uint8           count;          /*!< volume changes sent without a volume update from the USB host */
    int16           target;         /*!< target volume we are aiming for */
} usb_volume_t;

typedef struct
{
    TaskData            task;

    bool                usb_connected;  /*!< USB is connected */

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
    bredr_volume_t      bredr_volume;
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    le_audio_volume_t   le_audio_volume;
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    usb_volume_t        usb_volume;

    vo_internal_state_t state;
    bool                mic_muted;

    volume_t            old_volume;     /*!< Previous USB volume */
    host_os_type_t      host_os;        /*!< Holds the Host OS type */

} usb_dongle_volume_observer_data_t;

static usb_dongle_volume_observer_data_t volume_observer_data;

#define GetTaskData()   (&volume_observer_data)
#define GetTask()       (&(GetTaskData()->task))

/* The volume range of AVRCP */
const volume_config_t usb_dongle_observer_avrcp_config = {
    .range = {
        .max = 127,
        .min = 0
    },
    .number_of_steps = 127
};

/* The volume range of LE Audio */
const volume_config_t usb_dongle_observer_le_audio_config = {
    .range = {
        .max = LE_AUDIO_VOLUME_MAX_STEPS,
        .min = LE_AUDIO_VOLUME_MIN_STEPS
    },
    .number_of_steps = LE_AUDIO_VOLUME_NUM_STEPS
};

/* The volume range of USB */
const volume_config_t usb_dongle_observer_usb_config = {
    .range = {
        .max = USB_AUDIO_VOLUME_MAX_STEPS,
        .min = USB_AUDIO_VOLUME_MIN_STEPS
    },
    .number_of_steps = USB_AUDIO_VOLUME_NUM_STEPS
};

/* The volume range that the Host sends to us */
const volume_config_t usb_dongle_observer_host_out_config = {
    .range = {
        .max = USB_AUDIO_VOLUME_MAX_STEPS,
        .min = USB_AUDIO_VOLUME_MIN_STEPS
    },
    .number_of_steps = USB_AUDIO_VOLUME_NUM_STEPS
};

static const audio_source_observer_interface_t usb_dongle_observer_interface =
{
    .OnVolumeChange = usbDongle_NotifyVolumeChange,
    .OnAudioRoutingChange = usbDongle_NotifyAudioRoutingChange
};

static const voice_source_observer_interface_t usb_dongle_voice_observer_interface =
{
    .OnVolumeChange = usbDongle_NotifyVoiceVolumeChange,
    .OnMuteChange = usbDongle_NotifyVoiceMuteChange
};


static void usbDongle_UpdateSinkVolume(volume_t volume)
{
#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
    if (GetTaskData()->bredr_volume.aghfp_connected)
    {
        /* Call the volume service to update AGHFP Volume */
        VolumeService_SetVoiceSourceVolume(voice_source_hfp_1, event_origin_local, volume);

    }
    else
    {
       DEBUG_LOG("usbDongle_UpdateSinkVolume, no AGHFP connection");
    }

    if (GetTaskData()->bredr_volume.avrcp_connected)
    {
        volume_t old_avrcp_volume = AudioSources_GetVolume(audio_source_a2dp_1);
        int16 new_avrcp_volume;

        new_avrcp_volume = VolumeUtils_ConvertToVolumeConfig(volume, usb_dongle_observer_avrcp_config);
        DEBUG_LOG("UsbDongle_UpdateAvrcpVolume, USB %d, old AVRCP %d, new AVRCP %d", volume.value, old_avrcp_volume.value, new_avrcp_volume);
        usbDongle_QueueAvrcpVolumeUpdate(new_avrcp_volume);
    }
    else
    {
        DEBUG_LOG("usbDongle_UpdateSinkVolume, no AVRCP connection");
    }
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    if(GetTaskData()->le_audio_volume.audio_connected)
    {
        volume_t old_volume = AudioSources_GetVolume(audio_source_le_audio_unicast_sender);
        int16 new_volume;
        new_volume = VolumeUtils_ConvertToVolumeConfig(volume, usb_dongle_observer_le_audio_config);
        DEBUG_LOG_VERBOSE("usbDongle_LeaVolumeUpdateSinkVolume, USB %d, old %d, new %d",
                          volume.value, old_volume.value, new_volume);
        usbDongle_LeaVolumeQueueLeAudioVolumeUpdate(new_volume);
    }
    else
    {
        DEBUG_LOG_VERBOSE("usbDongle_LeaVolumeUpdateSinkVolume, no LE Audio connection");
    }
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
}

/*
 * Different strategies to update USB volume target depending on the host OS.
 */
static void usbDongle_UpdateUsbVolumeTarget(volume_t volume)
{
    volume_t usb_volume = AudioSources_GetVolume(audio_source_usb);
    int16 new_usb_volume = VolumeUtils_ConvertToVolumeConfig(volume, usb_dongle_observer_host_out_config);

    DEBUG_LOG("usbDongle_UpdateUsbVolumeTarget, AVRCP/LE audio %d, old USB %d, new USB %d", volume.value, usb_volume.value, new_usb_volume );
    MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_USB_VOLUME);

    switch(GetTaskData()->host_os)
    {
        /* Current USB volume compared to target USB volume. */
        case HOST_OS_TYPE_WINDOWS_MAC:
            usbDongle_UpdateVolumeChangeDirection(usb_volume, new_usb_volume);
            break;

        /* Previous target USB volume compared to new target USB volume.
         * Because we do not know current USB volume. */
        case HOST_OS_TYPE_LINUX_ANDROID:
            usbDongle_UpdateVolumeChangeDirection(GetTaskData()->old_volume, new_usb_volume);
            GetTaskData()->old_volume.value = new_usb_volume;
            break;

        /* For the first volume request, current USB volume is compared to target USB volume.
         * And the AVRCP/LE audio volume is set.*/
        case HOST_OS_TYPE_UNIDENTIFIED:
            usbDongle_UpdateVolumeChangeDirection(usb_volume, new_usb_volume);
            break;

        default:
            break;
    }
}

/*
 * Find if we need to increment or decrement the USB volume
 */
static void usbDongle_UpdateVolumeChangeDirection(volume_t usb_volume, int16 new_usb_volume)
{
    /* USB volume is already at the target, no change required */
    if (new_usb_volume == usb_volume.value)
    {
        DEBUG_LOG_VERBOSE("usbDongle_UpdateUsbVolumeChangeDirection, no change" );
        GetTaskData()->state = VO_INTERNAL_STATE_SINK_VOLUME_SYNC;
    }
    else    /* calculate the change, up or down */
    {
        GetTaskData()->usb_volume.target = new_usb_volume;
        if (new_usb_volume > usb_volume.value)
        {
            DEBUG_LOG_VERBOSE("usbDongle_UpdateUsbVolumeChangeDirection, volume up" );
            GetTaskData()->usb_volume.up = TRUE;
        }
        else
        {
            DEBUG_LOG_VERBOSE("usbDongle_UpdateUsbVolumeChangeDirection, volume down" );
            GetTaskData()->usb_volume.up = FALSE;
        }
        /* send HID volume messages */
        usbDongle_HandleUpdateUsbVolume();
    }
}

static void usbDongle_StateUsbVolumeSync(request_type_t request_type, volume_t volume)
{
    switch(request_type)
    {
        /* Host has sent us a USB volume update, check if we've met the target */
        case request_type_host_to_sink:
            GetTaskData()->usb_volume.count = 0;
            GetTaskData()->host_os = HOST_OS_TYPE_WINDOWS_MAC;
            DEBUG_LOG_INFO("usbDongle_StateUsbVolumeSync, likely host OS: Windows or Mac");
            if (GetTaskData()->usb_volume.up )
            {
                if ( volume.value >= GetTaskData()->usb_volume.target )
                {
                    DEBUG_LOG_VERBOSE("usbDongle_StateUsbVolumeSync, volume target met");
                    GetTaskData()->state = VO_INTERNAL_STATE_SINK_VOLUME_SYNC;
                    MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_USB_VOLUME);
                }
            } else
            {
                if ( volume.value <= GetTaskData()->usb_volume.target )
                {
                    DEBUG_LOG_VERBOSE("usbDongle_StateUsbVolumeSync, volume target met");
                    GetTaskData()->state = VO_INTERNAL_STATE_SINK_VOLUME_SYNC;
                    MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_USB_VOLUME);
                }
            }
            break;

        /* Sink is requesting a volume update while we are syncing the USB volume, update target */
        case request_type_sink_to_host:
            usbDongle_UpdateUsbVolumeTarget(volume);
            break;

        default:
            DEBUG_LOG("usbDongle_StateUsbVolumeSync, invalid source");
            break;
    }
}

static void usbDongle_StateSinkVolumeSync(request_type_t request_type, volume_t volume)
{
    switch(request_type)
    {
        /* Host volume changed, update the Sink volume */
        case request_type_host_to_sink:
            usbDongle_UpdateSinkVolume(volume);
            break;

        /* Sink volume changed, update the USB volume, change state */
        case request_type_sink_to_host:
            GetTaskData()->state = VO_INTERNAL_STATE_USB_VOLUME_SYNC;
            GetTaskData()->usb_volume.count = 0;
            usbDongle_UpdateUsbVolumeTarget(volume);
            break;

        default:
            DEBUG_LOG("usbDongle_StateSinkVolumeSync, invalid source");
            break;
    }
}

static void usbDongle_NotifyAudioRoutingChange(audio_source_t source, source_routing_change_t change)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_NotifyAudioRoutingChange, enum:audio_source_t:%d, enum:source_routing_change_t:%d", source, change);
}

static void usbDongle_HandleNotifyVolumeChange(request_type_t request_type, volume_t volume)
{
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        uint16 last_set_sink_volume = GetTaskData()->le_audio_volume.last_sync_sink_volume;

        /* Check if the volume change indication received from sink is due to the set volume command sent
           by the dongle earlier */
        if (request_type == request_type_sink_to_host && last_set_sink_volume != USB_DONGLE_VO_LE_INVALID_LE_AUDIO_VOLUME)
        {
            GetTaskData()->le_audio_volume.last_sync_sink_volume = USB_DONGLE_VO_LE_INVALID_LE_AUDIO_VOLUME;

            if (last_set_sink_volume == volume.value)
            {
                /* Ignore the volume change notification from sink */
                return;
            }
        }
    }

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    switch(GetTaskData()->state)
    {
        case VO_INTERNAL_STATE_SINK_VOLUME_SYNC:
            usbDongle_StateSinkVolumeSync(request_type, volume);
            break;

        case VO_INTERNAL_STATE_USB_VOLUME_SYNC:
            usbDongle_StateUsbVolumeSync(request_type, volume);
            break;

        case VO_INTERNAL_STATE_IDLE:
            DEBUG_LOG("UsbDongle_NotifyVolumeChange, IDLE, no change");
            break;

        default:
            break;
    }
}

static void usbDongle_NotifyVolumeChange(audio_source_t source, event_origin_t origin, volume_t volume)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_NotifyVolumeChange, enum:audio_source_t:%d, enum:event_origin_t:%d, volume %d", source, origin, volume.value);


    if ( origin == event_origin_external )
    {
        usbDongle_HandleNotifyVolumeChange(source == audio_source_usb ? request_type_host_to_sink : request_type_sink_to_host, volume);
    } else
    {
        DEBUG_LOG("usbDongle_NotifyVolumeChange, ignoring origin enum:event_origin_t:%d", origin);
    }
}

static void usbDongle_NotifyVoiceVolumeChange(voice_source_t source, event_origin_t origin, volume_t volume)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_NotifyVoiceVolumeChange, enum:voice_source_t:%d, enum:event_origin_t:%d, volume %d", source, origin, volume.value);

    if ( origin == event_origin_external )
    {
        usbDongle_HandleNotifyVolumeChange(source == voice_source_usb ? request_type_host_to_sink : request_type_sink_to_host, volume);
    } else
    {
        DEBUG_LOG("usbDongle_NotifyVoiceVolumeChange, ignoring origin: enum:event_origin_t:%d", origin);
    }

}

static void usbDongle_NotifyVoiceMuteChange(voice_source_t source, event_origin_t origin, bool mute_state)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_NotifyVoiceMuteChange, enum:voice_source_t:%d, enum:event_origin_t:%d, mute %d", source, origin, mute_state);

}

/* Returns TRUE if volume up/down command can be send to the host, FALSE otherwise */
static bool usbDongle_CheckIfUsbVolumeUpDownCanBeApplied(int16 new_usb_volume)
{
    int current_usb_volume_in_percent;
    bool status = TRUE;

    /* Applicable only when LE is connected */
    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected() && !GetTaskData()->usb_volume.up)
    {
        current_usb_volume_in_percent = VolumeUtils_GetVolumeInPercent(AudioSources_GetVolume(audio_source_usb));

        /* Against windows host, it is seen that sending volume down command when volume is lesser than
           USB_DONGLE_VO_LE_USB_VOLUME_CUTOFF_FOR_VOLUME_DOWN sometimes results in setting the volume
           directly to zero in windows sound bar UI along with a muted symbol. However the sink device
           volume level might still remain as non-zero. So to avoid this, here we make sure the volume
           down command is not sent unless the target usb volume is really zero */
        if (current_usb_volume_in_percent <= USB_DONGLE_VO_LE_USB_VOLUME_CUTOFF_FOR_VOLUME_DOWN &&
            new_usb_volume > 0)
        {
            status = FALSE;
        }
    }

    return status;
}

/*! \brief Send HID Volume Up/Down message depending on host OS. */
static void usbDongle_HandleUpdateUsbVolume(void)
{
    switch(GetTaskData()->host_os)
    {
        /* Send HID message until maximum count or target volume has been met. */
        case HOST_OS_TYPE_WINDOWS_MAC:
            if ( GetTaskData()->usb_volume.count < USB_VOLUME_CHANGE_COUNT_MAX &&
                 usbDongle_CheckIfUsbVolumeUpDownCanBeApplied(GetTaskData()->usb_volume.target))
            {
                GetTaskData()->usb_volume.count++;
                DEBUG_LOG_V_VERBOSE("usbDongle_HandleUpdateUsbVolume, usb_volume.change_count %d", GetTaskData()->usb_volume.count );

                usbDongle_SendHidVolumeMessage();

                /* reschedule */
                MessageSendLater(GetTask(), VO_INTERNAL_UPDATE_USB_VOLUME, NULL, USB_DONGLE_VO_USB_VOLUME_CHANGE_DELAY_MS );
            } else  /* USB host has become unresponsive */
            {
                DEBUG_LOG_ERROR("usbDongle_HandleUpdateUsbVolume, USB host has become unresponsive, stop sending USB HID volume messages.");
                GetTaskData()->state = VO_INTERNAL_STATE_SINK_VOLUME_SYNC;
            }
            break;

        /* Send single HID message per volume change request. */
        case HOST_OS_TYPE_LINUX_ANDROID:
            usbDongle_SendHidVolumeMessage();
            break;

        /* Start a burst of HID messages to identify host OS. */
        case HOST_OS_TYPE_UNIDENTIFIED:
            if ( GetTaskData()->usb_volume.count < BURST_SIZE)
            {
                GetTaskData()->usb_volume.count++;
                DEBUG_LOG_V_VERBOSE("usbDongle_HandleUpdateUsbVolume, usb_volume.change_count %d",GetTaskData()->usb_volume.count);
                usbDongle_SendHidVolumeMessage();
                MessageSendLater(GetTask(), VO_INTERNAL_UPDATE_USB_VOLUME, NULL, USB_DONGLE_VO_USB_VOLUME_CHANGE_DELAY_MS );
            } else
            {
                GetTaskData()->host_os = HOST_OS_TYPE_LINUX_ANDROID;
                DEBUG_LOG_INFO("usbDongle_HandleUpdateUsbVolume, likely host OS: Linux or Android");
                GetTaskData()->state = VO_INTERNAL_STATE_SINK_VOLUME_SYNC;
            }
            break;

        default:
            break;
    }
}

/*
 * Send a HID Volume Up/Down message to the host
 */
static void usbDongle_SendHidVolumeMessage(void)
{
    if ( GetTaskData()->usb_volume.up )
    {
        DEBUG_LOG_VERBOSE("usbDongle_SendHidVolumeMessage, up");
        Volume_SendAudioSourceVolumeIncrementRequest(audio_source_usb, event_origin_local);
    }
    else
    {
        DEBUG_LOG_VERBOSE("usbDongle_SendHidVolumeMessage, down");
        Volume_SendAudioSourceVolumeDecrementRequest(audio_source_usb, event_origin_local);
    }
}

static void usbDongle_HandleUsbAudioConnectedInd(void)
{
    bool is_profiles_connected = FALSE;

    DEBUG_LOG_FN_ENTRY("usbDongle_HandleUsbAudioConnectedInd");

    GetTaskData()->usb_connected = TRUE;

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
    is_profiles_connected =  GetTaskData()->bredr_volume.avrcp_connected || GetTaskData()->bredr_volume.aghfp_connected;
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    is_profiles_connected = is_profiles_connected || GetTaskData()->le_audio_volume.audio_connected;
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    if (is_profiles_connected)
    {
        GetTaskData()->state = VO_INTERNAL_STATE_SINK_VOLUME_SYNC;
    }
}

static void usbDongle_HandleUsbAudioDisconnectedInd(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleUsbAudioDisconnectedInd");
    GetTaskData()->usb_connected = FALSE;
    GetTaskData()->state = VO_INTERNAL_STATE_IDLE;
    MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_USB_VOLUME);
    MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_AVRCP_VOLUME);
    MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_LE_AUDIO_VOLUME);
}

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

/*
 * Throttle the rate at which we send AVRCP absolute volume updates
 * This is done to avoid a memory exhausation situation if the PC host sends us too many volume update messages
 */
static void usbDongle_QueueAvrcpVolumeUpdate(uint8  volume)
{
    int32 dummy;

    DEBUG_LOG_FN_ENTRY("usbDongle_QueueAvrcpVolumeUpdate");

    GetTaskData()->bredr_volume.avrcp_volume_target = volume;
    if ( !MessagePendingFirst(GetTask(), VO_INTERNAL_UPDATE_AVRCP_VOLUME, &dummy) )
    {
        DEBUG_LOG_V_VERBOSE("usbDongle_QueueAvrcpVolumeUpdate, queuing AVRCP volume update");
        MessageSendLater(GetTask(), VO_INTERNAL_UPDATE_AVRCP_VOLUME, NULL, USB_DONGLE_VO_SINK_VOLUME_CHANGE_DELAY_MS);
    }
    else
    {
        DEBUG_LOG_V_VERBOSE("usbDongle_QueueAvrcpVolumeUpdate, not queuing, volume update already pending");
    }
}

/* send the AVRCP volume update */
static void usbDongle_SendAvrcpVolumeUpdate(void)
{
    DEBUG_LOG_V_VERBOSE("usbDongle_SendAvrcpVolumeUpdate, volume %d", GetTaskData()->bredr_volume.avrcp_volume_target);

    if (GetTaskData()->bredr_volume.av_instance != NULL)
    {
        appAvrcpSetAbsoluteVolumeRequest(GetTaskData()->bredr_volume.av_instance, GetTaskData()->bredr_volume.avrcp_volume_target);
    }
}

static void usbDongle_HandleTelephonyMuteActive(TELEPHONY_MUTE_ACTIVE_T *mute)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleTelephonyMuteActive, source enum:voice_source_t:%d", mute->voice_source);

    /* If the microphone is already muted then ignore the message */
    if (GetTaskData()->mic_muted)
    {
        return;
    }

    /* If the message comes from HFP then forward to USB
       If the message comes from USB then forward to HFP
       Note: If the call was on hold we should not set the mute status */
    if (mute->voice_source == voice_source_hfp_1 && !GetTaskData()->bredr_volume.hold_active)
    {
        VoiceSources_ToggleMicrophoneMute(voice_source_usb);
    }
    else if (mute->voice_source == voice_source_usb)
    {
        VoiceSources_ToggleMicrophoneMute(voice_source_hfp_1);
    }

    GetTaskData()->mic_muted = TRUE;
}

static void usbDongle_HandleTelephonyMuteInactive(TELEPHONY_MUTE_INACTIVE_T *mute)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleTelephonyMuteInactive, source enum:voice_source_t:%d", mute->voice_source);

    /* If the microphone is already unmuted then ignore the message */
    if (!GetTaskData()->mic_muted)
    {
        return;
    }

    /* If the message comes from HFP then forward to USB
       If the message comes from USB then forward to HFP
       Note: If the call was on hold we should not set the mute status */
    if (mute->voice_source == voice_source_hfp_1 && !GetTaskData()->bredr_volume.hold_active)
    {
        VoiceSources_ToggleMicrophoneMute(voice_source_usb);
    }
    else if (mute->voice_source == voice_source_usb)
    {
        VoiceSources_ToggleMicrophoneMute(voice_source_hfp_1);
    }

    GetTaskData()->mic_muted = FALSE;
}

static void usbDongle_HandleAvAvrcpConnectedInd(AV_AVRCP_CONNECT_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleAvAvrcpConnectedInd");
    /* Only one connection supported */
    if (!GetTaskData()->bredr_volume.avrcp_connected)
    {
        GetTaskData()->bredr_volume.av_instance = msg->av_instance;
        GetTaskData()->bredr_volume.avrcp_connected = TRUE;
        if ( GetTaskData()->usb_connected )
        {
            GetTaskData()->state = VO_INTERNAL_STATE_SINK_VOLUME_SYNC;
            usbDongle_UpdateSinkVolume(AudioSources_GetVolume(audio_source_usb));
        }
    }
}

static void usbDongle_HandleAvAvrcpDisconnectedInd(AV_AVRCP_DISCONNECT_IND_T *msg)
{
    UNUSED(msg);
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleAvAvrcpDisconnectedInd");
    if (GetTaskData()->bredr_volume.avrcp_connected)
    {
        GetTaskData()->bredr_volume.av_instance  = NULL;
        GetTaskData()->bredr_volume.avrcp_connected = FALSE;
        MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_USB_VOLUME);
        MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_AVRCP_VOLUME);
        GetTaskData()->state = VO_INTERNAL_STATE_IDLE;
    }
}

static void usbDongle_HandleAghfpConnected(APP_AGHFP_CONNECTED_IND_T *ind)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleAghfpConnected");
    /* Only one connection supported */
    if (!GetTaskData()->bredr_volume.aghfp_connected)
    {
        GetTaskData()->bredr_volume.aghfp_instance = ind->instance;
        GetTaskData()->bredr_volume.aghfp_connected = TRUE;
        if ( GetTaskData()->usb_connected )
        {
            GetTaskData()->state = VO_INTERNAL_STATE_SINK_VOLUME_SYNC;
            usbDongle_UpdateSinkVolume(AudioSources_GetVolume(audio_source_usb));
        }
    }
}

static void usbDongle_HandleAghfpDisconnected(APP_AGHFP_DISCONNECTED_IND_T *ind)
{
    UNUSED(ind);
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleAghfpDisconnected");
    if (GetTaskData()->bredr_volume.aghfp_connected)
    {
        GetTaskData()->bredr_volume.aghfp_instance = NULL;
        GetTaskData()->bredr_volume.aghfp_connected = FALSE;
        MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_USB_VOLUME);
        GetTaskData()->state = VO_INTERNAL_STATE_IDLE;
    }
}

static void usbDongle_HandleCallHoldActiveInactive(bool is_hold_active)
{
    /* Ignore hold state messages that do not change state */
    if (GetTaskData()->bredr_volume.hold_active == is_hold_active)
    {
        return;
    }

    GetTaskData()->bredr_volume.hold_active = is_hold_active;

    if (GetTaskData()->bredr_volume.hold_active)
    {
        GetTaskData()->bredr_volume.pre_hold_mute_state = GetTaskData()->mic_muted;
    }
    else
    {
        DEBUG_LOG("usbDongle_HandleAghfpDisconnected, call off hold, setting mute state %d ",
                  GetTaskData()->bredr_volume.pre_hold_mute_state);
        VoiceSources_SetMuteState(voice_source_hfp_1, GetTaskData()->bredr_volume.pre_hold_mute_state ? mute : unmute);
        /* The sink has changed the mute status while we were on hold.
           toggle the mute so that it reverts to pre hold state */

        /* Set the mic status to the old state */
        GetTaskData()->mic_muted = GetTaskData()->bredr_volume.pre_hold_mute_state;
    }
}

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

/* if the USB Headset had been muted/unmuted, we pass it straight through to HFP */
static void usbDongle_HandleUsbHeadsetMuteStatus(bool mic_muted)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleUsbHeadsetMuteStatus %d", mic_muted);

    if(GetTaskData()->mic_muted != mic_muted)
    {
        GetTaskData()->mic_muted = mic_muted;

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
        if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
        {
            VoiceSources_ToggleMicrophoneMute(voice_source_hfp_1);
        }
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
        if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
        {
            if(GetTaskData()->le_audio_volume.audio_vbc_active)
            {
                /* TODO: update le audio client */
            }
        }
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
    }
}

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

/*
 * Throttle the rate at which we send LE Audio absolute volume updates
 * This is done to avoid a memory exhausation situation if the PC host sends us too many volume update messages
 */
static void usbDongle_LeaVolumeQueueLeAudioVolumeUpdate(uint8  volume)
{
    int32 dummy;

    DEBUG_LOG_VERBOSE("usbDongle_LeaVolumeQueueLeAudioVolumeUpdate");

    GetTaskData()->le_audio_volume.volume_target = volume;
    if(!MessagePendingFirst(GetTask(), VO_INTERNAL_UPDATE_LE_AUDIO_VOLUME, &dummy))
    {
        DEBUG_LOG_VERBOSE("usbDongle_LeaVolumeQueueLeAudioVolumeUpdate, queuing LEA volume update");
        MessageSendLater(GetTask(), VO_INTERNAL_UPDATE_LE_AUDIO_VOLUME, NULL, USB_DONGLE_VO_SINK_VOLUME_CHANGE_DELAY_MS);
    }
    else
    {
        DEBUG_LOG_VERBOSE("usbDongle_LeaVolumeQueueLeAudioVolumeUpdate, not queuing, volume update already pending");
    }
}

/* send the LE Audio volume update */
static void usbDongle_LeaVolumeHandleUpdateLeAudioVolume(void)
{
    DEBUG_LOG_VERBOSE("usbDongle_LeaVolumeHandleUpdateLeAudioVolume, volume %d", GetTaskData()->le_audio_volume.volume_target);

    if (GetTaskData()->le_audio_volume.audio_group_handle)
    {
        LeAudioClient_SetAbsoluteVolume(GetTaskData()->le_audio_volume.audio_group_handle, GetTaskData()->le_audio_volume.volume_target);
        GetTaskData()->le_audio_volume.last_sync_sink_volume = GetTaskData()->le_audio_volume.volume_target;
    }
}

static void usbDongle_LeaVolumeUpdateSinkVolume(volume_t volume)
{
    if(GetTaskData()->le_audio_volume.audio_connected)
    {
        volume_t old_volume = AudioSources_GetVolume(audio_source_le_audio_unicast_sender);
        int16 new_volume;

        new_volume = VolumeUtils_ConvertToVolumeConfig(volume, usb_dongle_observer_le_audio_config);
        DEBUG_LOG_VERBOSE("usbDongle_LeaVolumeUpdateSinkVolume, USB %d, old %d, new %d",
                          volume.value, old_volume.value, new_volume);
        usbDongle_LeaVolumeQueueLeAudioVolumeUpdate(new_volume);
    }
    else
    {
        DEBUG_LOG_VERBOSE("usbDongle_LeaVolumeUpdateSinkVolume, no LE Audio connection");
    }
}

static void usbDongle_LeaVolumeHandleLeAudioConnected(LE_AUDIO_CLIENT_CONNECT_IND_T *msg)
{
    DEBUG_LOG_VERBOSE("usbDongle_LeaVolumeHandleLeAudioConnected");

    if (msg->status == LE_AUDIO_CLIENT_STATUS_SUCCESS)
    {
        PanicFalse(GetTaskData()->le_audio_volume.audio_group_handle == 0);
        GetTaskData()->le_audio_volume.audio_group_handle = msg->group_handle;
    }
}

static void usbDongle_LeaVolumeHandleLeAudioDisconnected(LE_AUDIO_CLIENT_DISCONNECT_IND_T *msg)
{
    DEBUG_LOG_VERBOSE("usbDongle_LeaVolumeHandleLeAudioDisconnected");

    UNUSED(msg);
    GetTaskData()->le_audio_volume.audio_group_handle = 0;

    if(GetTaskData()->le_audio_volume.audio_connected)
    {
        GetTaskData()->le_audio_volume.audio_connected = FALSE;
        MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_USB_VOLUME);
        MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_LE_AUDIO_VOLUME);
        GetTaskData()->state = VO_INTERNAL_STATE_IDLE;
    }
}

static void usbDongle_LeaVolumeHandleLeAudioStreamConnected(LE_AUDIO_CLIENT_STREAM_START_IND_T *msg)
{
    DEBUG_LOG_VERBOSE("usbDongle_LeaVolumeHandleLeAudioStreamConnected");

    PanicFalse(GetTaskData()->le_audio_volume.audio_group_handle == msg->group_handle);

    if(msg->status != LE_AUDIO_CLIENT_STATUS_SUCCESS)
    {
        DEBUG_LOG_ERROR("usbDongle_LeaVolumeHandleLeAudioStreamConnected, unicast stream start failed");
        return;
    }

    if(msg->audio_context == CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC)
    {
        GetTaskData()->le_audio_volume.audio_vbc_active = TRUE;
    }

    /* Only one stream connection supported */
    if(msg->audio_context != CAP_CLIENT_CONTEXT_TYPE_PROHIBITED)
    {
        if(!GetTaskData()->le_audio_volume.audio_connected)
        {
            GetTaskData()->le_audio_volume.audio_connected = TRUE;
            if(GetTaskData()->usb_connected)
            {
                GetTaskData()->state = VO_INTERNAL_STATE_SINK_VOLUME_SYNC;
                usbDongle_LeaVolumeUpdateSinkVolume(AudioSources_GetVolume(audio_source_usb));
            }
        }
    }
}

static void usbDongle_LeaVolumeHandleLeAudioStreamDisconnected(LE_AUDIO_CLIENT_STREAM_STOP_IND_T *msg)
{
    DEBUG_LOG_VERBOSE("usbDongle_LeaVolumeHandleLeAudioStreamDisconnected");

    UNUSED(msg);
    PanicFalse(GetTaskData()->le_audio_volume.audio_group_handle == msg->group_handle);
    GetTaskData()->le_audio_volume.audio_vbc_active = FALSE;

    /* Only one stream connection supported */
    if(GetTaskData()->le_audio_volume.audio_connected)
    {
        GetTaskData()->le_audio_volume.audio_connected = FALSE;
        MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_USB_VOLUME);
        MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_LE_AUDIO_VOLUME);
        GetTaskData()->state = VO_INTERNAL_STATE_IDLE;
    }

}

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

static void usbDongle_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch(id)
    {
        case VO_INTERNAL_UPDATE_USB_VOLUME:
            usbDongle_HandleUpdateUsbVolume();
            break;

        case USB_AUDIO_CONNECTED_IND:
            usbDongle_HandleUsbAudioConnectedInd();
            break;

        case USB_AUDIO_DISCONNECTED_IND:
            usbDongle_HandleUsbAudioDisconnectedInd();
            break;

        case USB_AUDIO_HEADSET_MIC_MUTE_ACTIVE:
            usbDongle_HandleUsbHeadsetMuteStatus(TRUE);
            break;

        case USB_AUDIO_HEADSET_MIC_MUTE_INACTIVE:
            usbDongle_HandleUsbHeadsetMuteStatus(FALSE);
            break;

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

        case TELEPHONY_MUTE_ACTIVE:
            usbDongle_HandleTelephonyMuteActive((TELEPHONY_MUTE_ACTIVE_T*)message);
            break;

        case TELEPHONY_HOLD_ACTIVE:
                usbDongle_HandleCallHoldActiveInactive(TRUE);
            break;

        case TELEPHONY_HOLD_INACTIVE:
                usbDongle_HandleCallHoldActiveInactive(FALSE);
            break;

        case TELEPHONY_MUTE_INACTIVE:
            usbDongle_HandleTelephonyMuteInactive((TELEPHONY_MUTE_INACTIVE_T*)message);
            break;

        case VO_INTERNAL_UPDATE_AVRCP_VOLUME:
            usbDongle_SendAvrcpVolumeUpdate();
            break;

        case APP_AGHFP_CONNECTED_IND:
            usbDongle_HandleAghfpConnected((APP_AGHFP_CONNECTED_IND_T*)message);
            break;

        case APP_AGHFP_DISCONNECTED_IND:
            usbDongle_HandleAghfpDisconnected((APP_AGHFP_DISCONNECTED_IND_T*)message);
            break;

        case AV_AVRCP_CONNECTED_IND:
            usbDongle_HandleAvAvrcpConnectedInd((AV_AVRCP_CONNECT_IND_T *) message);
        break;

        case AV_AVRCP_DISCONNECTED_IND:
            usbDongle_HandleAvAvrcpDisconnectedInd((AV_AVRCP_DISCONNECT_IND_T *) message);
        break;

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

        case VO_INTERNAL_UPDATE_LE_AUDIO_VOLUME:
            usbDongle_LeaVolumeHandleUpdateLeAudioVolume();
            break;

        case LE_AUDIO_CLIENT_CONNECT_IND:
            usbDongle_LeaVolumeHandleLeAudioConnected((LE_AUDIO_CLIENT_CONNECT_IND_T *) message);
            break;

        case LE_AUDIO_CLIENT_DISCONNECT_IND:
            usbDongle_LeaVolumeHandleLeAudioDisconnected((LE_AUDIO_CLIENT_DISCONNECT_IND_T *) message);
            break;

        case LE_AUDIO_CLIENT_STREAM_START_IND:
            usbDongle_LeaVolumeHandleLeAudioStreamConnected((LE_AUDIO_CLIENT_STREAM_START_IND_T *) message);
            break;

        case LE_AUDIO_CLIENT_STREAM_STOP_IND:
            usbDongle_LeaVolumeHandleLeAudioStreamDisconnected((LE_AUDIO_CLIENT_STREAM_STOP_IND_T *) message);
            break;

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

        default:
            DEBUG_LOG("usbDongle_HandleMessage: unhandled %d", id);
            break;
    }
}


bool UsbDongle_VolumeObserverInit(Task init_task)
{
    volume_t mid_volume;

    DEBUG_LOG_FN_ENTRY("UsbDongle_VolumeObserverInit");

    UNUSED(init_task);
    GetTaskData()->task.handler = usbDongle_HandleMessage;

    GetTaskData()->usb_connected = FALSE;
    GetTaskData()->state = VO_INTERNAL_STATE_IDLE;
    GetTaskData()->mic_muted = FALSE;
    GetTaskData()->host_os = HOST_OS_TYPE_UNIDENTIFIED;

    /* Old volume initialised to midpoint of USB volume range */
    GetTaskData()->old_volume.config = usb_dongle_observer_usb_config;
    GetTaskData()->old_volume.value = (usb_dongle_observer_usb_config.range.max -
                                       usb_dongle_observer_usb_config.range.min) / 2;;

    AudioSources_RegisterObserver(audio_source_usb, &usb_dongle_observer_interface);

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    GetTaskData()->bredr_volume.aghfp_connected = FALSE;
    GetTaskData()->bredr_volume.avrcp_connected = FALSE;
    GetTaskData()->bredr_volume.av_instance = NULL;
    GetTaskData()->bredr_volume.aghfp_instance = NULL;
    GetTaskData()->bredr_volume.hold_active = FALSE;
    GetTaskData()->bredr_volume.pre_hold_mute_state = FALSE;

    AudioSources_RegisterObserver(audio_source_a2dp_1, &usb_dongle_observer_interface);
    VoiceSources_RegisterObserver(voice_source_hfp_1, &usb_dongle_voice_observer_interface);
    appAvStatusClientRegister(GetTask());
    AghfpProfile_RegisterStatusClient(GetTask());

    /* Mapping current host volume to the middle of the AVRCP volume range by setting the AVRCP volume to middle of its range. */
    mid_volume.config = usb_dongle_observer_avrcp_config;
    mid_volume.value = (usb_dongle_observer_avrcp_config.range.max - usb_dongle_observer_avrcp_config.range.min) / 2;
    AudioSources_SetVolume(audio_source_a2dp_1,mid_volume);

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

    GetTaskData()->le_audio_volume.audio_connected = FALSE;
    GetTaskData()->le_audio_volume.audio_vbc_active = FALSE;
    GetTaskData()->le_audio_volume.audio_group_handle = 0;
    GetTaskData()->le_audio_volume.last_sync_sink_volume = USB_DONGLE_VO_LE_INVALID_LE_AUDIO_VOLUME;

    /* Mapping current host volume to the middle of the LE Audio volume range by
     * setting the LE Audio volume to middle of its range. */
    mid_volume.config = usb_dongle_observer_le_audio_config;
    mid_volume.value = (usb_dongle_observer_le_audio_config.range.max - usb_dongle_observer_le_audio_config.range.min) / 2;
    AudioSources_SetVolume(audio_source_le_audio_unicast_sender, mid_volume);

    AudioSources_RegisterObserver(audio_source_le_audio_unicast_sender, &usb_dongle_observer_interface);
    VoiceSources_RegisterObserver(voice_source_le_audio_unicast_1, &usb_dongle_voice_observer_interface);
    LeAudioClient_ClientRegister(GetTask());

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

    UsbAudio_ClientRegister(GetTask(), USB_AUDIO_REGISTERED_CLIENT_STATUS);
    Telephony_RegisterForMessages(GetTask());

    return TRUE;
}

void UsbDongle_VolumeObserverSetAbsoluteSinkVolume(uint8 volume)
{
#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

    if (GetTaskData()->le_audio_volume.audio_group_handle)
    {
        LeAudioClient_SetAbsoluteVolume(GetTaskData()->le_audio_volume.audio_group_handle, volume);

        /* Cancel any queued message to set LE audio volume as volume is now updated with a user
           requested value. */
        MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_LE_AUDIO_VOLUME);
    }

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (GetTaskData()->bredr_volume.av_instance != NULL)
    {
        appAvrcpSetAbsoluteVolumeRequest(GetTaskData()->bredr_volume.av_instance, volume);

        /* Cancel any queued message to set AVRCP audio volume as volume is now updated with a user
           requested value. */
        MessageCancelFirst(GetTask(), VO_INTERNAL_UPDATE_AVRCP_VOLUME);
    }

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */
}
#endif /* defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) || defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) */

