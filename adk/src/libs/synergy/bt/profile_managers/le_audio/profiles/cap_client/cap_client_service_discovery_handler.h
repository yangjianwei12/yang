/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef CAP_CLIENT_SERVICE_DISCOVERY_HANDLER_H
#define CAP_CLIENT_SERVICE_DISCOVERY_HANDLER_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void capClientHandleGattSrvcDiscMsg(CAP_INST  *const inst, Msg msg);
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif
