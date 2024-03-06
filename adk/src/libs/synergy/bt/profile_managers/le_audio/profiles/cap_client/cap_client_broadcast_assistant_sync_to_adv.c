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
#include "cap_client_broadcast_assistant_sync_to_adv.h"

static void capClientSendBroadcastAssistantSyncToSrcTerminateCfm(AppTask profileTask,
                                                           CapClientResult result,
                                                           ServiceHandle groupId,
                                                           uint16 syncHandle)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastAsstSyncToSrcTerminateCfm);
    message->groupId = groupId;
    message->result = result;

    if(result == CAP_CLIENT_RESULT_SUCCESS)
        message->syncHandle = syncHandle;

    CapClientMessageSend(profileTask, CAP_CLIENT_BCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM, message);
}

/**************************************************************SYNC TO SOURCE START************************/
void capClientSendBapBroadcastAssistantStartSynctoSrcReq(BapInstElement *bap)
{
    BapBroadcastAssistantSyncToSrcStartReq(bap->bapHandle, &bap->bass->addrt, bap->bass->advSid);
}

static void capClientSendBroadcastAssistantSyncToSrcCfm(AppTask profileTask,
                                               ServiceHandle groupId,
                                               CapClientResult result,
                                               BapBroadcastAssistantSyncToSrcStartCfm *cfm)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastAsstSyncToSrcStartCfm);
    message->groupId = groupId;
    message->result = result;

    if(result == CAP_CLIENT_RESULT_SUCCESS && cfm)
    {
        tbdaddr_copy(&message->addrt, &cfm->addrt);
        message->advClockAccuracy = cfm->advClockAccuracy;
        message->advPhy = cfm->advPhy;
        message->advSid = cfm->advSid;
        message->periodicAdvInterval = cfm->periodicAdvInterval;
        message->syncHandle = cfm->syncHandle;
    }

    CapClientMessageSend(profileTask, CAP_CLIENT_BCAST_ASST_START_SYNC_TO_SRC_CFM, message);

}

void capClientHandleBroadcastAssistantSyncToSrcStartCfm(CAP_INST *inst,
                                BapBroadcastAssistantSyncToSrcStartCfm *cfm,
                                CapClientGroupInstance* cap)
{
    CapClientResult result;

    if(inst == NULL)
    {
        CAP_CLIENT_ERROR("\n capHandleBroadcastAssistantSyncToSrcStartCfm: NULL instance \n");
        return;
    }

    inst->bapRequestCount--;

    result = capClientBroadcastSourceGetResultCode(cfm->result);

    if(capClientBcastAsistantGetState(cap) == CAP_CLIENT_BCAST_ASST_STATE_SYNC_TO_SRC)
    {
        if (result != CAP_CLIENT_RESULT_INPROGRESS)
        {
            /* Clear the operation when it is success or when result is invalid */
            CAP_CLIENT_CLEAR_PENDING_OP(cap->pendingOp);

            capClientBcastAsistantSetState(cap, CAP_CLIENT_BCAST_ASST_STATE_IDLE);
        }

        capClientSendBroadcastAssistantSyncToSrcCfm(inst->profileTask,
                                             inst->activeGroupId,
                                             result,
                                             cfm);
    }
}

void handleBroadcastAssistantSyncToSrcStartReq(CAP_INST* inst, const Msg msg)
{
    BapInstElement *bap;
    CapClientResult result;
    CapClientGroupInstance *cap = NULL;
    CapClientInternalBcastAsstSyncToSrcStartReq *req =
                   (CapClientInternalBcastAsstSyncToSrcStartReq*)msg;

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    result = capClientBroadcastAssistantValidOperation(req->groupId, req->profileTask, inst, cap);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantSyncToSrcStartReq: result =%x \n", result);
        capClientSendBroadcastAssistantSyncToSrcCfm(inst->profileTask, inst->activeGroupId, result, NULL);
        return;
    }

    if (cap && !(capClientBcastAsistantGetState(cap) == CAP_CLIENT_BCAST_ASST_STATE_IDLE))
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantSyncToSrcStartReq: bcastState =%x \n", cap->bcastAsstState);
        capClientSendBroadcastAssistantSyncToSrcCfm(inst->profileTask, 
                                                    inst->activeGroupId, 
                                                    CAP_CLIENT_RESULT_INVALID_BROADCAST_ASSISTANT_STATE, 
                                                    NULL);
        return;
    }

    bap = (BapInstElement*)cap->bapList.first;

    if(bap == NULL)
    {
        result = CAP_CLIENT_RESULT_NULL_INSTANCE;
        capClientSendBroadcastAssistantSyncToSrcCfm(inst->profileTask, inst->activeGroupId, result, NULL);
        CAP_CLIENT_INFO("\n handleBroadcastAssistantSyncToSrcStartReq: Invalid BAP Handle \n");
        return;
    }

    bap->bass->advSid = req->advSid;
    bap->bass->addrt.type = req->addrt.type;
    bap->bass->addrt.addr.lap = req->addrt.addr.lap;
    bap->bass->addrt.addr.uap = req->addrt.addr.uap;
    bap->bass->addrt.addr.nap = req->addrt.addr.nap;

    cap->pendingOp = CAP_CLIENT_BAP_BASS_SYNC_START;
    capClientBcastAsistantSetState(cap, CAP_CLIENT_BCAST_ASST_STATE_SYNC_TO_SRC);
    inst->bapRequestCount++;
    capClientSendBapBroadcastAssistantStartSynctoSrcReq(bap);
}


/******************************************************SYNC TO SOURCE CANCEL************************/
static void capClientSendBroadcastAssistantSyncToSrcCancelCfm(AppTask profileTask,
                                                           CapClientResult result,
                                                           ServiceHandle groupId)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastAsstSyncToSrcTerminateCfm);
    message->groupId = groupId;
    message->result = result;

    CapClientMessageSend(profileTask, CAP_CLIENT_BCAST_ASST_CANCEL_SYNC_TO_SRC_CFM, message);
}

void capClientHandleBroadcastAssistantSyncToSrcCancelCfm(CAP_INST *inst,
                                BapBroadcastAssistantSyncToSrcCancelCfm *cfm,
                                CapClientGroupInstance* cap)
{
    CapClientResult result;

    if(inst == NULL)
    {
        CAP_CLIENT_ERROR("\n capHandleBroadcastAssistantSyncToSrcCancelCfm: NULL instance \n");
        return;
    }

    inst->bapRequestCount--;

    result = capClientBroadcastSourceGetResultCode(cfm->result);

    if(capClientBcastAsistantGetState(cap) == CAP_CLIENT_BCAST_ASST_STATE_SYNC_TO_SRC)
    {
        CAP_CLIENT_CLEAR_PENDING_OP(cap->pendingOp);

        capClientBcastAsistantSetState(cap, CAP_CLIENT_BCAST_ASST_STATE_IDLE);

        capClientSendBroadcastAssistantSyncToSrcCancelCfm(inst->profileTask,
                                                       result,
                                                       inst->activeGroupId);
    }
}

void capClientSendBapBroadcastAssistantSynctoSrcCancelReq(BapInstElement *bap)
{
    BapBroadcastAssistantSyncToSrcCancelReq(bap->bapHandle);
}

void handleBroadcastAssistantSyncToSrcCancelReq(CAP_INST* inst, const Msg msg)
{
    BapInstElement *bap;
    CapClientResult result;
    CapClientGroupInstance *cap = NULL;
    CapClientInternalBcastAsstSyncToSrcCancelReq *req = (CapClientInternalBcastAsstSyncToSrcCancelReq*)msg;

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    result = capClientBroadcastAssistantValidOperation(req->groupId, req->profileTask, inst, cap);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantSyncToSrcCancelReq: result =%x \n", result);
        capClientSendBroadcastAssistantSyncToSrcCancelCfm(inst->profileTask, result, inst->activeGroupId);
        return;
    }

    if (cap && !(capClientBcastAsistantGetState(cap) == CAP_CLIENT_BCAST_ASST_STATE_IDLE))
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantSyncToSrcStartReq: bcastState =%x \n", cap->bcastAsstState);
        capClientSendBroadcastAssistantSyncToSrcCancelCfm(req->profileTask,
                        CAP_CLIENT_RESULT_INVALID_BROADCAST_ASSISTANT_STATE,
                                   inst->activeGroupId);
        return;
    }

    bap = (BapInstElement*)cap->bapList.first;

    if(bap == NULL)
    {
        result = CAP_CLIENT_RESULT_NULL_INSTANCE;
        CAP_CLIENT_INFO("\n handleBroadcastAssistantSyncToTrainCancelReq: Invalid BAP Handle \n");
        capClientSendBroadcastAssistantSyncToSrcCancelCfm(inst->profileTask, result, inst->activeGroupId);
        return;
    }

    cap->pendingOp = CAP_CLIENT_BAP_BASS_SYNC_CANCEL;
    inst->bapRequestCount++;
    capClientSendBapBroadcastAssistantSynctoSrcCancelReq(bap);
}

/**************************************************************SYNC TO SOURCE TERMINATE*******************/
void capClientHandleBroadcastAssistantSyncToSrcTerminateCfm(CAP_INST *inst,
                                BapBroadcastAssistantSyncToSrcTerminateCfm *cfm,
                                CapClientGroupInstance* cap)
{

    CapClientResult result = capClientBroadcastSourceGetResultCode(cfm->result);

    if(inst == NULL)
    {
        CAP_CLIENT_ERROR("\n capHandleBroadcastAssistantSyncToSrcStartCfm: NULL instance \n");
        /* Do Nothing*/
        return;
    }

    inst->bapRequestCount--;

    CAP_CLIENT_CLEAR_PENDING_OP(cap->pendingOp);
    
    capClientSendBroadcastAssistantSyncToSrcTerminateCfm(inst->profileTask,
                                                    result,
                                                    inst->activeGroupId,
                                                    cfm->syncHandle);
}

void handleBroadcastAssistantTerminateSyncToSrcReq(CAP_INST* inst, const Msg msg)
{
    BapInstElement *bap;
    CapClientResult result;
    CapClientGroupInstance *cap = NULL;
    CapClientInternalBcastAsstTerminateSyncToSrcReq *req =
              (CapClientInternalBcastAsstTerminateSyncToSrcReq*)msg;

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    result = capClientBroadcastAssistantValidOperation(req->groupId, req->profileTask, inst, cap);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantTerminateSyncToSrcReq: result =%x \n", result);
        capClientSendBroadcastAssistantSyncToSrcTerminateCfm(inst->profileTask, result, inst->activeGroupId, 0);
        return;
    }

    /* cap instance can never be NUL here */
    if (cap && !(capClientBcastAsistantGetState(cap) == CAP_CLIENT_BCAST_ASST_STATE_IDLE))
    {
        CAP_CLIENT_INFO("\n handleBroadcastAssistantTerminateSyncToSrcReq: bcastState =%x \n", cap->bcastAsstState);
        capClientSendBroadcastAssistantSyncToSrcTerminateCfm(inst->profileTask,
                           CAP_CLIENT_RESULT_INVALID_BROADCAST_ASSISTANT_STATE,
                                                           inst->activeGroupId,
                                                                            0);
        return;
    }

    bap = (BapInstElement*)cap->bapList.first;

    if(bap == NULL)
    {
        result = CAP_CLIENT_RESULT_INVALID_PARAMETER;

        capClientSendBroadcastAssistantSyncToSrcTerminateCfm(inst->profileTask,
                                                               result,
                                                               inst->activeGroupId,
                                                               0);
        CAP_CLIENT_INFO("\n handleBroadcastAssistantTerminateSyncToSrcReq: Invalid BAP Handle \n");
        return;
    }

    cap->pendingOp = CAP_CLIENT_BAP_BASS_SYNC_TERMINATE;
    inst->bapRequestCount++;

    BapBroadcastAssistantSyncToSrcTerminateReq(bap->bapHandle, req->syncHandle);
}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */
