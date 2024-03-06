/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/
#ifndef CAP_CLIENT_INIT_STREAM_AND_CONTROL_REQ_H
#define CAP_CLIENT_INIT_STREAM_AND_CONTROL_REQ_H
#include "cap_client_private.h"
#ifdef INSTALL_LEA_UNICAST_CLIENT
void handleCapClientInitializeStreamControlReq(CAP_INST *inst, const Msg msg);
void capClientInitializeVcpInstance(CAP_INST *inst);
void capClientSendStreamControlInitCfm(CAP_INST *inst,
                                  bool vcpInitialised,
                                  CapClientResult result,
                                  CapClientRole role,
                                  CsrCmnListElm_t* elem);
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif
