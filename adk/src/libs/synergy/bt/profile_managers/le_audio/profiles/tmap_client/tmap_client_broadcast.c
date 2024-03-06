/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #60 $
******************************************************************************/

#include "tmap_client_private.h"
#include "tmap_client_debug.h"

#ifdef INSTALL_LEA_BROADCAST_SOURCE

extern TmapClientMainInst *tmapClientMain;

void TmapClientBroadcastSrcInitReq(AppTask appTask)
{
    TMAP *tmapClientInst = NULL;
    TmapClientProfileHandle profileHndl = 0;
    TmapClientProfileHandleListElm *elem = NULL;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        TMAP_CLIENT_PANIC("Application Task NULL\n");
    }

    elem = ADD_TMAP_CLIENT_SERVICE_HANDLE(tmapClientMain->profileHandleList);
    profileHndl = ADD_TMAP_CLIENT_INST(tmapClientInst);
    elem->profileHandle = profileHndl;

    if (profileHndl)
    {
        /* Reset all the service library memory */
        memset(tmapClientInst, 0, sizeof(TMAP));

        /* Set up library handler for external messages */
        tmapClientInst->libTask = CSR_BT_TMAP_CLIENT_IFACEQUEUE;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        tmapClientInst->appTask = appTask;
        tmapClientInst->bcastSrcHandle = 0xFFFFu;

        tmapClientInst->tmapSrvcHndl = profileHndl;

        tmapClientInst->tmasSrvcHndl = 0;
    }
    else
    {
        TMAP_CLIENT_PANIC("Memory allocation of TMAP Client Profile instance failed!\n");
    }

    CapClientBcastSrcInitReq(tmapClientInst->libTask);
}

void TmapClientBroadcastSrcDeinitReq(TmapClientProfileHandle brcstSrcProfileHandle)
{
    if ((FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(tmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        TMAP_CLIENT_PANIC("TmapClientBroadcastSrcDeinitReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcDeinitReq((uint32)brcstSrcProfileHandle);
}

void TmapClientBroadcastSrcSetBroadcastId(TmapClientProfileHandle brcstSrcProfileHandle, uint32 broadcastId)
{
    if ((FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(tmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        TMAP_CLIENT_PANIC("TmapClientBroadcastSrcSetBroadcastId: Invalid brcstSrcProfileHandle");
    }
    
    CapClientBcastSrcSetBroadcastId((uint32)brcstSrcProfileHandle, broadcastId);
}

void TmapClientBroadcastSrcConfigReq(TmapClientProfileHandle brcstSrcProfileHandle,
                                     uint8 ownAddrType,
                                     uint32 presentationDelay,
                                     uint8 numSubgroup,
                                     const TmapClientBigSubGroup* subgroupInfo,
                                     uint8 brcstSrcNameLen,
                                     const uint8* brcstSrcName,
                                     TmapClientBigConfigMode bigConfigMode,
                                     const TmapClientQhsBigConfig *qhsBigConfig)
{
    uint8 j;
    TmapClientBigMultiSubGroup multiSubGroupInfo;

    multiSubGroupInfo.config = subgroupInfo->config;
    multiSubGroupInfo.targetLatency = subgroupInfo->targetLatency;
    multiSubGroupInfo.lc3BlocksPerSdu = subgroupInfo->lc3BlocksPerSdu;
    multiSubGroupInfo.useCase = subgroupInfo->useCase;
    multiSubGroupInfo.metadataLen = subgroupInfo->metadataLen;
    multiSubGroupInfo.metadata = subgroupInfo->metadata;
    multiSubGroupInfo.numBis = subgroupInfo->numBis;
    multiSubGroupInfo.bisInfo = (TmapClientBisInfo*) CsrPmemZalloc(multiSubGroupInfo.numBis * sizeof(TmapClientBisInfo));

    for (j = 0; j < multiSubGroupInfo.numBis; j++)
    {

        multiSubGroupInfo.bisInfo[j].config = subgroupInfo->bisInfo[j].config;
        multiSubGroupInfo.bisInfo[j].lc3BlocksPerSdu = subgroupInfo->bisInfo[j].lc3BlocksPerSdu;
        multiSubGroupInfo.bisInfo[j].audioLocation = subgroupInfo->bisInfo[j].audioLocation;
        multiSubGroupInfo.bisInfo[j].targetLatency = subgroupInfo->bisInfo[j].targetLatency;
    }

    TmapClientBroadcastSrcConfigSubgroupReq(brcstSrcProfileHandle,
                                            ownAddrType,
                                            presentationDelay,
                                            numSubgroup,
                                            &multiSubGroupInfo,
                                            brcstSrcNameLen,
                                            brcstSrcName,
                                            bigConfigMode,
                                            qhsBigConfig);


    if (multiSubGroupInfo.bisInfo)
        CsrPmemFree(multiSubGroupInfo.bisInfo);
}

void TmapClientBroadcastSrcConfigSubgroupReq(TmapClientProfileHandle brcstSrcProfileHandle,
                                             uint8 ownAddrType,
                                             uint32 presentationDelay,
                                             uint8 numSubgroup,
                                             const TmapClientBigMultiSubGroup* subgroupInfo,
                                             uint8 brcstSrcNameLen,
                                             const uint8* brcstSrcName,
                                             TmapClientBigConfigMode bigConfigMode,
                                             const TmapClientQhsBigConfig *qhsBigConfig)
{
    TmapClientProfileHandleListElm* elem = NULL;
    TMAP *tmapClientInst = NULL;
    bool status = TRUE;
    uint16 context = 0x1234;
    CapClientBigSubGroup *capBigSubGroup = NULL;
    TmapClientBcastInfo broadcastInfo;
    uint8 i, j;

    if ((FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(tmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        TMAP_CLIENT_PANIC("TmapClientBroadcastSrcConfigReq: Invalid brcstSrcProfileHandle");
    }

    if ((numSubgroup < 1) || (subgroupInfo == NULL))
        status = FALSE;

    if ( status != TRUE )
    {
        TmapClientBroadcastSrcInitCfm* cfm = CsrPmemZalloc(sizeof(*cfm));

        cfm->type = TMAP_CLIENT_BROADCAST_SRC_CONFIG_CFM;
        cfm->result = CAP_CLIENT_RESULT_INVALID_PARAMETER;
        cfm->bcastSrcProfileHandle = brcstSrcProfileHandle;

        elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(tmapClientMain->profileHandleList, context);

        if (elem)
            tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

        if (tmapClientInst)
            TmapClientMessageSend(tmapClientInst->appTask, cfm); 
    }
    else
    {
        if (numSubgroup && subgroupInfo)
        {
            capBigSubGroup = (CapClientBigSubGroup*) CsrPmemZalloc(numSubgroup * sizeof(CapClientBigSubGroup));
    
            for (i = 0; i < numSubgroup; i++)
            {
                capBigSubGroup[i].config = subgroupInfo[i].config;
                capBigSubGroup[i].targetLatency = subgroupInfo[i].targetLatency;
                capBigSubGroup[i].lc3BlocksPerSdu = subgroupInfo[i].lc3BlocksPerSdu;
                capBigSubGroup[i].useCase = subgroupInfo[i].useCase;
    
                /*  Copy Metadata */
                capBigSubGroup[i].metadataLen = subgroupInfo[i].metadataLen;
                capBigSubGroup[i].metadata = subgroupInfo[i].metadata;
    
                /*  Copy BIS info*/
                capBigSubGroup[i].numBis = subgroupInfo[i].numBis;
    
                for (j = 0; j < capBigSubGroup[i].numBis; j++)
                {
    
                    capBigSubGroup[i].bisInfo[j].config = subgroupInfo[i].bisInfo[j].config;
                    capBigSubGroup[i].bisInfo[j].lc3BlocksPerSdu = subgroupInfo[i].bisInfo[j].lc3BlocksPerSdu;
                    capBigSubGroup[i].bisInfo[j].audioLocation = subgroupInfo[i].bisInfo[j].audioLocation;
                    capBigSubGroup[i].bisInfo[j].targetLatency = subgroupInfo[i].bisInfo[j].targetLatency;
                }
            }
        }

        broadcastInfo.appearanceValue = CSR_BT_APPEARANCE_GENERIC_PHONE;
        broadcastInfo.broadcast = TMAP_BROADCAST;
        broadcastInfo.flags = 0;
        broadcastInfo.bigNameLen = brcstSrcNameLen;

        if (brcstSrcNameLen)
        {
            broadcastInfo.bigName =  CsrPmemZalloc(brcstSrcNameLen);
            for (i = 0; i < brcstSrcNameLen; i++)
            {
                broadcastInfo.bigName[i] = brcstSrcName[i];
            }
        }

        CapClientBcastSrcConfigReq((uint32)brcstSrcProfileHandle,
                                   ownAddrType,
                                   presentationDelay,
                                   numSubgroup,
                                   capBigSubGroup,
                                   &broadcastInfo,
                                   bigConfigMode,
                                   qhsBigConfig);

        if (brcstSrcNameLen)
            CsrPmemFree(broadcastInfo.bigName);

        if (capBigSubGroup != NULL)
            CsrPmemFree(capBigSubGroup);
    }
}


void TmapClientBroadcastSrcStartStreamReq(TmapClientProfileHandle brcstSrcProfileHandle,
                                          bool  encryption,
                                          const uint8* broadcastCode)
{
    if ((FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(tmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        TMAP_CLIENT_PANIC("TmapClientBroadcastSrcStartStreamReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcStartStreamReq((uint32)brcstSrcProfileHandle,
                                    encryption, 
                                    (uint8*)broadcastCode);

}

void TmapClientBroadcastSrcUpdateStreamReq(TmapClientProfileHandle brcstSrcProfileHandle,
                                           TmapClientContext useCase,
                                           uint8_t numSubgroup,
                                           uint8 metadataLen,
                                           const uint8* metadata)
{
    if ((FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(tmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        TMAP_CLIENT_PANIC("TmapClientBroadcastSrcUpdateStreamReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcUpdateStreamReq((uint32)brcstSrcProfileHandle,
                                     (CapClientContext)useCase,
                                     numSubgroup,
                                     metadataLen,
                                     (uint8*)metadata);
}

void TmapClientBroadcastSrcStopStreamReq(TmapClientProfileHandle brcstSrcProfileHandle)
{
    if ((FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(tmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        TMAP_CLIENT_PANIC("TmapClientBroadcastSrcStopStreamReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcStopStreamReq((uint32)brcstSrcProfileHandle);
}

void TmapClientBroadcastSrcRemoveStreamReq(TmapClientProfileHandle brcstSrcProfileHandle)
{
    if ((FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(tmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        TMAP_CLIENT_PANIC("TmapClientBroadcastSrcRemoveStreamReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcRemoveStreamReq((uint32)brcstSrcProfileHandle);
}

#if 0
void TmapClientBroadcastSrcBigTestConfigReq(TmapClientProfileHandle brcstSrcProfileHandle,
                                            TmapClientContext useCase,
                                            TmapClientBigTestConfigParam *bigTestConfigParam,
                                            uint8 numBis,
                                            TmapClientBigConfigMode bigConfigMode,
                                            const TmapClientQhsBigConfig *qhsBigConfig)
{
    if ((FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(tmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        TMAP_CLIENT_PANIC("TmapClientBroadcastSrcBigTestConfigReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcBigTestConfigReq((uint32)brcstSrcProfileHandle,
                                      useCase,
                                      bigTestConfigParam,
                                      numBis,
                                      bigConfigMode,
                                      qhsBigConfig);
}
#endif

void TmapClientBroadcastSrcSetAdvParamsReq(TmapClientProfileHandle brcstSrcProfileHandle,
                                           const TmapClientBcastSrcAdvParams *srcAdvPaParams)
{
    if ((FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(tmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        TMAP_CLIENT_PANIC("TmapClientBroadcastSrcSetAdvParamsReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcSetAdvParamsReq((uint32)brcstSrcProfileHandle,
                                     srcAdvPaParams);
}

#endif /* INSTALL_LEA_BROADCAST_SOURCE */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
void TmapClientBroadcastAsstStartSrcScanReq(TmapClientProfileHandle profileHandle,
                                            ServiceHandle groupId,
                                            uint32 cid,
                                            TmapClientBroadcastSrcType bcastSrcType,
                                            TmapClientContext audioFilterContext,
                                            uint8 scanFlags,
                                            uint8 ownAddressType,
                                            uint8 scanningFilterPolicy)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstStartSrcScanReq(tmapClientInst->libTask,
                                      groupId,
                                      cid,
                                      bcastSrcType,
                                      NON_PUBLIC_BROADCAST,
                                      audioFilterContext,
                                      scanFlags,
                                      ownAddressType,
                                      scanningFilterPolicy);
}

void TmapClientBroadcastAsstStopSrcScanReq(TmapClientProfileHandle profileHandle,
                                           ServiceHandle groupId,
                                           uint32 cid,
                                           uint16 scanHandle)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstStopSrcScanReq(tmapClientInst->libTask,
                                      groupId,
                                      cid,
                                      scanHandle);
}

void TmapClientBroadcastAsstRegisterNotificationReq(TmapClientProfileHandle profileHandle,
                                                    ServiceHandle groupId,
                                                    uint32 cid,
                                                    uint8 sourceId,
                                                    bool noficationEnable)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstRegisterNotificationReq(tmapClientInst->libTask,
                                              groupId,
                                              cid,
                                              sourceId,
                                              TRUE,
                                              noficationEnable);
}

void TmapClientBroadcastAsstReadReceiverSinkStateReq(TmapClientProfileHandle profileHandle,
                                                     ServiceHandle groupId,
                                                     uint32  cid)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstReadReceiveStateReq(tmapClientInst->libTask,
                                          groupId,
                                          cid);
}

void TmapClientBroadcastAsstStartSyncToSrcReq(TmapClientProfileHandle profileHandle,
                                              ServiceHandle groupId,
                                              TYPED_BD_ADDR_T *addrt,
                                              uint8 advSid)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstSyncToSrcStartReq(tmapClientInst->libTask,
                                        groupId,
                                        addrt,
                                        advSid);
}

void TmapClientBroadcastAsstTerminateSyncToSrcReq(TmapClientProfileHandle profileHandle,
                                                  ServiceHandle groupId,
												  uint16 syncHandle)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstTerminateSyncToSrcReq(tmapClientInst->libTask,
                                            groupId,
                                            syncHandle);
}

void TmapClientBroadcastAsstCancelSyncToSrcReq(TmapClientProfileHandle profileHandle,
                                               ServiceHandle groupId)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstSyncToSrcCancelReq(tmapClientInst->libTask,
                                         groupId);
}

void TmapClientBroadcastAsstAddSrcReq(TmapClientProfileHandle profileHandle,
                                      ServiceHandle groupId,
                                      uint32 cid,
                                      TYPED_BD_ADDR_T *sourceAddrt,
                                      TmapClientBroadcastSrcType bcastSrcType,
                                      uint16 syncHandle,
                                      uint8 sourceAdvSid,
                                      uint8 paSyncState,
                                      uint16 paInterval,
                                      uint32 broadcastId,
                                      uint8 numbSubGroups,
                                      TmapClientSubgroupInfo *subgroupInfo)
{
    bool srcColocated = (bcastSrcType == TMAP_CLIENT_BROADCAST_SRC_COLLOCATED) ? TRUE : FALSE;
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

	CapClientBcastAsstAddSrcReq(tmapClientInst->libTask,
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

void TmapClientBroadcastAsstModifySrcReq(TmapClientProfileHandle profileHandle,
                                         ServiceHandle groupId,
                                         TmapClientBroadcastSrcType bcastSrcType,
                                         uint16 syncHandle,
                                         uint8 sourceAdvSid,
                                         uint8 paSyncState,
                                         uint16 paInterval,
                                         uint8 numBrcastSinkInfo,
                                         TmapClientBroadcastSinkInfo* brcastSinkInfo,
                                         uint8 numbSubGroups,
                                         TmapClientSubgroupInfo *subgroupInfo)
{
    bool srcColocated = (bcastSrcType == TMAP_CLIENT_BROADCAST_SRC_COLLOCATED) ? TRUE : FALSE;
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);
    CapClientDelegatorInfo* delegatorInfo = NULL;
    int i = 0;

    if (numBrcastSinkInfo != 0)
    {
        delegatorInfo = (CapClientDelegatorInfo*)CsrPmemZalloc(numBrcastSinkInfo * sizeof(CapClientDelegatorInfo));
        for (i = 0; i < numBrcastSinkInfo; i++)
        {
            delegatorInfo[i].cid = brcastSinkInfo[i].cid;
            delegatorInfo[i].sourceId = brcastSinkInfo[i].sourceId;
        }
    }

    CapClientBcastAsstModifySrcReq(tmapClientInst->libTask,
                                   groupId,
                                   srcColocated,
                                   syncHandle,
                                   sourceAdvSid,
                                   paSyncState,
                                   paInterval,
                                   numBrcastSinkInfo,
                                   delegatorInfo,
                                   numbSubGroups,
                                   (CapClientSubgroupInfo*)subgroupInfo);

    if (delegatorInfo != NULL)
        CsrPmemFree(delegatorInfo);
}

void TmapClientBroadcastAsstRemoveSrcReq(TmapClientProfileHandle profileHandle,
                                         ServiceHandle groupId,
                                         uint8 numBrcastSinkInfo,
                                         TmapClientBroadcastSinkInfo* brcastSinkInfo)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);
    CapClientDelegatorInfo* delegatorInfo = NULL;
    int i = 0;

    if (numBrcastSinkInfo != 0)
    {
        delegatorInfo = (CapClientDelegatorInfo*)CsrPmemZalloc(numBrcastSinkInfo * sizeof(CapClientDelegatorInfo));
        for (i = 0; i < numBrcastSinkInfo; i++)
        {
            delegatorInfo[i].cid = brcastSinkInfo[i].cid;
            delegatorInfo[i].sourceId = brcastSinkInfo[i].sourceId;
        }
    }

    CapClientBcastAsstRemoveSrcReq(tmapClientInst->libTask,
                                   groupId,
                                   numBrcastSinkInfo,
                                   delegatorInfo);

    if (delegatorInfo != NULL)
        CsrPmemFree(delegatorInfo);
}

void TmapClientBroadcastAsstSetCodeRsp(ServiceHandle groupId,
                                       uint32  cid,
                                       uint8* broadcastcode,
                                       uint8 numBrcastSinkInfo,
                                       TmapClientBroadcastSinkInfo* brcastSinkInfo)
{
    CapClientBcastAsstSetCodeRsp(groupId,
                                 cid,
                                 brcastSinkInfo->sourceId,
                                 broadcastcode);

   CSR_UNUSED(numBrcastSinkInfo);
}

#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */
