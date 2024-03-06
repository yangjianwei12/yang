/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #
******************************************************************************/

#ifndef CAP_CLIENT_MICP_OPERATION_REQ_H
#define CAP_CLIENT_MICP_OPERATION_REQ_H

#include "cap_client_private.h"
#include "gatt_service_discovery_lib.h"

#ifndef EXCLUDE_CSR_BT_MICP_MODULE

void InitMicpList(CsrCmnListElm_t* elem);
void deInitMicpList(CsrCmnListElm_t* elem);
void capClientInitMicpServiceHandler(CAP_INST *inst);
void capClientMicpHandler(CAP_INST *inst, GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *cfm, uint8 i);
void capClientHandleMicpInit(CAP_INST *inst, CapClientGroupInstance* cap, CapClientInternalInitOptionalServicesReq *req);


void handleCapClientSetMicpProfileAttribHandlesReq(CAP_INST* inst, const Msg msg);

void handleCapClientSetMicStateReq(CAP_INST* inst, const Msg msg);

void capClientMicMuteReqSend(MicpInstElement* micp,
                                    CAP_INST *inst,
                                    uint32 cid);

void capClientSendMicMuteCfm(AppTask appTask,
                   ServiceHandle groupId,
                   CapClientGroupInstance* cap,
                   uint32 cid,
                   CapClientResult result);

void handleCapClientReadMicStateReq(CAP_INST* inst, const Msg msg);

void capCLientSendReadMicStateCfm(AppTask appTask,
                                       ServiceHandle groupId,
                                       CapClientResult result,
                                       MicpInstElement *micp,
                                       uint8 muteValue);


void capClientHandleMicMuteStateInd(MicpMuteValueInd *ind,
    CAP_INST* inst, CapClientGroupInstance* gInst);

#endif
#endif
