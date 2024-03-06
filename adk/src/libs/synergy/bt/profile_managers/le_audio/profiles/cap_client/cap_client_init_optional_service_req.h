/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #
******************************************************************************/

#ifndef CAP_CLIENT_INIT_OPTIONAL_SERVICE_REQ_H
#define CAP_CLIENT_INIT_OPTIONAL_SERVICE_REQ_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void handleCapClientInitOptionalServicesReq(CAP_INST *inst, const Msg msg);

void capClientSendOptionalServiceInitCfm(AppTask appTask,
                                  uint8 deviceCount,
                                  ServiceHandle groupId,
                                  uint32 services,
                                  CapClientResult result,
                                  CsrCmnListElm_t* elem);
#endif

#ifndef EXCLUDE_CSR_BT_MICP_MODULE

void capClientInitInitializeMicpProfile(CapClientGroupInstance *const cap,
                                       GattMicsClientDeviceData *handles,
                                       uint32 cid);

#endif
#endif
