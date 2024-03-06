/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #
******************************************************************************/

#ifndef CAP_CLIENT_MICP_HANDLER_H
#define CAP_CLIENT_MICP_HANDLER_H

#include "cap_client_private.h"

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
void capClientHandleMicpMsg(CAP_INST *inst,const Msg msg);
void capClientSetMicpMuteStateReq(MicpInstElement* micp,
                                 CAP_INST *inst,
                                 CapClientGroupInstance *cap);

void capClientMicpEnableCcd(CAP_INST *inst, uint8 enable);

#endif
#endif
