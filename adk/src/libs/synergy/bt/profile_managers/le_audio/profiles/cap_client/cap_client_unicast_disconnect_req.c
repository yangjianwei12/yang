/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/
#include "cap_client_common.h"
#include "cap_client_util.h"
#include "cap_client_ase.h"
#include "cap_client_csip_handler.h"
#include "cap_client_bap_pac_record.h"
#include "cap_client_debug.h"
#include "cap_client_unicast_disconnect_req.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
static void capClientUnicastDisconnectionReqHandler(CAP_INST* inst,
                                                    void* msg,
                                                    CapClientGroupInstance* cap)
{
    CsipInstElement* csip;
    CapClientResult result;
    CapClientProfileMsgQueueElem* msgElem = (CapClientProfileMsgQueueElem*)msg;
    CapClientInternalUnicastDisconnectReq* req = (CapClientInternalUnicastDisconnectReq*)(msgElem->capMsg);
    CapClientCigElem* cig = NULL;


    /* Do all State related checks in queueHandler */

    result = capClientValidateCapState(cap->capState, req->type);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        capClientSendUnicastClientDisConnectCfm(inst->profileTask, inst, cap, result);
        CAP_CLIENT_INFO("\n handleUnicastStopStreamReq: invalid state transition \n");
        return;
    }

    cig = (CapClientCigElem*)CAP_CLIENT_GET_CIG_FROM_CONTEXT(cap->cigList, req->useCase);

    if (cig == NULL)
    {
        result = CAP_CLIENT_RESULT_NOT_CONFIGURED;
        capClientSendUnicastClientDisConnectCfm(inst->profileTask, inst, cap, result);
        CAP_CLIENT_INFO("\n handleUnicastStopStreamReq: Not configured for context 0x%x \n", req->useCase);
        return;
    }

    /* Check if any*/
    cap->useCase = req->useCase;
    cap->stopComplete = FALSE;

    if (capClientIsGroupCoordinatedSet(cap))
    {
        csip = (CsipInstElement*)(cap->csipList.first);

        /* Here we need to obtain lock on all the devices and the
         * Start BAP unicast Procedures
         * */

         /* check if the profile is already locked
          *
          * Note: If one device is in lock state in a group
          * it's assumed that all other participants are in lock state
          * */

         /* Send internal Pending operation Request  */

        if ((csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED))
        {
            capClientSendInternalPendingOpReq(CAP_CLIENT_UNICAST_DISCONNECT);
        }
        /* Otherwise obtain lock and the start BAP Procedures */
        else
        {
            /* Store the metadata Param and then Obtain Lock on
             * all the Devices */
            cap->pendingOp = CAP_CLIENT_UNICAST_DISCONNECT;
            capClientSetCsisLockState(csip, &inst->csipRequestCount, TRUE);
        }
    }
    else
    {
        /* Send internal Pending operation Request  */
        capClientSendInternalPendingOpReq(CAP_CLIENT_UNICAST_DISCONNECT);
    }
}


void capClientSendUnicastClientDisConnectCfm(AppTask appTask,
                                             CAP_INST* inst,
                                             CapClientGroupInstance* cap,
                                             CapClientResult result)
{

    CapClientProfileMsgQueueElem* msgElem = NULL;
    MAKE_CAP_CLIENT_MESSAGE(CapClientUnicastDisConnectCfm);
    BapInstElement* bap = NULL;

    message->groupId = inst->activeGroupId;

    /* Reassign the current active context to CAP */
    if (cap && cap->activeCig)
    {
        cap->useCase = cap->activeCig->context;
    }

    /* If overall result is success check if procedure succeeded for all devices*/
    
    message->result = result;

    if (cap && result == CAP_CLIENT_RESULT_SUCCESS)
    {
        bap = (BapInstElement*)cap->bapList.first;

        /* Clear all the BAP and ASE variables*/
        while(bap)
        {
            capClientClearAllStreamVariables(bap);

            capClientStopStreamIterateAses(bap, cap->useCase);

            bap = bap->next;
        }


        /* If there is failure even in any one of the constituent devices
         * We return CAP_CLIENT_RESULT_FAILURE_BAP_ERR to above layer indicating the failure in 
         * execution of procedure */

        while (bap)
        {
            if (bap->recentStatus)
            {
                message->result = CAP_CLIENT_RESULT_FAILURE_BAP_ERR;
                break;
            }
            bap = bap->next;
        }
    }

    CapClientMessageSend(appTask, CAP_CLIENT_UNICAST_DISCONNECT_CFM, message);

    /*
     * If the cfm was success and message queue is not
     * empty i.e msgElem is not NULL, handle the next
     * message
     *
     */


     /* Save CAP state and get next message */
     /* Pop the current message and service next after sending cfm */

    if (cap)
        msgElem = capClientGetNextMsgElem(cap);

    if (msgElem)
    {
        msgElem->handlerFunc(inst, (void*)msgElem, cap);
    }

}

void handleUnicastDisconnectReq(CAP_INST* inst, const Msg msg)
{
    CapClientInternalUnicastDisconnectReq* req = (CapClientInternalUnicastDisconnectReq*)msg;
    CapClientResult result;
    CapClientProfileTaskListElem* task = NULL;
    CapClientGroupInstance* cap = NULL;
    CapClientBool isQueueEmpty = FALSE;
    CapClientProfileMsgQueueElem* msgElem = NULL;

    /* if groupId is not same Switch the group
     *
     * Note: capSetNewActiveGroup sends CapActiveGroupChangeInd to the
     * application internally
     *
     * */
    cap = capClientSetNewActiveGroup(inst, req->groupId, FALSE);
    inst->profileTask = req->profileTask;
    /* If Group Id does not match with the list of cap groupiDs
     * Send CAP_CLIENT_RESULT_INVALID_GROUPID
     * */

    if (cap == NULL)
    {
        capClientSendUnicastClientDisConnectCfm(inst->profileTask, inst, NULL, CAP_CLIENT_RESULT_INVALID_GROUPID);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/
    task = (CapClientProfileTaskListElem*)
        CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

    if (task == NULL)
    {
        result = CAP_CLIENT_RESULT_TASK_NOT_REGISTERED;
        capClientSendUnicastClientDisConnectCfm(inst->profileTask, inst, cap, result);
        return;
    }

    isQueueEmpty = CAP_CLIENT_IS_MSG_QUEUE_EMPTY(cap->capClientMsgQueue);

    msgElem = CapClientMsgQueueAdd(&cap->capClientMsgQueue,
                                  (void*)req,
                                  0,
                                  req->type,
                                  capClientUnicastDisconnectionReqHandler,
                                  task);

    if (isQueueEmpty)
    {
        capClientUnicastDisconnectionReqHandler(inst, (void*)msgElem, cap);
    }

}
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
