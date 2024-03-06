/******************************************************************************
 Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/

#include "pbp_private.h"
#include "pbp_debug.h"

#ifdef INSTALL_LEA_BROADCAST_SOURCE
#define IS_NON_PUBLIC_BROADCAST_TYPE(x) (x == NON_PUBLIC_BROADCAST)

#define CHECK_MANDATORY_STANDARD_BROADCAST_STREAM_CONFIGURATION(x) (((x & CAP_CLIENT_STREAM_CAPABILITY_16_2) == CAP_CLIENT_STREAM_CAPABILITY_16_2) || \
                                                                    ((x & CAP_CLIENT_STREAM_CAPABILITY_24_2) == CAP_CLIENT_STREAM_CAPABILITY_24_2))
#define CHECK_MANDATORY_HQ_BROADCAST_STREAM_CONFIGURATION(x) (((x & CAP_CLIENT_STREAM_CAPABILITY_48_1) == CAP_CLIENT_STREAM_CAPABILITY_48_1) || \
                                                              ((x & CAP_CLIENT_STREAM_CAPABILITY_48_2) == CAP_CLIENT_STREAM_CAPABILITY_48_2) || \
                                                              ((x & CAP_CLIENT_STREAM_CAPABILITY_48_3) == CAP_CLIENT_STREAM_CAPABILITY_48_3) || \
                                                              ((x & CAP_CLIENT_STREAM_CAPABILITY_48_4) == CAP_CLIENT_STREAM_CAPABILITY_48_4) || \
                                                              ((x & CAP_CLIENT_STREAM_CAPABILITY_48_5) == CAP_CLIENT_STREAM_CAPABILITY_48_5) || \
                                                              ((x & CAP_CLIENT_STREAM_CAPABILITY_48_6) == CAP_CLIENT_STREAM_CAPABILITY_48_6))

#define MIN_NUM_SUBGROUPS_ALL_PBP_BROADCAST_TYPE_SUPPORTED (2)

#define RETRANSMISSION_NUMBER_MAX_VALUE  (0x1Eu)
#define MAX_SDU_MAX_VALUE                (0x0FFFu)
#define MAX_TRANSPORT_LATENCY_MIN_VALUE  (0x0005u)
#define MAX_TRANSPORT_LATENCY_MAX_VALUE  (0x0FA0u)
#define SDU_INTERVAL_MIN_VALUE           (0x000000FFu)
#define SDU_INTERVAL_MAX_VALUE           (0x000FFFFFu)

extern PbpMainInst* pbpMain;

/******************************************************************************/
static bool pbpCheckBroadcastSourceConfig(const PbpBigSubGroups *subgroupInfo,
                                          uint8_t numSubgroup,
                                          PbpBroadcastInfo broadcastInfo)
{
    uint8 subgroupIndex = 0, bisIndex = 0;
    bool foundSQ = FALSE, foundHQ = FALSE;
    CapClientSreamCapability capabilityConfig = 0;

    if (IS_NON_PUBLIC_BROADCAST_TYPE(broadcastInfo.broadcast))
        return FALSE;

    if (CHECK_ALL_PBP_BROADCAST_TYPE_SUPPORTED(broadcastInfo.broadcast) && numSubgroup < MIN_NUM_SUBGROUPS_ALL_PBP_BROADCAST_TYPE_SUPPORTED)
        return FALSE;

    for (subgroupIndex = 0; subgroupIndex < numSubgroup; subgroupIndex++)
    {
        for (bisIndex = 0; bisIndex < subgroupInfo[subgroupIndex].numBis; bisIndex++)
        {
            capabilityConfig = subgroupInfo[subgroupIndex].bisInfo[bisIndex].config;

            /* In level-3 capability may be 0 and if so consider value in level-2*/
            if (capabilityConfig == CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN)
            {
                capabilityConfig = subgroupInfo[subgroupIndex].config;
            }

            if (CHECK_MANDATORY_STANDARD_BROADCAST_STREAM_CONFIGURATION(capabilityConfig) && !foundSQ)
                foundSQ = TRUE;

            if (CHECK_MANDATORY_HQ_BROADCAST_STREAM_CONFIGURATION(capabilityConfig) && !foundHQ)
                foundHQ = TRUE;

            if (foundSQ && foundHQ)
                break;
        }
    }

    if ((CHECK_ALL_PBP_BROADCAST_TYPE_SUPPORTED(broadcastInfo.broadcast) && foundSQ && foundHQ) ||
        (CHECK_STANDARD_BROADCAST_TYPE(broadcastInfo.broadcast) && foundSQ && !foundHQ) ||
        (CHECK_HQ_BROADCAST_TYPE(broadcastInfo.broadcast) && foundHQ && !foundSQ))
        return TRUE;

    return FALSE;
}

