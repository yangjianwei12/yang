/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef CAP_CLIENT_VCP_OPERATION_REQ_H
#define CAP_CLIENT_VCP_OPERATION_REQ_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void handleChangeVolumeReq(CAP_INST* inst, const Msg msg);

void handleMuteReq(CAP_INST* inst, const Msg msg);

void capClientSetAbsoluteVolumeReq(CsrCmnListElm_t* elem,
                            uint8 volumeSetting,
                            CAP_INST* inst,
                            CapClientGroupInstance* cap);

void capClientSetVcpMuteStateReq(VcpInstElement* vcp,
                            bool mute,
                            CAP_INST* inst,
                            CapClientGroupInstance* cap);

void capClientSendMuteCfm(AppTask appTask,
                            ServiceHandle groupId,
                            CapClientResult result,
                            uint8 deviceCount,
                            VcpInstElement *vcp,
                            CAP_INST* inst);

void capClientSendChangeVolumeCfm(AppTask appTask,
                           ServiceHandle groupId,
                           CapClientResult result,
                           uint8 deviceCount,
                           VcpInstElement *vcp,
                           CAP_INST* inst);

void handleReadVolumeStateReq(CAP_INST* inst, const Msg msg);

void capCLientSendReadVolumeStateCfm(AppTask appTask,
                                    ServiceHandle groupId,
                                    CapClientResult result,
                                    VcpInstElement* vcp,
                                    uint8 mute,
                                    uint8 volumeSetting,
                                    uint8 changeCounter);

void capHandleVcpRequest(CapClientGroupInstance* cap,
                         CAP_INST* inst,
                         void *msg,
                         CapClientVcpMsgType msgType,
                         uint8 vcpIndex);

void capHandleVcpCmdQueue(CapClientGroupInstance* cap,
                          void *msg,
                          CapClientVcpMsgType msgType,
                          uint8 capVcpMsgState,
                          uint8 vcpIndex);
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

#endif
