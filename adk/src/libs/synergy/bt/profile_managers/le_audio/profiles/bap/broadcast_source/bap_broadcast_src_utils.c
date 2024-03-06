/*******************************************************************************

Copyright (C) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include <string.h>
#include <stdio.h>
#include "bap_client_lib.h"
#include "tbdaddr.h"
#include "csr_bt_profiles.h"
#include "csr_bt_tasks.h"
#include "../bap_utils.h"
#include "../bap_client_debug.h"
#include "../bap_client_list_util_private.h"

#ifdef INSTALL_LEA_BROADCAST_SOURCE
#include "bap_broadcast_src_utils.h"

uint8 bapBroadcastSrcGetCodecId(uint8 *codecIdInfo,
                                  BapCodecIdInfo *codecId)
{
    uint8 index =0;

    codecIdInfo[index++] = codecId->codecId;
    codecIdInfo[index++] = (uint8)(codecId->companyId & 0xff);
    codecIdInfo[index++] = (uint8)((codecId->companyId >> 8) & 0xff);
    codecIdInfo[index++] = (uint8)(codecId->vendorCodecId & 0xff);
    codecIdInfo[index++] = (uint8)((codecId->vendorCodecId >> 8) & 0xff);

    return index;
}

uint8 bapBroadcastSrcGetCodecConfigParam(uint8 *codecConfig,
                                           BapCodecConfiguration *codecParam)
{
    uint8 index =0;

    if(codecParam->samplingFrequency != BAP_SAMPLING_FREQUENCY_RFU)
    {
            codecConfig[index++] = SAMPLING_FREQUENCY_LENGTH;
            codecConfig[index++] = SAMPLING_FREQUENCY_TYPE;
        codecConfig[index++] = bapMapPacsSamplingFreqToAscsValue(codecParam->samplingFrequency);
    }

    if(codecParam->frameDuaration != BAP_SUPPORTED_FRAME_DURATION_NONE)
    {
        codecConfig[index++] = FRAME_DURATION_LENGTH;
        codecConfig[index++] = FRAME_DURATION_TYPE;
        if(codecParam->frameDuaration == BAP_SUPPORTED_FRAME_DURATION_10MS)
        {
            codecConfig[index++] = 0x01;
        }
        else
        {
            codecConfig[index++] = 0x00;
        }
    }

    if(codecParam->audioChannelAllocation != BAP_AUDIO_LOCATION_RFU)
    {
        codecConfig[index++] = AUDIO_CHANNEL_ALLOCATION_LENGTH;
        codecConfig[index++] = AUDIO_CHANNEL_ALLOCATION_TYPE;
        codecConfig[index++] = (uint8)(codecParam->audioChannelAllocation & 0x000000FF);
        codecConfig[index++] = (uint8)((codecParam->audioChannelAllocation & 0x0000ff00) >> 8);
        codecConfig[index++] = (uint8)((codecParam->audioChannelAllocation & 0x00ff0000) >> 16);
        codecConfig[index++] = (uint8)((codecParam->audioChannelAllocation & 0xff000000u) >> 24);
    }

    if(codecParam->octetsPerFrame != 0)
    {
        codecConfig[index++] = OCTETS_PER_CODEC_FRAME_LENGTH;
        codecConfig[index++] = OCTETS_PER_CODEC_FRAME_TYPE;
        codecConfig[index++] = (uint8)(codecParam->octetsPerFrame & 0x00FF);
        codecConfig[index++] = (uint8)((codecParam->octetsPerFrame & 0xff00) >> 8);
    }

    if(codecParam->lc3BlocksPerSdu != 0)
    {
        codecConfig[index++] = LC3_BLOCKS_PER_SDU_LENGTH;
        codecConfig[index++] = LC3_BLOCKS_PER_SDU_TYPE;
        codecConfig[index++] = (uint8)(codecParam->lc3BlocksPerSdu & 0x00FF);
    }

    return index;
}

uint8 bapBroadcastSrcGetMetadata(uint8 *metadataInfo,
                                   BapMetadata *metadata)
{
    uint8 index =0;

    if(metadata->streamingAudioContext != BAP_CONTEXT_TYPE_UNKNOWN)
    {
        metadataInfo[index++] = STREAMING_AUDIO_CONTEXT_LENGTH;
        metadataInfo[index++] = STREAMING_AUDIO_CONTEXT_TYPE;
        metadataInfo[index++] = (uint8)(metadata->streamingAudioContext & 0x00ff);
        metadataInfo[index++] = (uint8)((metadata->streamingAudioContext & 0xff00) >> 8);
    }

    if(metadata->metadataLen)
    {
        memcpy(&metadataInfo[index], metadata->metadata, metadata->metadataLen);
        index += metadata->metadataLen;
    }

    return index;
}

/* BAP Broadcast Downstream primitives */

void BapBroadcastSrcConfigureStreamReq(BapProfileHandle handle,
                                       uint8 bigId,
                                       uint8 ownAddrType,
                                       uint32 presentationDelay,
                                       uint8 numSubgroup,
                                       const BapBigSubgroups *subgroupInfo,
                                       const BapBroadcastInfo *broadcastInfo,
                                       uint8 bigNameLen,
                                       char* bigName)
{
    BapInternalBroadcastSrcConfigureStreamReq *pPrim = CsrPmemZalloc(sizeof(BapInternalBroadcastSrcConfigureStreamReq));
    uint8 i = 0;

    pPrim->type = BAP_INTERNAL_BROADCAST_SRC_CONFIGURE_STREAM_REQ;
    pPrim->handle = handle;
    pPrim->bigId = bigId;
    pPrim->ownAddrType = ownAddrType;
    pPrim->presentationDelay = presentationDelay;
    pPrim->numSubgroup = numSubgroup;
    pPrim->bigNameLen = bigNameLen;

    if(bigNameLen && bigName)
    {
        pPrim->bigName = CsrPmemZalloc(bigNameLen * sizeof(char));

        BAP_CLIENT_DEBUG("BapBroadcastSrcConfigureStreamReq  bigNameLen %d\n", bigNameLen);
        for(i = 0; i < bigNameLen ; i++)
            pPrim->bigName[i] = bigName[i];
	}

    if(numSubgroup && subgroupInfo)
    {
        pPrim->subgroupInfo = CsrPmemZalloc(numSubgroup *
                                               sizeof(BapBigSubgroup));

        BAP_CLIENT_DEBUG("BapBroadcastSrcConfigureStreamReq  num subgroup %d\n", numSubgroup);

        for(i = 0; i < numSubgroup ; i++)
        {
            pPrim->subgroupInfo[i].numBis = subgroupInfo[i].numBis;
            BAP_CLIENT_DEBUG("BapBroadcastSrcConfigureStreamReq  numBis %d\n", pPrim->subgroupInfo[i].numBis);

            pPrim->subgroupInfo[i].codecId.codecId = subgroupInfo[i].codecId;
            pPrim->subgroupInfo[i].codecId.companyId = subgroupInfo[i].companyId;
            pPrim->subgroupInfo[i].codecId.vendorCodecId = subgroupInfo[i].vendorCodecId;

            pPrim->subgroupInfo[i].codecConfigurations.samplingFrequency = subgroupInfo[i].samplingFrequency;
            pPrim->subgroupInfo[i].codecConfigurations.frameDuaration = subgroupInfo[i].frameDuaration;
            pPrim->subgroupInfo[i].codecConfigurations.audioChannelAllocation = subgroupInfo[i].audioChannelAllocation;
            pPrim->subgroupInfo[i].codecConfigurations.octetsPerFrame = subgroupInfo[i].octetsPerFrame;
            pPrim->subgroupInfo[i].codecConfigurations.lc3BlocksPerSdu = subgroupInfo[i].lc3BlocksPerSdu;

            pPrim->subgroupInfo[i].metadata.streamingAudioContext = subgroupInfo[i].streamingAudioContext;

            pPrim->subgroupInfo[i].metadata.metadataLen = subgroupInfo[i].metadataLen;
            pPrim->subgroupInfo[i].metadata.metadata = NULL;

            if(subgroupInfo[i].metadataLen && subgroupInfo[i].metadata)
            {
                pPrim->subgroupInfo[i].metadata.metadata = CsrPmemZalloc((subgroupInfo[i].metadataLen*sizeof(uint8)));
                memcpy(pPrim->subgroupInfo[i].metadata.metadata,
                    subgroupInfo[i].metadata, (subgroupInfo[i].metadataLen*sizeof(uint8)));
            }

            pPrim->subgroupInfo[i].bisInfo = CsrPmemZalloc(subgroupInfo[i].numBis *sizeof(BapBis));

            if(subgroupInfo[i].numBis)
            {
                uint8 j = 0;

                for(j = 0; j < subgroupInfo[i].numBis ; j++)
                {
                    pPrim->subgroupInfo[i].bisInfo[j].bisIndex = subgroupInfo[i].bisInfo[j].bisIndex;
                    pPrim->subgroupInfo[i].bisInfo[j].bisHandle = 0;

                    memcpy(&pPrim->subgroupInfo[i].bisInfo[j].codecConfigurations, &subgroupInfo[i].bisInfo[j].codecConfigurations,
                                            sizeof(BapCodecConfiguration));
                }
            }
        }
    }

    if (broadcastInfo)
    {
        pPrim->broadcastInfo = CsrPmemZalloc(sizeof(BapBroadcastInfo));

        pPrim->broadcastInfo->broadcast = broadcastInfo->broadcast;
        pPrim->broadcastInfo->flags = broadcastInfo->flags;
        pPrim->broadcastInfo->appearanceValue = broadcastInfo->appearanceValue;
    }

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}


void BapBroadcastSrcEnableStreamReq(BapProfileHandle handle,
                                    uint8 bigId, 
                                    const BapBigConfigParam *bigConfigParameters,
                                    uint8 numBis, 
                                    bool encryption,
                                    const uint8 *broadcastCode)
{
    BapInternalBroadcastSrcEnableStreamReq *pPrim = CsrPmemZalloc(sizeof(BapInternalBroadcastSrcEnableStreamReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_REQ;
    pPrim->handle = handle;
    pPrim->bigId = bigId;
    pPrim->numBis = numBis;
    pPrim->encryption = encryption;
    pPrim->broadcastCode = NULL;

    if(bigConfigParameters)
    {
        pPrim->bigConfigParameters = CsrPmemZalloc(sizeof(BapBigConfigParam));
        memcpy(pPrim->bigConfigParameters, bigConfigParameters, sizeof(BapBigConfigParam));
    }

    if(encryption && broadcastCode)
    {
        pPrim->broadcastCode = CsrPmemZalloc(BAP_BROADCAST_CODE_SIZE);
        if(pPrim->broadcastCode)
        {
            memcpy(pPrim->broadcastCode, broadcastCode, BAP_BROADCAST_CODE_SIZE);
        }
    }

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapBroadcastSrcEnableStreamTestReq(BapProfileHandle handle,
                                        uint8 bigId, 
                                        const BapBigTestConfigParam *bigTestConfigParameters,
                                        uint8 numBis, 
                                        bool encryption,
                                        const uint8 *broadcastCode)
{
    BapInternalBroadcastSrcEnableStreamTestReq *pPrim = CsrPmemZalloc(sizeof(BapInternalBroadcastSrcEnableStreamTestReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_TEST_REQ;
    pPrim->handle = handle;
    pPrim->bigId = bigId;
    pPrim->numBis = numBis;
    pPrim->encryption = encryption;
    pPrim->broadcastCode = NULL;

    if(bigTestConfigParameters)
    {
        pPrim->bigTestConfigParameters = CsrPmemZalloc(sizeof(BapBigTestConfigParam));
        memcpy(pPrim->bigTestConfigParameters, bigTestConfigParameters, sizeof(BapBigTestConfigParam));
    }

    if(encryption && broadcastCode)
    {
        pPrim->broadcastCode = CsrPmemZalloc(BAP_BROADCAST_CODE_SIZE);
        if(pPrim->broadcastCode)
        {
            memcpy(pPrim->broadcastCode, broadcastCode, BAP_BROADCAST_CODE_SIZE);
        }
    }

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapBroadcastSrcDisableStreamReq(BapProfileHandle handle, uint8 bigId)
{
    BapInternalBroadcastSrcDisableStreamReq *pPrim = CsrPmemZalloc(sizeof(BapInternalBroadcastSrcDisableStreamReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_SRC_DISABLE_STREAM_REQ;
    pPrim->handle = handle;
    pPrim->bigId = bigId;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapBroadcastSrcReleaseStreamReq(BapProfileHandle handle, uint8 bigId)
{
    BapInternalBroadcastSrcReleaseStreamReq *pPrim = CsrPmemZalloc(sizeof(BapInternalBroadcastSrcReleaseStreamReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_SRC_RELEASE_STREAM_REQ;
    pPrim->handle = handle;
    pPrim->bigId = bigId;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapBroadcastSrcUpdateMetadataReq(BapProfileHandle handle,
                                      uint8 bigId, 
                                      uint8 numSubgroup,
                                      const BapMetadata  *subgroupMetadata)
{
    BapInternalBroadcastSrcUpdateMetadataStreamReq *pPrim = CsrPmemZalloc(sizeof(BapInternalBroadcastSrcUpdateMetadataStreamReq));

    pPrim->type = BAP_INTERNAL_BROADCAST_SRC_UPDATE_METADATA_REQ;
    pPrim->handle = handle;
    pPrim->bigId = bigId;
    pPrim->numSubgroup = numSubgroup;
    pPrim->subgroupMetadata = NULL;

    if(numSubgroup && subgroupMetadata)
    {
        uint8 i = 0;
        pPrim->subgroupMetadata = CsrPmemZalloc(numSubgroup*sizeof(BapMetadata));

        for(i = 0; i<numSubgroup; i++)
        {
            memcpy(&pPrim->subgroupMetadata[i], &subgroupMetadata[i], sizeof(BapMetadata));
            pPrim->subgroupMetadata[i].metadata = NULL;
            
            if(subgroupMetadata[i].metadataLen && subgroupMetadata[i].metadata)
            {
                pPrim->subgroupMetadata[i].metadata = CsrPmemZalloc(subgroupMetadata[i].metadataLen*sizeof(uint8));
                memcpy(pPrim->subgroupMetadata[i].metadata,
                subgroupMetadata[i].metadata, (subgroupMetadata[i].metadataLen*sizeof(uint8)));
            }
        }
    }
    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}


/* BAP Broadcast Upstream primitives */

void bapBroadcastSrcConfigureStreamCfmSend(phandle_t phandle, 
                                           BapProfileHandle handle,
                                           BapResult result)
{
    BapBroadcastSrcConfigureStreamCfm* cfmPrim = CsrPmemZalloc(sizeof(BapBroadcastSrcConfigureStreamCfm));

    cfmPrim->type = BAP_BROADCAST_SRC_CONFIGURE_STREAM_CFM;
    cfmPrim->handle = handle;
    cfmPrim->result = result;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapBroadcastSrcEnableStreamCfmSend(phandle_t phandle,
                                        BapProfileHandle handle,
                                        BapResult result,
                                        uint8 bigId,
                                        uint32 bigSyncDelay,
                                        BapBigParam *bigParameters,
                                        uint8 numBis,
                                        uint16 *bisHandles)
{
    BapBroadcastSrcEnableStreamCfm* cfmPrim = CsrPmemZalloc(sizeof(BapBroadcastSrcEnableStreamCfm));

    cfmPrim->type = BAP_BROADCAST_SRC_ENABLE_STREAM_CFM;
    cfmPrim->handle = handle;
    cfmPrim->result = result;
    cfmPrim->bigId = bigId;
    cfmPrim->bigSyncDelay = bigSyncDelay;
    cfmPrim->numBis = numBis;
    if(bigParameters)
    {
        memcpy(&cfmPrim->bigParameters, bigParameters, sizeof(BapBigParam));
    }

    if(numBis && bisHandles)
    {
        cfmPrim->bisHandles = CsrPmemZalloc(numBis*sizeof(uint16));
        memcpy(cfmPrim->bisHandles, bisHandles, (numBis*sizeof(uint16)));

        CsrPmemFree(bisHandles);
        bisHandles = NULL;
    }

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);

}

void bapBroadcastSrcEnableStreamTestCfmSend(phandle_t phandle,
                                            BapProfileHandle handle,
                                            BapResult result,
                                            uint8 bigId,
                                            uint32 bigSyncDelay,
                                            BapBigParam *bigParameters,
                                            uint8 numBis,
                                            uint16 *bisHandles)
{
    BapBroadcastSrcEnableStreamTestCfm* cfmPrim = CsrPmemZalloc(sizeof(BapBroadcastSrcEnableStreamTestCfm));

    cfmPrim->type = BAP_BROADCAST_SRC_ENABLE_STREAM_TEST_CFM;
    cfmPrim->handle = handle;
    cfmPrim->result = result;
    cfmPrim->bigId = bigId;
    cfmPrim->bigSyncDelay = bigSyncDelay;
    cfmPrim->numBis = numBis;

    if(bigParameters)
    {
        memcpy(&cfmPrim->bigParameters, bigParameters, sizeof(BapBigParam));
    }

    if(numBis && bisHandles)
    {
        cfmPrim->bisHandles = CsrPmemZalloc(numBis*sizeof(uint16));
        memcpy(cfmPrim->bisHandles, bisHandles, (numBis*sizeof(uint16)));
    }

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapBroadcastSrcDisableStreamCfmSend(phandle_t phandle,
                                         BapProfileHandle handle,
                                         BapResult result,
                                         uint8 bigId)
{
    BapBroadcastSrcDisableStreamCfm* cfmPrim = CsrPmemZalloc(sizeof(BapBroadcastSrcDisableStreamCfm));

    cfmPrim->type = BAP_BROADCAST_SRC_DISABLE_STREAM_CFM;
    cfmPrim->handle = handle;
    cfmPrim->result = result;
    cfmPrim->bigId = bigId;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}

void bapBroadcastSrcReleaseStreamCfmSend(phandle_t phandle,
                                         BapProfileHandle handle,
                                         BapResult result,
                                         uint8 bigId)
{
    BapBroadcastSrcReleaseStreamCfm* cfmPrim = CsrPmemZalloc(sizeof(BapBroadcastSrcReleaseStreamCfm));

    cfmPrim->type = BAP_BROADCAST_SRC_RELEASE_STREAM_CFM;
    cfmPrim->handle = handle;
    cfmPrim->result = result;
    cfmPrim->bigId = bigId;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}


void bapBroadcastSrcUpdateMetadataCfmSend(phandle_t phandle,
                                          BapProfileHandle handle,
                                          BapResult result,
                                          uint8 bigId)
{
    BapBroadcastSrcUpdateMetadataStreamCfm* cfmPrim = CsrPmemZalloc(sizeof(BapBroadcastSrcUpdateMetadataStreamCfm));

    cfmPrim->type = BAP_BROADCAST_SRC_UPDATE_METADATA_CFM;
    cfmPrim->handle = handle;
    cfmPrim->result = result;
    cfmPrim->bigId = bigId;

    putMessageSynergy(PHANDLE_TO_QUEUEID(phandle), BAP_PRIM, cfmPrim);
}
#endif /* INSTALL_LEA_BROADCAST_SOURCE */
