/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef CAP_CLIENT_UNICAST_CONNECT_REQ_H
#define CAP_CLIENT_UNICAST_CONNECT_REQ_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void handleUnicastConnectionReq(CAP_INST* inst, const Msg msg);

void handleUnicastCigTestConfigReq(CAP_INST  *inst, const Msg msg);

void handleUnicastSetVsConfigDataReq(CAP_INST  *inst, const Msg msg);


void handleRegisterTaskReq(CAP_INST* inst, const Msg msg);

void capClientSendRegisterTaskCfm(AppTask profileTask,
                             ServiceHandle groupId,
                             CapClientResult result);



void capClientSendAllUnicastConfigCodecReq(CapClientGroupInstance *cap,
                                    CsipInstElement *csip,
                                    BapInstElement *bap,
                                    CAP_INST *inst);

void capClientHandleUnicastCodecConfigureInd(CAP_INST *inst,
                                     BapUnicastClientCodecConfigureInd* cfm,
                                     CapClientGroupInstance *cap);

void capClientHandleUnicastCodecConfigureCfm(CAP_INST *inst,
                                     BapUnicastClientCodecConfigureCfm* ind,
                                     CapClientGroupInstance *cap);

void capClientHandleUnicastQosConfigureInd(CAP_INST *inst,
                                     BapUnicastClientQosConfigureInd* ind,
                                     CapClientGroupInstance *cap);

void capClientHandleUnicastQosConfigureCfm(CAP_INST *inst,
                                     BapUnicastClientQosConfigureCfm* cfm,
                                     CapClientGroupInstance *cap);

void capClientSendUnicastClientConnectCfm(AppTask appTask,
                                   CAP_INST* inst,
                                   CapClientGroupInstance *gInst,
                                   CapClientResult result);

void capClientHandleUnicastCigConfigureCfm(CAP_INST *inst,
                    BapUnicastClientCigConfigureCfm* cfm,
                    CapClientGroupInstance *cap);

BapUnicastClientCigParameters* capClientPopulateCigConfigReqQuery(CapClientGroupInstance *cap,
    BapInstElement *bap,
    CapClientCigElem* cig);

void capClientSendBapCigConfigureReq(CAP_INST *inst,
    CapClientGroupInstance *cap,
    BapInstElement *bap);

BapAseCodecConfiguration* capClientBuildCodecConfigQueryReq(uint8* sourceAseCount,
    uint8* sinkAseCount,
    BapInstElement *bap,
    CapClientGroupInstance* cap,
    CapClientCigElem *cig);

void capClientSendBapUnicastConfigCodecReq(CapClientGroupInstance *cap,
                     BapInstElement *bap,
                     CapClientCigElem *cig,
                     CAP_INST *inst);


void handleDeRegisterTaskReq(CAP_INST* inst, const Msg msg);

void capClientSendDeRegisterTaskCfm(AppTask profileTask,
                              ServiceHandle groupId,
                              CapClientResult result);


void capClientSendUnicastCigTestCfm(CAP_INST *inst,
                                   CapClientGroupInstance *gInst,
                                   CapClientResult result);

void capClientSendUnicastSetVsConfigDataCfm(CAP_INST *inst,
                                   CapClientGroupInstance *gInst,
                                   CapClientResult result);

void capClientHandleUnicastCigTestConfigureCfm(CAP_INST *inst,
                    BapUnicastClientCigTestConfigureCfm* cfm,
                    CapClientGroupInstance *cap);

void capClientUnicastSetUpDataPathReqSend(BapInstElement* bap,
                                         CAP_INST* inst,
                                         CapClientGroupInstance* cap);


void capClientHandleSetupDataPathCfm(CAP_INST* inst,
                                    BapSetupDataPathCfm* cfm,
                                    CapClientGroupInstance* cap);
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

void handleSetParamReq(CAP_INST* inst, const Msg msg);

void capClientBcastSrcSetConfigParam(CAP_INST* inst, CapClientInternalSetParamReq* req);

#endif




