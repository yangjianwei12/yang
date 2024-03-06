/*!
\copyright  Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for application HFP source module
*/

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
#include "usb_dongle_logging.h"

#include "usb_dongle_hfp.h"

#include <aghfp_profile.h>
#include <device_types.h>
#include <device_list.h>
#include <device_properties.h>
#include <voice_sources.h>
#include <kymera.h>
#include <kymera_adaptation_voice_protected.h>
#include <ui.h>
#include <kymera_usb_sco.h>
#include <panic.h>

usb_dongle_hfp_data_t hfp_data;

static unsigned usbDongle_GetTelephonyStateContext(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_GetTelephonyStateContext");
    voice_source_provider_context_t context = BAD_CONTEXT;

    switch(hfp_data.state)
    {       
        case APP_HFP_STATE_STREAMING:
            context = context_voice_in_call;
            break;

        case APP_HFP_STATE_INCOMING:
            context = context_voice_ringing_incoming;
            break;

        case APP_HFP_STATE_CONNECTED:
            context = context_voice_connected;
            break;

        default:
            context = context_voice_connected;
            break;
     }

    return (unsigned)context;
}

static void usbDongle_HandleScoConnection(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleScoConnection");

    if (hfp_data.state == APP_HFP_STATE_OUTGOING && hfp_data.active_instance)
    {
        AghfpProfile_OutgoingCallAnswered(&hfp_data.active_instance->hf_bd_addr);
    }

    hfp_data.state = APP_HFP_STATE_STREAMING;
    Ui_InformContextChange(ui_provider_telephony, usbDongle_GetTelephonyStateContext());

}


static void usbDongle_HandleScoDisonnect(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleScoDisonnect");
}

static void usbDongle_HandleHfpConnected(APP_AGHFP_CONNECTED_IND_T *ind)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleHfpConnected");
    hfp_data.active_instance = ind->instance;
    hfp_data.state = APP_HFP_STATE_CONNECTED;
    hfp_data.connected_cb();
}

static void usbDongle_HandleHfpDisconnected(APP_AGHFP_DISCONNECTED_IND_T *ind)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleHfpDisconnected");
    hfp_data.active_instance = NULL;
    hfp_data.state = APP_HFP_STATE_DISCONNECTED;
    hfp_data.disconnected_cb();
    UNUSED(ind);
}

static void usbDongle_HandleHfpIncomingRing(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleHfpIncomingRing");

    hfp_data.state = APP_HFP_STATE_INCOMING;
    Ui_InformContextChange(ui_provider_telephony, usbDongle_GetTelephonyStateContext());
}

static void usbDongle_HandleHfpSlcStatusInd(APP_AGHFP_SLC_STATUS_IND_T *ind)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleHfpSlcStatusInd");

    if (ind->slc_connected == FALSE)
    {
        hfp_data.disconnected_cb();
    }
}

static void usbDongle_HandleCallStart(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleCallStart");
}

static void usbDongle_HandleCallEnd(void)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_HandleCallEnd");

    hfp_data.state = APP_HFP_STATE_CONNECTED;
    Ui_InformContextChange(ui_provider_telephony, usbDongle_GetTelephonyStateContext());
}

static void usbDongle_HandleHfpMessage(Task task, MessageId id, Message message)
{
    UNUSED(task); UNUSED(message);

    switch (id)
    {
    case APP_AGHFP_CONNECTED_IND:
        usbDongle_HandleHfpConnected((APP_AGHFP_CONNECTED_IND_T*)message);
        break;

    case APP_AGHFP_SCO_CONNECTED_IND:
        usbDongle_HandleScoConnection();
        break;

    case APP_AGHFP_SCO_DISCONNECTED_IND:
        usbDongle_HandleScoDisonnect();
        break;

    case APP_AGHFP_DISCONNECTED_IND:
        usbDongle_HandleHfpDisconnected((APP_AGHFP_DISCONNECTED_IND_T*)message);
        break;

    case APP_AGHFP_SCO_INCOMING_RING_IND:
        usbDongle_HandleHfpIncomingRing();
        break;

    case APP_AGHFP_SLC_STATUS_IND:
        usbDongle_HandleHfpSlcStatusInd((APP_AGHFP_SLC_STATUS_IND_T*)message);
        break;

    case  APP_AGHFP_CALL_START_IND:
        usbDongle_HandleCallStart();
        break;

    case  APP_AGHFP_CALL_END_IND:
        usbDongle_HandleCallEnd();
        break;

     default:
        DEBUG_LOG("usbDongle_HandleHfpMessage, unrecognised message %d", id);
    }
}

void UsbDongle_HfpInit(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_HfpInit");

    hfp_data.task.handler = usbDongle_HandleHfpMessage;

    AghfpProfile_RegisterStatusClient(&hfp_data.task);

    Ui_RegisterUiProvider(ui_provider_telephony, usbDongle_GetTelephonyStateContext);
    hfp_data.state = APP_HFP_STATE_DISCONNECTED;
    hfp_data.active_instance = NULL;
    hfp_data.connected_cb = NULL;
    hfp_data.disconnected_cb = NULL;
}

void UsbDongle_HfpConnect(const bdaddr *hf_addr)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_HfpConnect");

    if (hf_addr == NULL)
    {
        DEBUG_LOG_WARN("UsbDongle_HfpConnect, cannot connect HFP, invalid HF address");
        return;
    }
    if (hfp_data.state == APP_HFP_STATE_DISCONNECTED)
    {
        DEBUG_LOG_DEBUG("UsbDongle_HfpConnect, requesting HFP Connect, bdaddr %04x,%02x,%06lx",
                        hf_addr->nap, hf_addr->uap, hf_addr->lap);

        AghfpProfile_Connect(hf_addr);
        hfp_data.state = APP_HFP_STATE_CONNECTING;
    }
    else
    {
        DEBUG_LOG_WARN("UsbDongle_HfpConnect, cannot connect HFP, existing connection ongoing");
    }
}

void UsbDongle_HfpDisconnect(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_HfpDisconnect");

    if (hfp_data.active_instance)
    {
        AghfpProfile_Disconnect(&hfp_data.active_instance->hf_bd_addr);
        hfp_data.state = APP_HFP_STATE_DISCONNECTING;
    }
    else
    {
        DEBUG_LOG_WARN("UsbDongle_HfpDisconnect, cannot disconnect HFP, no instance");
        hfp_data.state = APP_HFP_STATE_DISCONNECTED;
    }
}

void UsbDongle_HfpIncomingVoiceCall(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_HfpIncomingVoiceCall");

    if (hfp_data.active_instance)
    {
        AghfpProfile_CallIncomingInd(&hfp_data.active_instance->hf_bd_addr);
    }
}

bool UsbDongle_HfpInConnectedState(void)
{
    return (hfp_data.state >= APP_HFP_STATE_CONNECTED);
}

void UsbDongle_HfpRegisterConnectionCallbacks(void (*connected_cb)(void), void (*disconnected_cb)(void))
{
    hfp_data.connected_cb = connected_cb;
    hfp_data.disconnected_cb = disconnected_cb;
}

void UsbDongle_HfpOutgoingCall(void)
{
    if (hfp_data.active_instance)
    {
        AghfpProfile_CallOutgoingInd(&hfp_data.active_instance->hf_bd_addr);
    }
    hfp_data.state = APP_HFP_STATE_OUTGOING;
}

void UsbDongle_HfpAnswerOutgoingCall(void)
{
    if (hfp_data.state == APP_HFP_STATE_OUTGOING && hfp_data.active_instance)
    {
        AghfpProfile_OutgoingCallAnswered(&hfp_data.active_instance->hf_bd_addr);
    }
}

void UsbDongle_HfpHoldActiveCall(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_HfpHoldActiveCall");

    if (hfp_data.active_instance)
    {
        AghfpProfile_HoldActiveCall(&hfp_data.active_instance->hf_bd_addr);
    }
}

void UsbDongle_HfpReleaseHeldCall(void)
{
    DEBUG_LOG_FN_ENTRY("UsbDongle_HfpReleaseHeldCall");

    if (hfp_data.active_instance)
    {
        AghfpProfile_ReleaseHeldCall(&hfp_data.active_instance->hf_bd_addr);
    }
}

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

