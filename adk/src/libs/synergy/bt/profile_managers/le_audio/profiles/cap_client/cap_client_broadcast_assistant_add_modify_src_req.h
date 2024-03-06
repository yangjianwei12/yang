/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef CAP_CLIENT_BROADCAST_ASSISTANT_ADD_MODIFY_SRC_REQ_H
#define CAP_CLIENT_BROADCAST_ASSISTANT_ADD_MODIFY_SRC_REQ_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
void handleBroadcastAssistantAddSrcReq(CAP_INST* inst, const Msg msg);

void capClientBcastAsstAddSrcReqSend(BapInstElement* bap,
                                    uint32 cid,
                                    AppTask appTask);

void capClientBcastAsstmodifySrcReqSend(BapInstElement* bap,
                                       uint32 cid,
                                       AppTask appTask);

void capClientHandleBroadcastAssistantAddModifySrcCfm(CAP_INST* inst,
                                                Msg msg,
                                                CapClientGroupInstance* cap,
                                                CapClientPrim type);

void capClientSendBroadcastAssistantCommCfm(AppTask profileTask,
                                     CapClientResult result,
                                     ServiceHandle groupId,
                                     BapProfileHandle bapHandle,
                                     CapClientPrim type);

void capClientSendBroadcastAssistantModifySrcReq(BapInstElement* bap);

void handleBroadcastAssistantModifySrcReq(CAP_INST* inst, const Msg msg);

void capClientHandleBassBrsInd(CAP_INST* inst, 
                             BapBroadcastAssistantBrsInd* ind,
                             CapClientGroupInstance* cap);

void capClientHandleBassSyncLossInd(CAP_INST* inst, BapBroadcastAssistantSyncLossInd* ind);

void handleBroadcastAssistantSetBroadcastCodeRsp(CAP_INST* inst, const Msg msg);
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

#endif
