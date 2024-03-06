/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Charger case application state machine.

*/

#include "charger_case_sm.h"

/* local includes */
#include "charger_case_a2dp_source.h"
#include "charger_case_audio.h"
#include "charger_case_dfu.h"
#include "charger_case_sm_private.h"

/* framework includes */
#include <av.h>
#include <adk_log.h>
#include <audio_sources.h>
#include <connection_manager.h>
#include <dfu.h>
#include <gatt_server_gatt.h>
#include <handset_service.h>
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
/* system includes */
#include <connection.h>
#include <device_list.h>
#include <message.h>
#include <panic.h>
#include <vmtypes.h>
#include <ps_key_map.h>
#include <ps.h>
#include <telephony_messages.h>

/*! \name Connection library factory reset keys 

    These keys should be deleted during a factory reset.
*/
/*! @{ */
#ifdef USE_SYNERGY
#define CSR_BT_PS_KEY_SYSTEM        100
#else
#define ATTRIBUTE_BASE_PSKEY_INDEX  100
#define TDL_BASE_PSKEY_INDEX        142
#define TDL_INDEX_PSKEY             141
#define TDL_SIZE                    BtDevice_GetMaxTrustedDevices()
#endif
/*! @} */

#define chargerCaseSmIsAvConnectedState(state) ((state == CHARGER_CASE_STATE_CONNECTED) || \
                                                (state == CHARGER_CASE_STATE_AUDIO_STARTING) || \
                                                (state == CHARGER_CASE_STATE_AUDIO_STREAMING) || \
                                                (state == CHARGER_CASE_STATE_AUDIO_STOPPING))

/*! \brief UI Inputs the state machine is interested in. */
const message_group_t sm_ui_inputs[] =
{
    UI_INPUTS_HANDSET_MESSAGE_GROUP,
    UI_INPUTS_DEVICE_STATE_MESSAGE_GROUP
};

/*! Application state machine task data instance. */
charger_case_sm_data_t charger_case_sm;


static void chargerCaseSetState(charger_case_state_t new_state);
static unsigned chargerCaseSm_GetApplicationCurrentContext(void);


/*! \brief Enter Idle state.
 */
static void chargerCaseEnterIdle(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseEnterIdle");
}

/*! \brief Exit Idle state.
 */
static void chargerCaseExitIdle(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseExitIdle");
}

/*! \brief Enter Idle state.
 */
static void chargerCaseEnterPairing(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseEnterPairing");
    ChargerCase_ResetSinkDevice();
    DEBUG_LOG_INFO("ChargerCase: Starting RSSI Pairing...");
    ChargerCase_InquiryStart();
}

/*! \brief Exit Idle state.
 */
static void chargerCaseExitPairing(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseExitPairing");
}

/*! \brief Enter Connecting state.
 */
static void chargerCaseEnterConnecting(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseEnterConnecting");
    ChargerCase_A2dpSourceConnect();
}

/*! \brief Exit Connecting state.
 */
static void chargerCaseExitConnecting(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseExitConnecting");
}

/*! \brief Enter Connected state.
 */
static void chargerCaseEnterConnected(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseEnterConnected");
}

/*! \brief Exit Connected state.
 */
static void chargerCaseExitConnected(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseExitConnected");
}

/*! \brief Enter Audio Starting state.
 */
static void chargerCaseEnterAudioStarting(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseEnterAudioStarting");

    /* Resume (or begin) AV streaming */
    ChargerCase_A2dpSourceResume();
}

/*! \brief Exit Audio Starting state.
 */
static void chargerCaseExitAudioStarting(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseExitAudioStarting");
}

/*! \brief Enter Audio Streaming state.
 */
static void chargerCaseEnterAudioStreaming(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseEnterAudioStreaming");

    /* Start audio, if not already started */
    if (!ChargerCase_AudioIsActive())
    {
        ChargerCase_AudioStart(NULL);
    }
}

/*! \brief Exit Audio Streaming state.
 */
static void chargerCaseExitAudioStreaming(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseExitAudioStreaming");
}

/*! \brief Enter Audio Stopping state.
 */
static void chargerCaseEnterAudioStopping(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseEnterAudioStopping");

    /* Suspend AV streaming */
    ChargerCase_A2dpSourceSuspend();
}

/*! \brief Exit Audio Stopping state.
 */
static void chargerCaseExitAudioStopping(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseExitAudioStopping");
}

/*! \brief Enter Disconnecting state.
 */
static void chargerCaseEnterDisconnecting(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseEnterDisconnecting");
    ChargerCase_A2dpSourceDisconnect();
}

/*! \brief Exit Disconnecting state.
 */
static void chargerCaseExitDisconnecting(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseExitDisconnecting");
    if (charger_case_sm.pairing_delete_requested)
    {
        DEBUG_LOG_INFO("ChargerCase: Pairing delete requested.");
        BtDevice_DeleteAllDevicesOfType(DEVICE_TYPE_HANDSET);
        ConnectionSmDeleteAllAuthDevices(0);
        charger_case_sm.pairing_delete_requested = FALSE;
    }
}

/*! \brief Provides application state machine context changes to the User Interface module.

    \param[in]  void

    \return     current_sm_ctxt - current application context of sm module.
*/
static unsigned chargerCaseSm_GetApplicationCurrentContext(void)
{
    sm_provider_context_t context = BAD_CONTEXT;

    switch(ChargerCaseSm_GetState())
    {
        case CHARGER_CASE_STATE_INIT:
            context = context_app_sm_inactive;
            break;

        case CHARGER_CASE_STATE_IDLE:
            context = context_app_sm_idle;
            break;

        case CHARGER_CASE_STATE_PAIRING:
            context = context_app_sm_pairing;
            break;

        case CHARGER_CASE_STATE_CONNECTING:
            context = context_app_sm_connecting;
            break;

        case CHARGER_CASE_STATE_CONNECTED:
        case CHARGER_CASE_STATE_DISCONNECTING:
        case CHARGER_CASE_STATE_AUDIO_STARTING:
            context = context_app_sm_connected;
            break;

        case CHARGER_CASE_STATE_AUDIO_STREAMING:
        case CHARGER_CASE_STATE_AUDIO_STOPPING:
            context = context_app_sm_streaming;
            break;

        default:
            break;
    }

    return (unsigned)context;
}

static void ChargerCaseSm_ClearPsStore(void)
{
    DEBUG_LOG_FN_ENTRY("ChargerCaseSm_ClearPsStore");

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
    PsStore(PS_KEY_HFP_CONFIG, NULL, 0);
#endif
    /* Clear out any in progress DFU status */
#ifdef INCLUDE_DFU
    Dfu_ClearPsStore();
#endif
}

/*! \brief Delete pairing and reboot device. */
static void ChargerCaseSm_DeletePairingAndReset(void)
{
    DEBUG_LOG_ALWAYS("ChargerCaseSm_DeletePairingAndReset");

    ChargerCaseSm_ClearPsStore();

    SystemReboot_Reboot();
}

/*! \brief Enter actions when we enter the factory reset state.
 */
static void chargerCase_EnterFactoryReset(void)
{
    DEBUG_LOG_DEBUG("chargerCase_EnterFactoryReset : CHARGER_CASE_STATE_FACTORY_RESET");
    ChargerCaseSm_DeletePairingAndReset();
}

/*! \brief Exit factory reset. */
static void ChargerCase_ExitFactoryReset(void)
{
    /* Should never happen */
    Panic();
}


/* This function is called to change the applications state, it automatically
   calls the entry and exit functions for the new and old states.
*/
static void chargerCaseSetState(charger_case_state_t new_state)
{
    charger_case_state_t previous_state = ChargerCaseSm_GetState();

    DEBUG_LOG_FN_ENTRY("chargerCaseSetState, enum:charger_case_state_t:%d -> "
                       "enum:charger_case_state_t:%d", previous_state, new_state);

    /* Handle state exit functions */
    switch (previous_state)
    {
        case CHARGER_CASE_STATE_INIT:
            break;

        case CHARGER_CASE_STATE_IDLE:
            chargerCaseExitIdle();
            break;

        case CHARGER_CASE_STATE_FACTORY_RESET:
            ChargerCase_ExitFactoryReset();
            break;

        case CHARGER_CASE_STATE_PAIRING:
            chargerCaseExitPairing();
            break;

        case CHARGER_CASE_STATE_CONNECTING:
            chargerCaseExitConnecting();
            break;

        case CHARGER_CASE_STATE_CONNECTED:
            chargerCaseExitConnected();
            break;

        case CHARGER_CASE_STATE_AUDIO_STARTING:
            chargerCaseExitAudioStarting();
            break;

        case CHARGER_CASE_STATE_AUDIO_STREAMING:
            chargerCaseExitAudioStreaming();
            break;

        case CHARGER_CASE_STATE_AUDIO_STOPPING:
            chargerCaseExitAudioStopping();
            break;

        case CHARGER_CASE_STATE_DISCONNECTING:
            chargerCaseExitDisconnecting();
            break;

        default:
            DEBUG_LOG_ERROR("Attempted to exit unsupported state "
                            "enum:charger_case_state_t:0x%02x", previous_state);
            Panic();
            break;
    }

    /* Set new state */
    SmGetTaskData()->state = new_state;

    /* Handle state entry functions */
    switch (new_state)
    {
        case CHARGER_CASE_STATE_IDLE:
            chargerCaseEnterIdle();
            break;

        case CHARGER_CASE_STATE_PAIRING:
            chargerCaseEnterPairing();
            break;

        case CHARGER_CASE_STATE_CONNECTING:
            chargerCaseEnterConnecting();
            break;

        case CHARGER_CASE_STATE_CONNECTED:
            chargerCaseEnterConnected();
            break;

        case CHARGER_CASE_STATE_AUDIO_STARTING:
            chargerCaseEnterAudioStarting();
            break;

        case CHARGER_CASE_STATE_AUDIO_STREAMING:
            chargerCaseEnterAudioStreaming();
            break;

        case CHARGER_CASE_STATE_AUDIO_STOPPING:
            chargerCaseEnterAudioStopping();
            break;

        case CHARGER_CASE_STATE_DISCONNECTING:
            chargerCaseEnterDisconnecting();
            break;

        case CHARGER_CASE_STATE_FACTORY_RESET:
            chargerCase_EnterFactoryReset();

        default:
            DEBUG_LOG_ERROR("Attempted to enter unsupported state "
                            "enum:charger_case_state_t:0x%02x", new_state);
            Panic();
            break;
    }

    Ui_InformContextChange(ui_provider_app_sm,
                           chargerCaseSm_GetApplicationCurrentContext());
}

/*! \brief Determine whether pairing is required. */
static bool chargerCasePairingIsRequired(void)
{
#if defined(INCLUDE_RSSI_PAIRING)
    uint8 type_sink = DEVICE_TYPE_SINK;
    device_t earbud_device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_type, &type_sink, sizeof(uint8));
    if (!earbud_device)
    {
        return TRUE;
    }
#endif
    return FALSE;
}

/*! \brief Request sink device connect. */
static void chargerCaseSmConnectSinkDevice(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseSmConnectSinkDevice");
    MessageSend(ChargerCaseSmGetTask(), SM_INTERNAL_CONNECT_SINK_DEVICE, NULL);
}

/*! \brief Request sink device delete. */
static void chargerCaseSmDeleteSinkDevices(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseSmDeleteSinkDevices");
    MessageSend(ChargerCaseSmGetTask(), SM_INTERNAL_DELETE_PAIRED_DEVICES, NULL);
}

/*! \brief Request sink device pairing. */
static void chargerCaseSmPairSinkDevice(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseSmPairSinkDevice");
    MessageSend(ChargerCaseSmGetTask(), SM_INTERNAL_START_PAIRING, NULL);
}

/*! \brief Rescan available audio inputs.

    Connect/disconnect, suspend/resume, or switch as required.
*/
static void chargerCaseSmRescanAudioInputs(void)
{
    charger_case_state_t current_state = ChargerCaseSm_GetState();

    DEBUG_LOG_FN_ENTRY("chargerCaseSmRescanAudioInputs, current_state:"
                       " enum:charger_case_state_t:%d", current_state);

    if (ChargerCase_AudioDetermineNewSource() == audio_source_none)
    {
        DEBUG_LOG_DEBUG("ChargerCase: No remaining audio sources");

        if (UsbDevice_IsConnectedToHost())
        {
            DEBUG_LOG_DEBUG("ChargerCase: USB still attached, stay connected");

            if ((current_state == CHARGER_CASE_STATE_AUDIO_STARTING) ||
                (current_state == CHARGER_CASE_STATE_AUDIO_STREAMING))
            {
                /* Cancel/suspend media streaming - can be resumed quickly
                   and still allows forwarding of AVRCP commands. */
                chargerCaseSetState(CHARGER_CASE_STATE_AUDIO_STOPPING);
            }
        }
        else /* USB detached, no audio sources. Disconnect A2DP. */
        {
            if (chargerCaseSmIsAvConnectedState(current_state))
            {
                chargerCaseSetState(CHARGER_CASE_STATE_DISCONNECTING);
                /* Disconnecting will take care of stopping audio. */
            }
        }
    }
    else /* There are still audio sources connected. */
    {
        switch (current_state)
        {
            case CHARGER_CASE_STATE_IDLE:
                /* Connect A2DP (which will take care of starting audio) */
                chargerCaseSmConnectSinkDevice();
                break;

            case CHARGER_CASE_STATE_DISCONNECTING:
                /* Cancel the pending disconnect by requesting a connection */
                chargerCaseSetState(CHARGER_CASE_STATE_CONNECTING);
                break;

            case CHARGER_CASE_STATE_CONNECTED:
            case CHARGER_CASE_STATE_AUDIO_STOPPING:
                /* Connected but no media. Resume A2DP media streaming */
                chargerCaseSetState(CHARGER_CASE_STATE_AUDIO_STARTING);
                break;

            case CHARGER_CASE_STATE_AUDIO_STREAMING:
                /* Already streaming, switch source if required */
                ChargerCase_AudioSwitchSourceIfRequired();
                break;

            default:
                break;
        }
    }
}

/*! \brief Connect to sink device, initiating pairing if necessary */
static void chargerCaseSmHandleInternalConnectSinkDevice(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseSmHandleInternalConnectSinkDevice");

    if (chargerCasePairingIsRequired())
    {
        DEBUG_LOG_INFO("ChargerCase: No Devices in paired device list");
        chargerCaseSmPairSinkDevice();
        /* Connection is automatically attempted after pairing */
        return;
    }

    if (ChargerCaseSm_GetState() != CHARGER_CASE_STATE_IDLE)
    {
        DEBUG_LOG_INFO("ChargerCase: State not idle, ignoring connect request");
        return;
    }

    if (!UsbDevice_IsConnectedToHost() &&
        ChargerCase_AudioDetermineNewSource() == audio_source_none)
    {
        DEBUG_LOG_INFO("ChargerCase: No Audio input, ignoring connect request");
        return;
    }

    chargerCaseSetState(CHARGER_CASE_STATE_CONNECTING);
}

/*! \brief Disconnect from a sink device*/
static void chargerCaseSmHandleInternalDisconnectSinkDevice(SM_INTERNAL_DISCONNECT_SINK_DEVICE_T* message)
{
    if (message != NULL)
    {
        charger_case_sm.pairing_delete_requested = message->request_pairing_delete;
    }
    if (ChargerCaseSm_GetState() != CHARGER_CASE_STATE_DISCONNECTING){
        chargerCaseSetState(CHARGER_CASE_STATE_DISCONNECTING);
    }
}

/*! \brief Delete pairing for all previously connected devices.
    \note There must be no active connections for this to succeed. */
static void chargerCaseSmHandleInternalDeletePairedDevices(void)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseSmHandleInternalDeletePairedDevices");

    ConnectionSmDeleteAllAuthDevices(0);
    DEBUG_LOG_INFO("ChargerCase: Deleted paired device list");

#if defined(INCLUDE_RSSI_PAIRING)
    chargerCaseSmPairSinkDevice();
#endif
}

/*! \brief Request a factory reset. */
static void ChargerCaseSm_FactoryReset(void)
{
    MessageSend(ChargerCaseSmGetTask(), SM_INTERNAL_FACTORY_RESET, NULL);
}

/*! \brief handles sm module specific ui inputs

    Invokes routines based on ui input received from ui module.

    \param[in] id - ui input

    \returns void
 */
static void chargerCaseSmHandleUiInput(MessageId ui_input)
{
    switch (ui_input)
    {
        case ui_input_connect_handset:
            /* Treat as request to connect sink device, rather than a handset */
            DEBUG_LOG("chargerCaseSmHandleUiInput -> ui_input_connect_handset");
            /* If the charger chase is already connected then disconnect */
            if (chargerCaseSmIsAvConnectedState(ChargerCaseSm_GetState()))
            {
                chargerCaseSetState(CHARGER_CASE_STATE_DISCONNECTING);
            }
            else
            {
                chargerCaseSmConnectSinkDevice();
            }
            break;

        case ui_input_sm_delete_handsets:
            /* Treat as request to clear sink pairing, as well as handset */
            DEBUG_LOG("chargerCaseSmHandleUiInput -> ui_input_sm_delete_handset");
            chargerCaseSmDeleteSinkDevices();
            break;

        case ui_input_factory_reset_request:
            DEBUG_LOG("chargerCaseSmHandleUiInput -> ui_input_factory_reset_request");
            ChargerCaseSm_FactoryReset();
            break;

        default:
            break;
    }
}

static void chargerCaseSmHandleUiPairingContextChanged(pairing_provider_context_t context)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseSmHandleUiPairingContextChanged");

    switch (context)
    {
        case context_handset_pairing_idle:
            if (ChargerCaseSm_GetState() == CHARGER_CASE_STATE_PAIRING)
            {
                chargerCaseSetState(CHARGER_CASE_STATE_IDLE);
            }
            break;

        case context_handset_pairing_active:
            /* SM is the initiator of this, so no need to react */
            break;

        default:
            break;
    }
}

static void chargerCaseSmHandleUiMediaPlayerContextChanged(audio_source_provider_context_t context)
{
    charger_case_state_t current_state = ChargerCaseSm_GetState();

    DEBUG_LOG_FN_ENTRY("chargerCaseSmHandleUiMediaPlayerContextChanged");

    switch (context)
    {
        case context_audio_disconnected:
        case context_audio_connected:
            if ((current_state == CHARGER_CASE_STATE_AUDIO_STREAMING) ||
                (current_state == CHARGER_CASE_STATE_AUDIO_STOPPING))
            {
                chargerCaseSetState(CHARGER_CASE_STATE_CONNECTED);
            }
            break;

        case context_audio_is_playing:
        case context_audio_is_streaming:
            if ((current_state == CHARGER_CASE_STATE_CONNECTING) ||
                (current_state == CHARGER_CASE_STATE_CONNECTED) ||
                (current_state == CHARGER_CASE_STATE_AUDIO_STARTING))
            {
                chargerCaseSetState(CHARGER_CASE_STATE_AUDIO_STREAMING);
            }
            break;

        default:
            break;
    }
}

static void chargerCaseSmHandleAvA2dpDisconnectedInd(AV_A2DP_DISCONNECTED_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseSmHandleAvA2dpDisconnectedInd Reason:enum:avA2dpDisconnectReason:%d ", msg->reason);

    if (ChargerCaseSm_GetState() == CHARGER_CASE_STATE_CONNECTING)
    {
        DEBUG_LOG_STATE("Charger Case: AV A2DP Disconnected");
        chargerCaseSetState(CHARGER_CASE_STATE_IDLE);
    }
}

static void chargerCaseSmHandleUiProviderContextUpdated(UI_PROVIDER_CONTEXT_UPDATED_T *msg)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseSmHandleUiProviderContextUpdated");

    switch (msg->provider)
    {
        case ui_provider_handset_pairing:
            chargerCaseSmHandleUiPairingContextChanged(msg->context);
            break;

        case ui_provider_media_player:
            chargerCaseSmHandleUiMediaPlayerContextChanged(msg->context);
            break;

        default:
            break;
    }
}

static void chargerCaseSmHandleSystemStateChange(SYSTEM_STATE_STATE_CHANGE_T *msg)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseSmHandleSystemStateChange");

    switch (msg->new_state)
    {
        case system_state_initialised:
            DEBUG_LOG_STATE("SYSTEM_STATE_STATE_CHANGE -> system_state_initialised");
            break;

        case system_state_starting_up:
            DEBUG_LOG_STATE("SYSTEM_STATE_STATE_CHANGE -> system_state_starting_up");
            break;

        case system_state_active:
            DEBUG_LOG_STATE("SYSTEM_STATE_STATE_CHANGE -> system_state_active");
            appPowerOn();
            chargerCaseSetState(CHARGER_CASE_STATE_IDLE);
            chargerCaseSmConnectSinkDevice();   /* Connect/pair as required */
            break;

        default:
            break;
    }
}

static void chargerCaseSmHandleConManagerConnectionInd(CON_MANAGER_CONNECTION_IND_T *msg)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseSmHandleConManagerConnectionInd");

    if (msg->ble || !appDeviceTypeIsSink(&msg->bd_addr))
    {
        /* Ignore connections from non-sink devices - these are tracked
           separately by other components (e.g. handset service) */
        return;
    }

    if (msg->connected)
    {
        DEBUG_LOG_STATE("Connection Manager -> connected");
        if (!chargerCaseSmIsAvConnectedState(ChargerCaseSm_GetState()))
        {
            chargerCaseSetState(CHARGER_CASE_STATE_CONNECTED);
        }
    }
    else
    {
        DEBUG_LOG_STATE("Connection Manager -> disconnected");
        if (ChargerCaseSm_GetState() != CHARGER_CASE_STATE_IDLE)
        {
            chargerCaseSetState(CHARGER_CASE_STATE_IDLE);
        }
    }
}

static void chargerCaseSmHandleWiredAudioDeviceConnectInd(WIRED_AUDIO_DEVICE_CONNECT_IND_T *msg)
{
    DEBUG_LOG_INFO("ChargerCase: Wired Audio Connected (Analogue)");
    UNUSED(msg);
    ChargerCase_AudioInputAdd(charger_case_audio_input_analogue);
    chargerCaseSmRescanAudioInputs();
}

static void chargerCaseSmHandleWiredAudioDeviceDisconnectInd(WIRED_AUDIO_DEVICE_DISCONNECT_IND_T *msg)
{
    DEBUG_LOG_INFO("ChargerCase: Wired Audio Disconnected (Analogue)");
    UNUSED(msg);
    ChargerCase_AudioInputRemove(charger_case_audio_input_analogue);
    chargerCaseSmRescanAudioInputs();
}

static void chargerCaseSmHandleUsbAudioConnectedInd(USB_AUDIO_CONNECT_MESSAGE_T *msg)
{
    DEBUG_LOG_DEBUG("ChargerCase: USB Audio Connected");
    UNUSED(msg);
    ChargerCase_AudioInputAdd(charger_case_audio_input_usb);
    chargerCaseSmRescanAudioInputs();
}

static void chargerCaseSmHandleUsbAudioDisconnectedInd(USB_AUDIO_DISCONNECT_MESSAGE_T *msg)
{
    DEBUG_LOG_DEBUG("ChargerCase: USB Audio Disconnected");
    UNUSED(msg);
    ChargerCase_AudioInputRemove(charger_case_audio_input_usb);
    chargerCaseSmRescanAudioInputs();
}

static void chargerCaseSmHandleUsbDeviceEnumerated(void)
{
     DEBUG_LOG_DEBUG("ChargerCase: USB Device Attached (USB_DEVICE_ENUMERATED)");
}

static void chargerCaseSmHandleUsbDeviceDeconfigured(void)
{
     DEBUG_LOG_DEBUG("ChargerCase: USB Device Detached (USB_DEVICE_DECONFIGURED)");
     /* Trigger A2DP disconnection, depending on other connected sources */
     chargerCaseSmRescanAudioInputs();
}

/*! \brief Handle request to start factory reset. */
static void ChargerCaseSm_HandleInternalFactoryReset(void)
{
    if (ChargerCaseSm_GetState() > CHARGER_CASE_STATE_FACTORY_RESET)
    {
        DEBUG_LOG_DEBUG("ChargerCaseSm_HandleInternalFactoryReset");
        chargerCaseSetState(CHARGER_CASE_STATE_FACTORY_RESET);
    }
    else
    {
        DEBUG_LOG_WARN("headsetSmHandleInternalFactoryReset cannot be done in state %d", ChargerCaseSm_GetState());
    }
}

/*! \brief Handle request to start pairing. */
static void ChargerCaseSm_HandleInternalStartPairing(void)
{
    if (ChargerCaseSm_GetState() != CHARGER_CASE_STATE_PAIRING)
    {
        DEBUG_LOG_DEBUG("ChargerCaseSm_HandleInternalStartPairing");
        if (ChargerCaseDfu_UpgradeInProgress())
        {
            DEBUG_LOG_INFO("ChargerCase: Pairing suppressed - DFU in progress");
        }
        else
        {
            chargerCaseSetState(CHARGER_CASE_STATE_PAIRING);
        }
    }
}

/*! \brief Handle request to start stop pairing. */
static void ChargerCaseSm_HandleInternalStopPairing(void)
{
    if (ChargerCaseSm_GetState() == CHARGER_CASE_STATE_PAIRING)
    {
        DEBUG_LOG_DEBUG("ChargerCaseSm_HandleInternalStopPairing");
        chargerCaseSetState(CHARGER_CASE_STATE_IDLE);
        a2dp_source_data.state = CHARGER_CASE_A2DP_SOURCE_STATE_DISCONNECTED;

#ifdef USE_SYNERGY
        CmCancelInquiryReqSend(&a2dp_source_data.task);
#else
        ConnectionInquireCancel(&a2dp_source_data.task);
#endif /* USE_SYNERGY */
    }
    else
    {
        DEBUG_LOG_INFO("ChargerCaseSm_HandleInternalStopPairing: Pairing not in progress");
    }
}

/*! \brief Application state machine message handler.
    \param task The SM task.
    \param id The message ID to handle.
    \param message The message content (if any).
*/
static void chargerCaseSmHandleMessage(Task task, MessageId id, Message message)
{
    DEBUG_LOG_FN_ENTRY("chargerCaseSmHandleMessage");

    UNUSED(task);
    UNUSED(message);

    if (isMessageUiInput(id))
    {
       chargerCaseSmHandleUiInput(id);
       return;
    }
    switch (id)
    {
        case AV_A2DP_DISCONNECTED_IND:
            chargerCaseSmHandleAvA2dpDisconnectedInd((AV_A2DP_DISCONNECTED_IND_T *) message);
            break;

        case UI_PROVIDER_CONTEXT_UPDATED:
            chargerCaseSmHandleUiProviderContextUpdated((UI_PROVIDER_CONTEXT_UPDATED_T *)message);
            break;

        case SYSTEM_STATE_STATE_CHANGE:
            chargerCaseSmHandleSystemStateChange((SYSTEM_STATE_STATE_CHANGE_T *)message);
            break;

        case CON_MANAGER_CONNECTION_IND:
            chargerCaseSmHandleConManagerConnectionInd((CON_MANAGER_CONNECTION_IND_T *)message);
            break;

        case WIRED_AUDIO_DEVICE_CONNECT_IND:
            chargerCaseSmHandleWiredAudioDeviceConnectInd((WIRED_AUDIO_DEVICE_CONNECT_IND_T *)message);
            break;

        case WIRED_AUDIO_DEVICE_DISCONNECT_IND:
            chargerCaseSmHandleWiredAudioDeviceDisconnectInd((WIRED_AUDIO_DEVICE_DISCONNECT_IND_T *)message);
            break;

        case USB_AUDIO_CONNECTED_IND:
            chargerCaseSmHandleUsbAudioConnectedInd((USB_AUDIO_CONNECT_MESSAGE_T *)message);
            break;

        case USB_AUDIO_DISCONNECTED_IND:
            chargerCaseSmHandleUsbAudioDisconnectedInd((USB_AUDIO_DISCONNECT_MESSAGE_T *)message);
            break;

        case USB_DEVICE_ENUMERATED:
            chargerCaseSmHandleUsbDeviceEnumerated();
            break;

        case USB_DEVICE_DECONFIGURED:
            chargerCaseSmHandleUsbDeviceDeconfigured();
            break;

        case TELEPHONY_AUDIO_CONNECTED:
            DEBUG_LOG_VERBOSE("chargerCaseSmHandleMessage: TELEPHONY_AUDIO_CONNECTED Unhandled");
            break;
        case TELEPHONY_AUDIO_DISCONNECTED:
            DEBUG_LOG_VERBOSE("chargerCaseSmHandleMessage: TELEPHONY_AUDIO_DISCONNECTED Unhandled");
            break;
        case TELEPHONY_INCOMING_CALL_OUT_OF_BAND_RINGTONE:
            DEBUG_LOG_VERBOSE("chargerCaseSmHandleMessage: TELEPHONY_INCOMING_CALL_OUT_OF_BAND_RINGTONE Unhandled");
            break;
        case TELEPHONY_INCOMING_CALL_ENDED:
            DEBUG_LOG_VERBOSE("chargerCaseSmHandleMessage: TELEPHONY_INCOMING_CALL_ENDED Unhandled");
            break;
        case TELEPHONY_CALL_ONGOING:
            DEBUG_LOG_VERBOSE("chargerCaseSmHandleMessage: TELEPHONY_CALL_ONGOING Unhandled");
            break;
        case TELEPHONY_CALL_ENDED:
            DEBUG_LOG_VERBOSE("chargerCaseSmHandleMessage: TELEPHONY_CALL_ENDED Unhandled");
            break;

        case SM_INTERNAL_CONNECT_SINK_DEVICE:
            chargerCaseSmHandleInternalConnectSinkDevice();
            break;

        case SM_INTERNAL_DISCONNECT_SINK_DEVICE:
            chargerCaseSmHandleInternalDisconnectSinkDevice((SM_INTERNAL_DISCONNECT_SINK_DEVICE_T *)message);
            break;

        case SM_INTERNAL_DELETE_PAIRED_DEVICES:
            chargerCaseSmHandleInternalDeletePairedDevices();
            break;

        case SM_INTERNAL_FACTORY_RESET:
            ChargerCaseSm_HandleInternalFactoryReset();
            break;

        case SM_INTERNAL_START_PAIRING:
            ChargerCaseSm_HandleInternalStartPairing();
            break;

        case SM_INTERNAL_STOP_PAIRING:
            ChargerCaseSm_HandleInternalStopPairing();
            break;

        default:
            UnexpectedMessage_HandleMessage(id);
            break;
    }
}


/* Public Functions */

Task ChargerCaseSmGetTask(void)
{
  return &(SmGetTaskData()->task);
}

/*! \brief Initialise the main application state machine.
 */
bool ChargerCaseSmInit(Task init_task)
{
    charger_case_sm_data_t* sm = SmGetTaskData();
    memset(sm, 0, sizeof(*sm));
    sm->task.handler = chargerCaseSmHandleMessage;
    sm->state = CHARGER_CASE_STATE_INIT;
    sm->pairing_delete_requested = FALSE;

    /* register with connection manager to get notification of (dis)connections */
    ConManagerRegisterConnectionsClient(&sm->task);

    /* Register for system state change indications */
    SystemState_RegisterForStateChanges(&sm->task);

    /* Register for AV indications */
    appAvStatusClientRegister(&sm->task);

    /* Register SM as a UI input consumer */
    Ui_RegisterUiInputConsumer(&sm->task, sm_ui_inputs, ARRAY_DIM(sm_ui_inputs));

    /* Register SM as a UI provider */
    Ui_RegisterUiProvider(ui_provider_app_sm, chargerCaseSm_GetApplicationCurrentContext);

    /* Register SM as a UI context consumer to get pairing and media events */
    Ui_RegisterContextConsumers(ui_provider_handset_pairing, &sm->task);
    Ui_RegisterContextConsumers(ui_provider_media_player, &sm->task);

    WiredAudioSource_ClientRegister(&sm->task);
    WiredAudioSource_StartMonitoring(&sm->task);
    UsbDevice_ClientRegister(&sm->task);
    UsbAudio_ClientRegister(&sm->task, USB_AUDIO_REGISTERED_CLIENT_MEDIA);
    UsbAudio_ClientRegister(&sm->task, USB_AUDIO_REGISTERED_CLIENT_TELEPHONY);

    Telephony_RegisterForMessages(&sm->task);

    UNUSED(init_task);
    return TRUE;
}

charger_case_state_t ChargerCaseSm_GetState(void)
{
    return SmGetTaskData()->state;
}
