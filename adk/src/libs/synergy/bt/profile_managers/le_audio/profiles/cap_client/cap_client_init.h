/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef CAP_CLIENT_INIT_H
#define CAP_CLIENT_INIT_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void handleCapClientInitReq(CAP_INST  *inst, Msg msg);

void capClientSendInitCfm(CAP_INST const *inst, CapClientResult result);
#endif /* INSTALL_LEA_UNICAST_CLIENT */
#endif

