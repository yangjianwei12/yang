/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/

#include "gmap_client_private.h"
#include "gmap_client_debug.h"

#ifdef INSTALL_LEA_BROADCAST_SOURCE

extern GmapClientMainInst *gmapClientMain;

void GmapClientBroadcastSrcInitReq(AppTask appTask)
{
    GMAP *gmapClientInst = NULL;
    GmapClientProfileHandle profileHndl = 0;
    GmapClientProfileHandleListElm *elem = NULL;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        GMAP_CLIENT_PANIC("Application Task NULL\n");
    }

    elem = ADD_GMAP_CLIENT_SERVICE_HANDLE(gmapClientMain->profileHandleList);
    profileHndl = ADD_GMAP_CLIENT_INST(gmapClientInst);
    elem->profileHandle = profileHndl;

    if (profileHndl)
    {
        /* Reset all the service library memory */
        memset(gmapClientInst, 0, sizeof(GMAP));

        /* Set up library handler for external messages */
        gmapClientInst->libTask = CSR_BT_GMAP_CLIENT_IFACEQUEUE;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        gmapClientInst->appTask = appTask;
        gmapClientInst->bcastSrcHandle = 0xFFFFu;

        gmapClientInst->gmapSrvcHndl = profileHndl;

        gmapClientInst->gmasSrvcHndl = 0;
    }
    else
    {
        GMAP_CLIENT_PANIC("Memory allocation of GMAP Client Profile instance failed!\n");
    }

    CapClientBcastSrcInitReq(gmapClientInst->libTask);
}

void GmapClientBroadcastSrcDeinitReq(GmapClientProfileHandle brcstSrcProfileHandle)
{
    if ((FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(gmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        GMAP_CLIENT_PANIC("GmapClientBroadcastSrcDeinitReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcDeinitReq((uint32)brcstSrcProfileHandle);
}

void GmapClientBroadcastSrcConfigReq(GmapClientProfileHandle brcstSrcProfileHandle,
                                     uint8 ownAddrType,
                                     uint32 presentationDelay,
                                     uint8 numSubgroup,
                                     const GmapClientBigSubGroup* subgroupInfo,
                                     uint8 brcstSrcNameLen,
                                     const uint8* brcstSrcName,
                                     GmapClientBigConfigMode bigConfigMode,
                                     const GmapClientQhsBigConfig *qhsBigConfig)
{
    GmapClientProfileHandleListElm* elem = NULL;
    GMAP *gmapClientInst = NULL;
    bool status = TRUE;
    uint16 context = 0x1234;
    CapClientBigSubGroup *capBigSubGroup = NULL;
    GmapClientBcastInfo broadcastInfo;
    uint8 i, j;

    if ((FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(gmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        GMAP_CLIENT_PANIC("GmapClientBroadcastSrcConfigReq: Invalid brcstSrcProfileHandle");
    }

    if ((numSubgroup < 1) || (subgroupInfo == NULL))
        status = FALSE;

    if ( status != TRUE )
    {
        GmapClientBroadcastSrcInitCfm* cfm = CsrPmemZalloc(sizeof(*cfm));

        cfm->type = GMAP_CLIENT_BROADCAST_SRC_CONFIG_CFM;
        cfm->result = CAP_CLIENT_RESULT_INVALID_PARAMETER;
        cfm->bcastSrcProfileHandle = brcstSrcProfileHandle;

        elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(gmapClientMain->profileHandleList, context);

        if (elem)
            gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

        if (gmapClientInst)
            GmapClientMessageSend(gmapClientInst->appTask, cfm); 
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
        broadcastInfo.broadcast = GMAP_BROADCAST;
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

void GmapClientBroadcastSrcStartStreamReq(GmapClientProfileHandle brcstSrcProfileHandle,
                                          bool  encryption,
                                          const uint8* broadcastCode)
{
    if ((FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(gmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        GMAP_CLIENT_PANIC("GmapClientBroadcastSrcStartStreamReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcStartStreamReq((uint32)brcstSrcProfileHandle,
                                    encryption, 
                                    (uint8*)broadcastCode);

}

void GmapClientBroadcastSrcUpdateStreamReq(GmapClientProfileHandle brcstSrcProfileHandle,
                                           GmapClientContext useCase,
                                           uint8_t numSubgroup,
                                           uint8 metadataLen,
                                           const uint8* metadata)
{
    if ((FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(gmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        GMAP_CLIENT_PANIC("GmapClientBroadcastSrcStartStreamReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcUpdateStreamReq((uint32)brcstSrcProfileHandle,
                                     (CapClientContext)useCase,
                                     numSubgroup,
                                     metadataLen,
                                     (uint8*)metadata);
}

void GmapClientBroadcastSrcStopStreamReq(GmapClientProfileHandle brcstSrcProfileHandle)
{
    if ((FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(gmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        GMAP_CLIENT_PANIC("GmapClientBroadcastSrcStartStreamReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcStopStreamReq((uint32)brcstSrcProfileHandle);
}

void GmapClientBroadcastSrcRemoveStreamReq(GmapClientProfileHandle brcstSrcProfileHandle)
{
    if ((FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(gmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        GMAP_CLIENT_PANIC("GmapClientBroadcastSrcStartStreamReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcRemoveStreamReq((uint32)brcstSrcProfileHandle);
}

#if 0
void GmapClientBroadcastSrcBigTestConfigReq(GmapClientProfileHandle brcstSrcProfileHandle,
                                            GmapClientContext useCase,
                                            GmapClientBigTestConfigParam *bigTestConfigParam,
                                            uint8 numBis,
                                            GmapClientBigConfigMode bigConfigMode,
                                            const GmapClientQhsBigConfig *qhsBigConfig)
{
    if ((FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(gmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        GMAP_CLIENT_PANIC("GmapClientBroadcastSrcStartStreamReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcBigTestConfigReq((uint32)brcstSrcProfileHandle,
                                      useCase,
                                      bigTestConfigParam,
                                      numBis,
                                      bigConfigMode,
                                      qhsBigConfig);
}
#endif

void GmapClientBroadcastSrcSetAdvParamsReq(GmapClientProfileHandle brcstSrcProfileHandle,
                                           const GmapClientBcastSrcAdvParams *srcAdvPaParams)
{
    if ((FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(gmapClientMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        GMAP_CLIENT_PANIC("GmapClientBroadcastSrcStartStreamReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcSetAdvParamsReq((uint32)brcstSrcProfileHandle,
                                     srcAdvPaParams);
}

#endif /* INSTALL_LEA_BROADCAST_SOURCE */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
void GmapClientBroadcastAsstStartSrcScanReq(GmapClientProfileHandle profileHandle,
                                            ServiceHandle groupId,
                                            uint32 cid,
                                            GmapClientBroadcastSrcType bcastSrcType,
                                            GmapClientContext audioFilterContext,
                                            uint8 scanFlags,
                                            uint8 ownAddressType,
                                            uint8 scanningFilterPolicy)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstStartSrcScanReq(gmapClientInst->libTask,
                                      groupId,
                                      cid,
                                      bcastSrcType,
                                      NON_PUBLIC_BROADCAST,
                                      audioFilterContext,
                                      scanFlags,
                                      ownAddressType,
                                      scanningFilterPolicy);
}

void GmapClientBroadcastAsstStopSrcScanReq(GmapClientProfileHandle profileHandle,
                                           ServiceHandle groupId,
                                           uint32 cid,
                                           uint16 scanHandle)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstStopSrcScanReq(gmapClientInst->libTask,
                                     groupId,
                                     cid,
                                     scanHandle);
}

void GmapClientBroadcastAsstRegisterNotificationReq(GmapClientProfileHandle profileHandle,
                                                    ServiceHandle groupId,
                                                    uint32 cid,
                                                    uint8 sourceId,
                                                    bool noficationEnable)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstRegisterNotificationReq(gmapClientInst->libTask,
                                              groupId,
                                              cid,
                                              sourceId,
                                              TRUE,
                                              noficationEnable);
}

void GmapClientBroadcastAsstReadReceiverSinkStateReq(GmapClientProfileHandle profileHandle,
                                                     ServiceHandle groupId,
                                                     uint32  cid)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstReadReceiveStateReq(gmapClientInst->libTask,
                                          groupId,
                                          cid);
}

void GmapClientBroadcastAsstStartSyncToSrcReq(GmapClientProfileHandle profileHandle,
                                              ServiceHandle groupId,
                                              TYPED_BD_ADDR_T *addrt,
                                              uint8 advSid)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstSyncToSrcStartReq(gmapClientInst->libTask,
                                        groupId,
                                        addrt,
                                        advSid);
}

void GmapClientBroadcastAsstTerminateSyncToSrcReq(GmapClientProfileHandle profileHandle,
                                                  ServiceHandle groupId,
												  uint16 syncHandle)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstTerminateSyncToSrcReq(gmapClientInst->libTask,
                                            groupId,
                                            syncHandle);
}

void GmapClientBroadcastAsstCancelSyncToSrcReq(GmapClientProfileHandle profileHandle,
                                               ServiceHandle groupId)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientBcastAsstSyncToSrcCancelReq(gmapClientInst->libTask,
                                         groupId);
}

void GmapClientBroadcastAsstAddSrcReq(GmapClientProfileHandle profileHandle,
                                      ServiceHandle groupId,
                                      uint32 cid,
                                      TYPED_BD_ADDR_T *sourceAddrt,
                                      GmapClientBroadcastSrcType bcastSrcType,
                                      uint16 syncHandle,
                                      uint8 sourceAdvSid,
                                      uint8 paSyncState,
                                      uint16 paInterval,
                                      uint32 broadcastId,
                                      uint8 numbSubGroups,
                                      GmapClientSubgroupInfo *subgroupInfo)
{
    bool srcColocated = (bcastSrcType == GMAP_CLIENT_BROADCAST_SRC_COLLOCATED) ? TRUE : FALSE;
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

	CapClientBcastAsstAddSrcReq(gmapClientInst->libTask,
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

void GmapClientBroadcastAsstModifySrcReq(GmapClientProfileHandle profileHandle,
                                         ServiceHandle groupId,
                                         GmapClientBroadcastSrcType bcastSrcType,
                                         uint16 syncHandle,
                                         uint8 sourceAdvSid,
                                         uint8 paSyncState,
                                         uint16 paInterval,
                                         uint8 numBrcastSinkInfo,
                                         GmapClientBroadcastSinkInfo* brcastSinkInfo,
                                         uint8 numbSubGroups,
                                         GmapClientSubgroupInfo *subgroupInfo)
{
    bool srcColocated = (bcastSrcType == GMAP_CLIENT_BROADCAST_SRC_COLLOCATED) ? TRUE : FALSE;
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);
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

    CapClientBcastAsstModifySrcReq(gmapClientInst->libTask,
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

void GmapClientBroadcastAsstRemoveSrcReq(GmapClientProfileHandle profileHandle,
                                         ServiceHandle groupId,
                                         uint8 numBrcastSinkInfo,
                                         GmapClientBroadcastSinkInfo* brcastSinkInfo)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);
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

    CapClientBcastAsstRemoveSrcReq(gmapClientInst->libTask,
                                   groupId,
                                   numBrcastSinkInfo,
                                   delegatorInfo);

    if (delegatorInfo != NULL)
        CsrPmemFree(delegatorInfo);
}

void GmapClientBroadcastAsstSetCodeRsp(ServiceHandle groupId,
                                       uint32  cid,
                                       uint8* broadcastcode,
                                       uint8 numBrcastSinkInfo,
                                       GmapClientBroadcastSinkInfo* brcastSinkInfo)
{
    CapClientBcastAsstSetCodeRsp(groupId,
                                 cid,
                                 brcastSinkInfo->sourceId,
                                 broadcastcode);

   CSR_UNUSED(numBrcastSinkInfo);
}

#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */
