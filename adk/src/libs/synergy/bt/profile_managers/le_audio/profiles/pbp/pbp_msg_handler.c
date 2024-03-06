/******************************************************************************
 Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #3 $
******************************************************************************/

#include "pbp_debug.h"
#include "pbp_init.h"
#include "pbp_prim.h"

#define RND_CONTEXT 0x1234

/****************************************************************************/
CsrBool findPbpInst(CsrCmnListElm_t *elem, void *data)
{
    CSR_UNUSED(data);
    PbpProfileHandleListElm* profileHndlElm = (PbpProfileHandleListElm*)elem;
    PBP *pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHndlElm->profileHandle);

    if (pbpInst)
        return TRUE;

    return FALSE;
}

/****************************************************************************/
CsrBool pbpInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    PbpProfileHandleListElm* profileHndlElm = (PbpProfileHandleListElm*)elem;
    ServiceHandle profileHandle = *(ServiceHandle *)data;

    if (profileHndlElm)
        return (profileHndlElm->profileHandle == profileHandle);

    return FALSE;
} 

/****************************************************************************/
CsrBool pbpFindPbpInstFromBcastSrcHandle(CsrCmnListElm_t* elem, void* data)
{
    PbpProfileHandleListElm* profileHndlElm = (PbpProfileHandleListElm*)elem;
    PBP* pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(profileHndlElm->profileHandle);
    PbpProfileHandle bcastSrcHandle = *(PbpProfileHandle*)data;

    if ((pbpInst) && (pbpInst->bcastSrcHandle == bcastSrcHandle))
        return TRUE;

    return FALSE;
}

/****************************************************************************/
static void pbpSendBroadcastSrcInitCfm(PBP *pbpInst, CapClientBcastSrcInitCfm *msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastSrcInitCfm);

    message->bcastSrcProfileHandle = msg->bcastSrcProfileHandle;
    message->result = msg->result;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_SRC_INIT_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastSrcDeinitCfm(PBP* pbpInst, CapClientBcastSrcDeinitCfm* msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastSrcDeinitCfm);

    message->bcastSrcProfileHandle = msg->bcastSrcProfileHandle;
    message->result = msg->result;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_SRC_DEINIT_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastSrcSetParamCfm(PBP* pbpInst, CapClientSetParamCfm *msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastSrcSetParamCfm);

    message->bcastSrcProfileHandle = msg->profileHandle;
    message->result = msg->result;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_SRC_SET_PARAM_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastSrcConfigCfm(PBP *pbpInst, CapClientBcastSrcConfigCfm* msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastSrcConfigCfm);

    message->bcastSrcProfileHandle = msg->bcastSrcProfileHandle;
    message->result = msg->result;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_SRC_CONFIG_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastSrcStartStreamCfm(PBP *pbpInst, CapClientBcastSrcStartStreamCfm* msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastSrcStartStreamCfm);

    message->bcastSrcProfileHandle = msg->bcastSrcProfileHandle;
    message->result = msg->result;
    message->bigId = msg->bigId;

    if (msg->bigParameters)
        message->bigParameters = msg->bigParameters;

    message->bigSyncDelay = msg->bigSyncDelay;
    message->numSubGroup = msg->numSubGroup;
    
    if (msg->subGroupInfo && msg->numSubGroup)
        message->subGroupInfo = msg->subGroupInfo;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_SRC_START_STREAM_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastSrcUpdateAudioCfm(PBP *pbpInst, CapClientBcastSrcUpdateStreamCfm *msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastSrcUpdateAudioCfm);

    message->bcastSrcProfileHandle = msg->bcastSrcProfileHandle;
    message->result = msg->result;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_SRC_UPDATE_AUDIO_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastSrcStopStreamCfm(PBP *pbpInst, CapClientBcastSrcStopStreamCfm *msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastSrcStopStreamCfm);

    message->bcastSrcProfileHandle = msg->bcastSrcProfileHandle;
    message->result = msg->result;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_SRC_STOP_STREAM_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastSrcRemoveStreamCfm(PBP* pbpInst, CapClientBcastSrcRemoveStreamCfm*msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastSrcRemoveStreamCfm);

    message->bcastSrcProfileHandle = msg->bcastSrcProfileHandle;
    message->result = msg->result;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_SRC_REMOVE_STREAM_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantScanSrcStartCfm(PBP* pbpInst, CapClientBcastAsstStartSrcScanCfm* msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantScanSrcStartCfm);

    message->groupId = msg->groupId;
    message->scanHandle = msg->scanHandle;
    message->statusLen = msg->statusLen;
    message->status = msg->status;
    message->result = msg->result;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_SCAN_SRC_START_CFM, message);
}

/****************************************************************************/
static void pbpPrepareBroadcastAssistantCommonMsg(CapClientBcastAsstCommonMsg *msg,
                                                  PbpBroadcastAssistantCommonMsg* message)
{
    message->groupId = msg->groupId;
    message->statusLen = msg->statusLen;
    message->status = msg->status;
    message->result = msg->result;
}

/****************************************************************************/
static void pbpSendBroadcastAssistantScanSrcStopCfm(PBP *pbpInst, CapClientBcastAsstStopSrcScanCfm *msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantScanSrcStopCfm);

    pbpPrepareBroadcastAssistantCommonMsg(msg, (PbpBroadcastAssistantCommonMsg *) message);

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_SCAN_SRC_STOP_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantStartSyncToSrcCfm(PBP *pbpInst, CapClientBcastAsstSyncToSrcStartCfm *msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantStartSyncToSrcCfm);

    message->addrt = msg->addrt;
    message->advClockAccuracy = msg->advClockAccuracy;
    message->advPhy = msg->advPhy;
    message->advSid = msg->advSid;
    message->groupId = msg->groupId;
    message->periodicAdvInterval = msg->periodicAdvInterval;
    message->result = msg->result;
    message->syncHandle = msg->syncHandle;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_START_SYNC_TO_SRC_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantTerminateSyncToSrcCfm(PBP* pbpInst, CapClientBcastAsstSyncToSrcTerminateCfm *msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantTerminateSyncToSrcCfm);

    message->groupId = msg->groupId;
    message->result = msg->result;
    message->syncHandle = msg->type;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_TERMINATE_SYNC_TO_SRC_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantSyncToSrcCancelCfm(PBP* pbpInst, CapClientBcastAsstSyncToSrcCancelCfm *msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantSyncToSrcCancelCfm);

    message->groupId = msg->groupId;
    message->result = msg->result;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantAddSrcCfm(PBP* pbpInst, CapClientBcastAsstAddSrcCfm *msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantAddSrcCfm);

    pbpPrepareBroadcastAssistantCommonMsg(msg, (PbpBroadcastAssistantCommonMsg*)message);

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_ADD_SRC_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantModifySrcCfm(PBP* pbpInst, CapClientBcastAsstModifySrcCfm* msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantModifySrcCfm);

    pbpPrepareBroadcastAssistantCommonMsg(msg, (PbpBroadcastAssistantCommonMsg*)message);

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_MODIFY_SRC_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantRemoveSrcCfm(PBP* pbpInst, CapClientBcastAsstRemoveCfm* msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantRemoveSrcCfm);

    pbpPrepareBroadcastAssistantCommonMsg(msg, (PbpBroadcastAssistantCommonMsg*)message);

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_REMOVE_SRC_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantRegForNotificationCfm(PBP* pbpInst, CapClientBcastAsstNotficationCfm* msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantRegForNotificationCfm);

    pbpPrepareBroadcastAssistantCommonMsg(msg, (PbpBroadcastAssistantCommonMsg*)message);

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_REG_FOR_NOTIFICATION_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantReadReceiveStateCfm(PBP* pbpInst, CapClientBcastAsstReadReceiveStateCfm* msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantReadReceiveStateCfm);

    message->groupId = msg->groupId;
    message->result = msg->result;
    message->cid = msg->cid;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_READ_RECEIVE_STATE_CFM, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantSrcReportInd(PBP* pbpInst, CapClientBcastAsstSrcReportInd *msg, AudioAnnouncementParserPbpDataType pbpFeatures)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantSrcReportInd);

    message->sourceAddrt = msg->sourceAddrt;
    message->advSid = msg->advSid;
    message->advHandle = msg->advHandle;
    message->collocated = msg->collocated;
    message->broadcastId = msg->broadcastId;
    message->numSubgroup = msg->numSubgroup;
    message->subgroupInfo = msg->subgroupInfo;
    message->bigNameLen = msg->bigNameLen;
    message->bigName = msg->bigName;
    message->pbpFeatures = pbpFeatures;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_SRC_REPORT_IND, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantBrsInd(PBP* pbpInst, CapClientBcastAsstBrsInd *msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantBrsInd);

    message->groupId = msg->groupId;
    message->cid = msg->cid;
    message->sourceId = msg->sourceId;
    message->sourceAddrt.addr.lap = msg->sourceAddress.lap;
    message->sourceAddrt.addr.uap = msg->sourceAddress.uap;
    message->sourceAddrt.addr.nap = msg->sourceAddress.nap;
    message->sourceAddrt.type = msg->advertiseAddType;
    message->advSid = msg->advSid;
    message->paSyncState = (PbpPaSyncState) msg->paSyncState;
    message->bigEncryption = msg->bigEncryption;
    message->broadcastId = msg->broadcastId;
    message->badCode = msg->badCode;
    message->numSubGroups = msg->numSubGroups;
    message->subGroupInfo = (PbpSubgroupInfo*) msg->subGroupInfo;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_BRS_IND, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantSyncLossInd(PBP* pbpInst, CapClientBcastAsstSyncLossInd *msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantSyncLossInd);

    message->groupId = msg->groupId;
    message->cid = msg->cid;
    message->syncHandle = msg->syncHandle;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_SYNC_LOSS_IND, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantSetCodeInd(PBP* pbpInst, CapClientBcastAsstSetCodeInd *msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantSetCodeInd);

    message->groupId = msg->groupId;
    message->cid = msg->cid;
    message->sourceId = msg->sourceId;
    message->flags = msg->flags;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_SET_CODE_IND, message);
}

/****************************************************************************/
static void pbpSendBroadcastAssistantReadReceiveStateInd(PBP* pbpInst, CapClientBcastAsstReadReceiveStateInd* msg)
{
    MAKE_PBP_MESSAGE(PbpBroadcastAssistantReadReceiveStateInd);

    message->groupId = msg->groupId;
    message->result = msg->result;
    message->cid = msg->cid;
    message->sourceId = msg->sourceId;
    message->sourceAddrt.addr.lap = msg->sourceAddress.lap;
    message->sourceAddrt.addr.uap = msg->sourceAddress.uap;
    message->sourceAddrt.addr.nap = msg->sourceAddress.nap;
    message->sourceAddrt.type = msg->advertiseAddType;
    message->advSid = msg->advSid;
    message->paSyncState = msg->paSyncState;
    message->bigEncryption = msg->bigEncryption;
    message->broadcastId = msg->broadcastId;
    message->badCode = msg->badCode;
    message->numSubGroups = msg->numSubGroups;
    message->subGroupInfo = (PbpSubgroupInfo *) msg->subGroupInfo;

    PbpMessageSend(pbpInst->appTask, PBP_BROADCAST_ASSISTANT_READ_RECEIVE_STATE_IND, message);
}

/****************************************************************************/
static void pbpSendtRegisterCapCfm(PBP* pbpInst, CapClientRegisterTaskCfm *msg)
{
    MAKE_PBP_MESSAGE(PbpRegisterTaskCfm);

    message->groupId = msg->groupId;
    message->result = msg->result;

    PbpMessageSend(pbpInst->appTask, PBP_REGISTER_CAP_CFM, message);
}

/****************************************************************************/
static void pbpSendtDeRegisterCapCfm(PBP* pbpInst, CapClientDeRegisterTaskCfm *msg)
{
    MAKE_PBP_MESSAGE(PbpDeRegisterTaskCfm);

    message->groupId = msg->groupId;
    message->result = msg->result;

    PbpMessageSend(pbpInst->appTask, PBP_DEREGISTER_TASK_CFM, message);
}

/****************************************************************************/
static void pbpHandleCapClientMsg(PbpMainInst *inst, void *msg)
{
    PBP *pbpInst = NULL;
    PbpProfileHandleListElm *elem = NULL;
    CapClientPrim *prim = (CapClientPrim *) msg;

    /* We need to pass this parameter to FIND_PBP_PROFILE_HANDLE_FROM_INST, but we will nver use it.
       We assign a random value.
     */
    CsrUint16 context = RND_CONTEXT;

    elem = FIND_PBP_PROFILE_HANDLE_FROM_INST(inst->profileHandleList, context);

    if (elem)
        pbpInst = FIND_PBP_INST_BY_PROFILE_HANDLE(elem->profileHandle);

    if (!pbpInst)
        return;

    switch (*prim)
    {
        case CAP_CLIENT_BCAST_SRC_INIT_CFM:
        {
            CapClientBcastSrcInitCfm* capCfm = (CapClientBcastSrcInitCfm*)msg;

            if(capCfm->result == CAP_CLIENT_RESULT_SUCCESS && pbpInst->bcastSrcHandle == 0xFFFFu)
            {
                pbpInst->bcastSrcHandle = capCfm->bcastSrcProfileHandle;
            }

            pbpSendBroadcastSrcInitCfm(pbpInst, capCfm);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_DEINIT_CFM:
        {
            CapClientBcastSrcDeinitCfm* capCfm = (CapClientBcastSrcDeinitCfm*)msg;

            pbpSendBroadcastSrcDeinitCfm(pbpInst, capCfm);
        }
        break;

        case CAP_CLIENT_SET_PARAM_CFM:
        {
            CapClientSetParamCfm *capCfm = (CapClientSetParamCfm *)msg;

            pbpSendBroadcastSrcSetParamCfm(pbpInst, capCfm);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_CONFIG_CFM:
        {
            CapClientBcastSrcConfigCfm* capCfm = (CapClientBcastSrcConfigCfm*)msg;

            if (capCfm->result != CAP_CLIENT_RESULT_SUCCESS)
            {
                if (pbpInst)
                {
                    pbpInst->numSubGroup = 0;
                }
                else
                {
                    PBP_PANIC("CAP_CLIENT_BCAST_SRC_CONFIG_CFM: PBP NULL instance!\n");
                }
            }

            pbpSendBroadcastSrcConfigCfm(pbpInst, (CapClientBcastSrcConfigCfm*)msg);
        }
        break;


        case CAP_CLIENT_BCAST_SRC_START_STREAM_CFM:
        {
            pbpSendBroadcastSrcStartStreamCfm(pbpInst, (CapClientBcastSrcStartStreamCfm*)msg);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_UPDATE_STREAM_CFM:
        {
            pbpSendBroadcastSrcUpdateAudioCfm(pbpInst, (CapClientBcastSrcUpdateStreamCfm*) msg);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_STOP_STREAM_CFM:
        {
            pbpSendBroadcastSrcStopStreamCfm(pbpInst, (CapClientBcastSrcStopStreamCfm*) msg);
        }
        break;

        case CAP_CLIENT_BCAST_SRC_REMOVE_STREAM_CFM:
        {
            pbpSendBroadcastSrcRemoveStreamCfm(pbpInst, (CapClientBcastSrcRemoveStreamCfm*) msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_START_SRC_SCAN_CFM:
        {
            pbpSendBroadcastAssistantScanSrcStartCfm(pbpInst, (CapClientBcastAsstStartSrcScanCfm*) msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_STOP_SRC_SCAN_CFM:
        {
            pbpSendBroadcastAssistantScanSrcStopCfm(pbpInst, (CapClientBcastAsstStopSrcScanCfm*) msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_START_SYNC_TO_SRC_CFM:
        {
            pbpSendBroadcastAssistantStartSyncToSrcCfm(pbpInst, (CapClientBcastAsstSyncToSrcStartCfm *)msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM:
        {
            pbpSendBroadcastAssistantTerminateSyncToSrcCfm(pbpInst, (CapClientBcastAsstSyncToSrcTerminateCfm *)msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_CANCEL_SYNC_TO_SRC_CFM:
        {
            pbpSendBroadcastAssistantSyncToSrcCancelCfm(pbpInst, (CapClientBcastAsstSyncToSrcCancelCfm *)msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM:
        {
            pbpSendBroadcastAssistantAddSrcCfm(pbpInst, (CapClientBcastAsstAddSrcCfm *)msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_MODIFY_SRC_CFM:
        {
            pbpSendBroadcastAssistantModifySrcCfm(pbpInst, (CapClientBcastAsstModifySrcCfm*)msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_REMOVE_SRC_CFM:
        {
            pbpSendBroadcastAssistantRemoveSrcCfm(pbpInst, (CapClientBcastAsstRemoveCfm*)msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_REGISTER_NOTIFICATION_CFM:
        {
            pbpSendBroadcastAssistantRegForNotificationCfm(pbpInst, (CapClientBcastAsstNotficationCfm*)msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_CFM:
        {
            pbpSendBroadcastAssistantReadReceiveStateCfm(pbpInst, (CapClientBcastAsstReadReceiveStateCfm*)msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_SRC_REPORT_IND:
        {
            CapClientBcastAsstSrcReportInd* capInd = (CapClientBcastAsstSrcReportInd*) msg;
            AudioAnnouncementParserStatus status = AUDIO_ANNOUNCEMENT_PARSER_STATUS_NOT_FOUND;
            AudioAnnouncementParserPbpDataType pbpFeatures = { 0 };

            if (!capInd->collocated)
            {
               status =  AudioAnnouncementParserPublicBcastAudioAnnouncementParsing(capInd->serviceDataLen, capInd->serviceData, &pbpFeatures);
            }
            /*Both extended and Periodic reports are received in the form of CAP_CLIENT_BCAST_ASST_SRC_REPORT_IND 
              PBP will drop the EA reports without PBP Announcement but should allow all PA reports.
              
              This is achieved by looking at the subgroupInfo. Subgroup info will be null for EA reports*/
            if (capInd->collocated || (status == AUDIO_ANNOUNCEMENT_PARSER_STATUS_SUCCESS || capInd->subgroupInfo != NULL)) 
            {
                pbpSendBroadcastAssistantSrcReportInd(pbpInst, (CapClientBcastAsstSrcReportInd*)msg, pbpFeatures);
            }
            else
            {
                if (capInd->bigName && capInd->bigNameLen)
                {
                    CsrPmemFree(capInd->bigName);
                }
                PBP_WARNING("PBP dropping report status:[0x%x] collocated:[0x%x]\n", status, capInd->collocated);
            }

            CsrPmemFree(capInd->serviceData);
            capInd->serviceData = NULL;
        }
        break;

        case CAP_CLIENT_BCAST_ASST_BRS_IND:
        {
            pbpSendBroadcastAssistantBrsInd(pbpInst, (CapClientBcastAsstBrsInd*)msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_SYNC_LOSS_IND:
        {
            pbpSendBroadcastAssistantSyncLossInd(pbpInst, (CapClientBcastAsstSyncLossInd *)msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_SET_CODE_IND:
        {
            pbpSendBroadcastAssistantSetCodeInd(pbpInst, (CapClientBcastAsstSetCodeInd *)msg);
        }
        break;

        case CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_IND:
        {
            pbpSendBroadcastAssistantReadReceiveStateInd(pbpInst, (CapClientBcastAsstReadReceiveStateInd *)msg);
        }
        break;

        case CAP_CLIENT_REGISTER_TASK_CFM:
        {
            pbpSendtRegisterCapCfm(pbpInst, (CapClientRegisterTaskCfm *)msg);
        }
        break;

        case CAP_CLIENT_DEREGISTER_TASK_CFM:
        {
            pbpSendtDeRegisterCapCfm(pbpInst, (CapClientDeRegisterTaskCfm*)msg);
        }
        break;

        default:
        {
            /* Unrecognised CAP Client message */
            PBP_WARNING("Cap Client Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}

/***************************************************************************/
static void  pbpHandleInternalMessage(PbpMainInst* inst, void* msg)
{
    CSR_UNUSED(inst);
    CSR_UNUSED(msg);
}

/****************************************************************************/
void pbpMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *msg = NULL;
    PbpMainInst *inst = (PbpMainInst * )*gash;

    if (inst == NULL)
        return;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case PBP_PRIM:
                pbpHandleInternalMessage(inst, msg);
            break;
            case CAP_CLIENT_PRIM:
                pbpHandleCapClientMsg(inst, msg);
            break;
            default:
                PBP_WARNING("Profile Msg not handled \n");
        }
        SynergyMessageFree(eventClass, msg);
    }
}