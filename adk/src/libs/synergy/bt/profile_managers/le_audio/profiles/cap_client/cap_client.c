/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void CapClientInitReq(AppTask appTask, CapClientInitData* initData, CapClientRole role)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalInitReq);

    message->appTask = appTask;
    message->role = role;
    message->initData = NULL;

    if (initData)
    {
        message->initData = (CapClientInitData*)CsrPmemZalloc(sizeof(CapClientInitData));
        message->initData->handles = NULL;
        message->initData->cid = initData->cid;

        if (initData->handles)
        {
            message->initData->handles = initData->handles;
            initData->handles = NULL;
        }
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE, CAP_CLIENT_INTERNAL_INIT_REQ, message);
}

void CapClientAddNewDevReq(ServiceHandle groupId, CapClientInitData *initData, bool discoveryComplete)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalAddNewDevReq);

    message->groupId = groupId;
    message->discoveryComplete = discoveryComplete;
    message->initData = NULL;

    if (initData)
    {
        message->initData = (CapClientInitData*)CsrPmemZalloc(sizeof(CapClientInitData));
        message->initData->cid = initData->cid;
        message->initData->handles = NULL;

        if (initData->handles)
        {
            message->initData->handles = initData->handles;
            initData->handles = NULL;
        }
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                         CAP_CLIENT_INTERNAL_ADD_NEW_DEV_REQ,
                         message);
}

void CapClientRemoveDevReq(ServiceHandle groupId, uint32 cid)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalRemoveDevReq);

    message->groupId = groupId;
    message->cid = cid;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                         CAP_CLIENT_INTERNAL_REMOVE_DEV_REQ,
                         message);
}

void CapClientInitStreamControlReq(ServiceHandle groupId)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalInitStreamControlReq);
    message->groupId = groupId;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                         CAP_CLIENT_INTERNAL_INIT_STREAM_CONTROL_REQ,
                         message);
}

void CapClientDiscoverStreamCapabilitiesReq(ServiceHandle groupId,
                                     CapClientPublishedCapability attribute)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalDiscoverStreamCapReq);
    message->groupId = groupId;
    message->attribute = attribute;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                         CAP_CLIENT_INTERNAL_DISCOVER_STREAM_CAP_REQ,
                         message);
}

void CapClientDiscoverAvailableAudioContextReq(ServiceHandle groupId)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalDiscoverAvailAudioContextReq);
    message->groupId = groupId;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                         CAP_CLIENT_INTERNAL_DISCOVER_AVAIL_AUDIO_CONTEXT_REQ,
                         message);
}

void CapClientUnicastConnectReq(AppTask profileTask,
                          ServiceHandle groupId,
                          CapClientSreamCapability sinkConfig,
                          CapClientSreamCapability srcConfig,
                          CapClientTargetLatency targetLatency,
                          CapClientContext useCase,
                          CapClientAudioLocation sinkAudioLocations,
                          CapClientAudioLocation srcAudioLocations,
                          uint8 numOfMic,
                          CapClientCigConfigMode cigConfigMode,
                          const CapClientQhsConfig *cigConfig)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalUnicastConnectReq);
    message->groupId = groupId;
    message->highReliability = targetLatency;
    message->sinkConfig = sinkConfig;
    message->srcConfig = srcConfig;
    message->useCase = useCase;
    message->profileTask = profileTask;
    message->numOfMic = numOfMic;
    message->sinkAudioLocations = sinkAudioLocations;
    message->srcAudioLocations = srcAudioLocations;

    message->cigConfigMode = cigConfigMode;
    if (CAP_CLIENT_QHS_CONFIGURED(cigConfigMode) && cigConfig)
    {
        CsrMemCpy(&message->cigConfig, cigConfig, sizeof(CapClientQhsConfig));
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                      CAP_CLIENT_INTERNAL_UNICAST_CONNECT_REQ,
                      message);
}

void CapClientUnicastDisConnectReq(AppTask profileTask,
                                  ServiceHandle groupId,
                                  CapClientContext useCase)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalUnicastDisconnectReq);
    message->profileTask = profileTask;
    message->groupId = groupId;  
    message->useCase = useCase;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                     CAP_CLIENT_INTERNAL_UNICAST_DISCONNECT_REQ,
                     message);
}

void CapClientUnicastStartStreamReq(AppTask profileTask,
                              ServiceHandle groupId,
                              CapClientContext useCase,
                              uint8 metadataLen,
                              uint8 *metadataParam)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalUnicastStartStreamReq);
    message->groupId = groupId;
    message->useCase = useCase;
    message->metadataLen = metadataLen;
    message->profileTask = profileTask;
    message->metadataParam = NULL;

    if (metadataLen && metadataParam)
    {
        message->metadataParam = (uint8*)CsrPmemZalloc(metadataLen);
        CsrMemCpy(message->metadataParam, metadataParam, metadataLen);
    }
    else
    {
        message->metadataLen = 0;
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                      CAP_CLIENT_INTERNAL_UNICAST_START_STREAM_REQ,
                      message);
}

void CapClientUnicastUpdateAudioReq(AppTask profileTask,
                              ServiceHandle groupId,
                              CapClientContext useCase,
                              uint8 metadataLen,
                              uint8* metadataParam)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalUnicastUpdateAudioReq);
    message->groupId = groupId;
    message->useCase = useCase;
    message->metadataLen = metadataLen;
    message->profileTask = profileTask;
    message->metadataParam = NULL;

    if (metadataLen && metadataParam)
    {
        message->metadataParam = (uint8*)CsrPmemZalloc(metadataLen);
        CsrMemCpy(message->metadataParam, metadataParam, metadataLen);
    }
    else
    {
        message->metadataLen = 0;
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                      CAP_CLIENT_INTERNAL_UNICAST_UPDATE_AUDIO_REQ,
                      message);
}

void CapClientMuteReq(AppTask profileTask, ServiceHandle groupId, bool mute)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalMuteReq);
    message->profileTask = profileTask;
    message->groupId = groupId;
    message->muteState = mute;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                         CAP_CLIENT_INTERNAL_MUTE_REQ,
                         message);
}

void CapClientChangeVolumeReq(AppTask profileTask, ServiceHandle groupId, uint8 volumeSetting)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalChangeVolumeReq);
    message->profileTask = profileTask;
    message->groupId = groupId;
    message->volumeState = volumeSetting;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                        CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ,
                        message);
}

void CapClientInitOptionalServicesReq(ServiceHandle groupId, CapClientOptionalServices optServices)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalInitOptionalServicesReq);

    message->groupId = groupId;
    message->servicesMask = optServices;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                         CAP_CLIENT_INTERNAL_INIT_OPTIONAL_SERVICES_REQ,
                         message);

}

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
void CapClientSetMicpProfileAttbuteHandlesReq(AppTask profileTask, ServiceHandle groupId, uint32 cid, GattMicsClientDeviceData* micsHandles)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalSetMicpAttribHandlesReq);

    message->profileTask = profileTask;
    message->groupId = groupId;
    message->cid = cid;
    message->micsHandles = micsHandles;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                         CAP_CLIENT_INTERNAL_SET_MICP_PROFILE_ATTRIB_HANDLES_REQ,
                         message);

}

void CapClientSetMicStateReq(AppTask appTask, ServiceHandle groupId, uint32 cid, CapClientMicState micState)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalSetMicStateReq);
    message->profileTask = appTask;
    message->groupId = groupId;
    message->cid = cid;
    message->micState = micState;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                         CAP_CLIENT_INTERNAL_SET_MIC_STATE_REQ,
                         message);
}

void CapClientReadMicStateReq(AppTask appTask, ServiceHandle groupId, uint32  cid)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalReadMicStateReq);

    message->profileTask = appTask;
    message->groupId = groupId;
    message->cid = cid;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                     CAP_CLIENT_INTERNAL_READ_MIC_STATE_REQ,
                     message);
}
#endif

