/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef CAP_CLIENT_BROADCAST_ASSISTANT_PERIODIC_SCAN_H
#define CAP_CLIENT_BROADCAST_ASSISTANT_PERIODIC_SCAN_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
typedef struct
{
    bool                          scanEnable;
    uint8_t                       flags;
    CapClientContext              filterContext;
    uint8_t                       scanFlags;
    uint8_t                       ownAddressType;
    uint8_t                       scanningFilterPolicy;
    CapClientBcastSrcLocation     bcasSrcType;
    CapClientBcastType            bcastType;
    uint16                        scanHandle;
} CapClientPeriodicScanParameters;

typedef struct
{
    bool            allSources;
    bool            notificationEnable;
    uint8           sourceId;
} CapClientRegisterForNoifyParams;


void handleBroadcastAssistantStartSrcScanReq(CAP_INST* inst, const Msg msg);

void capClientBcastAsstStartScanReqSend(BapInstElement* bap, uint32 cid, AppTask appTask);

void handleBroadcastAssistantStopSrcScanReq(CAP_INST* inst, const Msg msg);

void capClientBcastAsstStopScanReqSend(BapInstElement* bap, uint32 cid, AppTask appTask);

void capClientHandleBroadcastAssistantStartScanCfm(CAP_INST *inst,
                                Msg msg,
                                CapClientGroupInstance* cap);

void capClientHandleBroadcastAssistantSrcReportInd(CAP_INST *inst,
                                BapBroadcastAssistantSrcReportInd *ind,
                                CapClientGroupInstance* cap);

void capClientHandleBroadcastAssistantStopScanCfm(CAP_INST* inst,
                                                 Msg msg,
                                                 CapClientGroupInstance* cap);

void capClientSendBapBassRegisterNotificationReq(BapInstElement* bap);

void handleBassRegisterNotificationReq(CAP_INST* inst, const Msg msg);

void capClientBcastAsstNtfReqSend(BapInstElement* bap,
                                    uint32 cid,
                                    AppTask appTask);

void capClientHandleBroadcastAssistantRegisterNotificationCfm(CAP_INST* inst,
                                Msg msg,
                                CapClientGroupInstance* cap);

void handleReadBroadcastReceiveStateReq(CAP_INST* inst, const Msg msg);

void capClientSendBroadcastAssistantReadBrsReq(BapInstElement* bap);

void capClientHandleBroadcastAssistantReadBrsCfm(CAP_INST* inst,
                                                BapBroadcastAssistantReadBrsCfm* cfm,
                                                CapClientGroupInstance* cap);

void capClientSendBroadcastAssistantStartScanSrcCfm(AppTask appTask, 
                                CAP_INST *inst,
                                CapClientGroupInstance *cap,
                                uint32 cid,
                                CapClientResult result,
                                uint16 scanHandle);
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

#endif

