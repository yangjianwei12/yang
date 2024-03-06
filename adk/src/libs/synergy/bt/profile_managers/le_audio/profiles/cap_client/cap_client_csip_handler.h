/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef CAP_CLIENT_CSIP_HANDLER_H
#define CAP_CLIENT_CSIP_HANDLER_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void capClientHandleCsipMsg(CAP_INST *inst, Msg msg);

void handleCoordinatedSetUnlockReq(CAP_INST* inst, const Msg msg);

void capClientCsipEnableCcd(CAP_INST* inst, uint8 enable);

void handleCsipReadLock(CAP_INST* inst, const Msg msg);
#endif /* INSTALL_LEA_UNICAST_CLIENT */
#endif
