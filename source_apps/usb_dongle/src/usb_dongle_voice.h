/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      USB Dongle application voice interface

*/

#ifndef USB_DONGLE_VOICE_H
#define USB_DONGLE_VOICE_H

#include "voice_sources_list.h"

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO)

void UsbDongle_VoiceStart(void);

void UsbDongle_VoiceStop(void);

#elif defined(INCLUDE_SOURCE_APP_LE_AUDIO)

#define UsbDongle_VoiceStart()
#define UsbDongle_VoiceStop()

#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

/*! \brief Restart voice graph. Usually to apply updated sample rates
 */
void UsbDongle_VoiceRestartGraph(void);

/*! \brief Check if voice profile connection is present or not

    \return TRUE if voice profile connected, FALSE otherwise
 */
bool UsbDongle_VoiceIsSourceConnected(void);

/*! \brief Check if Voice endpoints are avaiable at peer device or not.
 */
bool UsbDongle_VoiceIsSourceAvailable(void);

/*! \brief Inform peer device to enable Voice interface.
 */
void UsbDongle_VoiceConnect(const bdaddr *sink_addr);

/*! \brief Inform peer device to disable Voice interface.
 */
void  UsbDongle_VoiceDisconnect(void);

/*! \brief Connect voice call.

     \return TRUE if voice call can be connected
*/
bool UsbDongle_VoiceStreamConnect(void);

/*! \brief Disconnect voice call.

     \return TRUE if voice call is disconnected
*/
bool UsbDongle_VoiceStreamDisconnect(void);

/*! \brief Get the current voice source

    \return voice source
*/
voice_source_t UsbDongle_VoiceGetCurrentVoiceSource(void);

/*! \brief Check if a voice call is active or not

    \return TRUE if voice call is active, FALSE otherwise
*/
bool UsbDongle_VoiceIsCallActive(void);

/*! \brief Handle an incoming voice call
*/
void UsbDongle_VoiceIncomingVoiceCall(void);

#endif /* USB_DONGLE_VOICE_H */
