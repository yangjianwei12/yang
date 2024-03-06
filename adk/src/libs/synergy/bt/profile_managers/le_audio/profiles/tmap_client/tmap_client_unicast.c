/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #60 $
******************************************************************************/

#include "tmap_client_private.h"
#include "tmap_client_debug.h"

extern TmapClientMainInst *tmapClientMain;

#ifdef INSTALL_LEA_UNICAST_CLIENT
void TmapClientRegisterTaskReq(ServiceHandle groupId,
                               TmapClientProfileHandle profileHandle)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientRegisterTaskReq(tmapClientInst->libTask, groupId);
}
#endif

void TmapClientSetParamsReq(TmapClientProfileHandle profileHandle,
                            ServiceHandle groupId,
                            TmapClientStreamCapability sinkConfig,
                            TmapClientStreamCapability srcConfig,
                            TmapClientParamsType type,
                            uint8 numOfParamsElems,
                            const void* paramsElems)
{
    TMAP *tmapClientInst = NULL;

        if (type == TMAP_CLIENT_PARAMS_TYPE_UNICAST_CONNECT ||
            type == TMAP_CLIENT_PARAMS_TYPE_UNICAST_CONNECT_V1)
        {
            tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);
        }
        else if (type == TMAP_CLIENT_PARAMS_TYPE_BROADCAST_CONFIG)
        {
            TmapClientProfileHandleListElm* elem = FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(tmapClientMain->profileHandleList, profileHandle);
            if (elem != NULL)
            {
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);
            }
        }

        if (tmapClientInst != NULL)
        {
            CapClientSetParamReq(tmapClientInst->libTask,
                                 groupId,
                                 sinkConfig,
                                 srcConfig,
                                 type,
                                 numOfParamsElems,
                                 paramsElems
    			                 );
        }
        else
        {
            TMAP_CLIENT_PANIC("TmapClientSetParamsReq:Tmap instance is NULL");
        }

}

#ifdef INSTALL_LEA_UNICAST_CLIENT
void TmapClientUnicastConnectReq(TmapClientProfileHandle profileHandle,
                                 ServiceHandle groupId,
                                 TmapClientStreamCapability sinkConfig,
                                 TmapClientStreamCapability srcConfig,
                                 TmapClientContext useCase,
                                 uint32 sinkAudioLocations,
                                 uint32 srcAudioLocations,
                                 uint8 numOfMic,
                                 TmapClientCigConfigMode cigConfigMode,
                                 const TmapClientQhsConfig* cigConfig)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);
    uint8_t targetLatency = 0;
    bool status = TRUE;

    if ( useCase == TMAP_CLIENT_CONTEXT_TYPE_MEDIA )
    {
        targetLatency = CAP_CLIENT_TARGET_HIGH_RELIABILITY;
    }
    else if ( useCase == TMAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL)
    {
        targetLatency = CAP_CLIENT_TARGET_BALANCE_LATENCY_AND_RELIABILITY;
    }
    else
        status = FALSE;

    if ( status != TRUE )
    {
            TmapClientUnicastConnectCfm* cfm = CsrPmemZalloc(sizeof(*cfm));

            cfm->type = TMAP_CLIENT_UNICAST_CONNECT_CFM;
            cfm->groupId = groupId;
            cfm->context = useCase;
            cfm->result = CAP_CLIENT_RESULT_INVALID_PARAMETER;
            cfm->numOfMicsConfigured = 0;
            cfm->deviceStatusLen = 0;
            cfm->deviceStatus = NULL;

            TmapClientMessageSend(tmapClientInst->appTask, cfm); 
    }
    else
    {
        CapClientUnicastConnectReq(tmapClientInst->libTask,
                                   groupId,
                                   (CapClientSreamCapability)sinkConfig,
                                   (CapClientSreamCapability)srcConfig,                        
                                   targetLatency,
                                   (CapClientContext)useCase,
                                   sinkAudioLocations,
                                   srcAudioLocations,
                                   numOfMic,
                                   cigConfigMode,
                                   cigConfig);
    }
}

void TmapClientUnicastStartStreamReq(TmapClientProfileHandle profileHandle,
                                     ServiceHandle groupId,
                                     TmapClientContext useCase,
                                     uint16 ccId,
                                     uint8 metadataLen,
                                     const uint8* metadata)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);
    uint8* data = NULL;
	uint8 length = metadataLen;

    if (ccId != 0x00)
        length += TMAP_CLIENT_CCID_LTV_LEN;

    data = (uint8*)CsrPmemZalloc(length);

    if ((length != 0x00) && (data == NULL))
    {
        TMAP_CLIENT_PANIC("TmapClientUnicastUpdateAudioReq:Unable to allocate");
        return;
    }

    if (ccId != 0x00)
    {
        /* LTV structure for CCID */
        data[0] = TMAP_CLIENT_CCID_LIST_LENGTH;
        data[1] = TMAP_CLIENT_CCID_LIST_TYPE;
        data[2] = (uint8)(ccId);
    }

	if (metadataLen && metadata)
    {
        if (length > metadataLen) /* ccId is not zero */
            CsrMemCpy((data + TMAP_CLIENT_CCID_LTV_LEN), metadata, metadataLen);
        else
            CsrMemCpy(data, metadata, metadataLen);
    }

    CapClientUnicastStartStreamReq(tmapClientInst->libTask,
                                   groupId,
                                   (CapClientContext)useCase,
                                   length,
                                   data);

    if (data != NULL)
        CsrPmemFree(data);
}

void TmapClientUnicastUpdateAudioReq(TmapClientProfileHandle profileHandle,
                                     ServiceHandle groupId,
                                     TmapClientContext useCase,
                                     uint16 ccId,
                                     uint8 metadataLen,
                                     const uint8* metadata)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);
    uint8* data = NULL;
    uint8 length = metadataLen;

    if (ccId != 0x00)
        length += TMAP_CLIENT_CCID_LTV_LEN;

    data = (uint8*)CsrPmemZalloc(length);

    if ((length != 0x00) && (data == NULL))
    {
        TMAP_CLIENT_PANIC("TmapClientUnicastUpdateAudioReq:Unable to allocate");
        return;
    }

    if (ccId != 0x00)
    {
        /* LTV structure for CCID */
        data[0] = TMAP_CLIENT_CCID_LIST_LENGTH;
        data[1] = TMAP_CLIENT_CCID_LIST_TYPE;
        data[2] = (uint8)(ccId);
    }

	if (metadataLen && metadata)
    {
        if (length > metadataLen) /* ccId is not zero */
            CsrMemCpy((data + TMAP_CLIENT_CCID_LTV_LEN), metadata, metadataLen);
        else
            CsrMemCpy(data, metadata, metadataLen);
    }

    CapClientUnicastUpdateAudioReq(tmapClientInst->libTask,
                                   groupId,
                                   (CapClientContext)useCase,
                                   length,
                                   data);

    if (data != NULL)
        CsrPmemFree(data);
}

void TmapClientUnicastStopStreamReq(TmapClientProfileHandle profileHandle,
                                    ServiceHandle groupId,
                                    bool doRelease)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientUnicastStopStreamReq(tmapClientInst->libTask,
                                  groupId,
                                  doRelease);
}

void TmapClientUnicastDisconnectReq(TmapClientProfileHandle profileHandle,
                                    ServiceHandle groupId,
                                    TmapClientContext useCase)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientUnicastDisConnectReq(tmapClientInst->libTask,
                                  groupId,
                                  (CapClientContext)useCase);
}

void TmapClientMuteReq(TmapClientProfileHandle profileHandle,
                       ServiceHandle groupId,
                       bool mute)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientMuteReq(tmapClientInst->libTask,
                     groupId,
                     mute);
}

void TmapClientSetAbsVolumeReq(TmapClientProfileHandle profileHandle,
                               ServiceHandle groupId,
                               uint8 volumeSetting)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientChangeVolumeReq(tmapClientInst->libTask,
                             groupId,
                             volumeSetting);
}

TmapClientGroupInfo *TmapClientGetStreamInfo(ServiceHandle groupId, uint8 id, TmapClientIsoGroupType flag)
{
    TmapClientGroupInfo *gInfo = NULL;

    gInfo = (TmapClientGroupInfo *)	CapClientGetStreamInfo(groupId, id, flag);

    return gInfo;
}

void TmapClientDeRegisterTaskReq(TmapClientProfileHandle profileHandle,
                                 ServiceHandle groupId)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientDeRegisterTaskReq(tmapClientInst->libTask, groupId);
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */
