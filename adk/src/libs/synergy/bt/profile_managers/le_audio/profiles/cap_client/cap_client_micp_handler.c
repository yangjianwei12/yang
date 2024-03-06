/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #
******************************************************************************/
#include "cap_client_util.h"
#include "cap_client_common.h"
#include "cap_client_init_optional_service_req.h"
#include "cap_client_remove_device_req.h"
#include "cap_client_micp_operation_req.h"
#include "cap_client_debug.h"
#include "cap_client_csip_handler.h"
#include "cap_client_vcp_handler.h"
#include "cap_client_micp_handler.h"

#ifndef EXCLUDE_CSR_BT_MICP_MODULE

static void capClientHandleMicpInitCfm(const Msg msg, CAP_INST *inst, CapClientGroupInstance *gInst)
{
    MicpInitCfm *cfm = (MicpInitCfm*)msg;

    MicpInstElement *micp = (MicpInstElement*)
                        CAP_CLIENT_GET_MICP_ELEM_FROM_CID(gInst->micpList, cfm->cid);


    CAP_CLIENT_INFO("\n(CAP) : capClientHandleMicpInitCfm: McpInitCfm: \n");
    CAP_CLIENT_INFO("(CAP) :  Status : 0x%02x, MICP Handle: 0x%04x, BtConnId: 0x%04x \n", cfm->status, cfm->prflHndl, cfm->cid);

    /* If no MICP found corresponding to cid
     * PANIC since the MICP instance would already be added
     * in INIT cfm for given CID*/

    if(micp == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP) capClientHandleMicpInitCfm: PANIC unable to find the MICP instance ");
        return;
    }

    micp->recentStatus = capClientGetCapClientResult(cfm->status, CAP_CLIENT_MICP);

    if(cfm->status == MICP_STATUS_SUCCESS)
        micp->micpHandle = cfm->prflHndl;

    if(cfm->status != MICP_STATUS_IN_PROGRESS)
        inst->micpRequestCount--;

    /* All the Init Cfms are recieved and counter Hits zero */
    if(inst->micpRequestCount == 0)
    {

        CsrCmnListElm_t *elem = (CsrCmnListElm_t*)(gInst->micpList->first);

        capClientSendOptionalServiceInitCfm(inst->appTask,
                                         inst->deviceCount,
                                         inst->activeGroupId,
                                         CAP_CLIENT_OPTIONAL_SERVICE_MICP,
                                         CAP_CLIENT_RESULT_SUCCESS,
                                         elem);

        /* Enable MICP ccd */
        capClientMicpEnableCcd(inst, TRUE);
    }
}

static void capClientHandleMuteValueCfm(const Msg msg, CAP_INST* inst, CapClientGroupInstance* gInst)
{
    MicpSetMuteValueCfm* cfm = (MicpSetMuteValueCfm*)msg;
    MicpInstElement* micp = (MicpInstElement*)
        CAP_CLIENT_GET_MICP_ELEM_FROM_PHANDLE(gInst->micpList, cfm->prflHndl);
    
    inst->micpRequestCount--;

    CAP_CLIENT_INFO("\n(CAP) : capClientHandleMuteValueCfm: micpRequestCount:  %d\n", inst->micpRequestCount);
    
    CsipInstElement* csip = (CsipInstElement*)gInst->csipList.first;

    micp->recentStatus = capClientGetCapClientResult(cfm->status, CAP_CLIENT_MICP);

    if (inst->micpRequestCount == 0)
    {
        if (capClientIsGroupCoordinatedSet(gInst))
        {
            /* If the CSIP is in Locked state and pending operation is not clear
             * then there is procedure which is already running hence just send
             * Cfm */
            if (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED
                && gInst->pendingOp == CAP_CLIENT_OP_NONE)
            {
                gInst->pendingOp = CAP_CLIENT_MICP_MUTE;
                capClientSetCsisLockState(csip, &inst->csipRequestCount, FALSE);
            }
            else
            {
                capClientSendMicMuteCfm(inst->micpProfileTask,
                    gInst->groupId,
                    gInst,
                    gInst->requestCid,
                    CAP_CLIENT_RESULT_SUCCESS);
            }
        }
        else
        {
            capClientSendMicMuteCfm(inst->micpProfileTask,
                gInst->groupId,
                gInst,
                gInst->requestCid,
                CAP_CLIENT_RESULT_SUCCESS);
        }
    }
    else if (inst->micpRequestCount & 0x80)
    {
        CAP_CLIENT_ERROR("\n(CAP) CapClientmicpRequestCount : Counter Overflow detected!! %x\n", inst->micpRequestCount);
    }
}

