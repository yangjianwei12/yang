/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for application LE Voice source module
*/

#if defined(INCLUDE_SOURCE_APP_LE_AUDIO) && defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)

#include "usb_dongle_logging.h"
#include "usb_dongle_lea.h"
#include "usb_dongle_le_voice.h"
#include "le_audio_client.h"
#include "telephony_messages.h"
#include <voice_sources.h>
#include <ui.h>

usb_dongle_le_voice_data_t le_voice_data;

static unsigned usbDongle_GetTelephonyStateContext(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_GetTelephonyStateContext");
    voice_source_provider_context_t context = BAD_CONTEXT;

    switch(le_voice_data.state)
    {
        case APP_LE_VOICE_STATE_STREAMING:
            context = context_voice_in_call;
            break;

        case APP_LE_VOICE_STATE_INCOMING:
            context = context_voice_ringing_incoming;
            break;

        case APP_LE_VOICE_STATE_CONNECTED:
            context = context_voice_connected;
            break;

        default:
            context = context_voice_connected;
            break;
     }

    return (unsigned)context;
}

void UsbDongle_LeVoiceCallEnded(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_LeVoiceCallEnded");

    le_voice_data.state = APP_LE_VOICE_STATE_CONNECTED;
    Telephony_NotifyCallAudioDisconnected(voice_source_le_audio_unicast_1);
    Ui_InformContextChange(ui_provider_telephony, usbDongle_GetTelephonyStateContext());
}

void UsbDongle_LeVoiceCallStart(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_LeVoiceCallStart");
    le_voice_data.state = APP_LE_VOICE_STATE_STREAMING;

    Telephony_NotifyCallAudioConnected(voice_source_le_audio_unicast_1);
    Ui_InformContextChange(ui_provider_telephony, usbDongle_GetTelephonyStateContext());
}

void UsbDongle_LeVoiceIncomingCall(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_LeVoiceIncomingCall");

    LeAudioClient_CreateIncomingCall();
    le_voice_data.state = APP_LE_VOICE_STATE_INCOMING;
    Ui_InformContextChange(ui_provider_telephony, usbDongle_GetTelephonyStateContext());
}

void UsbDongle_LeVoiceOutgoingCall(void)
{
    if (!LeAudioClient_IsCallActive())
    {
        DEBUG_LOG_FN_ENTRY("UsbDongle_LeVoiceOutgoingCall");

        LeAudioClient_CreateActiveOutgoingCall();
        le_voice_data.state = APP_LE_VOICE_STATE_OUTGOING;
    }
}

static void usbDongle_LeVoiceConnected(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_LeVoiceConnected");
    le_voice_data.state = APP_LE_VOICE_STATE_CONNECTED;
    le_voice_data.connected_cb();
}

static void usbDongle_LeVoiceDisconnected(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeVoiceDisconnected");
    le_voice_data.state = APP_LE_VOICE_STATE_DISCONNECTED;
    le_voice_data.connected_cb();
}

static void usbDongle_LeVoiceCallRemoteAccept(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeVoiceCallRemoteAccept");

    le_voice_data.state = APP_LE_VOICE_STATE_STREAMING;
    Ui_InformContextChange(ui_provider_telephony, usbDongle_GetTelephonyStateContext());
}

static void usbDongle_LeVoiceCallRemoteTerminate(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_LeVoiceCallRemoteTerminate");

    /* Update telephony UI context as connected which in turn will inform the host */
    Ui_InformContextChange(ui_provider_telephony, context_voice_connected);
}

/*! \brief Handles voice controls messages received from remote */
static void usbDongle_HandleLeVoiceCallControls(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch (id)
    {
        case LE_AUDIO_CLIENT_REMOTE_CALL_CONTROL_CALL_ACCEPT:
            usbDongle_LeVoiceCallRemoteAccept();
            break;

        case LE_AUDIO_CLIENT_REMOTE_CALL_CONTROL_CALL_TERMINATE:
            usbDongle_LeVoiceCallRemoteTerminate();
            break;

        default:
            DEBUG_LOG_FN_ENTRY("usbDongle_HandleLeVoiceCallControls Unhandled Id: %d", id);
            break;
    }
}

bool UsbDongle_LeVoiceConnectedState(void)
{
    return (le_voice_data.state >= APP_LE_VOICE_STATE_CONNECTED);
}

usb_dongle_le_voice_state_t UsbDongle_LeVoiceGetState(void)
{
    return le_voice_data.state;
}

void UsbDongle_LeVoiceInit(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_LeVoiceInit");

    /*Register with usb dongle LEA for connect and disconnect Indications */
    UsbDongle_LeaRegisterConnectionCallbacks(usbDongle_LeVoiceConnected,
                                               usbDongle_LeVoiceDisconnected);

    /* Register to receive voice control commands received from remote side */
    le_voice_data.task.handler = usbDongle_HandleLeVoiceCallControls;
    LeAudioClient_ClientRegister(&le_voice_data.task);

    Ui_RegisterUiProvider(ui_provider_telephony, usbDongle_GetTelephonyStateContext);
    le_voice_data.state = APP_LE_VOICE_STATE_DISCONNECTED;
    le_voice_data.connected_cb = NULL;
    le_voice_data.disconnected_cb = NULL;
}

void UsbDongle_LeVoiceRegisterConnectionCallbacks(void (*connected_cb)(void), void (*disconnected_cb)(void))
{
    le_voice_data.connected_cb = connected_cb;
    le_voice_data.disconnected_cb = disconnected_cb;
}

#endif /* defined(INCLUDE_SOURCE_APP_LE_AUDIO) && defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) */

