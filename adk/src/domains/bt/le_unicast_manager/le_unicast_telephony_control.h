/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_unicast_manager
    \brief      The voice source telephony control interface implementation for unicast voice sources
    @{
*/

#ifndef LE_UNICAST_TELEPHONY_CONTROL_H_
#define LE_UNICAST_TELEPHONY_CONTROL_H_

#include "voice_sources_telephony_control_interface.h"

/*! \brief Gets the LE unicast telephony control interface.

    \return The voice source telephony control interface for an unicast voice source
 */
const voice_source_telephony_control_interface_t * LeVoiceSource_GetTelephonyControlInterface(void);

#endif /* LE_UNICAST_TELEPHONY_CONTROL_H_ */

/*! @} */