/******************************************************************************
 Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #5 $
******************************************************************************/

#include "pbp_private.h"
#include "pbp_debug.h"

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT

/******************************************************************************/
void PbpBroadcastAssistantScansSrcStartSrcReq(PbpProfileHandle profileHandle,
                                              ServiceHandle groupId,
                                              uint32 cid,
                                              PbpBcastSrcLocation bcastSrcType,
                                              PbpBcastType bcastType,
                                              PbpContext filterContext,
                                              uint8 scanFlags,
                                              uint8 ownAddressType,
                                              uint8 scanningFilterPolicy)
{
    PBP *pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);

    if (pbpInst)
    {
        if (CHECK_STANDARD_BROADCAST_TYPE(bcastType) ||
            CHECK_HQ_BROADCAST_TYPE(bcastType) ||
            CHECK_ALL_PBP_BROADCAST_TYPE_SUPPORTED(bcastType))
        {
            CapClientBcastAsstStartSrcScanReq(pbpInst->libTask,
                groupId,
                cid,
                bcastSrcType,
                bcastType,
                filterContext,
                scanFlags,
                ownAddressType,
                scanningFilterPolicy);
        }
        else
        {
            MAKE_PBP_MESSAGE(PbpBroadcastAssistantScanSrcStartCfm);

            message->result = CAP_CLIENT_RESULT_INVALID_PARAMETER;

            PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_SCAN_SRC_START_CFM, message);
        }
    }
    else
    {
        PBP_PANIC("PbpBroadcastAssistantScansSrcStartSrcReq: PBP instance is NULL!\n");
    }
} 

/******************************************************************************/
void PbpBroadcastAssistantStopSrcScanReq(PbpProfileHandle profileHandle,
                                         ServiceHandle groupId,
                                         uint32 cid,
                                         uint16 scanHandle)
{
    PBP* pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);

    if (pbpInst)
    {
        CapClientBcastAsstStopSrcScanReq(pbpInst->libTask,
            groupId,
            cid,
            scanHandle);
    }
    else
    {
        PBP_PANIC("PbpBroadcastAssistantStopSrcScanReq: PBP instance is NULL!\n");
    }
}

/******************************************************************************/
void PbpBroadcastAssistantStartSyncToSrcReq(PbpProfileHandle profileHandle,
                                              ServiceHandle groupId,
                                              TYPED_BD_ADDR_T * addrt,
                                              uint8 advSid)
{

    PBP* pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);

    if (pbpInst)
    {
        CapClientBcastAsstSyncToSrcStartReq(pbpInst->libTask,
            groupId,
            addrt,
            advSid);
    }
    else
    {
        PBP_PANIC("PbpBroadcastAssistantStartSyncToSrcReq: PBP instance is NULL!\n");
    }
}

/******************************************************************************/
void PbpBroadcastAssistantTerminateSyncToSrcReq(PbpProfileHandle profileHandle,
                                                ServiceHandle groupId,
                                                uint16 syncHandle)
{
    PBP* pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);

    if (pbpInst)
    {
        CapClientBcastAsstTerminateSyncToSrcReq(pbpInst->libTask,
            groupId,
            syncHandle);
    }
    else
    {
        PBP_PANIC("PbpBroadcastAssistantTerminateSyncToSrcReq: PBP instance is NULL!\n");
    }
}

/******************************************************************************/
void PbpBroadcastAssistantSyncToSrcCancelReq(PbpProfileHandle profileHandle, ServiceHandle groupId)
{
    PBP* pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);

    if (pbpInst)
    {
        CapClientBcastAsstSyncToSrcCancelReq(pbpInst->libTask, groupId);
    }
    else
    {
        PBP_PANIC("PbpBroadcastAssistantSyncToSrcCancelReq: PBP instance is NULL!\n");
    }
} 

/******************************************************************************/
void PbpBroadcastAssistantAddSrcReq(PbpProfileHandle profileHandle,
                                    ServiceHandle groupId,
                                    uint32 cid,
                                    TYPED_BD_ADDR_T* sourceAddrt,
                                    PbpBcastSrcLocation srcLocation,
                                    uint16 syncHandle,
                                    uint8 sourceAdvSid,
                                    PbpPaSyncState paSyncState,
                                    uint16 paInterval,
                                    uint32 broadcastId,
                                    uint8 numbSubGroups,
                                    const PbpSubgroupInfo *subgroupInfo)
{
    bool srcColocated = (srcLocation == PBP_BCAST_SRC_COLLOCATED) ? TRUE : FALSE;
    PBP *pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);

    if (pbpInst)
    {
        CapClientBcastAsstAddSrcReq(pbpInst->libTask,
            groupId,
            cid,
            &(sourceAddrt->addr),
            sourceAddrt->type,
            srcColocated,
            syncHandle,
            sourceAdvSid,
            paSyncState,
            paInterval,
            broadcastId,
            numbSubGroups,
            (CapClientSubgroupInfo*)subgroupInfo);
    }
    else
    {
        PBP_PANIC("PbpBroadcastAssistantAddSrcReq: PBP instance is NULL!\n");
    }
}

/******************************************************************************/
static CapClientDelegatorInfo* pbpBroadcastAssistantSetDelegatorInfo(uint8 numBroadcastDelegator,
                                                                     const PbpBroadcastDelegatorInfo* broadcastDelegatorInfo)
{
    CapClientDelegatorInfo* delegatorInfo = NULL;

    if (numBroadcastDelegator)
    {
        delegatorInfo = CsrPmemZalloc(numBroadcastDelegator * sizeof(CapClientDelegatorInfo));

        SynMemCpyS(delegatorInfo, numBroadcastDelegator * sizeof(CapClientDelegatorInfo),
            broadcastDelegatorInfo, numBroadcastDelegator * sizeof(CapClientDelegatorInfo));
    }

    return delegatorInfo;
}

/******************************************************************************/
void PbpBroadcastAssistantModifySrcReq(PbpProfileHandle profileHandle,
                                       ServiceHandle groupId,
                                       PbpBcastSrcLocation srcLocation,
                                       uint16 syncHandle,
                                       uint8 sourceAdvSid,
                                       PbpPaSyncState paSyncState,
                                       uint16 paInterval,
                                       uint8 numBroadcastDelegator,
                                       const PbpBroadcastDelegatorInfo* broadcastDelegatorInfo,
                                       uint8 numbSubGroups,
                                       const PbpSubgroupInfo* subgroupInfo)
{
    bool srcColocated = (srcLocation == PBP_BCAST_SRC_COLLOCATED) ? TRUE : FALSE;
    PBP *pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);
    CapClientDelegatorInfo* delegatorInfo = NULL;

    if (pbpInst)
    {
        delegatorInfo = pbpBroadcastAssistantSetDelegatorInfo(numBroadcastDelegator, broadcastDelegatorInfo);

        CapClientBcastAsstModifySrcReq(pbpInst->libTask,
            groupId,
            srcColocated,
            syncHandle,
            sourceAdvSid,
            paSyncState,
            paInterval,
            numBroadcastDelegator,
            delegatorInfo,
            numbSubGroups,
            (CapClientSubgroupInfo*)subgroupInfo);

        if (delegatorInfo)
            CsrPmemFree(delegatorInfo);
    }
    else
    {
        PBP_PANIC("PbpBroadcastAssistantModifySrcReq: PBP instance is NULL!\n");
    }
}

/******************************************************************************/
void PbpBroadcastAssistantRemoveSrcReq(PbpProfileHandle profileHandle,
                                       ServiceHandle groupId,
                                       uint8 numBroadcastDelegator,
                                       const PbpBroadcastDelegatorInfo* broadcastDelegatorInfo)
{
    PBP* pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);
    CapClientDelegatorInfo* delegatorInfo = NULL;

    if (pbpInst)
    {
        delegatorInfo = pbpBroadcastAssistantSetDelegatorInfo(numBroadcastDelegator, broadcastDelegatorInfo);

        CapClientBcastAsstRemoveSrcReq(pbpInst->libTask,
            groupId,
            numBroadcastDelegator,
            delegatorInfo);

        if (delegatorInfo)
            CsrPmemFree(delegatorInfo);
    }
    else
    {
        PBP_PANIC("PbpBroadcastAssistantRemoveSrcReq: PBP instance is NULL!\n");
    }
}

/******************************************************************************/
void PbpBroadcastAssistantRegisterForNotificationReq(PbpProfileHandle profileHandle,
                                                     ServiceHandle groupId,
                                                     uint32 cid,
                                                     uint8 sourceId,
                                                     bool allSources,
                                                     bool noficationEnable)
{
    PBP* pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);

    if (pbpInst)
    {
        CapClientBcastAsstRegisterNotificationReq(pbpInst->libTask,
            groupId,
            cid,
            sourceId,
            allSources,
            noficationEnable);
    }
    else
    {
        PBP_PANIC("PbpBroadcastAssistantRegisterForNotificationReq: PBP instance is NULL!\n");
    }
}

/******************************************************************************/
void PbpBroadcastAssistantReadReceiveStateReq(PbpProfileHandle profileHandle,
                                              ServiceHandle groupId,
                                              uint32  cid)
{
    PBP* pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);

    if (pbpInst)
    {
        CapClientBcastAsstReadReceiveStateReq(pbpInst->libTask,
            groupId,
            cid);
    }
    else
    {
        PBP_PANIC("PbpBroadcastAssistantReadReceiveStateReq: PBP instance is NULL!\n");
    }
}

/******************************************************************************/
void PbpBroadcastAssistantSetCodeRsp(ServiceHandle groupId,
                                     uint32  cid,
                                     uint8 sourceId,
                                     uint8* broadcastcode)
{
    if (broadcastcode)
    {
        CapClientBcastAsstSetCodeRsp(groupId,
                                     cid,
                                     sourceId,
                                     broadcastcode);
    }
    else
    {
        PBP_PANIC("PbpBroadcastAssistantSetCodeRsp: broadcastcode is NULL!\n");
    }
}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

#ifdef INSTALL_LEA_UNICAST_CLIENT
/******************************************************************************/
void PbpRegisterTaskReq(ServiceHandle groupId, PbpProfileHandle profileHandle)
{
    PBP* pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);

    if (pbpInst)
    {
        CapClientRegisterTaskReq(pbpInst->libTask, groupId);
    }
    else
    {
        PBP_PANIC("PbpRegisterTaskReq: PBP instance is NULL!\n");
    }
}

/******************************************************************************/
void PbpDeRegisterTaskReq(PbpProfileHandle profileHandle, ServiceHandle groupId)
{
    PBP* pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);

    if (pbpInst)
    {
        CapClientDeRegisterTaskReq(pbpInst->libTask, groupId);
    }
    else
    {
        PBP_PANIC("PbpDeRegisterTaskReq: PBP instance is NULL!\n");
    }
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */
