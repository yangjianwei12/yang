/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_available_audio_context_req.h"
#include "cap_client_util.h"
#include "cap_client_csip_handler.h"
#include "cap_client_common.h"
#include "cap_client_debug.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void capClientSendDiscoverAvailableAudioContextCfm(CAP_INST *inst,
                                           CapClientGroupInstance *gInst,
                                           CapClientResult result)
{
    uint8 i;
    BapInstElement *iter;
    CapClientProfileMsgQueueElem* msgElem = NULL;
    MAKE_CAP_CLIENT_MESSAGE(CapClientDiscoverAvailableAudioContextCfm);

    message->groupId = inst->activeGroupId;
    message->status = result;
    message->deviceContextLen = inst->deviceCount;

    message->deviceContext = (CapClientAvailableAudioContextInfo*)
                            CsrPmemZalloc(sizeof(CapClientAvailableAudioContextInfo)*message->deviceContextLen);

    if (gInst)
    {
        CAP_CLIENT_CLEAR_PENDING_OP(gInst->pendingOp);
        CAP_CLIENT_INFO("\n(CAP) DiscoverAvailableAudioContextCfm: BAP List Count: %d \n", gInst->bapList.count);
    }

    CAP_CLIENT_INFO("\n(CAP) DiscoverAvailableAudioContextCfm: Device Count: %d \n", inst->deviceCount);

    if (result == CAP_CLIENT_RESULT_SUCCESS && gInst)
    {
        iter = (BapInstElement*)(gInst->bapList.first);

        for (i = 0; i < message->deviceContextLen && iter; i++ )
        {
            message->deviceContext[i].cid = iter->bapHandle;
            message->deviceContext[i].context = iter->availableAudioContext;
            message->deviceContext[i].result = iter->recentStatus;
            iter = iter->next;
        }
    }

    CapClientMessageSend(inst->appTask, CAP_CLIENT_DISCOVER_AVAILABLE_AUDIO_CONTEXT_CFM, message);

    /*
     * If the cfm was success and message queue is not
     * empty i.e msgElem is not NULL, handle the next
     * message
     *
     */
     /* Save CAP state and get next message */
    if (gInst)
    {
        msgElem = capClientGetNextMsgElem(gInst);
    }

    if (msgElem)
    {
        msgElem->handlerFunc(inst, (void*)msgElem, gInst);
    }

}

void capClientSendDiscoverAvailableAudioContextReq(CAP_INST *inst,
                                          BapInstElement *bap,
                                          CapClientGroupInstance *gInst)
{
    BapInstElement *iter = bap;

    for (; iter; iter = iter->next)
    {
        CAP_CLIENT_INFO("\n(BAP)capClientSendDiscoverAvailableAudioContextReq: Current BAP state : %d!! \n", iter->bapCurrentState);

        if (CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(iter, CAP_CLIENT_NO_CIG_ID_MASK) == CAP_CLIENT_BAP_STATE_DISCOVER_COMPLETE)
        {
                inst->bapRequestCount++;
                BapDiscoverAudioContextReq(iter->bapHandle, BAP_PAC_AVAILABLE_AUDIO_CONTEXT);
        }
        else
        {
            CAP_CLIENT_INFO("\n(BAP)capClientSendDiscoverAvailableAudioContextReq: Already discovered!! \n");
        }
    }

    if (inst->bapRequestCount == 0)
    {
        /* Send BAP audio context cfm*/
        capClientSendDiscoverAvailableAudioContextCfm(inst, gInst, CAP_CLIENT_RESULT_SUCCESS);
    }
}

void capClientHandleRemoteAvailableContextCfm(CAP_INST *inst,
                             BapDiscoverAudioContextCfm* cfm,
                             CapClientGroupInstance *gInst)
{
    BapInstElement *bap = CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(gInst->bapList, cfm->handle);

    /* Update Result*/
    inst->bapRequestCount--;
    bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);
    setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_AVLBLE_AUDIO_CONTEXT, CAP_CLIENT_BAP_STATE_INVALID);

    bap->availableAudioContext = (cfm->contextValue.sinkContext & 0x0000FFFF)
                                        | (cfm->contextValue.sourceContext << 16 & 0xFFFF0000);


    /* Send the Confirmation for all devices when request counter Hits zero*/
    if(inst->bapRequestCount == 0)
    {
        /* Send BAP audio context cfm*/
        capClientSendDiscoverAvailableAudioContextCfm(inst, gInst, CAP_CLIENT_RESULT_SUCCESS);
    }
}

static void capClientDiscoverAvailableAudioContextReqHandler(CAP_INST* inst,
                                          void* msg,
                                          CapClientGroupInstance* cap)
{

    BapInstElement *bap = (BapInstElement*) ((CsrCmnListElm_t*) (cap->bapList.first));
    capClientSendDiscoverAvailableAudioContextReq(inst, bap, cap);

    CSR_UNUSED(msg);
}


void handleCapClientDiscoverAvailableAudioContextReq(CAP_INST *inst, const Msg msg)
{
    /*
     * First Check if the Group is A co-ordinated Set
     * Note: Group is a co ordinated Set if the CSIS instance has
     * setSize of more than 1 and has a valid CSIP handle
     * */

     CapClientGroupInstance *cap = NULL;
     CapClientInternalDiscoverAvailAudioContextReq *req =
                        (CapClientInternalDiscoverAvailAudioContextReq*)msg;
     CapClientProfileTaskListElem* task = NULL;
     CapClientProfileMsgQueueElem* msgElem = NULL;
     CapClientBool isQueueEmpty = FALSE;

     /* if groupId is not same Switch the group
      *
      * Note: capClientSetNewActiveGroup sends CapClientActiveGroupChangeInd to the
      * application internally
      * */

     cap = capClientSetNewActiveGroup(inst, req->groupId, FALSE);


     /* If Group Id does not match with the list of cap groupiDs sned
      * Send CAP_RESULT_INVALID_GROUPID
      * */

     if (cap == NULL)
     {
         capClientSendDiscoverAvailableAudioContextCfm(inst, cap, CAP_CLIENT_RESULT_INVALID_GROUPID);
         return;
     }

     if (inst->streaming && req->groupId != inst->activeGroupId )
     {
          capClientSendDiscoverAvailableAudioContextCfm(inst, cap, CAP_CLIENT_RESULT_INVALID_OPERATION);
          return;
     }

     /* Add Check the message queue is empty
      *
      *  If the queue is empty, add the message to queue and
      *  proceed to process the request, else add the message and return.
      *  Queued message will be processed once current message being processed
      *  receives the cfm from lower layers
      */

     isQueueEmpty = CAP_CLIENT_IS_MSG_QUEUE_EMPTY(cap->capClientMsgQueue);

     msgElem = CapClientMsgQueueAdd(&cap->capClientMsgQueue, (void*)req, 0,
                                   req->type, capClientDiscoverAvailableAudioContextReqHandler, task);

     if (isQueueEmpty)
     {
         capClientDiscoverAvailableAudioContextReqHandler(inst, (void*)msgElem, cap);
     }
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */
