/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "cap_client_util.h"
#include "cap_client_common.h"
#include "cap_client_ase.h"
#include "cap_client_csip_handler.h"
#include "cap_client_debug.h"
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
#include "cap_client_broadcast_assistant_add_modify_src_req.h"

CapClientBroadcastSrcParams sourceParams;

void capClientSendBroadcastAssistantCommCfm(AppTask profileTask,
                                      CapClientResult result,
                                      ServiceHandle groupId,
                                      BapProfileHandle cid,
                                      CapClientPrim type)
{

    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastAsstCommonCfm);
    message->groupId = groupId;
    message->result = result;

    if(result == CAP_CLIENT_RESULT_SUCCESS)
    {
        message->cid = cid;
    }

    CapClientMessageSend(profileTask, type, message);
}


/**********************************************************************************************************/
void capClientHandleBroadcastAssistantAddModifySrcCfm(CAP_INST *inst,
                                Msg msg,
                                CapClientGroupInstance* cap,
                                CapClientPrim type)
{
    CapClientResult result = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;
    BapInstElement *bap = NULL;
    BapBroadcastAssistantAddSrcCfm  *addSrcCfm = NULL;
    BapBroadcastAssistantModifySrcCfm *modifySrcCfm = NULL;

    if(inst == NULL)
    {
        CAP_CLIENT_ERROR("\n capClientHandleBroadcastAssistantAddModifySrcCfm: NULL instance \n");
        return;
    }


    if (capClientBcastAsstOpComplete(cap))
    {
        /* if all the counters are clear then there is no need to proceed, just return*/
        CAP_CLIENT_INFO("\n capClientHandleBroadcastAssistantAddModifySrcCfm: Counters are Reset! \n");
        return;
    }

    if(type == CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM)
    {
        addSrcCfm = (BapBroadcastAssistantAddSrcCfm*)msg;
        result = capClientBroadcastSourceGetResultCode(addSrcCfm->result);
        bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, addSrcCfm->handle);

    }
    else
    {
        modifySrcCfm = (BapBroadcastAssistantModifySrcCfm*)msg;
        result = capClientBroadcastSourceGetResultCode(modifySrcCfm->result);
        bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, modifySrcCfm->handle);
    }

    if(bap == NULL)
    {
        CAP_CLIENT_ERROR("\n capHandleBroadcastAssistantSyncToSrcStartCfm: BAP NULL instance \n");
        result = CAP_CLIENT_RESULT_NULL_INSTANCE;
        capClientBcastAsstResetState(cap, CAP_CLIENT_BCAST_ASST_STATE_IDLE);
        capClientSendBcastCommonCfmMsgSend(inst->profileTask,
                                          inst,
                                          NULL,
                                          0,
                                          result,
                                          type);
        return;
    }

    bap->recentStatus = result;

    if (bap->recentStatus == CAP_CLIENT_RESULT_INPROGRESS
        || bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
    {
        /* Do Nothing */
        capClientIncrementSuccessCounter(&bap->operationCounter);
    }
    else
    {
        capClientIncrementErrorCounter(&bap->operationCounter);
    }

    if(capClientBcastAsstOpComplete(cap))
    {
        capClientBcastAsstResetState(cap, CAP_CLIENT_BCAST_ASST_STATE_IDLE);

        if (type == CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM)
        {
            capClientSendBcastCommonCfmMsgSend(inst->profileTask,
                                               inst,
                                               cap,
                                               cap->requestCid,
                                               result,
                                               CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM);
        }
        else
        {
            capClientSendSelectBcastAsstCommonCfmMsgSend(inst->profileTask,
                                                        inst,
                                                        cap,
                                                        sourceParams.infoCount,
                                                        sourceParams.info,
                                                        result,
                                                        CAP_CLIENT_BCAST_ASST_MODIFY_SRC_CFM);
        }
    }
}

void capClientBcastAsstAddSrcReqSend(BapInstElement* bap,
                                    uint32 cid,
                                    AppTask appTask)
{

    do {

        capClientIncrementOpCounter(&bap->operationCounter);

        CAP_CLIENT_INFO("\n capClientBcastAsstAddSrcReqSend: Counter :%d \n",
                                        bap->operationCounter.opReqCount);

        BapBroadcastAssistantAddSrcReq(bap->bapHandle,
                                       &sourceParams.sourceAddrt,
                                       sourceParams.advertiserAddressType,
                                       sourceParams.srcCollocated,
                                       sourceParams.syncHandle,
                                       sourceParams.sourceAdvSid,
                                       sourceParams.paSyncState,
                                       sourceParams.paInterval,
                                       sourceParams.broadcastId,
                                       sourceParams.numbSubGroups,
                                       sourceParams.subgroupInfo);

        if (bap->bass)
            bap->bass->reportToTask = appTask;

        bap = bap->next;
    } while (bap && (cid == 0));
}


void handleBroadcastAssistantAddSrcReq(CAP_INST* inst, const Msg msg)
{
    uint8 i;
    BapInstElement *bap = NULL;
    uint16 result = CAP_CLIENT_RESULT_INVALID_OPERATION;
    CapClientGroupInstance *cap = NULL;
    CapClientInternalBcastAsstAddSrcReq *req =
                  (CapClientInternalBcastAsstAddSrcReq*)msg;
    AppTask appTask = req->profileTask;

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    result = capClientBroadcastAssistantValidOperation(req->groupId, req->profileTask, inst, cap);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantAddSrcReq: result =%x \n", result);
        capClientSendBcastCommonCfmMsgSend(appTask, inst, NULL, 0, result, CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM);
        return;
    }

    if (cap == NULL)
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantAddSrcReq: NULL instance \n");
        return;
    }

    if (capClientBcastAsistantGetState(cap) != CAP_CLIENT_BCAST_ASST_STATE_IDLE)
    {
        result = CAP_CLIENT_RESULT_CAP_BUSY;
        CAP_CLIENT_INFO("\n handleBroadcastAssistantAddSrcReq: Invalid State\n");
        capClientSendBcastCommonCfmMsgSend(appTask, inst, NULL, 0, result, CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM);
        return;
    }

    /* co ordinated set?
     *
     * Based on if co ordinated Set or not decide number of ASEs required
     * and then start BAP procedures
     *
     * */
    cap->requestCid = req->cid;

    if (req->cid)
    {
        bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, req->cid);

        if (bap == NULL)
        {
            result = CAP_CLIENT_RESULT_INVALID_PARAMETER;
            CAP_CLIENT_INFO("\n handleBroadcastAssistantAddSrcReq: UKNOWN CID \n");
            capClientSendBcastCommonCfmMsgSend(appTask, inst, NULL, 0, result, CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM);
            return;
        }
    }

    /* Copy the Parameters*/

    capClientFreeSourceParamsContent();

    sourceParams.advertiserAddressType = req->advertiserAddressType;
    sourceParams.sourceAddrt.lap = req->sourceAddrt.lap;
    sourceParams.sourceAddrt.uap = req->sourceAddrt.uap;
    sourceParams.sourceAddrt.nap = req->sourceAddrt.nap;
    sourceParams.broadcastId = req->broadcastId;
    sourceParams.numbSubGroups = req->numbSubGroups;
    sourceParams.paInterval = req->paInterval;
    sourceParams.paSyncState = req->paSyncState;
    sourceParams.sourceAdvSid = req->sourceAdvSid;
    sourceParams.syncHandle = req->syncHandle;
    sourceParams.srcCollocated = req->srcCollocated;
    inst->profileTask = req->profileTask;
    /*Free the sourceParam  && metadata Value from previous call*/

    for(i = 0; i < req->numbSubGroups; i++)
    {
        sourceParams.subgroupInfo[i].bisSyncState = req->subgroupInfo[i]->bisIndex;
        sourceParams.subgroupInfo[i].metadataValue = NULL;
        sourceParams.subgroupInfo[i].metadataLen = 0;

        if (req->subgroupInfo[i]->metadataLen && req->subgroupInfo[i]->metadataValue)
        {

            sourceParams.subgroupInfo[i].metadataLen = req->subgroupInfo[i]->metadataLen;
            sourceParams.subgroupInfo[i].metadataValue = (uint8*)
                                  CsrPmemZalloc(sourceParams.subgroupInfo[i].metadataLen * sizeof(uint8));

            SynMemCpyS(sourceParams.subgroupInfo[i].metadataValue,
                      sourceParams.subgroupInfo[i].metadataLen, 
                         req->subgroupInfo[i]->metadataValue, 
                              sourceParams.subgroupInfo[i].metadataLen);
        }
    }

    capClientBcastAsistantSetState(cap, CAP_CLIENT_BCAST_ASST_STATE_ADDING_SOURCE);
    cap->pendingOp = CAP_CLIENT_BAP_BASS_ADD_SRC;
    capClientSendBapBcastAsstReq(cap, inst, capClientBcastAsstAddSrcReqSend);
}
/************************************************************************Modify Source****************************************/

void capClientBcastAsstmodifySrcReqSend(BapInstElement* bap,
                                    uint32 cid,
                                    AppTask appTask)
{
    uint8 i = 0;

    CapClientGroupInstance* cap =
        (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(bap->groupId);
    bap = NULL;
    CSR_UNUSED(cid);

    for(i = 0; cap && (i < sourceParams.infoCount); i++)
    {

        bap = CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, sourceParams.info[i].cid);

        if (bap)
        {
            capClientIncrementOpCounter(&bap->operationCounter);

            CAP_CLIENT_INFO("\n capClientBcastAsstmodifySrcReqSend: Counter :%d \n", bap->operationCounter.opReqCount);

            BapBroadcastAssistantModifySrcReq(sourceParams.info[i].cid,
                                            sourceParams.info[i].sourceId,
                                            sourceParams.srcCollocated,
                                            sourceParams.syncHandle,
                                            sourceParams.sourceAdvSid,
                                            sourceParams.paSyncState,
                                            sourceParams.paInterval,
                                            sourceParams.numbSubGroups,
                                            sourceParams.subgroupInfo);

            if (bap->bass)
                bap->bass->reportToTask = appTask;
        }
        else
        {
            CAP_CLIENT_INFO("\n capClientBcastAsstmodifySrcReqSend: No BAP Instance \n");
        }
    }
}

void handleBroadcastAssistantModifySrcReq(CAP_INST* inst, const Msg msg)
{
    uint8 i;
    BapInstElement *bap = NULL;
    uint16 result = CAP_CLIENT_RESULT_INVALID_OPERATION;
    CapClientGroupInstance *cap = NULL;
    CapClientInternalBcastAsstModifySrcReq *req =
                  (CapClientInternalBcastAsstModifySrcReq*)msg;
    AppTask appTask = req->profileTask;

    /*
     * TODO:if groupId is not same Switch the group return
     * by sending error Response
     * */

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    result = capClientBroadcastAssistantValidOperation(req->groupId, req->profileTask, inst, cap);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantModifySrcReq: result =%x \n", result);
        capClientSendBcastCommonCfmMsgSend(appTask, inst, NULL, 0, result, CAP_CLIENT_BCAST_ASST_MODIFY_SRC_CFM);
        return;
    }

    if (cap == NULL)
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantModifySrcReq: NULL instance \n");
        return;
    }

    if (capClientBcastAsistantGetState(cap) != CAP_CLIENT_BCAST_ASST_STATE_IDLE)
    {
        result = CAP_CLIENT_RESULT_CAP_BUSY;
        CAP_CLIENT_INFO("\n handleBroadcastAssistantModifySrcReq: Invalid State\n");
        capClientSendBcastCommonCfmMsgSend(appTask, inst, NULL, 0, result, CAP_CLIENT_BCAST_ASST_MODIFY_SRC_CFM);
        return;
    }


    if (req->infoCount && req->info)
        cap->requestCid = req->info[0].cid;

    /* Check if Parameters are valid */
    for (i = 0;(i < req->infoCount) && req->info;i++)
    {
        bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, req->info[i].cid);

        if (bap == NULL)
        {
            result = CAP_CLIENT_RESULT_INVALID_PARAMETER;
            capClientSendBcastCommonCfmMsgSend(appTask,
                                               inst,
                                               NULL,
                                               0,
                                               result,
                                               CAP_CLIENT_BCAST_ASST_MODIFY_SRC_CFM);

            CAP_CLIENT_ERROR("\n handleBroadcastAssistantModifySrcReq: Invalid BAP Handle \n");
            return;
        }
    }

    /*Free the sourceParam  && metadata Value from previous call*/
    capClientFreeSourceParamsContent();

    /* Copy the Parameters*/
    sourceParams.numbSubGroups = req->numbSubGroups;
    sourceParams.paInterval = req->paInterval;
    sourceParams.paSyncState = req->paSyncState;
    sourceParams.sourceAdvSid = req->sourceAdvSid;
    sourceParams.syncHandle = req->syncHandle;
    sourceParams.srcCollocated = req->srcCollocated;
    inst->profileTask = req->profileTask;

    sourceParams.infoCount = req->infoCount;
    sourceParams.info = req->info;
    req->info = NULL;

    for (i = 0; i < req->numbSubGroups; i++)
    {
        sourceParams.subgroupInfo[i].bisSyncState = req->subgroupInfo[i]->bisIndex;
        sourceParams.subgroupInfo[i].metadataValue = NULL;
        sourceParams.subgroupInfo[i].metadataLen = 0;

        if (req->subgroupInfo[i]->metadataLen && req->subgroupInfo[i]->metadataValue)
        {

            sourceParams.subgroupInfo[i].metadataLen = req->subgroupInfo[i]->metadataLen;
            sourceParams.subgroupInfo[i].metadataValue = (uint8*)
                              CsrPmemZalloc(sourceParams.subgroupInfo[i].metadataLen * sizeof(uint8));

            SynMemCpyS(sourceParams.subgroupInfo[i].metadataValue,
                            sourceParams.subgroupInfo[i].metadataLen,
                                    req->subgroupInfo[i]->metadataValue,
                                        sourceParams.subgroupInfo[i].metadataLen);
        }
    }
    cap->pendingOp = CAP_CLIENT_BAP_BASS_MODIFY_SRC;
    capClientBcastAsistantSetState(cap, CAP_CLIENT_BCAST_ASST_STATE_MODIFYING_SOURCE);
    capClientSendBapBcastAsstReq(cap, inst, capClientBcastAsstmodifySrcReqSend);
}

/*******************************************Handle Broadcast Receive State Indication**********************************/

void capClientHandleBassBrsInd(CAP_INST* inst,
                             BapBroadcastAssistantBrsInd *ind,
                             CapClientGroupInstance* cap)
{
    uint8 i = 0;
    BapInstElement* bap = NULL;
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastAsstBrsInd);
    message->advertiseAddType = ind->advertiseAddType;
    message->advSid = ind->advSid;
    message->bigEncryption = ind->bigEncryption;
    message->broadcastId = ind->broadcastId;
    message->groupId = inst->activeGroupId;
    message->cid = ind->handle;
    message->numSubGroups = ind->numSubGroups;
    message->sourceId = ind->sourceId;
    message->sourceAddress.lap = ind->sourceAddress.lap;
    message->sourceAddress.uap = ind->sourceAddress.uap;
    message->sourceAddress.nap = ind->sourceAddress.nap;
    message->paSyncState = ind->paSyncState;
    message->badCode = NULL;
    message->numSubGroups = 0;
    message->subGroupInfo = NULL;
    if (ind->badCode)
    {
        message->badCode = ind->badCode;
        ind->badCode = NULL;
    }

    if (ind->numSubGroups)
    {
        message->numSubGroups = ind->numSubGroups;
        message->subGroupInfo = (CapClientSubgroupInfo*)
                        CsrPmemZalloc(ind->numSubGroups * sizeof(CapClientSubgroupInfo));
    
        for (i = 0; i < ind->numSubGroups; i++)
        {

            CAP_CLIENT_INFO("(CAP)capClientHandleBassBrsInd: BisSyncState: %d\n", ind->subGroupInfo[i].bisSyncState);
            CAP_CLIENT_INFO("(CAP)capClientHandleBassBrsInd: metadtaLen: %d\n", ind->subGroupInfo[i].metadataLen);

            message->subGroupInfo[i].bisIndex = ind->subGroupInfo[i].bisSyncState;
            if (ind->subGroupInfo[i].metadataLen && ind->subGroupInfo[i].metadataValue)
            {

                message->subGroupInfo[i].metadataLen = ind->subGroupInfo[i].metadataLen;
                message->subGroupInfo[i].metadataValue =
                          CsrPmemZalloc(message->subGroupInfo[i].metadataLen * sizeof(uint8));
                SynMemCpyS(message->subGroupInfo[i].metadataValue,
                                         message->subGroupInfo[i].metadataLen,
                                           ind->subGroupInfo[i].metadataValue,
                                                 message->subGroupInfo[i].metadataLen);

               CsrPmemFree(ind->subGroupInfo[i].metadataValue);
            }
        }
        CsrPmemFree(ind->subGroupInfo);
    }
	

    if (ind->handle)
        bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, ind->handle);

    if(bap && bap->bass)
        CapClientMessageSend(bap->bass->reportToTask, CAP_CLIENT_BCAST_ASST_BRS_IND, message);
}

/**************************************************************Sync Loss************************************************/

void capClientHandleBassSyncLossInd(CAP_INST* inst, BapBroadcastAssistantSyncLossInd* ind)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastAsstSyncLossInd);
    message->groupId = inst->activeGroupId;
    message->cid = ind->handle;
    message->syncHandle = ind->syncHandle;

    CapClientMessageSend(inst->profileTask, CAP_CLIENT_BCAST_ASST_SYNC_LOSS_IND, message);
}

/***************************************************Send Broadcast Code **************************************************/

void handleBroadcastAssistantSetBroadcastCodeRsp(CAP_INST* inst, const Msg msg)
{
    CsipInstElement* csip = NULL;
    BapInstElement* bap = NULL;
    CapClientGroupInstance* cap = NULL;
    CapClientInternalBcastAsstSetCodeRsp* req =
                      (CapClientInternalBcastAsstSetCodeRsp*)msg;

    /*
     * TODO:if groupId is not same Switch the group return
     * by sending error Response
     * */

    if (req->groupId != inst->activeGroupId)
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantSetBroadcastCodeRsp: Unable to change GroupId \n");
        return;
    }

    if (req->broadcastCode == NULL)
    {
        CAP_CLIENT_ERROR("\n handleBroadcastAssistantSetBroadcastCodeRsp: broadcastCode NULL\n");
        return;
    }

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (cap == NULL)
    {
        CAP_CLIENT_ERROR("\n handleBroadcastAssistantSetBroadcastCodeRsp: CAP NULL instance \n");
        return;
    }

    bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, req->cid);
    cap->requestCid = req->cid;

    if (bap == NULL)
    {
        CAP_CLIENT_ERROR("\n handleBroadcastAssistantSetBroadcastCodeRsp: Bap NULL instance \n");
        return;
    }

    bap->bass->sourceId = req->sourceId;

    cap->broadcastCode = req->broadcastCode;
    
    if (capClientIsGroupCoordinatedSet(cap))
    {
        csip = (CsipInstElement*)CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(cap->csipList, req->cid);

        /* Here we need to obtain lock on all the devices and the
         * Start BAP unicast Procedures */

         /* check if the profile is already locked
          *
          * Note: If one device is in lock state in a group
          * it's assumed that all other participants are in lock state*/
        cap->pendingOp = CAP_CLIENT_BAP_BASS_SET_CODE;
        if ((csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED))
        {
            BapBroadcastAssistantSetCodeRsp(cap->requestCid, bap->bass->sourceId, cap->broadcastCode);
            CAP_CLIENT_CLEAR_PENDING_OP(cap->pendingOp);
        }
        /* Otherwise obtain lock and the start BAP Procedures */
        else
        {
            capClientSetCsisLockState(csip, &inst->csipRequestCount, TRUE);
        }
    }
    else
    {
        BapBroadcastAssistantSetCodeRsp(cap->requestCid, bap->bass->sourceId, cap->broadcastCode);
    }
}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */
