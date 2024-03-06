/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#ifndef CAP_CLIENT_BROADCAST_ASSISTANT_REMOVE_SRC_REQ_H
#define CAP_CLIENT_BROADCAST_ASSISTANT_REMOVE_SRC_REQ_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
void handleBroadcastAssistantRemoveSrcReq(CAP_INST* inst, const Msg msg);

void capClientHandleBroadcastAssistantRemoveSrcCfm(CAP_INST* inst,
                             Msg msg,
                             CapClientGroupInstance* cap);

void capClientBcastAsstRemoveSrcReqSend(BapInstElement* bap,
                                       uint32 cid,
                                       AppTask appTask);


#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

#endif
