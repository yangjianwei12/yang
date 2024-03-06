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
#include "cap_client_bap_handler.h"
#include "cap_client_update_audio_req.h"
#include "cap_client_debug.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
static void capClientPoplulateUnicastAudioUpdateReqParam(uint8 aseId,
                                                  uint8 metadataLen,
                                                  uint8* metadata,
                                                  uint16 useCase,
                                                  BapAseMetadataParameters *param)
{
    param->aseId  = aseId;
    param->metadataLen = metadataLen;
    param->streamingAudioContexts = capClientMapCapContextWithCap(useCase);
    param->metadata = NULL;

    if (metadataLen)
    {
        param->metadata = (uint8*)CsrPmemZalloc(sizeof(uint8) * metadataLen);
        CsrMemCpy(param->metadata, metadata, metadataLen);
    }
}

static void capClientFreeMetadataParam(uint8 paramCount, BapAseMetadataParameters *param)
{
    uint8 i;

    for (i = 0; i < paramCount; i++)
    {
        if (param[i].metadataLen && param[i].metadata)
        {
            CsrPmemFree(param[i].metadata);
            param[i].metadata = NULL;
        }
    }

}

static void capClientBuildAndUnicastAudioUpdateReq(BapInstElement *bap,
                                         CapClientGroupInstance *cap,
                                         CAP_INST *inst)
{
    uint8 i;
    BapAseMetadataParameters *param = NULL;
    uint8 aseCounter = 0;
    uint8 offset = 0;
    BapAseElement* ase = (BapAseElement*)(bap->sinkAseList.first);;

    if (cap->activeCig == NULL)
    {
        CAP_CLIENT_INFO("\n handleUnicastUpdateAudioReq: Not Configured \n");
        capClientUpdateAudioCfmSend(inst->profileTask, inst,
                               CAP_CLIENT_RESULT_NOT_CONFIGURED,
                              0, NULL, cap);
        return;
    }
    
    bap->asesInUse = aseCounter = capClientGetAseCountForUseCase(bap, cap->activeCig->context);

    /* At this point aseCounter should not be zero */

    if (aseCounter == 0)
    {
        CAP_CLIENT_INFO("\n handleUnicastUpdateAudioReq: No Configured Ases \n");
        capClientUpdateAudioCfmSend(inst->profileTask, inst,
                                  CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR, 
                                  0, NULL, cap);
        return;
    }

    param = (BapAseMetadataParameters*)CsrPmemZalloc(sizeof(BapAseMetadataParameters) * aseCounter);

    /* Populate with Sink ASES */

    for (i = 0; i < bap->sinkAseCount && ase && aseCounter; i++)
    {
        if (ase->state == BAP_ASE_STATE_ENABLING || ase->state == BAP_ASE_STATE_STREAMING)
        {
            capClientPoplulateUnicastAudioUpdateReqParam(ase->aseId,
                                                       cap->metadataLen,
                                                       cap->metadata,
                                                       cap->useCase,
                                                       &param[i]);
            ase->useCase = cap->useCase;
            aseCounter--;
        }
        ase = ase->next;
    }

    /* Populate with Source ASES */

    ase = (BapAseElement*)(bap->sourceAseList.first);
    offset = bap->asesInUse - aseCounter;

    for (i = 0; i < bap->sourceAseCount && ase && aseCounter; i++)
    {
        if (ase->state == BAP_ASE_STATE_ENABLING || ase->state == BAP_ASE_STATE_STREAMING)
        {
            capClientPoplulateUnicastAudioUpdateReqParam(ase->aseId,
                                                       cap->metadataLen,
                                                       cap->metadata,
                                                       cap->useCase,
                                                       &param[i + offset]);
            ase->useCase = cap->useCase;
            aseCounter--;
        }
        ase = ase->next;
    }

    /* At this point aseCounter has to be zero.
     * Useful in handling overflow/ underflow scenario */

    if (aseCounter != 0)
    {
        offset = bap->asesInUse - aseCounter;
        CAP_CLIENT_INFO("\n handleUnicastUpdateAudioReq: Mismatch!, Ase Offset: %d, In Use Count: %d, AseCounter: %d \n",
                                                                        offset, bap->asesInUse, aseCounter);

        /* Free the constructed param */
        capClientFreeMetadataParam(offset, param);
        CsrPmemFree(param);
        param = NULL;

        capClientUpdateAudioCfmSend(inst->profileTask,inst,
                                   CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR,
                                   0, NULL, NULL);
        return;
    }

    BapUnicastClientUpdateMetadataReq(bap->bapHandle, bap->asesInUse, param);
    inst->bapRequestCount++;

    /* Dont free metadata copied here                    
     * since pointer gets copied inside BAP and not data */

    CsrPmemFree(param);
    param = NULL;
}

void capClientHandleUpdateMetadataInd(CAP_INST *inst,
                                BapUnicastClientUpdateMetadataInd *ind,
                                CapClientGroupInstance* cap)
{
    CSR_UNUSED(inst);
    BapAseElement *ase = NULL;
    BapInstElement *bap = (BapInstElement*)
                              CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, ind->handle);
    bool isSink = FALSE;

    if(ind->result != BAP_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n capClientHandleUpdateMetadataInd: Failed to configure aseId: %d", ind->aseId);
    }

    ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sinkAseList, ind->aseId);

    if (ase == NULL)
    {
        ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sourceAseList, ind->aseId);
    }
    else
        isSink = TRUE;

    if(ase == NULL)
    {
        CAP_CLIENT_ERROR("\n ASE is null \n");
        return;
    }

    if (ase && (ind->result == CAP_CLIENT_RESULT_SUCCESS))
        ase->useCase = ind->streamingAudioContexts;

    if (ase && ind->metadataLength  && ind->metadata)
    {
        MAKE_CAP_CLIENT_MESSAGE(CapClientUpdateMetadataInd);
        message->cid = ind->handle;
        message->isSink = isSink;
        message->groupId = inst->activeGroupId;
        message->result = ind->result;
        message->streamingAudioContexts = ind->streamingAudioContexts;
        
		if(ind->metadataLength)
        {
            message->metadata = ind->metadata;
            message->metadataLen = ind->metadataLength;
        }

        CapClientMessageSend(inst->profileTask, CAP_CLIENT_UPDATE_METADATA_IND, message);
    }

}

void capClientUpdateAudioCfmSend(AppTask appTask,
                            CAP_INST *inst,
                            CapClientResult result,
                            CapClientContext useCase,
                            BapInstElement *bap,
                            CapClientGroupInstance *cap)
{
    CapClientProfileMsgQueueElem* msgElem = NULL;
    uint8 i = 0;
    MAKE_CAP_CLIENT_MESSAGE(CapClientUnicastAudioUpdateCfm);
    message->context = useCase;
    message->deviceStatusLen = 0;
    message->deviceStatus = NULL;
    message->groupId = inst->activeGroupId;
    message->result = result;

    if (cap && bap)
    {
        if (capClientManageError(bap, cap->bapList.count))
            result = CAP_CLIENT_RESULT_INVALID_PARAMETER;
    }

    if(result == CAP_CLIENT_RESULT_SUCCESS && cap && bap)
    {
        message->deviceStatusLen = inst->deviceCount;
        message->deviceStatus = (CapClientDeviceStatus*)
                CsrPmemZalloc(sizeof(CapClientDeviceStatus)*inst->deviceCount);


        /* NOTE: Internal API which populates the above details needs to be tested with multiple Device
         *       i.e Standard LE
         */

        for (i = 0; i < inst->deviceCount && bap; i++, bap = bap->next)
        {
            message->deviceStatus[i].cid = bap->bapHandle;
            message->deviceStatus[i].result = bap->recentStatus;
        }

        cap->capState = CAP_CLIENT_STATE_AUDIO_UPDATED;
        CAP_CLIENT_CLEAR_PENDING_OP(cap->pendingOp);
    }

    CapClientMessageSend(appTask, CAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM ,message);

    /*
     * If the cfm was success and message queue is not
     * empty i.e msgElem is not NULL, handle the next
     * message
     *
     */


    if (cap)
        msgElem = capClientGetNextMsgElem(cap);

    if (msgElem)
    {
        msgElem->handlerFunc(inst, (void*)msgElem, cap);
    }
}

void capClientHandleUpdateMetadataCfm(CAP_INST *inst,
                                BapUnicastClientUpdateMetadataCfm *cfm,
                                CapClientGroupInstance* cap)
{
    BapInstElement *bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);

    bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);
    inst->bapRequestCount--;

    if (cfm->result == CAP_CLIENT_RESULT_SUCCESS)
        cap->activeCig->context = cap->useCase;
    /*
     * When bapRequestCount hits zero, the build and
     * send update audio cfm
     * */

    if(inst->bapRequestCount == 0)
    {
         /* Check if the Cap Instance belongs to Coordinated Set */
        CsipInstElement *csip = (CsipInstElement*)(cap->csipList.first);
        bap = (BapInstElement*)(cap->bapList.first);

        if(capClientIsGroupCoordinatedSet(cap) && (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED))
        {
            capClientSetCsisLockState(csip, &inst->csipRequestCount, FALSE);
        }
        else
        {
            capClientUpdateAudioCfmSend(inst->profileTask, inst, 
                                      CAP_CLIENT_RESULT_SUCCESS, 
                                      cap->useCase, bap, cap);
        }
    }
}


void capClientSendUnicastAudioUpdateReq(BapInstElement *bap,
                                  CapClientGroupInstance *cap,
                                  CAP_INST *inst)
{
    CsipInstElement* csip = NULL;

    if (cap == NULL)
    {
        CAP_CLIENT_ERROR("(CAP) NULL Instance \n");
        return;
    }

    if(bap == NULL)
    {
         CAP_CLIENT_INFO("(CAP) NULL Instance \n");
    }

    while (bap)
    {
        csip = CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(cap->csipList, bap->bapHandle);

        if (bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS && (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED))
        {
            capClientBuildAndUnicastAudioUpdateReq(bap, cap, inst);
        }
        else
        {
            CAP_CLIENT_INFO("(CAP)capClientSendUnicastAudioUpdateReq: recent Failure at Device 0x%x \n", bap->bapHandle);
        }

        bap = bap->next;
    }
}

static void capClientUnicastUpdateAudioReqHandler(CAP_INST* inst,
                                               void* msg,
                                               CapClientGroupInstance* cap)
{
    CsipInstElement* csip;
    BapInstElement* bap;
    CapClientResult result;

    CapClientProfileMsgQueueElem* msgElem = (CapClientProfileMsgQueueElem*)msg;
    CapClientInternalUnicastUpdateAudioReq* req = (CapClientInternalUnicastUpdateAudioReq*)(msgElem->capMsg);

    /* Initialize the CAP with task specific parameters */
    inst->profileTask = req->profileTask;

    /* 
     * Do the Context availability check in queuehandler.
     *
     * Note: Context availabilty check is performed here since there can be a stop request
     *       before queue which can actually free that context, hence making the context available 
     *       while servicing this request.
     * 
     */

    if (!capClientIsContextAvailable(req->useCase, cap, TRUE)
          && (!capClientIsContextAvailable(req->useCase, cap, FALSE)
             && cap->numOfSourceAses))
    {
        CAP_CLIENT_INFO("\n(CAP) handleUnicastUpdateAudioReq: Context unavailable!");
        result = CAP_CLIENT_RESULT_CONTEXT_UNAVAILABLE;
        capClientUpdateAudioCfmSend(req->profileTask, inst, result, 0, NULL, NULL);
        return;
    }

    /* Do CAP state validation and state checks here */

    result = capClientValidateCapState(cap->capState, req->type);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        capClientUpdateAudioCfmSend(inst->profileTask, inst, result, 0, NULL, cap);
        CAP_CLIENT_INFO("\n handleUnicastUpdateAudioReq: invalid state transition! \n");
        return;
    }

    /* Check if given use case is supported for the configured frequency*/

    if (!capClientIsConfigSupportedByServer(cap, cap->activeCig->sinkConfig, req->useCase, BAP_ASE_SINK, NULL)
          && (!capClientIsConfigSupportedByServer(cap, cap->activeCig->srcConfig, req->useCase, BAP_ASE_SOURCE, NULL)
            && cap->numOfSourceAses))
    {
        CAP_CLIENT_INFO("\n(CAP) handleUnicastUpdateAudioReq: Usecase not supported!");
        result = CAP_CLIENT_RESULT_NOT_SUPPORTED;
        capClientUpdateAudioCfmSend(req->profileTask, inst, result, 0, NULL, NULL);
        return;
    }

    /* co ordinated set?
    *
    * Based on if co ordinated Set or not decide number of ASEs required
    * and then start BAP procedures
    *
    * */

    bap = (BapInstElement*)(cap->bapList.first);

    /* Store the metadata Param and then Obtain Lock on
     * all the Devices */
    cap->metadataLen = (uint8)req->metadataLen;
    cap->metadata = req->metadataParam;
    cap->useCase = req->useCase;

    if (capClientIsGroupCoordinatedSet(cap))
    {
        csip = (CsipInstElement*)(cap->csipList.first);

        /* Here we need to obtain lock on all the devices and the
         * Start BAP unicast Procedures*/

         /* check if the profile is already locked
          *
          * Note: If one device is in lock state in a group
          * it's assumed that all other participants are in lock state*/

        cap->pendingOp = CAP_CLIENT_BAP_UNICAST_AUDIO_UPDATE;

        if (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            /* Directly Send AudioUpdate */
            capClientSendUnicastAudioUpdateReq(bap, cap, inst);
        }

        /* Otherwise obtain lock and the start BAP Procedures */
        else
        {
            capClientSetCsisLockState(csip, &inst->csipRequestCount, TRUE);
        }
    }
    else
    {
        /* Directly Send AudioUpdate */
        capClientBuildAndUnicastAudioUpdateReq(bap, cap, inst);
    }
}


void handleUnicastUpdateAudioReq(CAP_INST* inst, const Msg msg)
{
    AppTask appTask;
    uint16 result;
    CapClientInternalUnicastUpdateAudioReq* req = (CapClientInternalUnicastUpdateAudioReq*)msg;
    CapClientGroupInstance* cap = NULL;
    CapClientProfileTaskListElem* task = NULL;
    CapClientProfileMsgQueueElem* msgElem = NULL;
    CapClientBool isQueueEmpty = FALSE;

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);
    appTask = req->profileTask;

    if (cap == NULL)
    {
        capClientUpdateAudioCfmSend(appTask, inst,
                                  CAP_CLIENT_RESULT_INVALID_GROUPID,
                                  0, NULL, NULL);
        return;
    }

    if (req->groupId != inst->activeGroupId)
    {
        result = CAP_CLIENT_RESULT_INVALID_GROUPID;
        CAP_CLIENT_INFO("\n handleUnicastUpdateAudioReq: Unable to change GroupId !\n");
        capClientUpdateAudioCfmSend(appTask, inst, result, 0, NULL, cap);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/
    task = (CapClientProfileTaskListElem*)
        CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

    if (task == NULL)
    {
        capClientUpdateAudioCfmSend(appTask, inst,
                                   CAP_CLIENT_RESULT_TASK_NOT_REGISTERED,
                                   0, NULL, cap);
        return;
    }

    if (cap->activeCig == NULL)
    {
        result = CAP_CLIENT_RESULT_NOT_CONFIGURED;
        CAP_CLIENT_INFO("\n handleUnicastUpdateAudioReq: CIG not configured!");
        capClientUpdateAudioCfmSend(appTask, inst, result, 0, NULL, cap);
        return;
    }

    if (CAP_CLIENT_ASES_NOT_CONFIGURED(cap->activeCig->context, 
                                       cap->activeCig->srcConfig, 
                                       cap->activeCig->sinkConfig))
    {
        result = CAP_CLIENT_RESULT_INVALID_OPERATION;
        CAP_CLIENT_INFO("\n handleUnicastUpdateAudioReq: ASES not configured!");
        capClientUpdateAudioCfmSend(inst->profileTask, inst, result, 0, NULL, cap);
        return;
    }

    isQueueEmpty = CAP_CLIENT_IS_MSG_QUEUE_EMPTY(cap->capClientMsgQueue);

    msgElem = CapClientMsgQueueAdd(&cap->capClientMsgQueue,
                                  (void*)req,
                                  0,
                                  req->type,
                                  capClientUnicastUpdateAudioReqHandler,
                                  task);

    if (isQueueEmpty)
    {
        capClientUnicastUpdateAudioReqHandler(inst, (void*)msgElem, cap);
    }
}
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
