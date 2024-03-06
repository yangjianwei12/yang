/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef CAP_CLIENT_COMMON_H
#define CAP_CLIENT_COMMON_H

#include "cap_client_private.h"

uint8 capClientGetFrameDurationFromCapability(CapClientSreamCapability config);

uint16 capClientGetSduSizeFromCapability(CapClientSreamCapability config, CapClientCigConfigMode mode, uint8 locCount, bool isJointStereoInSink);

uint16 capClientGetSamplingFreqFromCapability(CapClientSreamCapability config);

uint8 capClientGetMaxLatencyFromCapability(CapClientSreamCapability config,
                             CapClientTargetLatency highReliability,
                             bool isJointStereoInSink);

uint8 capClientGetFramingForCapability(CapClientSreamCapability config);

uint32 capClientGetSduIntervalForCapability(CapClientSreamCapability config, bool isJointStereoInSink);

uint8 capClientGetRtnFromCapability(CapClientSreamCapability config,
                             CapClientTargetLatency highReliablity);

uint8 capClientGetCodecIdFromCapability(CapClientSreamCapability config);

uint16 capClientGetVendorCodecIdFromCapability(CapClientSreamCapability config);

uint16 capClientGetCompanyIdFromCapability(CapClientSreamCapability config);

uint8 capClientGetMaxLatencyFromCapabilityBcast(CapClientSreamCapability config, CapClientTargetLatency highReliability);

uint8 capClientGetRtnFromCapabilityBcast(CapClientSreamCapability config, CapClientTargetLatency highReliablity);

void capClientSendSetParamCfm(AppTask appTask, CapClientResult result, uint32 profileHandle);

#ifdef INSTALL_LEA_UNICAST_CLIENT

void capClientSetCsisLockState(CsipInstElement* csip,
                              uint8* csipRequestCount,
                              bool lockState);

void capClientSendActiveGroupChangeInd(ServiceHandle newGroupId,
                             ServiceHandle previous,
                             AppTask appTask);

CapClientGroupInstance* capClientSetNewActiveGroup(CAP_INST *inst,
                             ServiceHandle newGroupId,
                             bool discoveryComplete);

void capClientBuildCodecConfigQuery(CapClientGroupInstance *cap,
                             CapClientSreamCapability config,
                             BapCodecConfiguration* codec,
                             uint32 audioLocation,
                             bool isSink,
                             bool isJointStereoInSink);

CapClientBool capClientManageError(BapInstElement* bap, uint8 bapListCount);

CapClientProfileMsgQueueElem* capClientGetNextMsgElem(CapClientGroupInstance* gInst);

void capClientCapGetCigSpecificParams(CapClientCigElem* cig,
                                     CapClientGroupInstance* cap);

void capClientStopStreamIterateAses(BapInstElement* bap, CapClientContext useCase);

void capClientClearAllStreamVariables(BapInstElement* bap);

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
void capClientMicpServiceReqSend(CapClientGroupInstance* cap,
                               CAP_INST *inst,
                               CapClientOptionalServices serviceId,
                               CapClientMicpReqSender reqSender);
#endif /* EXCLUDE_CSR_BT_MICP_MODULE */

#ifdef CAP_CLIENT_IOP_TEST_MODE
uint16 capClientGetSDUFromCapabilityForIOP(CapClientSreamCapability config, uint16 sdu, CapClientContext useCase);
uint16 capClientGetMTLFromCapabilityForIOP(CapClientSreamCapability config, uint16 mtl, CapClientContext useCase);
#endif /* CAP_CLIENT_IOP_TEST_MODE */

void capClientCopyMetadataToBap(bool isSink,
                               uint8 metadataLength,
                               uint8* metadata,
                               BapInstElement* bap);

#endif /* INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
void capClientSendBcastCommonCfmMsgSend(AppTask appTask,
                                       CAP_INST* inst,
                                       CapClientGroupInstance* cap,
                                       uint32 cid,
                                       CapClientResult result,
                                       CapClientPrim msgType);

void capClientSendBapBcastAsstReq(CapClientGroupInstance* cap,
                                 CAP_INST* inst,
                                 CapClientBcastAsstReqSender reqSender);

CapClientResult capClientBroadcastAssistantValidOperation(ServiceHandle groupId,
                                                         AppTask profileTask,
                                                         CAP_INST* inst,
                                                         CapClientGroupInstance* cap);

void capClientSendSelectBcastAsstCommonCfmMsgSend(AppTask appTask,
                                                  CAP_INST* inst,
                                                  CapClientGroupInstance* cap,
                                                  uint8  infoCount,
                                                  CapClientDelegatorInfo* info,
                                                  CapClientResult result,
                                                  CapClientPrim msgType);
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

#endif
