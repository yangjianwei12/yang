/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_util.h"
#include "cap_client_common.h"
#include "cap_client_vcp_operation_req.h"
#include "cap_client_vcp_handler.h"
#include "cap_client_init_stream_and_control_req.h"
#include "cap_client_remove_device_req.h"
#include "cap_client_csip_handler.h"
#include "cap_client_debug.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

void capCLientSendReadVolumeStateCfm(AppTask appTask, 
                                       ServiceHandle groupId,
                                       CapClientResult result,
                                       VcpInstElement *vcp,
                                       uint8 mute,
                                       uint8 volumeSetting,
                                       uint8 changeCounter)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientReadVolumeStateCfm);
    CapClientGroupInstance* cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(groupId);

    if (cap)
    {
        CAP_CLIENT_CLEAR_PENDING_OP(cap->vcpPendingOp);
    }

    message->groupId = groupId;
    message->result = result;

    message->changeCounter = 0;
    message->volumeSetting = 0;
    message->mute = 0;
    message->cid = 0;

    if (result == CAP_CLIENT_RESULT_SUCCESS && vcp)
    {
        message->changeCounter = changeCounter;
        message->volumeSetting = volumeSetting;
        message->mute = mute;
        message->cid = vcp->cid;
    }

    CapClientMessageSend(appTask,
                       CAP_CLIENT_READ_VOLUME_STATE_CFM,
                       message);
}


void capClientSendChangeVolumeCfm(AppTask appTask, ServiceHandle groupId,
                           CapClientResult result, uint8 deviceCount,
                           VcpInstElement *vcp,
                           CAP_INST *inst)
{
    CapClientGroupInstance* cap =
        (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(groupId);

    MAKE_CAP_CLIENT_MESSAGE(CapClientChangeVolumeCfm);
    uint8 i = 0;

    if (cap == NULL)
    {
        CAP_CLIENT_ERROR("\n capClientSendChangeVolumeCfm : NULL instance\n");
        CsrPmemFree(message);
        message = NULL;
        return;
    }

    if (cap->vcpPendingOp == CAP_CLIENT_INTERNAL_VCP_CHANGE_VOLUME || cap->vcpPendingOp == CAP_CLIENT_INTERNAL_VCP_MUTE)
    {
        /* Internal Pending op set to avoid sending cfm to app when the
         * change volume was triggered from CAP internally.
         * Free allocated message pointer as it will not be sent to app in this case */
        if (result != CAP_CLIENT_RESULT_VOLUME_REQ_FLUSHED)
        {
            CAP_CLIENT_CLEAR_PENDING_OP(cap->vcpPendingOp);
        }
        CsrPmemFree(message);
        message = NULL;
        return;
    }

    message->groupId = groupId;
    message->result = result;
    message->deviceStatusLen = 0;
    message->deviceStatus = NULL;

    if (result == CAP_CLIENT_RESULT_SUCCESS)
    {
        message->deviceStatusLen = deviceCount;
        message->deviceStatus = (CapClientDeviceStatus*)
                         CsrPmemZalloc(deviceCount*sizeof(CapClientDeviceStatus));

        for (i = 0; i < inst->deviceCount && vcp; vcp = vcp->next)
        {
             message->deviceStatus[i].cid = vcp->cid;
             message->deviceStatus[i].result = vcp->recentStatus;
             i++;
        }
    }

    CapClientMessageSend(appTask,
                      CAP_CLIENT_CHANGE_VOLUME_CFM,
                      message);
}

void capClientSendMuteCfm(AppTask appTask,
                   ServiceHandle groupId,
                   CapClientResult result,
                   uint8 deviceCount,
                   VcpInstElement *vcp,
                   CAP_INST* inst)
{
    CapClientGroupInstance* cap =
        (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(groupId);
    MAKE_CAP_CLIENT_MESSAGE(CapClientMuteCfm);

    uint8 i = 0;

    if (cap->vcpPendingOp == CAP_CLIENT_INTERNAL_VCP_MUTE || cap->vcpPendingOp == CAP_CLIENT_INTERNAL_VCP_CHANGE_VOLUME)
    {
        /* Internal Pending op set to avoid sending cfm to app when the
         * mute operation was triggered from CAP internally.
         * Free allocated message pointer as it will not be sent to app in this case */
        if (result != CAP_CLIENT_RESULT_VOLUME_REQ_FLUSHED)
        {
            CAP_CLIENT_CLEAR_PENDING_OP(cap->vcpPendingOp);
        }
        CsrPmemFree(message);
        message = NULL;
        return;
    }

    message->result = result;
    message->groupId = groupId;
    message->deviceStatusLen = 0x00;
    message->deviceStatus = NULL;

    if(result == CAP_CLIENT_RESULT_SUCCESS)
    {
        message->deviceStatusLen = deviceCount;
        message->deviceStatus = (CapClientDeviceStatus*)
                                CsrPmemZalloc(deviceCount*sizeof(CapClientDeviceStatus));

        for (i = 0; i < inst->deviceCount && vcp; vcp = vcp->next)
        {
             message->deviceStatus[i].cid = vcp->cid;
             message->deviceStatus[i].result = vcp->recentStatus;
             i++;
        }
    }

    CapClientMessageSend(appTask,
                      CAP_CLIENT_MUTE_CFM,
                      message);
}

void capHandleVcpCmdQueue(CapClientGroupInstance* cap, void *msg, CapClientVcpMsgType msgType, uint8 capVcpMsgState, uint8 vcpIndex)
{
    cap->capVcpCmdQueue[vcpIndex].capVcpMsg = msg;
    cap->capVcpCmdQueue[vcpIndex].capVcpMsgType = msgType;
    cap->capVcpCmdQueue[vcpIndex].capVcpMsgState = capVcpMsgState;
}

static void capSetMuteVolume(CapClientGroupInstance* cap, CAP_INST* inst, VcpInstElement* vcp, CapClientVcpMsgType msgType, uint8 volume, bool mute)
{
    if (msgType == CAP_CLIENT_INTERNAL_VCP_CHANGE_VOL_REQ)
    {
        capClientSetAbsoluteVolumeReq((CsrCmnListElm_t*)vcp, volume, inst, cap);
    }
    else if (msgType == CAP_CLIENT_INTERNAL_VCP_MUTE_REQ)
    {
        capClientSetVcpMuteStateReq(vcp, mute, inst, cap);
    }
}


void capHandleVcpRequest(CapClientGroupInstance* cap, CAP_INST* inst, void *msg, CapClientVcpMsgType msgType, uint8 vcpIndex)
{
    VcpInstElement* vcp = (VcpInstElement*)(cap->vcpList.first);
    CsipInstElement* csip = NULL;
    CapClientResult result = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;
    uint8 volume = 0;
    bool mute = 0;

    if (cap->capVcpCmdCount >= CAP_CLIENT_VOLUME_REQ_BUFFER_SIZE)
    {
         /* Check for the cmd count if its more than 2 then flush the previous vcp command and store the
          * next Recent VCP command in the queue. */
         if (cap->capVcpCmdCount == CAP_CLIENT_VOLUME_REQ_BUFFER_SIZE + 1)
         {
             result = CAP_CLIENT_RESULT_VOLUME_REQ_FLUSHED;
             cap->capVcpCmdCount--;

             if (msgType == CAP_CLIENT_INTERNAL_VCP_CHANGE_VOL_REQ)
             {
                /* Fetch the old vcp message from the cap vcp entry and send the confirmation with status as flushed */
                CapClientInternalChangeVolumeReq *prevMsg =
                        cap->capVcpCmdQueue[CAP_CLIENT_VCP_BUFFER_INDEX_FIRST].capVcpMsg;

                capClientSendChangeVolumeCfm(prevMsg->profileTask,
                                             prevMsg->groupId,
                                             result,
                                             0,
                                             NULL,
                                             inst);
                 /* Free the message as confirmation already sent to the App*/
                 capClientFreeCapInternalMsg(CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ, prevMsg);
            }
            else if (msgType == CAP_CLIENT_INTERNAL_VCP_MUTE_REQ)
            {
                /* Fetch the old vcp message from the cap vcp entry and send the confirmation with status as flushed */
                CapClientInternalMuteReq *prevMsg =
                        cap->capVcpCmdQueue[CAP_CLIENT_VCP_BUFFER_INDEX_FIRST].capVcpMsg;

                capClientSendMuteCfm(prevMsg->profileTask,
                                     prevMsg->groupId,
                                     result,
                                     0,
                                     NULL,
                                     inst);
                 /* Free the message as confirmation already sent to the App*/
                 capClientFreeCapInternalMsg(CAP_CLIENT_INTERNAL_MUTE_REQ, prevMsg);
            }
         }

         /* Store the recent value in the capVcpCmdQueue entry and then flush the previous entry
          * Send the confirmation for that packet with status as packet flushed */
         capHandleVcpCmdQueue(cap, msg, msgType, CAP_CLIENT_VOLUME_REQ_MESSAGE_QUEUED, CAP_CLIENT_VCP_BUFFER_INDEX_FIRST);
         return;
    }

    if (msgType == CAP_CLIENT_INTERNAL_VCP_CHANGE_VOL_REQ)
    {
        CapClientInternalChangeVolumeReq *req = (CapClientInternalChangeVolumeReq *)msg;

        if (req->profileTask == CSR_BT_CAP_CLIENT_IFACEQUEUE)
        {
            cap->vcpPendingOp = CAP_CLIENT_INTERNAL_VCP_CHANGE_VOLUME; /* To avoid sending the cfm to app */
        }

        volume = req->volumeState;
    }
    else if (msgType == CAP_CLIENT_INTERNAL_VCP_MUTE_REQ)
    {
        CapClientInternalMuteReq *req = (CapClientInternalMuteReq *)msg;

        if (req->profileTask == CSR_BT_CAP_CLIENT_IFACEQUEUE)
        {
            cap->vcpPendingOp = CAP_CLIENT_INTERNAL_VCP_MUTE; /* To avoid sending the cfm to app */
        }

        mute = req->muteState;
    }

    if (capClientIsGroupCoordinatedSet(cap))
    {
        csip = (CsipInstElement*)(cap->csipList.first);

        /* Here we need to obtain lock on all the devices and then
         * Start VCP Procedures*/

         /* check if the profile is already locked
          *
          * Note: If one device is in lock state in a group
          * it's assumed that all other participants are in lock state
          * and there is no need to serialize the CSIP and VCP procedures
          */

        capHandleVcpCmdQueue(cap, msg, msgType, CAP_CLIENT_VOLUME_REQ_MESSAGE_PROGRESS, vcpIndex);

        if (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capSetMuteVolume(cap, inst, vcp, msgType, volume, mute);
        }
        /* Otherwise obtain lock and the start BAP Procedures */
        else
        {
            /* Store the metadata Param and then Obtain Lock on
             * all the Devices */

            if (msgType == CAP_CLIENT_INTERNAL_VCP_CHANGE_VOL_REQ)
            {
                cap->pendingOp = CAP_CLIENT_VCP_SET_VOL;
                cap->volumeSetting = volume;
            }
            else if (msgType == CAP_CLIENT_INTERNAL_VCP_MUTE_REQ)
            {
                cap->pendingOp = CAP_CLIENT_VCP_MUTE;
                cap->volumeSetting = mute;
            }

            cap->requestCid = vcp->cid;
            capClientSetCsisLockState(csip, &inst->csipRequestCount, TRUE);
        }
    }
    else
    {
        capHandleVcpCmdQueue(cap, msg, msgType, CAP_CLIENT_VOLUME_REQ_MESSAGE_PROGRESS, vcpIndex);

        capSetMuteVolume(cap, inst, vcp, msgType, volume, mute);
    }
}

void handleChangeVolumeReq(CAP_INST* inst, const Msg msg)
{

    CapClientInternalChangeVolumeReq* req = (CapClientInternalChangeVolumeReq*)msg;
    CapClientGroupInstance* cap = NULL;
    CapClientResult result = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;
    CapClientProfileTaskListElem* task = NULL;

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (cap == NULL)
    {
        capClientSendChangeVolumeCfm(req->profileTask,
                              req->groupId,
                              CAP_CLIENT_RESULT_INVALID_GROUPID,
                              0,
                              NULL,
                              inst);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/
    task = (CapClientProfileTaskListElem*)
        CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

    if (task == NULL)
    {
        capClientSendChangeVolumeCfm(req->profileTask,
                                    req->groupId,
                                    CAP_CLIENT_RESULT_TASK_NOT_REGISTERED,
                                    0,
                                    NULL,
                                    inst);
        return;
    }

    /* Reject the API call if the Stream and control init is not done*/

    result = capClientValidateCapState(cap->capState, req->type);
    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n (CAP) capSendChangeVolumeCfm: Invalid state Trasition \n");
        capClientSendChangeVolumeCfm(req->profileTask,
                                     req->groupId,
                                     result,
                                     0,
                                     NULL,
                                     inst);
        return;
    }

    if ((cap->role & CAP_CLIENT_COMMANDER) != CAP_CLIENT_COMMANDER)
    {
        result = CAP_CLIENT_RESULT_INVALID_ROLE;
        CAP_CLIENT_INFO("\n (CAP) capSendChangeVolumeCfm: unsupported CAP role \n");
        capClientSendChangeVolumeCfm(req->profileTask,
                                    req->groupId,
                                    result,
                                    0,
                                    NULL,
                                    inst);
        return;
    }

    /* Reject the request if it is set member operation and pendingOp is not clear for other profiles
     *  saying CAP_CLIENT_RESULT_CAP_BUSY.
     *  If pendingOp is set to vol or mute, then this request will be added to the queue */
    if ((cap->setSize > 1 && cap->pendingOp != CAP_CLIENT_OP_NONE &&
            cap->pendingOp != CAP_CLIENT_VCP_SET_VOL && cap->pendingOp != CAP_CLIENT_VCP_MUTE)
            || (cap->vcpPendingOp == CAP_CLIENT_VCP_GET_VOL))
    {
        result = CAP_CLIENT_RESULT_CAP_BUSY;
        CAP_CLIENT_INFO("\n (CAP) capSendChangeVolumeCfm: CAP busy \n");
        capClientSendChangeVolumeCfm(req->profileTask,
                                     req->groupId,
                                     result,
                                     0,
                                     NULL,
                                     inst);
        return;
    }

    inst->vcpProfileTask = req->profileTask;

    /* Increment the CAP volume operation count */
    cap->capVcpCmdCount ++;

    /* handle the vcp request for set volume */
    capHandleVcpRequest(cap, inst, msg, CAP_CLIENT_INTERNAL_VCP_CHANGE_VOL_REQ, CAP_CLIENT_VCP_BUFFER_INDEX_ZERO);

}

void handleMuteReq(CAP_INST* inst, const Msg msg)
{
    CapClientInternalMuteReq* req = (CapClientInternalMuteReq*)msg;
    CapClientGroupInstance* cap = NULL;
    CapClientProfileTaskListElem* task = NULL;
    CapClientResult result = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (cap == NULL)
    {
        capClientSendMuteCfm(req->profileTask,
                       req->groupId,
                       CAP_CLIENT_RESULT_INVALID_GROUPID,
                       0,
                       NULL, 
                       inst);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/
    task = (CapClientProfileTaskListElem*)
        CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

    /* Reject the api call if the call is not from registered app*/
    if (task == NULL)
    {
        capClientSendMuteCfm(req->profileTask,
                            req->groupId,
                            CAP_CLIENT_RESULT_TASK_NOT_REGISTERED,
                            0,
                            NULL, 
                            inst);
        return;
    }

    result = capClientValidateCapState(cap->capState, req->type);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n (CAP) handleMuteReq: Invalid state Trasition \n");
        capClientSendMuteCfm(req->profileTask,
                             req->groupId,
                             result,
                             0,
                             NULL, 
                             inst);
        return;
    }

    if ((cap->role & CAP_CLIENT_COMMANDER) != CAP_CLIENT_COMMANDER)
    {
        result = CAP_CLIENT_RESULT_INVALID_ROLE;
        capClientSendMuteCfm(req->profileTask,
                            req->groupId,
                            result,
                            0,
                            NULL, 
                            inst);
        CAP_CLIENT_INFO("\n(CAP) handleMuteReq: Role Not Supported \n");
        return;
    }

    /* Reject the request if it is set member operation and pendingOp is not clear for other profiles
     *  saying CAP_CLIENT_RESULT_CAP_BUSY.
     *  If pendingOp is set to vol or mute, then this request will be added to the queue */
    if ((cap->setSize > 1 && cap->pendingOp != CAP_CLIENT_OP_NONE &&
            cap->pendingOp != CAP_CLIENT_VCP_SET_VOL && cap->pendingOp != CAP_CLIENT_VCP_MUTE)
            || (cap->vcpPendingOp == CAP_CLIENT_VCP_GET_VOL))
    {
        result = CAP_CLIENT_RESULT_CAP_BUSY;
        capClientFreeCapInternalMsg(req->type, (void*)req);
        capClientSendMuteCfm(req->profileTask,
                            req->groupId,
                            result,
                            0,
                            NULL,
                            inst);

        CAP_CLIENT_INFO("\n(CAP) handleMuteReq: CAP busy - Pending op is not cleared \n");
        return;
    }

    inst->vcpProfileTask = req->profileTask;

    /* Increment the CAP volume operation count */
    cap->capVcpCmdCount ++;

    /* handle the vcp request for mute/unmute */
    capHandleVcpRequest(cap, inst, msg, CAP_CLIENT_INTERNAL_VCP_MUTE_REQ, CAP_CLIENT_VCP_BUFFER_INDEX_ZERO);
}

void handleReadVolumeStateReq(CAP_INST* inst, const Msg msg)
{
    VcpInstElement* vcp = NULL;
    CapClientInternalReadVolumeStateReq* req = (CapClientInternalReadVolumeStateReq*)msg;
    CapClientGroupInstance* cap = NULL;
    CapClientProfileTaskListElem* task = NULL;
    CapClientResult result = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (cap == NULL)
    {
        capCLientSendReadVolumeStateCfm(req->profileTask, req->groupId,
                                      CAP_CLIENT_RESULT_INVALID_GROUPID,
                                      NULL, 0, 0, 0);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/
    task = (CapClientProfileTaskListElem*)
        CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

    if (task == NULL)
    {
        capCLientSendReadVolumeStateCfm(req->profileTask, req->groupId,
                                        CAP_CLIENT_RESULT_TASK_NOT_REGISTERED, NULL, 0, 0, 0);
        return;
    }

    /* Reject the API call if the Stream and control init is not done*/
    result = capClientValidateCapState(cap->capState, req->type);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        capCLientSendReadVolumeStateCfm(req->profileTask, req->groupId,
                                       result, NULL, 0, 0, 0);
        CAP_CLIENT_INFO("\n(CAP) handleReadVolumeStateReq: invalid state transition \n");
        return;
    }

    if ((cap->role & CAP_CLIENT_COMMANDER) != CAP_CLIENT_COMMANDER)
    {
        result = CAP_CLIENT_RESULT_INVALID_ROLE;
        capCLientSendReadVolumeStateCfm(req->profileTask, req->groupId,
                                          result, NULL, 0, 0, 0);
        CAP_CLIENT_INFO("\n(CAP) handleReadVolumeStateReq: Role Not Supported \n");
        return;
    }

    vcp = (VcpInstElement*)CAP_CLIENT_GET_VCP_ELEM_FROM_CID(cap->vcpList, req->cid);

    /* If VCP instance is null, just return sending NULL instance Error*/
    if (vcp == NULL)
    {
        result = CAP_CLIENT_RESULT_NULL_INSTANCE;
        capCLientSendReadVolumeStateCfm(req->profileTask, req->groupId,
            result, NULL, 0, 0, 0);
        CAP_CLIENT_INFO("\n(CAP) handleReadVolumeStateReq: Vcp instance NULL \n");
        return;
    }

    /* Reject the request if it is set member operation and pendingOp is not clear
     *  saying CAP_CLIENT_RESULT_CAP_BUSY */
    if ((cap->setSize > 1 && cap->pendingOp != CAP_CLIENT_OP_NONE) || (inst->vcpRequestCount > 0))
    {
        result = CAP_CLIENT_RESULT_CAP_BUSY;
        CAP_CLIENT_INFO("\n (CAP) capSendReadVolumeStateCfm: CAP busy \n");
        capCLientSendReadVolumeStateCfm(req->profileTask,
                                        req->groupId,
                                        result,
                                        NULL,
                                        0,
                                        0,
                                        0);
        return;
    }

    inst->vcpProfileTask = req->profileTask;
    inst->vcpRequestCount++;
    cap->vcpPendingOp = CAP_CLIENT_VCP_GET_VOL;
    VcpVolumeStateRequest(vcp->vcpHandle);
}
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
