/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Charger case application state machine.

*/
#include "usb_dongle_logging.h"

#include "usb_dongle_sm.h"

/* local includes */
#include "usb_dongle_audio.h"
#include "usb_dongle_config.h"
#include "usb_dongle_led.h"
#include "usb_dongle_sm_private.h"
#include "usb_dongle_voice.h"

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)
#include "usb_dongle_a2dp.h"
#include "usb_dongle_hfp.h"
#include "usb_dongle_inquiry.h"
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#if defined(INCLUDE_SOURCE_APP_LE_AUDIO)
#include "usb_dongle_lea.h"
#include "usb_dongle_le_voice.h"
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

/* framework includes */
#include <av.h>
#include <audio_sources.h>
#include <connection_manager.h>
#include <device_db_serialiser.h>
#include <pairing.h>
#include <power_manager.h>
#include <system_reboot.h>
#include <system_state.h>
#include <ui.h>
#include <unexpected_message.h>
#include <usb_audio.h>
#include <usb_device.h>
#include <wired_audio_source.h>
#include <device_properties.h>
#include <bredr_scan_manager.h>
#include <rssi_pairing.h>
#include <sink_service.h>
#include <profile_manager.h>
#include <kymera.h>
#include <led_manager.h>

/* system includes */
#include <connection.h>
#include <device_list.h>
#include <message.h>
#include <panic.h>
#include <vmtypes.h>
#include <ps_key_map.h>
#include <ps.h>
#include <pairing.h>
#include <telephony_messages.h>
#include <usb_source_hid.h>
#ifdef USE_SYNERGY
#include <csr_bt_td_db.h>
#endif

/*! \name Connection library factory reset keys

    These keys should be deleted during a factory reset.
*/
/*! @{ */
#ifdef USE_SYNERGY
#define CSR_BT_PS_KEY_SYSTEM  100
#else
#define ATTRIBUTE_BASE_PSKEY_INDEX  100
#define TDL_BASE_PSKEY_INDEX        142
#define TDL_INDEX_PSKEY             141
#define TDL_SIZE                    BtDevice_GetMaxTrustedDevices()
#endif
/*! @} */

#define usbDongleSmIsAclConnectedState(state) ((state == APP_STATE_CONNECTED) || \
                                                (state == APP_STATE_AUDIO_STARTING) || \
                                                (state == APP_STATE_AUDIO_STREAMING) || \
                                                (state == APP_STATE_AUDIO_STOPPING) || \
                                                (state == APP_STATE_VOICE_STARTING) || \
                                                (state == APP_STATE_VOICE_STOPPING) || \
                                                (state == APP_STATE_VOICE_STREAMING))

/*! \brief Check whether USB dongle is in audio streaming state or not */
#define usbDongleSmIsAudioStreamingState(state) ((state == APP_STATE_AUDIO_STARTING) || \
                                                 (state == APP_STATE_AUDIO_STREAMING))

/*! \brief UI Inputs the state machine is interested in. */
const message_group_t sm_ui_inputs[] =
{
    UI_INPUTS_HANDSET_MESSAGE_GROUP,
    UI_INPUTS_DEVICE_STATE_MESSAGE_GROUP,
    UI_INPUTS_GAMING_MODE_MESSAGE_GROUP
};

/*! \brief Application state machine task data structure. */
typedef struct
{
    TaskData task;                                  /*!< SM task */
    usb_dongle_state_t state;                       /*!< Application state */

    device_t connected_sink;
    bdaddr hold_pairing_acl_device_address;

    bool clear_pairing_requested;
    bool factory_reset_requested;
    bool disconnect_on_suspend;
    bool hold_active;
    bool usb_hid_call_active;
    bool usb_hid_call_incoming;
    bool transport_switch_in_progress;              /*!< TRUE if a transport switch is in progress */
} usb_dongle_sm_data_t;

/*! Application state machine task data instance. */
usb_dongle_sm_data_t usb_dongle_sm;

/*! Get pointer to application state machine task data. */
#define usbDongleSmGetTaskData() (&usb_dongle_sm)

static void usbDongleSetState(usb_dongle_state_t new_state);
static void usbDongleSmRescanAudioInputs(void);
static unsigned usbDongleSm_GetApplicationCurrentContext(void);
static void usbDongle_HandlePairingCfm(bool success, const bdaddr *bd_addr);
static void usbDongleSm_ClearPairing(void);
static void usbDongleSm_FactoryReset(void);
static device_t usbDongleSmDetermineSinkDevice(void);
static sink_service_mode_t usbDongleSm_GetSinkServiceModeToUse(usb_dongle_transport_mode_t transport_mode,
                                                               usb_dongle_audio_mode_t audio_mode);

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) && defined(INCLUDE_SOURCE_APP_LE_AUDIO)

/*! \brief Disconnect current transport as part of switching to a new transport */
static void usbDongleSm_DisconnectCurrentTransport(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSm_DisconnectCurrentTransport");

    usbDongleSetState(APP_STATE_DISCONNECTING);
    usbDongleSmGetTaskData()->transport_switch_in_progress = TRUE;

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

    if (UsbDongle_IsLeaBroadcastModeActive())
    {
        /* Disconnecting will not stop broadcast streaming. So explicitly call stop streaming */
        UsbDongle_LeaAudioStop();
    }

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
}

/*! \brief Reconnect with new transport to complete the transport switch */
static void usbDongleSm_ReconnectTransportIfRequired(void)
{
    if (usbDongleSmGetTaskData()->transport_switch_in_progress)
    {
        DEBUG_LOG_FN_ENTRY("usbDongleSm_ReconnectTransportIfRequired");

        usbDongleSmGetTaskData()->transport_switch_in_progress = FALSE;

        /* Disconnection complete as part of transport switch. Now save the new transport mode */
        (void) UsbDongleConfig_SaveMode(usbDongleConfig_GetAudioMode(), usbDongleConfig_GetNewTransportMode());

        /* Connect profiles (which will take care of starting audio) */
        usbDongleSetState(APP_STATE_CONNECTING);
    }
}

#else /* defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) && defined(INCLUDE_SOURCE_APP_LE_AUDIO) */

#define usbDongleSm_DisconnectCurrentTransport()
#define usbDongleSm_ReconnectTransportIfRequired()

#endif /* defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) && defined(INCLUDE_SOURCE_APP_LE_AUDIO) */

static void usbDongleSmPrintConnectedInputs(void)
{
    DEBUG_LOG_VERBOSE("usbDongleSmPrintConnectedInputs, connected Inputs:");

    if(UsbDongle_AudioInputIsConnected(usb_dongle_audio_input_usb))
    {
        DEBUG_LOG_VERBOSE("  usb_dongle_audio_input_usb");
    }

    if(UsbDongle_AudioInputIsConnected(usb_dongle_audio_input_analogue))
    {
        DEBUG_LOG_VERBOSE("  usb_dongle_audio_input_analogue");
    }

    if(UsbDongle_AudioInputIsConnected(usb_dongle_audio_input_usb_voice))
    {
        DEBUG_LOG_VERBOSE("  usb_dongle_audio_input_usb_voice");
    }

    if(UsbDongle_AudioInputIsConnected(usb_dongle_audio_input_sco_voice))
    {
        DEBUG_LOG_VERBOSE("  usb_dongle_audio_input_sco_voice");
    }
}

/*! \brief Enter Idle state.
 */
static void usbDongleEnterIdle(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleEnterIdle");
}

/*! \brief Exit Idle state.
 */
static void usbDongleExitIdle(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleExitIdle");
}

/*! \brief Enter Idle state.
 */
static void usbDongleEnterPairing(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleEnterPairing");
}

/*! \brief Exit Idle state.
 */
static void usbDongleExitPairing(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleExitPairing");
}

/*! \brief Enter Connecting state.
 */
static void usbDongleEnterConnecting(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleEnterConnecting");

    if (!SinkService_Connect())
    {
        /* If the call to Connect fails then check if the sink service is already connected.
         * Move to connected if that is the case */
        if (SinkService_IsConnected())
        {
            usbDongleSetState(APP_STATE_CONNECTED);
        }
        else
        {
            usbDongleSetState(APP_STATE_IDLE);

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
            if (usbDongleConfig_IsInBroadcastAudioMode())
            {
                /* Connect may fail if MRU device does not support LE and we are in broadcast mode.
                   In such scenario, we still like to start the broadcast streaming (source only) if a
                   media is active */
                UsbDongle_LeaHandleBroadcastModeToggle(FALSE);
            }
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
        }
    }
}

/*! \brief Exit Connecting state.
 */
static void usbDongleExitConnecting(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleExitConnecting");
}

/*! \brief Enter Connected state.
 */
static void usbDongleEnterConnected(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleEnterConnected");
    usbDongleSmRescanAudioInputs();
}

/*! \brief Exit Connected state.
 */
static void usbDongleExitConnected(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleExitConnected");
}

/*! \brief Enter Audio Starting state.
 */
static void usbDongleEnterAudioStarting(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleEnterAudioStarting");
    UsbDongle_AudioStreamConnect();
}

/*! \brief Exit Audio Starting state.
 */
static void usbDongleExitAudioStarting(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleExitAudioStarting");
}

/*! \brief Enter Audio Streaming state.
 */
static void usbDongleEnterAudioStreaming(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleEnterAudioStreaming");

    /* Start audio, if not already started */
    if (!UsbDongle_AudioIsActive())
    {
        UsbDongle_AudioStart(NULL);
    }
}

/*! \brief Exit Audio Streaming state.
 */
static void usbDongleExitAudioStreaming(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleExitAudioStreaming");
}

/*! \brief Enter Audio Stopping state.
 */
static void usbDongleEnterAudioStopping(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleEnterAudioStopping");

    /* Cancel any pending request to restart the audio graph as streaming is getting stopped */
    MessageCancelAll(UsbDongleSmGetTask(), SM_INTERNAL_AUDIO_GRAPH_RESTART);

    UsbDongle_AudioStreamDisconnect();
}

/*! \brief Exit Audio Stopping state.
 */
static void usbDongleExitAudioStopping(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleExitAudioStopping");
}

/*! \brief Enter Disconnecting state.
 */
static void usbDongleEnterDisconnecting(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleEnterDisconnecting, requesting profile disconnection");
    SinkService_DisconnectAll();
}

/*! \brief Exit Disconnecting state.
 */
static void usbDongleExitDisconnecting(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleExitDisconnecting");
}

/*! \brief Enter Voice starting state.
 */
static void usbDongleEnterVoiceStarting(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleEnterVoiceStarting");

    if(UsbDongle_VoiceStreamConnect())
    {
        /* Voice already connected, start chains immediately. */
        usbDongleSetState(APP_STATE_VOICE_STREAMING);
    }
}

/*! \brief Enter Voice starting state.
 */
static void usbDongleExitVoiceStarting(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleExitVoiceStarting");
}

/*! \brief Enter Voice stopping state.
 */
static void usbDongleEnterVoiceStopping(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleEnterVoiceStopping");

    if(UsbDongle_VoiceStreamDisconnect())
    {
        /* Voice already disconnected, nothing to do. */
        usbDongleSetState(APP_STATE_CONNECTED);
    }
}

/*! \brief Exit Voice stopping state.
 */
static void usbDongleExitVoiceStopping(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleExitVoiceStopping");
}

/*! \brief Enter voice streaming state.
 */
static void usbDongleEnterVoiceStreaming(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleEnterVoiceStreaming");
    UsbDongle_VoiceStart();
}

/*! \brief Exit voice streaming state.
 */
static void usbDongleExitVoiceStreaming(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleExitVoiceStreaming");
    UsbDongle_VoiceStop();
}

/*! \brief Provides application state machine context changes to the User Interface module.

    \param[in]  void

    \return     current_sm_ctxt - current application context of sm module.
*/
static unsigned usbDongleSm_GetApplicationCurrentContext(void)
{
    sm_provider_context_t context = BAD_CONTEXT;

    switch(UsbDongleSm_GetState())
    {
        case APP_STATE_INIT:
            context = context_app_sm_inactive;
            break;

        case APP_STATE_IDLE:
            context = context_app_sm_idle;
            break;

        case APP_STATE_PAIRING:
            context = context_app_sm_pairing;
            break;

        case APP_STATE_CONNECTING:
            context = context_app_sm_connecting;
            break;

        case APP_STATE_CONNECTED:
        case APP_STATE_DISCONNECTING:
        case APP_STATE_AUDIO_STARTING:
        case APP_STATE_VOICE_STARTING:
            context = context_app_sm_connected;
            break;

        case APP_STATE_AUDIO_STREAMING:
        case APP_STATE_AUDIO_STOPPING:
        case APP_STATE_VOICE_STREAMING:
        case APP_STATE_VOICE_STOPPING:
            context = context_app_sm_streaming;
            break;

        default:
            break;
    }

    return (unsigned)context;
}

static void usbDongleSm_ClearPairing(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongleSm_ClearPairing");
    BtDevice_DeleteAllDevicesOfType(DEVICE_TYPE_HANDSET);
    BtDevice_DeleteAllDevicesOfType(DEVICE_TYPE_SINK);
    ConnectionSmDeleteAllAuthDevices(0);
}

static void UsbDongleSm_ClearPsStore(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongleSm_ClearPsStore");

#ifdef USE_SYNERGY
    CsrBtTdDbDeleteAll(CSR_BT_TD_DB_FILTER_EXCLUDE_NONE);
    PsStore(CSR_BT_PS_KEY_SYSTEM, NULL, 0);
#else
    for (int i=0; i<TDL_SIZE; i++)
    {
        PsStore(ATTRIBUTE_BASE_PSKEY_INDEX+i, NULL, 0);
        PsStore(TDL_BASE_PSKEY_INDEX+i, NULL, 0);
    }

    PsStore(TDL_INDEX_PSKEY, NULL, 0);
#endif /* USE_SYNERGY */
#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
    PsStore(PS_KEY_HFP_CONFIG, NULL, 0);
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */
}

/*! \brief Delete pairing and reboot device. */
static void usbDongleSm_FactoryReset(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongleSm_FactoryReset");
    usbDongleSm_ClearPairing();
    UsbDongleSm_ClearPsStore();
    DEBUG_LOG_INFO("usbDongleSm_FactoryReset, factory reset complete, rebooting now");
    SystemReboot_Reboot();
}

/* This function is called to change the applications state, it automatically
   calls the entry and exit functions for the new and old states.
*/
static void usbDongleSetState(usb_dongle_state_t new_state)
{
    usb_dongle_state_t previous_state = UsbDongleSm_GetState();

    DEBUG_LOG_STATE("usbDongleSetState, enum:usb_dongle_state_t:%d -> "
                    "enum:usb_dongle_state_t:%d", previous_state, new_state);

    usbDongleSmPrintConnectedInputs();

    /* Handle state exit functions */
    switch (previous_state)
    {
        case APP_STATE_INIT:
            break;

        case APP_STATE_IDLE:
            usbDongleExitIdle();
            break;

        case APP_STATE_PAIRING:
            usbDongleExitPairing();
            break;

        case APP_STATE_CONNECTING:
            usbDongleExitConnecting();
            break;

        case APP_STATE_CONNECTED:
            usbDongleExitConnected();
            break;

        case APP_STATE_AUDIO_STARTING:
            usbDongleExitAudioStarting();
            break;

        case APP_STATE_AUDIO_STREAMING:
            usbDongleExitAudioStreaming();
            break;

        case APP_STATE_AUDIO_STOPPING:
            usbDongleExitAudioStopping();
            break;

        case APP_STATE_DISCONNECTING:
            usbDongleExitDisconnecting();
            break;

        case APP_STATE_VOICE_STARTING:
            usbDongleExitVoiceStarting();
            break;

        case APP_STATE_VOICE_STOPPING:
            usbDongleExitVoiceStopping();
            break;

        case APP_STATE_VOICE_STREAMING:
            usbDongleExitVoiceStreaming();
            break;

        default:
            DEBUG_LOG_ERROR("usbDongleSetState, attempted to exit unsupported state "
                            "enum:usb_dongle_state_t:0x%02x", previous_state);
            Panic();
            break;
    }

    /* Set new state */
    usbDongleSmGetTaskData()->state = new_state;

    /* Handle state entry functions */
    switch (new_state)
    {
        case APP_STATE_IDLE:
            usbDongleEnterIdle();
            break;

        case APP_STATE_PAIRING:
            usbDongleEnterPairing();
            break;

        case APP_STATE_CONNECTING:
            usbDongleEnterConnecting();
            break;

        case APP_STATE_CONNECTED:
            usbDongleEnterConnected();
            break;

        case APP_STATE_AUDIO_STARTING:
            usbDongleEnterAudioStarting();
            break;

        case APP_STATE_AUDIO_STREAMING:
            usbDongleEnterAudioStreaming();
            break;

        case APP_STATE_AUDIO_STOPPING:
            usbDongleEnterAudioStopping();
            break;

        case APP_STATE_DISCONNECTING:
            usbDongleEnterDisconnecting();
            break;

        case APP_STATE_VOICE_STARTING:
            usbDongleEnterVoiceStarting();
            break;

        case APP_STATE_VOICE_STOPPING:
            usbDongleEnterVoiceStopping();
            break;

        case APP_STATE_VOICE_STREAMING:
            usbDongleEnterVoiceStreaming();
            break;

        default:
            DEBUG_LOG_ERROR("usbDongleSetState, attempted to enter unsupported state "
                            "enum:usb_dongle_state_t:0x%02x", new_state);
            Panic();
            break;
    }

    Ui_InformContextChange(ui_provider_app_sm,
                           usbDongleSm_GetApplicationCurrentContext());
}


static void usbDongleSmReleasePairingAcl(void)
{
    if (!BdaddrIsZero(&usbDongleSmGetTaskData()->hold_pairing_acl_device_address))
    {
        DEBUG_LOG_DEBUG("usbDongleSmReleasePairingAcl, release pairing ACL");
        ConManagerReleaseAcl(&usbDongleSmGetTaskData()->hold_pairing_acl_device_address);
        BdaddrSetZero(&usbDongleSmGetTaskData()->hold_pairing_acl_device_address);
    }
}

static device_t usbDongleSmDetermineSinkDevice(void)
{
    device_t sink = NULL;

    /* Try most recently used (MRU) device first */
    sink = BtDevice_GetMruDevice();

    if (!sink)
    {
        /* No MRU device, fall back to first entry on the device list instead */
        deviceType type = DEVICE_TYPE_SINK;
        sink = DeviceList_GetFirstDeviceWithPropertyValue(device_property_type,
                                                          &type, sizeof(deviceType));
    }
    return sink;
}

/*! \brief Send internal message. */
static void usbDongleSmSendInternalMsg(enum sm_internal_message_ids id)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmSendInternalMsg id:%d", id);
    MessageSend(UsbDongleSmGetTask(), id, NULL);
}

/*! \brief Cancel and send rescan audio inputs message after a delay. */
static void usbDongleSmRescanAudioInputs(void)
{
    MessageCancelAll(UsbDongleSmGetTask(), SM_INTERNAL_RESCAN_AUDIO_INPUTS);
    MessageSendLater(UsbDongleSmGetTask(), SM_INTERNAL_RESCAN_AUDIO_INPUTS, NULL, USB_DONGLE_SM_RESCAN_INPUTS_DELAY);
}

/*! \brief Rescan available audio inputs.

    Connect/disconnect, suspend/resume, or switch as required.
*/
static void usbDongleSmHandleInternalRescanAudioInputs(void)
{
    usb_dongle_state_t current_state = UsbDongleSm_GetState();

    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleInternalRescanAudioInputs, current_state:"
                       " enum:usb_dongle_state_t:%d", current_state);

    MessageCancelAll(UsbDongleSmGetTask(), SM_INTERNAL_RESCAN_AUDIO_INPUTS);

    if (UsbDongle_AudioDetermineNewSource() == audio_source_none &&
        UsbDongle_VoiceDetermineNewSource() == voice_source_none)
    {
        DEBUG_LOG_DEBUG("usbDongleSmHandleInternalRescanAudioInputs, no remaining audio sources");

        if (UsbDevice_IsConnectedToHost())
        {
            switch (current_state)
            {
                case APP_STATE_AUDIO_STARTING:
                case APP_STATE_AUDIO_STREAMING:
                {
                    usbDongleSetState(APP_STATE_AUDIO_STOPPING);
                }
                break;
                case APP_STATE_VOICE_STARTING:
                case APP_STATE_VOICE_STREAMING:
                {
                    usbDongleSetState(APP_STATE_VOICE_STOPPING);
                }
                break;
                case APP_STATE_VOICE_STOPPING:
                case APP_STATE_AUDIO_STOPPING:
                {
                    /* Still waiting for audio/voice chain to be torn down. */
                    /* Do nothing. */
                }
                break;
                case APP_STATE_CONNECTED:
                {
                    DEBUG_LOG_DEBUG("usbDongleSmHandleInternalRescanAudioInputs, USB still attached, stay connected");
                    /* Do nothing. */
                }
                break;
                default:
                break;
            }
        }
        else /* USB detached, no audio sources. Disconnect profiles. */
        {
            if (usbDongleSmIsAclConnectedState(current_state))
            {
                usbDongleSetState(APP_STATE_DISCONNECTING);
                /* Disconnecting will take care of stopping audio. */
            }
        }
    }
    else /* There are still audio or voice sources connected. */
    {
        switch (current_state)
        {
            case APP_STATE_IDLE:
                if (usbDongleConfig_IsPairingDisallowed())
                {
                    /* Pairing is disabllowed in non colocated broadcast mode, remain in idle state */
                }
                else
                {
                    /* Connect profiles (which will take care of starting audio) */
                    usbDongleSetState(APP_STATE_CONNECTING);
                }
                break;

            case APP_STATE_DISCONNECTING:
                /* If disconnect is due to a transport switch, wait for the disconnect to complete
                   before trying to connect again */
                if (!usbDongleSmGetTaskData()->transport_switch_in_progress)
                {
                    /* Cancel the pending disconnect by requesting a connection */
                    usbDongleSetState(APP_STATE_CONNECTING);
                }
                break;

            case APP_STATE_VOICE_STARTING:
            case APP_STATE_VOICE_STREAMING:
                if(UsbDongle_VoiceDetermineNewSource() != voice_source_usb)
                {
                    /* Only stop streaming voice (and consequently stop SCO streaming)
                       if the call is not active and we are not on hold */
                    if (!usbDongleSmGetTaskData()->usb_hid_call_active &&
                        !usbDongleSmGetTaskData()->hold_active)
                    {
                        usbDongleSetState(APP_STATE_VOICE_STOPPING);
                    }
                }
                break;

            case APP_STATE_VOICE_STOPPING:
            case APP_STATE_AUDIO_STOPPING:
                /* A previous chain is still being destroyed. Wait until we get
                   back to APP_STATE_CONNECTED before starting any new chains.
                   A new rescan will automatically be triggered on entry to
                   APP_STATE_CONNECTED. */
                break;

            case APP_STATE_CONNECTED:
            {
                if(UsbDongle_VoiceDetermineNewSource() == voice_source_usb)
                {
                    /* USB voice source available, and audio stream is suspended.
                     * Can start voice streaming if voice connection is
                     * available OR can start Audio stream with VBC over LE
                     */
                    if (UsbDongle_VoiceIsSourceAvailable())
                    {
                        /* Voice connection is available, start call to route voice audio. */
                        usbDongleSetState(APP_STATE_VOICE_STARTING);
                    }
                    else if(UsbDongle_AudioIsVbcAvailable())
                    {
                        usbDongleSetState(APP_STATE_AUDIO_STARTING);
                    }
                    else
                    {
                        /* No Voice connection, try to connect it. Another rescan will be
                           triggered if successful. If not, there's not much we can do.
                           (remote device might not even support voice connection). */
                        DEBUG_LOG_DEBUG("usbDongleSmHandleInternalRescanAudioInputs, no voice connection, attempting profile connect");
                        device_t sink = usbDongleSmGetTaskData()->connected_sink;
                        if (sink)
                        {
                             bdaddr sink_addr = DeviceProperties_GetBdAddr(sink);
                             UsbDongle_VoiceConnect(&sink_addr);
                        }
                    }
                }
                else
                {
                    /* Audio source available, or a higher priority source
                       stopped. Resume audio streaming to complete the switch and start
                       the new source (if Audio connection is available). */
                    if (UsbDongle_AudioIsSourceAvailable())
                    {
                        DEBUG_LOG_DEBUG("usbDongleSmHandleInternalRescanAudioInputs, starting enum:audio_source_t:%d",
                                        UsbDongle_AudioDetermineNewSource());
#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
                        /* Do not start audio if incoming ring is in progress and connected with an LE audio sink */
                        if (!(usbDongleSmGetTaskData()->usb_hid_call_incoming && usbDongleConfig_IsConnectedToLeAudioSink()))
#endif
                        {
                            usbDongleSetState(APP_STATE_AUDIO_STARTING);
                        }
                    }
                    else
                    {
                        DEBUG_LOG_DEBUG("usbDongleSmHandleInternalRescanAudioInputs, Audio Sink (BT) not avilable");
                        UsbDongle_AudioSourceConnect();
                    }
                }
            }
            break;

            case APP_STATE_AUDIO_STARTING:
            case APP_STATE_AUDIO_STREAMING:
                if(UsbDongle_VoiceDetermineNewSource() == voice_source_usb)
                {
                    if (UsbDongle_VoiceIsSourceAvailable())
                    {
                        DEBUG_LOG_DEBUG("usbDongleSmHandleInternalRescanAudioInputs, voice connected while audio streaming");
                        /* Voice connection available, stop audio streaming to initiate switch.
                           Once complete another rescan will be triggered,
                           which can then start voice streaming. */
                        usbDongleSetState(APP_STATE_AUDIO_STOPPING);
                    }
                    else if (UsbDongle_AudioIsVbcAvailable())
                    {
                        if (UsbDongle_AudioSourceSwitchIsRequired())
                        {
                            DEBUG_LOG_DEBUG("usbDongleSmHandleInternalRescanAudioInputs, VBC connected while audio streaming");
                            usbDongleSetState(APP_STATE_AUDIO_STOPPING);
                        }
                    }
                    else
                    {
                        /* No Voice connection, try to connect it. Another rescan will be
                           triggered if successful. Keep Audio streaming going for now
                           (remote device might not even support Voice connection). */
                        DEBUG_LOG_DEBUG("usbDongleSmHandleInternalRescanAudioInputs, no voice connection, attempting profile connect");
                        device_t sink = usbDongleSmGetTaskData()->connected_sink;
                        if (sink)
                        {
                            bdaddr sink_addr = DeviceProperties_GetBdAddr(sink);
                            UsbDongle_VoiceConnect(&sink_addr);
                        }
                    }
                }
                else if (UsbDongle_AudioSourceSwitchIsRequired())
                {
                    /* A higher priority audio source has become available.
                       Suspend Audio streaming to stop the current source. Another rescan
                       will then be triggered, which can take care of starting
                       the new input source & resuming Audio streaming. */
                    DEBUG_LOG_INFO("usbDongleSmHandleInternalRescanAudioInputs, switching audio input source");
                    usbDongleSetState(APP_STATE_AUDIO_STOPPING);
                }
            break;

            default:
                break;
        }
    }
}

/*! \brief Delete pairing for all previously connected devices.
    \note There must be no active connections for this to succeed. */
static void usbDongleSmHandleInternalDeletePairedDevices(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleInternalDeletePairedDevices");

    usb_dongle_state_t state = UsbDongleSm_GetState();

    if (usbDongleSmIsAclConnectedState(state))
    {
        /* Current device can only be removed from the PDL if ACL disconnected.
           So, set a flag, disconnect first, and then clear pairing later. */
        usbDongleSmGetTaskData()->clear_pairing_requested = TRUE;
        usbDongleSetState(APP_STATE_DISCONNECTING);
        return;
    }

    if (state != APP_STATE_IDLE)
    {
        DEBUG_LOG_INFO("usbDongleSmHandleInternalDeletePairedDevices, cannot clear pairing from state"
                       " enum:usb_dongle_state_t:%d", state);
        return;
    }

    /* We are disconnected, so can clear the device list immediately */
    usbDongleSm_ClearPairing();

    /* Attempt to connect a new device */
    SinkService_Connect();
}

/*! \brief handles sm module specific ui inputs

    Invokes routines based on ui input received from ui module.

    \param[in] id - ui input

    \returns void
 */
static void usbDongleSmHandleUiInput(MessageId ui_input)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleUiInput");
    switch (ui_input)
    {
        case ui_input_sm_delete_handsets:
            /* Treat as request to clear sink pairing, as well as handsets */
            DEBUG_LOG("usbDongleSmHandleUiInput, ui_input_sm_delete_handsets");
            usbDongleSmSendInternalMsg(SM_INTERNAL_DELETE_PAIRED_DEVICES);
            break;

        case ui_input_factory_reset_request:
            DEBUG_LOG("usbDongleSmHandleUiInput, ui_input_factory_reset_request");
            usbDongleSmSendInternalMsg(SM_INTERNAL_FACTORY_RESET);
            break;

        case ui_input_gaming_mode_toggle:
            DEBUG_LOG("usbDongleSmHandleUiInput, ui_input_gaming_mode_toggle");
            usbDongleSmSendInternalMsg(SM_INTERNAL_AUDIO_MODE_TOGGLE);
            break;

        case ui_input_set_dongle_mode:
            DEBUG_LOG("usbDongleSmHandleUiInput, ui_input_set_dongle_mode");
            usbDongleSmSendInternalMsg(SM_INTERNAL_NEW_MODE_SWITCH);
            break;

        default:
            break;
    }
}

static void usbDongleSmHandleUiMediaPlayerContextChanged(audio_source_provider_context_t context)
{
    usb_dongle_state_t current_state = UsbDongleSm_GetState();
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleUiMediaPlayerContextChanged, context "
                       "enum:audio_source_provider_context_t:%d, current_state "
                       "enum:usb_dongle_state_t:%d", context, current_state);

    switch (context)
    {
        case context_audio_disconnected:
        case context_audio_connected:
            switch (current_state)
            {
                case APP_STATE_AUDIO_STOPPING:
                    usbDongleSetState(APP_STATE_CONNECTED);
                    break;
                case APP_STATE_AUDIO_STARTING:
                    /* Audio streaming not started, could be due to fast switching of audio sources.
                     * Moving to Audio stopping state and then to connected state for initiating stream restart.
                     * Once completed, another rescan will be triggered, which can then start audio streaming. */
                    usbDongleSetState(APP_STATE_AUDIO_STOPPING);
                    break;
                case APP_STATE_AUDIO_STREAMING:
                    /* Audio streaming ended unexpectedly, probably by remote device or by test interface.
                     * Continuing in APP_STATE_AUDIO_STREAMING state to avoid restarting audio streaming. */
                    usbDongleSmRescanAudioInputs();
                    break;
                default:
                    break;
            }
            break;

        case context_audio_is_playing:
        case context_audio_is_streaming:
            switch (current_state)
            {
                case APP_STATE_CONNECTING:
                case APP_STATE_CONNECTED:
                case APP_STATE_VOICE_STOPPING:
                case APP_STATE_AUDIO_STARTING:
#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
                    /* In case of LE-Audio, state can be in connecting state for broadcast case and so don't enter streaming state */
                    if (current_state != APP_STATE_CONNECTING)
#endif
                    {
                        usbDongleSetState(APP_STATE_AUDIO_STREAMING);
                    }
                    break;
                /* Audio has resumed while audio is stopping. Rescan audio inputs
                   to see if we should pause audio */
                case APP_STATE_AUDIO_STOPPING:
                    usbDongleSmRescanAudioInputs();
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
}

static void usbDongleSmHandleUiTelephonyContextChanged(voice_source_provider_context_t context)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleUiTelephonyContextChanged, enum:voice_source_provider_context_t:%d",
                       context);
    usb_dongle_state_t current_state = UsbDongleSm_GetState();

    switch (context)
    {
        case context_voice_in_call:
            if (usbDongleSmGetTaskData()->usb_hid_call_incoming)
            {
                /* The sink has accepted an incoming call, notify the host. */
                UsbSource_SendEvent(USB_SOURCE_HOOK_SWITCH_ANSWER);
            }
            break;

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
        case context_voice_ringing_incoming:
            if (usbDongleSmGetTaskData()->usb_hid_call_incoming && current_state == APP_STATE_AUDIO_STREAMING && UsbDongle_LeaIsAudioActive())
            {
                DEBUG_LOG("Pausing Line In/Usb Audio, while host has indicated an incoming ring event during LEA streaming ");
                usbDongleSetState(APP_STATE_AUDIO_STOPPING);
            }
            break;
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

        case context_voice_connected:
            /* The sink has rejected an incoming call. Send the Button One (call reject) to the host */
            if (UsbDongle_VoiceIsCallActive())
            {
                UsbSource_SendEvent(USB_SOURCE_BUTTON_ONE);
                /* Send Hook Switch Terminate to update HID status */
                UsbSource_SendEvent(USB_SOURCE_HOOK_SWITCH_TERMINATE);
            }
            switch (current_state)
            {
                case APP_STATE_VOICE_STARTING:
                case APP_STATE_VOICE_STREAMING:

                    /* If there is no active call and we are on hold but the sink hung up. For example, by pressing a button
                       (this normally happens when we have put the call on hold),
                       stop streaming voice to switch back to a2dp so that we don't stream silence. */
                    if (!usbDongleSmGetTaskData()->usb_hid_call_active && usbDongleSmGetTaskData()->hold_active)
                    {
                        DEBUG_LOG("usbDongleSmHandleUiTelephonyContextChanged, call on hold but sink hung up, stop streaming voice");
                        usbDongleSetState(APP_STATE_VOICE_STOPPING);
                    }
                    else
                    {
                        /* If the sink hung up during an ongoing call then send the hook switch terminate
                           (HOOK SWITCH (0x0)) to the HOST
                           This will also work if we were put on hold as the call will still be active.*/
                        UsbSource_SendEvent(USB_SOURCE_HOOK_SWITCH_TERMINATE);
                    }

                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
}

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
static void usbDongleSmHandleAvA2dpMediaConnected(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleAvA2dpMediaConnected");
    usbDongleSmRescanAudioInputs();
}
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

static void usbDongleSmHandleUiProviderContextUpdated(UI_PROVIDER_CONTEXT_UPDATED_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleUiProviderContextUpdated");

    switch (msg->provider)
    {
        case ui_provider_media_player:
            usbDongleSmHandleUiMediaPlayerContextChanged(msg->context);
            break;

        case ui_provider_telephony:
            usbDongleSmHandleUiTelephonyContextChanged(msg->context);
            break;

        default:
            break;
    }
}

static void usbDongleSmHandleSystemStateChange(SYSTEM_STATE_STATE_CHANGE_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleSystemStateChange");

    switch (msg->new_state)
    {
        case system_state_initialised:
            DEBUG_LOG_STATE("usbDongleSmHandleSystemStateChange, system_state_initialised");
            break;

        case system_state_starting_up:
            DEBUG_LOG_STATE("usbDongleSmHandleSystemStateChange, system_state_starting_up");
            break;

        case system_state_active:
            DEBUG_LOG_STATE("usbDongleSmHandleSystemStateChange, system_state_active");
            appPowerOn();
            if (usbDongleConfig_IsPairingDisallowed())
            {
                usbDongleSetState(APP_STATE_IDLE);
#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
                /* While in broadcast mode we may not want to enter pairing mode 
                   and allow dongle to operate as pure Broadcast only source 
                   In such scenario, we still like to start the broadcast streaming (source only) if a
                   media is active */
                UsbDongle_LeaHandleBroadcastModeToggle(FALSE);
                DEBUG_LOG_STATE("usbDongleSmHandleSystemStateChange, Auto pairing mode while in broadcast is disabled");
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
            }
            else
            {
                usbDongleSetState(APP_STATE_CONNECTING);    /* Connect/pair as required */
            }
            break;

        default:
            break;
    }
}

static void usbDongleSmHandlePairingPairCfm(PAIRING_PAIR_CFM_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandlePairingPairCfm, bdaddr 0x%04x 0x%02x 0x%06lx, status: %d",
                       msg->device_bd_addr.nap,
                       msg->device_bd_addr.uap,
                       msg->device_bd_addr.lap,
                       msg->status);

    usbDongle_HandlePairingCfm((msg->status == pairingSuccess), &msg->device_bd_addr);
}

#if defined(INCLUDE_A2DP_ANALOG_SOURCE) || defined(INCLUDE_LE_AUDIO_ANALOG_SOURCE)
static void usbDongleSmHandleWiredAudioDeviceConnectInd(WIRED_AUDIO_DEVICE_CONNECT_IND_T *msg)
{
    DEBUG_LOG_INFO("usbDongleSmHandleWiredAudioDeviceConnectInd, wired audio connected (analogue)");
    UNUSED(msg);
    UsbDongle_AudioInputAdd(usb_dongle_audio_input_analogue);

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
    UsbDongle_LeaAddContext(USB_DONGLE_LEA_ANALOG_AUDIO);
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

    usbDongleSmRescanAudioInputs();
}

static void usbDongleSmHandleWiredAudioDeviceDisconnectInd(WIRED_AUDIO_DEVICE_DISCONNECT_IND_T *msg)
{
    DEBUG_LOG_INFO("usbDongleSmHandleWiredAudioDeviceDisconnectInd, wired audio disconnected (analogue)");
    UNUSED(msg);
    UsbDongle_AudioInputRemove(usb_dongle_audio_input_analogue);

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
    UsbDongle_LeaRemoveContext(USB_DONGLE_LEA_ANALOG_AUDIO);
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

    usbDongleSmRescanAudioInputs();
}
#endif /* INCLUDE_A2DP_ANALOG_SOURCE) || INCLUDE_LE_AUDIO_ANALOG_SOURCE */

static void usbDongleSmHandleUsbAudioConnectedInd(USB_AUDIO_CONNECT_MESSAGE_T *msg)
{
    DEBUG_LOG_DEBUG("usbDongleSmHandleUsbAudioConnectedInd, USB audio connected");
    UNUSED(msg);
    UsbDongle_AudioInputAdd(usb_dongle_audio_input_usb);

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
    UsbDongle_LeaAddContext(USB_DONGLE_LEA_AUDIO);
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

    usbDongleSmRescanAudioInputs();
}

static void usbDongleSmHandleUsbAudioDisconnectedInd(USB_AUDIO_DISCONNECT_MESSAGE_T *msg)
{
    DEBUG_LOG_DEBUG("usbDongleSmHandleUsbAudioDisconnectedInd, USB audio disconnected");
    UNUSED(msg);
    UsbDongle_AudioInputRemove(usb_dongle_audio_input_usb);

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
    UsbDongle_LeaRemoveContext(USB_DONGLE_LEA_AUDIO);
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

    usbDongleSmRescanAudioInputs();
}

static void usbDongleSmHandleTelephonyAudioConnected(TELEPHONY_AUDIO_CONNECTED_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleTelephonyAudioConnected,"
                       " enum:voice_source_t:%d", msg->voice_source);

    if (msg->voice_source == voice_source_usb)
    {
        DEBUG_LOG_DEBUG("usbDongleSmHandleTelephonyAudioConnected, USB voice connected");
        UsbDongle_AudioInputAdd(usb_dongle_audio_input_usb_voice);

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
        if (usbDongleConfig_IsInHighQualityAudioMode())
        {
            UsbDongle_LeaAddContext(USB_DONGLE_LEA_VOICE);
        }
        else
        {
            UsbDongle_LeaAddContext(USB_DONGLE_LEA_AUDIO_VBC);

            /* We need to switch to high quality mode if we received call hid events
               (regardless of mode) or if we are in broadcast mode */
            if (usbDongleSmGetTaskData()->usb_hid_call_incoming ||
                usbDongleSmGetTaskData()->usb_hid_call_active   ||
                usbDongleConfig_IsInBroadcastAudioMode())
            {
                UsbDongle_LeaSwitchToVoiceContextIfRequired();

                /* Mode will be switched to high quality mode upon handling the ui input */
                Ui_InjectUiInput(ui_input_set_dongle_mode);
            }
        }
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

        usbDongleSmRescanAudioInputs();
    }
    else if (msg->voice_source == UsbDongle_VoiceGetCurrentVoiceSource())
    {
        DEBUG_LOG_DEBUG("usbDongleSmHandleTelephonyAudioConnected, Voice connected");
        switch (UsbDongleSm_GetState())
        {
            case APP_STATE_VOICE_STARTING:
                /* USB voice and HFP SCO/LE Voice both now connected and ready.
                   Route them together. */
                usbDongleSetState(APP_STATE_VOICE_STREAMING);
                break;
            case APP_STATE_VOICE_STREAMING:
                /* To get here there are two posibilities.
                 * 1. SCO must have dropped during a call, and then come back up again.
                 * 2. While entering APP_STATE_VOICE_STARTING, if SCO was active but
                 *    TELEPHONY_AUDIO_CONNECTED message not yet reached SM, streaming shall start immediately.
                 *    Refer usbDongleEnterVoiceStarting().
                 * So need to reconnect the voice chain if chain is not active. */
                if(Kymera_IsIdle())
                {
                    UsbDongle_VoiceStart();
                }
                break;
            default:
                break;
        }
    }
}

static void usbDongleSmHandleTelephonyAudioDisconnected(TELEPHONY_AUDIO_DISCONNECTED_T *msg)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleTelephonyAudioDisconnected,"
                       " enum:voice_source_t:%d", msg->voice_source);

    if (msg->voice_source == voice_source_usb)
    {
        DEBUG_LOG_DEBUG("usbDongleSmHandleTelephonyAudioDisconnected, USB voice disconnected");
        UsbDongle_AudioInputRemove(usb_dongle_audio_input_usb_voice);

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
        if (UsbDongle_LeaGetContext() & USB_DONGLE_LEA_VOICE)
        {
            UsbDongle_LeaRemoveContext(USB_DONGLE_LEA_VOICE);
        }
        else if (UsbDongle_LeaGetContext() & USB_DONGLE_LEA_AUDIO_VBC)
        {
            UsbDongle_LeaRemoveContext(USB_DONGLE_LEA_AUDIO_VBC);
        }
        else
        {
            /* Not expected to reach here */
            DEBUG_LOG_DEBUG("usbDongleSmHandleTelephonyAudioDisconnected, No context to remove");
        }
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

        usbDongleSmRescanAudioInputs();
    }
    else if (msg->voice_source == UsbDongle_VoiceGetCurrentVoiceSource())
    {
        DEBUG_LOG_DEBUG("usbDongleSmHandleTelephonyAudioDisconnected, voice disconnected");
        switch (UsbDongleSm_GetState())
        {
            case APP_STATE_VOICE_STOPPING:
                /* USB voice and HFP SCO/LE Voice both now disconnected. */
                usbDongleSetState(APP_STATE_CONNECTED);
                break;
            case APP_STATE_VOICE_STREAMING:
                /* HFP SCO dropped during a call, no longer anywhere to
                   route USB voice audio. Disconnect the chain. */
                UsbDongle_VoiceStop();
                break;
            default:
                break;
        }
    }
}

/*! \brief Check if dongle is streaming an audio or not */
static bool usbDongleSm_IsAudioStreaming(void)
{
    bool is_streaming = FALSE;

    /* Check if dongle SM is in audio streaming state */
    if (usbDongleSmIsAudioStreamingState(UsbDongleSm_GetState()))
    {
        is_streaming = TRUE;
    }

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

    /* Also check if dongle is streaming non-collocated broadcast or not (ie, without any connection)  */
    if (!usbDongleSmIsAclConnectedState(UsbDongleSm_GetState()) && UsbDongle_IsLeaBroadcastModeActive())
    {
        is_streaming = TRUE;
    }

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

    return is_streaming;
}

/*! \brief Restart the USB audio graph if USB audio remains connected and streaming.
           Return FALSE if USB audio is not connected. */
static bool usbDongleSm_RestartUsbAudioGraphIfConnected(void)
{
    bool usb_connected = FALSE;

    /* Restart the graph if USB audio remains connected and dongle is streaming audio */
    if (UsbDongle_AudioInputIsConnected(usb_dongle_audio_input_usb))
    {
        if (usbDongleSm_IsAudioStreaming())
        {
            UsbDongle_AudioRestartUsbGraph();
        }

        usb_connected = TRUE;
    }

    return usb_connected;
}

static void usbDongleSmHandleUsbAudioConfigChangedInd(bool is_sampling_rate_changed)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleUsbAudioConfigChangedInd rate_changed:%d", is_sampling_rate_changed);

    if (UsbDongle_IsGraphRestartNeededForUsbAudioConfigChange(is_sampling_rate_changed))
    {
        /* Restart if USB audio is connected and dongle is streaming */
        if (!usbDongleSm_RestartUsbAudioGraphIfConnected())
        {
            /* USB audio input is in disconnected state at the moment. This can happen momentarily if host PC changes
               only sample size and no change in sample rate. If this is the case, then restart the graph when audio input
               gets connected */
            MessageCancelAll(UsbDongleSmGetTask(), SM_INTERNAL_AUDIO_GRAPH_RESTART);
            MessageSendLater(UsbDongleSmGetTask(), SM_INTERNAL_AUDIO_GRAPH_RESTART, NULL, USB_DONGLE_SM_RESTART_GRAPH_DELAY);
        }
    }
}

static void usbDongleSmHandleRestartAudioGraph(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleRestartAudioGraph");

    /* Restart the graph if audio input got connected back quickly and dongle SM still remains
       in streaming state */
    usbDongleSm_RestartUsbAudioGraphIfConnected();
}

static void usbDongleSmHandleUsbVoiceConfigChangedInd(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleUsbVoiceConfigChangedInd");

    if (UsbDongleSm_GetState() == APP_STATE_VOICE_STREAMING ||
       (UsbDongleSm_GetState() == APP_STATE_AUDIO_STREAMING && UsbDongle_AudioIsVbcActive()))
    {
        UsbDongle_VoiceRestartGraph();
    }
}

static void usbDongleHandleIncomingOutOfBandRinging(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleHandleIncomingOutOfBandRinging");
    usbDongleSmGetTaskData()->usb_hid_call_incoming = TRUE;

    if (UsbDongle_VoiceIsSourceConnected())
    {
        UsbDongle_VoiceIncomingVoiceCall();
    }
}

static void usbDongleHandleIncomingCallEnded(void)
{
    voice_source_t voice_source = UsbDongle_VoiceGetCurrentVoiceSource();

    DEBUG_LOG_FN_ENTRY("usbDongleHandleIncomingCallEnded");
    usbDongleSmGetTaskData()->usb_hid_call_incoming = FALSE;

    /* Application is not currently in a call so the call has been dropped
     * before starting */
    if (UsbDongleSm_GetState() != APP_STATE_VOICE_STREAMING && VoiceSource_IsValid(voice_source))
    {
        VoiceSources_RejectIncomingCall(voice_source);
    }

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
    /* In case of LEA, we are stopping the streaming when a call comes. This needs to be resumed if the call gets rejected. */
    usbDongleSmRescanAudioInputs();
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */
}

static void usbDongleHandleCallActive(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleHandleCallActive");
    usbDongleSmGetTaskData()->usb_hid_call_incoming = FALSE;
    usbDongleSmGetTaskData()->usb_hid_call_active = TRUE;

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
    /* Reaching here indicates that a voice call is active. If we are in gaming with VBC,
       then we need to switch to voice call now. */
    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected() &&
        UsbDongle_LeaSwitchToVoiceContextIfRequired())
    {
        /* Return here as we are switching to voice context */
        DEBUG_LOG_STATE("usbDongleHandleCallActive Switching to high quality mode as voice call starting");

        /* Mode will be switched to high quality mode upon handling the ui input */
        Ui_InjectUiInput(ui_input_set_dongle_mode);
        return;
    }
#endif

    /* If we were receiving a call from HFP/LE Unicast and the USB has answered the call
     * Accept the call on HFP/LE Unicast */
    if (UsbDongle_VoiceIsSourceConnected())
    {
        VoiceSources_AcceptIncomingCall(UsbDongle_VoiceGetCurrentVoiceSource());
    }
}

static void usbDongleHandleCallEnded(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleHandleCallEnded");

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        VoiceSources_RejectIncomingCall(UsbDongle_VoiceGetCurrentVoiceSource());
    }
#endif

    usbDongleSmGetTaskData()->usb_hid_call_active = FALSE;
    usbDongleSmRescanAudioInputs();
}

static void usbDongleHandleCallHold(bool is_hold_active)
{
    DEBUG_LOG_FN_ENTRY("usbDongleHandleCallHold, hold active :%d",is_hold_active);
    /* Ignore repeat hold messages for the same state */
    if (usbDongleSmGetTaskData()->hold_active == is_hold_active)
    {
        return;
    }

    usbDongleSmGetTaskData()->hold_active = is_hold_active;

    /* If we have been taken off hold. Rescan inputs as we may need to start audio streaming */
    if (!usbDongleSmGetTaskData()->hold_active)
    {
        usbDongleSmRescanAudioInputs();
    }
}

static void usbDongleSmHandleUsbDeviceEnumerated(void)
{
     DEBUG_LOG_DEBUG("usbDongleSmHandleUsbDeviceEnumerated, USB device attached (USB_DEVICE_ENUMERATED)");
}

static void usbDongleSmHandleUsbDeviceDeconfigured(void)
{
     DEBUG_LOG_DEBUG("usbDongleSmHandleUsbDeviceDeconfigured, USB device detached (USB_DEVICE_DECONFIGURED)");
     /* Trigger Audio disconnection, depending on other connected sources */
     usbDongleSmRescanAudioInputs();
}

static void usbDongleSmHandleUsbDeviceSuspend(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleUsbDeviceSuspend");

    /*
     * The assumption at this point is that the Host PC has
     * disconnected the USB audio, and therefore we've stopped A2DP streaming
     * to the headset.
     *
     * There are two choices:
     * (1) remain connected while USB is suspended
     * (2) disconnect on USB suspend, and reconnect on USB resume.
     * Use DISCONNECT_ON_USB_SUSPEND in the Project Configuration to disconnect on suspend
     */
    if (usbDongleSmGetTaskData()->disconnect_on_suspend )
    {
        /*
         * Disable the Sink Service, which will disconnect from all devices
         * and then ignore button presses
         */
        SinkService_Disable();
    }
}

static void usbDongleSmHandleUsbDeviceResume(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleUsbDeviceResume");

    if (usbDongleSmGetTaskData()->disconnect_on_suspend )
    {
        /*
         * Re-enable the Sink Service as it was disabled when suspended
         * The assumption is that the Host PC will re-enable the USB port
         * and that should trigger the reconnection to the Sink device.
         * Using SinkService_Connect(); should not be needed here
         */
        SinkService_Enable();
    }
}

/*! \brief Handle request to start factory reset. */
static void usbDongleSm_HandleInternalFactoryReset(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSm_HandleInternalFactoryReset");

    switch (UsbDongleSm_GetState())
    {
        case APP_STATE_IDLE:
        case APP_STATE_PAIRING:
        case APP_STATE_CONNECTING:
            /* No connection, can factory reset immediately. */
            usbDongleSm_FactoryReset(); /* Does not return (reboots) */
            break;

        case APP_STATE_CONNECTED:
        case APP_STATE_AUDIO_STARTING:
        case APP_STATE_AUDIO_STREAMING:
        case APP_STATE_AUDIO_STOPPING:
        case APP_STATE_VOICE_STARTING:
        case APP_STATE_VOICE_STREAMING:
        case APP_STATE_VOICE_STOPPING:
            /* Disconnect gracefully first, so that the sink doesn't
               immediately try to reconnect after we reboot. */
            usbDongleSetState(APP_STATE_DISCONNECTING);
            /* Fall through */
        case APP_STATE_DISCONNECTING:
            /* Set flag, wait for disconnect to complete before rebooting. */
            usbDongleSmGetTaskData()->factory_reset_requested = TRUE;
            break;

        default:
            DEBUG_LOG_WARN("usbDongleSm_HandleInternalFactoryReset, factory reset cannot be performed in"
                           " enum:usb_dongle_state_t:%d", UsbDongleSm_GetState());
            break;
    }
}

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

/*! \brief Handle the mode transition from/to broadcast mode */
static void usbDongleSm_EnterOrExitBroadcastMode(void)
{
    if (UsbDongle_LeaHandleBroadcastModeToggle(UsbDongleSm_GetState() >= APP_STATE_CONNECTED))
    {
        usbDongleSmHandleInternalRescanAudioInputs();
    }
}

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

/*! \brief Handle the mode transition to gaming mode or high quality mode */
static void usbDongleSm_EnterGamingOrHighQualityMode(usb_dongle_audio_mode_t new_mode)
{
    bool is_restart_required = FALSE;

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    /* Update aptX Adaptive quality mode preference regardless of connection */
    Kymera_AptxAdEncoderSetDesiredQualityMode(new_mode == usb_dongle_audio_mode_high_quality ? aptxad_mode_high_quality :
                                                                                               aptxad_mode_low_latency);

    if (usbDongleConfig_IsInModeBredrOrDualWithBredrConnected())
    {
        /* New mode may require a different sample rate. If an A2DP media channel
           reconfiguration is triggered, then the suspend/resume will automatically
           take care of stopping & restarting the audio chain as required. */
        if (!UsbDongle_A2dpUpdatePreferredSampleRate())
        {
            /* No rate change / media channel reconfiguration was required.
               Check if audio chain needs manually restarting to switch to the new mode. */
            is_restart_required = (a2dp_source_data.a2dp_settings.seid == AV_SEID_APTX_ADAPTIVE_SRC);
        }
    }

#else

    UNUSED(new_mode);

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO*/

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        /* In LE audio, audio needs to be restarted upon mode change if already streaming */
        is_restart_required = TRUE;
    }

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

    if (UsbDongleSm_GetState() == APP_STATE_AUDIO_STREAMING && is_restart_required)
    {
        DEBUG_LOG_INFO("usbDongleSm_EnterGamingOrHighQualityMode restarting audio");
        /* Stopping will trigger an input rescan once complete, which will automatically restart audio again */
        usbDongleSetState(APP_STATE_AUDIO_STOPPING);
    }
}

/*! \brief Update the LED pattern as per the given mode */
static void usbDongleSm_UpdateLEDPattern(usb_dongle_audio_mode_t new_mode)
{
    uint16 client_lock_mask = 0x01;
    uint16 client_lock = 0x0;

    switch (new_mode)
    {
        case usb_dongle_audio_mode_high_quality:
            LedManager_SetPattern(app_led_pattern_streaming_hq, LED_PRI_MEDIUM, &client_lock, client_lock_mask);
        break;

        case usb_dongle_audio_mode_gaming:
            LedManager_SetPattern(app_led_pattern_streaming_gaming_mode, LED_PRI_MEDIUM, &client_lock, client_lock_mask);
        break;

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
        case usb_dongle_audio_mode_broadcast:
            LedManager_SetPattern(app_led_pattern_streaming_broadcast, LED_PRI_MEDIUM, &client_lock, client_lock_mask);
        break;
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

        default:
        break;
    }
}

/*! \brief Check if current transport needs to be disconnected as part of switching to new modes */
static bool usbDongleSm_IsTransportSwitchNeeded(usb_dongle_transport_mode_t new_transport_mode,
                                                usb_dongle_audio_mode_t new_audio_mode)
{
    sink_service_mode_t sink_service_mode_to_use = usbDongleSm_GetSinkServiceModeToUse(new_transport_mode,
                                                                                       new_audio_mode);

    return !SinkService_SetMode(sink_service_mode_to_use);
}

/*! \brief Handles the transition from current mode to new mode */
static void usbDongleSm_HandleAudioModeChange(usb_dongle_audio_mode_t current_audio_mode,
                                              usb_dongle_audio_mode_t new_audio_mode)
{
    if (current_audio_mode != new_audio_mode)
    {
        /* Update the LED pattern as per new mode */
        usbDongleSm_UpdateLEDPattern(new_audio_mode);

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
        /* Handles entering or exiting from broadcast mode */
        if (current_audio_mode == usb_dongle_audio_mode_broadcast ||
            new_audio_mode == usb_dongle_audio_mode_broadcast)
        {
            usbDongleSm_EnterOrExitBroadcastMode();
        }
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

        switch (new_audio_mode)
        {
            case usb_dongle_audio_mode_gaming:
            case usb_dongle_audio_mode_high_quality:
                usbDongleSm_EnterGamingOrHighQualityMode(new_audio_mode);
            break;

            default:
            break;
        }
    }
}

/*! \brief Handles the request from user to set the mode */
static void usbDongleSm_HandleInternalSetMode(void)
{
    usb_dongle_audio_mode_t current_audio_mode, new_audio_mode;
    usb_dongle_transport_mode_t current_transport_mode, new_transport_mode;

    current_audio_mode = usbDongleConfig_GetAudioMode();
    new_audio_mode = usbDongleConfig_GetNewAudioMode();
    current_transport_mode = usbDongleConfig_GetTransportMode();
    new_transport_mode = usbDongleConfig_GetNewTransportMode();

    DEBUG_LOG_FN_ENTRY("usbDongleSm_HandleInternalSetMode enum:usb_dongle_transport_mode_t:%d --> enum:usb_dongle_transport_mode_t:%d"
                       " enum:usb_dongle_audio_mode_t:%d --> enum:usb_dongle_audio_mode_t:%d",
                        current_transport_mode, new_transport_mode, current_audio_mode, new_audio_mode);

    if (!usbDongleConfig_IsModeCombinationValid(new_audio_mode, new_transport_mode))
    {
        /* Clear the new modes */
        UsbDongleConfig_SetNewMode(current_audio_mode, current_transport_mode);
        return;
    }

    /* Disconnect current transport if new modes requires a transport switch */
    if (usbDongleSm_IsTransportSwitchNeeded(new_transport_mode, new_audio_mode))
    {
        usbDongleSm_DisconnectCurrentTransport();
        /* Don't just return here as we may still needs to apply changes related to audio mode changes */
        /* New transport can not be saved now, wait for disconnection to complete */
        new_transport_mode = current_transport_mode;
    }

    /* Store the new mode in PS key if changed */
    (void) UsbDongleConfig_SaveMode(new_audio_mode, new_transport_mode);

    usbDongleSm_HandleAudioModeChange(current_audio_mode, new_audio_mode);

    /* Reconnect upon a mode change if there is no connection */
    if (UsbDongleSm_GetState() == APP_STATE_IDLE)
    {
        usbDongleSetState(APP_STATE_CONNECTING);
    }
}

/*! \brief Cycle across different modes supported by the source application */
static void usbDongleSm_HandleInternalCycleMode(void)
{
    usb_dongle_audio_mode_t new_audio_mode;

    new_audio_mode = (usbDongleConfig_GetAudioMode() + 1) % usb_dongle_audio_mode_max;

    /* Check if new audio mode is compatible with current transport mode or not */
    if (!usbDongleConfig_IsModeCombinationValid(new_audio_mode,
                                               usbDongleConfig_GetTransportMode()))
    {
        /* Pick next audio mode */
        new_audio_mode = (new_audio_mode + 1) % usb_dongle_audio_mode_max;
    }

    /* Store the new mode in PS key if changed */
    UsbDongleConfig_SetNewMode(new_audio_mode, usbDongleConfig_GetTransportMode());

    usbDongleSm_HandleInternalSetMode();
}

static void usbDongleSm_HandleInternalPairingAclComplete(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSm_HandleInternalPairingAclComplete");

    bdaddr *sink_addr = &usbDongleSmGetTaskData()->hold_pairing_acl_device_address;

    if (UsbDongleSm_GetState() == APP_STATE_PAIRING && !BdaddrIsZero(sink_addr))
    {
        if (ConManagerIsConnected(sink_addr))
        {
            DEBUG_LOG_DEBUG("usbDongleSm_HandleInternalPairingAclComplete, ACL created, pairing to 0x%04x,%02x,%06lx",
                            sink_addr->nap, sink_addr->uap, sink_addr->lap);
            Pairing_PairAddress(UsbDongleSmGetTask(), sink_addr);
        }
        else
        {
            DEBUG_LOG_INFO("usbDongleSm_HandleInternalPairingAclComplete, pairing timed out");
            usbDongleSmReleasePairingAcl();
            usbDongleSetState(APP_STATE_IDLE);
        }
    }
}

/*! \brief Take action following power's indication of imminent sleep.
    SM only permits sleep when soporific. */
static void UsbDongleSm_HandlePowerSleepPrepareInd(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongleSm_HandlePowerSleepPrepareInd");

    appPowerSleepPrepareResponse(UsbDongleSmGetTask());
}

/*! \brief Handle sleep cancellation. */
static void UsbDongleSm_HandlePowerSleepCancelledInd(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongleSm_HandlePowerSleepCancelledInd");
}

/*! \brief Take action following power's indication of imminent shutdown.
    Can be received in any state. */
static void UsbDongleSm_HandlePowerShutdownPrepareInd(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongleSm_HandlePowerShutdownPrepareInd");

    appPowerShutdownPrepareResponse(UsbDongleSmGetTask());
}

/*! \brief Handle shutdown cancelled. */
static void UsbDongleSm_HandlePowerShutdownCancelledInd(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongleSm_HandlePowerShutdownCancelledInd");
}

/*! \brief Handle sink service connected message. */
static void UsbDongleSm_HandleSinkServiceConnectedCfm(SINK_SERVICE_CONNECTED_CFM_T *msg)
{
    DEBUG_LOG_FN_ENTRY("UsbDongleSm_HandleSinkServiceConnectedCfm");

    switch (msg->status)
    {
    case sink_service_status_success:
        usbDongleSmGetTaskData()->connected_sink = msg->device;
        usbDongleConfig_SetConnectedTransport(msg->transport);

        if (!usbDongleSmIsAclConnectedState(usbDongleSmGetTaskData()->state))
        {
            usbDongleSetState(APP_STATE_CONNECTED);

            /* We may have to disconnect if the audio mode/transport mode changed while the connection is established */
            if (usbDongleSm_IsTransportSwitchNeeded(usbDongleConfig_GetTransportMode(),
                                                    usbDongleConfig_GetAudioMode()))
            {
                usbDongleSm_DisconnectCurrentTransport();
                return;
            }
        }
        break;
    case sink_service_status_failed:
        usbDongleSetState(APP_STATE_IDLE);
    default:
        break;
    }
}

/*! \brief Handle sink service disconnected message. */
static void UsbDongleSm_HandleSinkServiceDisconnectedCfm(SINK_SERVICE_DISCONNECTED_CFM_T *msg)
{
    DEBUG_LOG_FN_ENTRY("UsbDongleSm_HandleSinkServiceDisconnectedCfm");

    if (msg->device == usbDongleSmGetTaskData()->connected_sink)
    {
        usbDongleSmGetTaskData()->connected_sink = NULL;

        if (usbDongleSmGetTaskData()->clear_pairing_requested)
        {
            DEBUG_LOG_INFO("UsbDongleSm_HandleSinkServiceDisconnectedCfm, clear pairing requested");
            usbDongleSm_ClearPairing();
            usbDongleSmGetTaskData()->clear_pairing_requested = FALSE;
        }

        if (usbDongleSmGetTaskData()->factory_reset_requested)
        {
            usbDongleSm_FactoryReset(); /* Does not return (reboots) */
        }
    }
    else if (UsbDongleSm_GetState() == APP_STATE_CONNECTING)
    {
        DEBUG_LOG_INFO("UsbDongleSm_HandleSinkServiceDisconnectedCfm, connecting timed out");
    }

#if defined(INCLUDE_SOURCE_APP_LE_AUDIO)
    if (usbDongleConfig_IsInModeLeAudioOrDualWithLeConnected())
    {
        switch(UsbDongleSm_GetState())
        {
            case APP_STATE_AUDIO_STARTING:
            case APP_STATE_AUDIO_STREAMING:
                if (MessagePendingFirst(UsbDongleSmGetTask(), SM_INTERNAL_RESCAN_AUDIO_INPUTS, NULL))
                {
                    /* If the disconnection happened while in streaming state, then process the message
                       SM_INTERNAL_RESCAN_AUDIO_INPUTS immediately if it is pending for delivery. Processing this message
                       after the dongle state moved to APP_STATE_IDLE triggers unwanted reconnection in case of a user
                       initiated disconnection. */
                    usbDongleSmHandleInternalRescanAudioInputs();
                }
                usbDongleSetState(APP_STATE_AUDIO_STOPPING);
                break;

            case APP_STATE_VOICE_STARTING:
            case APP_STATE_VOICE_STREAMING:
                usbDongleSetState(APP_STATE_VOICE_STOPPING);
                break;

            default:
                break;
        }
    }
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

    if (UsbDongleSm_GetState() != APP_STATE_IDLE)
    {
        usbDongleSetState(APP_STATE_IDLE);
    }

    if (usbDongleSmGetTaskData()->connected_sink == NULL)
    {
        usbDongleConfig_SetConnectedTransport(SINK_SERVICE_TRANSPORT_UNKNOWN);
    }

    /* Trigger reconection if sink service disconnection happened as part of a transport switch */
    usbDongleSm_ReconnectTransportIfRequired();
}

/*! \brief Handle sink service first profile connected message. */
static void UsbDongleSm_HandleSinkServiceFirstProfile(SINK_SERVICE_FIRST_PROFILE_CONNECTED_IND_T *msg)
{
    DEBUG_LOG_INFO("UsbDongleSm_HandleSinkServiceFirstProfile, device %p", msg->device);

    /* Set the transport to BREDR here as this message is only for BREDR profile level connection */
    usbDongleConfig_SetConnectedTransport(SINK_SERVICE_TRANSPORT_BREDR);
    usbDongleSetState(APP_STATE_CONNECTED);
}

/*! \brief Application state machine message handler.
    \param task The SM task.
    \param id The message ID to handle.
    \param message The message content (if any).
*/
static void usbDongleSmHandleMessage(Task task, MessageId id, Message message)
{
    DEBUG_LOG_FN_ENTRY("usbDongleSmHandleMessage");

    UNUSED(task);
    UNUSED(message);

    if (isMessageUiInput(id))
    {
       usbDongleSmHandleUiInput(id);
       return;
    }

    switch (id)
    {
#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
        case AV_A2DP_MEDIA_CONNECTED:
            usbDongleSmHandleAvA2dpMediaConnected();
            break;
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

        case UI_PROVIDER_CONTEXT_UPDATED:
            usbDongleSmHandleUiProviderContextUpdated((UI_PROVIDER_CONTEXT_UPDATED_T *)message);
            break;

        case SYSTEM_STATE_STATE_CHANGE:
            usbDongleSmHandleSystemStateChange((SYSTEM_STATE_STATE_CHANGE_T *)message);
            break;

#if defined(INCLUDE_A2DP_ANALOG_SOURCE) || defined(INCLUDE_LE_AUDIO_ANALOG_SOURCE)
        case WIRED_AUDIO_DEVICE_CONNECT_IND:
            usbDongleSmHandleWiredAudioDeviceConnectInd((WIRED_AUDIO_DEVICE_CONNECT_IND_T *)message);
            break;

        case WIRED_AUDIO_DEVICE_DISCONNECT_IND:
            usbDongleSmHandleWiredAudioDeviceDisconnectInd((WIRED_AUDIO_DEVICE_DISCONNECT_IND_T *)message);
            break;
#endif /* INCLUDE_A2DP_ANALOG_SOURCE) || INCLUDE_LE_AUDIO_ANALOG_SOURCE */

        case PAIRING_PAIR_CFM:
            usbDongleSmHandlePairingPairCfm((PAIRING_PAIR_CFM_T *)message);
            break;

        case USB_AUDIO_CONNECTED_IND:
            usbDongleSmHandleUsbAudioConnectedInd((USB_AUDIO_CONNECT_MESSAGE_T *)message);
            break;

        case USB_AUDIO_DISCONNECTED_IND:
            usbDongleSmHandleUsbAudioDisconnectedInd((USB_AUDIO_DISCONNECT_MESSAGE_T *)message);
            break;

        case TELEPHONY_AUDIO_CONNECTED:
            usbDongleSmHandleTelephonyAudioConnected((TELEPHONY_AUDIO_CONNECTED_T *)message);
            break;

        case TELEPHONY_AUDIO_DISCONNECTED:
            usbDongleSmHandleTelephonyAudioDisconnected((TELEPHONY_AUDIO_DISCONNECTED_T *)message);
            break;

        case TELEPHONY_INCOMING_CALL_OUT_OF_BAND_RINGTONE:
            usbDongleHandleIncomingOutOfBandRinging();
            break;

        case TELEPHONY_INCOMING_CALL_ENDED:
            usbDongleHandleIncomingCallEnded();
            break;

        case TELEPHONY_CALL_ONGOING:
            usbDongleHandleCallActive();
            break;

        case TELEPHONY_CALL_ENDED:
            usbDongleHandleCallEnded();
            break;

        case TELEPHONY_HOLD_ACTIVE:
            usbDongleHandleCallHold(TRUE);
        break;

        case TELEPHONY_HOLD_INACTIVE:
            usbDongleHandleCallHold(FALSE);
        break;

        case USB_AUDIO_SAMPLE_RATE_AUDIO_IND:
            usbDongleSmHandleUsbAudioConfigChangedInd(TRUE);
            break;

        case USB_AUDIO_STREAMING_CONFIG_CHANGED_AUDIO_IND:
            usbDongleSmHandleUsbAudioConfigChangedInd(FALSE);
            break;

        case USB_AUDIO_SAMPLE_RATE_VOICE_IND:
        case USB_AUDIO_STREAMING_CONFIG_CHANGED_VOICE_IND:
            usbDongleSmHandleUsbVoiceConfigChangedInd();
            break;

        case USB_DEVICE_ENUMERATED:
            usbDongleSmHandleUsbDeviceEnumerated();
            break;

        case USB_DEVICE_DECONFIGURED:
            usbDongleSmHandleUsbDeviceDeconfigured();
            break;

        case USB_DEVICE_SUSPEND:
            usbDongleSmHandleUsbDeviceSuspend();
            break;

        case USB_DEVICE_RESUME:
            usbDongleSmHandleUsbDeviceResume();
            break;

        case SM_INTERNAL_AUDIO_GRAPH_RESTART:
            usbDongleSmHandleRestartAudioGraph();
            break;

        case SM_INTERNAL_DELETE_PAIRED_DEVICES:
            usbDongleSmHandleInternalDeletePairedDevices();
            break;

        case SM_INTERNAL_FACTORY_RESET:
            usbDongleSm_HandleInternalFactoryReset();
            break;

        case SM_INTERNAL_AUDIO_MODE_TOGGLE:
            usbDongleSm_HandleInternalCycleMode();
            break;

        case SM_INTERNAL_PAIRING_ACL_COMPLETE:
            usbDongleSm_HandleInternalPairingAclComplete();
            break;

        case SM_INTERNAL_RESCAN_AUDIO_INPUTS:
            usbDongleSmHandleInternalRescanAudioInputs();
            break;

        case SM_INTERNAL_NEW_MODE_SWITCH:
            usbDongleSm_HandleInternalSetMode();
            break;

        /* Power indications */
        case APP_POWER_SLEEP_PREPARE_IND:
            UsbDongleSm_HandlePowerSleepPrepareInd();
            break;

        case APP_POWER_SLEEP_CANCELLED_IND:
            UsbDongleSm_HandlePowerSleepCancelledInd();
            break;

        case APP_POWER_SHUTDOWN_PREPARE_IND:
            UsbDongleSm_HandlePowerShutdownPrepareInd();
            break;

        case APP_POWER_SHUTDOWN_CANCELLED_IND:
            UsbDongleSm_HandlePowerShutdownCancelledInd();
            break;

        case SINK_SERVICE_CONNECTED_CFM:
            UsbDongleSm_HandleSinkServiceConnectedCfm((SINK_SERVICE_CONNECTED_CFM_T *)message);
            break;

        case SINK_SERVICE_DISCONNECTED_CFM:
            UsbDongleSm_HandleSinkServiceDisconnectedCfm((SINK_SERVICE_DISCONNECTED_CFM_T *)message);
        break;

        case SINK_SERVICE_FIRST_PROFILE_CONNECTED_IND:
            UsbDongleSm_HandleSinkServiceFirstProfile((SINK_SERVICE_FIRST_PROFILE_CONNECTED_IND_T *)message);
        break;

        default:
            UnexpectedMessage_HandleMessage(id);
            break;
    }
}

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) || defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)

