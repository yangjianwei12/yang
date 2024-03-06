/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_start_stream_req.h"
#include "cap_client_common.h"
#include "cap_client_util.h"
#include "cap_client_ase.h"
#include "cap_client_csip_handler.h"
#include "cap_client_bap_pac_record.h"
#include "cap_client_debug.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
static void capClientBapCisConnectReqSend(BapInstElement* bap, 
                                          CapClientGroupInstance* cap,
                                          CAP_INST* inst);

/******************************************************************************************************/
/***************                   BapUnicastAseEnableReq              ********************************/
/******************************************************************************************************/
static uint8 capClientBuildMultipleAseEnableQuery(BapAseElement *ase,
                                           CapClientContext useCase,
                                           uint16 metadataLen,
                                           uint8 *metadataParam,
                                           BapAseEnableParameters *param,
                                           uint8 queriesBuilt)
{

    if(ase == NULL || param == NULL)
    {
        CAP_CLIENT_INFO("\n capBuildMultipleAseEnableQuery: NULL \n");
        return 0;
    }

    if (metadataParam == NULL)
    {
        CAP_CLIENT_INFO("\n capBuildMultipleAseEnableQuery: NULL Metadata \n");
    }

    while(ase)
    {
        if(ase->state == BAP_ASE_STATE_QOS_CONFIGURED && ase->useCase == useCase)
        {
            param[queriesBuilt].aseId = ase->aseId;
            param[queriesBuilt].streamingAudioContexts = capClientMapCapContextWithCap(useCase);
            param[queriesBuilt].metadataLen = (uint8)metadataLen;
            param[queriesBuilt].metadata = NULL;

            if (metadataLen && metadataParam)
            {
                param[queriesBuilt].metadata = (uint8*)CsrPmemZalloc(metadataLen * sizeof(uint8));
                CsrMemCpy(param[queriesBuilt].metadata, metadataParam, metadataLen);
            }
            queriesBuilt++;
        }
        ase = ase->next;
    }

    return queriesBuilt;
}

static uint8 deviceCountForSinkSrcAse(CapClientGroupInstance *cap)
{
    /* Check for All the devices in the group reciever start ready indication is received 
     * And start stream confirmation is not yet send to the application */
    BapInstElement *bap = (BapInstElement *)cap->bapList.first;
    uint8 deviceCount = 0;

    while (bap)
    {
        /* Check for All the devices in the group Rx start ready indication is recieved from remote
         * Except dummy devices which are not yet added in the group  */
        if ((bap->serverSinkSourceStreamCount == 0 &&
           (bap->bapCurrentState & CAP_CLIENT_BAP_STATE_INVALID) != CAP_CLIENT_BAP_STATE_INVALID))
        {
            deviceCount++;
        }

        bap = bap->next;
    }
    return deviceCount;
}

static void callUnicastStartStreamCfm(CapClientGroupInstance *cap,
                                                   CAP_INST *inst)
{
    /* Check if the Cap Instance belongs to Co ordinated Set */
    /* Unlock the CSIS*/
    if (capClientIsGroupCoordinatedSet(cap))
    {
        CsipInstElement* csip = (CsipInstElement*)(cap->csipList.first);

        cap->pendingOp = CAP_CLIENT_BAP_UNICAST_START_STREAM;
        capClientSetCsisLockState(csip, &inst->csipRequestCount, FALSE);
    }
    else
    {
        capClientSendUnicastStartStreamCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_SUCCESS);
    }
}

void capClientBapAseEnableReq(BapInstElement *bap,
                              CapClientGroupInstance *cap,
                              CAP_INST* inst)
{
    BapAseElement *ase = NULL;
    uint8 numOfAses = capClientGetAseCountForUseCase(bap, cap->useCase);
    uint8 queriesBuilt = 0;
    CapClientContext context = 0;

    BapAseEnableParameters *param = (BapAseEnableParameters*)
                             CsrPmemZalloc(numOfAses*sizeof(BapAseEnableParameters));

    if ((queriesBuilt < numOfAses) &&
        (CAP_CLIENT_CIS_IS_UNI_SINK(cap->cigDirection)
            || CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection)))
    {
        context = (cap->useCase &  (CAP_CLIENT_UNIDIRECTIONAL_SINK_USECASE |
                                  CAP_CLIENT_BIDIRECTIONAL_USECASE | 
                                      CAP_CLIENT_UNIDIRECTIONAL_SRC_USECASE));
        ase = (BapAseElement*)(bap->sinkAseList.first);

        queriesBuilt = capClientBuildMultipleAseEnableQuery(ase, context,
                                 cap->metadataLen, cap->metadata, param, queriesBuilt);
    }

    if(queriesBuilt < numOfAses &&
        (CAP_CLIENT_CIS_IS_UNI_SRC(cap->cigDirection)
            || CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection)))
    {
        context = (cap->useCase & (CAP_CLIENT_UNIDIRECTIONAL_SINK_USECASE |
                                 CAP_CLIENT_BIDIRECTIONAL_USECASE |
                                    CAP_CLIENT_UNIDIRECTIONAL_SRC_USECASE));

        ase = (BapAseElement*)(bap->sourceAseList.first);

        queriesBuilt = capClientBuildMultipleAseEnableQuery(ase, context,
                                              cap->metadataLen, cap->metadata, param, queriesBuilt);
    }

    CAP_CLIENT_INFO("\n capClientBapAseEnableReqSend: reqCount: %d,  Num of Ase: %d \n", queriesBuilt, numOfAses);

    inst->bapRequestCount++;
    BapUnicastClientEnableReq(bap->bapHandle, numOfAses, param);
    CsrPmemFree(param);
    param = NULL;
}



void capClientHandleUnicastAseEnableInd(CAP_INST *inst,
                             BapUnicastClientEnableInd* ind,
                             CapClientGroupInstance *cap)
{
    CSR_UNUSED(inst);
    BapAseElement *ase = NULL;
    BapInstElement *bap = (BapInstElement*)
                              CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, ind->handle);
    bool isSink = FALSE;
    /* if use case is Voice search in both sink and source list
     * else search only in sink list
     * */

    bap->recentStatus = capClientGetCapClientResult(ind->result, CAP_CLIENT_BAP);

    bap->serverSinkSourceStreamCount++;

    CAP_CLIENT_INFO("\n capHandleUnicastAseEnableInd: aseId: %x bap->serverSinkSourceStreamCount: %x", ind->aseId, bap->serverSinkSourceStreamCount);

    ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sinkAseList, ind->aseId);

    if(ase == NULL)
    {
        ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sourceAseList, ind->aseId);
    }
    else
    {
        isSink = TRUE;
    }


    if(ase == NULL)
    {
        CAP_CLIENT_ERROR("\n ASE is null \n");
        return;
    }

    /* Copy the metadata if metadata is present */
    capClientCopyMetadataToBap(isSink, ind->metadataLength, ind->metadata, bap);
 
    ase->cisId = ind->cisId;
    ase->state = ind->aseState;
    ase->useCase = cap->useCase;
    ase->inUse = TRUE;

    if (ind->metadataLength)
    {
        CsrPmemFree(ind->metadata);
        ind->metadata = NULL;
    }
}

void capClientHandleUnicastAseEnableCfm(CAP_INST *inst,
                             BapUnicastClientEnableCfm* cfm,
                             CapClientGroupInstance *cap)
{
    BapInstElement *bap = (BapInstElement*)
                              CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);

    if ((bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
        || (bap->recentStatus == CAP_CLIENT_RESULT_INPROGRESS))
    {
        bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);
    }

    inst->bapRequestCount--;

    /* Bap current state is meanignful only when status is success and is getting used just to skip 
     * operation on a discovered bap */
    setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_ENABLED, cap->activeCig->cigId);

#ifdef CAP_CLIENT_NTF_TIMER
    /* Cancel the timer as all NTF is received */
    capClientNtfTimerReset(bap);
#endif

    /*
     * When bapRequestCount hits zero, the build and
     * send CIS connect request
     *
     * */

    if (inst->bapRequestCount == 0)
    {
        /* Check if all devices erred, then send Error Confirm to App and return*/
        bap = (BapInstElement*)(cap->bapList.first);

        if (capClientManageError(bap, cap->bapList.count))
        {
            capClientSendUnicastStartStreamCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_FAILURE_BAP_ERR);
            return;
        }

        /* Reset totalCisCount to 0 as actual CIS count will be updated
         * based on CIS connect status.
         */
        cap->totalCisCount = 0;

        capClientBapCisConnectReqSend(bap, cap, inst);

    }

}

void capClientBapAseEnableReqSend(CsipInstElement* csip,
                                 BapInstElement* bap,
                                 CapClientGroupInstance* cap,
                                 CAP_INST* inst)
{
    bool locked = FALSE;

    if (cap == NULL || csip == NULL || bap == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP) capClientBapAseEnableReqSend: NULL instance \n");
        return;
    }

    while (bap && csip)
    {
        locked = (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED);

        CAP_CLIENT_INFO("(CAP) capClientBapAseEnableReqSend: Lock State: %d, Recent status: 0x%x, Device handle :0x%x\n", 
                                                                 csip->lock, csip->recentStatus, csip->csipHandle);

        if (CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_QOS_CONFIGURED && locked && !csip->recentStatus)
        {
            capClientBapAseEnableReq(bap, cap, inst);
        }

        bap = bap->next;
        csip = csip->next;
    }

}



/******************************************************************************************************/
/***************                   BapUnicastStartReadyreq              ******************************/
/******************************************************************************************************/


static void capClientUnicastReceiverStartReady(BapInstElement *bap, CAP_INST *inst)
{
    uint8 aseCount;
    uint8 *aseIds = NULL;

    if(bap == NULL)
    {
        CAP_CLIENT_ERROR("\n capUnicastClientReceiverStartReady: BAP NULL instance \n");
        return;
    }

    aseCount = capClientGetAseCountInState(BAP_ASE_STATE_ENABLING, bap, BAP_ASE_SOURCE);

    /* Only Source ASES on server need to be informed that Sink ASEs on CAP
     * are ready to recieve data */
    if (aseCount)
    {
        aseIds = capClientGetAseIdForGivenCidAndState(bap->bapHandle,
                                                      BAP_ASE_SOURCE,
                                                      aseCount,
                                                      BAP_ASE_STATE_ENABLING, bap);

        inst->bapRequestCount++;
        BapUnicastClientReceiverReadyReq(bap->bapHandle, BAP_RECEIVER_START_READY, aseCount, aseIds);

        /* Free the allocated pointers */
        CsrPmemFree(aseIds);
        CAP_CLIENT_INFO("\n capUnicastClientReceiverStartReady: %p \n", aseIds);
        aseIds = NULL;
    }

}

static void capClientUnicastReceiverStartReadySend(BapInstElement* bap,
                                                  CAP_INST* inst)
{
    if (bap == NULL)
    {
        CAP_CLIENT_ERROR("\n capUnicastClientReceiverStartReadySend: BAP NULL instance \n");
        return;
    }

    /* Here if the CAP instance is not co ordinated set then there will be only
     * one BAP instance */

    while (bap)
    {
        if (bap->recentStatus == BAP_RESULT_SUCCESS)
        {
            capClientUnicastReceiverStartReady(bap, inst);
        }
        else
        {
            CAP_CLIENT_INFO("\n(CAP) capUnicastClientReceiverStartReadySend: Unable to send Reciever ready due to recent failure on Device: 0x%x \n", bap->bapHandle);
        }
        bap = bap->next;
    }
}

void capClientHandleUnicastRecieverStartReadyInd(CAP_INST *inst,
                                 BapUnicastClientReceiverReadyInd* ind,
                                 CapClientGroupInstance *cap)
{
    CSR_UNUSED(inst);
    BapAseElement *ase = NULL;
    BapInstElement *bap = (BapInstElement*)
                              CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, ind->handle);

    bap->recentStatus = capClientGetCapClientResult(ind->result, CAP_CLIENT_BAP);

    bap->serverSinkSourceStreamCount--;

    CAP_CLIENT_INFO("\n capHandleUnicastRecieverStartReadyInd: aseId: %x bap->serverSinkSourceStreamCount: %x", ind->aseId, bap->serverSinkSourceStreamCount);

    ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sinkAseList, ind->aseId);

    if(ase == NULL)
    {
        ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sourceAseList, ind->aseId);
    }

    if(ase == NULL)
    {
        CAP_CLIENT_ERROR("\n ASE is null \n");
        return;
    }

    /* Update the ase state */
    ase->state = ind->aseState;

    if (bap->serverSinkSourceStreamCount == 0)
    {

        uint8 deviceCountAseInd = deviceCountForSinkSrcAse(cap);

        CAP_CLIENT_INFO("\n capHandleUnicastRecieverStartReadyInd: inst->bapRequestCount: %x deviceCountAseInd: %x", inst->bapRequestCount, deviceCountAseInd);
        /* Check there is no current operation pending and
         * All the Expected Indication for ASE's are recieved then send confirmation to the upper layer */
        if (inst->bapRequestCount == 0
            && (deviceCountAseInd == cap->currentDeviceCount))
        {
            callUnicastStartStreamCfm (cap, inst);
        }
    }
}

void capClientHandleUnicastRecieverStartReadycfm(CAP_INST *inst,
                                          BapUnicastClientReceiverReadyCfm *cfm,
                                          CapClientGroupInstance*  cap)
{
    BapInstElement *bap = (BapInstElement*)
                              CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);

    /* Don't update the previous ind failure */

    if ((bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
        || (bap->recentStatus == CAP_CLIENT_RESULT_INPROGRESS))
    {
        bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);
    }

    CAP_CLIENT_INFO("\n capClientHandleUnicastRecieverStartReadycfm:bap->serverSinkSourceStreamCount: %x", bap->serverSinkSourceStreamCount);

    /* Send Start Stream Indication for individual devices */
    capClientSendUnicastStartStreamInd(inst, cap, bap, bap->recentStatus);
    inst->bapRequestCount--;

    if (bap->serverSinkSourceStreamCount == 0)
    {
        uint8 deviceCountAseInd = deviceCountForSinkSrcAse(cap);

        CAP_CLIENT_INFO("\n capHandleUnicastRecieverStartReadyInd: inst->bapRequestCount: %x deviceCountAseInd: %x", inst->bapRequestCount, deviceCountAseInd);

        /* Check there are no current operation pending in CAP and
         * All the Expected Indication for ASE's are recieved then send confirmation to the upper layer */
        if (inst->bapRequestCount == 0
            && (deviceCountAseInd == cap->currentDeviceCount))
        {
            callUnicastStartStreamCfm (cap, inst);
        }
    }
}



/******************************************************************************************************/
/***************                   BapUnicastCisConnectReq              *******************************/
/******************************************************************************************************/

static void capClientBuildCisConnectReq(uint32 cid,
                                BapAseElement *ase,
                                BapUnicastClientCisConnection *connParam)
{
    CsrBtTypedAddr addr;

    connParam->cisHandle = ase->cisHandle;
    connParam->cisId = ase->cisId;
    connParam->tpAddrt.tp_type = LE_ACL;

    if(CsrBtGattClientUtilFindAddrByConnId(cid, &addr))
    {
        connParam->tpAddrt.addrt.type = addr.type;
        connParam->tpAddrt.addrt.addr.lap = addr.addr.lap;
        connParam->tpAddrt.addrt.addr.uap = addr.addr.uap;
        connParam->tpAddrt.addrt.addr.nap = addr.addr.nap;
    }
}

static void capClientBuildMultiCisConnectReq(BapInstElement *bap, 
                                             CapClientGroupInstance* cap,
                                             CAP_INST *inst)
{
    /*
     * Here it is assumed that based on Usecases supported
     * the CisId's will be same for sink and source (CONVERSATIONAL)
     * and in case of MEDIA use case we only have configured Server as
     * Sink ASEs. So Basic assumption here is SINK and SOURCE ASEs will share
     * same aseID and will be of same numbers if Configured.
     * */

     uint32 bapHandle     = bap->bapHandle;
     uint8 totalCisCount  = 0;
     uint8 index          = 0;
     uint8 offset         = 0;
     BapAseElement* ase   = NULL;
     BapInstElement *temp = bap;

     BapUnicastClientCisConnection* param = NULL;

     /* Calculcate total numbe rof cis count by traversing all the bap instance and get for each bap
      * As for devices CSIP dynamic addition total cis count will change based on for how many devices cis need to create */
     while (temp != NULL && CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(temp, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_ENABLED)
     {
         totalCisCount = totalCisCount + capClientGetCisCountPerBap(temp, cap->activeCig->context);

         /* Need to update the bap cis count which will get used during current use case disconnect as
          * Cap is having multi use case support there is a case where as part of first use case disconnect it was get cleared */
         temp->cisCount = capClientGetCisCountPerBap(bap, cap->activeCig->context);
         temp = temp->next;
     }

     CAP_CLIENT_INFO("\n capClientBuildMultiCisConnectReq, totalCisCount: %d\n", totalCisCount);
     param = (BapUnicastClientCisConnection*)
               CsrPmemZalloc(totalCisCount * sizeof(BapUnicastClientCisConnection));

     while (bap != NULL && CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_ENABLED)
     {
         ase = (BapAseElement*)(bap->sinkAseList.first);

         /* Check in Source ASE list if there are no ASEs in Sink List*/
         if ((ase == NULL) || (CAP_CLIENT_CIS_IS_UNI_SRC(cap->cigDirection)))
             ase = (BapAseElement*)(bap->sourceAseList.first);

         if (ase == NULL)
         {
             CAP_CLIENT_ERROR("\n capClientBuildMultiCisConnectReq: ASE NULL\n");
             return;
         }

         for (index = 0; index < bap->cisCount && ase; ase = ase->next)
         {
             if (ase->state == BAP_ASE_STATE_ENABLING
                 && ase->inUse && ase->useCase == cap->useCase)
             {
                 CAP_CLIENT_INFO("\n capClientBuildMultiCisConnectReq, cisHandle: %d  aseCisID : %x\n", ase->cisHandle, ase->cisId);
                 capClientBuildCisConnectReq(bap->bapHandle, ase, &param[offset + index]);
                 index++;
             }
         }

         if ((offset + index) <= totalCisCount)
             offset += index;
         else
             break;

         bap = bap->next;
     }

     inst->bapRequestCount++;
     CAP_CLIENT_INFO("\n capClientBuildMultiCisConnectReq, bapHandle: %x\n", bapHandle);

     BapUnicastClientCisConnectReq(bapHandle, totalCisCount, param);

#ifdef CAP_CLIENT_NTF_TIMER
     /* Trigger the CAP timer to get all the NTF for stream within spec defined time */
     capClientNtfTimerSet(inst, cap, bap, BAP_ASE_STATE_STREAMING);
#endif

     CAP_CLIENT_INFO("\n capClientBuildMultiCisConnectReq: %p \n", param);
     CsrPmemFree(param);
     param = NULL;
}


static void capClientBapCisConnectReqSend(BapInstElement* bap,
                                          CapClientGroupInstance* cap,
                                          CAP_INST* inst)
{
    BapInstElement* temp = bap;

    if (bap == NULL)
    {
        CAP_CLIENT_ERROR("\n capClientBapCisConnectReqSend: BAP NULL instance \n");
        return;
    }

    /* Here if the CAP instance is not co ordinated set then there will be only
     * one BAP instance */

     /* Get the correct bap whose use case need to move to streaming state */
    while(temp)
    {
        if (CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(temp, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_ENABLED)
        {
            bap = temp;
            break;
        }
        temp = temp->next;
    }

    /* If the Host issues this command before all the HCI_LE_CIS_Established
     * Events from the previous use of the command have been generated, the
     * Controller shall return the error code Command Disallowed (0x0C).*/
    CAP_CLIENT_INFO("\n(CAP): capClientBapCisConnectReqSend:bap->bapCurrentState: %x cigId id:%x bap->recentStatus:%x\n", bap->bapCurrentState, cap->activeCig->cigId,bap->recentStatus);
    if (bap->recentStatus == BAP_RESULT_SUCCESS)
    {
        capClientBuildMultiCisConnectReq(bap, cap, inst);
    }
    else
        CAP_CLIENT_ERROR("capClientBapCisConnectReqSend:error : recentStatus: %x", bap->recentStatus);
}


void capClientHandleUnicastCisConnectInd(CAP_INST *inst,
                             BapUnicastClientCisConnectInd *ind,
                             CapClientGroupInstance *cap)
{
    CSR_UNUSED(inst);
    BapAseElement *sinkAse = NULL;
    BapAseElement* sourceAse = NULL;
    BapInstElement* bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, ind->handle);

    bap->recentStatus = capClientGetCapClientResult(ind->result, CAP_CLIENT_BAP);

    if (ind->result == BAP_RESULT_SUCCESS)
    {
        uint8 srcCount = capClientGetSrcAseCountForUseCase(bap, cap->activeCig->context);

        /* Move the bap state to streaming */
        setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_STREAMING, cap->activeCig->cigId);

        CAP_CLIENT_INFO("================================================================\n");
        CAP_CLIENT_INFO("(CAP) : bap handle = %x CIS Handle = %x  CIS Status = %x \n",
                                            ind->handle, ind->cisHandle, ind->result);
        CAP_CLIENT_INFO("================================================================\n");

        /* Increment total cis count based on success */
        cap->totalCisCount++;

        CAP_CLIENT_INFO("\n capClientHandleUnicastCisConnectInd:  bap->serverSourceStreamCount %x cap->totalCisCount %x \n",bap->serverSinkSourceStreamCount, cap->totalCisCount);

         /* Send the capClientSendUnicastStartStreamInd for csip based devices as part of cis connect indication 
          * and in case of non csip based devices this will go as part cis connect confirmation */
        if (capClientIsGroupCoordinatedSet(cap) && (CAP_CLIENT_CIS_IS_UNI_SINK(cap->cigDirection) ||
        (CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection)
            && srcCount == 0)))
        {
            capClientSendUnicastStartStreamInd(inst, cap, bap, bap->recentStatus);
        }

    }
    else
    {
        CAP_CLIENT_INFO("================================================================\n");
        CAP_CLIENT_INFO("(CAP) : bap handle = %x CIS Handle = %x  CIS Status = %x \n",
                                            ind->handle, ind->cisHandle, ind->result);
        CAP_CLIENT_INFO("================================================================\n");

        sinkAse = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_CISHANDLE(bap->sinkAseList, ind->cisHandle);
        sourceAse = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_CISHANDLE(bap->sourceAseList, ind->cisHandle);

        /* Reset status of ASEs*/
        if (sinkAse)
        {
            CAP_CLIENT_INFO("(CAP): Sink AseId = %d is no longer in Use \n",sinkAse->aseId);
            sinkAse->cisHandle = 0;
            sinkAse->inUse = FALSE;
        }

        if (sourceAse)
        {
            CAP_CLIENT_INFO("(CAP): Source AseId = %d is no longer in Use \n", sourceAse->aseId);
            sourceAse->cisHandle = 0;
            sourceAse->inUse = FALSE;
        }
    }
}

void capClientHandleUnicastCisConnectCfm(CAP_INST *inst,
                             BapUnicastClientCisConnectCfm *cfm,
                             CapClientGroupInstance *cap)
{
    BapInstElement *bap = (BapInstElement*)
                              CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);
    uint8 srcCount = 0; 

    CAP_CLIENT_INFO("\n capClientHandleUnicastCisConnectCfm: cfm->status :%x \n", cfm->result);

    /* if invalid Handle is failure, stop further processes*/

    if (bap == NULL)
    {
        capClientSendUnicastStartStreamCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_NOT_SUPPORTED);
        return;
    }

    srcCount = capClientGetSrcAseCountForUseCase(bap, cap->activeCig->context);
#ifdef CAP_CLIENT_NTF_TIMER
    /* Cancel the timer as all NTF is received */
    capClientNtfTimerReset(bap);
#endif

    /* Don't update the previous ind failure */
    if ((bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
        || (bap->recentStatus == CAP_CLIENT_RESULT_INPROGRESS))
    {
        bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);
    }
    /* If Conversational Send reciever Start Ready*/
    inst->bapRequestCount--;

    /* Bap current state is meanignful only when status is success and is getting used just to skip 
     * operation on a discovered bap */
    setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_STREAMING, cap->activeCig->cigId);

    /*
     * Send Start Stream Indication to the application in case only Sinks are configured on
     * the server 
     * 
     * NOTE: A bidirectional usecase configured with no source Ases is
     *        treated as unidirectional use case and indication is sent
     *        without reciever start ready sent to remote side
     */


    if ((!capClientIsGroupCoordinatedSet(cap)) && (CAP_CLIENT_CIS_IS_UNI_SINK(cap->cigDirection) ||
        (CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection)
            && srcCount == 0)))
    {
        capClientSendUnicastStartStreamInd(inst, cap, bap, bap->recentStatus);
    }
    
    /*
     * When bapRequestCount hits zero, the build and
     * send CIS connect request
     *
     * */

    if (inst->bapRequestCount == 0)
    {
        bap = (BapInstElement*)(cap->bapList.first);
        uint8 aseCount = 0;
        BapInstElement *tmp = bap;
        CapClientBool allMicConfigured = FALSE;

        if (capClientManageError(bap, cap->bapList.count))
        {
            capClientSendUnicastStartStreamCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_FAILURE_BAP_ERR);
            return;
        }

        /* Find number of Mic configured for CSIP based remote device to handle late join use case 
		 * Where Application can add second device later and configuration is assymetric e.g. 
		 * For first device sink/src ases are configured(CIS will be bidirectional) and for second device only sink is required (still CIS will be bidirectional)
		 * So calculate total number of mic configured with overall mic required */
        while (tmp && capClientIsGroupCoordinatedSet(cap))
        {
            aseCount = aseCount + capClientGetAseCountInState(BAP_ASE_STATE_STREAMING, tmp, BAP_ASE_SOURCE);

            if (cap->numOfSourceAses == aseCount)
            {
                allMicConfigured = TRUE;
            }
            tmp = tmp->next;
            CAP_CLIENT_INFO("\n capClientHandleUnicastCisConnectCfm: send capUnicastClientReceiverStartReadySend cap->numOfSourceAses %x aseCount %d\n", cap->numOfSourceAses, aseCount);
        }


        if (CAP_CLIENT_CIS_IS_UNI_SRC(cap->cigDirection) ||
            (CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection)
                                       && !allMicConfigured))
        {
            CAP_CLIENT_INFO("\n capClientHandleUnicastCisConnectCfm: send capUnicastClientReceiverStartReadySend cap->numOfSourceAses %x\n", cap->numOfSourceAses);

            capClientUnicastReceiverStartReadySend(bap, inst);
        }
        else
        {
            uint8 deviceCountAseInd = deviceCountForSinkSrcAse(cap);

            /* Once All the Rx start ready indication for Sink and Src are received
             * Then send the confirmation as this case for handling the use cases which is unidirectional sink in nature
             * And All the receiver start ready indication is recieved */
            if (deviceCountAseInd == cap->currentDeviceCount)
            {
                callUnicastStartStreamCfm (cap, inst);
            }

        }
    }
}

/***************************************************************************************************/

static CapClientBool capClientCheckUniqueCisId(uint16 cisHandle,
                                             CapClientCisHandles* handles,
                                             uint8 filledCount)
{
    uint8 i;

    for (i = 0; i < filledCount; i++)
    {
        if (handles[i].cisHandle == cisHandle)
            return FALSE;
    }
    return TRUE;
}


static void capClientUpdateCapCisHandles(BapInstElement *bap,
                                   CapClientCisHandles *handles)
{
    uint8 index = 0;
    BapAseElement *ase = NULL;
    uint8 cisCount = 0;
    uint8 fillCount = 0;
    uint8 prevCisId = 0;

    if(bap == NULL)
    {
        CAP_CLIENT_ERROR("\n capUnicastClientSetUpDataPathReqSend: NULL instance \n");
        return;
    }

    /* Ensure content of handles is zero */
    CsrMemSet(handles, 0, sizeof(CapClientCisHandles));

    cisCount = bap->cisCount;

    ase = (BapAseElement*)(bap->sinkAseList.first);

    while(ase && index < cisCount)
    {
        if(ase->state == BAP_ASE_STATE_STREAMING ||
                         ase->state == BAP_ASE_STATE_ENABLING)
        {
            if (prevCisId != ase->cisId)
            {
                handles[index].cisHandle = ase->cisHandle;
                handles[index].direction = ase->datapathDirection;
                handles[index].audioLocation = ase->audioLocation;
                prevCisId = ase->cisId;
                index++;
            }
        }
        ase = ase->next;
    }

    /* If no sinks configured Then go and Check Source */

    ase = (BapAseElement*)((CsrCmnListElm_t*)(bap->sourceAseList.first));
    fillCount = index;
    prevCisId = 0;

    while (ase && index < cisCount)
    {
        if ((ase->state == BAP_ASE_STATE_STREAMING ||
            ase->state == BAP_ASE_STATE_ENABLING))
        {
            if (capClientCheckUniqueCisId(ase->cisHandle, handles, fillCount))
            {
                if (prevCisId != ase->cisId)
                {
                    handles[index].cisHandle = ase->cisHandle;
                    handles[index].direction = ase->datapathDirection;
                    handles[index].audioLocation = ase->audioLocation;
                    index++;
                    prevCisId = ase->cisId;
                }
            }
        }
        ase = ase->next;
    }
}

static void capClientUpdateAudioConfig(CapClientGroupInstance* gInst, 
                                     CapClientAudioConfig* audioConfig)
{
    bool isJointStereoInSink = FALSE; 
    uint8 channelCount = capClientGetChannelCountForContext(gInst, gInst->useCase, TRUE);
    /* Populate Codec Id*/

    CapClientCigElem* cig = CAP_CLIENT_GET_CIG_FROM_CONTEXT(gInst->cigList, gInst->useCase);

    CapClientSreamCapability config =
               (gInst->activeCig->sinkConfig == CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN) ?
                                 gInst->activeCig->srcConfig : gInst->activeCig->sinkConfig;

    audioConfig->codecId = capClientGetCodecIdFromCapability(config);
    audioConfig->vendorCodecId = capClientGetVendorCodecIdFromCapability(config);
    audioConfig->companyId = capClientGetCompanyIdFromCapability(config);
    audioConfig->codecVersionNum = gInst->codecVersionNum;

    /* CAP uses SNK channel count for identifying aptX Lite config values to be used*/
    if(channelCount > 1)
        isJointStereoInSink = TRUE;

    if ((CAP_CLIENT_CIS_IS_UNI_SRC(gInst->cigDirection))
        || (CAP_CLIENT_CIS_IS_BIDIRECTIONAL(gInst->cigDirection)))
    {
        audioConfig->srcPdelay = cig->srcMinPdelay;
        audioConfig->srcFrameDuaration = capClientGetFrameDurationFromCapability(gInst->activeCig->srcConfig);
        audioConfig->srcSamplingFrequency = capClientGetSamplingFreqFromCapability(gInst->activeCig->srcConfig);
        audioConfig->srcOctetsPerFrame = capClientGetSduSizeFromCapability(gInst->activeCig->srcConfig, 
                                                                          gInst->cigConfigMode,
                                                                          gInst->activeCig->srcLocCount,
                                                                          isJointStereoInSink);
        audioConfig->srcLc3BlocksPerSdu = (gInst->activeCig->unicastParam.codecBlocksPerSdu == 0) ?
                                                   1 : gInst->activeCig->unicastParam.codecBlocksPerSdu;

        audioConfig->srcOctetsPerFrame = (gInst->activeCig->unicastParam.sduSizePtoC == 0) ? 
                              audioConfig->srcOctetsPerFrame : gInst->activeCig->unicastParam.sduSizePtoC;

#ifdef CAP_CLIENT_IOP_TEST_MODE
        if ((gInst->cigConfigMode & CAP_CLIENT_IOP_TEST_CONFIG_MODE) == CAP_CLIENT_IOP_TEST_CONFIG_MODE)
        {
            audioConfig->srcOctetsPerFrame =
                capClientGetSDUFromCapabilityForIOP(gInst->activeCig->srcConfig, audioConfig->srcOctetsPerFrame, gInst->useCase);
        }
#endif
    }

    if ((CAP_CLIENT_CIS_IS_UNI_SINK(gInst->cigDirection))
        || (CAP_CLIENT_CIS_IS_BIDIRECTIONAL(gInst->cigDirection)))
    {
        audioConfig->sinkPdelay = cig->sinkMinPdelay;
        audioConfig->sinkFrameDuaration = capClientGetFrameDurationFromCapability(gInst->activeCig->sinkConfig);
        audioConfig->sinkSamplingFrequency = capClientGetSamplingFreqFromCapability(gInst->activeCig->sinkConfig);
        audioConfig->sinkOctetsPerFrame = capClientGetSduSizeFromCapability(gInst->activeCig->sinkConfig, 
                                                                           gInst->cigConfigMode,
                                                                           gInst->activeCig->sinkLocCount,
                                                                           isJointStereoInSink);
        audioConfig->sinkOctetsPerFrame = (gInst->activeCig->unicastParam.sduSizeCtoP == 0) ?
            audioConfig->sinkOctetsPerFrame : gInst->activeCig->unicastParam.sduSizeCtoP;

        audioConfig->sinkLc3BlocksPerSdu = (gInst->activeCig->unicastParam.codecBlocksPerSdu == 0) ?
            1 : gInst->activeCig->unicastParam.codecBlocksPerSdu;

#ifdef CAP_CLIENT_IOP_TEST_MODE
        if ((gInst->cigConfigMode & CAP_CLIENT_IOP_TEST_CONFIG_MODE) == CAP_CLIENT_IOP_TEST_CONFIG_MODE)
        {
            audioConfig->sinkOctetsPerFrame =
                capClientGetSDUFromCapabilityForIOP(gInst->activeCig->sinkConfig, audioConfig->sinkOctetsPerFrame, gInst->useCase);
        }
#endif
    }
}



void capClientSendUnicastStartStreamInd(CAP_INST* inst,
                            CapClientGroupInstance* gInst,
                            BapInstElement* bap,
                            CapClientResult result)
{
    CSR_UNUSED(gInst);
    MAKE_CAP_CLIENT_MESSAGE(CapClientUnicastStartStreamInd);
    message->groupId = inst->activeGroupId;
    message->result = result;
    message->cigId = CAP_CLIENT_INVALID_CIG_ID;

    if (bap && result == CAP_CLIENT_RESULT_SUCCESS)
    {
        message->cid = bap->bapHandle;
        message->cisCount = bap->cisCount;
        message->cigId = gInst->activeCig->cigId;
        message->cishandles = (CapClientCisHandles*)CsrPmemZalloc(bap->cisCount * sizeof(CapClientCisHandles));
        message->audioConfig = (CapClientAudioConfig*)CsrPmemZalloc(sizeof(CapClientAudioConfig));
        message->vsMetadata = NULL;

        if (bap->vsMetadata &&
            (bap->vsMetadata->sinkVsMetadataLen || bap->vsMetadata->srcVsMetadataLen))
        {
            message->vsMetadata = bap->vsMetadata;

            /* This metadata no longer needs reference in CAP. Needs to be freed by upper layer. */
            bap->vsMetadata =  NULL;
        }

        capClientUpdateCapCisHandles(bap, message->cishandles);
        capClientUpdateAudioConfig(gInst, message->audioConfig);
    }

    CapClientMessageSend(inst->profileTask, CAP_CLIENT_UNICAST_START_STREAM_IND, message);
}

void capClientSendUnicastStartStreamCfm(AppTask appTask,
                                      CAP_INST *inst,
                                      CapClientGroupInstance *gInst,
                                      CapClientResult result)
{
#ifdef CAP_CLIENT_NTF_TIMER
    /* Check for the notification flag if the flag is set it means timeout handler already sent the response */
    if (gInst && gInst->capNtfTimeOut == TRUE)
    {
        return;
    }
#endif

    CapClientProfileMsgQueueElem* msgElem = NULL;
    MAKE_CAP_CLIENT_MESSAGE(CapClientUnicastStartStreamCfm);
    message->groupId = inst->activeGroupId;
    message->result = result;

    if (gInst && result == CAP_CLIENT_RESULT_SUCCESS )
    {
        CAP_CLIENT_CLEAR_PENDING_OP(gInst->pendingOp);
        inst->streaming = TRUE;

        gInst->capState = CAP_CLIENT_STATE_STREAM_STARTED;
    }

    CapClientMessageSend(appTask, CAP_CLIENT_UNICAST_START_STREAM_CFM, message);

    /*
     * If the cfm was success and message queue is not
     * empty i.e msgElem is not NULL, handle the next
     * message
     *
     */

     /* Save CAP state and get next message */
    if(gInst)
        msgElem = capClientGetNextMsgElem(gInst);

    if (msgElem)
    {
        msgElem->handlerFunc(inst, (void*)msgElem, gInst);
    }
}

static void capClientUnicastStartStreamReqHandler(CAP_INST* inst,
                                                void* msg,
                                                CapClientGroupInstance* cap)
{
    CsipInstElement* csip;
    BapInstElement* bap;
    CapClientCigElem* cig;

    CapClientResult result;
    CapClientProfileMsgQueueElem* msgElem = (CapClientProfileMsgQueueElem*)msg;
    CapClientProfileTaskListElem* task = (CapClientProfileTaskListElem*)msgElem->task;
    CapClientInternalUnicastStartStreamReq* req = (CapClientInternalUnicastStartStreamReq*)(msgElem->capMsg);
    CapClientBool validateSinkCfg = FALSE;
    CapClientBool validateSrcCfg = FALSE;

    /* Validate cap State */

    result = capClientValidateCapState(cap->capState, req->type);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
#ifdef CAP_CLIENT_NTF_TIMER
         /* Reset the flag ere, as in if timer expirt happend flag will be set to TRUE and will not be reset untill new 
         timer is not kicked in but as the cap state failed reset the timer to send the confirmation */
        cap->capNtfTimeOut = FALSE;
#endif
        capClientSendUnicastStartStreamCfm(req->profileTask, inst, cap, result);
        CAP_CLIENT_INFO("\n handleUnicastStartStreamReq: invalid state transition \n");
        return;
    }

    inst->profileTask = req->profileTask;

    /* co ordinated set?
     *
     * Based on if co ordinated Set or not decide number of ASEs required
     * and then start BAP procedures
     *
     * */

    /*
     * If the useCase is Not configured then reject the Request
     */

    cig = (CapClientCigElem*)
        CAP_CLIENT_GET_CIG_FROM_CONTEXT(cap->cigList, req->useCase);

    if (cig == NULL)
    {
        result = CAP_CLIENT_RESULT_NOT_CONFIGURED;
        capClientSendUnicastStartStreamCfm(req->profileTask, inst, cap, result);
        CAP_CLIENT_INFO("\n handleUnicastStartStreamReq: Usecase not configured! \n");
        return;
    }

    /* Check if the context is still available */
    validateSinkCfg = (cig->sinkConfig != CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN) ? TRUE : FALSE;

    if (validateSinkCfg && !capClientIsContextAvailable(req->useCase, cap, TRUE))
    {
        result = CAP_CLIENT_RESULT_CONTEXT_UNAVAILABLE;
        CAP_CLIENT_INFO("\n(CAP) handleUnicastStartStreamReq: Sink Context Unavailable");
        capClientSendUnicastStartStreamCfm(req->profileTask, inst, cap, result);
        return;
    }

    validateSrcCfg = (cig->srcConfig != CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN) ? TRUE : FALSE;

    if (validateSrcCfg && !capClientIsContextAvailable(req->useCase, cap, FALSE))
    {
        result = CAP_CLIENT_RESULT_CONTEXT_UNAVAILABLE;
        CAP_CLIENT_INFO("\n(CAP) handleUnicastStartStreamReq: Source Context Unavailable");
        capClientSendUnicastStartStreamCfm(req->profileTask, inst, cap, result);
        return;
    }

    /* Initialize the CAP with task specific parameters */
    capClientCapGetCigSpecificParams(cig, cap);

    bap = (BapInstElement*)(cap->bapList.first);
    cap->useCase = req->useCase;
    cap->metadata = req->metadataParam;
    cap->metadataLen = req->metadataLen;
    req->metadataParam = NULL;

    task->activeCig = cig;

    if (capClientIsGroupCoordinatedSet(cap))
    {
        csip = (CsipInstElement*)(cap->csipList.first);

        /* Here we need to obtain lock on all the devices and the
         * Start BAP unicast Procedures */

        /* check if the profile is already locked
         *
         * Note: If one device is in lock state in a group
         * it's assumed that all other participants are in lock state*/

        cap->pendingOp = CAP_CLIENT_BAP_UNICAST_START_STREAM;

        if (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientBapAseEnableReqSend(csip, bap, cap,inst);

        }
        /* Otherwise obtain lock and the start BAP Procedures */
        else
        {
            capClientSetCsisLockState(csip, &inst->csipRequestCount, TRUE);
        }
    }
    else
    {
        capClientBapAseEnableReq(bap, cap, inst);
#ifdef CAP_CLIENT_NTF_TIMER
        /* Trigger the CAP timer to get all the NTF for enable within spec defined time */
        capClientNtfTimerSet(inst, cap,bap, BAP_ASE_STATE_ENABLING);
#endif
    }
}

void handleUnicastStartStreamReq(CAP_INST  *inst, const Msg msg)
{
    AppTask appTask;
    CapClientResult result;
    CapClientInternalUnicastStartStreamReq *req = (CapClientInternalUnicastStartStreamReq*)msg;
    CapClientGroupInstance *cap = NULL;
    CapClientProfileTaskListElem* task = NULL;
    CapClientProfileMsgQueueElem* msgElem = NULL;
    CapClientBool isQueueEmpty = FALSE;

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);
    appTask = req->profileTask;

    if(cap == NULL)
    {
        capClientSendUnicastStartStreamCfm(appTask, inst, NULL, CAP_CLIENT_RESULT_INVALID_GROUPID);
        return;
    }

    /*
     * if groupId is not same Switch the group return
     * by sending error Response
     * */

    if(req->groupId != inst->activeGroupId)
    {
        result = CAP_CLIENT_RESULT_INVALID_GROUPID;
        CAP_CLIENT_INFO("\n handleUnicastStartStreamReq: Unable to change GroupId \n");
        capClientSendUnicastStartStreamCfm(appTask, inst, cap, result);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/
    task = (CapClientProfileTaskListElem*)
                  CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);
    if (task == NULL)
    {
        result = CAP_CLIENT_RESULT_TASK_NOT_REGISTERED;
        capClientSendUnicastStartStreamCfm(appTask, inst, cap, result);
        return;
    }


    isQueueEmpty = CAP_CLIENT_IS_MSG_QUEUE_EMPTY(cap->capClientMsgQueue);

    msgElem = CapClientMsgQueueAdd(&cap->capClientMsgQueue,
                                  (void*)req,
                                  0,
                                  req->type,
                                  capClientUnicastStartStreamReqHandler,
                                  task);

    if (isQueueEmpty)
    {
        capClientUnicastStartStreamReqHandler(inst, (void*)msgElem, cap);
    }
}
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
