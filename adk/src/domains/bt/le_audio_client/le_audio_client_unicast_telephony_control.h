/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      The voice source telephony control interface implementation for LE Unicast vocie source
*/

#ifndef LE_AUDIO_CLIENT_VOICE_TELEPHONY_CONTROL_H
#define LE_AUDIO_CLIENT_VOICE_TELEPHONY_CONTROL_H

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE

#include "voice_sources_telephony_control_interface.h"

/*! \brief Gets the LE Voice telephony control interface.

    \return The voice source telephony control interface for an LE Voice source
 */
const voice_source_telephony_control_interface_t * LeAudioClient_GetTelephonyControlInterface(void);

#else /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#define LeAudioClient_GetTelephonyControlInterface() (NULL)

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#endif /* LE_AUDIO_CLIENT_VOICE_TELEPHONY_CONTROL_H */

