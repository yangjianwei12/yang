/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#ifndef CAP_CLIENT_AVAILABLE_AUDIO_CONTEXT_REQ_H
#define CAP_CLIENT_AVAILABLE_AUDIO_CONTEXT_REQ_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void capClientHandleRemoteAvailableContextCfm(CAP_INST *inst,
                             BapDiscoverAudioContextCfm* cfm,
                             CapClientGroupInstance *gInst);


void handleCapClientDiscoverAvailableAudioContextReq(CAP_INST *inst, const Msg msg);

void capClientSendDiscoverAvailableAudioContextReq(CAP_INST *inst,
                                          BapInstElement *bap,
                                          CapClientGroupInstance *gInst);


void capClientSendDiscoverAvailableAudioContextCfm(CAP_INST *inst,
                                           CapClientGroupInstance *gInst,
                                           CapClientResult result);
#endif /* INSTALL_LEA_UNICAST_CLIENT */
#endif

