/******************************************************************************
 Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

#ifndef CAP_CLIENT_UNICAST_DISCONNECT_REQ_H
#define CAP_CLIENT_UNICAST_DISCONNECT_REQ_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

void handleUnicastDisconnectReq(CAP_INST* inst, const Msg msg);

void capClientSendUnicastClientDisConnectCfm(AppTask appTask,
                                            CAP_INST* inst,
                                            CapClientGroupInstance* cap,
                                            CapClientResult result);

#endif
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */



