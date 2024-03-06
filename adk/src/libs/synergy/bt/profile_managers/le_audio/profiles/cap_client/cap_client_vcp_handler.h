/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#ifndef CAP_CLIENT_VCP_HANDLER_H
#define CAP_CLIENT_VCP_HANDLER_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void capClientHandleVcpMsg(CAP_INST *inst,const Msg msg);
void capClientHandleVolumeStateInd(VcpVolumeStateInd *ind, CAP_INST* inst, CapClientGroupInstance* gInst);
void capClientHandleUnMuteCfm(const Msg msg, CAP_INST* inst, CapClientGroupInstance* gInst);
void capClientHandleAbsVolCfm(const Msg msg, CAP_INST* inst, CapClientGroupInstance* gInst);
void capClientHandleMuteCfm(const Msg msg, CAP_INST* inst, CapClientGroupInstance* gInst);
void capClientHandleVolumeStateCfm(const Msg msg, CAP_INST* inst, CapClientGroupInstance* gInst);
void capClientHandleVolumeStateNtfCfm(const Msg msg, CAP_INST* inst, CapClientGroupInstance* gInst);
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif
