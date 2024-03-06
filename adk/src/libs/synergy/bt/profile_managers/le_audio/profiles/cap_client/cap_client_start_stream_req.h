/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef CAP_CLIENT_START_STREAM_REQ_H
#define CAP_CLIENT_START_STREAM_REQ_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void handleUnicastStartStreamReq(CAP_INST  *inst, const Msg msg);

void capClientBapAseEnableReqSend(CsipInstElement* csip,
    BapInstElement* bap,
    CapClientGroupInstance* cap,
    CAP_INST* inst);

void capClientBapAseEnableReq(BapInstElement* bap,
                             CapClientGroupInstance* cap,
                             CAP_INST* inst);

void capClientHandleUnicastAseEnableInd(CAP_INST *inst,
                             BapUnicastClientEnableInd* ind,
                             CapClientGroupInstance *cap);

void capClientHandleUnicastAseEnableCfm(CAP_INST *inst,
                             BapUnicastClientEnableCfm* cfm,
                             CapClientGroupInstance *cap);

void capClientHandleUnicastCisConnectInd(CAP_INST *inst,
                             BapUnicastClientCisConnectInd* ind,
                             CapClientGroupInstance *cap);

void capClientHandleUnicastCisConnectCfm(CAP_INST *inst,
                             BapUnicastClientCisConnectCfm* cfm,
                             CapClientGroupInstance *cap);

void capClientHandleUnicastRecieverStartReadyInd(CAP_INST *inst,
                                 BapUnicastClientReceiverReadyInd* ind,
                                 CapClientGroupInstance *cap);

void capClientHandleUnicastRecieverStartReadycfm(CAP_INST *inst,
                                          BapUnicastClientReceiverReadyCfm *cfm,
                                          CapClientGroupInstance*  cap);

void capClientSendUnicastStartStreamCfm(AppTask appTask,
                                      CAP_INST *inst,
                                      CapClientGroupInstance *gInst,
                                      CapClientResult result);

void capClientSendUnicastStartStreamInd(CAP_INST* inst,
                                        CapClientGroupInstance* gInst,
                                        BapInstElement* bap,
                                        CapClientResult result);

#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif

