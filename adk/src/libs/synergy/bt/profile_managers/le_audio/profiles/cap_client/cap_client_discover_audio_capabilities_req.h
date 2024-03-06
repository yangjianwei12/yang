/******************************************************************************
 Copyright (c) 2021- 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/
#ifndef CAP_CLIENT_DISCOVER_AUDIO_CAPABILITIES_REQ_H
#define CAP_CLIENT_DISCOVER_AUDIO_CAPABILITIES_REQ_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void handleCapClientDiscoverStreamCapabilitiesReq(CAP_INST *inst,const Msg msg);

void capClientSendDiscoverCapabilityReq(CapClientPublishedCapability capability,
                                  BapInstElement *bap,
                                  bool isSink,
                                  CAP_INST *inst,
                                  uint8 setSize, CapClientGroupInstance* gInst);

void capClientSendDiscoverStreamCapabilitiesCfm(CapClientGroupInstance* cap,
                                  CapClientResult result,
                                  CAP_INST* inst);
#endif /* #INSTALL_LEA_UNICAST_CLIENT */
#endif
