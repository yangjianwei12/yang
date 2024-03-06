/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#ifndef CAP_CLIENT_REMOVE_DEVICE_REQ_H
#define CAP_CLIENT_REMOVE_DEVICE_REQ_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void capClientRemoveGroup(Msg msg, CAP_INST* const inst, CapClientProfile profile);
void handleCapClientRemoveDeviceReq(CAP_INST* const inst, const Msg msg);

void deInitBapList(CsrCmnListElm_t *elem);

void deInitVcpList(CsrCmnListElm_t *elem);

void deInitCsipList(CsrCmnListElm_t *elem);
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif
