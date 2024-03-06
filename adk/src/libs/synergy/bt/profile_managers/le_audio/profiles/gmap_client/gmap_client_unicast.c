/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/

#include "gmap_client_private.h"
#include "gmap_client_debug.h"

extern GmapClientMainInst *gmapClientMain;

#ifdef INSTALL_LEA_UNICAST_CLIENT
void GmapClientRegisterToCapReq(ServiceHandle groupId,
                                GmapClientProfileHandle profileHandle)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientRegisterTaskReq(gmapClientInst->libTask, groupId);
}



void GmapClientUnicastConnectReq(GmapClientProfileHandle profileHandle,
                                 ServiceHandle groupId,
                                 GmapClientStreamCapability sinkConfig,
                                 GmapClientStreamCapability srcConfig,
                                 GmapClientTargetLatency targetLatency,
                                 uint32 sinkAudioLocations,
                                 uint32 srcAudioLocations,
                                 uint8 numOfMic,
                                 GmapClientCigConfigMode cigConfigMode,
                                 const GmapClientQhsConfig* cigConfig)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientUnicastConnectReq(gmapClientInst->libTask,
                               groupId,
                               (CapClientSreamCapability)sinkConfig,
                               (CapClientSreamCapability)srcConfig,                        
                               targetLatency,
                               (CapClientContext)GMAP_CLIENT_CONTEXT_TYPE_GAME,
                               sinkAudioLocations,
                               srcAudioLocations,
                               numOfMic,
                               cigConfigMode,
                               cigConfig);
}

void GmapClientUnicastStartStreamReq(GmapClientProfileHandle profileHandle,
                                     ServiceHandle groupId,
                                     uint8 metadataLen,
                                     const uint8* metadata)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientUnicastStartStreamReq(gmapClientInst->libTask,
                                   groupId,
                                   (CapClientContext)GMAP_CLIENT_CONTEXT_TYPE_GAME,
                                   metadataLen,
                                   (uint8*)metadata);
}

void GmapClientUnicastUpdateAudioReq(GmapClientProfileHandle profileHandle,
                                     ServiceHandle groupId,
                                     uint8 metadataLen,
                                     const uint8* metadata)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientUnicastUpdateAudioReq(gmapClientInst->libTask,
                                   groupId,
                                   (CapClientContext)GMAP_CLIENT_CONTEXT_TYPE_GAME,
                                   metadataLen,
                                   (uint8*)metadata);
}

void GmapClientUnicastStopStreamReq(GmapClientProfileHandle profileHandle,
                                    ServiceHandle groupId,
                                    bool doRelease)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientUnicastStopStreamReq(gmapClientInst->libTask,
                                  groupId,
                                  doRelease);
}

void GmapClientUnicastDisconnectReq(GmapClientProfileHandle profileHandle,
                                    ServiceHandle groupId)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientUnicastDisConnectReq(gmapClientInst->libTask,
                                  groupId,
                                  (CapClientContext)GMAP_CLIENT_CONTEXT_TYPE_GAME);
}

void GmapClientMuteReq(GmapClientProfileHandle profileHandle,
                       ServiceHandle groupId,
                       bool mute)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientMuteReq(gmapClientInst->libTask,
                     groupId,
                     mute);
}

void GmapClientSetAbsVolumeReq(GmapClientProfileHandle profileHandle,
                               ServiceHandle groupId,
                               uint8 volumeSetting)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientChangeVolumeReq(gmapClientInst->libTask,
                             groupId,
                             volumeSetting);
}

void GmapClientDeRegisterTaskReq(GmapClientProfileHandle profileHandle,
                                 ServiceHandle groupId)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    CapClientDeRegisterTaskReq(gmapClientInst->libTask, groupId);
}
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

void GmapClientSetParamsReq(GmapClientProfileHandle profileHandle,
                            ServiceHandle groupId,
                            GmapClientStreamCapability sinkConfig,
                            GmapClientStreamCapability srcConfig,
                            GmapClientParamsType type,
                            uint8 numOfParamsElems,
                            const void* paramsElems)
{
    GMAP *gmapClientInst = NULL;

        if (type == GMAP_CLIENT_PARAMS_TYPE_UNICAST_CONNECT)
        {
            gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);
        }
        else if (type == GMAP_CLIENT_PARAMS_TYPE_BROADCAST_CONFIG)
        {
            GmapClientProfileHandleListElm* elem = FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(gmapClientMain->profileHandleList, profileHandle);
            if (elem != NULL)
            {
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);
            }
        }

        if (gmapClientInst != NULL)
        {
            CapClientSetParamReq(gmapClientInst->libTask,
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
            GMAP_CLIENT_PANIC("GmapClientSetParamsReq:Gmap instance is NULL");
        }

}

