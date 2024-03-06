/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_util.h"
#include "cap_client_common.h"
#include "cap_client_ase.h"
#include "cap_client_csip_handler.h"
#include "cap_client_stop_stream_req.h"
#include "cap_client_unicast_disconnect_req.h"
#include "cap_client_debug.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
#define CAP_DATAPATH_BITMASK     1
#define DISCONNECT_TIMEOUT       0x08 /* HCI_ERROR_CONN_TIMEOUT */

static void capClientSendDisconnectInd(AppTask appTask,
                                    ServiceHandle groupId,
                                    uint16 cisHandle,
                                    uint16 reason)
{
    /* Check for the reason if its connection timeout then send link loss ind
     * rest all other cases send normal disconnect ind */
    if (reason == DISCONNECT_TIMEOUT)
    {
         MAKE_CAP_CLIENT_MESSAGE(CapClientUnicastLinkLossInd);
         message->activeGroupId = groupId;
         message->cisHandle = cisHandle;
         CapClientMessageSend(appTask, CAP_CLIENT_UNICAST_LINK_LOSS_IND, message);
    }
    else
    {
        MAKE_CAP_CLIENT_MESSAGE(CapClientUnicastDisconnectInd);
        message->activeGroupId = groupId;
        message->cisHandle = cisHandle;
        message->reason = reason;
        CapClientMessageSend(appTask, CAP_CLIENT_UNICAST_DISCONNECT_IND, message);
    }
}

#ifdef INSTALL_LEA_UNICAST_CLIENT_REMOTE_STOP
static void capClientSendUnicastStopStreamInd(AppTask appTask,
                                              CapClientGroupInstance *cap,
                                              uint32 btconnId,
                                              uint8 aseState,
                                              uint16 *cisHandles,
                                              uint8 cisCount)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientUnicastStopStreamInd);

    message->groupId = cap->groupId;
    message->cid = btconnId;
    message->released = CAP_CLIENT_ASE_STATE_INVALID;

    if (aseState == BAP_ASE_STATE_DISABLING)
    {
        message->released = CAP_CLIENT_STATE_DISABLING;
    }
    else if (aseState == BAP_ASE_STATE_RELEASING)
    {
        message->released = CAP_CLIENT_STATE_RELEASING ;
    }

    message->cisCount = cisCount;
    message->cishandles = cisHandles;
    message->cigId = cap->activeCig->cigId;

   CapClientMessageSend(appTask, CAP_CLIENT_UNICAST_STOP_STREAM_IND, message);
}
#endif

void capClientSendUnicastStopStreamCfm(AppTask appTask,
                                       CAP_INST *inst,
                                       CapClientResult result,
                                       bool clientReleased,
                                       BapInstElement *bap ,
                                       CapClientGroupInstance *cap)
{
#ifdef CAP_CLIENT_NTF_TIMER
    /* Check for the Notification flag if the flag is set it means timeout handler already sent the response */
    if (cap && cap->capNtfTimeOut == TRUE)
    {
        return;
    }
#endif

    uint8  i = 0;
    CapClientProfileMsgQueueElem* msgElem = NULL;
    MAKE_CAP_CLIENT_MESSAGE(CapClientUnicastStopStreamCfm);
    message->groupId = inst->activeGroupId;
    message->result = result;
    message->released = clientReleased;
    message->deviceStatusLen = 0;
    message->deviceStatus = NULL;

    if(cap && result == CAP_CLIENT_RESULT_SUCCESS)
    {
        message->deviceStatusLen = inst->deviceCount;
        message->deviceStatus = (CapClientDeviceStatus*)
                            CsrPmemZalloc(message->deviceStatusLen * sizeof(CapClientDeviceStatus));

        for (i = 0; i < inst->deviceCount && bap; i++)
        {
            message->deviceStatus[i].cid = bap->bapHandle;
            message->deviceStatus[i].result = bap->recentStatus;

            if (clientReleased)
            {
                capClientClearAllStreamVariables(bap);
                capClientStopStreamIterateAses(bap, cap->useCase);
            }

            bap = bap->next;
        }

        /* NOTE: Internal API which populates the above details needs to be tested with multiple Device
         *       i.e Standard LE
         */

       /* capGetAllProfileInstanceStatus(message->deviceCount, (CsrCmnListElm_t*)bap, &message->status, CAP_CLIENT_BAP);*/
        if (clientReleased)
        {
            cap->numOfSourceAses = 0;
            cap->useCase = CAP_CLIENT_CONTEXT_TYPE_PROHIBITED;
            cap->capState = CAP_CLIENT_STATE_STREAM_STOPPED;
        }
        else
        {
            cap->capState = CAP_CLIENT_STATE_UNICAST_CONNECTED;
        }
        inst->streaming = FALSE;

        /* Reset the pending operation in CAP as procedure is completed */
        CAP_CLIENT_CLEAR_PENDING_OP(cap->pendingOp);
    }

    CapClientMessageSend(appTask, CAP_CLIENT_UNICAST_STOP_STREAM_CFM, message);

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


/******************************************************************************************/
/***************************************REMOVE CIG*****************************************/
/******************************************************************************************/

static void cigIdReset(CapClientCigElem* cig)
{
    if((cig->cigId >= 0) && (cig->cigId < CAP_CLIENT_MAX_SUPPORTED_CIGS))
    {
        cigID[cig->cigId] = 0;
    }
}

void capClientHandleRemoveCigCfm(CAP_INST* inst,
                       BapUnicastClientRemoveCigCfm* cfm,
                       CapClientGroupInstance* cap)
{
    CSR_UNUSED(cfm);
    CsipInstElement* csip = NULL;
    BapInstElement* bap = (BapInstElement*)cap->bapList.first;
    BapInstElement* temp = bap;
    CapClientCigElem* cig = CAP_CLIENT_GET_CIG_FROM_CIGID(cap->cigList, cfm->cigId);

    cigIdReset(cig);

    CAP_CLIENT_INFO("\n(CAP) capHandleRemoveCigCfm: Removed CIG");

    if (cfm->result == BAP_RESULT_SUCCESS)
    {
        /* cig has been removed so clear the data path assoicated with the cig */
        cig->dataPath = FALSE;
        cap->stopComplete = TRUE;

        /* Remove vsData maintained in unicastParams */
        if(cig->unicastParam.vsConfigLen > 0)
        {
            CsrPmemFree(cig->unicastParam.vsConfig);
            cig->unicastParam.vsConfig = NULL;
        }

        /* remove the CIG instance from CAP */
        CAP_CLIENT_REMOVE_CIG(&cap->cigList, cig);
        cap->activeCig = NULL;

        BapInstElement *tmpBap;
        while (temp)
        {
            tmpBap = temp->next;
            /* Check for the bap cis handles free logic( running in loop as in future multiple can be there which need to be freed) */
            if (temp->cisHandles)
            {
                CsrPmemFree(temp->cisHandles);
                temp->cisHandles = NULL;
            }
			
			/* Check if the bap is dummy, this can happen when only one device is there which went to streaming 
			 * and then cig is getting removed as part of stop stream with release procedure */
            if (temp->bapCurrentState == CAP_CLIENT_BAP_STATE_INVALID)
            {
                CsrCmnListElementRemove(&cap->bapList, (CsrCmnListElm_t *)temp);
            }
            temp = tmpBap;
        }

        if (capClientIsGroupCoordinatedSet(cap))
        {
            if (capClientAsesReleaseComplete(cap))
            {
                csip = (CsipInstElement*)(cap->csipList.first);

                /* Release the lock for set members */
                capClientSetCsisLockState(csip, &inst->csipRequestCount, FALSE);
            }

        }
        else
        {
            /* Send CFM message*/

            if (cap->pendingOp == CAP_CLIENT_BAP_UNICAST_RELEASE
                || cap->pendingOp == CAP_CLIENT_BAP_UNICAST_DISABLE)
            {
                /* If Streaming has not started, then no need to wait for released Ind*/

                if (cap->stopComplete && capClientAsesReleaseComplete(cap))
                {
                    cap->stopComplete = FALSE;

                    capClientSendUnicastStopStreamCfm(inst->profileTask, inst,
                                                     CAP_CLIENT_RESULT_SUCCESS,
                                                     (cap->pendingOp == CAP_CLIENT_BAP_UNICAST_RELEASE),
                                                      bap, cap);

                }
            }
            else if (cap->pendingOp == CAP_CLIENT_UNICAST_DISCONNECT)
            {
                if (cap->stopComplete && capClientAsesReleaseComplete(cap))
                {
                    cap->stopComplete = FALSE;
                    capClientSendUnicastClientDisConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_SUCCESS);
                }
            }
            else
            {
                CAP_CLIENT_INFO("\n(CAP) capHandleRemoveCigCfm : Link Disconnecetion from Remote side \n");
            }
        }
    }
    else if (cap->forceRemoveCig)
    {
        CAP_CLIENT_PANIC("\n(CAP) capHandleRemoveCigCfm : CIG Remove Failed!! \n");
    }
    else
    {
        capClientSendUnicastStopStreamCfm(inst->profileTask,
                                         inst, CAP_CLIENT_RESULT_FAILURE_BAP_ERR,
                                         (cap->pendingOp == CAP_CLIENT_BAP_UNICAST_RELEASE), 
                                         bap, cap);
    }
}

static void capClientSendUnicastClientRemoveCig(CapClientCigElem *cig)
{
    /* All the CISes are configured with same CIG per connection_id */

    if (cig)
    {
        BapUnicastClientCigRemoveReq(CSR_BT_CAP_CLIENT_IFACEQUEUE, cig->cigId);
    }
    else
    {
        CAP_CLIENT_INFO("\n(CAP) capSendUnicastClientRemoveCig: NULL BAP instance");
    }
}

/****************************************************************************************/
/*****************************************CIS Diconnect**********************************/
/****************************************************************************************/

static void capClientSendUnicastClientCisDisconnect(BapInstElement *bap,
                                      CAP_INST *inst, 
                                     CapClientContext useCase,
                                     CapClientGroupInstance *cap)
{
    uint8 cisCount = capClientGetTotalCisCount(bap, useCase, cap->activeCig->cigDirection);
    uint16 *cisHandles = capClientGetCisHandlesForContext(useCase, cap, cisCount);
    uint8 index = 0;
    uint8 bapCisCount = bap->cisCount;

    CAP_CLIENT_INFO("\n capSendUnicastClientCisDisconnect: Cis disconnection in progress \n");

#ifdef CAP_CLIENT_NTF_TIMER
    if (cap->doRelease)
    {
        /* Trigger the CAP timer to get all the NTF for enable within spec defined time */
        capClientNtfTimerSet(inst, cap, bap, CAP_ASE_STATE_RELEASED);
    }
#endif

    for(index = 0; index < cisCount; index++)
    {
        inst->bapRequestCount++;
        CAP_CLIENT_INFO("\n(CAP) capSendUnicastClientCisDisconnectReq : %d  \t CisHandle %d Bap handle %d bapCisCount %d cisCount %d\n", inst->bapRequestCount, cisHandles[index], bap->bapHandle, bapCisCount, cisCount);

        BapUnicastClientCisDiconnectReq(bap->bapHandle, cisHandles[index], 0x13);

        if (cisCount > bapCisCount && 
            ((bapCisCount - (index + 1)) == 0))
        {
            bap = bap->next;
            bapCisCount += bap->cisCount;
        }

    }
    CsrPmemFree(cisHandles);
    cisHandles = NULL;
}


void capClientHandleCisDisconnectCfm(CAP_INST *inst,
                               BapUnicastClientCisDisconnectCfm *cfm,
                               CapClientGroupInstance *cap)
{
    BapAseElement *ase = NULL;
    BapInstElement *bap = (BapInstElement*)
                              CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);
    CsipInstElement* csip = NULL;

    bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);
    inst->bapRequestCount--;
    CAP_CLIENT_INFO("\n(CAP) capClientHandleCisDisconnectCfm : %d \n", inst->bapRequestCount);


    /* Reset the cis Id only for non csip case, in case of CSIP we need to preserve for dyanmic addition */
    if (cap->setSize == 0)
    {
        ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_CISHANDLE(bap->sinkAseList, cfm->cisHandle);

        if (ase)
        {
            ase->cisId = 0;
        }


        ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_CISHANDLE(bap->sourceAseList, cfm->cisHandle);

        if (ase)
        {
            ase->cisId = 0;
        }
    }

    /* 
     * Decrement the cap local CisCount every time the CIS disconnection happens,
     * Increment the same when cisConnect Happens
     */
    if (cfm->result == BAP_RESULT_SUCCESS)
    {
        cap->totalCisCount--;
        CAP_CLIENT_INFO("\n(CAP) capClientHandleCisDisconnectCfm cis handlecount: %d \n", cap->totalCisCount);

    }


    if(inst->bapRequestCount == 0)
    {
        if (cap->doRelease)
        {
            capClientSendUnicastClientRemoveCig(cap->activeCig);
        }
        else
        {
            if (capClientIsGroupCoordinatedSet(cap))
            {
                csip = (CsipInstElement*)(cap->csipList.first);

                /* Here we need to obtain release on all the devices and then send
                 * Cfm to application
                 * */

                cap->pendingOp = CAP_CLIENT_BAP_UNICAST_DISABLE;
                capClientSetCsisLockState(csip, &inst->csipRequestCount, FALSE);

            }
            else
            {
                /* Send CFM message*/

                if (cap->pendingOp == CAP_CLIENT_BAP_UNICAST_DISABLE)
                {
                    capClientSendUnicastStopStreamCfm(inst->profileTask ,inst, 
                                                    CAP_CLIENT_RESULT_SUCCESS,
                                                    FALSE, bap, cap);
                }
                else
                {
                    CAP_CLIENT_INFO("\n(CAP) capClientHandleCisDisconnectCfm : Invalid Operation \n");
                }
            }
        }
    }
}

void capClientHandleCisDisconnectInd(CAP_INST *inst,
                              BapUnicastClientCisDisconnectInd* ind,
                              CapClientGroupInstance* cap)
{
    BapAseElement* ase = NULL;
    BapInstElement* bap = (BapInstElement*)
        CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, ind->handle);

#ifdef INSTALL_LEA_UNICAST_CLIENT_REMOTE_STOP
    uint8 cisCount;
    uint16 *cisHandle = NULL;

    if (capClientIsGroupCoordinatedSet(cap))
    {
        cisHandle = (uint16*)CsrPmemZalloc(sizeof(uint16));
    }
    else
    {
        if (bap->cisHandles == NULL)
        {
            bap->cisHandles = (uint16 *)CsrPmemZalloc(sizeof(uint16)*cap->totalCisCount);
        }
    }
#endif

    ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_CISHANDLE(bap->sinkAseList, ind->cisHandle);

    if (ase == NULL)
        ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_CISHANDLE(bap->sourceAseList, ind->cisHandle);

    /* Reset the cis Id only for non csip case, in case of CSIP we need to preserve for dyanmic addition */
    if (ase && cap->setSize == 0)
    {
        ase->cisId = 0;
        ase->removeDatapath = FALSE;
    }
    capClientSendDisconnectInd(inst->profileTask, inst->activeGroupId, ind->cisHandle, ind->reason);

#ifdef INSTALL_LEA_UNICAST_CLIENT_REMOTE_STOP
    /* If remote device is CSIP then multiple stop stream indication will go to the App */
    if (ase && capClientIsGroupCoordinatedSet(cap))
    {
        cisCount = capClientGetCisCountPerBap(bap, cap->useCase);

        if (cisHandle)
        {
            SynMemCpyS(cisHandle, CAP_CLIENT_CIS_HANDLE_SIZE, &ind->cisHandle, CAP_CLIENT_CIS_HANDLE_SIZE);
        }
        capClientSendUnicastStopStreamInd(inst->profileTask, cap, bap->cid, ase->state, cisHandle, cisCount);
    }
    else
    {
        uint8 found = FALSE;
        uint8 cisHandleOffset = 0;

        /* Search for the offset to store the cis handle in the cishandles memory */
        while (!found)
        {
            if (bap->cisHandles[cisHandleOffset] == 0x00)
            {
                found = TRUE;
            }
            
            if (found)
            {
                SynMemCpyS(bap->cisHandles + cisHandleOffset, CAP_CLIENT_CIS_HANDLE_SIZE, &ind->cisHandle, CAP_CLIENT_CIS_HANDLE_SIZE);
                break;
            }
            cisHandleOffset = cisHandleOffset + 1;
        }
    }
#endif

    /* This counter is used to keep count of total number of CIS connection established by CAP
     * Decrement the CIS counter whenever link loss occurrs *
     * At a time CAP can only have on CIG active, hence if CIS loss occurs then its has to be from current
     * Active CIG*/

    cap->totalCisCount--;
    CAP_CLIENT_INFO("\n(CAP) capClientHandleCisDisconnectInd : %d\n", cap->totalCisCount);

    /* When the activeCig->cisHandleCount hits zero i.e when all CISes are disconnected, go ahead and remove CIG*/

    if (cap->totalCisCount == 0)
    {
        /* Remote has terminated the CIS so clear the streaming flag */
        inst->streaming = FALSE;
        
        /* If Remote device is Non CSIP then one stop stream indication will go to the App */
        if (ase)
        {
#ifdef INSTALL_LEA_UNICAST_CLIENT_REMOTE_STOP
            if (!capClientIsGroupCoordinatedSet(cap))
            {
                cisCount = capClientGetTotalCisCount((BapInstElement *)cap->bapList.first, cap->useCase, CAP_CLIENT_CIS_DIR_INVALID);
                capClientSendUnicastStopStreamInd(inst->profileTask, cap, bap->cid, ase->state, bap->cisHandles, cisCount);
            }
#endif
            /* Remove the cig only when remote is moving ASE's to idle/codec, if its qos config cig shall intact */
            if (ase && (ase->state == BAP_ASE_STATE_IDLE || ase->state == BAP_ASE_STATE_CODEC_CONFIGURED))
            {
                /* Change CAP state to stream stopped as all CIS's are disconnected*/
                cap->capState = CAP_CLIENT_STATE_STREAM_STOPPED;
                capClientSendUnicastClientRemoveCig(cap->activeCig);
                return;
            }
        }
        /* Change CAP state to unicast connected as all CIS's are disconnected but cig is active */
        cap->capState = CAP_CLIENT_STATE_UNICAST_CONNECTED;
    }
}

/****************************************************************************************/
/*****************************************Remove Datapath********************************/
/****************************************************************************************/

void capClientHandleDatapathRemoveCfm(CAP_INST* inst,
                                     BapRemoveDataPathCfm* cfm,
                                     CapClientGroupInstance* cap)
{
    BapInstElement* bap = (BapInstElement*)cap->bapList.first;
    BapAseElement* sinkAse = NULL;
    BapAseElement* sourceAse = NULL;

    while (bap)
    {
        sinkAse = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_CISHANDLE(bap->sinkAseList, cfm->isoHandle);
        sourceAse = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_CISHANDLE(bap->sourceAseList, cfm->isoHandle);

        if (sinkAse == NULL && sourceAse == NULL)
            bap = bap->next;
        else
            break;
    }

    if (bap)
        bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);

    inst->bapRequestCount--;
    CAP_CLIENT_INFO("\n(CAP) capClientHandleDatapathRemoveCfm : %d \n", inst->bapRequestCount);

    if (inst->bapRequestCount == 0)
    {
        bap = (BapInstElement*)(cap->bapList.first);

        /* Check if the Cap Instance belongs to Co ordinated Set */

        /* If co ordinated Set Build and send CIS disconnect Request to Each CIS Id
         * of every device in the Set
         * */

        /*BAP instance can never be NULL here*/
        if(bap)
            capClientSendUnicastClientCisDisconnect(bap, inst, cap->useCase, cap);
    }
}

static void capClientSendUnicastClientRemoveDatapathReq(uint32 bapHandle,
                                                       BapAseElement* ase,
                                                       CAP_INST* inst,
                                                       uint8 datapathDirection)
{
    uint8 datapath = CAP_DATAPATH_BITMASK << datapathDirection;

    for (;ase; ase = ase->next)
    {
        if (ase->removeDatapath &&
            (ase->datapathDirection & datapath))
        {
            inst->bapRequestCount++;
            CAP_CLIENT_INFO("\n(CAP) capClientSendUnicastClientRemoveDatapathReq : %d \n", inst->bapRequestCount);

            BapClientRemoveDataPathReq(bapHandle, ase->cisHandle, datapath);
        }
    }
}

static void capClientSendUnicastClientRemoveDatapath(BapInstElement* bap,
                                                    CAP_INST* inst,
                                                    uint8 sourceAses,
                                                    CapClientCisDirection cisDirection)
{
    BapAseElement* ase = NULL;

    while (bap)
    {
        if (bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
        {
            if (CAP_CLIENT_CIS_IS_UNI_SINK(cisDirection)
                || CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cisDirection))
            {
                ase = (BapAseElement*)(bap->sinkAseList.first);
                capClientSendUnicastClientRemoveDatapathReq(bap->bapHandle, ase, inst, DATAPATH_DIRECTION_INPUT);
            }

            if (CAP_CLIENT_CIS_IS_UNI_SRC(cisDirection)
                || (CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cisDirection) && sourceAses))
            {
                ase = (BapAseElement*)(bap->sourceAseList.first);
                capClientSendUnicastClientRemoveDatapathReq(bap->bapHandle, ase, inst, DATAPATH_DIRECTION_OUTPUT);
            }
        }
        else
        {
            CAP_CLIENT_INFO("\n(CAP)capSendUnicastClientRemoveDatapath, Device Status: 0x%x", bap->recentStatus);
        }
        bap = bap->next;
    }

}

/****************************************************************************************/
/*****************************************Reciever Stop Ready****************************/
/****************************************************************************************/

void capClientHandleBapAseRecieverStopReadyCfm(CAP_INST *inst,
                             BapUnicastClientReceiverReadyCfm* cfm,
                             CapClientGroupInstance *cap)
{
    BapInstElement *bap = (BapInstElement*)
                              CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);

    bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);

    /* Decrement the Request Counter whenver the CFM is recieved */
    inst->bapRequestCount--;
    CAP_CLIENT_INFO("\n(CAP) capClientHandleBapAseRecieverStopReadyCfm : %d \n", inst->bapRequestCount);


    /*
     * When bapRequestCount hits zero, the build and
     * send CIS connect request
     *
     * */

    if(inst->bapRequestCount == 0)
    {

        /* Check if the Cap Instance belongs to Co ordinated Set */

        /* If co ordinated Set Build and send CIS connect Request to Each CIS Id
        * of every device in the Set
        * */

        bap = (BapInstElement*)(cap->bapList.first);
        /* Do CIS disconnect */
        capClientSendUnicastClientCisDisconnect(bap, inst, cap->useCase, cap);
    }
}

void capClientHandleBapAseRecieverStopReadyInd(CAP_INST* inst,
                         BapUnicastClientReceiverReadyInd *ind,
                         CapClientGroupInstance *cap)
{
    CSR_UNUSED(inst);
    BapAseElement *ase = NULL;
    BapInstElement *bap = (BapInstElement*)
                           CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, ind->handle);

    /* if use case is Voice search in both sink and source list
     * else search only in sink list
     * */
    CAP_CLIENT_INFO("\n capHandleBapAseRecieverStopReadyInd: Stopped aseId: %d \n", ind->aseId);

    if(ind->result != BAP_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n capHandleBapAseRecieverStopReadyInd: Failed to Stop aseId: %d", ind->aseId);
    }

    ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sinkAseList, ind->aseId);

    if (ase == NULL)
    {
        ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sourceAseList, ind->aseId);
    }

    if (ase == NULL)
    {
        CAP_CLIENT_INFO("\n capHandleBapAseRecieverStopReadyInd: ASE is null \n");
        return;
    }

    ase->state = ind->aseState;
}


static void capClientSendUnicastClientRecieverStopReadyReq(BapInstElement *bap, CAP_INST *inst)
{
    CSR_UNUSED(inst);
    uint8 aseIdCount = capClientGetAseCountInState(BAP_ASE_STATE_DISABLING, bap, BAP_ASE_SOURCE);
    uint8* aseId = capClientGetAseIdForGivenCidAndState(bap->bapHandle,
                                                 BAP_ASE_SOURCE,
                                                 aseIdCount,
                                                 BAP_ASE_STATE_DISABLING, bap);

    if (aseIdCount)
    {
        inst->bapRequestCount++;
        CAP_CLIENT_INFO("\n(CAP) capClientSendUnicastClientRecieverStopReadyReq : %d \n", inst->bapRequestCount);

        BapUnicastClientReceiverReadyReq(bap->bapHandle, BAP_RECEIVER_STOP_READY, aseIdCount, aseId);
        CsrPmemFree(aseId);
        aseId = NULL;
    }
}

static void capClientSendUnicastClientRecieverStopReady(BapInstElement *bap, CAP_INST *inst)
{
    while(bap)
    {
        if(bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
        {
            capClientSendUnicastClientRecieverStopReadyReq(bap, inst);
        }
        else
        {
            CAP_CLIENT_INFO("\n(CAP)capSendUnicastClientRecieverStopReady, Device Status: 0x%x", bap->recentStatus);
        }

        bap = bap->next;
    }
}

/****************************************************************************************/
/*****************************************DISABLE****************************************/
/****************************************************************************************/


static void capClientSendUnicastClientAseDisableReq(BapInstElement *bap, CAP_INST *inst, CapClientContext useCase)
{
    uint8 index ;
    uint8 aseIdCount = capClientGetAseCountForUseCase(bap, useCase);
    uint8* aseId = capClientGetAseIdsInUse(bap, aseIdCount, useCase);
    BapAseParameters* param = (BapAseParameters*)CsrPmemZalloc(sizeof(BapAseParameters) * aseIdCount);

    for (index = 0; index < aseIdCount; index++)
        param[index].aseId = aseId[index];

    CAP_CLIENT_INFO("\n(CAP) capClientSendUnicastClientAseDisableReq bap->serverSinkSourceStreamCount : %d inst->bapRequestCount : %d\n", bap->serverSinkSourceStreamCount, inst->bapRequestCount);
    inst->bapRequestCount++;

    bap->serverSinkSourceStreamCount = capClientGetSrcAseCountForUseCase(bap, useCase);
    CAP_CLIENT_INFO("\n(CAP) capClientSendUnicastClientAseDisableReq bap->serverSinkSourceStreamCount : %d inst->bapRequestCount : %d\n", bap->serverSinkSourceStreamCount, inst->bapRequestCount);

    BapUnicastClientDisableReq(bap->bapHandle, aseIdCount, param);
    CsrPmemFree(aseId);
    CsrPmemFree(param);
    aseId = NULL;
    param = NULL;
}

static void capClientUnicastClientAseDisable(BapInstElement *bap, CAP_INST *inst, CapClientGroupInstance *cap)
{
    uint8 lockState = TRUE;
    CsipInstElement* csip = CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(cap->csipList, bap->bapHandle);

    if (csip)
    {
        lockState = capClientIsGroupCoordinatedSet(cap) ? csip->lock : TRUE;
    }

    while(bap && inst)
    {
        /* Disable only those BAP ASE's which are in streaming state */
        if((CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_STREAMING) && bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS && lockState)
        {
            capClientSendUnicastClientAseDisableReq(bap, inst, cap->useCase);
        }
        else
        { 
            CAP_CLIENT_INFO("\n(CAP)capClientUnicastClientAseDisable, bap->recentStatus: 0x%x bap->currentState :%x\n", bap->recentStatus, bap->bapCurrentState);
        }
        bap = bap->next;
    }
}


void capClientHandleUnicastAseDisableCfm(CAP_INST *inst,
                             BapUnicastClientDisableCfm* cfm,
                             CapClientGroupInstance *cap)
{
    BapInstElement *bap = (BapInstElement*)
                              CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);

    bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);

    /* Moved the BAP state to QOS configured again so that next time start stream can be performed
     * Bap Current State is meaningful only when status is success and is getting used just to skip
     * Operation on a discovered bap */
    setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_QOS_CONFIGURED, cap->activeCig->cigId);
    
    CAP_CLIENT_INFO("\n(CAP) capClientHandleUnicastAseDisableCfm  : %d bap->state : %x\n", bap->serverSinkSourceStreamCount, bap->bapCurrentState);

#ifdef CAP_CLIENT_NTF_TIMER
    /* Cancel the timer as all NTF is received */
    capClientNtfTimerReset(bap);
#endif

    if (bap->serverSinkSourceStreamCount == 0)
    {
        inst->bapRequestCount--;
        CAP_CLIENT_INFO("\n(CAP) capClientHandleUnicastAseDisableCfm :inst->bapRequestCount %d \n", inst->bapRequestCount);

        /*
         * When bapRequestCount hits zero, the build and
         * send CIS connect request
         *
         * */

        if (inst->bapRequestCount == 0)
        {

            /* Check if the Cap Instance belongs to Co ordinated Set */

            /* If co ordinated Set Build and send CIS connect Request to Each CIS Id
            * of every device in the Set
            * */
            bap = (BapInstElement*)(cap->bapList.first);

            if (inst->streaming)
            {
                if (cap->activeCig->configuredSrcAses)
                {
                    capClientSendUnicastClientRecieverStopReady(bap, inst);
                }
                else
                {
                    /* Do Cis disconnect */
                    capClientSendUnicastClientCisDisconnect(bap, inst, cap->useCase, cap);

                }
            }
            else
            {
                CAP_CLIENT_INFO("\n(CAP)capHandleUnicastAseDisableCfm:  Disable Successful!");
                capClientSendUnicastStopStreamCfm(inst->profileTask, inst,
                                                 CAP_CLIENT_RESULT_SUCCESS,
                                                 FALSE, bap, cap);
                return;
            }
        }
    }
}

void capClientHandleUnicastBapAseDisableInd(CAP_INST* inst,
                         BapUnicastClientDisableInd *ind,
                         CapClientGroupInstance *cap)
{
    CSR_UNUSED(inst);
    BapAseElement *ase = NULL;
    BapInstElement *bap = (BapInstElement*)
                           CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, ind->handle);

    /* if use case is Voice search in both sink and source list
     * else search only in sink list
     * */
    CAP_CLIENT_INFO("\n capHandleUnicastBapAseDisableInd: Disabled aseId: %d \n", ind->aseId);

    if(ind->result != BAP_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n capHandleUnicastBapAseDisableInd: Failed to Diasable aseId: %d", ind->aseId);
    }

    ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sinkAseList, ind->aseId);

    /* 
     *  The counter serverSourceStreamCount is used to track the number of 
     *  DisableIndications recieved for source ASES. Stop Stream Cfm in case of disable
     *  is sent only when this counter hits zero.
     *  
     *  We Generally recieve DisableIndications on both SINK and SOURCE ASEs
     *  Incase of Sink Stop Ready event is self triggered but CAP client needs to send Reciever Stop
     *  ready event in order to Transition Source ASEs to Disabled State. In both cases we recieve 
     *  BapUnicastClientDisableCfm, but we only track that related to Source ASEs making use of 
     *  'serverSourceStreamCount'.
     */
    if(ase == NULL)
    {
        CAP_CLIENT_ERROR("\n capHandleUnicastBapAseDisableInd: ASE id : %d \n", ind->aseId);

        ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sourceAseList, ind->aseId);
        if (ase)
            bap->serverSinkSourceStreamCount--;
    }

    if(ase == NULL)
    {
        CAP_CLIENT_ERROR("\n capHandleUnicastBapAseDisableInd: ASE is null \n");
        return;
    }

    ase->cisId = ind->cisId;
    ase->state = ind->aseState;
    ase->removeDatapath = TRUE;
}

/****************************************************************************************/
/*****************************************RELEASE****************************************/
/****************************************************************************************/

static void capClientSendUnicastClientAseReleaseReq(BapInstElement* bap, CAP_INST* inst, CapClientContext useCase)
{
    uint8 index = 0;
    uint8 aseIdCount = capClientGetAseCountForUseCase(bap, useCase);
    uint8* aseId = capClientGetAseIdsInUse(bap, aseIdCount, useCase);
    BapAseParameters* param = (BapAseParameters*)CsrPmemZalloc(sizeof(BapAseParameters) * aseIdCount);

    if (aseId == NULL)
    {
        CAP_CLIENT_INFO("\n capSendUnicastClientAseReleaseReq: No aseId in use");
        return;
    }

    for (index = 0; index < aseIdCount; index++)
        param[index].aseId = aseId[index];

    inst->bapRequestCount++;
    CAP_CLIENT_INFO("\n(CAP) capClientSendUnicastClientAseReleaseReq : %d \n", inst->bapRequestCount);

    BapUnicastClientReleaseReq(bap->bapHandle, aseIdCount, param);
    CsrPmemFree(aseId);
    CsrPmemFree(param);
    aseId = NULL;
    param = NULL;
}

static void capClientUnicastClientAseRelease(BapInstElement* bap, 
                                      CAP_INST* inst, 
                                      CapClientGroupInstance* cap)
{
    CsipInstElement* csip = (CsipInstElement*)cap->csipList.first;
    uint8 lockState = TRUE;

    while (bap && csip)
    {
        CapClientCigElem *cig = CAP_CLIENT_GET_CIG_FROM_CONTEXT(cap->cigList, cap->useCase);
        lockState = capClientIsGroupCoordinatedSet(cap) ? csip->lock : TRUE;

        /* Release only those BAP ASE's which are in streaming/QOS config state */
        if ((cig) && (CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, cig->cigId) == CAP_CLIENT_BAP_STATE_STREAMING
           || CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, cig->cigId) == CAP_CLIENT_BAP_STATE_QOS_CONFIGURED)
           && (bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS && lockState))
        {
            capClientSendUnicastClientAseReleaseReq(bap, inst, cap->useCase);
        }
        else
        {
            CAP_CLIENT_INFO("\n(CAP)capUnicastClientAseRelease, bap->recentStatus: 0x%x bap->currentState :%x\n", bap->recentStatus, bap->bapCurrentState);
        }

        bap = bap->next;
        csip = csip->next;
    }
}

void capClientHandleUnicastAseReleaseCfm(CAP_INST *inst,
                             BapUnicastClientReleaseCfm* cfm,
                             CapClientGroupInstance *cap)
{
    BapInstElement *bap = (BapInstElement*)
                              CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);
    CapClientCigElem* cig = CAP_CLIENT_GET_CIG_FROM_CONTEXT(cap->cigList, cap->useCase);

    bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);

    /* Moved the BAP state to QOS configured again so that next time start stream can be performed
     * Bap current state is meaningful only when status is success and is getting used just to skip
     * Operation on a discovered bap */
    if (cig)
        setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_AVLBLE_AUDIO_CONTEXT, cig->cigId);

	CAP_CLIENT_INFO("\n(CAP) capClientHandleUnicastAseReleaseCfm bap->state : %x\n", bap->bapCurrentState);

#ifdef CAP_CLIENT_NTF_TIMER
    /* Cancel the timer as all NTF is received */
    capClientNtfTimerReset(bap);
#endif

    CAP_CLIENT_INFO("\n capHandleUnicastAseReleaseCfm: requestCount: %d", inst->bapRequestCount);

    inst->bapRequestCount--;
    CAP_CLIENT_INFO("\n(CAP) capClientHandleUnicastAseReleaseCfm : %d \n", inst->bapRequestCount);

    /*
     * When bapRequestCount hits zero, the build and
     * send CIS connect request
     *
     * */

    if(inst->bapRequestCount == 0)
    {

        /* Check if the Cap Instance belongs to Co ordinated Set */

        /* If co ordinated Set Build and send CIS connect Request to Each CIS Id
         * of every device in the Set
         * */
        bap = (BapInstElement*)(cap->bapList.first);

        if (inst->streaming
            && cap->pendingOp != CAP_CLIENT_UNICAST_DISCONNECT)
        {
            capClientSendUnicastClientRemoveDatapath(bap, inst, cap->numOfSourceAses, cap->cigDirection);
        }
        else if (cap->pendingOp == CAP_CLIENT_UNICAST_DISCONNECT)
        {
            cig = CAP_CLIENT_GET_CIG_FROM_CONTEXT(cap->cigList, cap->useCase);
            capClientSendUnicastClientRemoveCig(cig);
        }
        else
        {
            CAP_CLIENT_INFO("\n(CAP)capHandleUnicastAseReleaseCfm: Device Release Successful!");
            capClientSendUnicastClientRemoveCig(cap->activeCig);
        }
    }
}

void capClientHandleBapAseReleasedInd(CAP_INST* inst,
                                  BapUnicastClientReleasedInd* ind,
                                  CapClientGroupInstance* cap)
{
    CSR_UNUSED(inst);
    BapAseElement* ase = NULL;
    CsipInstElement* csip = NULL;
    BapInstElement* bap = (BapInstElement*)
        CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, ind->handle);

    ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sinkAseList, ind->aseId);

    if (ase == NULL)
    {
        ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sourceAseList, ind->aseId);
    }

    if (ase == NULL)
    {
        CAP_CLIENT_ERROR("\n capHandleBapAseReleaseInd: ASE is null \n");
        return;
    }

    ase->state = ind->aseState;
    bap->releasedAses++;

    if (cap->stopComplete && capClientAsesReleaseComplete(cap))
    {
        if(capClientIsGroupCoordinatedSet(cap))
        {
            csip = (CsipInstElement*)(cap->csipList.first);

            /* Release the lock for set members */
            capClientSetCsisLockState(csip, &inst->csipRequestCount, FALSE);
        }
        else
        {
            cap->stopComplete = FALSE;
            bap = (BapInstElement*)cap->bapList.first;

            if (cap->pendingOp == CAP_CLIENT_BAP_UNICAST_RELEASE
                || cap->pendingOp == CAP_CLIENT_BAP_UNICAST_DISABLE)
            {
#ifdef CAP_CLIENT_NTF_TIMER
                /* Cancel the timer as all NTF is received */
                capClientNtfTimerReset(bap);
#endif
                capClientSendUnicastStopStreamCfm(inst->profileTask,
                                              inst, CAP_CLIENT_RESULT_SUCCESS,
                                              (cap->pendingOp == CAP_CLIENT_BAP_UNICAST_RELEASE),
                                               bap, cap);
            }
            else
            {
                capClientSendUnicastClientDisConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_SUCCESS);
            }
        }
    }
}

void capClientHandleBapAseReleaseInd(CAP_INST* inst,
                         BapUnicastClientReleaseInd *ind,
                         CapClientGroupInstance *cap)
{
    CSR_UNUSED(inst);
    BapAseElement *ase = NULL;
    BapInstElement *bap = (BapInstElement*)
                           CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, ind->handle);

    /* if use case is Voice search in both sink and source list
     * else search only in sink list
     * */

    if(ind->result != BAP_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n capHandleBapAseReleaseInd: Failed to Enable aseId: %d", ind->aseId);
    }

    ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sinkAseList, ind->aseId);

    if(ase == NULL)
    {
        ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sourceAseList, ind->aseId);
    }

    if(ase == NULL)
    {
        CAP_CLIENT_ERROR("\n capHandleBapAseReleaseInd: ASE is null \n");
        return;
    }

    ase->cisId = ind->cisId;
    ase->state = ind->aseState;
    ase->removeDatapath = TRUE;
}

/****************************************************************************************/

void capClientUnicastDisableReleaseReq(CapClientGroupInstance *cap, CAP_INST *inst)
{
    BapInstElement *bap =  (BapInstElement*)(cap->bapList.first);
    uint8 bapAseState = BAP_ASE_STATE_IDLE;

    if(cap->pendingOp == CAP_CLIENT_BAP_UNICAST_RELEASE
        || cap->pendingOp == CAP_CLIENT_UNICAST_DISCONNECT)
    {
        bapAseState = BAP_ASE_STATE_RELEASING;
        capClientUnicastClientAseRelease(bap, inst, cap);
    }
    else if(cap->pendingOp == CAP_CLIENT_BAP_UNICAST_DISABLE)
    {
         bapAseState = BAP_ASE_STATE_DISABLING;
         capClientUnicastClientAseDisable(bap, inst, cap);
    }

    CSR_UNUSED(bapAseState);
#ifdef CAP_CLIENT_NTF_TIMER
    /* Trigger the CAP timer to get all the NTF for enable within spec defined time */
    capClientNtfTimerSet(inst, cap, bap, bapAseState);
#endif
}

static void capClientUnicastStopStreamReqHandler(CAP_INST* inst,
                                                  void* msg,
                                                  CapClientGroupInstance* cap)
{
    CsipInstElement* csip = NULL;

    CapClientProfileMsgQueueElem* msgElem = (CapClientProfileMsgQueueElem*)msg;
    CapClientInternalUnicastStopStreamReq* req = (CapClientInternalUnicastStopStreamReq*)(msgElem->capMsg);
	
    /* Sanity Validation to make sure Streaming is triggered before calling stop stream */
    if (!cap->activeCig)
    {
#ifdef CAP_CLIENT_NTF_TIMER
        /* Reset the flag here, as in if timer expiry happend flag will be set to TRUE and will not be reset until new
         * timer is not kicked in but as the cap state failed reset the timer to send the confirmation */
        cap->capNtfTimeOut = FALSE;
#endif
        capClientSendUnicastStopStreamCfm(req->profileTask, inst, CAP_CLIENT_RESULT_STREAM_NOT_ACTIVE, FALSE, NULL, cap);
        return;
	}		

    /* Reject request if trying to disable in UNICAST_CONNECTED_STATE*/
    if (cap->capState == CAP_CLIENT_STATE_UNICAST_CONNECTED && !req->doRelease)
    {
        capClientSendUnicastStopStreamCfm(req->profileTask, inst, CAP_CLIENT_RESULT_STREAM_NOT_ACTIVE, FALSE, NULL, cap);
        CAP_CLIENT_INFO("\n handleUnicastStopStreamReq: Stream not active \n");
        return;
    }
	
    inst->profileTask = req->profileTask;
    cap->useCase = cap->activeCig->context;
    cap->pendingOp = req->doRelease ? CAP_CLIENT_BAP_UNICAST_RELEASE : CAP_CLIENT_BAP_UNICAST_DISABLE;
    cap->doRelease = req->doRelease;
    cap->stopComplete = FALSE;

    /* co ordinated set?
    *
    * Based on if co ordinated Set or not decide number of ASEs required
    * and then start BAP procedures
    *
    * */
    if (capClientIsGroupCoordinatedSet(cap))
    {
        csip = (CsipInstElement*)(cap->csipList.first);

        /* Here we need to obtain lock on all the devices and the
         * Start BAP unicast Procedures
         * */

         /* Check if the profile is already locked
          *
          * Note: If one device is in lock state in a group
          * it's assumed that all other participants are in lock state
          * */
        if (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientUnicastDisableReleaseReq(cap, inst);
        }
        /* Otherwise obtain lock and the start BAP Procedures */
        else
        {
            /* Store the metadata Param and then Obtain Lock on
             * All the Devices */
            capClientSetCsisLockState(csip, &inst->csipRequestCount, TRUE);
        }
    }
    else
    {
        /* Directly Send Disable/Release req */
        capClientUnicastDisableReleaseReq(cap, inst);
    }
}



void handleUnicastStopStreamReq(CAP_INST* inst, const Msg msg)
{
    uint16 result;
    AppTask appTask;
    CapClientInternalUnicastStopStreamReq* req = (CapClientInternalUnicastStopStreamReq*)msg;
    CapClientGroupInstance* cap = NULL;

    CapClientProfileTaskListElem* task = NULL;
    CapClientProfileMsgQueueElem* msgElem = NULL;
    CapClientBool isQueueEmpty = FALSE;

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);
    appTask = req->profileTask;


    if (cap == NULL)
    {
        capClientSendUnicastStopStreamCfm(appTask, inst, CAP_CLIENT_RESULT_INVALID_GROUPID, FALSE, NULL, NULL);
        return;
    }

    if (req->groupId != inst->activeGroupId)
    {
        result = CAP_CLIENT_RESULT_INVALID_GROUPID;
        /* Service ative group*/
        capClientSendUnicastStopStreamCfm(appTask, inst, result, FALSE, NULL, cap);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/

    task = (CapClientProfileTaskListElem*)
        CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

    if (task == NULL)
    {
        result = CAP_CLIENT_RESULT_TASK_NOT_REGISTERED;
        capClientSendUnicastStopStreamCfm(appTask, inst, result, FALSE, NULL, cap);
        return;
    }

    isQueueEmpty = CAP_CLIENT_IS_MSG_QUEUE_EMPTY(cap->capClientMsgQueue);

    msgElem = CapClientMsgQueueAdd(&cap->capClientMsgQueue,
                                  (void*)req,
                                  0,
                                  req->type,
                                  capClientUnicastStopStreamReqHandler,
                                  task);

    if (isQueueEmpty)
    {
        capClientUnicastStopStreamReqHandler(inst, (void*)msgElem, cap);
    }

}
#endif /* INSTALL_LEA_UNICAST_CLIENT */
