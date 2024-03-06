/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_util.h"
#include "cap_client_common.h"
#include "cap_client_init_stream_and_control_req.h"
#include "cap_client_remove_device_req.h"
#include "cap_client_vcp_operation_req.h"
#include "cap_client_debug.h"
#include "cap_client_csip_handler.h"
#include "cap_client_vcp_handler.h"
#ifdef INSTALL_LEA_UNICAST_CLIENT

static void capHandleFreeCapVcpInternalMsg(uint16 type, Msg msg)
{
    capClientFreeCapInternalMsg(type, msg);
}

static void updateCcIndCount(CAP_INST *inst, VcpInstElement* vcp)
{
    vcp->expChangeCounter = vcp->changeCounter + 1; /* Expected change counter from remote device */
    inst->vcpRequestCount++;
    inst->vcpIndicationCount++; /* Total expected Volume state indications as per CAP's volume change */

}
static void capClientVcpIndTimerFire(CsrUint8 dummy, CAP_INST* inst)
{
    /* 10 seconds timer waiting for all the expected Volume state indications has fired */
    CapClientGroupInstance *gInst =
               (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    VcpInstElement *elem;

    inst->vcpIndicationCount = 0;

    if (gInst == NULL)
    {
        CAP_CLIENT_ERROR("\n capClientVcpIndTimerFire : NULL instance\n");
        return;
    }

    elem = (VcpInstElement*) gInst->vcpList.first;

    gInst->vcpPendingOp = CAP_CLIENT_INTERNAL_VCP_READ_VOLUME_STATE;

    for (;elem; elem = elem->next)
    { /* Read volume state of all devices currently in the group */
        VcpVolumeStateRequest(elem->vcpHandle);
        inst->vcpRequestCount++;
    }
    CSR_UNUSED(dummy);
}

/* This is utility function for enabling/disabling VCP CCD  */
static void capClientVcpEnableCcd(CAP_INST *inst, uint8 enable, CapClientVcpCccdType type)
{
    CapClientGroupInstance* cap = CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);
    VcpInstElement* vcp = NULL;

    if (cap)
    {
        vcp = (VcpInstElement*)cap->vcpList.first;
    }

    while (vcp)
    {
        if (vcp->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
        {
            if (type == CAP_CLIENT_INTERNAL_VCP_STATE_CCCD)
            {
                CAP_CLIENT_INFO("\n(CAP) : capClientVcpEnableCcd VcpVolumeStateRegisterForNotificationReq\n");
                VcpVolumeStateRegisterForNotificationReq(vcp->vcpHandle, enable);
                inst->vcpRequestCount++;
            }
            else if (type == CAP_CLIENT_INTERNAL_VCP_FLAG_CCCD)
            {
                CAP_CLIENT_INFO("\n(CAP) : capClientVcpEnableCcd VcpVolumeFlagRegisterForNotificationReq\n");
                VcpVolumeFlagRegisterForNotificationReq(vcp->vcpHandle, enable);
            }
        }

        vcp = vcp->next;
    }
}

static void capClientSendInitStreamCfm(CAP_INST *inst, CapClientGroupInstance *gInst)
{
    CsrCmnListElm_t* elem = (CsrCmnListElm_t*)((&gInst->vcpList)->first);

    gInst->capState = CAP_CLIENT_STATE_INIT_STREAM_CTRL;
    gInst->pendingOp = CAP_CLIENT_OP_NONE;

    capClientSendStreamControlInitCfm(inst,
                                     TRUE,
                                     CAP_CLIENT_RESULT_SUCCESS,
                                     gInst->role,
                                     elem);
    /* Enable CSIP cccd*/
    capClientCsipEnableCcd(inst, TRUE);
    capClientVcpEnableCcd(inst, TRUE, CAP_CLIENT_INTERNAL_VCP_FLAG_CCCD);
}

static void handleQueuedMsg(CAP_INST *inst, CapClientGroupInstance *cap)
{
    CapClientVcpMsgType msgType = cap->capVcpCmdQueue[CAP_CLIENT_VCP_BUFFER_INDEX_ZERO].capVcpMsgType;
    /* Check for the remaining pending request in the Cap VCP command queue */
    if (cap && cap->capVcpCmdQueue[CAP_CLIENT_VCP_BUFFER_INDEX_ZERO].capVcpMsgState == CAP_CLIENT_VOLUME_REQ_MESSAGE_PROGRESS)
    {
        uint16 type;
        type = (msgType == CAP_CLIENT_INTERNAL_VCP_CHANGE_VOL_REQ) ? CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ : CAP_CLIENT_INTERNAL_MUTE_REQ;

        capHandleFreeCapVcpInternalMsg(type, cap->capVcpCmdQueue[CAP_CLIENT_VCP_BUFFER_INDEX_ZERO].capVcpMsg);

        cap->capVcpCmdQueue[CAP_CLIENT_VCP_BUFFER_INDEX_ZERO].capVcpMsgState = CAP_CLIENT_VOLUME_REQ_MESSAGE_IDLE;
        cap->capVcpCmdCount--;
    }
    /* If some request is queued by this time, process it */
    if (cap && cap->capVcpCmdCount &&
        cap->capVcpCmdQueue[CAP_CLIENT_VCP_BUFFER_INDEX_FIRST].capVcpMsgState == CAP_CLIENT_VOLUME_REQ_MESSAGE_QUEUED)
    {
        msgType = cap->capVcpCmdQueue[CAP_CLIENT_VCP_BUFFER_INDEX_FIRST].capVcpMsgType;
        /* Get the queued messaged and assign it to 0 index */
        capHandleVcpCmdQueue(cap, cap->capVcpCmdQueue[CAP_CLIENT_VCP_BUFFER_INDEX_FIRST].capVcpMsg, msgType, CAP_CLIENT_VOLUME_REQ_MESSAGE_PROGRESS, CAP_CLIENT_VCP_BUFFER_INDEX_ZERO);

        cap->capVcpCmdQueue[CAP_CLIENT_VCP_BUFFER_INDEX_FIRST].capVcpMsgState = CAP_CLIENT_VOLUME_REQ_MESSAGE_PROGRESS;
        capHandleVcpRequest(cap, inst, cap->capVcpCmdQueue[CAP_CLIENT_VCP_BUFFER_INDEX_ZERO].capVcpMsg, msgType, CAP_CLIENT_VCP_BUFFER_INDEX_ZERO);
    }
}

static void updateVcpParamaters(VcpInstElement* vcp, VcpVolumeStateInd *ind)
{
    vcp->changeCounter = ind->changeCounter; /* Update change counter received from remote device*/
    vcp->volumeState = ind->volumeState;
    vcp->muteState = ind->mute;
}


static void updateVolumeState(CAP_INST *inst, CapClientGroupInstance *cap, VcpInstElement* vcp, VcpVolumeStateInd *ind, uint8 volume)
{
    /* Message created in CAP for volume change, must be freed by CAP only */
    CapClientInternalChangeVolumeReq *msg = (CapClientInternalChangeVolumeReq*) CsrPmemZalloc(sizeof(CapClientInternalChangeVolumeReq));

    updateVcpParamaters(vcp, ind);

    msg->type = CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ;
    msg->profileTask = cap->libTask;
    msg->groupId = inst->activeGroupId;
    msg->volumeState = volume;

    capHandleVcpRequest(cap, inst, msg, CAP_CLIENT_INTERNAL_VCP_CHANGE_VOL_REQ, CAP_CLIENT_VCP_BUFFER_INDEX_ZERO);
}

static void updateMuteState(CAP_INST *inst, CapClientGroupInstance *cap, VcpInstElement* vcp, VcpVolumeStateInd *ind, uint8 mute)
{
    /* Message created in CAP for volume change, must be freed by CAP only */
    CapClientInternalMuteReq *msg = (CapClientInternalMuteReq*) CsrPmemZalloc(sizeof(CapClientInternalMuteReq));

    updateVcpParamaters(vcp, ind);

    msg->type = CAP_CLIENT_INTERNAL_MUTE_REQ;
    msg->profileTask = cap->libTask;
    msg->groupId = inst->activeGroupId;
    msg->muteState = mute;

    capHandleVcpRequest(cap, inst, msg, CAP_CLIENT_INTERNAL_VCP_MUTE_REQ, CAP_CLIENT_VCP_BUFFER_INDEX_ZERO);
}


static void handleVolumeStateInd(CAP_INST *inst, CapClientGroupInstance *cap, VcpInstElement* vcp, VcpVolumeStateInd *ind)
{
    if (vcp->expChangeCounter == ind->changeCounter && (vcp->expVolumeState == ind->volumeState || vcp->expMuteState == ind->mute))
    { /* Indication received as a part of CAP triggered volume change */
        /* Decrement ind counter */
        inst->vcpIndicationCount--;

        updateVcpParamaters(vcp, ind);

        if (vcp->expMuteState == ind->mute)
        { /* Reset expMuteState to default value (0xFF) to avoid race condition where we have changed volume on a change counter x
           * and before it got processed on the server, it changed the volume locally so the expChangeCounter value will match with us and
           * if expMuteState is still some older value (0 or 1) then this case might be treated as local change rather than remote change */
            vcp->expMuteState = CAP_CLIENT_VCP_DEFAULT_MUTE_STATE;
        }

        if (inst->vcpIndicationCount == 0)
        { /* All expected indications have been received */
            uint8 flag = 0;

            if (ind->volumeState != cap->groupVolume || ind->mute != cap->groupMute)
            { /* If volume or mute of this device doesn't matches with group volume then only proceed to handle,
               * otherwise if there is something already in the queue it will be picked up from below function
               * handleQueuedMsg and the group parameters will be updated so later the check for group volume/mute
               * with current device will not match and we will unnecessarily process them */
                flag = 1;
            }

            CsrSchedTimerCancel(cap->timerId, NULL, NULL); /* Cancel the indication timer */
            handleQueuedMsg(inst, cap);

            if (!flag)
            { /* Group volume/mute might be updated from queued msg processing, if current volume/mute of the device
               * matches with previous group volume/mute then no need to proceed further */
                return;
            }

        }

        if (ind->volumeState != cap->groupVolume)
        {
            /* Increment the CAP volume operation count */
            cap->capVcpCmdCount++;
            updateVolumeState(inst, cap, vcp, ind, cap->groupVolume);
        }
        else if (ind->mute != cap->groupMute)
        {
            /* Increment the CAP volume operation count */
            cap->capVcpCmdCount++;
            updateMuteState(inst, cap, vcp, ind, cap->groupMute);
        }
    }
    else
    { /* Indication received due to volume change done on remote device locally, Add into queue */

        if (cap->currentDeviceCount == 1)
        { /* If there is only one device in the group, update the group volume here only
           * as CAP will skip the set volume on this device later because this ind came
           * from this single device in the group only, no need to write the same value again*/
            cap->groupVolume = ind->volumeState;
            cap->groupMute = ind->mute;
            updateVcpParamaters(vcp, ind);

        }
        else
        {
            if (vcp->volumeState != ind->volumeState && vcp->muteState != ind->mute)
            { /* If both mute and volume has changed on remote device at the same time, preference would be given to volume
               * In this case update group volume and mute to current values as we will have to update mute as well after
               * volume changes is done */
               cap->groupVolume = ind->volumeState;
               cap->groupMute = ind->mute;

            }
            if (vcp->volumeState != ind->volumeState)
            {
                /* Increment the CAP volume operation count */
                cap->capVcpCmdCount++;
                updateVolumeState(inst, cap, vcp, ind, ind->volumeState);
            }
            else if (vcp->muteState != ind->mute)
            {
                /* Increment the CAP volume operation count */
                cap->capVcpCmdCount++;
                updateMuteState(inst, cap, vcp, ind, ind->mute);
            }
        }
    }

    if (capClientIsGroupCoordinatedSet(cap))
    { /* If the CSIP is in Locked state and there is no pending message in VCP queue, release the lock now */
        CsipInstElement *csip = (CsipInstElement*) cap->csipList.first;

        if (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED && cap->capVcpCmdCount == 0)
        {
            capClientSetCsisLockState(csip, &inst->csipRequestCount, FALSE);
        }
    }
}

void capClientSetAbsoluteVolumeReq(CsrCmnListElm_t *elem,
                             uint8 volumeSetting, 
                             CAP_INST *inst,
                             CapClientGroupInstance *cap)
{
    VcpInstElement* vcp = (VcpInstElement*)elem;

    CAP_CLIENT_INFO("\n(CAP) : capClientSetAbsoluteVolumeReq \n");

    for (; vcp; vcp = vcp->next)
    {
        if (vcp->volumeState != volumeSetting)
        { /* Skip volume change on the devices which are already on the same value */
            if (vcp->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
            {
                vcp->expVolumeState = volumeSetting; /* Expected volume state from remote device when CAP triggers the volume change*/
                updateCcIndCount(inst, vcp);
                VcpAbsoluteVolumeRequest(vcp->vcpHandle, volumeSetting);
            }
            else
            {
                CAP_CLIENT_INFO("\n(CAP) capClientSetAbsoluteVolumeReq : Recent Error \n");
            }
        }
    }

    if (inst->vcpRequestCount == 0)
    {
        elem = (CsrCmnListElm_t*)(cap->vcpList.first);

        capClientSendChangeVolumeCfm(inst->vcpProfileTask,
                                     inst->activeGroupId, 
                                     CAP_CLIENT_RESULT_SUCCESS,
                                     inst->deviceCount,
                                     (VcpInstElement*)elem,
                                     inst);
        handleQueuedMsg(inst, cap);
    }
    else
    {
        cap->groupVolume = volumeSetting;
    }
    
}

void capClientSetVcpMuteStateReq(VcpInstElement* vcp,
                                 bool mute,
                                 CAP_INST *inst,
                                 CapClientGroupInstance *cap)
{
    VcpInstElement *tmp = vcp;
    CAP_CLIENT_INFO("\n(CAP) : capClientSetVcpMuteStateReq \n");

    for (; vcp; vcp = vcp->next)
    {
        if (mute!= vcp->muteState)
        {
            if (vcp->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
            {
                updateCcIndCount(inst, vcp);
                vcp->expMuteState = mute; /* Expected mute state from remote device when CAP triggers the volume change*/

                mute ? VcpMuteRequest(vcp->vcpHandle) : VcpUnmuteRequest(vcp->vcpHandle);
            }
            else
            {
                CAP_CLIENT_INFO("\n(CAP) capClientSetVcpMuteStateReq : Recent Error \n");
            }
        }
    }

    if (inst->vcpRequestCount == 0)
    {
        tmp = (VcpInstElement*)cap->vcpList.first;
        capClientSendMuteCfm(inst->vcpProfileTask, cap->groupId, CAP_CLIENT_RESULT_SUCCESS,inst->deviceCount, tmp, inst);

        handleQueuedMsg(inst, cap);
    }
    else
    {
        cap->groupMute = mute;
    }
}

static void capClientHandleVcpInitCfm(const Msg msg, CAP_INST *inst, CapClientGroupInstance *gInst)
{
    VcpInitCfm *cfm = (VcpInitCfm*)msg;

    VcpInstElement *vcp = (VcpInstElement*)
                        CAP_CLIENT_GET_VCP_ELEM_FROM_CID(gInst->vcpList, cfm->cid);


    CAP_CLIENT_INFO("\n(CAP) : capClientHandleVcpInitCfm: VcpInitCfm: \n");
    CAP_CLIENT_INFO("(CAP) :  Status : 0x%02x, Vcp Handle: 0x%04x, BtConnId: 0x%04x \n", cfm->status, cfm->prflHndl, cfm->cid);

    /* If no VCP found corresponding to cid
     * PANIC since the VCP instance would already be added
     * in INIT cfm for given CID*/

    if (vcp == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP) capClientHandleVcpInitCfm: PANIC unable to find the VCP instance ");
        return;
    }

    vcp->recentStatus = capClientGetCapClientResult(cfm->status, CAP_CLIENT_VCP);

    if (cfm->status == VCP_STATUS_SUCCESS)
    {
        vcp->vcpHandle = cfm->prflHndl;
    }

    if (cfm->status != VCP_STATUS_IN_PROGRESS)
    {
        inst->vcpRequestCount--;
    }

    /* All the Init Cfms are received and counter Hits zero */
    if (inst->vcpRequestCount == 0)
    {
        capClientVcpEnableCcd(inst, TRUE, CAP_CLIENT_INTERNAL_VCP_STATE_CCCD);

        if (inst->vcpRequestCount == 0)
        { /* All instance of VCP returned error, send the cfm from here */
            capClientSendInitStreamCfm(inst, gInst);
        }
    }
}

void capClientHandleVolumeStateInd(VcpVolumeStateInd *ind, CAP_INST* inst, CapClientGroupInstance* cap)
{
    VcpInstElement* vcp = (VcpInstElement*)
        CAP_CLIENT_GET_VCP_ELEM_FROM_PHANDLE(cap->vcpList, ind->prflHndl);

    MAKE_CAP_CLIENT_MESSAGE(CapClientVolumeStateInd);

    if (cap->pendingOp == CAP_CLIENT_VCP_INIT)
    {
        /* Indication received as part of volume change in init phase, can be ignored.
         * Free allocated message pointer as it will not be sent to app in this case  */
        CsrPmemFree(message);
        message = NULL;
        return;
    }

    CAP_CLIENT_INFO("(CAP) : Vcp Handle: 0x%04x, VolumeState : 0x%02x, Mute: 0x%02x \n", ind->prflHndl,ind->volumeState, ind->mute);

    /* Send Volume State Indication to application */
    message->groupId = inst->activeGroupId;
    message->volumeState = ind->volumeState;
    message->mute = ind->mute;
    message->changeCounter = ind->changeCounter;
    message->cid = vcp->cid;

    CapClientMessageSend(inst->profileTask, CAP_CLIENT_VOLUME_STATE_IND, message);

    handleVolumeStateInd(inst, cap, vcp, ind);
}

void capClientHandleAbsVolCfm(const Msg msg, CAP_INST* inst, CapClientGroupInstance* gInst)
{
    VcpAbsVolCfm* cfm = (VcpAbsVolCfm*)msg;

    VcpInstElement* vcp = (VcpInstElement*)
                       CAP_CLIENT_GET_VCP_ELEM_FROM_PHANDLE(gInst->vcpList, cfm->prflHndl);

    inst->vcpRequestCount--;
    CAP_CLIENT_INFO("\n(CAP) : capClientHandleAbsVolCfm: vcpRequestCount:  %d\n", inst->vcpRequestCount);

    if (vcp == NULL)
    {
        capClientSendChangeVolumeCfm(inst->vcpProfileTask, gInst->groupId, CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR,
                               0, NULL, inst);
        CAP_CLIENT_ERROR("\n(CAP) capClientHandleAbsVolCfm: PANIC unable to find the VCP instance");
        return;
    }

    vcp->recentStatus = capClientGetCapClientResult(cfm->status, CAP_CLIENT_VCP);

    if (inst->vcpRequestCount == 0)
    {
        if (gInst->pendingOp == CAP_CLIENT_VCP_INIT)
        {
            VcpInstElement *vcpElem = (VcpInstElement*) gInst->vcpList.first;
            for (; vcpElem; vcpElem = vcpElem->next)
            {
                if (vcpElem->muteState != gInst->groupMute)
                {
                    if (vcpElem->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
                    {
                        updateCcIndCount(inst, vcpElem);
                        vcpElem->expChangeCounter = vcpElem->changeCounter + 1; /* Expected change counter from remote device */
                        vcpElem->expMuteState = gInst->groupMute; /* Expected mute state from remote device when CAP triggers the volume change*/

                        gInst->groupMute ? VcpMuteRequest(vcpElem->vcpHandle) : VcpUnmuteRequest(vcpElem->vcpHandle);
                    }
                }
            }

            if (inst->vcpRequestCount == 0)
            {
                capClientSendInitStreamCfm(inst, gInst);
                return;
            }
        }

        /* All cfms have been received, Start the indications timer */

        gInst->timerId = CsrSchedTimerSet(CSR_BT_CAP_CLIENT_VCP_INDICATION_TIMER,
                                          (void (*) (CsrUint16, void *)) capClientVcpIndTimerFire,
                                          0,
                                          (void*)inst);

        vcp = (VcpInstElement*) (gInst->vcpList.first);

        capClientSendChangeVolumeCfm(inst->vcpProfileTask,
                                     gInst->groupId,
                                     CAP_CLIENT_RESULT_SUCCESS,
                                     inst->deviceCount,
                                     vcp,
                                     inst);
    }
}

void capClientHandleUnMuteCfm(const Msg msg, CAP_INST* inst, CapClientGroupInstance* gInst)
{
    VcpUnmuteCfm* cfm = (VcpUnmuteCfm*)msg;
    VcpInstElement* vcp = (VcpInstElement*)
          CAP_CLIENT_GET_VCP_ELEM_FROM_PHANDLE(gInst->vcpList, cfm->prflHndl);


    CAP_CLIENT_INFO("\n(CAP) : capClientHandleUnMuteCfm: VcpAbsVolCfm: \n");


    if (vcp == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP) capClientHandleUnMuteCfm: PANIC unable to find the VCP instance ");
        return;
    }

    vcp->recentStatus = capClientGetCapClientResult(cfm->status, CAP_CLIENT_VCP);
    inst->vcpRequestCount--;

    if (inst->vcpRequestCount == 0)
    {
        if (gInst->pendingOp == CAP_CLIENT_VCP_INIT)
        {
            capClientSendInitStreamCfm(inst, gInst);
            return;
        }

        /* All cfms have been received, Start the indications timer */

        gInst->timerId = CsrSchedTimerSet(CSR_BT_CAP_CLIENT_VCP_INDICATION_TIMER,
                                          (void (*) (CsrUint16, void *)) capClientVcpIndTimerFire,
                                          0,
                                          (void*)inst);

        vcp = (VcpInstElement*) (gInst->vcpList.first);

        capClientSendMuteCfm(inst->vcpProfileTask,
                             gInst->groupId,
                             CAP_CLIENT_RESULT_SUCCESS,
                             inst->deviceCount,
                             vcp,
                             inst);
    }
}

void capClientHandleMuteCfm(const Msg msg, CAP_INST* inst, CapClientGroupInstance* gInst)
{
    VcpMuteCfm* cfm = (VcpMuteCfm*)msg;
    VcpInstElement* vcp = (VcpInstElement*)
                    CAP_CLIENT_GET_VCP_ELEM_FROM_PHANDLE(gInst->vcpList, cfm->prflHndl);


    CAP_CLIENT_INFO("\n(CAP) : capClientHandleMuteCfm: VcpAbsVolCfm: \n");


    if (vcp == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP) capClientHandleMuteCfm: PANIC unable to find the VCP instance ");
        return;
    }

    vcp->recentStatus = capClientGetCapClientResult(cfm->status, CAP_CLIENT_VCP);
    inst->vcpRequestCount--;

    if (inst->vcpRequestCount == 0)
    {
        if (gInst->pendingOp == CAP_CLIENT_VCP_INIT)
        {
            capClientSendInitStreamCfm(inst, gInst);
            return;
        }

        /* All cfms have been received, Start the indications timer */

        gInst->timerId = CsrSchedTimerSet(CSR_BT_CAP_CLIENT_VCP_INDICATION_TIMER,
                                          (void (*) (CsrUint16, void *)) capClientVcpIndTimerFire,
                                          0,
                                          (void*)inst);

        vcp = (VcpInstElement*) (gInst->vcpList.first);

        capClientSendMuteCfm(inst->vcpProfileTask,
                             gInst->groupId,
                             CAP_CLIENT_RESULT_SUCCESS,
                             inst->deviceCount,
                             vcp,
                             inst);
    }

}

void capClientHandleVolumeStateCfm(const Msg msg, CAP_INST* inst, CapClientGroupInstance* gInst)
{
    CapClientResult result;
    VcpReadVolumeStateCfm* cfm = (VcpReadVolumeStateCfm*)msg;
    VcpInstElement* vcp = (VcpInstElement*)
        CAP_CLIENT_GET_VCP_ELEM_FROM_PHANDLE(gInst->vcpList, cfm->prflHndl);

    if (vcp == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP) capClientHandleVolumeStateCfm: Error unable to find the VCP instance ");
        return;
    }

    vcp->muteState = cfm->mute;
    vcp->volumeState = cfm->volumeSetting;
    vcp->changeCounter = cfm->changeCounter;

    result = capClientGetCapClientResult(cfm->status, CAP_CLIENT_VCP);

    /* Decrement the VCP Operation Counter on Cfm*/
    if (inst->vcpRequestCount != 0)
    {
        inst->vcpRequestCount--;
    }

    if (gInst->pendingOp != CAP_CLIENT_VCP_INIT && gInst->vcpPendingOp != CAP_CLIENT_INTERNAL_VCP_READ_VOLUME_STATE)
    { /* This is App triggered read request.
       * CAP_CLIENT_VCP_INIT is used while initialisation, CAP_CLIENT_INTERNAL_VCP_READ_VOLUME_STATE is for internal read request */
        capCLientSendReadVolumeStateCfm(inst->vcpProfileTask,
                                        inst->activeGroupId,
                                        result,
                                        vcp,
                                        cfm->mute,
                                        cfm->volumeSetting,
                                        cfm->changeCounter);
        return;
    }

    /* All the Read Cfms are received and counter Hits zero */
    if (inst->vcpRequestCount == 0 && gInst->pendingOp == CAP_CLIENT_VCP_INIT)
    { /* CAP_CLIENT_VCP_INIT is used during initialisation time */
        VcpInstElement *vcpElem = (VcpInstElement*) gInst->vcpList.first;

        if (gInst->groupVolume == CAP_CLIENT_VCP_DEFAULT_VOLUME_STATE && gInst->groupMute == CAP_CLIENT_VCP_DEFAULT_MUTE_STATE)
        { /* First device addition in the group */
            gInst->groupVolume = vcpElem->volumeState;
            gInst->groupMute = vcpElem->muteState;
            capClientSendInitStreamCfm(inst, gInst);
        }
        else
        {
            for (; vcpElem; vcpElem = vcpElem->next)
            {
                if (vcpElem->volumeState != gInst->groupVolume)
                { /* Skip volume change on the devices which are already on the same value */
                    if (vcpElem->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
                    {
                        updateCcIndCount(inst, vcpElem);
                        vcpElem->expVolumeState = gInst->groupVolume; /* Expected volume state from remote device when CAP triggers the volume change*/

                        VcpAbsoluteVolumeRequest(vcpElem->vcpHandle, gInst->groupVolume);
                    }
                }
                else if (vcpElem->muteState != gInst->groupMute)
                {
                    if (vcpElem->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
                    {
                        updateCcIndCount(inst, vcpElem);
                        vcpElem->expMuteState = gInst->groupMute; /* Expected mute state from remote device when CAP triggers the volume change*/

                        gInst->groupMute ? VcpMuteRequest(vcpElem->vcpHandle) : VcpUnmuteRequest(vcpElem->vcpHandle);
                    }
                }
            }

            if (inst->vcpRequestCount == 0)
            {
                capClientSendInitStreamCfm(inst, gInst);
            }
        }
    }

    if (inst->vcpRequestCount == 0 && gInst->vcpPendingOp == CAP_CLIENT_INTERNAL_VCP_READ_VOLUME_STATE)
    { /* CAP_CLIENT_INTERNAL_VCP_READ_VOLUME_STATE is used when CAP internally triggers the read volume state as a part of
       * indication timer fire */

        CAP_CLIENT_CLEAR_PENDING_OP(gInst->vcpPendingOp);

        handleQueuedMsg(inst, gInst);
    }
}

void capClientHandleVolumeStateNtfCfm(const Msg msg, CAP_INST* inst, CapClientGroupInstance* gInst)
{
    VcpVolumeStateSetNtfCfm *cfm = (VcpVolumeStateSetNtfCfm*) msg;
    VcpInstElement *vcp = (VcpInstElement*) CAP_CLIENT_GET_VCP_ELEM_FROM_PHANDLE(gInst->vcpList, cfm->prflHndl);

    if (vcp == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP) capClientHandleVolumeStateNtfCfm: Error unable to find the VCP instance ");
        return;
    }

    /* Decrement the VCP Operation Counter on Cfm*/
    if (inst->vcpRequestCount != 0)
    {
        inst->vcpRequestCount--;
    }

    if (inst->vcpRequestCount == 0)
    {
        VcpInstElement *elem = (VcpInstElement*) gInst->vcpList.first;

        for (; elem; elem = elem->next)
        {
            if (elem->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
            { /* Reading volume state of all devices after init is mandatory */
                VcpVolumeStateRequest(elem->vcpHandle);
                inst->vcpRequestCount++;
            }
        }

        if (inst->vcpRequestCount == 0)
        { /* All instance of VCP returned error, send the cfm from here */
            capClientSendInitStreamCfm(inst, gInst);
        }
    }
}
void capClientHandleVcpMsg(CAP_INST *inst, const Msg msg)
{
    CsrBtGattPrim *prim = (CsrBtGattPrim *)msg;

    CapClientGroupInstance *gInst =
               (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (gInst == NULL)
    {
        CAP_CLIENT_INFO("capClientHandleVcpMsg: gInst is NULL");
        return;
    }

    switch(*prim)
    {
        case VCP_INIT_CFM:
        {
            gInst->pendingOp = CAP_CLIENT_VCP_INIT;
            capClientHandleVcpInitCfm(msg, inst, gInst);
        }
        break;

        case VCP_VCS_TERMINATE_CFM:
        {
            capClientRemoveGroup(msg, inst, CAP_CLIENT_VCP);
        }
        break;

        case VCP_ABS_VOL_CFM:
        {
            capClientHandleAbsVolCfm(msg, inst, gInst);
        }
        break;

        case VCP_MUTE_CFM:
        {
            capClientHandleMuteCfm(msg, inst, gInst);
        }
        break;
        case VCP_UNMUTE_CFM:
        {
            capClientHandleUnMuteCfm(msg, inst, gInst);
        }
        break;

        case VCP_DESTROY_CFM:
        {
            VcpDestroyCfm *cfm = (VcpDestroyCfm*)msg;

            if(cfm->status == VCP_STATUS_SUCCESS)
            {
                capClientRemoveGroup(NULL, inst, CAP_CLIENT_VCP);
            }
        }
        break;

        case VCP_READ_VOLUME_STATE_CFM:
        {
            capClientHandleVolumeStateCfm(msg, inst, gInst);
        }
        break;

        case VCP_READ_VOLUME_FLAG_CFM:
        {
            VcpReadVolumeFlagCfm* cfm = (VcpReadVolumeFlagCfm*)msg;
            VcpInstElement* vcp = (VcpInstElement*)
                CAP_CLIENT_GET_VCP_ELEM_FROM_PHANDLE(gInst->vcpList, cfm->prflHndl);

            vcp->flags = cfm->volumeFlag;
        }
        break;

        case VCP_VOLUME_STATE_IND:
        {
            VcpVolumeStateInd* ind = (VcpVolumeStateInd*)msg;
            capClientHandleVolumeStateInd(ind, inst, gInst);
        }
        break;

        case VCP_VOLUME_FLAG_IND:
        {
            VcpVolumeFlagInd* ind = (VcpVolumeFlagInd*)msg;
            VcpInstElement* vcp = (VcpInstElement*)
                CAP_CLIENT_GET_VCP_ELEM_FROM_PHANDLE(gInst->vcpList, ind->prflHndl);

            vcp->flags = ind->volumeFlag;
        }
        break;

        case VCP_VOLUME_STATE_SET_NTF_CFM:
        {
            capClientHandleVolumeStateNtfCfm(msg, inst, gInst);
        }
        break;

        case VCP_VOLUME_FLAG_SET_NTF_CFM:
        {
            VcpVolumeFlagSetNtfCfm* cfm = (VcpVolumeFlagSetNtfCfm*)msg;
            CSR_UNUSED(cfm);
        }
        break;

        default:
            break;
    }
}
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
