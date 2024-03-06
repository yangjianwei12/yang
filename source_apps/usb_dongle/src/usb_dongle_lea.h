/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for USB dongle LE audio.

*/

#ifndef USB_DONGLE_LE_AUDIO_H
#define USB_DONGLE_LE_AUDIO_H

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO

/*! \brief USB Dongle LEA context. */
typedef enum
{
    /*! Initial state before state machine is running. */
    USB_DONGLE_LEA_NONE             = 0,
    USB_DONGLE_LEA_AUDIO            = (1 << 0),
    USB_DONGLE_LEA_ANALOG_AUDIO     = (1 << 1),
    USB_DONGLE_LEA_AUDIO_VBC        = (1 << 2),
    USB_DONGLE_LEA_VOICE            = (1 << 3),
} usb_dongle_lea_context_t;

/*! \brief USB donngle LE state machine internal message IDs */
enum usb_dongle_lea_sm_internal_message_ids
{
    /*!< Process the change in LEA context */
    USB_DONGLE_LEA_SM_INTERNAL_PROCESS_CONTEXT_CHANGE,
};

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) && defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)
/*! \brief Check if switching between broadcast and conventional mode(ie, unicast) is possible */
#define UsbDongle_LeaIsBroadcastModeSwitchingPossible()     TRUE
#else
/*! \brief Check if switching between broadcast and conventional mode(ie, unicast) is possible */
#define UsbDongle_LeaIsBroadcastModeSwitchingPossible()     FALSE
#endif

/*! \brief Add LE context based on USB interface.

    \param lea_ctx The context is based on usb_dongle_lea_context_t.

     Note: When mic interface is activated, context is USB_DONGLE_LEA_AUDIO_VBC.
           When mic interface is activated and HID command received for voice,
           context is USB_DONGLE_LEA_VOICE.
           When speaker interface is activated, context is USB_DONGLE_LEA_AUDIO.
*/
void UsbDongle_LeaAddContext(usb_dongle_lea_context_t lea_ctx);

/*! \brief Remove LE context based on USB interface.

    \param lea_ctx The context is based on usb_dongle_lea_context_t.

     Note: When mic interface is activated, context is USB_DONGLE_LEA_AUDIO_VBC.
           When mic interface is activated and HID command received for voice,
           context is USB_DONGLE_LEA_VOICE.
           When speaker interface is activated, context is USB_DONGLE_LEA_AUDIO.
*/
void UsbDongle_LeaRemoveContext(usb_dongle_lea_context_t lea_ctx);

/*! \brief Get the currently set LE context

    \return LE context which is set currently
*/
usb_dongle_lea_context_t UsbDongle_LeaGetContext(void);

/*! \brief Check if we are in broadcast mode active in LEA SM.

    \return TRUE if broadcast mode enabled, FALSE otherwise
*/
bool UsbDongle_IsLeaBroadcastModeActive(void);

/*! \brief Start LE Audio stream.
*/
void UsbDongle_LeaAudioStart( void );

/*! \brief Stop LE Audio stream.
*/
void UsbDongle_LeaAudioStop( void );

/*! \brief To check if audio context is available in LE connection.

     \return TRUE if audio context is available.
*/
bool UsbDongle_LeaIsAudioAvailable(void);

/*! \brief To check if VBC context is available in LE connection.

     \return TRUE if VBC context is available.
*/
bool UsbDongle_LeaIsVbcAvailable(void);

/*! \brief To check if both voice connection as well as the voice context is available in LE connection.

     \return TRUE if Voice context is available.

     Note: Move this api when usb_dongle_voice.c is available.
*/
bool UsbDongle_LeaIsVoiceAvailable(void);

/*! \brief To check if LEA Voice connection is available

     \return TRUE if Voice connection is available.
*/
bool UsbDongle_LeaIsVoiceConnected(void);

/*! \brief To check if LEA source is active.

     \return TRUE if LEA source is active.
*/
bool UsbDongle_LeaIsSourceActive(void);

/*! \brief To check if Audio source is active.

     \return TRUE if Audio source is active.
*/
bool UsbDongle_LeaIsAudioActive(void);

/*! \brief Check if currently active audio source is analog(wired) or not

     \return TRUE if analog audio is active, FALSE otherwise
*/
bool UsbDongle_LeaIsAnalogAudioActive(void);

/*! \brief To check if Audio source with VBC is active.

     \return TRUE if Audio source with VBC is active.
*/
bool UsbDongle_LeaIsVbcActive(void);

/*! \brief To check if Voice source is active.

     \return TRUE if Voice source is active.
*/
bool UsbDongle_LeaIsVoiceActive(void);

/*! \brief To check context switch between VBC and Audio is required.

     \return TRUE if context switch is required.
*/
bool UsbDongle_LeaContextSwitchIsRequired(void);

/*! \brief Register to receive connect/disconnect notification from LE profile.

    \param connected_cb The callback when LE connection is successful.
    \param disconnected_cb The callback when LE connection is disconnected.
*/
void UsbDongle_LeaRegisterConnectionCallbacks(void (*connected_cb)(void),
                                              void (*disconnected_cb)(void));

/*! \brief Initializes the USB Dongle LE Audio interface.

     \param init_task Task to receive responses.
     \return TRUE if able to initialize, returns FALSE otherwise.
*/
bool UsbDongle_LeaInit(Task task);

/*! \brief Determine it there is context change required when enabling/disabling gaming mode

     \return TRUE if context change is required, returns FALSE otherwise.
*/
bool UsbDongle_LeaIsContextChangeRequired(void);

/*! \brief Switches to voice context if required.
           It will check if dongle is in gaming with VBC or not.
           If yes, it will be switched to voice context.

    \return TRUE if switching to voice context, returns FALSE otherwise.
*/
bool UsbDongle_LeaSwitchToVoiceContextIfRequired(void);

/*! \brief Restart the audio graph

    \param enable_mic TRUE if restarting voice/gaming+vbc

    \return TRUE if switching to voice context, returns FALSE otherwise.
*/
void UsbDongle_LeaRestartAudioGraph(bool enable_mic);

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

/*! \brief To check if audio transport(unicast/broadcast) needs to be switched
           Eg. Broadcast mode is enabled while unicast streaming in progress

     \return TRUE if context switch is required.
*/
bool UsbDongle_LeaAudioTransportSwitchRequired(void);

/*! \brief Handle broadcast mode toggle

    \param is_connected TRUE to indicate device is connected.

    \return TRUE means main SM has to take care of the toggling.
*/
bool UsbDongle_LeaHandleBroadcastModeToggle(bool is_connected);

#else /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

/*! \brief To check if audio transport(unicast/broadcast) needs to be switched */
#define UsbDongle_LeaAudioTransportSwitchRequired()             (FALSE)

#define UsbDongle_LeaHandleBroadcastModeToggle(is_connected)    (FALSE)

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

#endif /* USB_DONGLE_LE_AUDIO_H */
