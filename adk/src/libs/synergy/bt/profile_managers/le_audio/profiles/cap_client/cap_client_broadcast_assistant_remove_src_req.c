/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_private.h"
#include "cap_client_util.h"
#include "cap_client_common.h"
#include "cap_client_ase.h"
#include "cap_client_csip_handler.h"
#include "cap_client_debug.h"
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
#include "cap_client_broadcast_assistant_add_modify_src_req.h"
#include "cap_client_broadcast_assistant_remove_src_req.h"

extern CapClientBroadcastSrcParams sourceParams;

void capClientHandleBroadcastAssistantRemoveSrcCfm(CAP_INST *inst,
                                                Msg msg,
                                                CapClientGroupInstance* cap)
{
    CapClientResult result = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;
    BapInstElement *bap = NULL;
    BapBroadcastAssistantRemoveSrcCfm  *cfm =
                                    (BapBroadcastAssistantRemoveSrcCfm*)msg;

    if(inst == NULL)
    {
        CAP_CLIENT_ERROR("\n capHandleBroadcastAssistantRemoveSrcCfm: NULL instance \n");
        return;
    }

    if (capClientBcastAsstOpComplete(cap))
    {
        /* if all the counters are clear then there is no need to proceed, just return*/
        CAP_CLIENT_INFO("\n capClientHandleBroadcastAssistantAddModifySrcCfm: Counters are Reset! \n");
        return;
    }

    bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);
    CAP_CLIENT_CLEAR_PENDING_OP(cap->pendingOp);

    if(bap == NULL)
    {
        CAP_CLIENT_INFO("\n capHandleBroadcastAssistantRemoveSrcCfm: BAP NULL instance \n");
        capClientBcastAsstResetState(cap, CAP_CLIENT_BCAST_ASST_STATE_IDLE);
        capClientSendBcastCommonCfmMsgSend(inst->profileTask,
                                           inst,
                                           NULL,
                                           0,
                                           result,
                                           CAP_CLIENT_BCAST_ASST_REMOVE_SRC_CFM);
        return;
    }
    result = capClientBroadcastSourceGetResultCode(cfm->result);

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

    if (capClientBcastAsstOpComplete(cap))
    {
        capClientBcastAsstResetState(cap, CAP_CLIENT_BCAST_ASST_STATE_IDLE);
        capClientSendSelectBcastAsstCommonCfmMsgSend(inst->profileTask,
                                                     inst,
                                                     cap,
                                                     sourceParams.infoCount,
                                                     sourceParams.info,
                                                     result,
                                                     CAP_CLIENT_BCAST_ASST_REMOVE_SRC_CFM);
    }
}

void capClientBcastAsstRemoveSrcReqSend(BapInstElement* bap,
                                       uint32 cid,
                                       AppTask appTask)
{
    uint8 i = 0;

    CapClientGroupInstance* cap =
        (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(bap->groupId);
    bap = NULL;
    CSR_UNUSED(cid);

    for (i = 0; cap && (i < sourceParams.infoCount); i++)
    {

        bap = CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, sourceParams.info[i].cid);

        if (bap)
        {
            capClientIncrementOpCounter(&bap->operationCounter);

            CAP_CLIENT_INFO("\n capClientBcastAsstRemoveSrcReqSend: Counter :%d \n", bap->operationCounter.opReqCount);

            BapBroadcastAssistantRemoveSrcReq(sourceParams.info[i].cid,
                                              sourceParams.info[i].sourceId);

            if (bap->bass)
                bap->bass->reportToTask = appTask;
        }
        else
        {
            CAP_CLIENT_INFO("\n capClientBcastAsstRemoveSrcReqSend: No BAP Instance \n");
        }
    }
}

void handleBroadcastAssistantRemoveSrcReq(CAP_INST* inst, const Msg msg)
{
    uint8 i ;
    BapInstElement *bap = NULL;
    uint16 result = CAP_CLIENT_RESULT_INVALID_OPERATION;
    CapClientGroupInstance *cap = NULL;
    CapClientInternalBcastAsstRemoveSrcReq *req =
                  (CapClientInternalBcastAsstRemoveSrcReq*)msg;
    AppTask appTask = req->profileTask;

    /*
     * TODO:if groupId is not same Switch the group return
     * by sending error Response
     * */

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    result = capClientBroadcastAssistantValidOperation(req->groupId, req->profileTask, inst, cap);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantRemoveSrcReq: result =%x \n", result);
        capClientSendBcastCommonCfmMsgSend(appTask, inst, NULL, 0, result, CAP_CLIENT_BCAST_ASST_REMOVE_SRC_CFM);
        return;
    }

    if (cap == NULL)
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantRemoveSrcReq: NULL instance \n");
        return;
    }

    if (capClientBcastAsistantGetState(cap) != CAP_CLIENT_BCAST_ASST_STATE_IDLE)
    {
        result = CAP_CLIENT_RESULT_CAP_BUSY;
        CAP_CLIENT_INFO("\n handleBroadcastAssistantRemoveSrcReq: Invalid State\n");
        capClientSendBcastCommonCfmMsgSend(appTask, inst, NULL, 0, result, CAP_CLIENT_BCAST_ASST_REMOVE_SRC_CFM);
        return;
    }

    /* co ordinated set?
     *
     * Based on if co ordinated Set or not decide number of ASEs required
     * and then start BAP procedures
     *
     * */
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
                                              CAP_CLIENT_BCAST_ASST_REMOVE_SRC_CFM);

            CAP_CLIENT_ERROR("\n handleBroadcastAssistantRemoveSrcReq: Invalid BAP Handle \n");
            return;
        }
    }

    /*Free the sourceParam  && metadata Value from previous call*/
    capClientFreeSourceParamsContent();

    sourceParams.infoCount = req->infoCount;
    sourceParams.info = req->info;
    req->info = NULL;

    cap->pendingOp = CAP_CLIENT_BAP_BASS_REMOVE_SRC;
    capClientBcastAsistantSetState(cap, CAP_CLIENT_BCAST_ASST_STATE_REMOVING_SOURCE);
    capClientSendBapBcastAsstReq(cap, inst, capClientBcastAsstRemoveSrcReqSend);
}

#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */
