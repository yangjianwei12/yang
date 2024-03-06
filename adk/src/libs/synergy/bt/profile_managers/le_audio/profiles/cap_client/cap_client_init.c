/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "gatt_service_discovery_lib.h"
#include "cap_client_new_device_req.h"
#include "cap_client_debug.h"

#include "cap_client_init.h"
#include "cap_client_util.h"


#define CAP_CLIENT_INVALID_ROLE              (0)
#define CAP_ROLE_INVALID_MASK                (0xFC)
#define IS_CAP_ROLE_INVALID(_ROLE)           ((_ROLE == CAP_CLIENT_INVALID_ROLE) || \
                                                     ((_ROLE & CAP_ROLE_INVALID_MASK) == _ROLE))

#define CAP_CLIENT_MAX_CIGID                  (0xFF)

#ifdef INSTALL_LEA_UNICAST_CLIENT
#define CAP_MAX_ALLOWED_GROUPS                (2)

#endif 

#ifdef INSTALL_LEA_BROADCAST_SOURCE
static void InitBcastSrcList(CsrCmnListElm_t* elem)
{
    BroadcastSrcInst* srcInst = (BroadcastSrcInst*)elem;
    srcInst->appTask = CSR_SCHED_QID_INVALID;
    srcInst->bcastSrcProfileHandle = CAP_CLIENT_INVALID_CID;
    srcInst->numSubGroup = 0;
    srcInst->bcastSrcOpCnt.opReqCount = 0;
    srcInst->bcastSrcOpCnt.successCount = 0;
    srcInst->bcastSrcOpCnt.errorCount = 0;
    srcInst->bcastParam = NULL;

    srcInst->subGroupInfo = NULL;
    srcInst->bigId = CAP_CLIENT_INVALID_BIG_ID;
    srcInst->bcastBigParam = NULL;
    srcInst->mode = 0;
    srcInst->bigSyncDelay = 0;
    srcInst->qhsConfig.framing = 0;
    srcInst->qhsConfig.phy = 0;
    srcInst->qhsConfig.rtn = 0;
}

static void deInitBcastSrcList(CsrCmnListElm_t* elem)
{
    BroadcastSrcInst* srcInst = (BroadcastSrcInst*)elem;

    /* SubGroupInfo gets released during*/
    if (srcInst->bcastBigParam)
    {
        CsrPmemFree(srcInst->bcastBigParam);
        srcInst->bcastBigParam = NULL;
    }

    if (srcInst->bcastParam)
    {
        CsrPmemFree(srcInst->bcastParam);
        srcInst->bcastParam = NULL;
    }
}
#endif /* INSTALL_LEA_BROADCAST_SOURCE */

static void capClientInitMainInstance(CAP_INST* inst)
{
    /* Initialize the groupList*/

    inst->appTask = 0;
    inst->profileTask = 0;
    inst->cigId = CAP_CLIENT_MAX_CIGID;

#ifdef INSTALL_LEA_UNICAST_CLIENT
    inst->activeGroupId = INVALID_PHANDLE;
    inst->deviceCount = 0;
    inst->streaming = FALSE;
    inst->bapRequestCount = 0;
    inst->csipRequestCount = 0;
    inst->discoveryRequestCount = 0;
    inst->vcpRequestCount = 0;
    inst->vcpIndicationCount = 0;
    inst->addNewDevice = FALSE;
    inst->discoverSource = FALSE;
    inst->streaming = FALSE;

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    inst->micpRequestCount = 0;
#endif
#endif /* INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_SOURCE
    /*Initialize broadcast source List*/
    inst->capClientBcastSrcList = (CsrCmnList_t*)CsrPmemZalloc(sizeof(CsrCmnList_t));
    CsrCmnListInit(inst->capClientBcastSrcList, 0, InitBcastSrcList, deInitBcastSrcList);
#endif /*  INSTALL_LEA_BROADCAST_SOURCE */
}

#ifdef INSTALL_LEA_UNICAST_CLIENT
static void initCapClientGroup(CsrCmnListElm_t* elem)
{
    CapClientHandleElem* gElem = (CapClientHandleElem*)elem;

    /* Initialise all the Element lists inside CAP Instance */
    gElem->profileHandle = CAP_CLIENT_INVALID_SERVICE_HANDLE;
}

static void deInitBapList(CsrCmnListElm_t* elem)
{
    BapInstElement* bap = (BapInstElement*)elem;
    CsrPmemFree(bap->bapData);
    bap->bapData = NULL;
}

static void deInitVcpList(CsrCmnListElm_t* elem)
{
    VcpInstElement* vcp = (VcpInstElement*)elem;
    CsrPmemFree(vcp->vcsData);
    vcp->vcsData = NULL;
}

static void deInitCsipList(CsrCmnListElm_t* elem)
{
    CsipInstElement* csip = (CsipInstElement*)elem;
    CsrPmemFree(csip->csipData);
    csip->csipData = NULL;
}

static void InitRecordList(CsrCmnListElm_t* elem)
{
    CapClientBapPacRecord* rec = (CapClientBapPacRecord*)elem;

    rec->channelCount = 0;
    rec->isLc3Epc = FALSE;
    rec->lc3EpcVersionNum = 0;
    rec->preferredAudioContext = 0;
    rec->recordNum = 0;
    rec->streamCapability = 0;
    rec->streamingAudioContext = 0;
    rec->supportedMaxCodecFramesPerSdu = 0;
    rec->metadataLen = 0;
    rec->metadata = NULL;
}

static void deinitRecordList(CsrCmnListElm_t* elem)
{
    CapClientBapPacRecord* rec = (CapClientBapPacRecord*)elem;

    if (rec->metadataLen && rec->metadata)
    {
        CsrPmemFree(rec->metadata);
        rec->metadata = NULL;
    }
}

static void InitBapAseList(CsrCmnListElm_t* elem)
{
    BapAseElement* ase = (BapAseElement*)elem;

    ase->aseId = 0;
    ase->cisHandle = 0;
    ase->cisId = 0;
    ase->state = 0;
    ase->removeDatapath = FALSE;
    ase->useCase = 0;
    ase->datapathDirection = 0;
    ase->inUse = FALSE;
    ase->cig = NULL;
}

static void DeinitBapAseList(CsrCmnListElm_t* elem)
{
    CSR_UNUSED(elem);
    /* Nothing to free Here*/
}

static void InitBapList(CsrCmnListElm_t* elem)
{
    /* Initialize a BapInst list element. This function is called every
     * time a new entry is made on the Bap Inst Element list */
    BapInstElement* bElem = (BapInstElement*)elem;

    bElem->bapHandle = CAP_CLIENT_INVALID_CID;
    bElem->cid = CAP_CLIENT_INVALID_CID;
    bElem->cigId = CAP_CLIENT_INVALID_CIG_ID;
    bElem->sinkAseCount = 0;
    bElem->sourceAseCount = 0;
    bElem->groupId = CAP_CLIENT_INVALID_SERVICE_HANDLE;
#ifdef CAP_CLIENT_NTF_TIMER
    bElem->ntfTimer = CSR_SCHED_TID_INVALID;
#endif
    bElem->bapHandle = CAP_CLIENT_INVALID_CID;
    setBapStatePerCigId(bElem, CAP_CLIENT_BAP_STATE_INVALID, CAP_CLIENT_BAP_STATE_INVALID);

    CsrCmnListInit(&bElem->sinkAseList, 0, InitBapAseList, DeinitBapAseList);
    CsrCmnListInit(&bElem->sourceAseList, 0, InitBapAseList, DeinitBapAseList);
}

static void InitVcpList(CsrCmnListElm_t* elem)
{
    /* Initialize a VcpInst list element. This function is called every
     * time a new entry is made on the VCP Inst Element list */

    VcpInstElement* vElem = (VcpInstElement*)elem;

    vElem->cid = CAP_CLIENT_INVALID_CID;
    vElem->vcpHandle = CAP_CLIENT_INVALID_SERVICE_HANDLE;
    vElem->muteState = 0;
    vElem->volumeState = 0;
    vElem->groupId = CAP_CLIENT_INVALID_SERVICE_HANDLE;
    vElem->changeCounter = 0;
    vElem->expChangeCounter = 0;
    vElem->expVolumeState = 0;
    vElem->expMuteState = CAP_CLIENT_VCP_DEFAULT_MUTE_STATE;
}

static void InitCsipList(CsrCmnListElm_t* elem)
{
    /* Initialize a CsipInst list element. This function is called every
     * time a new entry is made on the CSIP Inst Element list */

    CsipInstElement* cElem = (CsipInstElement*)elem;

    cElem->cid = CAP_CLIENT_INVALID_CID;
    cElem->csipHandle = CAP_CLIENT_INVALID_SERVICE_HANDLE;
    cElem->groupId = CAP_CLIENT_INVALID_SERVICE_HANDLE;
    cElem->currentOperation = CAP_CLIENT_INTERNAL_INVALID_OPERATION;
}

static void InitProfileTaskList(CsrCmnListElm_t* elem)
{
    CapClientProfileTaskListElem* task = (CapClientProfileTaskListElem*)elem;

    task->profileTask = INVALID_PHANDLE;
    task->numOfSrcAses = 0;
    task->activeCig = NULL;
    task->vendorConfigDataLen = 0;
    task->vendorConfigData = NULL;
    task->sinkConfig = CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN;
    task->srcConfig = CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN;
    task->unicastParam = NULL;
}

static void InitCapClientMsgQueue(CsrCmnListElm_t* elem)
{
    CapClientProfileMsgQueueElem* msgElem = (CapClientProfileMsgQueueElem*)elem;

    msgElem->MessageType = 0;
    msgElem->capMsg = NULL;
    msgElem->confirmationRecieved = FALSE;
    msgElem->ExpectedIndCount = 0;
    msgElem->handlerFunc = NULL;
    msgElem->task = NULL;
}

static void deInitCapClientMsgQueue(CsrCmnListElm_t* elem)
{
    CapClientProfileMsgQueueElem* msgElem = (CapClientProfileMsgQueueElem*)elem;

    CsrPmemFree(msgElem->capMsg);
    msgElem->capMsg = NULL;
}

static void capClientAddtoCigList(CsrCmnListElm_t* elem)
{
    CapClientCigElem* cig = (CapClientCigElem*)elem;

    cig->cigDirection = 0;
    cig->cigId = 0xFF;
    cig->sinkConfig = 0;
    cig->srcConfig = 0;
    cig->configuredSrcAses = 0;
    cig->configureSinkAses = 0;
    cig->context = 0;
    cig->latency = 0;
    cig->sinkMaxPdelay = 0xFFFFFFFF;
    cig->sinkMinPdelay = 0;
    cig->srcMaxPdelay = 0xFFFFFFFF;
    cig->srcMinPdelay = 0;
    cig->state = CAP_CLIENT_STATE_INVALID;
    CsrMemSet(&cig->unicastParam, 0, sizeof(CapClientUnicastConnectParamV1));
}

static void capClientRemoveFromCigList(CsrCmnListElm_t* elem)
{
    CapClientCigElem* cig = (CapClientCigElem*)elem;
    cig->cisHandleCount = 0;
}

static void initCisHandlesList(CsrCmnListElm_t* elem)
{
    capClientCisHandlesListElem *cisHandlesElem = (capClientCisHandlesListElem*)elem; 

    cisHandlesElem->cisHandlesData = (cisData *)CsrPmemZalloc(sizeof(cisData));

    if (cisHandlesElem->cisHandlesData)
    {
        cisHandlesElem->cisHandlesData->cisHandle[0] = CAP_CLIENT_INVALID_SERVICE_HANDLE;
        cisHandlesElem->cisHandlesData->cisId        = CAP_CLIENT_INVALID_CIS_ID;
    }
}

static void deInitCisHandlesList(CsrCmnListElm_t* elem)
{
    capClientCisHandlesListElem *cisHandlesElem = (capClientCisHandlesListElem*)elem;

    if (cisHandlesElem->cisHandlesData)
    {
         CsrPmemFree(cisHandlesElem->cisHandlesData);
         cisHandlesElem->cisHandlesData = NULL;
    }
}


static void capClientInitElementList(CapClientGroupInstance *capClient)
{
    CsrCmnListInit(&capClient->bapList, 0, InitBapList, deInitBapList);
    CsrCmnListInit(&capClient->csipList, 0, InitCsipList, deInitCsipList);
    CsrCmnListInit(&capClient->vcpList, 0, InitVcpList, deInitVcpList);

    /* initialize sirk */
    capClient->sirk = (uint8*)CsrPmemZalloc(sizeof(uint8)*CAP_CLIENT_SIRK_SIZE);

    CsrCmnListInit(&capClient->capClientMsgQueue, 0, InitCapClientMsgQueue, deInitCapClientMsgQueue);
    CsrCmnListInit(&capClient->profileTaskList, 0, InitProfileTaskList, NULL);
    CsrCmnListInit(&capClient->cigList, 0, capClientAddtoCigList, capClientRemoveFromCigList);

    capClient->cisHandlesList = (CsrCmnList_t*)CsrPmemZalloc(sizeof(CsrCmnList_t));

    /* Initialize the cisHandlesList list */
    CsrCmnListInit(capClient->cisHandlesList, 0, initCisHandlesList, deInitCisHandlesList);

    capClient->sourceRecordList = (CsrCmnList_t*)CsrPmemZalloc(sizeof(CsrCmnList_t));
    capClient->sinkRecordList = (CsrCmnList_t*)CsrPmemZalloc(sizeof(CsrCmnList_t));

    /* Initialize the sourceRecordList and sinkRecordList list */
    CsrCmnListInit(capClient->sourceRecordList, 0, InitRecordList, deinitRecordList);
    CsrCmnListInit(capClient->sinkRecordList, 0, InitRecordList, deinitRecordList);

    capClient->bcastAsstState = CAP_CLIENT_BCAST_ASST_STATE_IDLE;

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    capClient->micpList = NULL;
#endif

}

void capClientSendInitCfm(CAP_INST const *inst, CapClientResult result)
{
    CapClientGroupInstance *cap = 
               (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);
    CapClientProfileTaskListElem* task = NULL;

    CAP_CLIENT_INFO("CAP:capClientSendInitCfm: Task init Instance address: %p\n", inst);

    MAKE_CAP_CLIENT_MESSAGE(CapClientInitCfm);

    message->deviceCount = 0;
    message->result = result;
    message->setAttrib = NULL;
    message->cid = 0;

    if(result == CAP_CLIENT_RESULT_INSUFFICIENT_RESOURCES)
        message->groupId = 0;
    else
        message->groupId = inst->activeGroupId;

    /* Copy the Sirk and Set size*/
    if (cap && ((result == CAP_CLIENT_RESULT_SUCCESS)
          || (result == CAP_CLIENT_RESULT_SUCCESS_DISCOVERY_ERR)))
    {
        /* Update CAP State */
        cap->capState = CAP_CLIENT_STATE_INIT;

        message->deviceCount = inst->deviceCount;
        message->setAttrib = (CapClientCoordinatedSetAttributes*)
                        CsrPmemZalloc(sizeof(CapClientCoordinatedSetAttributes));
        message->setAttrib->sirkType = cap->sirkType;
        CsrMemCpy(&message->setAttrib->sirk[0], &cap->sirk[0], CAP_CLIENT_SIRK_SIZE);

        /* Print sirk only when its encrypted for debugging */
        if (cap->sirkType == ENCRYPTED_SIRK)
        {
            uint8 i= 0;
            for (i = 0; i < CAP_CLIENT_SIRK_SIZE; i++)
                CAP_CLIENT_INFO(" 0x%x", cap->sirk[i]);
            CAP_CLIENT_INFO("\n\n");
        }

        message->setAttrib->setSize = cap->setSize;
        message->cid = cap->requestCid;

        /* Add application task to registered Task list */

        /* 
         * App which calls Cap Init is not required to register itself with CAP 
         * In order access unicast/Broadcast API's
         */

        task = (CapClientProfileTaskListElem*)CAP_CLIENT_ADD_TASK_TO_LIST(cap->profileTaskList);
        task->numOfSrcAses = 0;
        task->activeCig = NULL;
        task->profileTask = inst->appTask;
        task->vendorConfigDataLen = 0;
        task->vendorConfigData = NULL;
    }

    CapClientMessageSend(inst->appTask, CAP_CLIENT_INIT_CFM, message);
}


/*****************************************************************************************/

void handleCapClientInitReq(CAP_INST *inst, const Msg msg)
{
    CapClientInternalInitReq *req = (CapClientInternalInitReq*)msg;
    ServiceHandle prflHndl;
    CapClientGroupInstance *capClient = NULL;
    CapClientHandleElem* handleElem  = NULL; 

    if (inst == NULL || msg == NULL)
    {
        CAP_CLIENT_PANIC("\n handleCapInitReq: NULL instance\n");
        return;
    }

    if (inst->capClientGroupList.count >= CAP_MAX_ALLOWED_GROUPS)
    {
        capClientSendInitCfm(inst, CAP_CLIENT_RESULT_INSUFFICIENT_RESOURCES);
        CAP_CLIENT_INFO("\n handleCapInitReq:  Max groups supported =%x \n", CAP_MAX_ALLOWED_GROUPS);
        return;
    }

    prflHndl = CAP_CLIENT_ADD_GROUP_SERVICE_HANDLE_INST(capClient);

    if (prflHndl == CAP_CLIENT_INVALID_SERVICE_HANDLE || capClient == NULL)
    {
        capClientSendInitCfm(inst, CAP_CLIENT_RESULT_INSUFFICIENT_RESOURCES);
        CAP_CLIENT_PANIC("\n handleCapInitReq: Insufficient resource! \n");
        return;
    }

    if (IS_CAP_ROLE_INVALID(req->role))
    {
        /*Send CAP_RESULT_FAILURE*/
        capClientSendInitCfm(inst, CAP_CLIENT_RESULT_INVALID_ROLE);
        return;
    }

    /* Initialize the CAP group Instance elements */
    capClient->libTask = CSR_BT_CAP_CLIENT_IFACEQUEUE;
    capClient->appTask = req->appTask;
    capClient->broadcastCode = NULL;
    capClient->groupId = prflHndl;

    capClient->capVcpCmdCount = 0;
    capClient->forceRemoveCig = FALSE;
    
    inst->activeGroupId = prflHndl;
    inst->deviceCount = 1;
    inst->appTask = req->appTask;
    inst->profileTask = inst->appTask;
    inst->streaming = FALSE;

    capClientInitElementList(capClient);
    capClient->role = req->role;
    capClient->prevCisId = 0x00;
    capClient->numOfSourceAses = 0;
    capClient->useCase = CAP_CLIENT_CONTEXT_TYPE_PROHIBITED;
    capClient->activeCig = NULL;
    capClient->currentDeviceCount = 1;
    capClient->groupVolume = CAP_CLIENT_VCP_DEFAULT_VOLUME_STATE;
    capClient->groupMute = CAP_CLIENT_VCP_DEFAULT_MUTE_STATE;
    capClient->vcpPendingOp = CAP_CLIENT_INTERNAL_VCP_OP_NONE;
    capClient->csipStatus = CAP_CLIENT_RESULT_SUCCESS;

    capClient->metadataLen = 0;
    capClient->metadata = NULL;

    capClient->totalCisCount = 0;

    handleElem = (CapClientHandleElem*)CAP_CLIENT_ADD_CAP_GROUP_INST(&inst->capClientGroupList);
    handleElem->profileHandle = inst->activeGroupId;
    /* add all the devices in the InitData*/
    /* And Start CAS service discovery and CSIS Secondary service Discovery */

    if (req->initData)
    {
        capClient->requestCid = req->initData->cid;
        capClientInitCoordinatedSet(capClient);

        capClientInitAddProfileAndServices(capClient, req->initData->cid,
                                    req->initData->handles);

        GattServiceDiscoveryFindServiceRange(capClient->libTask,
                                             req->initData->cid,
                                             GATT_SD_CAS_SRVC);

        inst->discoveryRequestCount++;

        CsrPmemFree(req->initData->handles);
        req->initData->handles = NULL;

        CsrPmemFree(req->initData);
        req->initData = NULL;
    }
}
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

/**********************************************************************************************/

void CapClientTaskInit(void** gash)
{
    static CAP_INST* capInstanceMain;

    capInstanceMain = (CAP_INST*)CsrPmemAlloc(sizeof(CAP_INST));
   
    /*Initialize A CAP Group instance*/
    CAP_CLIENT_INFO("CAP: Task init Instance address: %p\n", capInstanceMain);

#ifdef INSTALL_LEA_UNICAST_CLIENT
    CsrCmnListInit(&capInstanceMain->capClientGroupList, 0, initCapClientGroup, NULL);
#endif

    capClientInitMainInstance(capInstanceMain);

    *gash = capInstanceMain;

}

#ifdef ENABLE_SHUTDOWN
void GattCapClientTaskDeinit(void** gash)
{
    CAP_INST* mainInst = *gash;
#ifdef INSTALL_LEA_UNICAST_CLIENT
    CsrCmnListDeinit(&mainInst->capClientGroupList);
#endif /* INSTALL_LEA_UNICAST_CLIENT */
#ifdef INSTALL_LEA_BROADCAST_SOURCE
    CsrCmnListDeinit(mainInst->capClientBcastSrcList);
    CsrPmemFree(mainInst->capClientBcastSrcList);
#endif /* INSTALL_LEA_BROADCAST_SOURCE */
    CsrPmemFree(mainInst);
    mainInst = NULL;
}
#endif
