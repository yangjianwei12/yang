/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#ifndef CAP_CLIENT_BROADCAST_SRC_H
#define CAP_CLIENT_BROADCAST_SRC_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_BROADCAST_SOURCE

void capClientHandleBroadcastSourceMsg(CAP_INST* inst, const Msg msg);

void handleBroadcastSrcInitReq(CAP_INST* inst, const Msg msg);

void handleBroadcastSrcDeinitReq(CAP_INST* inst, const Msg msg);

void  capClientRemoveBcastSrcInst(CAP_INST* inst, const Msg msg);

void handleBroadcastSrcConfigReq(CAP_INST* inst, const Msg msg);

void handleBroadcastSrcStartStreamReq(CAP_INST* inst, const Msg msg);

void handleBroadcastSrcStopStreamReq(CAP_INST* inst, const Msg msg);

void handleBroadcastSrcRemoveStreamReq(CAP_INST* inst, const Msg msg);

void handleBroadcastSrcUpdateStreamReq(CAP_INST* inst, const Msg msg);

void capClientSendBroadcastSrcInitCfm(AppTask appTask, uint32 pHandle, CapClientResult result);

void capClientHandleBapBroadcastInitCfm(CAP_INST* inst, const BapInitCfm* cfm);

void capClientHandleBroadcastSrcConfigureStreamCfm(CAP_INST *inst,
                                       BapBroadcastSrcConfigureStreamCfm *cfm);

void capClientHandleBroadcastSrcEnableStreamCfm(CAP_INST* inst,
                                       BapBroadcastSrcEnableStreamCfm* cfm);

void capClientBroadcastHandleSetupDataPathCfm(CAP_INST *inst,
                              BapSetupDataPathCfm *cfm);

void capClientBcastSourceStartStreamCfm(CAP_INST       *inst,
                                       uint32             handle,
                                       CapClientResult    result,
                                       BroadcastSrcInst* brcstSrc);

void capClientHandleBroadcastSrcStopStreamCfm(CAP_INST* inst,
                                       BapBroadcastSrcDisableStreamCfm* cfm);

void capClientHandleBroadcastSrcRemoveStreamCfm(CAP_INST* inst,
                                       BapBroadcastSrcReleaseStreamCfm* cfm);


void capClientHandleBroadcastSrcUpdateStreamCfm(CAP_INST *inst,
                                       BapBroadcastSrcUpdateMetadataStreamCfm *cfm);

#endif /* INSTALL_LEA_BROADCAST_SOURCE*/

#endif
