/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "cap_client_remove_device_req.h"
#include "cap_client_bap_pac_record.h"
#include "cap_client_private.h"
#include "cap_client_debug.h"
#include "cap_client_util.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
static ServiceHandle CapClientGroupId;
static uint8 CapClientDestroyCount;


static CapClientResult capClientValidateCid(CapClientGroupInstance* cap, uint32 cid)
{
    CapClientResult result = CAP_CLIENT_RESULT_SUCCESS;

    if (cap->capState == CAP_CLIENT_STATE_INIT)
    {
        result = CAP_CLIENT_RESULT_PROFILES_NOT_INITIALIZED;
    }
    else if (cap->capState == CAP_CLIENT_STATE_STREAM_STARTED)
    {
        if (cid == 0)
        {
            result = CAP_CLIENT_RESULT_INVALID_OPERATION;
        }
        else
        {
            CsrBtTypedAddr addr;
            if (CsrBtGattClientUtilFindAddrByConnId(cid, &addr))
            {
                result = CAP_CLIENT_RESULT_INVALID_OPERATION;
            }
        }
    }
    else
    {
        if (cid != 0)
        {
            BapInstElement* bap = CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cid);
            VcpInstElement* vcp = CAP_CLIENT_GET_VCP_ELEM_FROM_CID(cap->vcpList, cid);
            CsipInstElement* csip = CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(cap->csipList, cid);

            if ((bap == NULL) && (vcp == NULL) && (csip == NULL))
            {
                result = CAP_CLIENT_RESULT_INVALID_OPERATION;
            }
        }
    }
    return result;
}

static void capClientResetMainInst(CAP_INST* inst)
{
    CapClientHandleElem* elem = (CapClientHandleElem*)
             CAP_CLIENT_GET_HANDLE_ELEM_FROM_GROUPID(&inst->capClientGroupList, CapClientGroupId);

    inst->addNewDevice = FALSE;
    inst->bapRequestCount = 0;
    inst->vcpRequestCount = 0;
    inst->csipRequestCount = 0;
    inst->csipReadType = 0;
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    inst->micpRequestCount = 0;
#endif
    inst->deviceCount = 0;
    inst->discoverSource = FALSE;
    inst->discoveryRequestCount = 0;
    inst->activeGroupId = CAP_CLIENT_INVALID_SERVICE_HANDLE;

    /* Free the capClientGroupList */
    CsrCmnListElementRemove(&inst->capClientGroupList, (CsrCmnListElm_t*)elem);
}

static void capClientFreeBapHandles(BapHandles **bapHandles)
{
    BapHandles *bapData = *bapHandles;

    if (bapData == NULL)
    {
        CAP_CLIENT_INFO("capClientFreeBapHandles: Nothing to Free here \n");
        return;
    }

    if (bapData->ascsHandles)
    {
        if (bapData->ascsHandles->sourceAseCount)
        {
            CsrPmemFree(bapData->ascsHandles->sourceAseId);
            CsrPmemFree(bapData->ascsHandles->sourceAseHandle);
            CsrPmemFree(bapData->ascsHandles->sourceAseCcdHandle);

            bapData->ascsHandles->sourceAseId = NULL;
            bapData->ascsHandles->sourceAseHandle = NULL;
            bapData->ascsHandles->sourceAseCcdHandle = NULL;
        }

        if (bapData->ascsHandles->sinkAseCount)
        {
            CsrPmemFree(bapData->ascsHandles->sinkAseId);
            CsrPmemFree(bapData->ascsHandles->sinkAseHandle);
            CsrPmemFree(bapData->ascsHandles->sinkAseCcdHandle);

            bapData->ascsHandles->sinkAseId = NULL;
            bapData->ascsHandles->sinkAseHandle = NULL;
            bapData->ascsHandles->sinkAseCcdHandle = NULL;
        }

        CsrPmemFree(bapData->ascsHandles);
        bapData->ascsHandles = NULL;
    }

    if (bapData->pacsHandles)
    {
        if (bapData->pacsHandles->sourcePacRecordCount)
        {
            CsrPmemFree(bapData->pacsHandles->pacsSourcePacRecordHandle);
            CsrPmemFree(bapData->pacsHandles->pacsSourcePacRecordCcdHandle);

            bapData->pacsHandles->pacsSourcePacRecordCcdHandle = NULL;
            bapData->pacsHandles->pacsSourcePacRecordHandle = NULL;
        }

        if (bapData->pacsHandles->sinkPacRecordCount)
        {
            CsrPmemFree(bapData->pacsHandles->pacsSinkPacRecordHandle);
            CsrPmemFree(bapData->pacsHandles->pacsSinkPacRecordCcdHandle);

            bapData->pacsHandles->pacsSinkPacRecordCcdHandle = NULL;
            bapData->pacsHandles->pacsSinkPacRecordHandle = NULL;
        }


        CsrPmemFree(bapData->pacsHandles);
        bapData->pacsHandles = NULL;
    }

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
    if (bapData->bassHandles)
    {
        if (bapData->bassHandles->broadcastSourceNum)
        {
            CsrPmemFree(bapData->bassHandles->broadcastReceiveStateHandleCcc);
            CsrPmemFree(bapData->bassHandles->broadcastReceiveStateHandle);

            bapData->bassHandles->broadcastReceiveStateHandleCcc = NULL;
            bapData->bassHandles->broadcastReceiveStateHandle = NULL;
        }

        CsrPmemFree(bapData->bassHandles);
        bapData->bassHandles = NULL;
    }
#endif

    CsrPmemFree(*bapHandles);
    *bapHandles = NULL;
}
static void capClientSendRemoveDeviceCfm(AppTask appTask,
                                    CapClientResult result)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientRemoveDeviceCfm);
    CAP_CLIENT_INFO("\n(CAP) : capClientSendRemoveDeviceCfm result: 0x%x \n", result);

    message->result = result;
    message->groupId = CapClientGroupId;

    CapClientMessageSend(appTask, CAP_CLIENT_REMOVE_DEV_CFM, message);
}

static void capClientCapInstanceRemoveCfmSend(CapClientGroupInstance* gInst,
                                              CAP_INST* inst)
{
    AppTask appTask = inst->appTask;
    /* Send Device Remove Cfm*/
    capClientSendRemoveDeviceCfm(appTask, CAP_CLIENT_RESULT_SUCCESS);
    gInst->profileListSort = FALSE;

    /* Completely Free remove the CapClientGroup Instance */
    if (CapClientDestroyCount == gInst->currentDeviceCount)
    {
        /* Deinit all the lists */
        CsrCmnListDeinit(&gInst->profileTaskList);
        CsrCmnListDeinit(&gInst->capClientMsgQueue);
        CsrCmnListDeinit(&gInst->cigList);

        if (gInst->cisHandlesList)
        {
             CsrCmnListDeinit(gInst->cisHandlesList);
             CsrPmemFree(gInst->cisHandlesList);
             gInst->cisHandlesList = NULL;
        }

        if (gInst->sourceRecordList)
        {
             CsrCmnListDeinit(gInst->sourceRecordList);
             CsrPmemFree(gInst->sourceRecordList);
             gInst->sourceRecordList = NULL;
        }

        if (gInst->sinkRecordList)
        {
             CsrCmnListDeinit(gInst->sinkRecordList);
             CsrPmemFree(gInst->sinkRecordList);
             gInst->sinkRecordList = NULL;
        }

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
        if (gInst->micpList)
        {
            CsrCmnListDeinit(gInst->micpList);
            CsrPmemFree(gInst->micpList);
            gInst->micpList = NULL;
        }
#endif /* EXCLUDE_CSR_BT_MICP_MODULE */

        if (gInst->sirk)
        {
            CsrPmemFree(gInst->sirk);
            gInst->sirk = NULL;
        }
		
        if (gInst->metadataLen && gInst->metadata != NULL)
        {
            CAP_CLIENT_INFO("(CAP) : metadata %x len %d \n", gInst->metadata, gInst->metadataLen);
            gInst->metadataLen = 0;
            CsrPmemFree(gInst->metadata);
            gInst->metadata = NULL;
        }

        /* Reset all the Variables in Main Instance*/
        capClientResetMainInst(inst);

        if (!CAP_CLIENT_REMOVE_GROUP_INST_DATA(CapClientGroupId))
        {
            CAP_CLIENT_ERROR("(CAP) : capClientRemoveGroup unable to free instance data \n");
        }
    }
    else
    {
        gInst->currentDeviceCount--;
        inst->deviceCount--;
        CAP_CLIENT_INFO("(CAP) : capClientCapInstanceRemoveCfmSend decrement the device count %d \n", inst->deviceCount);
    }

    /* Send the CFM to the application */
}

static void capClientCleanupGroupInst(CapClientGroupInstance* cap, CAP_INST *inst)
{
    CsipInstElement* csip = (CsipInstElement*)
        ((CsrCmnListElm_t*)(cap->csipList.first));
    VcpInstElement* vcp = (VcpInstElement*)((CsrCmnListElm_t*)(cap->vcpList.first));
    BapInstElement* bap = (BapInstElement*)((CsrCmnListElm_t*)(cap->bapList.first));
	
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    MicpInstElement* micp = NULL;

    /* First check if the MICP list is existing */
    if (cap->micpList)
        micp = (MicpInstElement*)(cap->micpList->first);
#endif

    CsrCmnListElm_t* tmp;

    while (bap)
    {
        tmp = (CsrCmnListElm_t*)bap->next;

        if (bap->bapHandle == INVALID_CONNID)
        {
            CsrPmemFree(bap->bapData);
            bap->bapData = NULL; /* Should not be freed when instance is removed*/
            CsrCmnListElementRemove(&cap->bapList, (CsrCmnListElm_t*)bap);
        }
        bap = (BapInstElement*)tmp;
    }

    while (csip)
    {
        tmp = (CsrCmnListElm_t*)csip->next;
        if (csip->csipHandle == CAP_CLIENT_INVALID_SERVICE_HANDLE)
        {
            CsrPmemFree(csip->csipData);
            csip->csipData = NULL; /* Should not be freed when instance is removed*/
            CsrCmnListElementRemove(&cap->csipList, (CsrCmnListElm_t*)csip);
        }
        csip = (CsipInstElement*)tmp;
    }

    while (vcp)
    {
        tmp = (CsrCmnListElm_t*)vcp->next;
        if (vcp->vcpHandle == CAP_CLIENT_INVALID_SERVICE_HANDLE)
        {
            CsrPmemFree(vcp->vcsData);
            vcp->vcsData = NULL; /* Should not be freed when instance is removed*/
            CsrCmnListElementRemove(&cap->vcpList, (CsrCmnListElm_t*)vcp);
        }
        vcp = (VcpInstElement*)tmp;
    }

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    while (micp)
    {
        tmp = (CsrCmnListElm_t*)micp->next;
        if (micp->micpHandle == CAP_CLIENT_INVALID_SERVICE_HANDLE)
        {
            CsrPmemFree(micp->micsData);
            micp->micsData = NULL; /* Should not be freed when instance is removed*/
            CsrCmnListElementRemove(cap->micpList, (CsrCmnListElm_t*)micp);
        }
        micp = (MicpInstElement*)tmp;
    }
#endif

    if (cap->vcpList.count == 0 &&
            cap->bapList.count == 0 &&
              cap->csipList.count == 0 
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
        && 
        (cap->micpList && cap->micpList->count == 0)
#endif
    )
    {
        capClientCapInstanceRemoveCfmSend(cap, inst);
    }

}

static void capClientCleanupConfiguredCigs(CapClientGroupInstance* capClient, 
                                           uint32 cid,
                                           bool isStreaming)
{

    CapClientCigElem* cig = (CapClientCigElem*)capClient->cigList.first;

    /* Remove all the Configured CIGs. This has to happen only when       *
     * the request is to destroy the CAP Group or currentDeviceCount is 1 */

    if (cid == 0 || capClient->currentDeviceCount == 1)
    {

        uint8 activeCigId = CAP_CLIENT_INVALID_CIG_ID;
        capClient->forceRemoveCig = TRUE;

        cig = (CapClientCigElem*)capClient->cigList.first;

        if (capClient->activeCig)
            activeCigId = capClient->activeCig->cigId;

        for (; cig; cig = cig->next)
        {
            if (!(cig->cigId == activeCigId && isStreaming))
                BapUnicastClientCigRemoveReq(CSR_BT_CAP_CLIENT_IFACEQUEUE, cig->cigId);
        }
    }

}

static void storeCisHandles(CapClientGroupInstance* cap, BapInstElement *bap)
{

    /* First check for the list count if count is zero it means remove device is called for All the members in the group
     * But as we are not storing the cid from Application so one way to know the cap device list for csip if its zero then cap remove is called
     * For all the devices in the group */
    if (cap->csipList.count != 0)
    {
        uint8 cisCount;
        uint8 i;

        cisCount = NO_OF_CIS_HANDLES_PER_DEVICE;
        BapAseElement* ase = (BapAseElement*)(bap->sinkAseList.first);


        if (ase == NULL)
        {
            ase = (BapAseElement*)(bap->sourceAseList.first);
        }

        for (i = 0; cisCount && i < cisCount && ase; ase = ase->next)
        {
            CAP_CLIENT_INFO("\n(CAP)storeCisHandles: cisCount: %x ase state : %x ase in use :%x\n", cisCount, ase->state, ase->inUse);
            if (ase->state >= BAP_ASE_STATE_CODEC_CONFIGURED &&
                ase->inUse && ase->useCase == cap->useCase)
            {
                capClientCisHandlesListElem *cisHandlesEntry = (capClientCisHandlesListElem*)CAP_CLIENT_ADD_CIS_HANDLES(cap->cisHandlesList);
                cisHandlesEntry->cisHandlesData = (cisData *)CsrPmemZalloc(sizeof(cisData));

                cisHandlesEntry->cisHandlesData->cisHandle[0] = ase->cisHandle;
                cisHandlesEntry->cisHandlesData->cisId = ase->cisId;
                CAP_CLIENT_INFO("\n(CAP)storeCisHandles: ase->cisHandle: %x cisId: %x\n",ase->cisHandle, cisHandlesEntry->cisHandlesData->cisId);
            }
        }
    }
}

void capClientRemoveGroup(Msg msg, CAP_INST* const inst, CapClientProfile profile)
{
    CapClientGroupInstance * gInst = CAP_CLIENT_GET_GROUP_INST_DATA(CapClientGroupId);

    if (gInst == NULL)
    {
        CAP_CLIENT_INFO("capClientRemoveGroup: gInst is NULL");
        return;
    }

    /* Clean up BAP/CSIP/VCP Instances which are not initialized */

    switch(profile)
    {
        case CAP_CLIENT_CSIP:
        {
            CsipDestroyCfm* cfm = (CsipDestroyCfm*)msg;
            CsipInstElement *csip =
                   (CsipInstElement*)CAP_CLIENT_GET_CSIP_ELEM_FROM_PHANDLE(gInst->csipList, cfm->prflHndl);

            CAP_CLIENT_INFO("\n(CAP) : capClientRemoveGroup CSIP: Profile Handle: 0x%x \n", cfm->prflHndl);

            if (cfm->status != CSIP_STATUS_IN_PROGRESS)
            {
                inst->csipRequestCount--;
            }

            if(cfm->status == CSIP_STATUS_SUCCESS)
            {
                CsrPmemFree(cfm->handles);
                cfm->handles = NULL;
                csip->csipHandle = 0;

                CsrPmemFree(csip->csipData);
                csip->csipData = NULL; /* Should not be freed when instance is removed*/
                CsrCmnListElementRemove(&gInst->csipList, (CsrCmnListElm_t*)csip);
            }
            else if (cfm->status != CSIP_STATUS_IN_PROGRESS)
            {
                CAP_CLIENT_PANIC("\n(CAP) : Unable to remove CSIP!! \n");
            }
        }
        break;

        case CAP_CLIENT_BAP:
        {
            BapDeinitCfm* cfm = (BapDeinitCfm*)msg;
            BapInstElement *bap =
                  (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(gInst->bapList, cfm->handle);

            inst->bapRequestCount--;
            CAP_CLIENT_INFO("\n(CAP) : capClientRemoveGroup BAP: Profile Handle: 0x%x, role: %d \n", cfm->handle, cfm->role);

            if (cfm->result == BAP_RESULT_SUCCESS)
            {
                capClientResetCachedAudioLoc(bap->cid);
                capClientFreeBapHandles(&cfm->handles);
                cfm->handles = NULL;
                bap->bapHandle = 0;

                /* Cis handle store logic is only applicable for CSIP based remote devices */
                if (gInst->setSize > 1)
                {
                    storeCisHandles(gInst, bap);

                    /* Bap cis handls will get freed as part of unicast connect operation but for link loss and other failure cases it may possible it wont get freed 
                     * So check here while removing the bap instance if the cis handles memory is still allocated then free it as part of BAP removal */
                    if (bap->cisHandles)
                    {
                        CsrPmemFree(bap->cisHandles);
                        bap->cisHandles = NULL;
                    }
                }

                if (bap->sinkAseCount)
                {
                    CsrCmnListDeinit(&bap->sinkAseList);
                }

                if (bap->sourceAseCount)
                {
                    CsrCmnListDeinit(&bap->sourceAseList);
                }


                capClientFreeBapHandles(&bap->bapData);
                bap->bapData = NULL; /* Should not be freed when instance is removed*/

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
                if ((gInst->role & CAP_CLIENT_COMMANDER) && bap->bass)
                {
                    CsrPmemFree(bap->bass);
                    bap->bass = NULL;
                }
#endif
                CsrCmnListElementRemove(&gInst->bapList, (CsrCmnListElm_t*)bap);
            }
            else if( cfm->result != BAP_RESULT_INPROGRESS)
            {
                CAP_CLIENT_PANIC("\n(CAP) : Unable to remove BAP!! \n");
            }
        }
        break;

        case CAP_CLIENT_VCP:
        {
            /* VCP when deinitialized sends two messages VcpVcsTerminateCfm and VcpDestroyCfm *
             * Incase of BAP and CSIP, only one message is sent by each profiles when Profile *
             *  DeInit is called. Hence the difference in handling the CFMs*/

            if(msg)
            {
                VcpVcsTerminateCfm* cfm = (VcpVcsTerminateCfm*)msg;
                VcpInstElement *vcp =
                    (VcpInstElement*)CAP_CLIENT_GET_VCP_ELEM_FROM_PHANDLE(gInst->vcpList, cfm->prflHndl);
                CAP_CLIENT_INFO("\n(CAP) : capClientRemoveGroup VCP: Profile Handle: 0x%x \n", cfm->prflHndl);

                if (cfm->status == VCP_STATUS_SUCCESS)
                {
                    vcp->vcpHandle = 0;

                    CsrPmemFree(vcp->vcsData);
                    vcp->vcsData = NULL; /* Should not be freed when instance is removed*/
                    CsrCmnListElementRemove(&gInst->vcpList, (CsrCmnListElm_t*)vcp);
                }
                else if (cfm->status != VCP_STATUS_IN_PROGRESS)
                {
                    CAP_CLIENT_PANIC("\n(CAP) : Unable to remove VCP!! \n");
                }
            }
            else
            {
                inst->vcpRequestCount--;
            }
        }
        break;

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
        case CAP_CLIENT_MICP:
        {
            /* MICP when deinitialized sends two messages MicpVcsTerminateCfm and MicpDestroyCfm *
             * Incase of BAP and CSIP, only one message is sent by each profiles when Profile *
             *  DeInit is called. Hence the difference in handling the CFMs*/

            if(msg)
            {
                MicpMicsTerminateCfm *cfm = (MicpMicsTerminateCfm*)msg;
                MicpInstElement *micp =
                    (MicpInstElement*)CAP_CLIENT_GET_MICP_ELEM_FROM_PHANDLE(gInst->micpList, cfm->prflHndl);

                CAP_CLIENT_INFO("\n(CAP) : capClientRemoveGroup MICP: Profile Handle: 0x%x  cfm->status : %x\n", cfm->prflHndl, cfm->status);

                if (cfm->status == MICP_STATUS_SUCCESS)
                {
                    micp->micpHandle = 0;

                    CsrPmemFree(micp->micsData);
                    micp->micsData = NULL; /* Should not be freed when instance is removed*/
                    CsrCmnListElementRemove(gInst->micpList, (CsrCmnListElm_t*)micp);
                }
                else if (cfm->status != MICP_STATUS_IN_PROGRESS)
                {
                    CAP_CLIENT_ERROR("\n(CAP) : Unable to remove MICP!! \n");
                }
            }
            else
            {
                inst->micpRequestCount--;
            }
        }
        break;
#endif

        default:
            break;
    }

    CAP_CLIENT_INFO("\n(CAP) capClientRemoveGroup: inst->bapRequestCount = %d, inst->vcpRequestCount = %d, inst->csipRequestCount = %d \n",
        inst->bapRequestCount, inst->vcpRequestCount, inst->csipRequestCount);

    CAP_CLIENT_INFO("\n(CAP) capClientRemoveGroup: BapListCount = %d, VcpListCount = %d, CsipListCount = %d \n",
                                              gInst->bapList.count, gInst->vcpList.count, gInst->csipList.count);

    if((!inst->bapRequestCount && !inst->vcpRequestCount && !inst->csipRequestCount 
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
        && !inst->micpRequestCount
#endif
    )
         ||(!gInst->bapList.count && !gInst->vcpList.count && !gInst->csipList.count
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
             &&
            ( !(gInst->micpList && gInst->micpList->count))
#endif
    ))
    {
        capClientCapInstanceRemoveCfmSend(gInst, inst);
    }
    else
    {
        /* Can't Panic here since every time Cfm arrives control flows into else case*/
        /* TODO: Handle Per device removal in case of Standard LE audio Solution */
    }
}

void handleCapClientRemoveDeviceReq(CAP_INST* inst, const Msg msg)
{
    CapClientGroupInstance *capClient = NULL;
    CsipInstElement *csip = NULL;
    BapInstElement *bap = NULL;
    VcpInstElement *vcp = NULL;
	
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    MicpInstElement* micp = NULL;
#endif

    CapClientInternalRemoveDevReq *req = (CapClientInternalRemoveDevReq*)msg;
    BapRole role = 0;
    CapClientResult result = CAP_CLIENT_RESULT_SUCCESS;

    capClient = CAP_CLIENT_GET_GROUP_INST_DATA(req->groupId);
    CapClientGroupId = req->groupId;

    if (capClient == NULL || msg ==  NULL)
    {
        /* Return Error */
        capClientSendRemoveDeviceCfm(inst->appTask, CAP_CLIENT_RESULT_INVALID_GROUPID);
        return;
    }

    result = capClientValidateCid(capClient, req->cid);

    if (result != CAP_CLIENT_RESULT_PROFILES_NOT_INITIALIZED
                      && result != CAP_CLIENT_RESULT_SUCCESS)
    {
        capClientSendRemoveDeviceCfm(inst->appTask, result);
        CAP_CLIENT_INFO("\n capRemoveDeviceReq: invalid state transition \n");
        return;
    }
    else if (result == CAP_CLIENT_RESULT_PROFILES_NOT_INITIALIZED)
    {
        CAP_CLIENT_INFO("(CAP) : capRemoveDeviceReq  Free before initstreamandControlreq \n");
        inst->csipRequestCount = 0;
        inst->vcpRequestCount = 0;
        inst->bapRequestCount = 0;
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
        inst->micpRequestCount = 0;
 #endif
        CapClientDestroyCount = capClient->currentDeviceCount;
    }

    if (capClient->role & CAP_CLIENT_INITIATOR)
        role |= BAP_ROLE_UNICAST_CLIENT;
    
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
    if (capClient->role & CAP_CLIENT_COMMANDER)
        role |= BAP_ROLE_BROADCAST_ASSISTANT;
#endif


    /* Remove all the Configured CIGs. This has to happen only when       *
     * the request is to destroy the CAP Group or currentDeviceCount is 1 */

    capClientCleanupConfiguredCigs(capClient, req->cid, inst->streaming);

    /* If Invalid Cid Deinit all the device in the Group*/

    /*Clean up uninitialised Group Instance */
    capClientCleanupGroupInst(capClient, inst);

    capClient = CAP_CLIENT_GET_GROUP_INST_DATA(req->groupId);

    if (capClient == NULL)
    {
        return;
    }

    if(req->cid == 0)
    {

        csip = (CsipInstElement*)((CsrCmnListElm_t*)(capClient->csipList.first));
        CapClientDestroyCount = capClient->currentDeviceCount;

        for (;csip && (csip->csipHandle != 0);csip = csip->next)
        {
            inst->csipRequestCount++;
            CsipDestroyReq(csip->csipHandle);
        }

        /* Kill BAP/VCP instances*/
        bap = (BapInstElement*)((CsrCmnListElm_t*)(capClient->bapList.first));

        for(;bap && (bap->bapHandle != 0);bap = bap->next)
        {
            inst->bapRequestCount++;
            BapDeinitReq(bap->bapHandle, role);
            /* Directly Kill BAP/VCP instance*/
        }

        vcp = (VcpInstElement*)((CsrCmnListElm_t*)(capClient->vcpList.first));

        for(;vcp && (vcp->vcpHandle != 0);vcp = vcp->next)
        {
            inst->vcpRequestCount++;
            VcpDestroyReq(vcp->vcpHandle);
            /* Directly Kill BAP/VCP instance*/
        }

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
        if (capClient->micpList)
            micp = (MicpInstElement*)((CsrCmnListElm_t*)(capClient->micpList->first));

        for (; micp && (micp->micpHandle != 0); micp = micp->next)
        {
            /* Kill MICP instance*/
            inst->micpRequestCount++;
            MicpDestroyReq(micp->micpHandle);
        }
#endif
    }
    /* If CID is passed alongside group ID remove only that particular Device
     * from the Group*/
    else
    {
        CsipInstElement *csipElem =
                   (CsipInstElement*)CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(capClient->csipList, req->cid);
        bap =
                   (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(capClient->bapList, req->cid);
        vcp =
                   (VcpInstElement*)CAP_CLIENT_GET_VCP_ELEM_FROM_CID(capClient->vcpList, req->cid);

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
        if (capClient->micpList)
            micp =
                   (MicpInstElement*)CAP_CLIENT_GET_MICP_ELEM_FROM_CID(capClient->micpList, req->cid);
#endif

        if (csipElem == NULL && bap == NULL && vcp == NULL
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
            &&
            micp == NULL
#endif
         )
        {
            CAP_CLIENT_ERROR("(CAP) : capRemoveDeviceReq  Invalid Cid \n");
            capClientSendRemoveDeviceCfm(inst->appTask, CAP_CLIENT_RESULT_INVALID_PARAMETER);
            return;
        }

        CapClientDestroyCount = 1;


        if(csipElem != NULL && csipElem->csipHandle != 0)
        {
            inst->csipRequestCount++;
            CsipDestroyReq(csipElem->csipHandle);
        }

        if(bap!= NULL && bap->bapHandle != 0)
        {
            inst->bapRequestCount++;
            BapDeinitReq(bap->bapHandle, role);
        }

        if(vcp != NULL && vcp->vcpHandle != 0)
        {
            inst->vcpRequestCount++;
            VcpDestroyReq(vcp->vcpHandle);
        }

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
        if (micp != NULL && micp->micpHandle != 0)
        {
            inst->micpRequestCount++;
            MicpDestroyReq(micp->micpHandle);
        }
#endif
    }
}
#endif
