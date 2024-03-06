/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_broadcast_src.h"
#include "cap_client_util.h"
#include "cap_client_common.h"
#include "cap_client_debug.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "cap_client_unicast_connect_req.h"

#define CAP_CLIENT_GENERATE_BIS_PRIM_NUM          (0x98)
#define CAP_CLIENT_GENERATE_BIS_LSHIFT            (0x03)

#define CAP_CLIENT_INVALID_BIS_ID                 (0x00)
#define CAP_CLIENT_INVALID_BIS_HANDLE             (0x00)

#ifdef INSTALL_LEA_BROADCAST_SOURCE

static bool capClientBroadcastSrcJointStereoEnabled(CapClientBigConfigMode mode)
{
    bool isBroadcastSrcJointStereo = ((mode & CAP_CLIENT_BIG_CONFIG_MODE_JOINT_STEREO)
                           == CAP_CLIENT_BIG_CONFIG_MODE_JOINT_STEREO)? TRUE: FALSE;

    return isBroadcastSrcJointStereo;
}
/***********************************************************************************************************/
static BroadcastSrcInst* capClientGetUninitializedBcastSrcInst(CAP_INST *inst)
{
    BroadcastSrcInst* srcInst = (BroadcastSrcInst*)
                      CAP_CLIENT_GET_BCAST_SRC_PEEK_FIRST(inst->capClientBcastSrcList);
    while (srcInst)
    {
        if (srcInst->bcastSrcProfileHandle == CAP_CLIENT_INVALID_CID
            && srcInst->bcastSrcOpCnt.opReqCount > 0)
            break;
        else
            srcInst = srcInst->next;
    }

    return srcInst;
}

static void capClientFreeCapSubGroupInfo(BroadcastSrcInst **src)
{
    BroadcastSrcInst* bcastSrc = *src;
    uint8 i = 0;

    if (bcastSrc->numSubGroup && bcastSrc->subGroupInfo)
    {
        for (i = 0; i < bcastSrc->numSubGroup; i++)
        {
            if (bcastSrc->subGroupInfo[i].metadataLen)
            {
                CsrPmemFree(bcastSrc->subGroupInfo[i].metadata);
                bcastSrc->subGroupInfo[i].metadata = NULL;
            }
        }

        CsrPmemFree(bcastSrc->subGroupInfo);
        bcastSrc->subGroupInfo = NULL;
        bcastSrc->numSubGroup = 0;
    }
}
/***********************************************************************************************************/
static void capClientBroadcastSendUpstreamCfm(uint32 profileHandle,
                                       CapClientResult result,
                                       AppTask appTask,
                                       CapClientPrim type)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastCommonCfm);
    message->bcastSrcProfileHandle = profileHandle;
    message->result = capClientBroadcastSourceGetResultCode(result);

    CapClientMessageSend(appTask, type, message);
}

/*****************************************************UTILITY_FUNCTIONS******************************************************/
static CapClientBcastSubGroupInfo* capClientGetBcastBisHandles(BroadcastSrcInst* bcastSrc)
{
    if (bcastSrc->numSubGroup)
    {
        CapClientBcastSubGroupInfo* subGroupInfo = (CapClientBcastSubGroupInfo*)
                                   CsrPmemZalloc(bcastSrc->numSubGroup*sizeof(CapClientBcastSubGroupInfo));
        bool isBroadcastSrcJointStereo = capClientBroadcastSrcJointStereoEnabled(bcastSrc->mode);
        uint8 subGroupIndex;
        uint8 bisIndex;

        for (subGroupIndex = 0; subGroupIndex < bcastSrc->numSubGroup; subGroupIndex++)
        {
            subGroupInfo[subGroupIndex].audioConfig = (CapClientBcastAudioConfig*)
                CsrPmemZalloc(bcastSrc->subGroupInfo[subGroupIndex].numBis*sizeof(CapClientBcastAudioConfig));

            subGroupInfo[subGroupIndex].bisHandles = (uint16*)CsrPmemZalloc(bcastSrc->subGroupInfo[subGroupIndex].numBis*sizeof(uint16));

            for (bisIndex = 0;bisIndex < bcastSrc->subGroupInfo[subGroupIndex].numBis; bisIndex++)
            {
                /* Copy the bishandles for the subgroup */
                uint8 locCount = capClientNumOfBitSet(bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].audioLocation);
                subGroupInfo[subGroupIndex].bisHandles[bisIndex] = bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].bisHandle;

                /* Copy the audio configuration for the subgroup*/
                subGroupInfo[subGroupIndex].audioConfig[bisIndex].codecId = capClientGetCodecIdFromCapability(bcastSrc->subGroupInfo[subGroupIndex].config);
                subGroupInfo[subGroupIndex].audioConfig[bisIndex].vendorCodecId = capClientGetVendorCodecIdFromCapability(bcastSrc->subGroupInfo[subGroupIndex].config);
                subGroupInfo[subGroupIndex].audioConfig[bisIndex].companyId = capClientGetCompanyIdFromCapability(bcastSrc->subGroupInfo[subGroupIndex].config);
                subGroupInfo[subGroupIndex].audioConfig[bisIndex].samplingFrequency = capClientGetSamplingFreqFromCapability(bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].config);
                subGroupInfo[subGroupIndex].audioConfig[bisIndex].frameDuaration = capClientGetFrameDurationFromCapability(bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].config);
                subGroupInfo[subGroupIndex].audioConfig[bisIndex].audioChannelAllocation = bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].audioLocation;
                subGroupInfo[subGroupIndex].audioConfig[bisIndex].octetsPerFrame = 
                    capClientGetSduSizeFromCapability(bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].config, CAP_CLIENT_BIG_CONFIG_MODE_DEFAULT, locCount, FALSE);

                if (bcastSrc->bcastParam && (bcastSrc->bcastParam[subGroupIndex].subGroupConfig == bcastSrc->subGroupInfo[subGroupIndex].config))
                {
                    if(isBroadcastSrcJointStereo)
                    {
                       subGroupInfo[subGroupIndex].audioConfig[bisIndex].octetsPerFrame = (bcastSrc->bcastParam[subGroupIndex].sduSize / 2 );
                    }
                    else
                    {
                       subGroupInfo[subGroupIndex].audioConfig[bisIndex].octetsPerFrame = bcastSrc->bcastParam[subGroupIndex].sduSize;
                    }
                    CAP_CLIENT_INFO("\n capClientGetBcastBisHandles: Codec octetsPerFrame =%x \n", subGroupInfo[subGroupIndex].audioConfig[bisIndex].octetsPerFrame );
                }

            }

            /* Copy the meta data for the subgroup*/
            if (bcastSrc->subGroupInfo[subGroupIndex].metadataLen && bcastSrc->subGroupInfo[subGroupIndex].metadata)
            {
                subGroupInfo[subGroupIndex].metadata = (uint8*)CsrPmemZalloc(bcastSrc->subGroupInfo[subGroupIndex].metadataLen * sizeof(uint8));
                SynMemCpyS(subGroupInfo[subGroupIndex].metadata, bcastSrc->subGroupInfo[subGroupIndex].metadataLen,
                    bcastSrc->subGroupInfo[subGroupIndex].metadata, bcastSrc->subGroupInfo[subGroupIndex].metadataLen);
            }
            else
            {
                subGroupInfo[subGroupIndex].metadata = NULL;
            }

            /* Copy the number of  bis for the subgroup*/
            subGroupInfo[subGroupIndex].numBis = bcastSrc->subGroupInfo[subGroupIndex].numBis;
        }
        return subGroupInfo;
    }
    return NULL;
}

/*************************************************SETUP DATAPATH*********************************************/
static void capClientBroadCastSrcSetUpDataPathReqSend(CAP_INST* inst,
                                                        BroadcastSrcInst *bcastSrc,
                                                        uint8 subGroupIndex,
                                                        uint8 bisIndex,
                                                        uint16 bisHandles)
{
    CSR_UNUSED(inst);
    bool isBroadcastSrcJointStereo = capClientBroadcastSrcJointStereoEnabled(bcastSrc->mode);
    CapClientBcastSetupDatapath* datapath;
    datapath = (CapClientBcastSetupDatapath*)CsrPmemZalloc(sizeof(CapClientBcastSetupDatapath));

    datapath->isoHandle = bisHandles;
    datapath->codecId.companyId = 0;

    datapath->codecId.codecId = capClientGetCodecIdFromCapability(bcastSrc->subGroupInfo[subGroupIndex].config);
    datapath->codecId.vendorCodecId = capClientGetVendorCodecIdFromCapability(bcastSrc->subGroupInfo[subGroupIndex].config);
    /* Check for codecId, if its vendor defined then company id need to assign to Qualcomm Technologies Intl. Ltd.*/
    datapath->codecId.companyId = capClientGetCompanyIdFromCapability(bcastSrc->subGroupInfo[subGroupIndex].config);

    datapath->codecConfiguration.samplingFrequency = capClientGetSamplingFreqFromCapability(bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].config);
    datapath->codecConfiguration.frameDuaration = capClientGetFrameDurationFromCapability(bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].config);
    datapath->codecConfiguration.audioChannelAllocation = bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].audioLocation;
    datapath->codecConfiguration.octetsPerFrame =
         capClientGetSduSizeFromCapability(bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].config, CAP_CLIENT_BIG_CONFIG_MODE_DEFAULT, 1, FALSE);
    datapath->codecConfiguration.lc3BlocksPerSdu = bcastSrc->subGroupInfo[subGroupIndex].lc3BlocksPerSdu;

    if (bcastSrc->bcastParam && (bcastSrc->bcastParam[subGroupIndex].subGroupConfig == bcastSrc->subGroupInfo[subGroupIndex].config))
    {
       datapath->codecConfiguration.lc3BlocksPerSdu = bcastSrc->bcastParam[subGroupIndex].maxCodecFramesPerSdu;
       if(isBroadcastSrcJointStereo)
       {
           datapath->codecConfiguration.octetsPerFrame = (bcastSrc->bcastParam[subGroupIndex].sduSize / 2);
       }
       else
       {
          datapath->codecConfiguration.octetsPerFrame = bcastSrc->bcastParam[subGroupIndex].sduSize;
       }
       CAP_CLIENT_INFO("\n capClientBroadCastSrcSetUpDataPathReqSend: lc3BlocksPerSdu =%x, Codec octetsPerFrame =%x \n", datapath->codecConfiguration.lc3BlocksPerSdu, datapath->codecConfiguration.octetsPerFrame );
    }

    datapath->vendorDataLen = (uint8)bcastSrc->subGroupInfo[subGroupIndex].metadataLen;

    if(datapath->vendorDataLen != 0)
    {
        /* Don't free this memory as it is freed in BAP*/
        datapath->vendorData = (uint8*)CsrPmemZalloc(bcastSrc->subGroupInfo[subGroupIndex].metadataLen);
        SynMemCpyS(datapath->vendorData,
                   bcastSrc->subGroupInfo[subGroupIndex].metadataLen,
                   bcastSrc->subGroupInfo[subGroupIndex].metadata,
                   bcastSrc->subGroupInfo[subGroupIndex].metadataLen);
    }
 
#ifdef CSR_TARGET_PRODUCT_VM
    datapath->dataPathId = DATAPATH_ID_RAW_STREAM_ENDPOINTS_ONLY;
#else
    datapath->dataPathId = DATAPATH_ID;
#endif 

    datapath->dataPathDirection = DATAPATH_DIRECTION_INPUT;
    datapath->controllerDelay = 0;

    BapClientSetupDataPathReq(bcastSrc->bcastSrcProfileHandle, datapath);

    CsrPmemFree(datapath);
    datapath = NULL;
}

void capClientBroadcastHandleSetupDataPathCfm(CAP_INST *inst,
                              BapSetupDataPathCfm *cfm)
{
    CSR_UNUSED(inst);
    BroadcastSrcInst* bcastSrc = CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, cfm->handle);

    if (bcastSrc == NULL)
    {
        CAP_CLIENT_PANIC("\n capClientBroadcastHandleSetupDataPathCfm: brcstSrc is NULL \n");
        return;
    }

    capClientDecrementOpCounter(&bcastSrc->bcastSrcOpCnt);

    /* Check for the operation count for multiple bis handles , multiple setup data path cfm will come
     * So once confirmation for the final data path receieved then send the stream start conformation to 
     * The upper layer */
    if (bcastSrc->bcastSrcOpCnt.opReqCount == 0)
    {
        /* Send the confirmation to the upper layer with success */
        capClientBcastSourceStartStreamCfm(inst, 
                      bcastSrc->bcastSrcProfileHandle, 
                      cfm->result, 
                      bcastSrc);
    }
}


/*************************************************SRC ENABLE REQ**********************************************/

static void capClientSendBroadcastSrcStartStreamReq(uint8 numBis,
                                              uint32 bapBrcstProfileHandle,
                                              CapClientBigConfigParam *bigParam,
                                              uint8 bigId,
                                              bool encryption,
                                              uint8 *broadcastCode)
{

    BapBroadcastSrcEnableStreamReq(bapBrcstProfileHandle,
                                   bigId,
                                   bigParam,
                                   numBis,
                                   encryption,
                                   broadcastCode);

    CsrPmemFree(broadcastCode);
    broadcastCode = NULL;
    CsrPmemFree(bigParam);
    bigParam = NULL;
}

void capClientBcastSourceStartStreamCfm(CAP_INST       *inst,
                                       uint32             handle,
                                       CapClientResult    result,
                                       BroadcastSrcInst* brcstSrc)
{
    CSR_UNUSED(inst);
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastSrcStartStreamCfm);

    if (result == CAP_CLIENT_RESULT_SOURCE_STREAM_ALREADY_ACTIVATED)
    {
        message->result = result;
    }
    else
    {
        message->result = capClientBroadcastSourceGetResultCode(result);
    }

    message->bcastSrcProfileHandle = handle;

    if (brcstSrc)
    {
        message->bigId = brcstSrc->bigId;
        message->bigSyncDelay = brcstSrc->bigSyncDelay;
        message->numSubGroup = brcstSrc->numSubGroup;

        if (brcstSrc->bcastBigParam)
        {
            message->bigParameters = (CapClientBigParam*)
                              CsrPmemZalloc(sizeof(CapClientBigParam));

            SynMemCpyS(message->bigParameters, 
                         sizeof(CapClientBigParam), 
                           brcstSrc->bcastBigParam, 
                             sizeof(CapClientBigParam));

        }

        if (brcstSrc->numSubGroup)
        {
            message->subGroupInfo = capClientGetBcastBisHandles(brcstSrc);
        }

        CapClientMessageSend(brcstSrc->appTask,
                             CAP_CLIENT_BCAST_SRC_START_STREAM_CFM,
                              message);
    }
}

void capClientHandleBroadcastSrcEnableStreamCfm(CAP_INST *inst,
                      BapBroadcastSrcEnableStreamCfm *cfm)
{
    BroadcastSrcInst *bcastSrc = CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, cfm->handle);

    /* This should never happen */
    if (bcastSrc == NULL)
    {
        CAP_CLIENT_PANIC("\n capClientHandleBroadcastSrcEnableStreamCfm: brcstSrc is NULL \n");
        return;
    }

    CAP_CLIENT_INFO("\n capClientHandleBroadcastSrcEnableStreamCfm: Setup data Path result: %x numofBis: %x\n", cfm->result, cfm->numBis);

    /* Check for the result,if its success then only create setup data path */
    if (cfm->result == CAP_CLIENT_RESULT_SUCCESS)
    {
        uint8 i = 0;
        uint8 subGroupIndex;
        uint8 bisIndex;

        /* Free the old big parameters if they are already populated in previous iteration*/
        if (bcastSrc->bcastBigParam)
        {
            CsrPmemFree(bcastSrc->bcastBigParam);
            bcastSrc->bcastBigParam = NULL;
        }

        bcastSrc->bcastBigParam = (CapClientBigParam*)CsrPmemZalloc(sizeof(CapClientBigParam));

        /* Store the bigParameters */
        SynMemCpyS(bcastSrc->bcastBigParam, sizeof(CapClientBigParam), &cfm->bigParameters, sizeof(CapClientBigParam));
        bcastSrc->bigSyncDelay = cfm->bigSyncDelay;

        /* Store the bis handles in the respective subgroups and setup a data path for each bis handles */
        for (subGroupIndex = 0; subGroupIndex < bcastSrc->numSubGroup; subGroupIndex++)
        {
            for (bisIndex = 0; bisIndex < bcastSrc->subGroupInfo[subGroupIndex].numBis; bisIndex++)
            {
                bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].bisHandle = cfm->bisHandles[i];

                /* Call Setup data path for each bis handle */
                capClientIncrementOpCounter(&bcastSrc->bcastSrcOpCnt);
                capClientBroadCastSrcSetUpDataPathReqSend(inst, bcastSrc, subGroupIndex, bisIndex, cfm->bisHandles[i]);
                i++;
            }
        }
    }
    else
    {
        capClientBcastSourceStartStreamCfm(inst, 
                     bcastSrc->bcastSrcProfileHandle,
                     cfm->result, 
                     bcastSrc);
    }

    if (cfm->numBis)
    {
        CsrPmemFree(cfm->bisHandles);
        cfm->bisHandles = NULL;
    }
}

void handleBroadcastSrcStartStreamReq(CAP_INST* inst, const Msg msg)
{
    CapClientInternalBcastStartStreamSrcReq* req;
    BroadcastSrcInst* bcastSrc = NULL;
    CapClientBigConfigParam *bigParam = NULL;
    uint8 noOfBis = 0;
    bool isBroadcastSrcJointStereo;
    uint8 subGroupIndex;
    uint8 bisIndex;


    if (inst == NULL)
    {
        /* Do Nothing */
        CAP_CLIENT_INFO("\n(CAP)handleBroadcastSrcEnableReq: NULL instance ");
        return;
    }

    req = (CapClientInternalBcastStartStreamSrcReq*)msg;
    bcastSrc = CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, req->bcastSrcProfileHandle);
    
    if (bcastSrc == NULL)
    {
        CAP_CLIENT_PANIC("\n(CAP)handleBroadcastSrcEnableReq: Invalid Profile Handle ");
        return;
    }

    /* Stream is not configured  the numbSubgroup in bcastSrc instance shall be 0. *
     * CAP shall reject the start stream req in such cases.                        */

    if (bcastSrc->numSubGroup == 0
        || bcastSrc->subGroupInfo == NULL)
    {
        CAP_CLIENT_INFO("\n(CAP)handleBroadcastSrcEnableReq: Stream Active ");

        capClientBcastSourceStartStreamCfm(inst,
                                          bcastSrc->bcastSrcProfileHandle,
                                          CAP_CLIENT_RESULT_NOT_CONFIGURED,
                                          bcastSrc);

        return;
    }

    isBroadcastSrcJointStereo = capClientBroadcastSrcJointStereoEnabled(bcastSrc->mode);

    bigParam = (CapClientBigConfigParam*)CsrPmemZalloc(sizeof(CapClientBigConfigParam)*bcastSrc->numSubGroup);

    for (subGroupIndex = 0; subGroupIndex < bcastSrc->numSubGroup; subGroupIndex++)
    {
        for (bisIndex = 0; bisIndex < bcastSrc->subGroupInfo[subGroupIndex].numBis; bisIndex++)
        {
            bigParam[subGroupIndex].sduInterval = capClientGetSduIntervalForCapability(bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].config, isBroadcastSrcJointStereo);
            bigParam[subGroupIndex].maxSdu = 
                capClientGetSduSizeFromCapability(bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].config, bcastSrc->mode, 1, isBroadcastSrcJointStereo);
            bigParam[subGroupIndex].maxTransportLatency = capClientGetMaxLatencyFromCapabilityBcast(bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].config,
                                                                       bcastSrc->subGroupInfo[subGroupIndex].targetLatency);
            bigParam[subGroupIndex].rtn = capClientGetRtnFromCapabilityBcast(bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].config,
                                                                       bcastSrc->subGroupInfo[subGroupIndex].targetLatency);

            bigParam[subGroupIndex].phy = CAP_LE_2M_PHY;
            bigParam[subGroupIndex].framing = capClientGetFramingForCapability(bcastSrc->subGroupInfo[subGroupIndex].bisInfo[bisIndex].config);

            /* Check for the mode if mode is QHS then app has set the values for below params */
            if ((bcastSrc->mode & CAP_CLIENT_BIG_CONFIG_MODE_QHS) == CAP_CLIENT_BIG_CONFIG_MODE_QHS)
            {
                bigParam[subGroupIndex].phy = bcastSrc->qhsConfig.phy;
                bigParam[subGroupIndex].rtn = bcastSrc->qhsConfig.rtn;
                bigParam[subGroupIndex].framing = bcastSrc->qhsConfig.framing;
            }

            /*Override standard values with app supplied values*/
            if (bcastSrc->bcastParam)
            {
                bigParam[subGroupIndex].phy = bcastSrc->bcastParam[subGroupIndex].phy;
                bigParam[subGroupIndex].rtn = bcastSrc->bcastParam[subGroupIndex].rtn;
                bigParam[subGroupIndex].maxTransportLatency = bcastSrc->bcastParam[subGroupIndex].maxLatency;
                bigParam[subGroupIndex].maxSdu = bcastSrc->bcastParam[subGroupIndex].sduSize;
                bigParam[subGroupIndex].sduInterval = bcastSrc->bcastParam[subGroupIndex].sduInterval;
                CAP_CLIENT_INFO("(CAP)handleBroadcastSrcStartStreamReq: Application supplied values taken for bigParam\n");
            }



            bigParam[subGroupIndex].packing = CAP_CLIENT_PACKING_INTERLEAVED;
#ifdef AUTO_BRCST
            bigParam[subGroupIndex].maxTransportLatency = 10;  /* 10 ms */
            bigParam[subGroupIndex].packing = CAP_CLIENT_PACKING_SEQUENTIAL;  /* Sequential */
#endif /* AUTO_BRCST */
            noOfBis++;
        }
    }

    capClientSendBroadcastSrcStartStreamReq(noOfBis,
                                                req->bcastSrcProfileHandle,
                                                bigParam,
                                                bcastSrc->bigId,
                                                req->encryption,
                                                req->broadcastCode);
}


/*************************************************SRC CONFIG REQ**********************************************/

static void capClientFreeSubGroups(BapBigSubgroups* subGroupInfo, uint8 numOfSubGroups)
{
    uint8 i = 0;

    if (numOfSubGroups && subGroupInfo)
    {
        for (i = 0; i < numOfSubGroups; i++)
        {
            if (subGroupInfo[i].metadataLen && subGroupInfo->metadata)
            {
                CsrPmemFree(subGroupInfo[i].metadata);
                subGroupInfo[i].metadata = NULL;
            }
        }
        CsrPmemFree(subGroupInfo);
        subGroupInfo = NULL;
    }
}

static void capClientFreeCapSubGroups(CapClientBigSubGroup* subGroupInfo, uint8 numOfSubGroups)
{
    uint8 i = 0;

    if (numOfSubGroups && subGroupInfo)
    {
        for (i = 0; i < numOfSubGroups; i++)
        {
            if (subGroupInfo[i].metadataLen && subGroupInfo->metadata)
            {
                CsrPmemFree(subGroupInfo[i].metadata);
                subGroupInfo[i].metadata = NULL;
            }
        }
        CsrPmemFree(subGroupInfo);
        subGroupInfo = NULL;
    }
}

static void capClientBroadcastSrcConfigCfm(AppTask task,
                                          uint32 profileHandle,
                                          CapClientResult result)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastSrcConfigCfm);
    message->result = result;
    message->bcastSrcProfileHandle = profileHandle;

    CapClientMessageSend(task,
        CAP_CLIENT_BCAST_SRC_CONFIG_CFM,
        message);
}

static void capClientPopulateLocalSrcInst(CapClientLocalSubGroupInfo *localInfo,
                                          const CapClientBigSubGroup *bigSubGroupInfo,
                                          CapClientSreamCapability subGroupCap)
{
    uint8 i = 0;
    localInfo->targetLatency = bigSubGroupInfo->targetLatency;
    localInfo->config = bigSubGroupInfo->config;
    localInfo->useCase = bigSubGroupInfo->useCase;
    localInfo->metadataLen = bigSubGroupInfo->metadataLen;
    localInfo->numBis = bigSubGroupInfo->numBis;
    localInfo->lc3BlocksPerSdu = bigSubGroupInfo->lc3BlocksPerSdu;

    if (localInfo->metadataLen && bigSubGroupInfo->metadata)
    {
        localInfo->metadata = (uint8*)CsrPmemZalloc(localInfo->metadataLen);
        SynMemCpyS(localInfo->metadata, localInfo->metadataLen,
                    bigSubGroupInfo->metadata, bigSubGroupInfo->metadataLen);
    }

    for (i = 0; i < localInfo->numBis; i++)
    {
        localInfo->bisInfo[i].audioLocation = bigSubGroupInfo->bisInfo[i].audioLocation;
        localInfo->bisInfo[i].targetLatency = bigSubGroupInfo->bisInfo[i].targetLatency;
        localInfo->bisInfo[i].config = bigSubGroupInfo->bisInfo[i].config;
        localInfo->bisInfo[i].bisIndex = 0;
        localInfo->bisInfo[i].bisHandle = 0;

        if (bigSubGroupInfo->bisInfo[i].config == CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN)
            localInfo->bisInfo[i].config = subGroupCap;
    }
}

static BapBigSubgroups* capClientBroadcastSrcPopulateSubGroupInfo(uint8 numSubGroup,
                                                                BroadcastSrcInst *bcastSrc,
                                                                CapClientBigConfigMode mode,
                                                               const CapClientBigSubGroup* bigSubGroupInfo,
                                                               CAP_INST *inst)
{
    uint8 i = 0, j = 0;
    uint8 bisId = 0;
    BapBigSubgroups* subGroupInfo = NULL;
    uint8 indexTracker = 0;
    uint32 audioLocation = 0;
    bool isBroadcastSrcJointStereo = capClientBroadcastSrcJointStereoEnabled(mode);

    if (numSubGroup == 0
        || bigSubGroupInfo == NULL)
    {
        CAP_CLIENT_INFO("\n(CAP)capClientBroadcastSrcPopulateSubGroupInfo: Invalid Parameters ");
        return NULL;
    }

    /* Free any previous configurations which are present */
    capClientFreeCapSubGroupInfo(&bcastSrc);

    /* Assign mode */
    bcastSrc->mode = mode;

    /* Generate BIGId, Both CIG and BIG uses same generator*/
    if (bcastSrc->bigId == CAP_CLIENT_INVALID_BIG_ID)
    {
        if (inst->cigId == CAP_CLIENT_INVALID_CIG_ID)
            ++inst->cigId;

        bcastSrc->bigId = ++inst->cigId;
    }

    bcastSrc->numSubGroup = numSubGroup;

    if (numSubGroup)
    {
        bcastSrc->subGroupInfo = (CapClientLocalSubGroupInfo*)
                                   CsrPmemZalloc(numSubGroup * sizeof(CapClientLocalSubGroupInfo));
        subGroupInfo = (BapBigSubgroups*)CsrPmemZalloc(numSubGroup * sizeof(BapBigSubgroups));
    }


    for (i = 0; i < numSubGroup; i++)
    {
        /* Fill Subgroup level info  for local Instance*/
        uint8 locationCount = 0;
        ++bisId;

        capClientPopulateLocalSrcInst(&bcastSrc->subGroupInfo[i], 
                                      &bigSubGroupInfo[i], 
                                      bigSubGroupInfo[i].config);

        subGroupInfo[i].codecId =
            capClientGetCodecIdFromCapability(bigSubGroupInfo[i].config);
        subGroupInfo[i].vendorCodecId = 
            capClientGetVendorCodecIdFromCapability(bigSubGroupInfo[i].config);
        subGroupInfo[i].companyId = 
            capClientGetCompanyIdFromCapability(bigSubGroupInfo[i].config); /* TODO: Add api to get Company ID from config */


        subGroupInfo[i].samplingFrequency =
            capClientGetSamplingFreqFromCapability(bigSubGroupInfo[i].config);
        subGroupInfo[i].frameDuaration =
            capClientGetFrameDurationFromCapability(bigSubGroupInfo[i].config);

        subGroupInfo[i].lc3BlocksPerSdu = bigSubGroupInfo[i].lc3BlocksPerSdu;

        if (bcastSrc->bcastParam && (bcastSrc->bcastParam[i].subGroupConfig == bigSubGroupInfo[i].config))
        {
            subGroupInfo[i].lc3BlocksPerSdu = bcastSrc->bcastParam[i].maxCodecFramesPerSdu;
        }

        subGroupInfo[i].metadataLen = bigSubGroupInfo[i].metadataLen;
        subGroupInfo[i].streamingAudioContext = bigSubGroupInfo[i].useCase;

        if (bigSubGroupInfo[i].metadataLen && bigSubGroupInfo[i].metadata)
        {
            subGroupInfo[i].metadata = 
                (uint8*)CsrPmemZalloc(bigSubGroupInfo[i].metadataLen);

            SynMemCpyS(subGroupInfo[i].metadata,
                       bigSubGroupInfo[i].metadataLen,
                       bigSubGroupInfo[i].metadata,
                       bigSubGroupInfo[i].metadataLen);
        }

        subGroupInfo[i].numBis = bigSubGroupInfo[i].numBis;

        for (j = 0; j < subGroupInfo[i].numBis; j++)
        {
            /* update BisIndex */
            uint8 locCount = capClientNumOfBitSet(bigSubGroupInfo[i].bisInfo[j].audioLocation);
            subGroupInfo[i].bisInfo[j].bisIndex =
                bcastSrc->subGroupInfo[i].bisInfo[j].bisIndex = bisId + (indexTracker++); 

            /* Fill BIS level Info */

            subGroupInfo[i].bisInfo[j].codecConfigurations.audioChannelAllocation
                = bigSubGroupInfo[i].bisInfo[j].audioLocation;
            subGroupInfo[i].bisInfo[j].codecConfigurations.frameDuaration
                = capClientGetFrameDurationFromCapability(bigSubGroupInfo[i].bisInfo[j].config);
            subGroupInfo[i].bisInfo[j].codecConfigurations.samplingFrequency
                = capClientGetSamplingFreqFromCapability(bigSubGroupInfo[i].bisInfo[j].config);

            subGroupInfo[i].bisInfo[j].codecConfigurations.lc3BlocksPerSdu
                                     = bigSubGroupInfo[i].bisInfo[j].lc3BlocksPerSdu;
            subGroupInfo[i].bisInfo[j].codecConfigurations.octetsPerFrame
                = capClientGetSduSizeFromCapability(bigSubGroupInfo[i].bisInfo[j].config, CAP_CLIENT_CIG_CONFIG_MODE_DEFAULT, locCount, FALSE);

            if (bcastSrc->bcastParam && (bcastSrc->bcastParam[i].subGroupConfig == bigSubGroupInfo[i].config))
            {
                subGroupInfo[i].bisInfo[j].codecConfigurations.lc3BlocksPerSdu = bcastSrc->bcastParam[i].maxCodecFramesPerSdu;
                if (isBroadcastSrcJointStereo)
                   subGroupInfo[i].bisInfo[j].codecConfigurations.octetsPerFrame = (bcastSrc->bcastParam[i].sduSize/2);
                else
                   subGroupInfo[i].bisInfo[j].codecConfigurations.octetsPerFrame = bcastSrc->bcastParam[i].sduSize;
            }

            /*Get Subgroup Audio location */
            audioLocation |= bigSubGroupInfo[i].bisInfo[j].audioLocation;
        }

        locationCount = capClientNumOfBitSet(audioLocation);
        subGroupInfo[i].audioChannelAllocation = audioLocation;
        subGroupInfo[i].octetsPerFrame =
               capClientGetSduSizeFromCapability(bigSubGroupInfo[i].config, CAP_CLIENT_CIG_CONFIG_MODE_DEFAULT, locationCount, FALSE);

        if ((bcastSrc->bcastParam) && (bcastSrc->bcastParam[i].subGroupConfig == bigSubGroupInfo[i].config))
        {
            if (isBroadcastSrcJointStereo)
            {
                subGroupInfo[i].octetsPerFrame = (bcastSrc->bcastParam[i].sduSize / 2);
            }
            else
            {
                subGroupInfo[i].octetsPerFrame = bcastSrc->bcastParam[i].sduSize;
            }
            subGroupInfo[i].lc3BlocksPerSdu = bcastSrc->bcastParam[i].maxCodecFramesPerSdu;
        }

        CAP_CLIENT_INFO("\n(CAP): capClientBroadcastSrcPopulateSubGroupInfo: Codec octetsPerFrame: %x  lc3BlocksPerSdu =%x, isBroadcastSrcJointStereo =%x\n",
            subGroupInfo[i].octetsPerFrame,
            subGroupInfo[i].lc3BlocksPerSdu,
            isBroadcastSrcJointStereo);
    }

    return subGroupInfo;
}

static void capClientSendBroadcastSrcConfigReq(BroadcastSrcInst* bcastSrc,
                                             CapClientInternalBcastSrcConfigReq *req,
                                             CAP_INST* inst)
{
    static BapBroadcastInfo broadcastInfo = { 0 };
    uint8 numOfSubGroups = 0;
    BapBigSubgroups* subGroupInfo = NULL;
    uint8 bigNameLen = 0;
    uint8* bigName = NULL;

    numOfSubGroups = req->numSubgroup;
    CAP_CLIENT_INFO("\n(CAP)capClientSendBroadcastSrcConfigReq: numSubGroup: %d \n ", numOfSubGroups);

    /*populate SubgroupInfo*/
    if (numOfSubGroups)
    {
        subGroupInfo = capClientBroadcastSrcPopulateSubGroupInfo(numOfSubGroups, 
                                                                 bcastSrc,
                                                                 req->mode,
                                                                 req->subgroupInfo, inst);

        if (subGroupInfo == NULL)
        {
            CAP_CLIENT_INFO("\n(CAP)capClientSendBroadcastSrcConfigReq: subGroupInfo is NULL \n ");

            capClientBroadcastSrcConfigCfm(bcastSrc->appTask,
                                           req->bcastSrcProfileHandle,
                                           CAP_CLIENT_RESULT_INVALID_PARAMETER);
            return;
        }
    }

    /* populate broadcastInfo */
    if (req->broadcastInfo)
    {
        broadcastInfo.appearanceValue  = req->broadcastInfo->appearanceValue;
        broadcastInfo.broadcast = req->broadcastInfo->broadcast;
        broadcastInfo.flags = req->broadcastInfo->flags;

        bigNameLen = req->broadcastInfo->bigNameLen;

        if (bigNameLen)
        {
            bigName = (uint8*)CsrPmemZalloc(bigNameLen);
            SynMemCpyS(bigName, bigNameLen, req->broadcastInfo->bigName, bigNameLen);
        }
    }

    /* Copy QHS configuration */

    if (((bcastSrc->mode & CAP_CLIENT_BIG_CONFIG_MODE_QHS) == CAP_CLIENT_BIG_CONFIG_MODE_QHS)
         && req->qhsConfig)
    {
        bcastSrc->qhsConfig.framing = req->qhsConfig->framing;
        bcastSrc->qhsConfig.phy = req->qhsConfig->phy;
        bcastSrc->qhsConfig.rtn = req->qhsConfig->rtn;
    }
    else
    {
        bcastSrc->qhsConfig.framing = 0;
        bcastSrc->qhsConfig.phy = 0;
        bcastSrc->qhsConfig.rtn = 0;
    }

    BapBroadcastSrcConfigureStreamReq(req->bcastSrcProfileHandle,
                                     bcastSrc->bigId,
                                     req->ownAddress,
                                     req->presentationDelay,
                                     numOfSubGroups,
                                     subGroupInfo,
                                     &broadcastInfo,
                                     bigNameLen,
                                     (char*)bigName);

    if(bigNameLen)
        CsrPmemFree(bigName);


    capClientFreeSubGroups(subGroupInfo, numOfSubGroups);
    capClientFreeCapSubGroups(req->subgroupInfo, req->numSubgroup);


    if (req->mode != CAP_CLIENT_BIG_CONFIG_MODE_DEFAULT)
    {
        CsrPmemFree(req->qhsConfig);
        req->qhsConfig = NULL;
    }

    if (req->broadcastInfo)
    {
        if (req->broadcastInfo->bigNameLen && 
                     req->broadcastInfo->bigName)
        {
            CsrPmemFree(req->broadcastInfo->bigName);
            req->broadcastInfo->bigName = NULL;
        }

        CsrPmemFree(req->broadcastInfo);
        req->broadcastInfo = NULL;
    }
}

void capClientHandleBroadcastSrcConfigureStreamCfm(CAP_INST *inst,
                      BapBroadcastSrcConfigureStreamCfm *cfm)
{
    CapClientResult result = capClientBroadcastSourceGetResultCode(cfm->result);

    BroadcastSrcInst* bcastSrc = (BroadcastSrcInst*)
         CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, cfm->handle);

    if (bcastSrc == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP)capClientHandleBroadcastSrcConfigureStreamCfm: Invalid Parameters ");
        return;
    }

    capClientBroadcastSrcConfigCfm(bcastSrc->appTask, cfm->handle, result);
}

void handleBroadcastSrcConfigReq(CAP_INST* inst, const Msg msg)
{
     CapClientInternalBcastSrcConfigReq *req =
                 (CapClientInternalBcastSrcConfigReq*)msg;
     BroadcastSrcInst* bcastSrc = NULL;

    if (inst == NULL)
    {
        /* Do Nothing */
        return;
    }

    bcastSrc = (BroadcastSrcInst*)
        CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, req->bcastSrcProfileHandle);

    /* If Instance already exist, then return */
    if (bcastSrc == NULL)
    {
        CAP_CLIENT_ERROR("(CAP) handleBroadcastSrcConfigReq: Invalid Broadcast Handle: \n");
        return;
    }

    if ((req->broadcastInfo != NULL)
		&& (req->broadcastInfo->broadcast == 
             (CAP_CLIENT_SQ_PUBLIC_BROADCAST | CAP_CLIENT_HQ_PUBLIC_BROADCAST))
                && (req->numSubgroup < 2))
    {
        capClientBroadcastSrcConfigCfm(bcastSrc->appTask,
                                 bcastSrc->bcastSrcProfileHandle,
                                 CAP_CLIENT_RESULT_INVALID_PARAMETER);
        return;
    }

    if ((bcastSrc->bcastParam) && (bcastSrc->numBcastParam != req->numSubgroup))
    {
        CAP_CLIENT_WARNING("(CAP) handleBroadcastSrcConfigReq:  App supplied Parameters not applied\n");
    }

    capClientSendBroadcastSrcConfigReq(bcastSrc, req, inst);
}

/**************************************************BROADCAST SRC INIT*******************************************/

void capClientSendBroadcastSrcInitCfm(AppTask appTask, uint32 pHandle, CapClientResult result)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastSrcInitCfm);
    message->bcastSrcProfileHandle = pHandle;
    message->result = capClientBroadcastSourceGetResultCode(result);

    CapClientMessageSend(appTask,
                        CAP_CLIENT_BCAST_SRC_INIT_CFM,
                        message);
}

void capClientHandleBapBroadcastInitCfm(CAP_INST* inst, const BapInitCfm* cfm)
{
    BroadcastSrcInst* srcInst = NULL;

    if (inst == NULL)
    {
        /* Do Nothing */
        return;
    }

    /* Get the unintialised instance of Broadcast that is Src with invalid profileHandle 
     * and non zero requestCounter from BroadcastSrcList */
    srcInst = capClientGetUninitializedBcastSrcInst(inst);

    if (srcInst == NULL)
    {
        CAP_CLIENT_PANIC("(CAP) capClientHandleBapBroadcastInitCfm: NULL Instance \n");
        return;
    }

    if (cfm->result != BAP_RESULT_INPROGRESS)
    {
        capClientDecrementOpCounter(&srcInst->bcastSrcOpCnt);
        CAP_CLIENT_INFO("(CAP) capClientHandleBapBroadcastInitCfm:Post decrement ReqCount: %d \n", 
                                                                   srcInst->bcastSrcOpCnt.opReqCount);

    }

    if (CAP_CLIENT_REQ_COMPLETE(srcInst->bcastSrcOpCnt))
    {
         srcInst->bcastSrcProfileHandle = cfm->handle;
    }

    capClientSendBroadcastSrcInitCfm(srcInst->appTask, cfm->handle, cfm->result);
}

void handleBroadcastSrcInitReq(CAP_INST* inst, const Msg msg)
{
    BapInitData initData;
    CapClientInternalBcastSrcInitReq *req =
                 (CapClientInternalBcastSrcInitReq*)msg;
    BroadcastSrcInst *bcastSrc = NULL;

    if (inst == NULL)
    {
        /* Send NULL instance error to req->appTask */
        capClientSendBroadcastSrcInitCfm(req->appTask, 0, CAP_CLIENT_RESULT_NULL_INSTANCE);
        return;
    }

    bcastSrc = (BroadcastSrcInst*)CAP_CLIENT_ADD_BCAST_SRC(inst->capClientBcastSrcList);
    bcastSrc->appTask = req->appTask;
    bcastSrc->bcastSrcProfileHandle = INVALID_CONNID;

    CAP_CLIENT_INFO("(CAP) handleBroadcastSrcInitReq: ReqCount: %d \n", bcastSrc->bcastSrcOpCnt.opReqCount);
    capClientIncrementOpCounter(&bcastSrc->bcastSrcOpCnt);

    initData.cid = INVALID_CONNID;
    initData.role = BAP_ROLE_BROADCAST_SOURCE;
    initData.handles = NULL;

    BapInitReq(CSR_BT_CAP_CLIENT_IFACEQUEUE, initData);
}

/****************************************************Deinit***************************************************/

void  capClientRemoveBcastSrcInst(CAP_INST* inst, const Msg msg)
{
    BapDeinitCfm* cfm = (BapDeinitCfm*)msg;
    AppTask appTask;
    BroadcastSrcInst* bcastSrc = NULL;

    CAP_CLIENT_INFO("(CAP) capClientRemoveBcastSrcInst: Result: 0x%x \n", cfm->result);

    if (inst == NULL)
    {
        /* Nowhere to send from here*/
        CAP_CLIENT_INFO("(CAP) capClientRemoveBcastSrcInst: CAP_INST is null: \n");
        return;
    }

    bcastSrc = (BroadcastSrcInst*)
          CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, cfm->handle);

    if (bcastSrc == NULL)
    {
        CAP_CLIENT_PANIC("(CAP) capClientRemoveBcastSrcInst: Invalid Broadcast Handle: \n");
        return;
    }

    appTask = bcastSrc->appTask;

    /* remove the Source Instance if the Deinit is successful*/
    if (cfm->result == BAP_RESULT_SUCCESS)
    {
        CAP_CLIENT_REMOVE_BCAST_SRC(inst->capClientBcastSrcList, (CsrCmnListElm_t*)bcastSrc);
    }

    /* send Cfm to upper layer */
    if (cfm->result != BAP_RESULT_INPROGRESS)
    {
        capClientBroadcastSendUpstreamCfm(cfm->handle,
                                         cfm->result,
                                         appTask,
                                         CAP_CLIENT_BCAST_SRC_DEINIT_CFM);
    }

    if (cfm->result == BAP_RESULT_SUCCESS)
    {
        CsrPmemFree(cfm->handles);
        cfm->handles = NULL;
    }

}

void handleBroadcastSrcDeinitReq(CAP_INST* inst, const Msg msg)
{
    CapClientInternalBcastSrcDeinitReq* req =
                      (CapClientInternalBcastSrcDeinitReq*)msg;
    BroadcastSrcInst* bcastSrc = NULL;
    BapRole role = BAP_ROLE_BROADCAST_SOURCE;

    if (inst == NULL)
    {
    
        /* Nowhere to send from here*/
        CAP_CLIENT_INFO("(CAP) handleBroadcastSrcDeinitReq: CAP_INST is null: \n");
        return;
    }

    bcastSrc = (BroadcastSrcInst*)
        CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, req->bcastSrcProfileHandle);

    /* If Instance already exist, then return */
    if (bcastSrc == NULL)
    {
        CAP_CLIENT_PANIC("(CAP) handleBroadcastSrcDeinitReq: Invalid Broadcast Handle: \n");
        return;
    }

    BapDeinitReq(req->bcastSrcProfileHandle, role);
}

/************************************************************DISABLE SRC**************************************/
void handleBroadcastSrcStopStreamReq(CAP_INST* inst, const Msg msg)
{
    CapClientInternalBcastStopStreamSrcReq* req =
                       (CapClientInternalBcastStopStreamSrcReq*)msg;
    BroadcastSrcInst* bcastSrc = NULL;

    CAP_CLIENT_INFO("(CAP) handleBroadcastSrcStopStreamReq \n");

    if (inst == NULL)
    {
        /* Do Nothing */
        CAP_CLIENT_ERROR("(CAP) Null Instance \n");
        return;
    }

    bcastSrc = (BroadcastSrcInst*)
        CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, req->bcastSrcProfileHandle);

    if (bcastSrc == NULL)
    {
        CAP_CLIENT_PANIC("\n(CAP)handleBroadcastSrcStopStreamReq: Invalid Parameters ");
        return;
    }

    BapBroadcastSrcDisableStreamReq(req->bcastSrcProfileHandle, bcastSrc->bigId);
}


void capClientHandleBroadcastSrcStopStreamCfm(CAP_INST* inst,
                       BapBroadcastSrcDisableStreamCfm* cfm)
{
    BroadcastSrcInst* bcastSrc = NULL;
    uint8 i = 0, j = 0;
    bcastSrc = (BroadcastSrcInst*)
        CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, cfm->handle);

    if (bcastSrc == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP)capClientHandleBroadcastSrcStopStreamCfm: Invalid Parameters ");
        return;
    }

    /* Reset BIS handles */
    for (i = 0; i < bcastSrc->numSubGroup; i++)
        for (j = 0; j < bcastSrc->subGroupInfo[i].numBis; j++)
            bcastSrc->subGroupInfo[i].bisInfo[j].bisHandle = CAP_CLIENT_INVALID_BIS_HANDLE;

    if (cfm->result != BAP_RESULT_INPROGRESS)
    {
        capClientBroadcastSendUpstreamCfm(cfm->handle,
                                        cfm->result,
                                        bcastSrc->appTask,
                                        CAP_CLIENT_BCAST_SRC_STOP_STREAM_CFM);
    }
}

/*************************************************************************************************************/

/************************************************************RELEASE SRC**************************************/

void handleBroadcastSrcRemoveStreamReq(CAP_INST* inst, const Msg msg)
{
    CapClientInternalBcastRemoveStreamSrcReq* req =
             (CapClientInternalBcastRemoveStreamSrcReq*)msg;
    BroadcastSrcInst* bcastSrc = NULL;


    CAP_CLIENT_INFO("(CAP) handleBroadcastSrcRemoveStreamReq \n");

    if (inst == NULL)
    {
        /* Do Nothing */
        CAP_CLIENT_ERROR("(CAP) Null Instance \n");
        return;
    }

    bcastSrc = (BroadcastSrcInst*)
        CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, req->bcastSrcProfileHandle);

    if (bcastSrc == NULL)
    {
        CAP_CLIENT_PANIC("(CAP)handleBroadcastSrcRemoveStreamReq: Null  Broadcast Src Instance \n");
        return;
    }

    BapBroadcastSrcReleaseStreamReq(req->bcastSrcProfileHandle, bcastSrc->bigId);
}

void capClientHandleBroadcastSrcRemoveStreamCfm(CAP_INST* inst,
                          BapBroadcastSrcReleaseStreamCfm* cfm)
{
    BroadcastSrcInst* bcastSrc = NULL;
    bcastSrc = (BroadcastSrcInst*)
        CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, cfm->handle);

    if (bcastSrc == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP)capClientHandleBroadcastSrcRemoveStreamCfm: Invalid Parameters ");
        return;
    }

    if (cfm->result != BAP_RESULT_INPROGRESS)
    {

        /* On Removing the stream free subgroupInfo and reset numSubGroup to 0 */
        if(cfm->result == BAP_RESULT_SUCCESS)
            capClientFreeCapSubGroupInfo(&bcastSrc);

        capClientBroadcastSendUpstreamCfm(cfm->handle,
                                         cfm->result,
                                         bcastSrc->appTask,
                                         CAP_CLIENT_BCAST_SRC_REMOVE_STREAM_CFM);

    }
}


/***************************************************************************************************/
/**************************************UPDATE AUDIO*************************************************/
void handleBroadcastSrcUpdateStreamReq(CAP_INST* inst, const Msg msg)
{
    CapClientInternalBcastUpdateStreamSrcReq* req =
                                 (CapClientInternalBcastUpdateStreamSrcReq*)msg;
    uint8 i = 0;
    BapMetadata* metadata = NULL;
    BroadcastSrcInst* bcastSrc = NULL;


    CAP_CLIENT_INFO("(CAP) handleBroadcastSrcUpdateStreamReq \n");

    if (inst == NULL)
    {
        /* Do Nothing */
        CAP_CLIENT_ERROR("(CAP) Null Instance \n");
        return;
    }

    bcastSrc = (BroadcastSrcInst*)
        CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, req->bcastSrcProfileHandle);

    if (bcastSrc == NULL)
    {
        CAP_CLIENT_PANIC("(CAP)handleBroadcastSrcUpdateStreamReq: Null BcastSrc Instance \n");
        return;
    }

    if (req->numSubgroup)
    {
        metadata = (BapMetadata*)CsrPmemZalloc(req->numSubgroup * sizeof(BapMetadata));

        for (i = 0; i < req->numSubgroup; i++)
        {
            metadata[i].streamingAudioContext = req->useCase;

            if (req->metadataLen && req->metadata)
            {
                metadata[i].metadataLen = req->metadataLen;
                metadata[i].metadata = (uint8*)CsrPmemZalloc(req->metadataLen);
                CsrMemCpy(metadata[i].metadata, req->metadata, req->metadataLen);
            }
            else
            {
                metadata[i].metadataLen = 0;
                metadata[i].metadata = NULL;
            }

        }
    }

    BapBroadcastSrcUpdateMetadataReq(bcastSrc->bcastSrcProfileHandle,
                                    bcastSrc->bigId,
                                    req->numSubgroup, metadata);

    if (req->metadataLen)
    {
        CsrPmemFree(req->metadata);
        req->metadata = NULL;
    }

    for (i = 0; i < req->numSubgroup; i++)
    {
        if (req->metadataLen)
        {
            CsrPmemFree(metadata[i].metadata);
            metadata[i].metadata = NULL;
        }
    }

    CsrPmemFree(metadata);
    metadata = NULL;
}

void capClientHandleBroadcastSrcUpdateStreamCfm(CAP_INST* inst,
                                     BapBroadcastSrcUpdateMetadataStreamCfm *cfm)
{
    BroadcastSrcInst *bcastSrc = (BroadcastSrcInst*)
        CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, cfm->handle);

    if (bcastSrc == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP)capClientHandleBroadcastSrcUpdateStreamCfm: Invalid Parameters ");
        return;
    }

    capClientBroadcastSendUpstreamCfm(bcastSrc->bcastSrcProfileHandle,
                             cfm->result,
                             bcastSrc->appTask,
                             CAP_CLIENT_BCAST_SRC_UPDATE_STREAM_CFM);
}


void capClientHandleBroadcastSourceMsg(CAP_INST* inst, const Msg msg)
{
    CsrBtGattPrim* prim = (CsrBtGattPrim*)msg;

    if (inst->capClientBcastSrcList == NULL 
          || inst->capClientBcastSrcList->count == 0)
    {
        CAP_CLIENT_INFO("\n(CAP)capClientHandleBroadcastSourceMsg: No Broadcast Soource Present \n");
        return;
    }

    switch (*prim)
    {
        case BAP_BROADCAST_SRC_CONFIGURE_STREAM_CFM:
        {
            BapBroadcastSrcConfigureStreamCfm *cfm =
                             (BapBroadcastSrcConfigureStreamCfm*)msg;

            capClientHandleBroadcastSrcConfigureStreamCfm(inst, cfm);
        }
        break;

        case BAP_BROADCAST_SRC_ENABLE_STREAM_CFM:
        {
            BapBroadcastSrcEnableStreamCfm *cfm =
                             (BapBroadcastSrcEnableStreamCfm*)msg;

            capClientHandleBroadcastSrcEnableStreamCfm(inst, cfm);
        }
        break;

        case BAP_BROADCAST_SRC_DISABLE_STREAM_CFM:
        {
            BapBroadcastSrcDisableStreamCfm* cfm =
                (BapBroadcastSrcDisableStreamCfm*)msg;

            capClientHandleBroadcastSrcStopStreamCfm(inst, cfm);
        }
        break;

        case BAP_BROADCAST_SRC_RELEASE_STREAM_CFM:
        {
            BapBroadcastSrcReleaseStreamCfm* cfm =
                (BapBroadcastSrcReleaseStreamCfm*)msg;

            capClientHandleBroadcastSrcRemoveStreamCfm(inst, cfm);
        }
        break;

        case BAP_BROADCAST_SRC_UPDATE_METADATA_CFM:
        {
            BapBroadcastSrcUpdateMetadataStreamCfm* cfm =
                             (BapBroadcastSrcUpdateMetadataStreamCfm*)msg;

            capClientHandleBroadcastSrcUpdateStreamCfm(inst, cfm);
        }
        break;

        case BAP_CLIENT_SETUP_DATA_PATH_CFM:
        {
            BapSetupDataPathCfm* cfm =
                             (BapSetupDataPathCfm*)msg;
            capClientBroadcastHandleSetupDataPathCfm(inst, cfm);
        }
        break;

        default:
        {
            CAP_CLIENT_INFO("capClientHandleBroadcastSourceMsg: Invalid prim: %d", *prim);
        }
        break;

    }

}

void CapClientBcastSrcSetAdvParamsReq(uint32 bcastSrcProfileHandle,
                                   const CapClientBcastSrcAdvParams *srcAdvPaParams)
{
    BapBroadcastSrcSetAdvParams(bcastSrcProfileHandle,srcAdvPaParams);
}

/***************************************************Set Param*************************************************************/

void capClientBcastSrcSetConfigParam(CAP_INST* inst, CapClientInternalSetParamReq* req)
{
    BroadcastSrcInst* bcastSrc = NULL;
 
    CAP_CLIENT_INFO("(CAP) capClientBcastSrcSetConfigParam \n");

    if (inst == NULL)
    {
        /* Do Nothing */
        CAP_CLIENT_ERROR("(CAP) Null Instance \n");
        capClientSendSetParamCfm(req->profileTask, CAP_CLIENT_RESULT_NULL_INSTANCE, req->profileHandle);
        return;
    }

    bcastSrc = (BroadcastSrcInst*)
        CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, req->profileHandle);

    if (bcastSrc == NULL || req->numOfParamElems < 1)
    {
        capClientSendSetParamCfm(req->profileTask, CAP_CLIENT_RESULT_INVALID_PARAMETER, req->profileHandle);
        CAP_CLIENT_ERROR("(CAP)handleBroadcastSrcUpdateStreamReq: Null BcastSrc Instance \n");
        return;
    }

    /* Free old Params*/
    if (bcastSrc->bcastParam)
    {
        CsrPmemFree(bcastSrc->bcastParam);
        bcastSrc->bcastParam = NULL;
    }

    bcastSrc->numBcastParam = req->numOfParamElems;
    bcastSrc->bcastParam = (CapClientBcastConfigParam*)req->paramElems;

    capClientSendSetParamCfm(req->profileTask, CAP_CLIENT_RESULT_SUCCESS, req->profileHandle);
}



#endif /* INSTALL_LEA_BROADCAST_SOURCE */
