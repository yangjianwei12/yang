/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#ifndef CAP_CLIENT_UPDATE_AUDIO_REQ_H
#define CAP_CLIENT_UPDATE_AUDIO_REQ_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void capClientSendUnicastAudioUpdateReq(BapInstElement* bap,
                               CapClientGroupInstance* cap,
                               CAP_INST* inst);


void capClientUpdateAudioCfmSend(AppTask appTask,
                                CAP_INST* inst,
                                CapClientResult result,
                                CapClientContext useCase,
                                BapInstElement* bap,
                                CapClientGroupInstance* cap);

void handleUnicastUpdateAudioReq(CAP_INST* inst, const Msg msg);

#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif
