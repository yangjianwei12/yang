/*******************************************************************************

Copyright (C) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Broadcast Assistant Msg handler.
 */

/**
 * \defgroup BAP_BROADCAST_ASSISTANT BAP
 * @{
 */
#ifndef BAP_BROADCAST_ASSISTANT_MSG_HANDLER_H_
#define BAP_BROADCAST_ASSISTANT_MSG_HANDLER_H_

#include "csr_bt_cm_prim.h"
#include "../bap_client_list_util_private.h"
#include "bap_client_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT

void bapBroadcastAssistantInternalMsgHandler(BAP * const bap, BapUPrim * const primitive);

void bapBroadcastAssistantCmPrimitiveHandler(BAP * const bap, CsrBtCmPrim * const primitive);
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

#ifdef __cplusplus
}
#endif

#endif /* BAP_BROADCAST_ASSISTANT_MSG_HANDLER_H */

/**@}*/


