/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef CAP_CLIENT_NEW_DEVICE_REQ_H
#define CAP_CLIENT_NEW_DEVICE_REQ_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void capClientInitAddProfileAndServices(CapClientGroupInstance *const cap,
                                         uint32 cid, CapClientHandleList *handles);

void capClientInitCoordinatedSet(CapClientGroupInstance* cap);
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif
