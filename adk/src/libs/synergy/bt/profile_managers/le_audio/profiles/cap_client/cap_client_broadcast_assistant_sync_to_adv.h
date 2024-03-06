/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#ifndef CAP_CLIENT_BROADCAST_ASSISTANT_SYNC_TO_ADV_H
#define CAP_CLIENT_BROADCAST_ASSISTANT_SYNC_TO_ADV_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
void handleBroadcastAssistantSyncToSrcStartReq(CAP_INST* inst, const Msg msg);

void capClientSendBapBroadcastAssistantStartSynctoSrcReq(BapInstElement *bap);

void capClientHandleBroadcastAssistantSyncToSrcStartCfm(CAP_INST *inst,
                                BapBroadcastAssistantSyncToSrcStartCfm *cfm,
                                CapClientGroupInstance* cap);


void handleBroadcastAssistantTerminateSyncToSrcReq(CAP_INST* inst, const Msg msg);

void capClientHandleBroadcastAssistantSyncToSrcTerminateCfm(CAP_INST *inst,
                                BapBroadcastAssistantSyncToSrcTerminateCfm *cfm,
                                CapClientGroupInstance* cap);

void capClientHandleBroadcastAssistantSyncToSrcCancelCfm(CAP_INST *inst,
                                BapBroadcastAssistantSyncToSrcCancelCfm *cfm,
                                CapClientGroupInstance* cap);

void capClientSendBapBroadcastAssistantSynctoSrcCancelReq(BapInstElement *bap);

void handleBroadcastAssistantSyncToSrcCancelReq(CAP_INST* inst, const Msg msg);
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

#endif