void capClientHandleMicMuteStateInd(MicpMuteValueInd *ind,
    CAP_INST* inst, CapClientGroupInstance* gInst)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientMicStateInd);

    MicpInstElement* micp = (MicpInstElement*)
        CAP_CLIENT_GET_MICP_ELEM_FROM_PHANDLE(gInst->micpList, ind->prflHndl);

    micp->micValue = ind->muteValue;

    CAP_CLIENT_INFO("(CAP) : Micp Handle: 0x%04x,Mute: 0x%02x \n", ind->prflHndl,ind->muteValue);

    /* Send MICP State Indication to application */
    message->groupId = inst->activeGroupId;
    message->cid = micp->cid;
    message->micState = micp->micValue;

    CapClientMessageSend(inst->profileTask, CAP_CLIENT_MIC_STATE_IND, message);
}

void capClientSetMicpMuteStateReq(MicpInstElement* micp,
                                 CAP_INST *inst,
                                 CapClientGroupInstance *cap)
{
    MicpInstElement *tmp = micp;
    CAP_CLIENT_INFO("\n(CAP) : capClientSetMicpMuteStateReq \n");

    for (; tmp; tmp = tmp->next)
    {
        if (tmp->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
        {
            inst->micpRequestCount++;
            MicpSetMuteValueReq(tmp->micpHandle, tmp->micValue);
        }
        else
        {
            CAP_CLIENT_INFO("\n capClientSetMicpMuteStateReq: Recent error :%d \n",
                                        micp->recentStatus);
        }
    }

    if (inst->micpRequestCount == 0)
    {
        tmp = (MicpInstElement*)cap->micpList->first;
        capClientSendMicMuteCfm(inst->micpProfileTask, cap->groupId, cap, cap->requestCid, CAP_CLIENT_RESULT_SUCCESS);
    }
}

void capClientHandleMicpMsg(CAP_INST *inst, const Msg msg)
{
    CsrBtGattPrim *prim = (CsrBtGattPrim *)msg;
    CapClientGroupInstance *gInst =
               (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (gInst == NULL)
    {
        CAP_CLIENT_INFO("capClientHandleMicpMsg: gInst is NULL");
        return;
    }

    switch(*prim)
    {
        case MICP_INIT_CFM:
        {
            gInst->pendingOp = CAP_CLIENT_MICP_INIT;
            capClientHandleMicpInitCfm(msg, inst, gInst);
        }
        break;

        case MICP_MICS_TERMINATE_CFM:
        {
            capClientRemoveGroup(msg, inst, CAP_CLIENT_MICP);
        }
        break;

        case MICP_DESTROY_CFM:
        {
            MicpDestroyCfm *cfm = (MicpDestroyCfm*)msg;

            if(cfm->status == MICP_STATUS_SUCCESS)
            {
                capClientRemoveGroup(NULL, inst, CAP_CLIENT_MICP);
            }
        }
        break;

        case MICP_NTF_CFM:
        {
            CAP_CLIENT_INFO("(LEA) : MICP_NTF_CFM  Status\n");
        }
        break;

        case MICP_READ_MUTE_VALUE_CFM:
        {
            MicpReadMuteValueCfm* cfm = (MicpReadMuteValueCfm*)msg;
            MicpInstElement* micp = (MicpInstElement*)
            CAP_CLIENT_GET_MICP_ELEM_FROM_PHANDLE(gInst->micpList, cfm->prflHndl);
            CapClientResult result;

            micp->micValue = cfm->muteValue;
            result = capClientGetCapClientResult(cfm->status, CAP_CLIENT_MICP);

            capCLientSendReadMicStateCfm(inst->micpProfileTask,
                                           inst->activeGroupId,
                                           result,
                                           micp,
                                           cfm->muteValue);
        }
        break;

        case MICP_READ_MUTE_VALUE_CCC_CFM:
        {
            CAP_CLIENT_INFO("(LEA) : MICP_READ_MUTE_VALUE_CCC_CFM  Status\n");
        }
        break;

        case MICP_MUTE_VALUE_IND:
        {
            MicpMuteValueInd* ind = (MicpMuteValueInd*)msg;
            capClientHandleMicMuteStateInd(ind, inst, gInst);
        }
        break;

        case MICP_SET_MUTE_VALUE_CFM:
        {
            capClientHandleMuteValueCfm(msg, inst, gInst);
        }
        break;

        default:
            CAP_CLIENT_INFO("capClientHandleMicpMsg: unhandled message %x", *prim);
            break;
    }
}


/* This is utility function for enabling/disabling VCP CCD  */
void capClientMicpEnableCcd(CAP_INST *inst, uint8 enable)
{
    CapClientGroupInstance* cap = CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);
    MicpInstElement* micp = NULL;

    if (cap)
    {
        micp = (MicpInstElement*)cap->micpList->first;
    }

    while (micp && micp->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n(CAP) : capClientMicpEnableCcd \n");
        MicpRegisterForNotificationReq(micp->micpHandle, enable);
        micp = micp->next;
    }
}
#endif
