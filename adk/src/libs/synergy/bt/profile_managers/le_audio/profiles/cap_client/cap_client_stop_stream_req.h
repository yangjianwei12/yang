/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef CAP_CLIENT_STOP_STREAM_REQ_H
#define CAP_CLIENT_STOP_STREAM_REQ_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

void handleUnicastStopStreamReq(CAP_INST* inst, const Msg msg);


void capClientSendUnicastStopStreamCfm(AppTask appTask,
                                       CAP_INST *inst,
                                       CapClientResult result,
                                       bool clientReleased,
                                       BapInstElement *bap,
                                       CapClientGroupInstance *cap);

void capClientHandleDatapathRemoveCfm(CAP_INST *inst,
                      BapRemoveDataPathCfm *cfm,
                      CapClientGroupInstance *cap);

void capClientHandleBapAseRecieverStopReadyCfm(CAP_INST *inst,
                             BapUnicastClientReceiverReadyCfm* cfm,
                             CapClientGroupInstance *cap);

void capClientHandleBapAseRecieverStopReadyInd(CAP_INST* inst,
                         BapUnicastClientReceiverReadyInd *ind,
                         CapClientGroupInstance *cap);

void capClientHandleUnicastAseDisableCfm(CAP_INST *inst,
                             BapUnicastClientDisableCfm* cfm,
                             CapClientGroupInstance *cap);

void capClientHandleUnicastAseReleaseCfm(CAP_INST *inst,
                             BapUnicastClientReleaseCfm* cfm,
                             CapClientGroupInstance *cap);

void capClientHandleBapAseReleaseInd(CAP_INST* inst,
                         BapUnicastClientReleaseInd *ind,
                         CapClientGroupInstance *cap);

void capClientHandleUnicastBapAseDisableInd(CAP_INST* inst,
                         BapUnicastClientDisableInd *ind,
                         CapClientGroupInstance *cap);

void capClientUnicastDisableReleaseReq(CapClientGroupInstance *cap, CAP_INST *inst);

void capClientHandleCisDisconnectCfm(CAP_INST *inst,
                               BapUnicastClientCisDisconnectCfm *cfm,
                               CapClientGroupInstance *cap);

void capClientHandleCisDisconnectInd(CAP_INST *inst,
                              BapUnicastClientCisDisconnectInd* ind,
                              CapClientGroupInstance* cap);

void capClientHandleRemoveCigCfm(CAP_INST* inst,
                       BapUnicastClientRemoveCigCfm* cfm,
                       CapClientGroupInstance* cap);

void capClientHandleBapAseReleasedInd(CAP_INST* inst,
                                  BapUnicastClientReleasedInd* ind,
                                  CapClientGroupInstance* cap);
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif
