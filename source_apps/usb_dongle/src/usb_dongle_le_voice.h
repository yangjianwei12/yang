/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for application LE Voice source module
*/

#ifndef USB_DONGLE_LE_VOICE_H
#define USB_DONGLE_LE_VOICE_H

#if defined(INCLUDE_SOURCE_APP_LE_AUDIO) && defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)

#include <message.h>

/*! \brief LE Unicast Voice states. */
typedef enum
{
    APP_LE_VOICE_STATE_DISCONNECTED,
    APP_LE_VOICE_STATE_DISCONNECTING,
    APP_LE_VOICE_STATE_CONNECTING,
    APP_LE_VOICE_STATE_CONNECTED,
    APP_LE_VOICE_STATE_INCOMING,
    APP_LE_VOICE_STATE_OUTGOING,
    APP_LE_VOICE_STATE_STREAMING
} usb_dongle_le_voice_state_t;

/*! \brief USB Dongle LE Unicast Voice  data structure */
typedef struct
{
    TaskData task;                      /*!< LE Voice task */
    usb_dongle_le_voice_state_t state;  /*!< Current state of LE unicast voice */
    void (*connected_cb)(void);         /*!< Callback for connected state. */
    void (*disconnected_cb)(void);      /*!< Callback for dis connected state. */
} usb_dongle_le_voice_data_t;

extern usb_dongle_le_voice_data_t le_voice_data;

/*! \brief Interface to update LE Voice call has ended
 */
void UsbDongle_LeVoiceCallEnded(void);

/*! \brief Interface to update LE Voice call is active
 */
void UsbDongle_LeVoiceCallStart(void);

/*! \brief Interface to update an Incoming LE Voice call
 */
void UsbDongle_LeVoiceIncomingCall(void);

/*! \brief Interface to update an Outgoing LE Voice call
 */
void UsbDongle_LeVoiceOutgoingCall(void);

/*! \brief Initialise the LE voice interface, register with LE unicast
           for connection update and as a UI provider for telephony.

 */
void UsbDongle_LeVoiceInit(void);

/*! \brief Checks if the LE unicast Voice is connected

     \return TRUE if LE unicast Voice is connected, returns FALSE otherwise.
 */
bool UsbDongle_LeVoiceConnectedState(void);

/*! \brief Gets the LE unicast Voice state

     \return TRUE if LE unicast Voice is connected, returns FALSE otherwise.
 */
usb_dongle_le_voice_state_t UsbDongle_LeVoiceGetState(void);

/*! \brief Register to receive connect/disconnect notification for LE Unicast Voice.

    \param connected_cb The callback when LE Voice connection is successful.
    \param disconnected_cb The callback when LE Voice connection is disconnected.
*/
void UsbDongle_LeVoiceRegisterConnectionCallbacks(void (*connected_cb)(void), void (*disconnected_cb)(void));

#else /* defined(INCLUDE_SOURCE_APP_LE_AUDIO) && defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) */

#define UsbDongle_LeVoiceInit()
#define UsbDongle_LeVoiceCallStart()
#define UsbDongle_LeVoiceCallEnded()

#endif /* defined(INCLUDE_SOURCE_APP_LE_AUDIO) && defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) */

#endif /* USB_DONGLE_LE_VOICE_H */