static void usbDongle_HandleVoiceConnection(void)
{
    DEBUG_LOG_INFO("usbDongle_HandleVoiceConnection, Voice Profile connected");

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

    if (!usbDongleConfig_IsInBredrMode() && !usbDongleConfig_IsInDualModeWithBredrConnected())
    {
        return;
    }

    switch (UsbDongleSm_GetState())
    {
        case APP_STATE_CONNECTED:
        case APP_STATE_AUDIO_STARTING:
        case APP_STATE_AUDIO_STREAMING:
        case APP_STATE_AUDIO_STOPPING:
        case APP_STATE_VOICE_STOPPING:
            /* We might have been waiting on this to start voice.
               Rescan audio inputs to find out. */
            usbDongleSmRescanAudioInputs();
            break;

        default:
            break;
    }
#endif
}

static void usbDongle_HandleVoiceDisconnection(void)
{
    DEBUG_LOG_INFO("usbDongle_HandleVoiceDisconnection, Voice Profile disconnected");
}

#endif /*  defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) || defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) */

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
static void usbDongle_HandleA2dpSourceConnection(void)
{
    DEBUG_LOG_INFO("usbDongle_HandleA2dpSourceConnection, A2DP profile connected");
}

static void usbDongle_HandleA2dpSourceDisconnection(void)
{
    DEBUG_LOG_INFO("usbDongle_HandleA2dpSourceDisconnection, A2DP profile disconnected");
}
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

static void usbDongle_HandlePairingCfm(bool success, const bdaddr *bd_addr)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandlePairingCfm");

    if (success)
    {
        /* Pairing request is locally initiated, so the device is a sink. */
        device_t sink = BtDevice_GetDeviceCreateIfNew(bd_addr, DEVICE_TYPE_SINK);
        BtDevice_SetDefaultProperties(sink);

        /* Now that we have successfully paired,
         * we can set the link behavior within bluestack to disable connection retries */
        BtDevice_SetLinkBehavior(bd_addr);

        BtDevice_AddSupportedProfilesToDevice(sink, sink_service_config.supported_profile_mask);

        DeviceDbSerialiser_SerialiseDevice(sink);

        DEBUG_LOG_INFO("usbDongle_HandlePairingCfm, sink paired successfully 0x%04x,%02x,%06lx",
                       bd_addr->nap, bd_addr->uap, bd_addr->lap);

        if (ConManagerIsConnected(bd_addr))
        {
            /* Set the newly created device as the current connected_sink. */
            if (!usbDongleSmGetTaskData()->connected_sink)
            {
                usbDongleSmGetTaskData()->connected_sink = sink;
                usbDongleConfig_SetConnectedTransport(SINK_SERVICE_TRANSPORT_BREDR);
            }
            /* Pairing is finished, ACL up, so move to the connected state. */
            usbDongleSetState(APP_STATE_CONNECTED);

            if (ConManagerIsAclLocal(bd_addr))
            {
                /* Use the sink service to connect profiles (sink service will
                   then register its own interest in the ACL at this point). */
                SinkService_Connect();
            }
        }
        else
        {
            /* Should be impossible to get here, we held the pairing ACL open */
            DEBUG_LOG_WARN("usbDongle_HandlePairingCfm, ACL unexpectedly dropped after pairing");
            usbDongleSetState(APP_STATE_IDLE);
        }
    }
    else
    {
        DEBUG_LOG_INFO("usbDongle_HandlePairingCfm, pairing failed");

        if (UsbDongleSm_GetState() == APP_STATE_PAIRING)
        {
            usbDongleSetState(APP_STATE_IDLE);
        }
    }

    /* Ensure ACL is released regardless of pairing outcome - either we are no
       longer interested in the device, or sink service has taken over. */
    usbDongleSmReleasePairingAcl();
}

/*! \brief Get the sink service operating mode for given transport mode and audio mode */
static sink_service_mode_t usbDongleSm_GetSinkServiceModeToUse(usb_dongle_transport_mode_t transport_mode,
                                                               usb_dongle_audio_mode_t audio_mode)
{
    sink_service_mode_t mode;

    switch(transport_mode)
    {
#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
        case usb_dongle_transport_mode_bredr:
            mode = SINK_SERVICE_MODE_BREDR;
            break;
#endif

#if defined(INCLUDE_SOURCE_APP_LE_AUDIO) && defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)
        case usb_dongle_transport_mode_le_audio:
            mode = SINK_SERVICE_MODE_LE;
            break;
#endif

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) && defined(INCLUDE_SOURCE_APP_LE_AUDIO) && defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)
        case usb_dongle_transport_mode_dual:
            /* Choose sink service dual mode preference based on the audio mode */
            if (usbDongleConfig_GetTransportForAudioMode(audio_mode) == SINK_SERVICE_TRANSPORT_BREDR)
            {
                mode = SINK_SERVICE_MODE_DUAL_PREF_BREDR;
            }
            else
            {
                /* Preferred transport for audio mode is LE. If current audio mode is only possible
                   in LE transport, we need to enforce LE mode */
                mode = (audio_mode == usb_dongle_audio_mode_broadcast) ? SINK_SERVICE_MODE_LE :
                                                                         SINK_SERVICE_MODE_DUAL_PREF_LE;
            }
            break;
#endif

        default:
#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
            mode = SINK_SERVICE_MODE_BREDR;
#elif defined(INCLUDE_SOURCE_APP_LE_AUDIO) && defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)
            mode = SINK_SERVICE_MODE_LE;
#else
            mode = SINK_SERVICE_MODE_INVALID;
#endif
            break;
    }

    UNUSED(audio_mode);

    return mode;
}

/*! \brief Get the sink service operating mode for current transport mode and audio mode */
static sink_service_mode_t usbDongleSm_GetSinkServiceMode(void)
{
    return usbDongleSm_GetSinkServiceModeToUse(usbDongleConfig_GetTransportMode(), usbDongleConfig_GetAudioMode());
}

/* Public Functions */

Task UsbDongleSmGetTask(void)
{
    return &(usbDongleSmGetTaskData()->task);
}

/*! \brief Initialise the main application state machine.
 */
bool UsbDongleSmInit(Task init_task)\
{
    usb_dongle_sm_data_t* sm = usbDongleSmGetTaskData();

    UsbDongleConfigInit();

    memset(sm, 0, sizeof(*sm));
    sm->task.handler = usbDongleSmHandleMessage;
    sm->state = APP_STATE_INIT;
    sm->clear_pairing_requested = FALSE;
    sm->factory_reset_requested = FALSE;
    sm->connected_sink = NULL;
    sm->disconnect_on_suspend = APP_CONFIG_DISCONNECT_ON_USB_SUSPEND;
    sm->hold_active = FALSE;
    sm->usb_hid_call_active = FALSE;
    sm->usb_hid_call_incoming = FALSE;
    sm->transport_switch_in_progress = FALSE;

    BdaddrSetZero(&sm->hold_pairing_acl_device_address);

    /* Set sink service mode */
    SinkService_SetMode(usbDongleSm_GetSinkServiceMode());
    SinkService_ClientRegister(&sm->task);

    /* Register for system state change indications */
    SystemState_RegisterForStateChanges(&sm->task);

    /* Register for AV indications */
    appAvStatusClientRegister(&sm->task);

    /* Register SM as a UI input consumer */
    Ui_RegisterUiInputConsumer(&sm->task, sm_ui_inputs, ARRAY_DIM(sm_ui_inputs));

    /* Register SM as a UI provider */
    Ui_RegisterUiProvider(ui_provider_app_sm, usbDongleSm_GetApplicationCurrentContext);

    /* Register SM as a UI context consumer to get media and telephony events */
    Ui_RegisterContextConsumers(ui_provider_media_player, &sm->task);
    Ui_RegisterContextConsumers(ui_provider_telephony, &sm->task);

    WiredAudioSource_ClientRegister(&sm->task);
    WiredAudioSource_StartMonitoring(&sm->task);
    UsbDevice_ClientRegister(&sm->task);
    UsbAudio_ClientRegister(&sm->task, USB_AUDIO_REGISTERED_CLIENT_MEDIA);
    UsbAudio_ClientRegister(&sm->task, USB_AUDIO_REGISTERED_CLIENT_TELEPHONY);
    UsbAudio_ClientRegister(&sm->task, USB_AUDIO_REGISTERED_CLIENT_CONFIG_CHANGE);

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)
    UsbDongle_HfpRegisterConnectionCallbacks(usbDongle_HandleVoiceConnection,
                                             usbDongle_HandleVoiceDisconnection);

    Kymera_AptxAdEncoderSetDesiredQualityMode(usbDongleConfig_IsInGamingAudioMode() ?
                                                aptxad_mode_low_latency : aptxad_mode_high_quality);
    UsbDongle_A2dpRegisterConnectionCallbacks(usbDongle_HandleA2dpSourceConnection,
                                              usbDongle_HandleA2dpSourceDisconnection);
#endif

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)
    UsbDongle_LeVoiceRegisterConnectionCallbacks(usbDongle_HandleVoiceConnection,
                                                 usbDongle_HandleVoiceDisconnection);
#endif

    Telephony_RegisterForMessages(&sm->task);

    /* register with power to receive sleep/shutdown messages. */
    appPowerClientRegister(&sm->task);

    UNUSED(init_task);
    return TRUE;
}

usb_dongle_state_t UsbDongleSm_GetState(void)
{
    return usbDongleSmGetTaskData()->state;
}

device_t UsbDongleSm_GetCurrentSink(void)
{
    /* Get currently connected sink device */
    device_t sink = usbDongleSmGetTaskData()->connected_sink;

    if (!sink)
    {
        /* Not connected, try to determine future/MRU device instead */
        sink = usbDongleSmDetermineSinkDevice();
    }
    return sink;
}

bool UsbDongleSm_PairSink(const bdaddr *sink_addr)
{
    if (UsbDongleSm_GetState() != APP_STATE_IDLE)
    {
        DEBUG_LOG_INFO("UsbDongleSm_PairSink, cannot pair, state not idle");
        return FALSE;
    }

    if (!BdaddrIsZero(&usbDongleSmGetTaskData()->hold_pairing_acl_device_address))
    {
        DEBUG_LOG_INFO("UsbDongleSm_PairSink cannot pair, already pairing");
        return FALSE;
    }

    /* Open ACL before pairing, to hold over into profile connection. */
    usbDongleSmGetTaskData()->hold_pairing_acl_device_address = *sink_addr;
    MessageSendConditionally(UsbDongleSmGetTask(), SM_INTERNAL_PAIRING_ACL_COMPLETE,
                             NULL, ConManagerCreateAcl(sink_addr));

    usbDongleSetState(APP_STATE_PAIRING);
    return TRUE;
}

