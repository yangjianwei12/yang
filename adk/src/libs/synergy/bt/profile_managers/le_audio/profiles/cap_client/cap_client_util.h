/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef CAP_CLIENT_UTIL_H
#define CAP_CLIENT_UTIL_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_BROADCAST_SOURCE
typedef struct
{
    BD_ADDR_T                    sourceAddrt;
    uint8_t                      advertiserAddressType;
    bool                         srcCollocated;
    uint16_t                     syncHandle;    /*!< SyncHandle of PA or advHandle of
                                                      collocated Broadcast src */
    uint8_t                      sourceAdvSid;  /*! Advertising SID */
    CapClientPaSyncState         paSyncState;   /*! PA Synchronization state */
    uint16_t                     paInterval;
    uint32_t                     broadcastId;   /*!< Broadcast ID */
    uint8_t                      infoCount;
    CapClientDelegatorInfo* info;
    uint8_t                      numbSubGroups; /*! Number of subgroups */
    BapSubgroupInfo              subgroupInfo[BAP_MAX_SUPPORTED_NUM_SUBGROUPS];
}CapClientBroadcastSrcParams;

CapClientBool capClientGetBcastSrcFromBigId(CsrCmnListElm_t* elem, void* value);

CapClientBool capClientGetBcastSrcFromPhandle(CsrCmnListElm_t* elem, void* value);

CapClientBool capClientGetBcastSrcFromAppTask(CsrCmnListElm_t* elem, void* value);

void capClientDecrementOpCounter(CapClientGenericCounter* counter);

#endif /* INSTALL_LEA_BROADCAST_SOURCE */

#define CAP_CLIENT_SET_OP_COUNTER_VALUE(_OP_COUNTER, _VAL)  (_OP_COUNTER.opReqCount = _VAL)

#if defined(INSTALL_LEA_BROADCAST_SOURCE) || defined(INSTALL_LEA_BROADCAST_ASSISTANT)
void capClientIncrementOpCounter(CapClientGenericCounter* counter);
#endif

#ifdef INSTALL_LEA_UNICAST_CLIENT
CapClientBool capClientGetCapGroupFromCid(CsrCmnListElm_t* elem, void *value);

BapInstElement *capClientGetBapInstanceFromCid(CsrCmnList_t list, uint32 cid);

CsipInstElement *capClientGetCsipInstanceFromCid(CsrCmnList_t list, uint32 cid);

VcpInstElement *capClientGetVcpInstanceFromCid(CsrCmnList_t list, uint32 cid);

BapInstElement *capClientGetBapInstanceFromPhandle(CsrCmnList_t list, ServiceHandle srvcHndl);

CsipInstElement *capClientGetCsipInstanceFromPhandle(CsrCmnList_t list, ServiceHandle srvcHndl);

VcpInstElement *capClientGetVcpInstanceFromPhandle(CsrCmnList_t list, ServiceHandle srvcHndl);

CapClientBool capClientGetCsipFromPhandle(CsrCmnListElm_t* elem, void *value);

CapClientBool capClientGetVcpFromPhandle(CsrCmnListElm_t* elem, void *value);

CapClientBool capClientGetBapFromPhandle(CsrCmnListElm_t* elem, void *value);
CapClientBool capClientGetBapFromGroupId(CsrCmnListElm_t* elem, void *value);

CapClientBool capClientGetBapFromCid(CsrCmnListElm_t* elem, void* value);

CapClientBool capClientGetVcpFromCid(CsrCmnListElm_t* elem, void *value);

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
CapClientBool capClientGetMicpFromPhandle(CsrCmnListElm_t* elem, void* value);

CapClientBool capClientGetMicpFromCid(CsrCmnListElm_t* elem, void* value);
#endif

CapClientBool capClientGetCsipFromCid(CsrCmnListElm_t* elem, void *value);
CsrInt32 capClientSortCsipProfileList(CsrCmnListElm_t *elem1, CsrCmnListElm_t *elem2);
CsrInt32 capClientSortBapProfileList(CsrCmnListElm_t *elem1, CsrCmnListElm_t *elem2);
CsrInt32 capClientSortVcpProfileList(CsrCmnListElm_t *elem1, CsrCmnListElm_t *elem2);


CapClientResult capClientGetCapClientResult(uint16 result, CapClientProfile profile);

CapClientBool capClientSearchGroupId(CsrCmnListElm_t* elem, void *value);

void capClientGetAllProfileInstanceStatus(uint8 deviceCount,
                               CsrCmnListElm_t *elem,
                               CapClientDeviceStatus **status,
                               CapClientProfile profile);

CapClientBool capClientIsGroupCoordinatedSet(CapClientGroupInstance* cap);

uint32 capClientGetAudioLocationFromBitMask(uint32* configuredLocation,
                                           CapClientCigConfigMode mode,
                                           uint8 channelCount);

CapClientBool capClientGetHandleElmFromGroupId(CsrCmnListElm_t* elem, void* value);

CapClientBool capClientIsConfigSupportedByServer(CapClientGroupInstance* cap,
		                             CapClientSreamCapability config,
                                     CapClientContext useCase,
                                     uint8 direction, 
                                     uint8 *versionNum);

CapClientBool capClientIsContextAvailable(CapClientContext useCase,
                              CapClientGroupInstance* cap, CapClientBool isSink);

CapClientResult capClientValidateCapState(CapClientState state, CapClientPrim operation);

uint8 capClientGetAseCountForUseCase(BapInstElement* bap, CapClientContext useCase);
uint8 capClientGetSrcAseCountForUseCase(BapInstElement* bap, CapClientContext useCase);
uint8 capClientGetSinkAseCountForUseCase(BapInstElement* bap, CapClientContext useCase);

void* capClientGetHandlesInfo(uint32 cid, CapClientGroupInstance *cap, uint8 clntProfile);

void capClientMessageQueueRemove(CsrCmnList_t* list);

CapClientBool capClientGetTaskElemFromAppHandle(CsrCmnListElm_t* elem, void* value);

void capClientRemoveTaskFromList(CsrCmnList_t *list, AppTask appTask);

CapClientProfileMsgQueueElem* CapClientMsgQueueAdd(CsrCmnList_t* list,
                                                   void* req,
                                                   int indCount,
                                                   CapClientPrim type,
                                                   CapClientMessageHandler handlerFunc,
                                                   CapClientProfileTaskListElem* task);

CapClientBool capClientAsesReleaseComplete(CapClientGroupInstance* cap);

uint8 capClientGetSetBitCount(uint32 num);

CapClientCisDirection capClientGetCisDirectionFromConfig(CapClientSreamCapability sinkConfig,
                                                         CapClientSreamCapability srcConfig);

void capClientClearCounters(CAP_INST* inst);

void capClientFreeCapInternalMsg(uint16 type, Msg msg);

uint8 capClientGetTotalCisCount(BapInstElement* bap,
                               CapClientContext useCase,
                               CapClientCisDirection cisDir);

CapClientBool capClientGetCigFromContext(CsrCmnListElm_t* elem, void* value);

CapClientBool capClientGetCigFromCigId(CsrCmnListElm_t* elem, void* value);

CapClientContext capClientMapCapContextWithCap(CapClientContext useCase);

uint8 capClientGetCisCountPerBap(BapInstElement* bap, CapClientContext useCase);

void capClientSendInternalPendingOpReq(CapClientPendingOp op);

void capClientNtfTimerSet(CAP_INST *inst, CapClientGroupInstance* cap,
                                  BapInstElement *bap,
                                  uint8 capAseState);

void capClientNtfTimerReset(BapInstElement *bap);


void capclientTimerExpiryHandler(CsrUint16 start, void* data);

void setBapStatePerCigId(BapInstElement * bap, uint32 state, uint8 cigId);
#endif /* INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
CapClientBcastAssistState capClientBcastAsistantGetState(CapClientGroupInstance *cap);

void capClientBcastAsistantSetState(CapClientGroupInstance *cap,
                                              CapClientBcastAssistState state);

void capClientResetOpCounter(CapClientGroupInstance* cap);

void capClientIncrementErrorCounter(CapClientGenericCounter* counter);

void capClientIncrementSuccessCounter(CapClientGenericCounter* counter);

CapClientBool capClientIsGroupIntsanceLocked(CapClientGroupInstance* cap);

CapClientBool capClientBcastAsstOpComplete(CapClientGroupInstance* cap);

void capClientBcastAsstResetState(CapClientGroupInstance* cap, CapClientBcastAssistState state);

void capClientFreeSourceParamsContent(void);
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

CapClientResult capClientBroadcastSourceGetResultCode(BapResult result);

uint8 capClientNumOfBitSet(uint32 num);

bool capClientUtilsFindLtvValue(uint8 * ltvData,
                          uint8 ltvDataLength,
                          uint8 type,
                          uint8 * value,
                          uint8 valueLength);
#endif