void CapClientRegisterTaskReq(AppTask profileTask, ServiceHandle groupId)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalRegisterTaskReq);
    message->groupId = groupId;
    message->profileTask = profileTask;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                       CAP_CLIENT_INTERNAL_REGISTER_TASK_REQ,
                       message);
}

void CapClientDeRegisterTaskReq(AppTask profileTask, ServiceHandle groupId)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalDeRegisterTaskReq);
    message->groupId = groupId;
    message->profileTask = profileTask;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                       CAP_CLIENT_INTERNAL_DEREGISTER_TASK_REQ,
                       message);
}

void CapClientUnicastStopStreamReq(AppTask profileTask, ServiceHandle groupId, bool doRelease)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalUnicastStopStreamReq);
    message->profileTask = profileTask;
    message->groupId = groupId;
    message->doRelease = doRelease;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                        CAP_CLIENT_INTERNAL_UNICAST_STOP_STREAM_REQ,
                        message);
}

void CapClientCsipReadReq(ServiceHandle groupId,
    uint32  cid,
    uint8 csipCharType)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalCsipReadReq);

    message->cid = cid;
    message->csipCharType = csipCharType;
    message->groupId = groupId;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
        CAP_CLIENT_INTERNAL_CSIP_READ_REQ,
        message);
}

void CapClientReadVolumeStateReq(AppTask appTask, ServiceHandle groupId, uint32  cid)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalReadVolumeStateReq);

    message->cid = cid;
    message->groupId = groupId;
    message->profileTask = appTask;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                     CAP_CLIENT_INTERNAL_READ_VOLUME_STATE_REQ,
                     message);
}

void CapClientUnicastCigTestConfigReq(AppTask profileTask,
    ServiceHandle groupId,
    CapClientContext useCase,
    const CapClientCigTestConfig *cigConfig)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalUnicastCigTestConfigReq);
    message->groupId = groupId;
    message->useCase = useCase;
    message->profileTask = profileTask;

    if (cigConfig)
    {
        message->cigConfig = CsrPmemZalloc(sizeof(CapClientCigTestConfig));
        CsrMemCpy(message->cigConfig, cigConfig, sizeof(CapClientCigTestConfig));

        if (cigConfig->cisTestConfig)
        {
            message->cigConfig->cisTestConfig = CsrPmemZalloc(sizeof(CapClientCisTestConfig));

            CsrMemCpy(message->cigConfig->cisTestConfig, cigConfig->cisTestConfig,
                sizeof(CapClientCisTestConfig));
        }
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                      CAP_CLIENT_INTERNAL_UNICAST_CIG_TEST_CONFIG_REQ,
                      message);

}

void CapClientUnicastSetVsConfigDataReq(AppTask profileTask,
    ServiceHandle groupId,
    uint8 vsConfigLen,
    const uint8 *vsConfig)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalUnicastSetVsConfigDataReq);
    message->groupId = groupId;
    message->profileTask = profileTask;
    message->metadataLen = 0;

    if (vsConfigLen && vsConfig)
    {
        message->metadataParam = (uint8*)CsrPmemZalloc(vsConfigLen);
        CsrMemCpy(message->metadataParam, vsConfig, vsConfigLen);

        message->metadataLen = vsConfigLen;
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                      CAP_CLIENT_INTERNAL_UNICAST_VS_SET_CONFIG_DATA_REQ,
                      message);
}
#endif /* INSTALL_LEA_UNICAST_CLIENT*/

#ifdef INSTALL_LEA_BROADCAST_SOURCE
void CapClientBcastSrcInitReq(AppTask appTask)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastSrcInitReq);
    message->appTask = appTask;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                        CAP_CLIENT_INTERNAL_BCAST_SRC_INIT_REQ,
                        message);
}

void CapClientBcastSrcDeinitReq(uint32 bcastSrcProfileHandle)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastSrcDeinitReq);
    message->bcastSrcProfileHandle = bcastSrcProfileHandle;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
        CAP_CLIENT_INTERNAL_BCAST_SRC_DEINIT_REQ,
        message);
}

void CapClientBcastSrcSetBroadcastId(uint32 bcastSrcProfileHandle, uint32 broadcastId)
{
    BapBroadcastSrcSetBroadcastId(bcastSrcProfileHandle, broadcastId);
}

void CapClientBcastSrcConfigReq(uint32 bcastSrcProfileHandle,
                                    uint8 ownAddrType,
                                    uint32 presentationDelay,
                                    uint8 numSubGroup,
                                    const CapClientBigSubGroup *subgroupInfo,
                                    const CapClientBcastInfo   *broadcastInfo,
                                    CapClientBigConfigMode mode,
                                    const CapClientQhsBigConfig* bigConfig)
{
    uint8 i = 0, j= 0;
    uint8 numBis = 0;
    uint8 metadataLen = 0;
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastSrcConfigReq);
    message->bcastSrcProfileHandle = bcastSrcProfileHandle;
    message->presentationDelay = presentationDelay;
    message->numSubgroup = numSubGroup;
    message->ownAddress = ownAddrType;

    if (message->numSubgroup && subgroupInfo)
    {
        message->subgroupInfo = (CapClientBigSubGroup*)
                              CsrPmemZalloc(numSubGroup * sizeof(CapClientBigSubGroup));

        for (i = 0; i < numSubGroup; i++)
        {
            numBis = subgroupInfo[i].numBis;
            metadataLen = subgroupInfo[i].metadataLen;

            message->subgroupInfo[i].lc3BlocksPerSdu = subgroupInfo[i].lc3BlocksPerSdu;
            message->subgroupInfo[i].config = subgroupInfo[i].config;
            message->subgroupInfo[i].targetLatency = subgroupInfo[i].targetLatency;
            message->subgroupInfo[i].useCase = subgroupInfo[i].useCase;

            /*  Copy Metadata */
            message->subgroupInfo[i].metadataLen = metadataLen;

            if (metadataLen && subgroupInfo[i].metadata)
            {
                message->subgroupInfo[i].metadata = CsrPmemZalloc(metadataLen * sizeof(uint8));
                SynMemCpyS(message->subgroupInfo[i].metadata, metadataLen, subgroupInfo[i].metadata, metadataLen);
            }

            /*  Copy BIS info*/
            message->subgroupInfo[i].numBis = numBis;

            for (j = 0; j < numBis; j++)
            {

                message->subgroupInfo[i].bisInfo[j].config = 
                                      subgroupInfo[i].bisInfo[j].config;
                message->subgroupInfo[i].bisInfo[j].lc3BlocksPerSdu = 
                                      subgroupInfo[i].bisInfo[j].lc3BlocksPerSdu;
                message->subgroupInfo[i].bisInfo[j].audioLocation =
                                      subgroupInfo[i].bisInfo[j].audioLocation;
                message->subgroupInfo[i].bisInfo[j].targetLatency =
                                      subgroupInfo[i].bisInfo[j].targetLatency;
            }
        }
    }

    if (broadcastInfo)
    {
        message->broadcastInfo = (CapClientBcastInfo*)
                   CsrPmemZalloc(sizeof(CapClientBcastInfo));

        message->broadcastInfo->broadcast = broadcastInfo->broadcast;
        message->broadcastInfo->flags = broadcastInfo->flags;
        message->broadcastInfo->appearanceValue = broadcastInfo->appearanceValue;
        message->broadcastInfo->bigNameLen = 0;
        message->broadcastInfo->bigName = NULL;

        if (broadcastInfo->bigNameLen && broadcastInfo->bigName)
        {
            message->broadcastInfo->bigNameLen = broadcastInfo->bigNameLen;
            message->broadcastInfo->bigName = CsrPmemZalloc(broadcastInfo->bigNameLen);
            SynMemCpyS(message->broadcastInfo->bigName, broadcastInfo->bigNameLen, 
                        broadcastInfo->bigName, broadcastInfo->bigNameLen);
        }
    }

    message->mode = mode;
    message->qhsConfig = NULL;
    
    if ((message->mode & CAP_CLIENT_BIG_CONFIG_MODE_QHS)== CAP_CLIENT_BIG_CONFIG_MODE_QHS)
    {
        message->qhsConfig = CsrPmemZalloc(sizeof(CapClientQhsBigConfig));
        message->qhsConfig->framing = bigConfig->framing;
        message->qhsConfig->phy = bigConfig->phy;
        message->qhsConfig->rtn = bigConfig->rtn;
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                        CAP_CLIENT_INTERNAL_BCAST_SRC_CONFIG_REQ,
                        message);
}


void CapClientBcastSrcStartStreamReq(uint32 bcastSrcProfileHandle,
                                   bool  encryption,
                                   uint8* broadcastCode)
{
    uint8 size = 0;
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastStartStreamSrcReq);
    message->bcastSrcProfileHandle = bcastSrcProfileHandle;
    message->encryption = encryption;

    size = BAP_BROADCAST_CODE_SIZE * sizeof(uint8);

    if (broadcastCode && encryption)
    {
        message->broadcastCode = (uint8*)CsrPmemZalloc(size);
        CsrMemCpy(message->broadcastCode, broadcastCode, size);
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                     CAP_CLIENT_INTERNAL_BCAST_SRC_START_STREAM_REQ,
                     message);

}


void CapClientBcastSrcStopStreamReq(uint32 bcastSrcProfileHandle)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastStopStreamSrcReq);
    message->bcastSrcProfileHandle = bcastSrcProfileHandle;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                      CAP_CLIENT_INTERNAL_BCAST_SRC_STOP_STREAM_REQ,
                      message);
}

void CapClientBcastSrcRemoveStreamReq(uint32 bcastSrcProfileHandle)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastRemoveStreamSrcReq);
    message->bcastSrcProfileHandle = bcastSrcProfileHandle;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                       CAP_CLIENT_INTERNAL_BCAST_SRC_REMOVE_STREAM_REQ,
                       message);
}

void CapClientBcastSrcUpdateStreamReq(uint32 bcastSrcProfileHandle,
                                      CapClientContext useCase,
                                      uint8 numSubgroup,
                                      uint8 metadataLen,
                                      const uint8 *metadata)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastUpdateStreamSrcReq);
    message->useCase = useCase;
    message->metadataLen = metadataLen;
    message->numSubgroup = numSubgroup;
    message->bcastSrcProfileHandle = bcastSrcProfileHandle;

    if (metadataLen && metadata)
    {
        message->metadata = (uint8*)CsrPmemZalloc(metadataLen);
        CsrMemCpy(message->metadata, metadata, metadataLen);
    }
    else
    {
        message->metadataLen = 0;
        message->metadata = NULL;
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                       CAP_CLIENT_INTERNAL_BCAST_SRC_UPDATE_STREAM_REQ,
                       message);
}
#endif /* INSTALL_LEA_BROADCAST_SOURCE*/

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
void CapClientBcastAsstStartSrcScanReq(AppTask profileTask,
                                      ServiceHandle groupId,
                                      uint32 cid,
                                      CapClientBcastSrcLocation bcastSrcType,
                                      CapClientBcastType  bcastType,
                                      CapClientContext filterContext,
                                      uint8 scanFlags,
                                      uint8 ownAddressType,
                                      uint8 scanningFilterPolicy)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastAsstStartSrcScanReq);
    message->cid = cid;
    message->groupId = groupId;
    message->filterContext = filterContext;
    message->bcastSrcType = bcastSrcType;
    message->ownAddressType = ownAddressType;
    message->bcastType = bcastType;
    message->scanFlags = scanFlags;
    message->scanningFilterPolicy = scanningFilterPolicy;
    message->profileTask = profileTask;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                         CAP_CLIENT_INTERNAL_BCAST_ASST_START_SRC_SCAN_REQ,
                         message);
}


void CapClientBcastAsstStopSrcScanReq(AppTask profileTask,
                                     ServiceHandle groupId,          
                                     uint32 cid,
                                     uint16 scanHandle)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastAsstStopSrcScanReq);
    message->groupId = groupId;
    message->profileTask = profileTask;
    message->scanHandle = scanHandle;
    message->cid = cid;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
        CAP_CLIENT_INTERNAL_BCAST_ASST_STOP_SRC_SCAN_REQ,
        message);
}

void CapClientBcastAsstSyncToSrcStartReq(AppTask profileTask,
                                            ServiceHandle groupId,
                                            TYPED_BD_ADDR_T *addrt,
                                            uint8 advSid)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastAsstSyncToSrcStartReq);
    message->addrt.type = addrt->type;
    message->addrt.addr.lap = addrt->addr.lap;
    message->addrt.addr.uap = addrt->addr.uap;
    message->addrt.addr.nap = addrt->addr.nap;
    message->advSid = advSid;
    message->groupId = groupId;
    message->profileTask = profileTask;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                       CAP_CLIENT_INTERNAL_BCAST_ASST_START_SYNC_TO_SRC_REQ,
                       message);

}

void CapClientBcastAsstTerminateSyncToSrcReq(AppTask profileTask,
                                          ServiceHandle groupId,
                                          uint16 syncHandle)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastAsstTerminateSyncToSrcReq);
    message->groupId = groupId;
    message->profileTask = profileTask;
    message->syncHandle = syncHandle;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                       CAP_CLIENT_INTERNAL_BCAST_ASST_TERMINATE_SYNC_TO_SRC_REQ,
                       message);
}

void CapClientBcastAsstSyncToSrcCancelReq(AppTask profileTask,
                                           ServiceHandle groupId)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastAsstSyncToSrcCancelReq);
    message->groupId = groupId;
    message->profileTask = profileTask;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
        CAP_CLIENT_INTERNAL_BCAST_ASST_CANCEL_SYNC_TO_SRC_REQ,
        message);
}

void CapClientBcastAsstAddSrcReq(AppTask profileTask,
                                   ServiceHandle groupId,
                                   uint32 cid,
                                   BD_ADDR_T *sourceAddrt,
                                   uint8 advertiserAddressType,
                                   bool    srcCollocated,
                                   uint16 syncHandle,
                                   uint8 sourceAdvSid,
                                   CapClientPaSyncState paSyncState,
                                   uint16 paInterval,
                                   uint32 broadcastId,
                                   uint8 numbSubGroups,
                                   const CapClientSubgroupInfo *subgroupInfo)
{
    uint8 i = 0;
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastAsstAddSrcReq);
    message->cid = cid;
    message->groupId = groupId;
    message->advertiserAddressType = advertiserAddressType;
    message->srcCollocated = srcCollocated;
    message->syncHandle = syncHandle;
    message->sourceAdvSid = sourceAdvSid;
    message->paSyncState = paSyncState;
    message->paInterval = paInterval;
    message->broadcastId = broadcastId;
    message->numbSubGroups  = numbSubGroups;
    message->profileTask = profileTask;

    bd_addr_copy(&message->sourceAddrt, sourceAddrt);

    for(i = 0; (i < numbSubGroups) && numbSubGroups && subgroupInfo; i++)
    {
        message->subgroupInfo[i] = (CapClientSubgroupInfo*)CsrPmemZalloc(sizeof(CapClientSubgroupInfo));
        message->subgroupInfo[i]->bisIndex = subgroupInfo[i].bisIndex;
        message->subgroupInfo[i]->metadataValue = NULL;
        message->subgroupInfo[i]->metadataLen = 0;

        if(subgroupInfo[i].metadataLen && subgroupInfo[i].metadataValue)
        {
            message->subgroupInfo[i]->metadataLen = subgroupInfo[i].metadataLen;
            message->subgroupInfo[i]->metadataValue = (uint8*)
                            CsrPmemZalloc(subgroupInfo[i].metadataLen*sizeof(uint8));
            SynMemCpyS(message->subgroupInfo[i]->metadataValue, subgroupInfo[i].metadataLen,
                                   subgroupInfo[i].metadataValue, subgroupInfo[i].metadataLen);
             
        }
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                       CAP_CLIENT_INTERNAL_BCAST_ASST_ADD_SRC_REQ,
                       message);
}


void CapClientBcastAsstModifySrcReq(AppTask profileTask,
                                   ServiceHandle groupId,
                                   bool srcCollocated,
                                   uint16 syncHandle,
                                   uint8 sourceAdvSid,
                                   CapClientPaSyncState  paSyncState,
                                   uint16 paInterval,
                                   uint8 infoCount,
                                   const CapClientDelegatorInfo* info,
                                   uint8 numbSubGroups,
                                   const CapClientSubgroupInfo* subgroupInfo)
{
    uint8 i = 0;
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastAsstModifySrcReq);
    message->groupId = groupId;
    message->srcCollocated = srcCollocated;
    message->syncHandle = syncHandle;
    message->sourceAdvSid = sourceAdvSid;
    message->paSyncState = paSyncState;
    message->paInterval = paInterval;
    message->numbSubGroups  = numbSubGroups;
    message->profileTask = profileTask;
    message->infoCount = 0;
    message->info = NULL;

    for(i = 0; (i < numbSubGroups) && numbSubGroups && subgroupInfo; i++)
    {
        message->subgroupInfo[i] = (CapClientSubgroupInfo*)CsrPmemZalloc(sizeof(CapClientSubgroupInfo));
        message->subgroupInfo[i]->bisIndex = subgroupInfo[i].bisIndex;

        if(subgroupInfo[i].metadataLen && subgroupInfo[i].metadataValue)
        {
            message->subgroupInfo[i]->metadataLen = subgroupInfo[i].metadataLen;
            message->subgroupInfo[i]->metadataValue = (uint8*)
                            CsrPmemZalloc(subgroupInfo[i].metadataLen*sizeof(uint8));
            CsrMemCpy(message->subgroupInfo[i]->metadataValue,
                                 subgroupInfo[i].metadataValue, subgroupInfo[i].metadataLen);

        }
    }

    if (infoCount && info)
    {
        message->infoCount = infoCount;
        message->info = (CapClientDelegatorInfo*)
                CsrPmemZalloc(sizeof(CapClientDelegatorInfo) * message->infoCount);

        for (i = 0; i < infoCount; i++)
        {
            message->info[i].cid = info[i].cid;
            message->info[i].sourceId = info[i].sourceId;
        }
    }
    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                       CAP_CLIENT_INTERNAL_BCAST_ASST_MODIFY_SRC_REQ,
                       message);

}

void CapClientBcastAsstRemoveSrcReq(AppTask profileTask,
                                  ServiceHandle groupId,
                                  uint8 infoCount,
                                  const CapClientDelegatorInfo* info)
{
    uint8 i = 0;
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastAsstRemoveSrcReq);
    message->groupId = groupId;
    message->profileTask = profileTask;

    if (infoCount && info)
    {
        message->infoCount = infoCount;
        message->info = (CapClientDelegatorInfo*)
               CsrPmemZalloc(sizeof(CapClientDelegatorInfo) * message->infoCount);

        for (i = 0; i < infoCount; i++)
        {
            message->info[i].cid = info[i].cid;
            message->info[i].sourceId = info[i].sourceId;
        }
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                       CAP_CLIENT_INTERNAL_BCAST_ASST_REMOVE_SRC_REQ,
                       message);
}

void CapClientBcastAsstRegisterNotificationReq(AppTask profileTask,
                                                ServiceHandle groupId,
                                                uint32 cid,
                                                uint8 sourceId,
                                                bool allSources,
                                                bool noficationEnable)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastAsstRegNotificationReq);
    message->groupId = groupId;
    message->cid = cid;
    message->sourceId = sourceId;
    message->profileTask = profileTask;
    message->notificationEnable = noficationEnable;
    message->allSources = allSources;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                        CAP_CLIENT_INTERNAL_BCAST_ASST_REG_FOR_NOTIFICATION_REQ,
                        message);
}

void CapClientBcastAsstReadReceiveStateReq(AppTask profileTask,
                                              ServiceHandle groupId,
                                              uint32 cid)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastAsstReadReceiveStateReq);
    message->profileTask = profileTask;
    message->cid = cid;
    message->groupId = groupId;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                        CAP_CLIENT_INTERNAL_BCAST_ASST_READ_RECEIVE_STATE_REQ,
                        message);
}


void CapClientUnlockCoordinatedSetReq(ServiceHandle groupId)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalUnlockCoordinatedSetReq);

    message->groupId = groupId;

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                       CAP_CLIENT_INTERNAL_UNLOCK_COORDINATED_SET_REQ,
                       message);
}

void CapClientBcastAsstSetCodeRsp(ServiceHandle groupId,
                               uint32  cid,
                               uint8 sourceId,
                               uint8* broadcastcode)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalBcastAsstSetCodeRsp);
    message->broadcastCode = NULL;
    message->groupId = groupId;
    message->cid = cid;
    message->sourceId = sourceId;

    if (broadcastcode)
    {
        message->broadcastCode = CsrPmemZalloc(BAP_BROADCAST_CODE_SIZE);
        CsrMemCpy(message->broadcastCode, broadcastcode, BAP_BROADCAST_CODE_SIZE);
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                    CAP_CLIENT_INTERNAL_BCAST_ASST_SET_CODE_RSP,
                    message);
}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

void  CapClientSetParamReq(AppTask profileTask,
                          uint32 profileHandle,
                          CapClientSreamCapability sinkConfig,
                          CapClientSreamCapability srcConfig,
                          CapClientParamType type,
                          uint8 numOfParamElems,
                          const void* paramElems)
{
    uint16 size = 0;
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalSetParamReq);
    message->profileHandle = profileHandle;
    message->sinkConfig = sinkConfig;
    message->srcConfig = srcConfig;
    message->paramType = type;
    message->profileTask = profileTask;
    message->numOfParamElems = numOfParamElems;

    if (message->paramType != CAP_CLIENT_PARAM_TYPE_NONE
         && numOfParamElems)
    {
        if (message->paramType == CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT ||
            message->paramType == CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT_V1)
        {
            if(numOfParamElems == 1)
            {
                if (message->paramType == CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT_V1)
                {
                    CapClientUnicastConnectParamV1 *unicastParams =
                        (CapClientUnicastConnectParamV1*)paramElems;
                    uint16 vendorDataSize;

                    size = sizeof(CapClientUnicastConnectParamV1) * numOfParamElems;

                    vendorDataSize = unicastParams->vsConfigLen;

                    message->paramElems = (void*)CsrPmemZalloc(size);
                    SynMemCpyS(&message->paramElems->unicastParamV1, size, paramElems, size);

                    if (vendorDataSize != 0)
                    {
                        message->paramElems->unicastParamV1.vsConfig = (uint8*) CsrPmemZalloc(vendorDataSize);
                        SynMemCpyS((uint8*)&message->paramElems->unicastParamV1.vsConfig,
                                   vendorDataSize,
                                   unicastParams->vsConfig, vendorDataSize);
                    }

                }
                else
                {
                    size = sizeof(CapClientUnicastConnectParam) * numOfParamElems;

                    message->paramElems = (void*)CsrPmemZalloc(size);
                    SynMemCpyS(&message->paramElems->unicastParam, size, paramElems, size);
                }
            }
        }
        else if (message->paramType == CAP_CLIENT_PARAM_TYPE_BCAST_CONFIG)
        {
            size = sizeof(CapClientBcastConfigParam) * numOfParamElems;

            message->paramElems = (void*)CsrPmemZalloc(size);
            SynMemCpyS(&message->paramElems->bcastParam, size, paramElems, size);
        }
    }

    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                         CAP_CLIENT_INTERNAL_SET_PARAM_REQ,
                         message);
}