/******************************************************************************/
void PbpBroadcastSrcInitReq(PbpProfileHandle profileHandle)
{
    PBP* pbpInst = NULL;

    if (!profileHandle)
    {
        PBP_PANIC("Profile handle is zero!\n");
    }

    pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHandle);

    if (pbpInst)
    {
        pbpInst->bcastSrcHandle = NO_PROFILE_HANDLE;
        CapClientBcastSrcInitReq(pbpInst->libTask);
    }
    else
        PBP_PANIC("PbpBroadcastSrcInitReq: Invalid pbpInst");
}

/******************************************************************************/
void PbpBroadcastSrcDeinitReq(PbpProfileHandle brcstSrcProfileHandle)
{
    if ((FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(pbpMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        PBP_PANIC("PbpBroadcastSrcDeinitReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcDeinitReq((uint32)brcstSrcProfileHandle);
}

/******************************************************************************/
void PbpBroadcastSrcSetAdvParamsReq(PbpProfileHandle brcstSrcProfileHandle, const PbpBcastSrcAdvParams* srcAdvPaParams)
{
    if ((FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(pbpMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        PBP_PANIC("PbpBroadcastSrcSetAdvParamsReq: Invalid brcstSrcProfileHandle");
    }

    if (!srcAdvPaParams)
    {
        PBP_PANIC("PbpBroadcastSrcSetAdvParamsReq: Invalid srcAdvPaParams");
    }

    CapClientBcastSrcSetAdvParamsReq((uint32)brcstSrcProfileHandle, srcAdvPaParams);
}

/******************************************************************************/
static bool pbpBroadcastSrcIsConfigurationParamsValid(uint8 numOfSubGroups,
                                                      const PbpBcastConfigParam* param)
{
    uint8 subGroupIndex = 0;

    for (subGroupIndex = 0; subGroupIndex < numOfSubGroups; subGroupIndex++)
    {
        if (!(CHECK_MANDATORY_STANDARD_BROADCAST_STREAM_CONFIGURATION(param[subGroupIndex].subGroupConfig) ||
             CHECK_MANDATORY_HQ_BROADCAST_STREAM_CONFIGURATION(param[subGroupIndex].subGroupConfig)))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/******************************************************************************/
void  PbpBroadcastSrcSetParamReq(PbpProfileHandle brcstSrcProfileHandle,
                                 uint8 numOfSubGroups,
                                 const PbpBcastConfigParam* param)
{
    PBP* pbpInst = NULL;
    PbpProfileHandleListElm* elem = NULL;

    elem = FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(pbpMain->profileHandleList, brcstSrcProfileHandle);

    if (elem == NULL)
    {
        PBP_PANIC("PbpBroadcastSrcSetParamReq: Invalid brcstSrcProfileHandle");
    }

    if (!param || !numOfSubGroups)
    {
        PBP_PANIC("PbpBroadcastSrcSetParamReq: NULL parameters!");
    }

    pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(elem->profileHandle);

    if (!pbpInst)
    {
        PBP_PANIC("PbpBroadcastSrcSetParamReq: Invalid PBP instance!");
    }

    if (pbpBroadcastSrcIsConfigurationParamsValid(numOfSubGroups, param))
    {
        CapClientSetParamReq(pbpInst->libTask,
            (uint32)brcstSrcProfileHandle,
            CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN,
            CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN,
            CAP_CLIENT_PARAM_TYPE_BCAST_CONFIG,
            numOfSubGroups,
            (const void*)param);
    }
    else
    {
        MAKE_PBP_MESSAGE(PbpBroadcastSrcSetParamCfm);

        PBP_WARNING("PbpBroadcastSrcSetParamReq: Invalid parameters!");

        message->bcastSrcProfileHandle = brcstSrcProfileHandle;
        message->result = CAP_CLIENT_RESULT_INVALID_PARAMETER;

        PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_SRC_SET_PARAM_CFM, message);
    }
}

void PbpBroadcastSrcSetBroadcastId(PbpProfileHandle brcstSrcProfileHandle, uint32 broadcastId)
{
    PbpProfileHandleListElm* elem = NULL;
    PbpMainInst* inst = pbpGetMainInstance();
    PBP* pbpInst = NULL;

    elem = FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, brcstSrcProfileHandle);

    if (elem)
        pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(elem->profileHandle);

    if (!pbpInst)
    {
        PBP_PANIC("PbpBroadcastSrcSetBroadcastId: NO PBP Instance!\n");
    }

    if ((FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(pbpMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        PBP_PANIC("PbpBroadcastSrcSetBroadcastId: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcSetBroadcastId((uint32)brcstSrcProfileHandle, broadcastId);
}

/******************************************************************************/
void PbpBroadcastSrcConfigReq(PbpProfileHandle brcstSrcProfileHandle,
                              uint8 ownAddrType,
                              uint32 presentationDelay,
                              uint8 numSubGroup,
                              const PbpBigSubGroups* subgroupInfo,
                              PbpBroadcastInfo *broadcastInfo,
                              PbpBigConfigMode mode,
                              const PbpQhsBigConfig* bigConfig)
{
    CapClientResult result = CAP_CLIENT_RESULT_SUCCESS;
    PbpProfileHandleListElm* elem = NULL;
    PbpMainInst* inst = pbpGetMainInstance();
    PBP* pbpInst = NULL;

    elem = FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, brcstSrcProfileHandle);

    if (elem)
        pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(elem->profileHandle);

    if (!pbpInst)
    {
        PBP_PANIC("PbpBroadcastSrcConfigReq: NO PBP Instance!\n");
    }

    if ((FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(pbpMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        PBP_PANIC("PbpBroadcastSrcConfigReq: Invalid brcstSrcProfileHandle");
    }

    if (!subgroupInfo || !broadcastInfo || (((mode & CAP_CLIENT_BIG_CONFIG_MODE_QHS)== CAP_CLIENT_BIG_CONFIG_MODE_QHS) &&  !bigConfig))
    {
        result = CAP_CLIENT_RESULT_INVALID_PARAMETER;
    }

    /* Check if the Broadcast Audio Stream Configuration is a valid one
       for a Public Broadcast Source.
     */
    if (result == CAP_CLIENT_RESULT_SUCCESS && pbpCheckBroadcastSourceConfig(subgroupInfo, numSubGroup, *(broadcastInfo)))
    {
        pbpInst->numSubGroup = numSubGroup;

        /* Check Broadcast Type is a valid one for PBP */
        CapClientBcastSrcConfigReq(brcstSrcProfileHandle,
            ownAddrType,
            presentationDelay,
            numSubGroup,
            subgroupInfo,
            broadcastInfo,
            mode,
            bigConfig);
    }
    else
    {
        MAKE_PBP_MESSAGE(PbpBroadcastSrcConfigCfm);

        message->bcastSrcProfileHandle = 0;
        message->result = CAP_CLIENT_RESULT_INVALID_PARAMETER;

        PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_SRC_CONFIG_CFM, message);
    }
}

/******************************************************************************/
void PbpBroadcastSrcStartStreamReq(PbpProfileHandle brcstSrcProfileHandle,
                                   bool  encryption,
                                   uint8 *broadcastCode)
{
    CapClientResult result = CAP_CLIENT_RESULT_SUCCESS;

    if ((FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(pbpMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        PBP_PANIC("PbpBroadcastSrcStartStreamReq: Invalid brcstSrcProfileHandle");
    }

    if (encryption && !broadcastCode)
    {
        result = CAP_CLIENT_RESULT_INVALID_PARAMETER;
    }

    if (result == CAP_CLIENT_RESULT_SUCCESS)
    {
        CapClientBcastSrcStartStreamReq(brcstSrcProfileHandle,
            encryption,
            broadcastCode);
    }
    else
    {
        MAKE_PBP_MESSAGE(PbpBroadcastSrcConfigCfm);
        PbpProfileHandleListElm* elem = NULL;
        PbpMainInst* inst = pbpGetMainInstance();
        PBP* pbpInst = NULL;

        elem = FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, brcstSrcProfileHandle);

        if (elem)
            pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(elem->profileHandle);

        if (pbpInst)
        {
            message->result = result;
            message->bcastSrcProfileHandle = brcstSrcProfileHandle;

            PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_SRC_START_STREAM_CFM, message);
        }
        else
        {
            PBP_PANIC("PbpBroadcastSrcStartStreamReq: NO PBP Instance!\n");
        }
    }
}

/******************************************************************************/
void PbpBroadcastSrcUpdateAudioReq(PbpProfileHandle brcstSrcProfileHandle,
                                   PbpContext useCase,
                                   uint8_t numSubgroup,
                                   uint8 metadataLen,
                                   uint8* metadata)
{
    CapClientResult result = CAP_CLIENT_RESULT_SUCCESS;
    PbpProfileHandleListElm* elem = NULL;
    PbpMainInst* inst = pbpGetMainInstance();
    PBP* pbpInst = NULL;

    if ((FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(pbpMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
        PBP_PANIC("PbpBroadcastSrcUpdateAudioReq: Invalid brcstSrcProfileHandle");

    elem = FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, brcstSrcProfileHandle);

    if (elem)
        pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(elem->profileHandle);

    if (!pbpInst)
        PBP_PANIC("PbpBroadcastSrcUpdateAudioReq: PBP NULL instance\n");

    if (pbpInst->numSubGroup != numSubgroup)
        result = CAP_CLIENT_RESULT_INVALID_PARAMETER;

    if (metadataLen && !metadata)
        result = CAP_CLIENT_RESULT_INVALID_PARAMETER;

    if (result == CAP_CLIENT_RESULT_SUCCESS)
    {
        CapClientBcastSrcUpdateStreamReq(brcstSrcProfileHandle,
            (CapClientContext)useCase,
            numSubgroup,
            metadataLen,
            metadata);
    }
    else
    {
        MAKE_PBP_MESSAGE(PbpBroadcastSrcUpdateAudioCfm);

        message->result = result;
        message->bcastSrcProfileHandle = brcstSrcProfileHandle;

        PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_SRC_UPDATE_AUDIO_CFM, message);
    }
}

/******************************************************************************/
void PbpBroadcastSrcStopStreamReq(PbpProfileHandle brcstSrcProfileHandle)
{
    if ((FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(pbpMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        PBP_PANIC("PbpBroadcastSrcStopStreamReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcStopStreamReq(brcstSrcProfileHandle);
}

/******************************************************************************/
void PbpBroadcastSrcRemoveStreamReq(PbpProfileHandle brcstSrcProfileHandle)
{
    if ((FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(pbpMain->profileHandleList, brcstSrcProfileHandle)) == NULL)
    {
        PBP_PANIC("PbpBroadcastSrcRemoveStreamReq: Invalid brcstSrcProfileHandle");
    }

    CapClientBcastSrcRemoveStreamReq(brcstSrcProfileHandle);
}
#endif /* #ifdef INSTALL_LEA_BROADCAST_SOURCE */
