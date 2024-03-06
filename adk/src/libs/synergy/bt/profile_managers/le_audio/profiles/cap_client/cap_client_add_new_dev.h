/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef CAP_CLIENT_ADD_NEW_DEV_H
#define CAP_CLIENT_ADD_NEW_DEV_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

void handleCapClientAddnewDevReq(CAP_INST  *const inst,const Msg msg);

void capClientSendAddNewDeviceCfm( ServiceHandle groupId,
                             uint8 deviceCount,
                             CapClientResult result,
                             AppTask appTask,
                             CapClientGroupInstance *elem);
#endif /* INSTALL_LEA_UNICAST_CLIENT */
#endif
