/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #60 $
******************************************************************************/

#include "tmap_client_debug.h"
#include "tmap_client_init.h"
#include "tmap_client_destroy.h"
#include "tmap_client_read.h"
#include "gatt_service_discovery_lib.h"

CsrBool tmapClientFindTmapInst(CsrCmnListElm_t *elem, void *data)
{
    CSR_UNUSED(data);
    TmapClientProfileHandleListElm *profileHndlElm = (TmapClientProfileHandleListElm *)elem;
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHndlElm->profileHandle);

    if (tmapClientInst)
        return TRUE;

    return FALSE;
}

CsrBool tmapClientInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    TmapClientProfileHandleListElm *profileHndlElm = (TmapClientProfileHandleListElm *)elem;
    ServiceHandle profileHandle = *(ServiceHandle *)data;

    if (profileHndlElm)
        return (profileHndlElm->profileHandle == profileHandle);

    return FALSE;
}

CsrBool tmapClientProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data)
{
    TmapClientProfileHandleListElm *profileHndlElm = (TmapClientProfileHandleListElm *)elem;
    CsrBtConnId     btConnId   = *(CsrBtConnId *) data;
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHndlElm->profileHandle);
    TmapClientCidListElm *cidListElm = NULL;

    if (tmapClientInst)
    {
        cidListElm = FIND_TMAP_CLIENT_CID_ELEM(tmapClientInst->tmapClientCidList.cidList, btConnId);

        if (cidListElm != NULL)
            return (cidListElm->cid == btConnId);
    }

    return FALSE;
}

CsrBool tmapClientProfileHndlFindByTmasSrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    TmapClientProfileHandleListElm *profileHndlElm = (TmapClientProfileHandleListElm *)elem;
    ServiceHandle tmasSrvcHndl = *(ServiceHandle *)data;
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHndlElm->profileHandle);

    if (tmapClientInst)
        return (tmapClientInst->tmasSrvcHndl == tmasSrvcHndl);

    return FALSE;
}

CsrBool tmapClientElemFindByCid(CsrCmnListElm_t *elem, void *data)
{
    TmapClientCidListElm *cidListElm = (TmapClientCidListElm *)elem;
    uint32 cid = *(uint32 *)data;

    if (cidListElm)
        return (cidListElm->cid == cid);

    return FALSE;
}

CsrBool tmapClientFindTmapInstFromBcastSrcHandle(CsrCmnListElm_t *elem, void *data)
{
    TmapClientProfileHandleListElm *profileHndlElm = (TmapClientProfileHandleListElm *)elem;
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHndlElm->profileHandle);
    TmapClientProfileHandle bcastSrcHandle = *(TmapClientProfileHandle *)data;

    if ((tmapClientInst) && (tmapClientInst->bcastSrcHandle == bcastSrcHandle))
        return TRUE;

    return FALSE;
}
/****************************************************************************/
static void tmapClientHandleGattSrvcDiscMsg(TmapClientMainInst *inst, Msg *msg)
{
    TMAP *tmapClientInst = NULL;
    TmapClientProfileHandleListElm* elem = NULL;
    GattSdPrim* prim = (GattSdPrim*)msg;

    switch (*prim)
    {
        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
        {
            GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *cfm =
                (GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *) msg;

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_BY_BTCONNID(inst->profileHandleList, cfm->cid);
            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst == NULL)
                return;

            if ((cfm->result == GATT_SD_RESULT_SUCCESS) && cfm->srvcInfoCount)
            {
                uint16 index, count;

                count = cfm->srvcInfoCount;

                for (index = 0; index < count; index++)
                {
                    GattTmasClientInitData init_data;
                    TMAP_CLIENT_DEBUG("(TMAP) : Start Hndl = 0x%x, End Hndl = 0x%x, Id = 0x%x\n",
                                     cfm->srvcInfo[0].startHandle, cfm->srvcInfo[0].endHandle, cfm->srvcInfo[0].srvcId);

                    init_data.cid = cfm->cid;
                    init_data.startHandle = cfm->srvcInfo[index].startHandle;
                    init_data.endHandle = cfm->srvcInfo[index].endHandle;

                    GattTmasClientInitReq(tmapClientInst->libTask, &init_data, NULL);
                }
                CsrPmemFree(cfm->srvcInfo);
            }
#ifdef INSTALL_LEA_UNICAST_CLIENT
            else if (cfm->result == GATT_SD_RESULT_SRVC_ID_NOT_FOUND)
            {
                tmapClientSendInitCfm(tmapClientInst, TMAP_CLIENT_STATUS_SUCCESS_TMAS_SRVC_NOT_FOUND);
            }
#endif
            else
            {
                tmapClientSendInitCfm(tmapClientInst, TMAP_CLIENT_STATUS_DISCOVERY_ERR);
                REMOVE_TMAP_CLIENT_SERVICE_HANDLE(inst->profileHandleList, tmapClientInst->tmapSrvcHndl);
                FREE_TMAP_CLIENT_INST(tmapClientInst->tmapSrvcHndl);
            }
            break;
        }

        default:
        {
            /* Unrecognised GATT Manager message */
            TMAP_CLIENT_WARNING("Gatt SD Msg not handled \n");
        }
        break;
    }
}

/*************************************************************/
static void tmapClientHandleGattTmasClientMsg(TmapClientMainInst *inst, void *msg)
{
    TMAP * tmapClientInst = NULL;
    TmapClientProfileHandleListElm* elem = NULL;
    GattTmasClientMessageId *prim = (GattTmasClientMessageId *)msg;

    TMAP_CLIENT_INFO("tmapClientHandleGattTmasClientMsg MESSAGE:GattTmasClientMessageId:0x%x", *prim);

    switch (*prim)
    {
        case GATT_TMAS_CLIENT_INIT_CFM:
        {
            const GattTmasClientInitCfm* message;
            message = (GattTmasClientInitCfm*) msg;

            /* Find tmap instance using connection_id_t */
            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_BY_BTCONNID(inst->profileHandleList,
                                                               message->cid);
            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                tmapClientHandleTmasClientInitResp(tmapClientInst,
                                                   (const GattTmasClientInitCfm *)msg);
        }
        break;

        case GATT_TMAS_CLIENT_ROLE_CFM:
        {
            const GattTmasClientRoleCfm* message;
            message = (GattTmasClientRoleCfm*) msg;

            /* Find TMAP instance using connection_id_t */
            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_BY_TMAS_SERVICE_HANDLE(inst->profileHandleList,
                                                                          message->srvcHndl);
            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                tmapClientHandleReadRoleCharacCfm(tmapClientInst,
                                                  (const GattTmasClientRoleCfm *)msg);
        }
        break;

        case GATT_TMAS_CLIENT_TERMINATE_CFM:
        {
            const GattTmasClientTerminateCfm* message;
            message = (GattTmasClientTerminateCfm*) msg;
            /* Find TMAP instance using connection_id_t */
            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_BY_TMAS_SERVICE_HANDLE(inst->profileHandleList,
                                                                          message->srvcHndl);
            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                tmapClientHandleTmasClientTerminateResp(tmapClientInst,
                                                        (const GattTmasClientTerminateCfm *)msg);
        }
        break;

        default:
        {
            /* Unrecognised GATT TMAS Client message */
            TMAP_CLIENT_WARNING("Gatt TMAS Client Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}

#ifdef INSTALL_LEA_UNICAST_CLIENT
static void tmapClientHandleCapClientUnicastMsg(TmapClientMainInst *inst, void *msg)
{
    TMAP * tmapClientInst = NULL;
    TmapClientProfileHandleListElm* elem = NULL;
    CapClientPrim *prim = (CapClientPrim *)msg;
    CsrUint16 context = 0x1234;
    uint8 i;

    TMAP_CLIENT_INFO("tmapClientHandleCapClientUnicastMsg MESSAGE:CapClientPrim:0x%x", *prim);

    switch (*prim)
    {
    	case CAP_CLIENT_UNICAST_CONNECT_CFM:
        {
            TmapClientUnicastConnectCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientUnicastConnectCfm* capCfm = (CapClientUnicastConnectCfm*)msg;

            cfm->type = TMAP_CLIENT_UNICAST_CONNECT_CFM;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);

        }
        break;

        case CAP_CLIENT_UNICAST_START_STREAM_IND:
        {
            TmapClientUnicastStartStreamInd* ind = CsrPmemZalloc(sizeof(*ind));
            CapClientUnicastStartStreamInd* capInd = (CapClientUnicastStartStreamInd*)msg;

            ind->type = TMAP_CLIENT_UNICAST_START_STREAM_IND;
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
            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);


            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, ind);

        }
        break;

        case CAP_CLIENT_UNICAST_START_STREAM_CFM:
        {
            TmapClientUnicastStartStreamCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientUnicastStartStreamCfm* capCfm = (CapClientUnicastStartStreamCfm*)msg;
			
            cfm->type = TMAP_CLIENT_UNICAST_START_STREAM_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;


            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_UPDATE_METADATA_IND:
        {
            TmapClientUnicastUpdateMetadataInd* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientUpdateMetadataInd* capInd = (CapClientUpdateMetadataInd*)msg;

            cfm->type = TMAP_CLIENT_UNICAST_UPDATE_METADATA_IND;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM:
        {
            TmapClientUnicastUpdateAudioCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientUnicastAudioUpdateCfm* capCfm = (CapClientUnicastAudioUpdateCfm*)msg;

            cfm->type = TMAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

#ifdef INSTALL_LEA_UNICAST_CLIENT_REMOTE_STOP
        case CAP_CLIENT_UNICAST_STOP_STREAM_IND:
        {
            TmapClientUnicastStopStreamInd* cfm = (TmapClientUnicastStopStreamInd*)msg;

            cfm->type = TMAP_CLIENT_UNICAST_STOP_STREAM_IND;

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);

        }
        break;
#endif

        case CAP_CLIENT_UNICAST_STOP_STREAM_CFM:
        {
            TmapClientUnicastStopStreamCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientUnicastStopStreamCfm* capCfm = (CapClientUnicastStopStreamCfm*)msg;

            cfm->type = TMAP_CLIENT_UNICAST_STOP_STREAM_CFM;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_UNICAST_DISCONNECT_CFM:
        {
            TmapClientUnicastDisconnectCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientUnicastDisConnectCfm* capCfm = (CapClientUnicastDisConnectCfm*)msg;

            cfm->type = TMAP_CLIENT_UNICAST_DISCONNECT_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_CHANGE_VOLUME_CFM:
        {
            TmapClientAbsVolumeCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientChangeVolumeCfm* capCfm = (CapClientChangeVolumeCfm*)msg;

            cfm->type = TMAP_CLIENT_ABS_VOLUME_CFM;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;


        case CAP_CLIENT_MUTE_CFM:
        {
            TmapClientMuteCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientMuteCfm* capCfm = (CapClientMuteCfm*)msg;

            cfm->type = TMAP_CLIENT_MUTE_CFM;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_VOLUME_STATE_IND:
        {
            TmapClientVolumeStateInd* ind = CsrPmemZalloc(sizeof(*ind));
            CapClientVolumeStateInd* capInd = (CapClientVolumeStateInd*)msg;

            ind->type = TMAP_CLIENT_VOLUME_STATE_IND;
            ind->groupId = capInd->groupId;
            ind->volumeState = capInd->volumeState;
            ind->mute = capInd->mute;
            ind->changeCounter = capInd->changeCounter;

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, ind);
        }
        break;

		default:
        {
            /* Unrecognised CAP Client message */
            TMAP_CLIENT_WARNING("Cap Client Unicast Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_SOURCE
static void tmapClientHandleCapClientBroadcastSourceMsg(TmapClientMainInst *inst, void *msg)
{
    TMAP * tmapClientInst = NULL;
    TmapClientProfileHandleListElm* elem = NULL;
    CapClientPrim *prim = (CapClientPrim *)msg;
    CsrUint16 context = 0x1234;
    uint8 i;

    TMAP_CLIENT_INFO("tmapClientHandleCapClientBroadcastSourceMsg MESSAGE:CapClientPrim:0x%x", *prim);

    switch (*prim)
    {
    	case CAP_CLIENT_BCAST_SRC_INIT_CFM:
        {
            TmapClientBroadcastSrcInitCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcInitCfm* capCfm = (CapClientBcastSrcInitCfm*)msg;
            AppTask appTask = 0x00;

            cfm->type = TMAP_CLIENT_BROADCAST_SRC_INIT_CFM;
            cfm->bcastSrcProfileHandle = (TmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
            cfm->result = capCfm->result;
            context = 0xFFFFu;

            elem = FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                appTask = tmapClientInst->appTask;

            if (cfm->result != CAP_CLIENT_RESULT_SUCCESS)
            {
                TmapClientMainInst *mainInst = tmapClientGetMainInstance();
            
                /* Free the profile instance memory */
                if (tmapClientInst)
                    FREE_TMAP_CLIENT_INST(tmapClientInst->tmapSrvcHndl);
            
                /* Remove the profile element from main list */
                if (mainInst)
                {
                    if (tmapClientInst)
                        REMOVE_TMAP_CLIENT_SERVICE_HANDLE(mainInst->profileHandleList, tmapClientInst->tmapSrvcHndl);
                }
            }
            else
            {
                if (tmapClientInst)
                    tmapClientInst->bcastSrcHandle = cfm->bcastSrcProfileHandle;
            }

            TmapClientMessageSend(appTask, cfm);
        }
        break;


        case CAP_CLIENT_BCAST_SRC_CONFIG_CFM:
        {
            TmapClientBroadcastSrcConfigCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcConfigCfm* capCfm = (CapClientBcastSrcConfigCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_SRC_CONFIG_CFM;
            cfm->bcastSrcProfileHandle = (TmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
            cfm->result = capCfm->result;

            elem = FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, cfm->bcastSrcProfileHandle);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_START_STREAM_CFM:
        {
            uint8 j;
            TmapClientBroadcastSrcStartStreamCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcStartStreamCfm* capCfm = (CapClientBcastSrcStartStreamCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_SRC_START_STREAM_CFM;
            cfm->bcastSrcProfileHandle = (TmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
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

            elem = FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, cfm->bcastSrcProfileHandle);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_UPDATE_STREAM_CFM:
        {
            TmapClientBroadcastSrcUpdateStreamCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcUpdateStreamCfm* capCfm = (CapClientBcastSrcUpdateStreamCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_SRC_UPDATE_STREAM_CFM;
            cfm->bcastSrcProfileHandle = (TmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
            cfm->result = capCfm->result;

            elem = FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, cfm->bcastSrcProfileHandle);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_STOP_STREAM_CFM:
        {
            TmapClientBroadcastSrcStopStreamCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcStopStreamCfm* capCfm = (CapClientBcastSrcStopStreamCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_SRC_STOP_STREAM_CFM;
            cfm->bcastSrcProfileHandle = (TmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
            cfm->result = capCfm->result;

            elem = FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, cfm->bcastSrcProfileHandle);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_REMOVE_STREAM_CFM:
        {
            TmapClientBroadcastSrcRemoveStreamCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcRemoveStreamCfm* capCfm = (CapClientBcastSrcRemoveStreamCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_SRC_REMOVE_STREAM_CFM;
            cfm->bcastSrcProfileHandle = (TmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
            cfm->result = capCfm->result;

            elem = FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, cfm->bcastSrcProfileHandle);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);

        }
        break;

        case CAP_CLIENT_BCAST_SRC_DEINIT_CFM:
        {
            TmapClientBroadcastSrcDeinitCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastSrcDeinitCfm* capCfm = (CapClientBcastSrcDeinitCfm*)msg;
            AppTask appTask = 0x00;

            cfm->type = TMAP_CLIENT_BROADCAST_SRC_DEINIT_CFM;
            cfm->bcastSrcProfileHandle = (TmapClientProfileHandle)capCfm->bcastSrcProfileHandle;
            cfm->result = capCfm->result;

            elem = FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(inst->profileHandleList, cfm->bcastSrcProfileHandle);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                appTask = tmapClientInst->appTask;

            if (cfm->result == CAP_CLIENT_RESULT_SUCCESS)
            {
                TmapClientMainInst *mainInst = tmapClientGetMainInstance();
            
                /* Free the profile instance memory */
                if (tmapClientInst)
                    FREE_TMAP_CLIENT_INST(tmapClientInst->tmapSrvcHndl);
            
                /* Remove the profile element from main list */
                if (mainInst)
                {
                    if (tmapClientInst)
                        REMOVE_TMAP_CLIENT_SERVICE_HANDLE(mainInst->profileHandleList, tmapClientInst->tmapSrvcHndl);
                }
            }
            TmapClientMessageSend(appTask, cfm);
        }
        break;
		default:
        {
            /* Unrecognised CAP Client message */
            TMAP_CLIENT_WARNING("Cap Client Broadcast Source Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}
#endif /* INSTALL_LEA_BROADCAST_SOURCE */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT 
static void tmapClientHandleCapClientBroadcastAssistantMsg(TmapClientMainInst *inst, void *msg)
{
    TMAP * tmapClientInst = NULL;
    TmapClientProfileHandleListElm* elem = NULL;
    CapClientPrim *prim = (CapClientPrim *)msg;
    CsrUint16 context = 0x1234;
    uint8 i;

    TMAP_CLIENT_INFO("tmapClientHandleCapClientBroadcastAssistantMsg MESSAGE:CapClientPrim:0x%x", *prim);

    switch (*prim)
    {
    	case CAP_CLIENT_BCAST_ASST_START_SRC_SCAN_CFM:
        {
            TmapClientBroadcastAsstStartSrcScanCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstStartSrcScanCfm* capCfm = (CapClientBcastAsstStartSrcScanCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_ASST_START_SRC_SCAN_CFM;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_SRC_REPORT_IND:
        {
            TmapClientBroadcastAsstSrcReportInd* ind = CsrPmemZalloc(sizeof(*ind));
            CapClientBcastAsstSrcReportInd* capInd = (CapClientBcastAsstSrcReportInd*)msg;

            ind->type = TMAP_CLIENT_BROADCAST_ASST_SRC_REPORT_IND;
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

            /* Free the serviceData as it is not being used */
            if (capInd->serviceDataLen && capInd->serviceData)
            {
                CsrPmemFree(capInd->serviceData);
            }
            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, ind);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_STOP_SRC_SCAN_CFM:
        {
            TmapClientBroadcastAsstStopSrcScanCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstStopSrcScanCfm* capCfm = (CapClientBcastAsstStopSrcScanCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_ASST_STOP_SRC_SCAN_CFM;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_REGISTER_NOTIFICATION_CFM:
        {
            TmapClientBroadcastAsstRegisterNotificationCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstNotficationCfm* capCfm = (CapClientBcastAsstNotficationCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_IND:
		{
            TmapClientBroadcastAsstReadBrsInd* ind = CsrPmemZalloc(sizeof(*ind));
            CapClientBcastAsstReadReceiveStateInd* capInd = (CapClientBcastAsstReadReceiveStateInd*)msg;

            ind->type = TMAP_CLIENT_BROADCAST_ASST_READ_BRS_IND;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, ind);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_CFM:
        {
            TmapClientBroadcastAsstReadBrsCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstReadReceiveStateCfm* capCfm = (CapClientBcastAsstReadReceiveStateCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_ASST_READ_BRS_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;
            cfm->cid = capCfm->cid;

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_BRS_IND:
        {
            TmapClientBroadcastAsstBrsInd* ind = CsrPmemZalloc(sizeof(*ind));
            CapClientBcastAsstBrsInd* capInd = (CapClientBcastAsstBrsInd*)msg;

            ind->type = TMAP_CLIENT_BROADCAST_ASST_BRS_IND;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, ind);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_START_SYNC_TO_SRC_CFM:
        {
            TmapClientBroadcastAsstStartSyncToSrcCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstSyncToSrcStartCfm* capCfm = (CapClientBcastAsstSyncToSrcStartCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_ASST_START_SYNC_TO_SRC_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;
            cfm->syncHandle = capCfm->syncHandle;
            cfm->advSid = capCfm->advSid;
            tbdaddr_copy(&cfm->addrt, &capCfm->addrt);
            cfm->advPhy = capCfm->advPhy;
            cfm->periodicAdvInterval =  capCfm->periodicAdvInterval;
            cfm->advClockAccuracy =   capCfm->advClockAccuracy;

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM:
        {
            TmapClientBroadcastAsstTerminateSyncToSrcCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstSyncToSrcTerminateCfm* capCfm = (CapClientBcastAsstSyncToSrcTerminateCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;
            cfm->syncHandle = capCfm->syncHandle;

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_CANCEL_SYNC_TO_SRC_CFM:
        {
            TmapClientBroadcastAsstCancelSyncToSrcCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstSyncToSrcCancelCfm* capCfm = (CapClientBcastAsstSyncToSrcCancelCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_ASST_CANCEL_SYNC_TO_SRC_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM:
        {
            TmapClientBroadcastAsstAddSrcCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstAddSrcCfm* capCfm = (CapClientBcastAsstAddSrcCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_ASST_ADD_SRC_CFM;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_MODIFY_SRC_CFM:
        {
            TmapClientBroadcastAsstModifySrcCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstModifySrcCfm* capCfm = (CapClientBcastAsstModifySrcCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_ASST_MODIFY_SRC_CFM;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_REMOVE_SRC_CFM:
        {
            TmapClientBroadcastAsstRemoveSrcCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstRemoveCfm* capCfm = (CapClientBcastAsstRemoveCfm*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_ASST_REMOVE_SRC_CFM;
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

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_SET_CODE_IND:
        {
            TmapClientBroadcastAsstSetCodeInd* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientBcastAsstSetCodeInd* capInd = (CapClientBcastAsstSetCodeInd*)msg;

            cfm->type = TMAP_CLIENT_BROADCAST_ASST_SET_CODE_IND;
            cfm->groupId = capInd->groupId;
            cfm->cid = capInd->cid;
            cfm->sourceId = capInd->sourceId;
            cfm->flags = capInd->flags;

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

		default:
        {
            /* Unrecognised CAP Client message */
            TMAP_CLIENT_WARNING("Cap Client Broadcast Assistant Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

static void tmapClientHandleCapClientMsg(TmapClientMainInst *inst, void *msg)
{
    TMAP * tmapClientInst = NULL;
    TmapClientProfileHandleListElm* elem = NULL;
    CapClientPrim *prim = (CapClientPrim *)msg;
    CsrUint16 context = 0x1234;

    TMAP_CLIENT_INFO("tmapClientHandleCapClientMsg MESSAGE:CapClientPrim:0x%x", *prim);

    switch (*prim)
    {
        case CAP_CLIENT_REGISTER_TASK_CFM:
        {
            TmapClientRegisterTaskCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientRegisterTaskCfm* capCfm = (CapClientRegisterTaskCfm*)msg;

            cfm->type = TMAP_CLIENT_REGISTER_CAP_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        case CAP_CLIENT_SET_PARAM_CFM:
        {
            TmapClientSetParamsCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientSetParamCfm* capCfm = (CapClientSetParamCfm*)msg;

            cfm->type = TMAP_CLIENT_SET_PARAMS_CFM;
            cfm->result = capCfm->result;
            cfm->profileHandle = capCfm->profileHandle;

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

#ifdef INSTALL_LEA_UNICAST_CLIENT
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
            tmapClientHandleCapClientUnicastMsg(inst,msg);
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
        	tmapClientHandleCapClientBroadcastSourceMsg(inst,msg);
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
        	tmapClientHandleCapClientBroadcastAssistantMsg(inst,msg);
        }
		break;
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

        case CAP_CLIENT_DEREGISTER_TASK_CFM:
        {
            TmapClientDeRegisterTaskCfm* cfm = CsrPmemZalloc(sizeof(*cfm));
            CapClientDeRegisterTaskCfm* capCfm = (CapClientDeRegisterTaskCfm*)msg;

            cfm->type = TMAP_CLIENT_DEREGISTER_CAP_CFM;
            cfm->groupId = capCfm->groupId;
            cfm->result = capCfm->result;

            elem = FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

            if (elem)
                tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (tmapClientInst)
                TmapClientMessageSend(tmapClientInst->appTask, cfm);
        }
        break;

        default:
        {
            /* Unrecognised CAP Client message */
            TMAP_CLIENT_WARNING("Cap Client Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}

/***************************************************************************/
static void  tmapClientHandleInternalMessage(TmapClientMainInst *inst, void *msg)
{
    CSR_UNUSED(inst);
    CSR_UNUSED(msg);
}

/****************************************************************************/
void tmapClientMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *msg = NULL;
    TmapClientMainInst *inst = (TmapClientMainInst * )*gash;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case GATT_SRVC_DISC_PRIM:
                tmapClientHandleGattSrvcDiscMsg(inst, msg);
                break;
            case TMAP_CLIENT_PRIM:
                tmapClientHandleInternalMessage(inst, msg);
                break;
            case TMAS_CLIENT_PRIM:
                tmapClientHandleGattTmasClientMsg(inst, msg);
                break;
            case CAP_CLIENT_PRIM:
                tmapClientHandleCapClientMsg(inst,msg);
                break;
            default:
                TMAP_CLIENT_WARNING("Profile Msg not handled \n");
        }
        SynergyMessageFree(eventClass, msg);
    }
}
