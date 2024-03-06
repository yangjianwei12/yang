/*******************************************************************************

Copyright (C) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Broadcast GATT client BASS service handler
 */

/**
 * \defgroup BAP_BROADCAST_GATT_CLIENT BAP
 * @{
 */
#ifndef BAP_BROADCAST_GATT_CLIENT_BASS_H_
#define BAP_BROADCAST_GATT_CLIENT_BASS_H_

#include "../bap_client_list_util_private.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT

void handleBassPrimitive(BAP* const bap, uint16 primitive_id, void* primitive);
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

#ifdef __cplusplus
}
#endif

#endif /* BAP_BROADCAST_GATT_CLIENT_BASS_H_ */

/**@}*/


