/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/

#include "gmap_client_debug.h"
#include "gmap_client_init.h"
#include "gmap_client_destroy.h"
#include "gmap_client_read.h"
#include "gatt_service_discovery_lib.h"

CsrBool gmapClientFindGmapInst(CsrCmnListElm_t *elem, void *data)
{
    CSR_UNUSED(data);
    GmapClientProfileHandleListElm *profileHndlElm = (GmapClientProfileHandleListElm *)elem;
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHndlElm->profileHandle);

    if (gmapClientInst)
        return TRUE;

    return FALSE;
}

CsrBool gmapClientInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    GmapClientProfileHandleListElm *profileHndlElm = (GmapClientProfileHandleListElm *)elem;
    ServiceHandle profileHandle = *(ServiceHandle *)data;

    if (profileHndlElm)
        return (profileHndlElm->profileHandle == profileHandle);

    return FALSE;
}

CsrBool gmapClientProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data)
{
    GmapClientProfileHandleListElm *profileHndlElm = (GmapClientProfileHandleListElm *)elem;
    CsrBtConnId     btConnId   = *(CsrBtConnId *) data;
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHndlElm->profileHandle);
    GmapClientCidListElm *cidListElm = NULL;

    if (gmapClientInst)
    {
        cidListElm = FIND_GMAP_CLIENT_CID_ELEM(gmapClientInst->gmapClientCidList.cidList, btConnId);

        if (cidListElm != NULL)
            return (cidListElm->cid == btConnId);
    }

    return FALSE;
}

CsrBool gmapClientProfileHndlFindByGmasSrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    GmapClientProfileHandleListElm *profileHndlElm = (GmapClientProfileHandleListElm *)elem;
    ServiceHandle gmasSrvcHndl = *(ServiceHandle *)data;
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHndlElm->profileHandle);

    if (gmapClientInst)
        return (gmapClientInst->gmasSrvcHndl == gmasSrvcHndl);

    return FALSE;
}

CsrBool gmapClientElemFindByCid(CsrCmnListElm_t *elem, void *data)
{
    GmapClientCidListElm *cidListElm = (GmapClientCidListElm *)elem;
    uint32 cid = *(uint32 *)data;

    if (cidListElm)
        return (cidListElm->cid == cid);

    return FALSE;
}

CsrBool gmapClientFindGmapInstFromBcastSrcHandle(CsrCmnListElm_t *elem, void *data)
{
    GmapClientProfileHandleListElm *profileHndlElm = (GmapClientProfileHandleListElm *)elem;
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHndlElm->profileHandle);
    GmapClientProfileHandle bcastSrcHandle = *(GmapClientProfileHandle *)data;

    if ((gmapClientInst) && (gmapClientInst->bcastSrcHandle == bcastSrcHandle))
        return TRUE;

    return FALSE;
}
/****************************************************************************/
static void gmapClientHandleGattSrvcDiscMsg(GmapClientMainInst *inst, Msg *msg)
{
    GMAP *gmapClientInst = NULL;
    GmapClientProfileHandleListElm* elem = NULL;
    GattSdPrim* prim = (GattSdPrim*)msg;

    switch (*prim)
    {
        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
        {
            GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *cfm =
                (GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *) msg;

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_BY_BTCONNID(inst->profileHandleList, cfm->cid);
            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst == NULL)
                return;

            if ((cfm->result == GATT_SD_RESULT_SUCCESS) && cfm->srvcInfoCount)
            {
                uint16 index, count;

                count = cfm->srvcInfoCount;

                for (index = 0; index < count; index++)
                {
                    GattGmasClientInitData init_data;
                    GMAP_CLIENT_DEBUG("(GMAP) : Start Hndl = 0x%x, End Hndl = 0x%x, Id = 0x%x\n",
                                     cfm->srvcInfo[index].startHandle, cfm->srvcInfo[index].endHandle, cfm->srvcInfo[index].srvcId);

                    if ( cfm->srvcInfo[index].srvcId == GATT_SD_QGMAS_SRVC)
                    {
                        gmapClientInst->supportedGmasSrvcs |= GMAP_CLIENT_QGMAP_SUPPORTED;
						if(cfm->srvcInfoCount == 1) /* Remote is supporting only QGMAS  */
						{
                            gmapClientSendInitCfm(gmapClientInst, GMAP_CLIENT_STATUS_SUCCESS);
                        }
                    }

                    if ( cfm->srvcInfo[index].srvcId == GATT_SD_GMAS_SRVC)
                    {
                        init_data.cid = cfm->cid;
                        init_data.startHandle = cfm->srvcInfo[index].startHandle;
                        init_data.endHandle = cfm->srvcInfo[index].endHandle;
                        gmapClientInst->supportedGmasSrvcs |= GMAP_CLIENT_GMAP_SUPPORTED;

                        GattGmasClientInitReq(gmapClientInst->libTask, &init_data, NULL);
                    }
                }
                CsrPmemFree(cfm->srvcInfo);
            }
            else if (cfm->result == GATT_SD_RESULT_SRVC_ID_NOT_FOUND)
            {
                gmapClientSendInitCfm(gmapClientInst, GMAP_CLIENT_STATUS_SUCCESS_GMAS_SRVC_NOT_FOUND);
            }
            else
            {
                gmapClientSendInitCfm(gmapClientInst, GMAP_CLIENT_STATUS_DISCOVERY_ERR);
                REMOVE_GMAP_CLIENT_SERVICE_HANDLE(inst->profileHandleList, gmapClientInst->gmapSrvcHndl);
                FREE_GMAP_CLIENT_INST(gmapClientInst->gmapSrvcHndl);
            }
            break;
        }

        default:
        {
            /* Unrecognised GATT Manager message */
            GMAP_CLIENT_WARNING("Gatt SD Msg not handled \n");
        }
        break;
    }
}

/*************************************************************/
static void gmapClientHandleGattGmasClientMsg(GmapClientMainInst *inst, void *msg)
{
    GMAP * gmapClientInst = NULL;
    GmapClientProfileHandleListElm* elem = NULL;
    GattGmasClientMessageId *prim = (GattGmasClientMessageId *)msg;

    GMAP_CLIENT_INFO("gmapClientHandleGattGmasClientMsg MESSAGE:GattGmasClientMessageId:0x%x", *prim);

    switch (*prim)
    {
        case GATT_GMAS_CLIENT_INIT_CFM:
        {
            const GattGmasClientInitCfm* message;
            message = (GattGmasClientInitCfm*) msg;

            /* Find gmap instance using connection_id_t */
            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_BY_BTCONNID(inst->profileHandleList,
                                                               message->cid);
            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                gmapClientHandleGmasClientInitResp(gmapClientInst,
                                                   (const GattGmasClientInitCfm *)msg);
        }
        break;

        case GATT_GMAS_CLIENT_READ_ROLE_CFM:
        {
            const GattGmasClientReadRoleCfm* message;
            message = (GattGmasClientReadRoleCfm*) msg;

            /* Find GMAP instance using connection_id_t */
            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_BY_GMAS_SERVICE_HANDLE(inst->profileHandleList,
                                                                          message->srvcHndl);
            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                gmapClientHandleReadRoleCfm(gmapClientInst,
                                            (const GattGmasClientReadRoleCfm *)msg);
        }
        break;

        case GATT_GMAS_CLIENT_READ_UNICAST_FEATURES_CFM:
        {
            const GattGmasClientReadUnicastFeaturesCfm* message;
            message = (GattGmasClientReadUnicastFeaturesCfm*) msg;

            /* Find GMAP instance using connection_id_t */
            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_BY_GMAS_SERVICE_HANDLE(inst->profileHandleList,
                                                                          message->srvcHndl);
            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                gmapClientHandleReadUnicastFeaturesCfm(gmapClientInst,
                                                      (const GattGmasClientReadUnicastFeaturesCfm *)msg);
        }
        break;

        case GATT_GMAS_CLIENT_READ_BROADCAST_FEATURES_CFM:
        {
            const GattGmasClientReadBroadcastFeaturesCfm* message;
            message = (GattGmasClientReadBroadcastFeaturesCfm*) msg;

            /* Find GMAP instance using connection_id_t */
            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_BY_GMAS_SERVICE_HANDLE(inst->profileHandleList,
                                                                          message->srvcHndl);
            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                gmapClientHandleReadBroadcastFeaturesCfm(gmapClientInst,
                                                        (const GattGmasClientReadBroadcastFeaturesCfm *)msg);
        }
        break;

        case GATT_GMAS_CLIENT_TERMINATE_CFM:
        {
            const GattGmasClientTerminateCfm* message;
            message = (GattGmasClientTerminateCfm*) msg;
            /* Find GMAP instance using connection_id_t */
            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_BY_GMAS_SERVICE_HANDLE(inst->profileHandleList,
                                                                          message->srvcHndl);
            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                gmapClientHandleGmasClientTerminateResp(gmapClientInst,
                                                        (const GattGmasClientTerminateCfm *)msg);
        }
        break;

        default:
        {
            /* Unrecognised GATT GMAS Client message */
            GMAP_CLIENT_WARNING("Gatt GMAS Client Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}

#ifdef INSTALL_LEA_UNICAST_CLIENT
static void gmapClientHandleCapClientUnicastMsg(GmapClientMainInst *inst, void *msg)
{
    GMAP * gmapClientInst = NULL;
    GmapClientProfileHandleListElm* elem = NULL;
    CapClientPrim *prim = (CapClientPrim *)msg;
    CsrUint16 context = 0x1234;
    uint8 i;

    GMAP_CLIENT_INFO("gmapClientHandleCapClientUnicastMsg MESSAGE:CapClientPrim:0x%x", *prim);

    switch (*prim)
    {
    	case CAP_CLIENT_REGISTER_TASK_CFM:
        {
            GmapClientRegisterTaskCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientRegisterTaskCfm* capCfm = (CapClientRegisterTaskCfm*)msg;

            cfm->type = GMAP_CLIENT_REGISTER_CAP_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_SET_PARAM_CFM:
        {
            GmapClientSetParamsCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientSetParamCfm* capCfm = (CapClientSetParamCfm*)msg;

            cfm->type = GMAP_CLIENT_SET_PARAMS_CFM;
            cfm->result = capCfm->result;
            cfm->profileHandle = capCfm->profileHandle;

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_UNICAST_CONNECT_CFM:
        {
            GmapClientUnicastConnectCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientUnicastConnectCfm* capCfm = (CapClientUnicastConnectCfm*)msg;

            cfm->type = GMAP_CLIENT_UNICAST_CONNECT_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->deviceStatusLen = capCfm->deviceStatusLen;
            cfm->context = capCfm->context;
            cfm->result = capCfm->result;
            cfm->numOfMicsConfigured = capCfm->numOfMicsConfigured;
            cfm->cigId = capCfm->cigId;
            cfm->deviceStatus = NULL;

            if (cfm->deviceStatusLen)
            {
                cfm->deviceStatus = CsrPmemZalloc(cfm->deviceStatusLen * sizeof(CapClientDeviceStatus));

                for (i = 0; i < cfm->deviceStatusLen; i++)
                {
                    cfm->deviceStatus[i].cid = capCfm->deviceStatus[i].cid;
                    cfm->deviceStatus[i].result = capCfm->deviceStatus[i].result;
                }
                CsrPmemFree(capCfm->deviceStatus);
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);

        }
        break;

        case CAP_CLIENT_UNICAST_START_STREAM_IND:
        {
            GmapClientUnicastStartStreamInd* ind = CsrPmemZalloc(sizeof(*ind));
            CapClientUnicastStartStreamInd* capInd = (CapClientUnicastStartStreamInd*)msg;

            ind->type = GMAP_CLIENT_UNICAST_START_STREAM_IND;
            ind->groupId = capInd->groupId;
            ind->cid = capInd->cid;
            ind->result = capInd->result;
            ind->cisCount = capInd->cisCount;
            ind->cigId = capInd->cigId;
            ind->cishandles = NULL;
            ind->audioConfig = NULL;

            if (ind->cisCount)
            {
                ind->cishandles = CsrPmemZalloc(sizeof(CapClientCisHandles) * ind->cisCount);

                for (i = 0; i < ind->cisCount; i++)
                {
                    ind->cishandles[i].cisHandle = capInd->cishandles[i].cisHandle;
                    ind->cishandles[i].direction = capInd->cishandles[i].direction;
                    ind->cishandles[i].audioLocation = capInd->cishandles[i].audioLocation;
                }

                CsrPmemFree(capInd->cishandles);
            }

            if (capInd->audioConfig)
            {
                ind->audioConfig = CsrPmemZalloc(sizeof(CapClientAudioConfig));
                if(ind->audioConfig)
                    CsrMemCpy(ind->audioConfig, capInd->audioConfig, sizeof(CapClientAudioConfig));

                CsrPmemFree(capInd->audioConfig);
            }

            if (capInd->vsMetadata)
            {
                if (capInd->vsMetadata->srcVsMetadata)
                    CsrPmemFree(capInd->vsMetadata->srcVsMetadata);

                if (capInd->vsMetadata->sinkVsMetadata)
                    CsrPmemFree(capInd->vsMetadata->sinkVsMetadata);

                CsrPmemFree(capInd->vsMetadata);
            }
            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);


            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, ind);

        }
        break;

        case CAP_CLIENT_UNICAST_START_STREAM_CFM:
        {
            GmapClientUnicastStartStreamCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientUnicastStartStreamCfm* capCfm = (CapClientUnicastStartStreamCfm*)msg;
			
            cfm->type = GMAP_CLIENT_UNICAST_START_STREAM_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;


            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_UPDATE_METADATA_IND:
        {
            GmapClientUnicastUpdateMetadataInd* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientUpdateMetadataInd* capInd = (CapClientUpdateMetadataInd*)msg;

            cfm->type = GMAP_CLIENT_UNICAST_UPDATE_METADATA_IND;
            cfm->result = capInd->result;
            cfm->streamingAudioContexts = capInd->streamingAudioContexts;
            cfm->groupId = capInd->groupId;
            cfm->cid = capInd->cid;
            cfm->isSink = capInd->isSink;
            cfm->metadataLen = capInd->metadataLen;
            cfm->metadata = NULL;

            if (cfm->metadataLen)
            {
                cfm->metadata = CsrPmemZalloc(cfm->metadataLen * sizeof(uint8));
                SynMemCpyS(cfm->metadata, cfm->metadataLen, capInd->metadata, cfm->metadataLen);
                CsrPmemFree(capInd->metadata);
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM:
        {
            GmapClientUnicastUpdateAudioCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientUnicastAudioUpdateCfm* capCfm = (CapClientUnicastAudioUpdateCfm*)msg;

            cfm->type = GMAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->deviceStatusLen = capCfm->deviceStatusLen;
            cfm->context = capCfm->context;
            cfm->result = capCfm->result;
            cfm->deviceStatus = NULL;

            if (cfm->deviceStatusLen)
            {
                cfm->deviceStatus = CsrPmemZalloc(cfm->deviceStatusLen * sizeof(CapClientDeviceStatus));

                for (i = 0; i < cfm->deviceStatusLen; i++)
                {
                    cfm->deviceStatus[i].cid = capCfm->deviceStatus[i].cid;
                    cfm->deviceStatus[i].result = capCfm->deviceStatus[i].result;
                }

                CsrPmemFree(capCfm->deviceStatus);
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_UNICAST_STOP_STREAM_CFM:
        {
            GmapClientUnicastStopStreamCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientUnicastStopStreamCfm* capCfm = (CapClientUnicastStopStreamCfm*)msg;

            cfm->type = GMAP_CLIENT_UNICAST_STOP_STREAM_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->deviceStatusLen = capCfm->deviceStatusLen;
            cfm->result = capCfm->result;
            cfm->released = capCfm->released;
            cfm->deviceStatus = NULL;

            if (cfm->deviceStatusLen)
            {
                cfm->deviceStatus = CsrPmemZalloc(cfm->deviceStatusLen * sizeof(CapClientDeviceStatus));

                for (i = 0; i < cfm->deviceStatusLen; i++)
                {
                    cfm->deviceStatus[i].cid = capCfm->deviceStatus[i].cid;
                    cfm->deviceStatus[i].result = capCfm->deviceStatus[i].result;
                }

                CsrPmemFree(capCfm->deviceStatus);
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_UNICAST_DISCONNECT_CFM:
        {
            GmapClientUnicastDisconnectCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientUnicastDisConnectCfm* capCfm = (CapClientUnicastDisConnectCfm*)msg;

            cfm->type = GMAP_CLIENT_UNICAST_DISCONNECT_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_CHANGE_VOLUME_CFM:
        {
            GmapClientAbsVolumeCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientChangeVolumeCfm* capCfm = (CapClientChangeVolumeCfm*)msg;

            cfm->type = GMAP_CLIENT_ABS_VOLUME_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->deviceStatusLen = capCfm->deviceStatusLen;
            cfm->result = capCfm->result;
            cfm->deviceStatus = NULL;

            if (cfm->deviceStatusLen)
            {
                cfm->deviceStatus = CsrPmemZalloc(cfm->deviceStatusLen * sizeof(CapClientDeviceStatus));

                for (i = 0; i < cfm->deviceStatusLen; i++)
                {
                    cfm->deviceStatus[i].cid = capCfm->deviceStatus[i].cid;
                    cfm->deviceStatus[i].result = capCfm->deviceStatus[i].result;
                }

                CsrPmemFree(capCfm->deviceStatus);
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_MUTE_CFM:
        {
            GmapClientMuteCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientMuteCfm* capCfm = (CapClientMuteCfm*)msg;

            cfm->type = GMAP_CLIENT_MUTE_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->deviceStatusLen = capCfm->deviceStatusLen;
            cfm->result = capCfm->result;
            cfm->deviceStatus = NULL;

            if (cfm->deviceStatusLen)
            {
                cfm->deviceStatus = CsrPmemZalloc(cfm->deviceStatusLen * sizeof(CapClientDeviceStatus));

                for (i = 0; i < cfm->deviceStatusLen; i++)
                {
                    cfm->deviceStatus[i].cid = capCfm->deviceStatus[i].cid;
                    cfm->deviceStatus[i].result = capCfm->deviceStatus[i].result;
                }
                CsrPmemFree(capCfm->deviceStatus);
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_VOLUME_STATE_IND:
        {
            GmapClientVolumeStateInd* ind = CsrPmemZalloc(sizeof(*ind));
            CapClientVolumeStateInd* capInd = (CapClientVolumeStateInd*)msg;

            ind->type = GMAP_CLIENT_VOLUME_STATE_IND;
            ind->groupId = capInd->groupId;
            ind->volumeState = capInd->volumeState;
            ind->mute = capInd->mute;
            ind->changeCounter = capInd->changeCounter;

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, ind);
        }
        break;

		default:
        {
            /* Unrecognised CAP Client message */
            GMAP_CLIENT_WARNING("Cap Client Unicast Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_SOURCE
static void gmapClientHandleCapClientBroadcastSourceMsg(GmapClientMainInst *inst, void *msg)
{
    GMAP * gmapClientInst = NULL;
    GmapClientProfileHandleListElm* elem = NULL;
    CapClientPrim *prim = (CapClientPrim *)msg;
    CsrUint16 context = 0x1234;
    uint8 i;

    GMAP_CLIENT_INFO("gmapClientHandleCapClientBroadcastSourceMsg MESSAGE:CapClientPrim:0x%x", *prim);

    switch (*prim)
    {
    	case CAP_CLIENT_BCAST_SRC_INIT_CFM:
        {
            GmapClientBroadcastSrcInitCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcInitCfm* capCfm = (CapClientBcastSrcInitCfm*)msg;
            AppTask appTask = 0x00;

            cfm->type = GMAP_CLIENT_BROADCAST_SRC_INIT_CFM;
            cfm->bcastSrcProfileHandle = (GmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
            cfm->result = capCfm->result;
            context = 0xFFFFu;

            elem = FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                appTask = gmapClientInst->appTask;

            if (cfm->result != CAP_CLIENT_RESULT_SUCCESS)
            {
                GmapClientMainInst *mainInst = gmapClientGetMainInstance();
            
                /* Free the profile instance memory */
                if (gmapClientInst)
                    FREE_GMAP_CLIENT_INST(gmapClientInst->gmapSrvcHndl);
            
                /* Remove the profile element from main list */
                if (mainInst)
                {
                    if (gmapClientInst)
                        REMOVE_GMAP_CLIENT_SERVICE_HANDLE(mainInst->profileHandleList, gmapClientInst->gmapSrvcHndl);
                }
            }
            else
            {
                if (gmapClientInst)
                    gmapClientInst->bcastSrcHandle = cfm->bcastSrcProfileHandle;
            }

            GmapClientMessageSend(appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_CONFIG_CFM:
        {
            GmapClientBroadcastSrcConfigCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcConfigCfm* capCfm = (CapClientBcastSrcConfigCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_SRC_CONFIG_CFM;
            cfm->bcastSrcProfileHandle = (GmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
            cfm->result = capCfm->result;

            elem = FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, cfm->bcastSrcProfileHandle);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_START_STREAM_CFM:
        {
            uint8 j;
            GmapClientBroadcastSrcStartStreamCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcStartStreamCfm* capCfm = (CapClientBcastSrcStartStreamCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_SRC_START_STREAM_CFM;
            cfm->bcastSrcProfileHandle = (GmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
            cfm->result = capCfm->result;
            cfm->bigId = capCfm->bigId;
            cfm->bigSyncDelay = capCfm->bigSyncDelay;
            cfm->bigParameters = capCfm->bigParameters;
            cfm->numSubGroup = capCfm->numSubGroup;
            cfm->subGroupInfo = NULL;

            if (cfm->numSubGroup)
            {
                cfm->subGroupInfo = CsrPmemZalloc(cfm->numSubGroup * sizeof(CapClientBcastSubGroupInfo));

                for (i = 0; i < cfm->numSubGroup; i++)
                {
                    cfm->subGroupInfo[i].numBis = capCfm->subGroupInfo[i].numBis;
                    cfm->subGroupInfo[i].audioConfig = NULL;
                    cfm->subGroupInfo[i].bisHandles = NULL;

                    if (cfm->subGroupInfo[i].numBis)
                    {
                        cfm->subGroupInfo[i].audioConfig = CsrPmemZalloc(cfm->subGroupInfo[i].numBis * sizeof(CapClientBcastAudioConfig));
                        cfm->subGroupInfo[i].bisHandles = CsrPmemZalloc(cfm->subGroupInfo[i].numBis * sizeof(uint16));
    
                        for (j = 0; j < cfm->subGroupInfo[i].numBis; j++)
                        {
                            cfm->subGroupInfo[i].audioConfig[j] = capCfm->subGroupInfo[i].audioConfig[j];
                            cfm->subGroupInfo[i].bisHandles[j] = capCfm->subGroupInfo[i].bisHandles[j];
                        }
                        CsrPmemFree(capCfm->subGroupInfo[i].audioConfig);
                        CsrPmemFree(capCfm->subGroupInfo[i].bisHandles);
                    }
    
                    cfm->subGroupInfo[i].metadataLen = capCfm->subGroupInfo[i].metadataLen;
                    cfm->subGroupInfo[i].metadata = NULL;
                    if (cfm->subGroupInfo[i].metadataLen)
                    {
                        cfm->subGroupInfo[i].metadata = CsrPmemZalloc(cfm->subGroupInfo[i].metadataLen);
                        CsrMemCpy(cfm->subGroupInfo[i].metadata, capCfm->subGroupInfo[i].metadata,
                                  cfm->subGroupInfo[i].metadataLen);
                        CsrPmemFree(capCfm->subGroupInfo[i].metadata);
                    }
                }
                CsrPmemFree(capCfm->subGroupInfo); 
            }

            elem = FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, cfm->bcastSrcProfileHandle);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_UPDATE_STREAM_CFM:
        {
            GmapClientBroadcastSrcUpdateStreamCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcUpdateStreamCfm* capCfm = (CapClientBcastSrcUpdateStreamCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_SRC_UPDATE_STREAM_CFM;
            cfm->bcastSrcProfileHandle = (GmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
            cfm->result = capCfm->result;

            elem = FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, cfm->bcastSrcProfileHandle);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_STOP_STREAM_CFM:
        {
            GmapClientBroadcastSrcStopStreamCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcStopStreamCfm* capCfm = (CapClientBcastSrcStopStreamCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_SRC_STOP_STREAM_CFM;
            cfm->bcastSrcProfileHandle = (GmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
            cfm->result = capCfm->result;

            elem = FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, cfm->bcastSrcProfileHandle);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_REMOVE_STREAM_CFM:
        {
            GmapClientBroadcastSrcRemoveStreamCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcRemoveStreamCfm* capCfm = (CapClientBcastSrcRemoveStreamCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_SRC_REMOVE_STREAM_CFM;
            cfm->bcastSrcProfileHandle = (GmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
            cfm->result = capCfm->result;

            elem = FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, cfm->bcastSrcProfileHandle);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);

        }
        break;

        case CAP_CLIENT_BCAST_SRC_DEINIT_CFM:
        {
            GmapClientBroadcastSrcDeinitCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcDeinitCfm* capCfm = (CapClientBcastSrcDeinitCfm*)msg;
            AppTask appTask = 0x00;

            cfm->type = GMAP_CLIENT_BROADCAST_SRC_DEINIT_CFM;
            cfm->bcastSrcProfileHandle = (GmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
            cfm->result = capCfm->result;

            elem = FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, cfm->bcastSrcProfileHandle);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                appTask = gmapClientInst->appTask;

            if (cfm->result == CAP_CLIENT_RESULT_SUCCESS)
            {
                GmapClientMainInst *mainInst = gmapClientGetMainInstance();
            
                /* Free the profile instance memory */
                if (gmapClientInst)
                    FREE_GMAP_CLIENT_INST(gmapClientInst->gmapSrvcHndl);
            
                /* Remove the profile element from main list */
                if (mainInst)
                {
                    if (gmapClientInst)
                        REMOVE_GMAP_CLIENT_SERVICE_HANDLE(mainInst->profileHandleList, gmapClientInst->gmapSrvcHndl);
                }
            }
            GmapClientMessageSend(appTask, cfm);
        }
        break;

		default:
        {
            /* Unrecognised CAP Client message */
            GMAP_CLIENT_WARNING("Cap Client Broadcast Source Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}
#endif /* INSTALL_LEA_BROADCAST_SOURCE */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
static void gmapClientHandleCapClientBroadcastAssistantMsg(GmapClientMainInst *inst, void *msg)
{
    GMAP * gmapClientInst = NULL;
    GmapClientProfileHandleListElm* elem = NULL;
    CapClientPrim *prim = (CapClientPrim *)msg;
    CsrUint16 context = 0x1234;
    uint8 i;

    GMAP_CLIENT_INFO("gmapClientHandleCapClientBroadcastAssistantMsg MESSAGE:CapClientPrim:0x%x", *prim);

    switch (*prim)
    {
    	case CAP_CLIENT_BCAST_ASST_START_SRC_SCAN_CFM:
        {
            GmapClientBroadcastAsstStartSrcScanCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstStartSrcScanCfm* capCfm = (CapClientBcastAsstStartSrcScanCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_ASST_START_SRC_SCAN_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;
            cfm->scanHandle = capCfm->scanHandle;
            cfm->statusLen = capCfm->statusLen;
            cfm->status = NULL;

            if (cfm->statusLen)
            {
                cfm->status = CsrPmemZalloc(cfm->statusLen * sizeof(CapClientDeviceStatus));

                for (i = 0; i < cfm->statusLen; i++)
                {
                    cfm->status[i].cid = capCfm->status[i].cid;
                    cfm->status[i].result = capCfm->status[i].result;
                }
                CsrPmemFree(capCfm->status);
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_SRC_REPORT_IND:
        {
            GmapClientBroadcastAsstSrcReportInd* ind = CsrPmemZalloc(sizeof(*ind));
            CapClientBcastAsstSrcReportInd* capInd = (CapClientBcastAsstSrcReportInd*)msg;

            ind->type = GMAP_CLIENT_BROADCAST_ASST_SRC_REPORT_IND;
            ind->cid = capInd->handle;
            ind->sourceAddrt = capInd->sourceAddrt;
            ind->advSid = capInd->advSid;
            ind->advHandle = capInd->advHandle;
            ind->collocated = capInd->collocated;
            ind->broadcastId = capInd->broadcastId;
            ind->numSubgroup = capInd->numSubgroup;
            ind->subgroupInfo = NULL;
            ind->bigName = NULL;

            if (capInd->numSubgroup && capInd->subgroupInfo)
            {
                ind->subgroupInfo = capInd->subgroupInfo;
            }

            ind->bigNameLen = capInd->bigNameLen;
            if (ind->bigNameLen && (capInd->bigName))
            {
                ind->bigName = capInd->bigName;
            }

            //Free the serviceData as it is not being used
            if (capInd->serviceDataLen && capInd->serviceData)
            {
                CsrPmemFree(capInd->serviceData);
            }
            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, ind);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_STOP_SRC_SCAN_CFM:
        {
            GmapClientBroadcastAsstStopSrcScanCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstStopSrcScanCfm* capCfm = (CapClientBcastAsstStopSrcScanCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_ASST_STOP_SRC_SCAN_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;
            cfm->statusLen = capCfm->statusLen;
            cfm->status = NULL;

            if (cfm->statusLen)
            {
                cfm->status = CsrPmemZalloc(cfm->statusLen * sizeof(CapClientDeviceStatus));

                for (i = 0; i < cfm->statusLen; i++)
                {
                    cfm->status[i].cid = capCfm->status[i].cid;
                    cfm->status[i].result = capCfm->status[i].result;
                }
                CsrPmemFree(capCfm->status);
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_REGISTER_NOTIFICATION_CFM:
        {
            GmapClientBroadcastAsstRegisterNotificationCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstNotficationCfm* capCfm = (CapClientBcastAsstNotficationCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;
            cfm->statusLen = capCfm->statusLen;
            cfm->status = NULL;

            if (cfm->statusLen)
            {
                cfm->status = CsrPmemZalloc(cfm->statusLen * sizeof(CapClientDeviceStatus));

                for (i = 0; i < cfm->statusLen; i++)
                {
                    cfm->status[i].cid = capCfm->status[i].cid;
                    cfm->status[i].result = capCfm->status[i].result;
                }
                CsrPmemFree(capCfm->status);
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_IND:
		{
            GmapClientBroadcastAsstReadBrsInd* ind = CsrPmemZalloc(sizeof(*ind));
            CapClientBcastAsstReadReceiveStateInd* capInd = (CapClientBcastAsstReadReceiveStateInd*)msg;

            ind->type = GMAP_CLIENT_BROADCAST_ASST_READ_BRS_IND;
            ind->groupId = capInd->groupId;
            ind->result = capInd->result;
            ind->cid = capInd->cid;
            ind->sourceId = capInd->sourceId;
            ind->sourceAddress.lap = capInd->sourceAddress.lap;
            ind->sourceAddress.uap = capInd->sourceAddress.uap;
            ind->sourceAddress.nap = capInd->sourceAddress.nap;
            ind->advertiseAddType = capInd->advertiseAddType;
            ind->advSid = capInd->advSid;
            ind->paSyncState = capInd->paSyncState;
            ind->bigEncryption = capInd->bigEncryption;
            ind->broadcastId = capInd->broadcastId;
            ind->badCode = NULL;
            ind->subGroupInfo = NULL;

            if (capInd->badCode)
            {
                ind->badCode = capInd->badCode;
            }

            ind->numSubGroups = capInd->numSubGroups;
            if (ind->numSubGroups && capInd->subGroupInfo)
            {
                 ind->subGroupInfo = capInd->subGroupInfo;
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, ind);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_CFM:
        {
            GmapClientBroadcastAsstReadBrsCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstReadReceiveStateCfm* capCfm = (CapClientBcastAsstReadReceiveStateCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_ASST_READ_BRS_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;
            cfm->cid = capCfm->cid;

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_BRS_IND:
        {
            GmapClientBroadcastAsstBrsInd* ind = CsrPmemZalloc(sizeof(*ind));
            CapClientBcastAsstBrsInd* capInd = (CapClientBcastAsstBrsInd*)msg;

            ind->type = GMAP_CLIENT_BROADCAST_ASST_BRS_IND;
            ind->groupId = capInd->groupId;
            ind->cid = capInd->cid;
            ind->sourceId = capInd->sourceId;
            ind->sourceAddress.lap = capInd->sourceAddress.lap;
            ind->sourceAddress.uap = capInd->sourceAddress.uap;
            ind->sourceAddress.nap = capInd->sourceAddress.nap;
            ind->advertiseAddType = capInd->advertiseAddType;
            ind->advSid = capInd->advSid;
            ind->paSyncState = capInd->paSyncState;
            ind->bigEncryption = capInd->bigEncryption;
            ind->broadcastId = capInd->broadcastId;
            ind->badCode = NULL;
            ind->subGroupInfo = NULL;

            if (capInd->badCode)
            {
                ind->badCode = capInd->badCode;
            }

            ind->numSubGroups = capInd->numSubGroups;
            if (ind->numSubGroups && capInd->subGroupInfo)
            {
                ind->subGroupInfo = capInd->subGroupInfo;
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, ind);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_START_SYNC_TO_SRC_CFM:
        {
            GmapClientBroadcastAsstStartSyncToSrcCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstSyncToSrcStartCfm* capCfm = (CapClientBcastAsstSyncToSrcStartCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_ASST_START_SYNC_TO_SRC_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;
            cfm->syncHandle = capCfm->syncHandle;
            cfm->advSid = capCfm->advSid;
            tbdaddr_copy(&cfm->addrt, &capCfm->addrt);
            cfm->advPhy = capCfm->advPhy;
            cfm->periodicAdvInterval =  capCfm->periodicAdvInterval;
            cfm->advClockAccuracy =   capCfm->advClockAccuracy;

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM:
        {
            GmapClientBroadcastAsstTerminateSyncToSrcCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstSyncToSrcTerminateCfm* capCfm = (CapClientBcastAsstSyncToSrcTerminateCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;
            cfm->syncHandle = capCfm->syncHandle;

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_CANCEL_SYNC_TO_SRC_CFM:
        {
            GmapClientBroadcastAsstCancelSyncToSrcCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstSyncToSrcCancelCfm* capCfm = (CapClientBcastAsstSyncToSrcCancelCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_ASST_CANCEL_SYNC_TO_SRC_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM:
        {
            GmapClientBroadcastAsstAddSrcCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstAddSrcCfm* capCfm = (CapClientBcastAsstAddSrcCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_ASST_ADD_SRC_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;
            cfm->statusLen = capCfm->statusLen;
            cfm->status = NULL;

            if (cfm->statusLen)
            {
                cfm->status = CsrPmemZalloc(cfm->statusLen * sizeof(CapClientDeviceStatus));

                for (i = 0; i < cfm->statusLen; i++)
                {
                    cfm->status[i].cid = capCfm->status[i].cid;
                    cfm->status[i].result = capCfm->status[i].result;
                }
                CsrPmemFree(capCfm->status);
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_MODIFY_SRC_CFM:
        {
            GmapClientBroadcastAsstModifySrcCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstModifySrcCfm* capCfm = (CapClientBcastAsstModifySrcCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_ASST_MODIFY_SRC_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;
            cfm->statusLen = capCfm->statusLen;
            cfm->status = NULL;

            if (cfm->statusLen)
            {
                cfm->status = CsrPmemZalloc(cfm->statusLen * sizeof(CapClientDeviceStatus));

                for (i = 0; i < cfm->statusLen; i++)
                {
                    cfm->status[i].cid = capCfm->status[i].cid;
                    cfm->status[i].result = capCfm->status[i].result;
                }
                CsrPmemFree(capCfm->status);
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_REMOVE_SRC_CFM:
        {
            GmapClientBroadcastAsstRemoveSrcCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstRemoveCfm* capCfm = (CapClientBcastAsstRemoveCfm*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_ASST_REMOVE_SRC_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;
            cfm->statusLen = capCfm->statusLen;
            cfm->status = NULL;

            if (cfm->statusLen)
            {
                cfm->status = CsrPmemZalloc(cfm->statusLen * sizeof(CapClientDeviceStatus));

                for (i = 0; i < cfm->statusLen; i++)
                {
                    cfm->status[i].cid = capCfm->status[i].cid;
                    cfm->status[i].result = capCfm->status[i].result;
                }
                CsrPmemFree(capCfm->status);
            }

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_SET_CODE_IND:
        {
            GmapClientBroadcastAsstSetCodeInd* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstSetCodeInd* capInd = (CapClientBcastAsstSetCodeInd*)msg;

            cfm->type = GMAP_CLIENT_BROADCAST_ASST_SET_CODE_IND;
            cfm->groupId = capInd->groupId;
            cfm->cid = capInd->cid;
            cfm->sourceId = capInd->sourceId;
            cfm->flags = capInd->flags;

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

		default:
        {
            /* Unrecognised CAP Client message */
            GMAP_CLIENT_WARNING("Cap Client Broadcast Assistant Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

static void gmapClientHandleCapClientMsg(GmapClientMainInst *inst, void *msg)
{
    GMAP * gmapClientInst = NULL;
    GmapClientProfileHandleListElm* elem = NULL;
    CapClientPrim *prim = (CapClientPrim *)msg;
    CsrUint16 context = 0x1234;

    GMAP_CLIENT_INFO("gmapClientHandleCapClientMsg MESSAGE:CapClientPrim:0x%x", *prim);

    switch (*prim)
    {
#ifdef INSTALL_LEA_UNICAST_CLIENT
        case CAP_CLIENT_REGISTER_TASK_CFM:
        case CAP_CLIENT_SET_PARAM_CFM:
        case CAP_CLIENT_UNICAST_CONNECT_CFM:
        case CAP_CLIENT_UNICAST_START_STREAM_IND:
        case CAP_CLIENT_UNICAST_START_STREAM_CFM:
        case CAP_CLIENT_UPDATE_METADATA_IND:
        case CAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM:
        case CAP_CLIENT_UNICAST_STOP_STREAM_CFM:
        case CAP_CLIENT_UNICAST_DISCONNECT_CFM:
        case CAP_CLIENT_CHANGE_VOLUME_CFM:
        case CAP_CLIENT_MUTE_CFM:
        case CAP_CLIENT_VOLUME_STATE_IND:
        {
            gmapClientHandleCapClientUnicastMsg(inst, msg);
        }
        break;
#endif /* INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_SOURCE
		case CAP_CLIENT_BCAST_SRC_INIT_CFM:
        case CAP_CLIENT_BCAST_SRC_CONFIG_CFM:
        case CAP_CLIENT_BCAST_SRC_START_STREAM_CFM:
        case CAP_CLIENT_BCAST_SRC_UPDATE_STREAM_CFM:
        case CAP_CLIENT_BCAST_SRC_STOP_STREAM_CFM:
        case CAP_CLIENT_BCAST_SRC_REMOVE_STREAM_CFM:
        case CAP_CLIENT_BCAST_SRC_DEINIT_CFM:
        {
            gmapClientHandleCapClientBroadcastSourceMsg(inst, msg);
        }
        break;
#endif /* INSTALL_LEA_BROADCAST_SOURCE */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
        case CAP_CLIENT_BCAST_ASST_START_SRC_SCAN_CFM:
        case CAP_CLIENT_BCAST_ASST_SRC_REPORT_IND:
        case CAP_CLIENT_BCAST_ASST_STOP_SRC_SCAN_CFM:
        case CAP_CLIENT_BCAST_ASST_REGISTER_NOTIFICATION_CFM:
        case CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_IND:
		case CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_CFM:
        case CAP_CLIENT_BCAST_ASST_BRS_IND:
        case CAP_CLIENT_BCAST_ASST_START_SYNC_TO_SRC_CFM:
        case CAP_CLIENT_BCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM:
        case CAP_CLIENT_BCAST_ASST_CANCEL_SYNC_TO_SRC_CFM:
        case CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM:
        case CAP_CLIENT_BCAST_ASST_MODIFY_SRC_CFM:
        case CAP_CLIENT_BCAST_ASST_REMOVE_SRC_CFM:
        case CAP_CLIENT_BCAST_ASST_SET_CODE_IND:
        {
            gmapClientHandleCapClientBroadcastAssistantMsg(inst, msg);
        }
        break;
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

        case CAP_CLIENT_DEREGISTER_TASK_CFM:
        {
            GmapClientDeRegisterTaskCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientDeRegisterTaskCfm* capCfm = (CapClientDeRegisterTaskCfm*)msg;

            cfm->type = GMAP_CLIENT_DEREGISTER_CAP_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;

            elem = FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (gmapClientInst)
                GmapClientMessageSend(gmapClientInst->appTask, cfm);
        }
        break;

        default:
        {
            /* Unrecognised CAP Client message */
            GMAP_CLIENT_WARNING("Cap Client Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}

/***************************************************************************/
static void  gmapClientHandleInternalMessage(GmapClientMainInst *inst, void *msg)
{
    CSR_UNUSED(inst);
    CSR_UNUSED(msg);
}

/****************************************************************************/
void gmapClientMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *msg = NULL;
    GmapClientMainInst *inst = (GmapClientMainInst * )*gash;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case GATT_SRVC_DISC_PRIM:
                gmapClientHandleGattSrvcDiscMsg(inst, msg);
                break;
            case GMAP_CLIENT_PRIM:
                gmapClientHandleInternalMessage(inst, msg);
                break;
            case GMAS_CLIENT_PRIM:
                gmapClientHandleGattGmasClientMsg(inst, msg);
                break;
            case CAP_CLIENT_PRIM:
                gmapClientHandleCapClientMsg(inst,msg);
                break;
            default:
                GMAP_CLIENT_WARNING("Profile Msg not handled \n");
        }
        SynergyMessageFree(eventClass, msg);
    }
}
