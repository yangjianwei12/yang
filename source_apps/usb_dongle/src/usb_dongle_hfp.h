/*!
\copyright  Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for application HFP source module
*/

#ifndef USB_DONGLE_HFP_H
#define USB_DONGLE_HFP_H

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

#include <aghfp_profile.h>
#include <message.h>
#include <device.h>
#include <bdaddr.h>

typedef enum
{
    APP_HFP_STATE_DISCONNECTED,
    APP_HFP_STATE_DISCONNECTING,
    APP_HFP_STATE_CONNECTING,
    APP_HFP_STATE_CONNECTED,
    APP_HFP_STATE_INCOMING,
    APP_HFP_STATE_OUTGOING,
    APP_HFP_STATE_STREAMING
} usb_dongle_hfp_state_t;

typedef struct
{
    TaskData task;
    aghfpInstanceTaskData *active_instance; /*!< The connected AGHFP instance for the current HF */
    usb_dongle_hfp_state_t state;
    Sink sco_sink;
    Sink mic_sink;
    Source sprk_source;
    void (*connected_cb)(void);
    void (*disconnected_cb)(void);
} usb_dongle_hfp_data_t;

extern usb_dongle_hfp_data_t hfp_data;

void UsbDongle_HfpInit(void);

void UsbDongle_HfpConnect(const bdaddr *hf_addr);

void UsbDongle_HfpDisconnect(void);

bool UsbDongle_HfpInConnectedState(void);

void UsbDongle_HfpIncomingVoiceCall(void);

void UsbDongle_HfpRegisterConnectionCallbacks(void (*connected_cb)(void), void (*disconnected_cb)(void));

void UsbDongle_HfpOutgoingCall(void);

void UsbDongle_HfpAnswerOutgoingCall(void);

void UsbDongle_HfpHoldActiveCall(void);

void UsbDongle_HfpReleaseHeldCall(void);

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#endif /* USB_DONGLE_HFP_H */
