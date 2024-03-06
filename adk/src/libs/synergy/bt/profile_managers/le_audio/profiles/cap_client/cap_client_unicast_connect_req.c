/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_common.h"
#include "cap_client_util.h"
#include "cap_client_ase.h"
#include "cap_client_csip_handler.h"
#include "cap_client_bap_pac_record.h"
#include "cap_client_debug.h"
#include "cap_client_unicast_connect_req.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

#define CAP_CLIENT_RESET_DP_REQ_COUNT(_BAP)  (_BAP->datapathReqCount = 0)

#define CAP_CLIENT_GET_MIN(_X, _Y)  ((_X > _Y) ? _Y : _X )

#define CAP_CLIENT_GENERATE_CIS_PRIM_NUM          (0x98)
#define CAP_CLIENT_GENERATE_CIS_LSHIFT            (0x03)

#define CAP_CLIENT_CIG_ID_MAX                     (0xEF)
#define CAP_CLIENT_MIN_PARAM_ELEMS                (0x01)

/*! RTN from Central to Peripheral value for Gaming */
#define CAP_CLIENT_APTX_LITE_RTN_GAME_C_TO_P_DEFAULT    (0x4)

/*! RTN from Peripheral to Central value for Gaming */
#define CAP_CLIENT_APTX_LITE_RTN_GAME_P_TO_C_DEFAULT    (0x9)

/*! RTN from Central to Peripheral value for Aptx adaptive (Media) */
#define CAP_CLIENT_APTX_ADAPTIVE_RTN_MEDIA_C_TO_P_DEFAULT    (15)

/*! RTN from Peripheral to Central value for Aptx adaptive (Media) */
#define CAP_CLIENT_APTX_ADAPTIVE_RTN_MEDIA_P_TO_C_DEFAULT    (15)

/*! Worst case Sleep clock accuracy */
#define CAP_CLIENT_WORST_CASE_SCA        (0x00)              /* 251 ppm to 500 ppm */

uint8 cigID[CAP_CLIENT_MAX_SUPPORTED_CIGS] = {0,0,0,0};

/******************************************************************************************************/
/***************                   BapUnicastSetUpdataPathReq          ********************************/
/******************************************************************************************************/

static void capClientSetupDataPathReq(BapInstElement* bap,
                                      CAP_INST* inst,
                                      CapClientGroupInstance* cap,
                                      uint16 isoHandle,
                                      uint32 bapHandle,
                                      uint32 audioLocation,
                                      bool isSink)
{
    BapSetupDataPath* dataPath;
    CapClientProfileTaskListElem* task = NULL;
    CapClientSreamCapability config = isSink? cap->activeCig->sinkConfig:
                                              cap->activeCig->srcConfig;
    uint8 channelCount = capClientGetChannelCountForContext(cap, cap->activeCig->context, TRUE);
    uint8 isJointStereoInSink = FALSE;

    dataPath = (BapSetupDataPath*)CsrPmemZalloc(sizeof(BapSetupDataPath));

    dataPath->dataPathDirection =
        isSink ? DATAPATH_DIRECTION_INPUT : DATAPATH_DIRECTION_OUTPUT;

#ifdef CSR_TARGET_PRODUCT_VM
    dataPath->dataPathId = DATAPATH_ID_RAW_STREAM_ENDPOINTS_ONLY;
#else
    dataPath->dataPathId = DATAPATH_ID;
#endif 

    dataPath->controllerDelay = 0;

    dataPath->codecId.codecId = capClientGetCodecIdFromCapability(config);
    dataPath->codecId.companyId = capClientGetCompanyIdFromCapability(config);
    dataPath->codecId.vendorCodecId = capClientGetVendorCodecIdFromCapability(config);

    dataPath->isoHandle = isoHandle;
    dataPath->codecId.codecId = BAP_CODEC_ID_VENDOR_DEFINED;
    /*
     *  The Counter 'datapathReqCount' keeps track of number of datapath
     *  requests sent per device. Every bap instance has its own 'datapathReqCount'
     *  counter.
     *
     *  Increment this counter when sending the datapath Request.
     */

    bap->datapathReqCount++;

    /* CAP uses SNK channel count for identifying aptX Lite config values to be used*/
    if (channelCount > 1)
        isJointStereoInSink = TRUE;

    capClientBuildCodecConfigQuery(cap, config, &dataPath->codecConfiguration, audioLocation, isSink, isJointStereoInSink);

    task = (CapClientProfileTaskListElem*)
                CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, inst->profileTask);

    if ((dataPath->codecId.codecId == BAP_CODEC_ID_VENDOR_DEFINED) &&
        (task &&
         task->vendorConfigDataLen != 0 &&
         task->vendorConfigData))
    {
        dataPath->vendorDataLen = task->vendorConfigDataLen;
        dataPath->vendorData = CsrPmemZalloc(task->vendorConfigDataLen);

        SynMemCpyS(dataPath->vendorData,
                (uint16)dataPath->vendorDataLen,
                task->vendorConfigData,
                (uint16)dataPath->vendorDataLen);
    }
    else
    {
        dataPath->vendorDataLen = 0;
        dataPath->vendorData = NULL;
    }

    BapClientSetupDataPathReq(bapHandle, dataPath);

    CsrPmemFree(dataPath);
    dataPath = NULL;
}

static void capClientSetupDataPathForAllAses(BapInstElement* bap,
                                             CAP_INST* inst,
                                             CapClientGroupInstance* cap)
{
    BapAseElement* sinkAse = NULL, * srcAse = NULL;

    if (bap == NULL || cap == NULL)
    {
        CAP_CLIENT_ERROR("\n capClientSetupDataPathForAllAses: NULL instance \n");
        return;
    }

    sinkAse = (BapAseElement*)((CsrCmnListElm_t*)(bap->sinkAseList.first));

    while (sinkAse)
    {
        if ((sinkAse->state == BAP_ASE_STATE_QOS_CONFIGURED) 
            && sinkAse->inUse && sinkAse->cisHandle != 0
              && sinkAse->useCase == cap->activeCig->context)
        {
            capClientSetupDataPathReq(bap, inst, cap,
                                      sinkAse->cisHandle, bap->bapHandle, sinkAse->audioLocation, TRUE);
                                      sinkAse->datapathDirection |= CAP_CLIENT_DATAPATH_INPUT;

            /* Check if the same CIS handle exist for Source Ase as well
             * If present, update the Cis handle direction to bidirectional
             */
            srcAse = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_CISHANDLE(bap->sourceAseList, sinkAse->cisHandle);

            if (srcAse)
                srcAse->datapathDirection |= CAP_CLIENT_DATAPATH_INPUT;
        }
        sinkAse = sinkAse->next;
    }

    /* Now setup datapath for Output */

    srcAse = (BapAseElement*)((CsrCmnListElm_t*)(bap->sourceAseList.first));

    while (srcAse)
    {
        if ((srcAse->state == BAP_ASE_STATE_QOS_CONFIGURED) 
            && srcAse->inUse && srcAse->cisHandle != 0
               && srcAse->useCase == cap->activeCig->context)
        {
            capClientSetupDataPathReq(bap, inst, cap,
                srcAse->cisHandle, bap->bapHandle, srcAse->audioLocation, FALSE);
            srcAse->datapathDirection |= CAP_CLIENT_DATAPATH_OUTPUT;

            /*
             * Check if the same CIS handle exist for Source Ase as well
             * If present, update the Cis handle direction to bidirectional
             */

            sinkAse = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_CISHANDLE(bap->sinkAseList, srcAse->cisHandle);

            if (sinkAse)
                sinkAse->datapathDirection |= CAP_CLIENT_DATAPATH_OUTPUT;
        }
        srcAse = srcAse->next;
    }

    inst->bapRequestCount++;
}

static void capConfigureDummyAseDataPath(CAP_INST* inst, CapClientGroupInstance* cap, BapInstElement* bap, uint8 aseType)
{
    BapAseElement* ase = NULL;
    uint8 iter = 0;
    bool isSink = TRUE;

    /* It was assumed that non existence link will be having same settings which connected link will be having 
     * so get the values for sink/src from connected link and setup data path for non existence link cis handles */
    BapInstElement* connectedBap = (BapInstElement*)cap->bapList.first;

    if (aseType == BAP_ASE_SINK)
    {
        ase = (BapAseElement*)((CsrCmnListElm_t*)(connectedBap->sinkAseList.first));
    }
    else if (aseType == BAP_ASE_SOURCE)
    {
        ase = (BapAseElement*)((CsrCmnListElm_t*)(connectedBap->sourceAseList.first));
        isSink = FALSE;
    }

    while (ase)
    {
        if ((ase->state == BAP_ASE_STATE_QOS_CONFIGURED)
            && ase->inUse && bap->cisHandles[iter] != 0
            && ase->useCase == cap->activeCig->context)
        {
            /* Need to take care that cishandles are not going to overflow */
            capClientSetupDataPathReq(bap, inst, cap,
                bap->cisHandles[iter], bap->bapHandle, ase->audioLocation, isSink);
            CAP_CLIENT_INFO("capConfigureDummyAseDataPath bap->cisHandles[iter] %x\n", bap->cisHandles[iter]);
            iter++;
        }
        ase = ase->next;
    }
}

static void capClientSetupDataPathForDummyBap(CAP_INST* inst,
                                             CapClientGroupInstance* cap,
                                             BapInstElement* bap)
{

    if (cap == NULL || bap == NULL)
    {
        CAP_CLIENT_ERROR("\n capClientSetupDataPathForDummyBap: NULL instance \n");
        return;
    }

    capConfigureDummyAseDataPath(inst, cap, bap, BAP_ASE_SINK);
    capConfigureDummyAseDataPath(inst, cap, bap, BAP_ASE_SOURCE);

    inst->bapRequestCount++;
}
void capClientUnicastSetUpDataPathReqSend(BapInstElement* bap,
                                         CAP_INST* inst,
                                         CapClientGroupInstance* cap)
{
    if (bap == NULL)
    {
        CAP_CLIENT_INFO("\n capUnicastClientSetUpDataPathReqSend: NULL instance \n");
        return;
    }

    /* Here if the CAP instance is not co ordinated set then there will be only
     * one BAP instance */

    while (bap && bap->recentStatus == BAP_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n capClientUnicastSetUpDataPathReqSend: bap->handle  %x \tbap->bapCurrentState: %x\n", bap->bapHandle, bap->bapCurrentState);
        /* Need to create the data path for connected and non connnected both, In case of CSIP based remote device */
        if (CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_QOS_CONFIGURED)
        {
            /* Ensure Bap datapath request Counter is 0 for new req*/
            CAP_CLIENT_RESET_DP_REQ_COUNT(bap);
            capClientSetupDataPathForAllAses(bap, inst, cap);
        }
        else if(CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_INVALID)
        {
            /* call the setup data path for the dummy BAP */
            capClientSetupDataPathForDummyBap(inst,cap,bap);
        }
        bap = bap->next;
    }
}

void capClientHandleSetupDataPathCfm(CAP_INST* inst,
                                     BapSetupDataPathCfm* cfm,
                                     CapClientGroupInstance* cap)
{
    BapInstElement* bap = NULL;
    CapClientProfileTaskListElem* task = NULL;
    uint8  codecId;
    CapClientSreamCapability config;
    CapClientCigElem* cig = CAP_CLIENT_GET_CIG_FROM_CONTEXT(cap->cigList, cap->useCase);
    bool dummyBapDataPath = FALSE;

    bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);

    /* In case of non connected device there will be dummy bap whose handle will be zero but status will be success */
    if (bap == NULL && cfm->result == CAP_CLIENT_RESULT_SUCCESS)
    {
        bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_GROUPID(cap->bapList, cap->groupId);
        dummyBapDataPath = TRUE;
        CAP_CLIENT_INFO("\n capClientHandleSetupDataPathCfm: bap->datapathReqCount %x \t bap->handle : %x\n",bap->datapathReqCount, bap->bapHandle);
    }

    /* If setting up datapath fails send failure response */
    if (bap == NULL && (cfm->result != CAP_CLIENT_RESULT_SUCCESS && cfm->handle == CAP_CLIENT_INVALID_SERVICE_HANDLE))
    {
        CAP_CLIENT_INFO("\n capClientHandleSetupDataPathCfm: Invalid BAP Handle cfm->result %x\n", cfm->result);
        capClientSendUnicastClientConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_FAILURE_BAP_ERR);
        return;
    }

    /* bap is not expected to be NULL, if its NULL, better to have panic for getting the core dump for debug */
    if (bap == NULL)
    {
        CAP_CLIENT_PANIC("\n capClientHandleSetupDataPathCfm: Invalid BAP Handle \n");
        return;
    }

    bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);

    /*
     * Since the Datapath set up request can only take once CIS and one direction at a time
     * CAP sends datapath setup requests equal to number of configured ASEs. For Example if
     * for Media two Sinks are configured, two BapDatapathSetupReq queries are sent. This is
     * Significantly different from other Bap procedures, for example to enable two ASEs, only
     * one Enable queuery is Enough.
     *
     * Here the counter 'datapathReqCount' keeps track of the number of datapathCfm recieved,
     * 'bapRequestCount' keeps track of number of bap instances.
     * 'datapathReqCount' of a particular BAP instance is decremented everytime corrseponding a
     *  cfm is recieved. The 'bapRequestCount' is decreased only when all the datapath requests
     * sent from the instance recieve cfm, inother words when 'datapathReqCount' hits zero.
     *
     */

    bap->datapathReqCount--;

    /*
     * When bapRequestCount hits zero, the build and
     * send ReceiverStartReady request
     *
     * */

    if (bap->datapathReqCount == 0)
    {
        inst->bapRequestCount--;

        /* Set the setup data for the respective cig to be true */
        if (cig)
        {
            cig->dataPath = TRUE;
            if (dummyBapDataPath == TRUE)
            {
                /* Set the data path to dummy datapath as for multiple dummy devices in the group
                 * cap need to uniquely find the correct dummy bap */
                setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_DUMMY_DATAPTH, cap->activeCig->cigId);
            }
        }

        if (inst->bapRequestCount == 0)
        {
            /* We have received all datapath cfm so we can now free VS metadata associated
             * for this usecase if datapath req for VS is in progress
             */
            if(cap->activeCig->sinkConfig != CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN)
                config = cap->activeCig->sinkConfig;
            else
                config = cap->activeCig->srcConfig;
            
            codecId = capClientGetCodecIdFromCapability(config);
            
            task = (CapClientProfileTaskListElem*)
                CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, inst->profileTask);
            
            if(codecId ==  BAP_CODEC_ID_VENDOR_DEFINED &&
                task &&
                task->vendorConfigDataLen != 0 &&
                task->vendorConfigData)
            {
            
                CsrPmemFree(task->vendorConfigData);
                task->vendorConfigData = NULL;
                task->vendorConfigDataLen = 0;
            }

            /* This check is added to move the dummy bap state to invalid again which got moved to 
              * dummy setup data path as part of data path creation before sending the confirmation to the App */
            if (cap->setSize > 1 && cap->currentDeviceCount < cap->setSize)
            {
                bap = (BapInstElement*)cap->bapList.first;

                while (bap)
                {
                    if (CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_DUMMY_DATAPTH)
                        setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_INVALID, cap->activeCig->cigId);

                    bap = bap->next;
                }
            }

            /*
             * Check if the Cap Instance is co ordinate set. If a set, Send Release lock
             * request on all the devices.
             *
             * If not a coordinated Set. Send Message to application indicating the completion
             * of  the Unicast Connect Procedure */

            CsipInstElement* csip = (CsipInstElement*)(cap->csipList.first);
            if (capClientIsGroupCoordinatedSet(cap) && (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED))
            {

                cap->pendingOp = CAP_CLIENT_BAP_UNICAST_CONNECT;
                capClientSetCsisLockState(csip, &inst->csipRequestCount, FALSE);
            }
            else
            {
                capClientSendUnicastClientConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_SUCCESS);
            }
        }
    }
}

/****************************************************************************************************/

static void capClientUpdateAseElemAudioLocation(bool isSink,
                                        BapInstElement *bap,
                                        uint8 ase,
                                        uint32 audioLocation,
                                        CapClientCigElem *cig)
{
    BapAseElement* aseElem = NULL;

    if (isSink)
    {
        aseElem = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sinkAseList, ase);
    }
    else
    {
        aseElem = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sourceAseList, ase);
    }

    aseElem->audioLocation = audioLocation;
    aseElem->cig = cig;
}

static void capClientGenerateBapCodecConfig(BapInstElement *bap,
                                      uint8 numOfAses,
                                      uint8 *ase,
                                      uint8 direction,
                                      CapClientSreamCapability capability,
                                      BapAseCodecConfiguration *config,
                                      uint32  *audioLocations,
                                      CapClientCigElem *cig,
                                      CapClientCigConfigMode mode,
                                      uint8 channelCount,
                                      bool isJointStereoInSink)
{
    uint8 i;
    bool isSink = (direction == BAP_ASE_SINK);
    uint32 audioLocation = CAP_CLIENT_AUDIO_LOCATION_MONO;
    uint32 supportedAudioLocations = BAP_AUDIO_LOCATION_FL;
    uint8 audioLocCount = capClientNumOfBitSet(*audioLocations);

    if(ase == NULL || config == NULL)
    {
        CAP_CLIENT_ERROR("capGenerateBapCodecConfig: NULL pointer \n");
        return;
    }

    for(i = 0; i < numOfAses; i++)
    {
        /* 
         *
         *  Check if the recieved audiolocation is zero or invalid. 
         *  If Invalid use supported audio locations already discovered or else
         *  use requested locations
         * 
         */

        if (*audioLocations != CAP_CLIENT_AUDIO_LOCATION_MONO)
        {
            supportedAudioLocations = capClientGetRemoteAudioLocationsForCid(bap->bapHandle, isSink);

            CAP_CLIENT_INFO("capGenerateBapCodecConfig: audioLocations %x supportedAudioLocations %x\n", *audioLocations, supportedAudioLocations);
            /* If none of the set Audiolocations match set bits then get audioLocation from supported Audiolocations */

            if ((*audioLocations & supportedAudioLocations) == CAP_CLIENT_AUDIO_LOCATION_MONO)
            {
                *audioLocations = supportedAudioLocations;
            }

            supportedAudioLocations = (*audioLocations & supportedAudioLocations);

            audioLocation = capClientGetAudioLocationFromBitMask(&supportedAudioLocations, mode, channelCount);

            *audioLocations &= ~audioLocation;
        }

        config[i].serverDirection = direction;
        config[i].targetPhy = BAP_LE_2M_PHY;
        config[i].targetLatency = cig->latency;

        config[i].codecId.codecId = capClientGetCodecIdFromCapability(capability);
        config[i].codecId.vendorCodecId = capClientGetVendorCodecIdFromCapability(capability);
        config[i].codecId.companyId = capClientGetCompanyIdFromCapability(capability);
        config[i].codecConfiguration.frameDuaration =
            capClientGetFrameDurationFromCapability(capability);
        config[i].codecConfiguration.lc3BlocksPerSdu = BAP_DEFAULT_LC3_BLOCKS_PER_SDU;
        config[i].codecConfiguration.octetsPerFrame =
                                capClientGetSduSizeFromCapability(capability, mode, audioLocCount, isJointStereoInSink);

        /* OverWrite the Parameter values with values given by application */
        if((cig->unicastParam.codecBlocksPerSdu)
             && (cig->unicastParam.sduSizeCtoP || cig->unicastParam.sduSizePtoC))
        {
            config[i].codecConfiguration.lc3BlocksPerSdu = cig->unicastParam.codecBlocksPerSdu;
            config[i].codecConfiguration.octetsPerFrame = 
                       isSink ? cig->unicastParam.sduSizeCtoP : cig->unicastParam.sduSizePtoC;
        }


        config[i].aseId = ase[i];
        config[i].codecConfiguration.audioChannelAllocation = audioLocation;


#ifdef CAP_CLIENT_IOP_TEST_MODE
        if ((mode & CAP_CLIENT_IOP_TEST_CONFIG_MODE) == CAP_CLIENT_IOP_TEST_CONFIG_MODE)
        {
            config[i].codecConfiguration.octetsPerFrame =
                capClientGetSDUFromCapabilityForIOP(capability, config[i].codecConfiguration.octetsPerFrame, cig->context);
        }
#endif

        config[i].codecConfiguration.samplingFrequency =
                                capClientGetSamplingFreqFromCapability(capability);

        /* Update Audio location associated with ASE ID */
        capClientUpdateAseElemAudioLocation(isSink, bap, ase[i], audioLocation, cig);
    }
}

static uint8 capClientUpdateCisHandleInAseElem(BapAseElement *ase, 
                                               uint8 cisCount,
                                               uint16 *cisHandles,
                                               uint8 aseCount,
                                               CapClientContext useCase)
{
    uint8 i;

    for(i = 0; cisCount && i < cisCount && ase; ase = ase->next)
    {
        if(ase->state == BAP_ASE_STATE_CODEC_CONFIGURED &&
            ase->inUse && ase->useCase == useCase && aseCount)
        {
            ase->cisHandle = cisHandles[i];
            i++;  
            aseCount--;
        }
    }

    return i;
}

void capClientHandleUnicastQosConfigureInd(CAP_INST *inst,
                                     BapUnicastClientQosConfigureInd* ind,
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
        CAP_CLIENT_INFO("\n capHandleUnicastQosConfigureInd: Failed to configure aseId: %d", ind->aseId);
    }

    bap->recentStatus = capClientGetCapClientResult(ind->result, CAP_CLIENT_BAP);

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

    ase->state = ind->aseState;
}



void capClientHandleUnicastCodecConfigureInd(CAP_INST *inst,
                                     BapUnicastClientCodecConfigureInd* ind,
                                     CapClientGroupInstance *cap)
{
    CSR_UNUSED(inst);
    BapAseElement *ase = NULL;
    BapInstElement *bap = (BapInstElement*)
                              CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, ind->handle);
    bool isSink = TRUE;

    /* if use case is Voice search in both sink and source list
     * else search only in sink list
     * */

    if(ind->result != BAP_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n capHandleUnicastCodecConfigureInd: Failed to configure aseId: %d", ind->aseId);
    }

    bap->recentStatus = capClientGetCapClientResult(ind->result, CAP_CLIENT_BAP);

    ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sinkAseList, ind->aseId);

    if(ase == NULL)
    {
        ase = (BapAseElement*)CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(bap->sourceAseList, ind->aseId);
        isSink = FALSE;
    }

    if(ase == NULL)
    {
        CAP_CLIENT_ERROR("\n ASE is null \n");
        return;
    }

    if (cap->pendingOp == CAP_CLIENT_BAP_UNICAST_CONNECT)
        bap->asesInUse++;

    ase->useCase = cap->useCase;
    ase->state = ind->aseState;
    ase->inUse = TRUE;
    /* Update Presentation Delay */

    if (ase->cig == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP) CIG unavailable \n");
        return;
    }

    if (isSink)
    {
        if (ind->presentationDelayMin > ase->cig->sinkMinPdelay)
            ase->cig->sinkMinPdelay = ind->presentationDelayMin;

        if (ind->presentationDelayMax < ase->cig->sinkMaxPdelay)
            ase->cig->sinkMaxPdelay = ind->presentationDelayMax;
    }
    else
    {
        if (ind->presentationDelayMin > ase->cig->srcMinPdelay)
            ase->cig->srcMinPdelay = ind->presentationDelayMin;

        if (ind->presentationDelayMax < ase->cig->srcMaxPdelay)
            ase->cig->srcMaxPdelay = ind->presentationDelayMax;
    }
    bap->rtn  = ind->rtn;
    bap->transportLatency = ind->transportLatency;
}

static void capClientPopulateCisConfig(CapClientGroupInstance *cap,
    BapUnicastClientCisConfig *cis,
    uint8 cisId,
    uint8 rtn,
    uint8 *sinkCount,
    uint8 *srcCount,
    bool isJointStereoInSink)
{
    CapClientCigElem* cig = cap->activeCig;
    CapClientSreamCapability sinkConfig = cig->sinkConfig;
    CapClientSreamCapability srcConfig = cig->srcConfig;
    CapClientTargetLatency latency = cig->latency;
    CapClientCigConfigMode cigConfigMode = cap->cigConfigMode;
    CapClientQhsConfig cigQhsConfig = cap->cigConfig;

    CapClientCisDirection cisDirection = capClientGetCisDirectionFromConfig(sinkConfig, srcConfig);

    uint8 rtnMtoS;
    uint8 rtnStoM;

    cis->maxSduStoM = 0;
    cis->rtnStoM = 0;

    cis->maxSduMtoS = 0;
    cis->rtnMtoS = 0;

    cis->cisId = cisId;

    /* Priority: App supplied > QHS > Default(SIG) */
    if (CAP_CLIENT_QHS_CONFIGURED(cigConfigMode))
    {
        /* assign QHS supplied params */
        cis->phyMtoS = cigQhsConfig.phyCtoP;
        cis->phyStoM = cigQhsConfig.phyPtoC;
    }
    else
    {
        cis->phyMtoS = BAP_LE_2M_PHY;
        cis->phyStoM = BAP_LE_2M_PHY;
    }

    if (cig->unicastParam.phyCtoP != 0)
    {
        cis->phyMtoS = cig->unicastParam.phyCtoP;
    }

    if (cig->unicastParam.phyPtoC != 0)
    {
        cis->phyStoM = cig->unicastParam.phyPtoC;
    }

    if ((CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cisDirection)
        || CAP_CLIENT_CIS_IS_UNI_SINK(cisDirection))
             && (*sinkCount))
    {
        cis->maxSduMtoS = capClientGetSduSizeFromCapability(sinkConfig, cigConfigMode, cig->sinkLocCount, isJointStereoInSink);

        /*Overwrite sdu value with app supplied values*/
        cis->maxSduMtoS = (cig->unicastParam.sduSizeCtoP == 0) ? cis->maxSduMtoS : cig->unicastParam.sduSizeCtoP;

#ifdef CAP_CLIENT_IOP_TEST_MODE
        if ((cigConfigMode & CAP_CLIENT_IOP_TEST_CONFIG_MODE) == CAP_CLIENT_IOP_TEST_CONFIG_MODE)
        {
            cis->maxSduMtoS = capClientGetSDUFromCapabilityForIOP(sinkConfig, cis->maxSduMtoS, useCase);
        }
#endif

        if (CAP_CLIENT_QHS_CONFIGURED(cigConfigMode))
        {
            if (!isJointStereoInSink  &&
               ((sinkConfig & CAP_CLIENT_CODEC_ID_MASK) == CAP_CLIENT_STREAM_CAP_VS_APTX_LITE))
            {
                rtnMtoS = CAP_CLIENT_APTX_LITE_RTN_GAME_C_TO_P_DEFAULT;
            }
            else if (!isJointStereoInSink  &&
                    ((sinkConfig & CAP_CLIENT_CODEC_ID_MASK) == CAP_CLIENT_STREAM_CAP_VS_APTX_ADAPTIVE))
            {
                rtnMtoS = CAP_CLIENT_APTX_ADAPTIVE_RTN_MEDIA_C_TO_P_DEFAULT;
            }
            else
            {
                rtnMtoS = cigQhsConfig.rtnCtoP;
            }
        }
        else
        {
            /* Default Specification value */
            rtnMtoS = capClientGetRtnFromCapability(sinkConfig, latency);
        }

        /*Assign App supplied values fo rtn*/
        cis->rtnMtoS = (cig->unicastParam.rtnCtoP == 0) ? rtnMtoS : cig->unicastParam.rtnCtoP;

        /* We should prefer rtn whichever is less (remote supplied or local derived) */
        cis->rtnMtoS = (cis->rtnMtoS > rtn) ? rtn : cis->rtnMtoS ;

        (*sinkCount)--;
    }


    if ((CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cisDirection)
        || CAP_CLIENT_CIS_IS_UNI_SRC(cisDirection))
            && (*srcCount))
    {
        cis->maxSduStoM = capClientGetSduSizeFromCapability(srcConfig, cigConfigMode, cig->srcLocCount, isJointStereoInSink);

        /*Overwrite sdu value with app supplied values*/
        cis->maxSduStoM = (cig->unicastParam.sduSizePtoC == 0) ? cis->maxSduStoM : cig->unicastParam.sduSizePtoC;

#ifdef CAP_CLIENT_IOP_TEST_MODE
        if ((cigConfigMode & CAP_CLIENT_IOP_TEST_CONFIG_MODE) == CAP_CLIENT_IOP_TEST_CONFIG_MODE)
        {
            cis->maxSduStoM = capClientGetSDUFromCapabilityForIOP(sinkConfig, cis->maxSduStoM, useCase);
        }
#endif

        if (CAP_CLIENT_QHS_CONFIGURED(cigConfigMode))
        {
            if (!isJointStereoInSink  &&
               ((srcConfig & CAP_CLIENT_CODEC_ID_MASK) == CAP_CLIENT_STREAM_CAP_VS_APTX_LITE))
            {
                rtnStoM = CAP_CLIENT_APTX_LITE_RTN_GAME_P_TO_C_DEFAULT;
            }
            else if(!isJointStereoInSink  &&
                   ((srcConfig & CAP_CLIENT_CODEC_ID_MASK) == CAP_CLIENT_STREAM_CAP_VS_APTX_ADAPTIVE))
            {
                rtnStoM = CAP_CLIENT_APTX_ADAPTIVE_RTN_MEDIA_P_TO_C_DEFAULT;
            }
            else
            {
                rtnStoM = cigQhsConfig.rtnPtoC;
            }
        }
        else
        {
            /* Default Specification value */
            rtnStoM =  capClientGetRtnFromCapability(srcConfig, latency);
        }

        /*Assign App supplied values for rtn*/
        cis->rtnStoM = (cig->unicastParam.rtnPtoC == 0) ? rtnStoM : cig->unicastParam.rtnPtoC;

        /* We should prefer rtn whichever is less (remote supplied or local derived) */
        cis->rtnStoM = (cis->rtnStoM > rtn) ? rtn : cis->rtnStoM ;

        (*srcCount)--;
    }
}

static void capClientPopulateQosConfigForAse(BapAseQosConfig *aseQos,
                                     CapClientSreamCapability config,
                                     uint16 pDelayMax,
                                     BapInstElement *bap,
                                     CapClientTargetLatency highReliability,
                                     CapClientGroupInstance* cap,
                                     CapClientContext useCase,
                                     uint8 locCount,
                                     bool isSink,
                                     bool isJointStereoInSink)
{
    if((aseQos == NULL) || (cap->activeCig == NULL))
    {
        CAP_CLIENT_ERROR("\n capPopulateQosConfigForBap: aseQos is null\n");
        return;
    }

    uint32 sduInterval = capClientGetSduIntervalForCapability(config, isJointStereoInSink);
    uint16 sduSize = capClientGetSduSizeFromCapability(config, cap->cigConfigMode, locCount, isJointStereoInSink);
    uint8 retNum = capClientGetRtnFromCapability(config, highReliability);
    uint16 mtl = capClientGetMaxLatencyFromCapability(config, highReliability, isJointStereoInSink);


    aseQos->framing = capClientGetFramingForCapability(config);
    aseQos->phy = BAP_LE_2M_PHY;
    aseQos->presentationDelay = pDelayMax;
    aseQos->rtn = (retNum > bap->rtn) ? bap->rtn : retNum;
    aseQos->sduInterval = sduInterval;
    aseQos->sduSize = sduSize;
    aseQos->transportLatency = (mtl > bap->transportLatency) ? bap->transportLatency : mtl;
    CSR_UNUSED(useCase);

    /*Overide spec define value with app supplied values*/

    retNum = isSink ? cap->activeCig->unicastParam.rtnCtoP : cap->activeCig->unicastParam.rtnPtoC;
    aseQos->rtn = (retNum == 0) ? aseQos->rtn : retNum;

    sduSize = isSink ? cap->activeCig->unicastParam.sduSizeCtoP : cap->activeCig->unicastParam.sduSizePtoC;
    aseQos->sduSize = (sduSize == 0) ? aseQos->sduSize : sduSize;

    mtl = isSink ? cap->activeCig->unicastParam.maxLatencyCtoP : cap->activeCig->unicastParam.maxLatencyPtoC;
    aseQos->transportLatency = (mtl == 0) ? aseQos->transportLatency : mtl;

    if(isSink)
    {
        aseQos->phy = (cap->activeCig->unicastParam.phyCtoP == 0) ? aseQos->phy : cap->activeCig->unicastParam.phyCtoP;
    }
    else
    {
        aseQos->phy = (cap->activeCig->unicastParam.phyPtoC == 0) ? aseQos->phy : cap->activeCig->unicastParam.phyPtoC;
    }

    if(isSink)
    {
        aseQos->sduInterval = (cap->activeCig->unicastParam.sduIntervalCtoP == 0) ? aseQos->sduInterval : 
                                                                  cap->activeCig->unicastParam.sduIntervalCtoP;
    }
    else
    {
        aseQos->sduInterval = (cap->activeCig->unicastParam.sduIntervalPtoC == 0) ? aseQos->sduInterval : 
                                                                  cap->activeCig->unicastParam.sduIntervalPtoC;
    }

#ifdef CAP_CLIENT_IOP_TEST_MODE
    if ((cap->cigConfigMode & CAP_CLIENT_IOP_TEST_CONFIG_MODE) == CAP_CLIENT_IOP_TEST_CONFIG_MODE)
    {
        aseQos->sduSize = capClientGetSDUFromCapabilityForIOP(config, aseQos->sduSize, useCase);
        aseQos->transportLatency = capClientGetMTLFromCapabilityForIOP(config, aseQos->transportLatency, useCase);
    }
#endif
}

static void capClientPopulateQosConfigForBap(BapAseQosConfiguration *qos,
                                   BapAseElement *ase,
                                   BapInstElement *bap,
                                   CapClientSreamCapability config,
                                   CapClientTargetLatency highReliability,
                                   uint16 pDelayMax,
                                   CapClientGroupInstance* cap,
                                   CapClientContext useCase,
                                   uint8 locCount,
                                   bool isSink,
                                   bool isJointStereoInSink)
{
    if(qos == NULL)
    {
        CAP_CLIENT_ERROR("\n capPopulateQosConfigForBap: qos is null\n");
        return;
    }
    qos->aseId = ase->aseId;
    qos->cigId = bap->cigId;
    qos->cisId = ase->cisId;
    qos->cisHandle = ase->cisHandle;
    capClientPopulateQosConfigForAse(&qos->qosConfiguration, 
                                    config, pDelayMax, bap, 
                                    highReliability, cap, useCase, 
                                    locCount, isSink, isJointStereoInSink);
}

static void capClientPopulateQosConfigParamForAllAses(BapAseElement *ase,
                                               CapClientCigElem* cig,
                                                BapAseState state,
                                                uint8 aseCount,
                                                uint8 numOfAses,
                                                BapAseQosConfiguration *config,
                                                uint8 *qosCount,
                                                BapInstElement *bap,
                                                CapClientGroupInstance* cap,
                                                uint8 locCount,
                                                uint16 pDelayMax,
                                                CapClientSreamCapability setting,
                                                bool isSink,
                                                bool isJointStereoInSink)
{
    uint8 i;

    if(ase == NULL || cig == NULL)
    {
        CAP_CLIENT_ERROR("\n capSendBapUnicaseQosConfigReq: NULL instance \n");
        return;
    }

    for( i = 0; i < aseCount && ase ;ase = ase->next)
    {
       if(ase->state == state && 
           (ase->useCase == cig->context) && (*qosCount + i) < numOfAses)
       {
           capClientPopulateQosConfigForBap(&config[i],
                                  ase, bap, setting, cig->latency,
                                  pDelayMax, cap, cig->context, locCount, isSink, isJointStereoInSink);
           i++;
       }
    }

    (*qosCount) += i;

}

static void capClientSendBapUnicastQosConfigReq(BapInstElement *bap,
                                               CapClientGroupInstance* cap,
                                               CAP_INST *inst,
                                               CapClientCigElem *cig,
                                               uint8 numOfAses)
{
    CSR_UNUSED(cap);
    CSR_UNUSED(inst);
    uint8 qosCount = 0;
    BapAseQosConfiguration *config = NULL;
    BapAseElement *ase = NULL;
    CapClientCisDirection cisDirection = 
                  capClientGetCisDirectionFromConfig(cig->sinkConfig, cig->srcConfig);
    uint8 channelCount = capClientGetChannelCountForContext(cap, cig->context, TRUE);
    bool isJointStereoInSink = FALSE;
    /*
     * For Conversational use case numOfAses will include both source and
     * Sink ASEs. Hence we need to build query for Both
     *
     * For MEDIA usecase numOfAses will be 2(stereo) in case of only one device in
     * CAP, will be only one in case CAP has more than 1 device in set
     *
     * For CONVERSATIONAL usecase numOfAses will 4 in case of one device set and 2 otherwise
     * i.e one device acts as both source and sink
     *
     * */

    if(bap == NULL)
    {
        CAP_CLIENT_ERROR("\n capSendBapUnicastQosConfigReq: NULL instance \n");
        return;
    }

    if(bap->recentStatus != CAP_CLIENT_RESULT_SUCCESS)
    {
        CAP_CLIENT_ERROR("\n capSendBapUnicastQosConfigReq: There was a recent failure here \n");
        return;
    }

    /* CAP uses SNK channel count for identifying aptX Lite config values to be used*/
    if (channelCount > 1)
        isJointStereoInSink = TRUE;
    config = (BapAseQosConfiguration*)
                        CsrPmemZalloc(numOfAses*sizeof(BapAseQosConfiguration));

    if ((qosCount < numOfAses) &&
        (CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cisDirection)
            || CAP_CLIENT_CIS_IS_UNI_SINK(cisDirection)))
    {
        bool isSink = TRUE;
        ase = (BapAseElement*)(bap->sinkAseList.first);
        capClientPopulateQosConfigParamForAllAses(ase, cig,BAP_ASE_STATE_CODEC_CONFIGURED,
            bap->sinkAseCount, numOfAses, &config[0], &qosCount, bap,
               cap, cig->sinkLocCount, cig->sinkMinPdelay, cig->sinkConfig, isSink, isJointStereoInSink);
    }

    if ((qosCount < numOfAses) &&
        (CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cisDirection)
            || CAP_CLIENT_CIS_IS_UNI_SRC(cisDirection)))
    {
        bool isSink = FALSE;
        ase = (BapAseElement*)(bap->sourceAseList.first);
        capClientPopulateQosConfigParamForAllAses(ase, cig,BAP_ASE_STATE_CODEC_CONFIGURED,
            bap->sourceAseCount, numOfAses, &config[qosCount], &qosCount, bap, 
               cap, cig->srcLocCount, cig->srcMinPdelay, cig->srcConfig, isSink, isJointStereoInSink);
    }

    BapUnicastClientQosConfigReq(bap->bapHandle, numOfAses, config);

#ifdef CAP_CLIENT_NTF_TIMER
    /* Trigger the CAP timer to get all the NTF for QOS config within spec defined time */
    capClientNtfTimerSet(inst, cap, bap, BAP_ASE_STATE_QOS_CONFIGURED);
#endif

    /*  Free the memory and fill with NULL so as to ensure it doesnt get
     * freed in some other place */
    CsrPmemFree(config);
    config = NULL;
}

static void capClientSendBapUnicastQosConfigReqSend(BapInstElement *bap ,
                                             CapClientGroupInstance *cap,
                                             CAP_INST *inst,
                                             uint8 *count)
{
    if(cap == NULL || bap == NULL)
    {
        CAP_CLIENT_ERROR("\n capSendBapUnicastQosConfigReqSend: NULL instance \n");
        return;
    }

    while(bap)
    {
        /* We might have configured lesser number of ASEs.
         * Check the ases in use if less reduce numOfAses to
         * configured value */
        /* Need to do QOS confiugre only for disocvered devices in the group */
        if (CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_CODEC_CONFIGURED)
        {
            uint8 configuredAses= (bap->sinkAsesToBeConfigured + bap->srcAsesToBeConfigured);
            capClientSendBapUnicastQosConfigReq(bap, cap, inst, cap->activeCig, configuredAses);
            (*count)++;
        }
        bap = bap->next;
    }
}

static bool capClientUpdateCisIdForAse(BapAseElement *ase,
                                 uint8 cisId,
                                 BapAseState state,
                                 uint8* aseInUse,
                                 CapClientGroupInstance *cap)
{
    if (ase && ase->state == state && 
          ase->inUse && cisId != 0
             && ase->useCase == cap->useCase)
    {
        /* If the CIS ID is available already, then reuse same cis and 
            reset the prevCisId counter used for cisId generation */
        
        ase->cisId = cisId;
        (*aseInUse)--;

        return TRUE;
    }

    return FALSE;
}

/*
 *                       capClientGenerateValidCisId
 * 
 * This function generates unique CIS ID  range for each Context useCase.
 * CIS id is derive by using basic modulus hashing using following equation
 *  
 * 
 *     hashNumber = (CapContext << 3) % 152
 * 
 * Here numbers are chosen such that there is atleast spacing of 5 for
 * CIS id's of each unique CapContext. The script used to arrive at these
 * values is attached in 
 * https://confluence.qualcomm.com/confluence/display/BTSynergy/CAP+redesign+to+allow+multiple+context+configuration
 */

static uint8 capClientGenerateValidCisId(CapClientGroupInstance* cap)
{
    uint8 cisId = 0;

    if (cap->useCase == CAP_CLIENT_CONTEXT_TYPE_PROHIBITED)
        return CAP_CLIENT_INVALID_CIS_ID;

    cisId = (uint8)(((uint32)((cap->useCase << CAP_CLIENT_GENERATE_CIS_LSHIFT)) % CAP_CLIENT_GENERATE_CIS_PRIM_NUM));
    return cisId;
}

/*
 * This Function evalautes number of sink ase and source ase required to be vconfigured for the use case as:
 * Case 1: When Audio location is Mono, for sink always 1 ase is required and in case of source if mic is non zero then 1 Ase
 *         and if mic is zero then no ase
 * Case 2: When Audio location is other than mono then based on the location bit set for sink and for src min of App input and location bit set.
 */

static uint8 evaluateSinkSrcAse(CapClientGroupInstance *cap, uint32 location, uint8 channelCount, uint8 sinkSrc, uint8 noOfMic)
{
    uint8 aseCountLocal = 0;

    CAP_CLIENT_INFO("\n capClientEvaluateRequiredAses:location %x noOfMic %x cap->numOfSourceAses %x\n", location, noOfMic, cap->numOfSourceAses);

    aseCountLocal = capClientGetSetBitCount(location);

    if (sinkSrc == BAP_ASE_SINK)
    {
        /* aseCountLocal is 0 means location is MONO and for MONO always choose 1 Sink ASE */
        if (aseCountLocal == CAP_CLIENT_AUDIO_LOCATION_MONO)
        {
            aseCountLocal = 1;
        }
    }
    else
    {
        /* In case of Src ASE, we compare number of Source Location bits and numberOfAses(sent from application)
         * Mics configured will be minimum of the two parameters in case of input mic is non zero */
        if (noOfMic == 0x00)
        {
            aseCountLocal = 0;
        }
        else
        {
            /* aseCountLocal is 0 means location is MONO and for MONO always choose 1 Src ASE */
            if (aseCountLocal == CAP_CLIENT_AUDIO_LOCATION_MONO)
            {
                aseCountLocal = 1;
            }
            else
            {
                aseCountLocal = CAP_CLIENT_GET_MIN(aseCountLocal, cap->numOfSourceAses);
            }
            cap->numOfSourceAses = aseCountLocal;
        }
    }

    if (((cap->cigConfigMode & CAP_CLIENT_MODE_JOINT_STEREO) == CAP_CLIENT_MODE_JOINT_STEREO)
                     && (channelCount > 1) && (aseCountLocal > 1))
    {
        aseCountLocal = aseCountLocal >> 1;

        /* Update the cap group overall mic in case of src ASE */
        if (sinkSrc == BAP_ASE_SOURCE)
        {
            cap->numOfSourceAses = aseCountLocal;
        }
    }
    return aseCountLocal;
}

static void capClientEvaluateRequiredAses(CapClientGroupInstance *cap,
                                         uint8 *sinkAseCount,
                                         uint8 *srcAseCount,
                                         CapClientCigElem *cig,
                                         uint8 noOfMic)
{
    /* TODO: Also use supported audiolocation discovered in the calculations of number of ASEs 
     * to be configured */
    uint8 channelCount;

    channelCount = capClientGetChannelCountForContext(cap, cap->useCase, TRUE);
    *sinkAseCount = evaluateSinkSrcAse(cap, cig->sinkLoc, channelCount, BAP_ASE_SINK, 0xFF);

    channelCount = capClientGetChannelCountForContext(cap, cap->useCase, FALSE);
    *srcAseCount = evaluateSinkSrcAse(cap, cig->sourceLoc, channelCount, BAP_ASE_SOURCE, noOfMic);

    /*
     * Context takes higher precedent than the audiolocation and Number of Mic parameters. 
     * For example in case of Media context, there is no point in configuring mics,
     * hence those parameters are ignored. 
     *
     */

    if (CAP_CLIENT_CIS_IS_UNI_SINK(cap->cigDirection))
    {
        *srcAseCount = 0;
        cap->numOfSourceAses = *srcAseCount;
    }
    else if (CAP_CLIENT_CIS_IS_UNI_SRC(cap->cigDirection))
    {
        *sinkAseCount  = 0;
    }
}

static void  capClientRequiredAsesPerBap(CapClientGroupInstance* cap,
                                         BapInstElement* bap,
                                         uint8 cigId)
{
    BapInstElement* tmp = bap;
    CapClientBool coordinatedSet = capClientIsGroupCoordinatedSet(cap);
    uint8 sinkAseCountPerDevice = cap->requiredSinks;
    uint8 srcAseCountPerDevice = cap->requiredSrcs;
    uint8 totalSinkCount = cap->requiredSinks;
    uint8 totalSrcCount = cap->requiredSrcs;

    if (coordinatedSet || cap->setSize > 1)
    {
        sinkAseCountPerDevice = CAP_CLIENT_GET_MIN(sinkAseCountPerDevice, 1);
        srcAseCountPerDevice = CAP_CLIENT_GET_MIN(srcAseCountPerDevice, 1);
    }
    else
    {
        /* If in case of Non CoordinatedSet Limit the number of ASEs getting configured to 2*/

        sinkAseCountPerDevice = CAP_CLIENT_GET_MIN(sinkAseCountPerDevice, 2);
        srcAseCountPerDevice = CAP_CLIENT_GET_MIN(srcAseCountPerDevice, 2);

    }

    while(tmp)
    {
        CAP_CLIENT_INFO("\n capClientRequiredAsesPerBap: bap->bapCurrentState : %x, cig id : %d \n", tmp->bapCurrentState, cigId);
        if (CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(tmp, cigId) == CAP_CLIENT_BAP_STATE_AVLBLE_AUDIO_CONTEXT)
        {
            sinkAseCountPerDevice = CAP_CLIENT_GET_MIN(sinkAseCountPerDevice, tmp->sinkAseCount);
            srcAseCountPerDevice = CAP_CLIENT_GET_MIN(srcAseCountPerDevice, tmp->sourceAseCount);

            tmp->sinkAsesToBeConfigured = sinkAseCountPerDevice;
            tmp->srcAsesToBeConfigured = srcAseCountPerDevice;

            /* Sink/Source ASE Counts assigned per Device needs to be reduced from
             * total Count */

            if (totalSinkCount)
                totalSinkCount -= sinkAseCountPerDevice;

            if (totalSrcCount)
                totalSrcCount -= srcAseCountPerDevice;

            /* If both the totalSinkCount and source count are Zero
             *  Break out of the Loop */

            if (totalSinkCount == 0 && totalSrcCount == 0)
                break;

            sinkAseCountPerDevice = totalSinkCount;
            srcAseCountPerDevice = totalSrcCount;
        }
        tmp = tmp->next;
    }
}

static void dummyBapCreation(CapClientGroupInstance *cap, bool* dummBap)
{
    if (cap->setSize > 1 && cap->currentDeviceCount < cap->setSize)
    {
         *dummBap = TRUE;
         BapInstElement *dummyBap = (BapInstElement *)CAP_CLIENT_GET_BAP_ELEM_FROM_GROUPID(cap->bapList, cap->groupId);

         if (dummyBap == NULL)
         {
             dummyBap = (BapInstElement*)CAP_CLIENT_ADD_BAP_INST(cap->bapList);
         }

         /* We have choosen groupId and bap state to identify the correct bap while adding in the list */
         dummyBap->groupId = cap->groupId;
         /* Set the state to invalid so that while adding the device during add new device should
          * Not create one more entry and append the bap instance to the existing entry if found */
         setBapStatePerCigId(dummyBap, CAP_CLIENT_BAP_STATE_INVALID, CAP_CLIENT_BAP_STATE_INVALID);
    }
}

BapUnicastClientCigParameters* capClientPopulateCigConfigReqQuery(CapClientGroupInstance *cap,
    BapInstElement *bap,
    CapClientCigElem* cig)
{
    /* Configure CIG's*/
    uint8 i = 0;
    uint8 aseInUse = 0;
    uint8 generatedCisId = 0;
    bool isJointStereoInSink = FALSE;
    BapUnicastClientCigParameters* param = NULL;
    uint8 sinkCount = capClientGetSinkAseCountForUseCase(bap, cap->useCase);
    uint8 srcCount = capClientGetSrcAseCountForUseCase(bap, cap->useCase);
    uint8 sinkChannelCount = capClientGetChannelCountForContext(cap, cap->useCase, TRUE);
    uint32 sduIntervalMtoS;
    uint16 maxLatencyMtoS;
    uint32 sduIntervalStoM;
    uint16 maxLatencyStoM;
    bool dummBap = FALSE;

    /* CAP uses SNK channel count for identifying aptX Lite config values to be used*/
    if (sinkChannelCount > 1)
        isJointStereoInSink = TRUE;

    maxLatencyMtoS = capClientGetMaxLatencyFromCapability(cig->sinkConfig, cig->latency, isJointStereoInSink);
    maxLatencyStoM = capClientGetMaxLatencyFromCapability(cig->srcConfig, cig->latency, isJointStereoInSink);

    sduIntervalMtoS = capClientGetSduIntervalForCapability(cig->sinkConfig, isJointStereoInSink);
    sduIntervalStoM = capClientGetSduIntervalForCapability(cig->srcConfig, isJointStereoInSink);

#ifdef CAP_CLIENT_IOP_TEST_MODE
    if ((cap->cigConfigMode & CAP_CLIENT_IOP_TEST_CONFIG_MODE) == CAP_CLIENT_IOP_TEST_CONFIG_MODE)
    {
        maxLatencyStoM = capClientGetMTLFromCapabilityForIOP(cig->sinkConfig, maxLatencyStoM, cap->useCase);
        maxLatencyMtoS = capClientGetMTLFromCapabilityForIOP(cig->srcConfig, maxLatencyMtoS, cap->useCase);
    }
#endif

    BapAseElement* sinkAse = (BapAseElement*)bap->sinkAseList.first;
    BapAseElement* sourceAse = (BapAseElement*)bap->sourceAseList.first;

    if (VALIDATE_USECASE(cap->useCase))
    {
        /*
         * If Usecase is not supported the request should not be coming here
         */
        CAP_CLIENT_ERROR("\n(CAP) capPopulateCigConfigReqQuery: Invalid UseCase \n");
        return NULL;
    }

    /* Check sduInterval and MaxLatency are non zero values. if not CIG configure fails */

    if (!(maxLatencyMtoS || maxLatencyStoM || sduIntervalStoM || sduIntervalMtoS))
    {
        CAP_CLIENT_ERROR("\n(CAP) capPopulateCigConfigReqQuery: Invalid Settings \n");
        return NULL;
    }

    if (!(maxLatencyMtoS && sduIntervalMtoS))
    {
        maxLatencyMtoS = maxLatencyStoM;
        sduIntervalMtoS = sduIntervalStoM;
    }
    else if (!(maxLatencyStoM && sduIntervalStoM))
    {
        maxLatencyStoM = maxLatencyMtoS;
        sduIntervalStoM = sduIntervalMtoS;
    }

    param = (BapUnicastClientCigParameters*)
                CsrPmemZalloc(sizeof(BapUnicastClientCigParameters));

    param->cisConfig = (BapUnicastClientCisConfig*)
                CsrPmemZalloc(cap->totalCisCount * sizeof(BapUnicastClientCisConfig));

    param->cigId = cig->cigId;

    if(cig->unicastParam.packing)
    {
        param->packing = cig->unicastParam.packing;
    }
    else if(!isJointStereoInSink &&
        ((cig->sinkConfig & CAP_CLIENT_CODEC_ID_MASK) == CAP_CLIENT_STREAM_CAP_VS_APTX_LITE) &&
         CAP_CLIENT_QHS_CONFIGURED(cap->cigConfigMode))
    {
        param->packing = CAP_CLIENT_PACKING_OVERLAPPING_TYPE1;
    }
    else
    {
        param->packing = CAP_CLIENT_PACKING_INTERLEAVED;
    }

    if(cig->unicastParam.sca)
    {
        param->sca = cig->unicastParam.sca;
    }
    else
    {
        param->sca = CAP_CLIENT_WORST_CASE_SCA;
    }

    param->cisCount = cap->totalCisCount;

    /* Overwrite framing with Apps, Priority :App supplied > QHS Supplied > Default SIG */
    if(cig->unicastParam.framing)
    {
        param->framing = cig->unicastParam.framing;
    }
    else if(CAP_CLIENT_QHS_CONFIGURED(cap->cigConfigMode))
    {
        param->framing = cap->cigConfig.framing;
    }
    else
    {
        param->framing = capClientGetFramingForCapability(cig->sinkConfig);
    }

    param->maxTransportLatencyMtoS = maxLatencyMtoS;
    param->maxTransportLatencyStoM = maxLatencyStoM;

    if(cig->unicastParam.maxLatencyCtoP && cig->unicastParam.maxLatencyPtoC)
    {
        /*Copy app supplied values*/
        param->maxTransportLatencyMtoS = cig->unicastParam.maxLatencyCtoP;
        param->maxTransportLatencyStoM = cig->unicastParam.maxLatencyPtoC;
    }
    else
    {
        param->maxTransportLatencyMtoS = (param->maxTransportLatencyMtoS > bap->transportLatency) ?
                                          bap->transportLatency : param->maxTransportLatencyMtoS;
        param->maxTransportLatencyStoM = (param->maxTransportLatencyStoM > bap->transportLatency) ?
                                          bap->transportLatency : param->maxTransportLatencyStoM;
    }

    if(cig->unicastParam.sduIntervalCtoP &&  cig->unicastParam.sduIntervalPtoC)
    {
        param->sduIntervalMtoS = cig->unicastParam.sduIntervalCtoP;
        param->sduIntervalStoM = cig->unicastParam.sduIntervalPtoC;
    }
    else
    {
        param->sduIntervalMtoS = sduIntervalMtoS;
        param->sduIntervalStoM =  sduIntervalStoM;
    }

    aseInUse = capClientGetAseCountForUseCase(bap, cap->useCase);

    /* Check and Create A dummy BAP instance for the not disovered device */
    dummyBapCreation(cap, &dummBap);

    for(i = 0; i < cap->totalCisCount && bap;)
    {
        bool srcFlag = FALSE;
        bool sinkFlag = FALSE;

        bap->cigId = param->cigId; /* Save CIG ID, One Application can have only one CIG ID*/

        generatedCisId = capClientGenerateValidCisId(cap) + i;

        if (generatedCisId == CAP_CLIENT_INVALID_CIS_ID)
        {
            CAP_CLIENT_INFO("\n(CAP) capPopulateCigConfigReqQuery: Invalid CIS ID\n");

            CsrPmemFree(param->cisConfig);
            param->cisConfig = NULL;
            CsrPmemFree(param);
            param = NULL;
            return param;
        }


        if ((CAP_CLIENT_CIS_IS_UNI_SINK(cap->cigDirection)
            || CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection)) && sinkCount)
        {
            while (sinkAse)
            {
                sinkFlag = capClientUpdateCisIdForAse(sinkAse, generatedCisId,
                                    BAP_ASE_STATE_CODEC_CONFIGURED, &aseInUse, cap);

                sinkAse = sinkAse->next;

                if (sinkFlag)
                    break;
            }
        }

        if ((CAP_CLIENT_CIS_IS_UNI_SRC(cap->cigDirection)
             || CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection)) && srcCount)
        {
            while (sourceAse && cap->numOfSourceAses)
            {
                srcFlag = capClientUpdateCisIdForAse(sourceAse, generatedCisId,
                                    BAP_ASE_STATE_CODEC_CONFIGURED, &aseInUse, cap);

                sourceAse = sourceAse->next;

                if (srcFlag)
                    break;
            }
        }

        capClientPopulateCisConfig(cap, &param->cisConfig[i], generatedCisId,
                                   bap->rtn, &sinkCount , &srcCount, isJointStereoInSink);

        /* Get next BAP instance in which ASEs are in use */

        if (aseInUse == 0 && cap->totalCisCount)
        {
            bap = bap->next;

            while (bap)
            {
                aseInUse = capClientGetAseCountForUseCase(bap, cap->useCase);
                sinkCount = capClientGetSinkAseCountForUseCase(bap, cap->useCase);
                srcCount = capClientGetSrcAseCountForUseCase(bap, cap->useCase);

                if (aseInUse > 0)
                {
                    sinkAse = (BapAseElement*)bap->sinkAseList.first;
                    sourceAse = (BapAseElement*)bap->sourceAseList.first;
                    break;
                }

                
                if (dummBap || (bap->sinkAseCount == 0 && bap->sourceAseCount == 0))
                {
                    BapInstElement* bap1 = (BapInstElement*)cap->bapList.first;
                    sinkCount = capClientGetSinkAseCountForUseCase(bap1, cap->useCase);
                    srcCount = capClientGetSrcAseCountForUseCase(bap1, cap->useCase);
                    break;
                }
                bap = bap->next;
            }
        }

        if (sinkFlag || srcFlag)
            i++;
    }

    return param;
}

void capClientSendBapCigConfigureReq(CAP_INST *inst,
    CapClientGroupInstance *cap,
    BapInstElement *bap)
{

    BapUnicastClientCigParameters *param = NULL;

    if(bap == NULL)
    {
        CAP_CLIENT_INFO("\n capSendBapCigConfigureReq: NULL instance BAP\n");
        capClientSendUnicastClientConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_NULL_INSTANCE);
        return;
    }

    /* if there was a recent failure the inform the application*/

    if (bap->recentStatus)
    {
        CAP_CLIENT_INFO("\n capSendBapCigConfigureReq: There was a recent failure in BAP\n");
        capClientSendUnicastClientConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_NOT_SUPPORTED);
        return;
    }

    /* Creating CIG is a CAP level Change*/
    param = capClientPopulateCigConfigReqQuery(cap, bap, cap->activeCig);

    if (param == NULL)
    {
        CAP_CLIENT_INFO("\n capSendBapCigConfigureReq: Insufficient Resources\n");
        capClientSendUnicastClientConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_INSUFFICIENT_RESOURCES);
        return;
    }

    inst->bapRequestCount++;
    BapUnicastClientCigConfigReq(cap->libTask, param);

    /* Free the allocated memory */
    CsrPmemFree(param->cisConfig);
    param->cisConfig = NULL;

    CsrPmemFree(param);
    param = NULL;
}

BapAseCodecConfiguration* capClientBuildCodecConfigQueryReq(uint8* sourceAseCount,
    uint8* sinkAseCount,
    BapInstElement *bap,
    CapClientGroupInstance* cap,
    CapClientCigElem *cig)
{
    uint8 localSinkAseCount = 0, localSrcAseCount = 0;

    /* retrieve Discovered ASE's */
    uint8* srcAse = NULL;
    uint8* sinkAse = NULL;
    uint8 totalAseCount = 0;
    uint8 state = BAP_ASE_STATE_QOS_CONFIGURED;
    uint8 sinkChannelCount = capClientGetChannelCountForContext(cap, cap->useCase, TRUE);
    uint8 srcChannelCount = capClientGetChannelCountForContext(cap, cap->useCase, FALSE);
    bool isJointStereoInSink =FALSE;

    BapAseCodecConfiguration* codecConfig = NULL;

    /* CAP uses SNK channel count for identifying Joint Stereo config values to be used*/
    if (sinkChannelCount > 1)
        isJointStereoInSink = TRUE;

    localSrcAseCount = *sourceAseCount;
    localSinkAseCount = *sinkAseCount;

    if(bap->sourceAseCount < localSrcAseCount)
        localSrcAseCount = bap->sourceAseCount;

    if (bap->sinkAseCount < localSinkAseCount)
        localSinkAseCount = bap->sinkAseCount;
    /* adjust source and sink ASE counts */

    srcAse = capClientGetCodecConfigurableAses(BAP_ASE_SOURCE, localSrcAseCount, state, bap, cap->useCase);
    sinkAse = capClientGetCodecConfigurableAses(BAP_ASE_SINK, localSinkAseCount, state, bap, cap->useCase);

    /* if Required ASE Count is non zero and  NULL is returned either in sink/ source configurable ASE
     * Then there is resource crunch and ASEs needs to be freed. Retuen resource unavaliable */

    if (((srcAse == NULL) && localSrcAseCount)
         || ((sinkAse == NULL) && localSinkAseCount))
    {
        CAP_CLIENT_ERROR("\n(CAP): capBuildCodecConfigQuery ASES Exhausted!! Src Ase count : %d, Sink Ase count : %d \n", localSrcAseCount, localSinkAseCount);
        return NULL;
    }

    totalAseCount = localSrcAseCount + localSinkAseCount;

    CAP_CLIENT_INFO("\n(CAP): capClientBuildCodecConfigQueryReq, localSrcAseCount: %d, localSinkAseCount: %d \n", localSrcAseCount, localSinkAseCount);

    codecConfig = (BapAseCodecConfiguration*)
                        CsrPmemZalloc(totalAseCount * sizeof(BapAseCodecConfiguration));

    if (CAP_CLIENT_CIS_IS_UNI_SINK(cap->cigDirection)
          || CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection))
    {
        /* populate Sink Configs*/
        capClientGenerateBapCodecConfig(bap, *sinkAseCount,
                             sinkAse, BAP_ASE_SINK, cig->sinkConfig,
                             &codecConfig[0],
                             &cig->sinkLoc, cig,
                             cap->cigConfigMode, 
                             sinkChannelCount,
                             isJointStereoInSink);
    }

    if (CAP_CLIENT_CIS_IS_UNI_SRC(cap->cigDirection)
         || CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection))
    {
        /* populate Source Configs*/
        capClientGenerateBapCodecConfig(bap, *sourceAseCount,
                                 srcAse, BAP_ASE_SOURCE, cig->srcConfig,
                                 &codecConfig[*sinkAseCount],
                                 &cig->sourceLoc, cig, cap->cigConfigMode,
                                 srcChannelCount,
                                 isJointStereoInSink);

    }

    CsrPmemFree(srcAse);
    srcAse = NULL;

    CsrPmemFree(sinkAse);
    sinkAse = NULL;

    return codecConfig;
}

void capClientSendBapUnicastConfigCodecReq(CapClientGroupInstance *cap,
                     BapInstElement *bap,
                     CapClientCigElem *cig,
                     CAP_INST *inst)
{
    uint8 sinkAseCount = 0;
    uint8 sourceAseCount = 0;
    uint8 numOfCodecConfigs = 0;
    BapAseCodecConfiguration *codecConfig  = NULL;


    /* if more than one ase channels available and more than once ASE IDs
     * available configure 2 Source and 2 sink ASE's
     *
     * Note Check if its possible to construct query for 2 sink and 2 source
     * ASEs else fallback to 1 Source and 1 Sink ASE configuration
     */

    sourceAseCount = bap->srcAsesToBeConfigured;
    sinkAseCount = bap->sinkAsesToBeConfigured;

    if (sourceAseCount == 0 && sinkAseCount == 0)
    {
        CAP_CLIENT_INFO("\n(CAP): capClientSendBapUnicastConfigCodecReq Invalid Config \n");
        capClientSendUnicastClientConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_INVALID_PARAMETER);
        return;
    }

    codecConfig = capClientBuildCodecConfigQueryReq(&sourceAseCount, &sinkAseCount,
                                                    bap, cap, cig);

    if (codecConfig == NULL)
    {
        CAP_CLIENT_INFO("\n(CAP): capcapClientSendBapUnicastConfigCodecReq Insufficient Resources \n");
        capClientSendUnicastClientConnectCfm(inst->profileTask,inst, cap, CAP_CLIENT_RESULT_INSUFFICIENT_RESOURCES);
        return;
    }

    numOfCodecConfigs = sinkAseCount + sourceAseCount;

    /* Call Codec Config Req*/
    inst->bapRequestCount++;
    BapUnicastClientCodecConfigReq(bap->bapHandle, numOfCodecConfigs, codecConfig);

#ifdef CAP_CLIENT_NTF_TIMER
    /* Trigger the CAP timer to get all the NTF for Codec config within spec defined time */
    capClientNtfTimerSet(inst, cap, bap, BAP_ASE_STATE_CODEC_CONFIGURED);
#endif

    /* Free allocated Memory */
    CsrPmemFree(codecConfig);
    codecConfig = NULL;
}

void capClientSendAllUnicastConfigCodecReq(CapClientGroupInstance *cap,
                                    CsipInstElement *csip,
                                    BapInstElement *bap,
                                    CAP_INST *inst)
{
    bool locked = FALSE;
    CapClientCigElem* cig = (CapClientCigElem*)
                              CAP_CLIENT_GET_CIG_FROM_CONTEXT(cap->cigList, cap->useCase);

    while(bap && csip && cig)
    {
        locked = (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED);

        CAP_CLIENT_INFO("\n(CAP): capClientSendAllUnicastConfigCodecReq:bap->bapCurrentState : %x \n", bap->bapCurrentState);
        if((CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_AVLBLE_AUDIO_CONTEXT) && locked && csip->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
        {
            capClientSendBapUnicastConfigCodecReq(cap, bap ,cig, inst);
        }
        bap = bap->next;
        csip = csip->next;
    }
}

void handleRegisterTaskReq(CAP_INST* inst, const Msg msg)
{
    CapClientInternalRegisterTaskReq* req = (CapClientInternalRegisterTaskReq*)msg;
    CapClientGroupInstance* cap = NULL;
    CapClientProfileTaskListElem* task = NULL;

    cap = CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (inst->activeGroupId == req->groupId && cap)
    {
        task = (CapClientProfileTaskListElem*)
            CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

        if (task == NULL)
        {
            task = (CapClientProfileTaskListElem*)CAP_CLIENT_ADD_TASK_TO_LIST(cap->profileTaskList);
            task->profileTask = req->profileTask;
        }
        else
        {
            CAP_CLIENT_INFO("\n(CAP): handleRegisterTaskReq Task already registered \n");
            capClientSendRegisterTaskCfm(req->profileTask, req->groupId, CAP_CLIENT_RESULT_SUCCESS);
            return;
        }
        capClientSendRegisterTaskCfm(req->profileTask, req->groupId, CAP_CLIENT_RESULT_SUCCESS);
    }
    else
    {
        capClientSendRegisterTaskCfm(req->profileTask, req->groupId, CAP_CLIENT_RESULT_INVALID_GROUPID);
    }
}


void handleDeRegisterTaskReq(CAP_INST* inst, const Msg msg)
{
    CapClientInternalDeRegisterTaskReq* req = (CapClientInternalDeRegisterTaskReq*)msg;
    CapClientGroupInstance* cap = NULL;

    cap = CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (inst->activeGroupId == req->groupId && cap)
    {
        CAP_CLIENT_REMOVE_TASK_FROM_LIST(cap->profileTaskList, req->profileTask);

        capClientSendDeRegisterTaskCfm(req->profileTask, req->groupId, CAP_CLIENT_RESULT_SUCCESS);
    }
    else
    {
        capClientSendDeRegisterTaskCfm(req->profileTask, req->groupId, CAP_CLIENT_RESULT_INVALID_GROUPID);
    }
}

static uint8 generateCigId(CAP_INST* inst)
{
    uint8 i = 0;

    for (i = 0; i<CAP_CLIENT_MAX_SUPPORTED_CIGS; i++)
    {
        if(cigID[i] == 0)
        {
            cigID[i] = 1;
            inst->cigId = i;
            break;
        }
    }
    return inst->cigId;
}

static CapClientResult capClientUnicastConnectParamsValid(
    CapClientInternalUnicastConnectReq* req,
    CapClientGroupInstance *cap)
{
    CapClientSreamCapability sinkConfig = req->sinkConfig;
    CapClientSreamCapability srcConfig = req->srcConfig;

    CapClientBool validateSinkCfg =
            (req->sinkConfig != CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN) ? TRUE : FALSE;

    CapClientBool validateSrcCfg =
            (req->srcConfig != CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN) ? TRUE : FALSE;

    /* Validating  the Configuration
     * First we need to if only one bit is set in the CONFIG Bitmask or else reject
     * the query.
     * Cap needs to validate Configuration being used with discovered and reject
     * if the CONFIG is not present
     */

    if (sinkConfig & CAP_CLIENT_CODEC_ID_MASK)
        sinkConfig &= ~CAP_CLIENT_CODEC_ID_MASK;

    if (srcConfig & CAP_CLIENT_CODEC_ID_MASK)
        srcConfig &= ~CAP_CLIENT_CODEC_ID_MASK;

    if (validateSinkCfg && !VALIDATE_INPUT_CONFIG(sinkConfig))
    {
        CAP_CLIENT_INFO("\n(CAP) handleUnicastConnectionReq: Invalid Sink Codec Configuration");
        return CAP_CLIENT_RESULT_INVALID_PARAMETER;
    }

    if (validateSrcCfg && !VALIDATE_INPUT_CONFIG(srcConfig))
    {
        CAP_CLIENT_INFO("\n(CAP) handleUnicastConnectionReq: Invalid Source Codec Configuration");
        return CAP_CLIENT_RESULT_INVALID_PARAMETER;
    }

    if (VALIDATE_USECASE(req->useCase))
    {
        CAP_CLIENT_INFO("\n(CAP) handleUnicastConnectionReq: INVALID uscase");
        return CAP_CLIENT_RESULT_INVALID_PARAMETER;
    }

    if (validateSinkCfg && !capClientIsContextAvailable(req->useCase, cap, TRUE))
    {
        CAP_CLIENT_INFO("\n(CAP) handleUnicastConnectionReq: Sink Context Unavailable");
        return CAP_CLIENT_RESULT_CONTEXT_UNAVAILABLE;
    }

    if (validateSrcCfg && !capClientIsContextAvailable(req->useCase, cap, FALSE))
    {
        CAP_CLIENT_INFO("\n(CAP) handleUnicastConnectionReq: Source Context Unavailable");
        return CAP_CLIENT_RESULT_CONTEXT_UNAVAILABLE;
    }

    if ((req->cigConfigMode & CAP_CLIENT_MODE_JOINT_STEREO) == CAP_CLIENT_MODE_JOINT_STEREO
           && (!validateSinkCfg || (capClientNumOfBitSet(req->sinkAudioLocations) > CAP_CLIENT_JS_LOC_MAX))
           && (!validateSrcCfg || (capClientNumOfBitSet(req->srcAudioLocations) > CAP_CLIENT_JS_LOC_MAX)))
    {
        CAP_CLIENT_INFO("\n(CAP) handleUnicastConnectionReq:Audilocation Not Proper!! \n");
        return CAP_CLIENT_RESULT_INVALID_PARAMETER;
    }

    return CAP_CLIENT_RESULT_SUCCESS;
}

static void capClientUnicastConnectionReqHandler(CAP_INST* inst,
                                          void* msg,
                                          CapClientGroupInstance* cap)
{
    CsipInstElement* csip;
    BapInstElement* bap = (BapInstElement*)(cap->bapList.first);;
    CapClientCigElem* cig ;
    CapClientProfileMsgQueueElem* msgElem = (CapClientProfileMsgQueueElem*)msg;
    CapClientProfileTaskListElem* task = (CapClientProfileTaskListElem*)msgElem->task;
    CapClientInternalUnicastConnectReq* req = (CapClientInternalUnicastConnectReq*)(msgElem->capMsg);
    CapClientResult result = CAP_CLIENT_RESULT_SUCCESS;
    bool exitLoop = FALSE;

    /* Here we need to obtain lock on all the devices and the
     * Start BAP unicast Procedures*/

    /* check if the profile is already locked
     *
     * Note: If one device is in lock state in a group
     * it's assumed that all other participants are in lock state*/

    /* Now check if the group is  a co-ordinated set*/

    /* co ordinated set?
     *
     * Based on if co ordinated Set or not decide number of ASEs required
     * and then start BAP procedures
     *
     * */

    CAP_CLIENT_INFO("\n(CAP) CAPCLIENT STATE : %d\n", cap->capState);

    /* Check if the CIG is present for same context, if already present then reuse else add new CIG */

    cig = CAP_CLIENT_GET_CIG_FROM_CONTEXT(cap->cigList, req->useCase);
    cap->useCase = req->useCase;

    /* Cehck the streaming flag, if streaming is there it can be two cases as:
     * Case 1: unicast connect came for non CSIP device *
     * Case 2: Unicast connect came for the CSIP device and its for the new device in the group */
    if (inst->streaming || cig)
    {
        BapInstElement* temp = bap;
        uint8 bapDeviceCount = 0;
        /* Need to add failure handling */
        while (temp)
        {
            CAP_CLIENT_INFO("\n handleUnicastConnectionReq:bap->bapCurrentState : %x \n", temp->bapCurrentState);

            if ((CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(temp, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_QOS_CONFIGURED) ||
                (CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(temp, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_STREAMING))
            {
                bapDeviceCount++;
                /* If all the devices are in qos configured state and App sent
                 * Unicast connect for the same use case then reject the command */
                if (bapDeviceCount == cap->bapList.count)
                {
                    /* Streaming is going on so unicast connect is not allowed */
                    if (cig)
                        result = CAP_CLIENT_RESULT_ALREADY_CONFIGURED;
                    else
                        result = CAP_CLIENT_RESULT_STREAM_ALREADY_ACTIVATED;
                    exitLoop = TRUE;
                }
            }
            else
            {
                CAP_CLIENT_INFO("\n handleUnicastConnectionReq:bap->bapCurrentState : %x \n", temp->bapCurrentState);

                /* Change the status to success if first device is streaming and unicast connect is called for the remaing device(for CSIP config) */
                if (cig && capClientIsGroupCoordinatedSet(cap))
                {
                    CAP_CLIENT_INFO("\n handleUnicastConnectionReq:This is called for other devices in the group bap->bapHandle : %x \n", bap->bapHandle);
                    result = CAP_CLIENT_RESULT_SUCCESS;
                }
                else if (CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(temp, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_STREAMING)
                    result = CAP_CLIENT_RESULT_STREAM_ALREADY_ACTIVATED;
                exitLoop = TRUE;
            }
            if (exitLoop == TRUE)
                break;

            temp = temp->next;
        }
    }

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
#ifdef CAP_CLIENT_NTF_TIMER
        /* Reset the flag here, as in if timer expirt happend flag will be set to TRUE and will not be reset untill new
         * timer is not kicked in but as the cap state failed reset the timer to send the confirmation */
        cap->capNtfTimeOut = FALSE;
#endif
        capClientSendUnicastClientConnectCfm(req->profileTask, inst, cap, result);
        CAP_CLIENT_INFO("\n handleUnicastConnectionReq: invalid state transition \n");
        return;
    }

    if (cig == NULL)
    {
        cig = (CapClientCigElem*)CAP_CLIENT_ADD_CIG(&cap->cigList);

        /* Genereate CIG Id using CIG Id Generator cap->cigId*/
        cig->cigId = generateCigId(inst);

        /* Check if ID exceeds Max CID, if yes then wrap around to 0*/
        if ((inst->cigId & CAP_CLIENT_CIG_ID_MAX) == CAP_CLIENT_CIG_ID_MAX)
            cig->cigId = inst->cigId = 0;

        /* Copy other parameters */
        cig->sinkConfig = req->sinkConfig;
        cig->srcConfig = req->srcConfig;
        cig->latency = req->highReliability;
        cig->context = req->useCase;
        cig->cisHandleCount = 0;		
        cig->sinkLocCount = 0;
        cig->srcLocCount = 0;
		cig->configuredSrcAses = 0;
		cig->configureSinkAses = 0;
		
        cap->numOfSourceAses = 0;

        cig->cigDirection = cap->cigDirection = 
                    capClientGetCisDirectionFromConfig(req->sinkConfig, req->srcConfig);

        /* Copy the Unicast Connect Parameters recieved to CIG*/

        if (task->unicastParam && 
            ((task->sinkConfig && (task->sinkConfig == req->sinkConfig)) ||
             (task->srcConfig  && (task->srcConfig == req->srcConfig))
            ))
        {
            SynMemCpyS(&cig->unicastParam,
                sizeof(CapClientUnicastConnectParamV1),
                task->unicastParam,
                sizeof(CapClientUnicastConnectParamV1));

             if(task->unicastParam->vsConfigLen > 0)
             {
                 cig->unicastParam.vsConfig = task->unicastParam->vsConfig;
             }

            /* Free the unicastParam stored in task, as we have copied relevant params
             * to specified CIG
             * vsconfig shall be freed when CIG is removed.
             */
             CsrPmemFree(task->unicastParam);
             task->unicastParam = NULL;
        }
    }

    inst->profileTask = req->profileTask;

    /* Copy audio locations to CAP */
    cig->sourceLoc  = req->srcAudioLocations;
    cig->sinkLoc  = req->sinkAudioLocations;

    cap->pendingOp = CAP_CLIENT_BAP_UNICAST_CONNECT;

    /* Default or QHS mode or Joint Stereo */
    cap->cigConfigMode = req->cigConfigMode;
	
	/* Increment the Number of Mic in cap->numOfSourceAses(overall mic), which will handle the case for devices getting added together or late join */
    cap->numOfSourceAses = cap->numOfSourceAses + req->numOfMic;

    CAP_CLIENT_INFO("\n handleUnicastConnectionReq:cig->cigId %d cig->sinkLoc %x cig->sourceLoc %x cap->numOfSourceAses %x\n", cig->cigId, cig->sinkLoc, cig->sourceLoc, cap->numOfSourceAses);

    /* Evaluate Number of Source and sink Ases required on CAP level */
    capClientEvaluateRequiredAses(cap, &cap->requiredSinks, &cap->requiredSrcs, cig, req->numOfMic);

    task->activeCig = cig;

    /* Make current cig to be active cig which can change once more for streaming use case */
    cap->activeCig = cig;
	
    /* Increment the Number of configuredSrcAses/configureSinkAses in CIG, which will handle the case for devices getting added together or late join */
    cig->configuredSrcAses = cig->configuredSrcAses + cap->requiredSrcs;
    cig->configureSinkAses = cig->configureSinkAses + cap->requiredSinks;
	
    /* Increment the Number of sinkLocCount/srcLocCount in CIG, which will handle the case for devices getting added together or late join 
	 * Although location count is currently getting ignored while calculating sdusize in  capClientGetSduSizeFromCapability*/
    cig->sinkLocCount = cig->sinkLocCount + capClientNumOfBitSet(req->sinkAudioLocations);
    cig->srcLocCount = cig->srcLocCount + capClientNumOfBitSet(req->srcAudioLocations);

    if (CAP_CLIENT_QHS_CONFIGURED(req->cigConfigMode))
    {
        CsrMemCpy(&cap->cigConfig, &req->cigConfig, sizeof(CapClientQhsConfig));
    }

    /* Calculate the number of Sink and src Ases Which needs to be configured per
     * BAP instance using Channel Count and Discovered Sink/Src ASE count */
    capClientRequiredAsesPerBap(cap ,bap, cig->cigId);

    if (capClientIsGroupCoordinatedSet(cap))
    {

        csip = (CsipInstElement*)((CsrCmnListElm_t*)(cap->csipList.first));

        if (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED)
        {
            capClientSendAllUnicastConfigCodecReq(cap, csip, bap, inst);
        }
        /* Otherwise obtain lock and the start BAP Procedures */
        else
        {
            /* Obtain Lock on the Devices */
            capClientSetCsisLockState(csip, &inst->csipRequestCount, TRUE);
        }
    }
    else
    {
        /* Only one device in the group */

        /* Usecase and What to do
         *
         * Media/Gaming usecase: Check if 2 sink channels available and then Proceed
         *                  Configure 2 sink ASEs
         *
         * Voice usecase: 2 sink and 2 source channels needs to be configured
         *                  configure 2 sink and 2 source ASEs
         *
         * Stereo recording usecase: 2 source channels
         *                  configure 2 Source ASEs
         *
         * Gaming with Voiceback :  2 sink and 2 Source channels
         *                  configure 2 sink and 2 source ASEs
         *
         * */

        capClientSendBapUnicastConfigCodecReq(cap, bap, cig, inst);
    }
}

void handleUnicastConnectionReq(CAP_INST  *inst, const Msg msg)
{
    CapClientInternalUnicastConnectReq* req = (CapClientInternalUnicastConnectReq*)msg;
    AppTask appTask;
    CapClientResult result;
    CapClientProfileMsgQueueElem* msgElem = NULL;
    CapClientProfileTaskListElem* task = NULL;
    CapClientBool isQueueEmpty = FALSE;
    CapClientGroupInstance *cap = NULL;

    /* if groupId is not same Switch the group
     *
     * Note: capSetNewActiveGroup sends CapActiveGroupChangeInd to the
     * application internally
     *
     * */
    cap = capClientSetNewActiveGroup(inst, req->groupId, FALSE);
    appTask = req->profileTask;

    /* If Group Id does not match with the list of cap groupiDs
     * Send CAP_CLIENT_RESULT_INVALID_GROUPID
     * */

    if (cap == NULL)
    {
        capClientSendUnicastClientConnectCfm(appTask, inst, cap, CAP_CLIENT_RESULT_INVALID_GROUPID);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/
    task = (CapClientProfileTaskListElem*)
                CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

    if (task == NULL)
    {
        capClientSendUnicastClientConnectCfm(appTask, inst, cap, CAP_CLIENT_RESULT_TASK_NOT_REGISTERED);
        return;
    }

    result = capClientUnicastConnectParamsValid(req, cap);
    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        capClientSendUnicastClientConnectCfm(req->profileTask, inst, cap, result);
        return;
    }

    /* Add Check the message queue is empty
     *
     *  If the queue is empty, add the message to queue and
     *  proceed to process the request, else add the message and return.
     *  Queued message will be processed once current message being processed
     *  recieves the cfm from lower layers
     */

    isQueueEmpty = CAP_CLIENT_IS_MSG_QUEUE_EMPTY(cap->capClientMsgQueue);

    msgElem = CapClientMsgQueueAdd(&cap->capClientMsgQueue, (void*)req, 0,
                                  req->type, capClientUnicastConnectionReqHandler, task);

    if (isQueueEmpty)
    {
        capClientUnicastConnectionReqHandler(inst, (void*)msgElem, cap);
    }
}

static void capClientPopulateCisTestConfig(
    BapUnicastClientCisTestConfig *cisTestConfig,
    uint8 cisId,
    uint8 cisCount,
    const CapClientCisTestConfig *config)
{
    uint8 i;
    bool cisIdFound = FALSE;

    for(i = 0; i < cisCount; i++)
    {
        if (cisTestConfig[i].cisId == 0)
        {
            /* first free entry, use it */
            cisTestConfig[i].cisId = cisId;
            cisIdFound = TRUE;
            break;
        }
        else if(cisTestConfig[i].cisId == cisId)
        {
            /* this index entry needs update */
            cisIdFound = TRUE;
            break;
        }
    }

    if (cisIdFound)
    {
        cisTestConfig[i].maxSduStoM = config->maxSduPtoC;
        cisTestConfig[i].maxPduStoM = config->maxPduPtoC;
        cisTestConfig[i].nse = config->nse;
        cisTestConfig[i].phyStoM = config->phyPtoC;
        cisTestConfig[i].bnStoM = config->bnPtoC;
        cisTestConfig[i].maxSduMtoS = config->maxSduCtoP;
        cisTestConfig[i].maxPduMtoS = config->maxPduCtoP;
        cisTestConfig[i].nse = config->nse;
        cisTestConfig[i].phyMtoS = config->phyCtoP;
        cisTestConfig[i].bnMtoS = config->bnCtoP;
    }
}

static BapUnicastClientCigTestParameters* capClientPopulateCigTestConfigReqQuery(
    CapClientInternalUnicastCigTestConfigReq* req,
    BapInstElement *bap,
    CapClientCigElem *cig,
    CapClientGroupInstance *cap)
{
    CSR_UNUSED(cap);
    BapUnicastClientCigTestParameters* param = NULL;
    BapAseElement* sinkAse = (BapAseElement*)bap->sinkAseList.first;
    BapAseElement* sourceAse = (BapAseElement*)bap->sourceAseList.first;

    param = (BapUnicastClientCigTestParameters*)
                CsrPmemZalloc(sizeof(BapUnicastClientCigParameters));

    param->cisTestConfig = (BapUnicastClientCisTestConfig*)
                CsrPmemZalloc(cig->cisHandleCount * sizeof(BapUnicastClientCisTestConfig));

    param->cigId = cig->cigId;
    param->cisCount = cig->cisHandleCount;
    param->framing = req->cigConfig->framing;
    param->ftMtoS = req->cigConfig->ftCtoP;
    param->ftStoM = req->cigConfig->ftPtoC;
    param->isoInterval = req->cigConfig->isoIinterval;
    param->packing = req->cigConfig->packing;
    param->sca = req->cigConfig->sca;
    param->sduIntervalMtoS = req->cigConfig->sduIntervalCtoP;
    param->sduIntervalStoM = req->cigConfig->sduIntervalPtoC;

    /* Identify the CIS associated with the useCase, check the direction and
     * then apply the new params for those CIS
     */
    /* Get the CIS ID for the use case */
    while(sourceAse)
    {
        if(sourceAse->useCase == req->useCase)
        {
            capClientPopulateCisTestConfig(param->cisTestConfig,
                                           sourceAse->cisId,
                                           cig->cisHandleCount,
                                           req->cigConfig->cisTestConfig);
        }
        sourceAse = sourceAse->next;
    }

    while(sinkAse)
    {
        if(sinkAse->useCase == req->useCase)
        {
            capClientPopulateCisTestConfig(param->cisTestConfig,
                                           sinkAse->cisId,
                                           cig->cisHandleCount,
                                           req->cigConfig->cisTestConfig);
        }
        sinkAse = sinkAse->next;
    }

    return param;
}

static void capClientUnicastSetVsMetadataReqHandler(CAP_INST* inst,
                                             void* msg,
                                             CapClientGroupInstance* cap)
{
    CapClientProfileMsgQueueElem* msgElem = (CapClientProfileMsgQueueElem*)msg;
    CapClientInternalUnicastSetVsConfigDataReq* req = (CapClientInternalUnicastSetVsConfigDataReq*)(msgElem->capMsg);
    CapClientProfileTaskListElem* task = NULL;

    task = (CapClientProfileTaskListElem*)
            CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

    if (task != NULL)
    {
        if(task->vendorConfigData)
            CsrPmemFree(task->vendorConfigData);

        task->vendorConfigData = CsrPmemZalloc(req->metadataLen);

        task->vendorConfigDataLen = req->metadataLen;

        cap->metadataLen = req->metadataLen;
        cap->metadata = req->metadataParam;

        SynMemCpyS(task->vendorConfigData,
                   task->vendorConfigDataLen,
                    cap->metadata,
                    cap->metadataLen);

        capClientSendUnicastSetVsConfigDataCfm(inst, cap, CAP_CLIENT_RESULT_SUCCESS);
    }
}

static void capClientUnicastCigTestReqHandler(CAP_INST* inst,
                                             void* msg,
                                             CapClientGroupInstance* cap)
{
    CapClientResult result;
    BapInstElement* bap;
    BapUnicastClientCigTestParameters *param = NULL;
    CapClientCigElem* cig;
    CapClientProfileMsgQueueElem* msgElem = (CapClientProfileMsgQueueElem*)msg;
    CapClientInternalUnicastCigTestConfigReq* req = (CapClientInternalUnicastCigTestConfigReq*)(msgElem->capMsg);

    bap = (BapInstElement*)(cap->bapList.first);

    result = capClientValidateCapState(cap->capState, req->type);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        capClientSendUnicastCigTestCfm(inst, cap, result);
        CAP_CLIENT_INFO("\n handleUnicastCigTestReq: invalid state transition \n");
        return;
    }


    if(bap == NULL)
    {
        CAP_CLIENT_INFO("\n capClientUnicastCigTestReqHandler: NULL instance BAP\n");
        capClientSendUnicastCigTestCfm(inst, cap, CAP_CLIENT_RESULT_NULL_INSTANCE);
        return;
    }

    /* if there was a recent failure the inform the application*/

    if (bap->recentStatus)
    {
        CAP_CLIENT_INFO("\n capClientUnicastCigTestReqHandler: There was a recent failure in BAP\n");
        capClientSendUnicastCigTestCfm(inst, cap, CAP_CLIENT_RESULT_NOT_SUPPORTED);
        return;
    }

    cig = CAP_CLIENT_GET_CIG_FROM_CONTEXT(cap->cigList, req->useCase);


    if (cig == NULL)
    {
        CAP_CLIENT_INFO("\n capClientUnicastCigTestReqHandler: CIG not configured for useCase\n");
        capClientSendUnicastCigTestCfm(inst, cap, CAP_CLIENT_RESULT_NOT_CONFIGURED);
        return;
    }
    
    param = capClientPopulateCigTestConfigReqQuery(req, bap, cig, cap);

    if (param == NULL)
    {
        CAP_CLIENT_INFO("\n capClientUnicastCigTestReqHandler: ASEs are Exhausted \n");
        capClientSendUnicastCigTestCfm(inst, cap, CAP_CLIENT_RESULT_INSUFFICIENT_RESOURCES);
        return;
    }

    inst->bapRequestCount++;
    BapUnicastClientCigTestConfigReq(cap->libTask, param);

    /* Free the allocated memory */
    CsrPmemFree(param->cisTestConfig);
    param->cisTestConfig = NULL;

    CsrPmemFree(param);
    param = NULL;

    /* Free the allocated memory */

    CsrPmemFree(req->cigConfig->cisTestConfig);
    req->cigConfig->cisTestConfig = NULL;

    CsrPmemFree(req->cigConfig);
    req->cigConfig = NULL;

}

void handleUnicastCigTestConfigReq(CAP_INST  *inst, const Msg msg)
{
    CapClientGroupInstance *cap = NULL;
    CapClientInternalUnicastCigTestConfigReq* req = (CapClientInternalUnicastCigTestConfigReq*)msg;
    CapClientProfileTaskListElem* task = NULL;
    CapClientProfileMsgQueueElem* msgElem = NULL;
    CapClientBool isQueueEmpty = FALSE;

    /* if groupId is not same Switch the group
     *
     * Note: capSetNewActiveGroup sends CapActiveGroupChangeInd to the
     * application internally
     *
     * */

    cap = capClientSetNewActiveGroup(inst, req->groupId, FALSE);

    /* If Group Id does not match with the list of cap groupiDs
     * Send CAP_CLIENT_RESULT_INVALID_GROUPID
     * */

    if (cap == NULL)
    {
        capClientSendUnicastCigTestCfm(inst, cap, CAP_CLIENT_RESULT_INVALID_GROUPID);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/
    task = (CapClientProfileTaskListElem*)
            CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

    if (task == NULL)
    {
        capClientSendUnicastCigTestCfm(inst, cap, CAP_CLIENT_RESULT_TASK_NOT_REGISTERED);
        return;
    }

    /* validate if use case is valid and CAP is in connected state for the passed
     * use case else return failure
     */
    if (VALIDATE_USECASE(req->useCase) &&
        (cap->capState != CAP_CLIENT_STATE_UNICAST_CONNECTED ||
        cap->useCase != req->useCase))
    {
        CAP_CLIENT_INFO("\n(CAP) handleUnicastCigTestReq: INVALID uscase or CAP is not in connected state");
        capClientSendUnicastCigTestCfm(inst, cap, CAP_CLIENT_RESULT_INVALID_OPERATION);
        return;
    }


    isQueueEmpty = CAP_CLIENT_IS_MSG_QUEUE_EMPTY(cap->capClientMsgQueue);

    msgElem = CapClientMsgQueueAdd(&cap->capClientMsgQueue, (void*)req, 
            0, req->type, capClientUnicastCigTestReqHandler, task);

    /* Add Check the message queue is empty
     *
     *  If the queue is empty, add the message to queue and
     *  proceed to process the request, else add the message and return.
     *  Queued message will be processed once current message being processed
     *  recieves the cfm from lower layers
     */


    if (isQueueEmpty)
    {
        capClientUnicastCigTestReqHandler(inst, (void*)msgElem, cap);
    }
}

void handleUnicastSetVsConfigDataReq(CAP_INST  *inst, const Msg msg)
{
    CapClientGroupInstance *cap = NULL;
    CapClientInternalUnicastSetVsConfigDataReq* req = (CapClientInternalUnicastSetVsConfigDataReq*)msg;
    CapClientProfileTaskListElem* task = NULL;
    CapClientProfileMsgQueueElem* msgElem = NULL;
    CapClientBool isQueueEmpty = FALSE;

    /* if groupId is not same Switch the group
     *
     * Note: capSetNewActiveGroup sends CapActiveGroupChangeInd to the
     * application internally
     *
     * */

    cap = capClientSetNewActiveGroup(inst, req->groupId, FALSE);

    /* If Group Id does not match with the list of cap groupiDs
     * Send CAP_CLIENT_RESULT_INVALID_GROUPID
     * */

    if (cap == NULL)
    {
        capClientSendUnicastSetVsConfigDataCfm(inst, cap, CAP_CLIENT_RESULT_INVALID_GROUPID);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/
    task = (CapClientProfileTaskListElem*)
            CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

    if (task == NULL)
    {
        capClientSendUnicastSetVsConfigDataCfm(inst, cap, CAP_CLIENT_RESULT_TASK_NOT_REGISTERED);
        return;
    }

    isQueueEmpty = CAP_CLIENT_IS_MSG_QUEUE_EMPTY(cap->capClientMsgQueue);

    msgElem = CapClientMsgQueueAdd(&cap->capClientMsgQueue, (void*)req, 
            0, req->type, capClientUnicastSetVsMetadataReqHandler, task);

    /* Add Check the message queue is empty
     *
     *  If the queue is empty, add the message to queue and
     *  proceed to process the request, else add the message and return.
     *  Queued message will be processed once current message being processed
     *  recieves the cfm from lower layers
     */


    if (isQueueEmpty)
    {
        capClientUnicastSetVsMetadataReqHandler(inst, (void*)msgElem, cap);
    }

}


void capClientHandleUnicastCigTestConfigureCfm(CAP_INST *inst,
                    BapUnicastClientCigTestConfigureCfm* cfm,
                    CapClientGroupInstance *cap)
{
    CapClientResult result = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);
    inst->bapRequestCount--;

    CAP_CLIENT_INFO("\n(CAP)capClientHandleUnicastCigTestConfigureCfm: Result: 0x%x, requestCount: %d\n",
        cfm->result, inst->bapRequestCount);

    if(inst->bapRequestCount == 0)
    {
        capClientSendUnicastCigTestCfm(inst, cap, result);
    }
}

static void capClientSendUnicastconnectCfm(CapClientGroupInstance *cap, CAP_INST *inst)
{
    BapInstElement* bap = (BapInstElement*)cap->bapList.first;

    while (bap)
    {
        /* Check for the bap cis handles free logic( running in loop as in future multiple can be there which need to be freed) */
        if (bap->cisHandles)
        {
            CsrPmemFree(bap->cisHandles);
            bap->cisHandles = NULL;
        }
        bap = bap->next;
    }

    CsipInstElement* csip = (CsipInstElement*)(cap->csipList.first);

    if (capClientIsGroupCoordinatedSet(cap) && (csip->lock == CAP_CLIENT_LOCK_STATE_ENABLED))
    {
        cap->pendingOp = CAP_CLIENT_BAP_UNICAST_CONNECT;
        capClientSetCsisLockState(csip, &inst->csipRequestCount, FALSE);
    }
    else
    {
        capClientSendUnicastClientConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_SUCCESS);
    }
}
void capClientHandleUnicastQosConfigureCfm(CAP_INST *inst,
                                     BapUnicastClientQosConfigureCfm* cfm,
                                     CapClientGroupInstance *cap)
{
    BapInstElement *bap = (BapInstElement*)
                              CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);

    bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);
    CapClientCigElem* cig = CAP_CLIENT_GET_CIG_FROM_CONTEXT(cap->cigList, cap->useCase);

    inst->bapRequestCount--;

    /* Bap current state is meanignful only when status is success and is getting used just to skip 
     * operation on a discovered bap */
    setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_QOS_CONFIGURED, cap->activeCig->cigId);

#ifdef CAP_CLIENT_NTF_TIMER
    /* Cancel the timer as all NTF is received */
    capClientNtfTimerReset(bap);
#endif

    CAP_CLIENT_INFO("\n(CAP)capHandleUnicastQosConfigureCfm: Qos Configure Cfm, Result: 0x%x, requestCount: %d\n",
                                                                         cfm->result, inst->bapRequestCount);

    /* Don't update the previous ind failure */
    if ((bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
        || (bap->recentStatus == CAP_CLIENT_RESULT_INPROGRESS))
    {
        bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);
    }

    if(inst->bapRequestCount == 0)
    {
        /* if all of the recent BAP procedures failed, return from here */
        bap = (BapInstElement*)cap->bapList.first;

        if (capClientManageError(bap, cap->bapList.count))
        {
            capClientSendUnicastClientConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_FAILURE_BAP_ERR);
            return;
        }

        /* Setup data is done(including dummy bap) and QOS config is completed so send the confirmation */
        if (cig && cig->dataPath == TRUE)
        {
            capClientSendUnicastconnectCfm(cap, inst);
            return;
        }

        capClientUnicastSetUpDataPathReqSend(bap, inst, cap);
    }
}

static void updateAseAndEnableQosConfig(CapClientGroupInstance *cap,
                                         CAP_INST *inst)
{
    BapInstElement *bap = (BapInstElement*)cap->bapList.first;
    BapAseElement  *aseFirst = (BapAseElement*)(bap->sinkAseList.first);
    uint8 cisId = aseFirst->cisId;
    uint8 cisCount;
    uint8 offset = 0;
    uint8 nextSinkCisHandleIdx = 0;
    uint8 nextSrcCisHandleIdx = 0;

    while (bap)
    {
        if (CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_CODEC_CONFIGURED)
        {
            cisCount = capClientGetCisCountPerBap(bap, cap->activeCig->context);
            BapAseElement* ase = NULL;
            uint8 aseInUse;

            CAP_CLIENT_INFO("\n(CAP)updateAseAndEnableQosConfig: cisCount: %x\n", cisCount);

            if (bap->cisHandles == NULL)
            {
                bap->cisHandles = (uint16*)CsrPmemZalloc(sizeof(uint16) * (cisCount));
                capClientCisHandlesListElem  *cisHandlesEntry = (capClientCisHandlesListElem*)cap->cisHandlesList->first;
                
                if (cisHandlesEntry && cisHandlesEntry->cisHandlesData)
                {
                    bap->cisHandles[0] = cisHandlesEntry->cisHandlesData->cisHandle[0];
                    cisId = cisHandlesEntry->cisHandlesData->cisId;

                    CAP_CLIENT_INFO("\n(CAP)updateAseAndEnableQosConfig: bap->cisHandles: %x cisId: %x\n", bap->cisHandles[0], cisId);

                    /* Free the Cis handles data and remove the entry from the cis list */
                    CsrPmemFree(cisHandlesEntry->cisHandlesData);
                    cisHandlesEntry->cisHandlesData = NULL;
                    CsrCmnListElementRemove(cap->cisHandlesList, (CsrCmnListElm_t*)cisHandlesEntry);
                }
            }
            else
            {
                cisId = cisId+1;
            }

            CAP_CLIENT_INFO("\n(CAP)updateAseAndEnableQosConfig outside: bap->cisHandles: %x cisId: %x\n", bap->cisHandles[0], cisId);
            /* Update the Cis handles to corresponding ASE for the previous dummy BAP */
            if (CAP_CLIENT_CIS_IS_UNI_SINK(cap->cigDirection)
                || CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection))
            {
                ase = (BapAseElement*)(bap->sinkAseList.first);
                nextSinkCisHandleIdx = capClientUpdateCisHandleInAseElem(ase,
                    cisCount - offset,
                    &bap->cisHandles[offset],
                    bap->asesInUse,
                    cap->useCase);

                /* update the ase data path direction which was updated during setup data path for the connected device */
                ase->datapathDirection = cap->cigDirection;

                aseInUse = capClientGetAseCountForUseCase(bap, cap->useCase);

                /* cis id updataion,from the previous bap ase cis id and increment by one to get the next cis id */
                capClientUpdateCisIdForAse(ase, cisId,
                    BAP_ASE_STATE_CODEC_CONFIGURED, &aseInUse, cap);
            }

            if (CAP_CLIENT_CIS_IS_UNI_SRC(cap->cigDirection)
                || CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection))
            {
                ase = (BapAseElement*)(bap->sourceAseList.first);
                nextSrcCisHandleIdx = capClientUpdateCisHandleInAseElem(ase,
                    cisCount - offset,
                    &bap->cisHandles[offset],
                    bap->asesInUse,
                    cap->useCase);

                /* update the ase data path direction which was updated during setup data path for the connected device */
                ase->datapathDirection = cap->cigDirection;

                aseInUse = capClientGetAseCountForUseCase(bap, cap->useCase);

                capClientUpdateCisIdForAse(ase, cisId,
                    BAP_ASE_STATE_CODEC_CONFIGURED, &aseInUse, cap);
            }
            /* 
             *  'offset' indicates whether the handle assignment is complete
             *  When 'cfm->cisCount - offset' hits zero there are no more handles to be assigned
             *  hence break out of the loop 
             */
            if (cisCount> offset)
                offset += CAP_CLIENT_GET_MAX(nextSinkCisHandleIdx, nextSrcCisHandleIdx);
            else
                break;
        }
        bap = bap->next;
    }

    /* Cig and data path is set so call only codec/qos operations */
    inst->bapRequestCount = 0;
    /*
     * If Co ordinated Set with multiple devices then send requests
     * else send only one. In this case if there is only one device then
     * for second iteration bap becomes NULL hence stopping the procedure
     *
     * This is taken care of internally by capSendBapUnicaseQosConfigReqSend
     */
    bap = (BapInstElement*)cap->bapList.first;
    capClientSendBapUnicastQosConfigReqSend(bap, cap, inst, &inst->bapRequestCount);
}

static void allocateBandwidth(CapClientGroupInstance *cap, uint8* totalCisCount)
{
    BapInstElement* bap = (BapInstElement*)cap->bapList.first;
    bool bapDiscovered = TRUE;
    uint8 cisCount  = capClientGetCisCountPerBap(bap,cap->useCase);

    while (bap && cap->setSize > 1)
    {
        if (bap->sinkAseCount == 0 && bap->sourceAseCount == 0)
            bapDiscovered = FALSE;
        bap = bap->next;
    }

    /* Check if all the devices are disocvered or not and all discovered then check the state also to determine */
    if (cap->setSize > 1 && ((cap->currentDeviceCount < cap->setSize) || !bapDiscovered))
    {
        *totalCisCount = *totalCisCount + (cap->setSize - cap->currentDeviceCount) * (cisCount);

        CAP_CLIENT_INFO("\n(CAP)allocateBandwidth: totalCisCount: %x \t deviceCisCount : %x \t currentDeviceCount : %d\n", 
        *totalCisCount, cisCount, cap->currentDeviceCount);
    }
}
void capClientHandleUnicastCodecConfigureCfm(CAP_INST *inst,
                                     BapUnicastClientCodecConfigureCfm* cfm,
                                     CapClientGroupInstance *cap)
{
    BapInstElement *bap = (BapInstElement*)
                              CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cfm->handle);

    CapClientCigElem* cig = CAP_CLIENT_GET_CIG_FROM_CONTEXT(cap->cigList, cap->useCase);

    inst->bapRequestCount--;

#ifdef CAP_CLIENT_NTF_TIMER
    /* Cancel the timer as all NTF is received */
    capClientNtfTimerReset(bap);
#endif

    CAP_CLIENT_INFO("\n(CAP)capHandleUnicastCodecConfigureCfm: Codec Configure Cfm, Result: 0x%x, requestCount: %d\n",
                                                           cfm->result, inst->bapRequestCount);

    /* Don't update the previous ind failure */
    if ((bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
         || (bap->recentStatus == CAP_CLIENT_RESULT_INPROGRESS))
    {
        bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);
    }

    /* Check if there are more than one devices
     * and also more than one ASE id is configured
     * Also if the usecase requires bidirectional CIS
     * reduce number of cises by factor of 2
     * */

    if(cap->activeCig)
    {
        bap->cisCount = capClientGetCisCountPerBap(bap, cap->activeCig->context);

        /* Bap current state is meaningful only when status is success and is getting used just to skip
         * operation on a discovered bap */
        setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_CODEC_CONFIGURED, cap->activeCig->cigId);
    }

    if(inst->bapRequestCount == 0)
    {
        uint8 totalCisCount = 0;

        /* Configure CIG*/
        /* Note: For media Use case we need two unidirectional CISes can be either with one device
         * or multiple device
         *
         * Generally in case if cap has more devices in group we can have only CIS with each device
         * in case of only one device with multiple ASEs, proceed with two CISes on single Device
         *
         * And there can be only CIG per Client
         * only one cis if more than one device and 2 if only one
         * */


        if (cap->activeCig == NULL)
        {
            /* return an Error saying internal issue */
            CAP_CLIENT_INFO("\n (CAP): At this point activeCig Cannot be NULL \n");
            capClientSendUnicastClientConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR);
            return;
        }

        /* if all of the recent BAP procedures failed, return from here */

        bap = (BapInstElement*)cap->bapList.first;

        if (capClientManageError(bap, cap->bapList.count))
        {
            capClientSendUnicastClientConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_FAILURE_BAP_ERR);
            return;
        }

        /* Setup data path is done for the dummy bap so need to update the ase with respectve cis handles */
        if (cig && cig->dataPath== TRUE)
        {
            updateAseAndEnableQosConfig(cap, inst);
            return;
        }

        totalCisCount = capClientGetTotalCisCount(bap, cap->useCase, cap->activeCig->cigDirection);

        /* Check how many devices are discovered in the group and if some devices are not disovered then CAP need
         * To allocate the bandwidth for all the non disocvered devices also */
        allocateBandwidth(cap, &totalCisCount);

        cap->totalCisCount = totalCisCount;
        /* IF there are no ASEs in use. Then send CAP_RESULT_FAILURE_BAP_ERR*/

        if (totalCisCount)
        {
            capClientSendBapCigConfigureReq(inst, cap, bap);
        }
        else
        {
            capClientSendUnicastClientConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_FAILURE_BAP_ERR);
            return;
        }
    }
}

void capClientHandleUnicastCigConfigureCfm(CAP_INST *inst,
                    BapUnicastClientCigConfigureCfm* cfm,
                    CapClientGroupInstance *cap)
{
    BapAseElement *ase = NULL;
    BapInstElement *bap = (BapInstElement*)(cap->bapList.first);
    uint8 cisCount = capClientGetCisCountPerBap(bap, cap->useCase);
    CapClientCigElem* cig = (CapClientCigElem*)CAP_CLIENT_GET_CIG_FROM_CIGID(cap->cigList, cfm->cigId);
    uint8 nextSinkCisHandleIdx = 0;
    uint8 nextSrcCisHandleIdx = 0;
    uint8 offset = 0;
    inst->bapRequestCount--;

    /* If the Cig Configure itself failed, there is no need to proceed further, return BAP failure */

    if(cfm->result)
    {
        CAP_CLIENT_INFO("\n(CAP)capHandleUnicastCigConfigureCfm: CIG config Failure in device\n");
        capClientSendUnicastClientConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_NOT_SUPPORTED);
        return;
    }

    /* Copy the handles number to corresponding CIG */
    cig->cisHandleCount = cfm->cisCount;

    /* Update both SINK and Source ASE list with CIS handles recieved in Cfm
     * Updation depends on Usecase, For Example only Conversational usecase needs
     * to update CIS handle on Source List */


    while (bap && cfm->cisCount)
    {
        if (CAP_CLIENT_CIS_IS_UNI_SINK(cap->cigDirection)
            || CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection))
        {
            ase = (BapAseElement*)(bap->sinkAseList.first);
            nextSinkCisHandleIdx = capClientUpdateCisHandleInAseElem(ase,
                                                   cfm->cisCount - offset,
                                                   &cfm->cisHandles[offset],
                                                   bap->asesInUse, 
                                                   cap->useCase);
        }

        if (CAP_CLIENT_CIS_IS_UNI_SRC(cap->cigDirection)
            || CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection))
        {
            ase = (BapAseElement*)(bap->sourceAseList.first);
            nextSrcCisHandleIdx = capClientUpdateCisHandleInAseElem(ase,
                                                        cfm->cisCount - offset,
                                                        &cfm->cisHandles[offset],
                                                        bap->asesInUse, 
                                                        cap->useCase);
        }

        bap = bap->next;

        /* 
         *  'offset' indicates whether the handle assignment is complete
         *  When 'cfm->cisCount - offset' hits zero there are no more handles to be assigned
         *  hence break out of the loop 
         */

        if (cfm->cisCount > offset)
            offset += CAP_CLIENT_GET_MAX(nextSinkCisHandleIdx, nextSrcCisHandleIdx);
        else
            break;

        /* If the bap is dummy then bandwidth reservation logic got kicked in store the cis handles in the bap */
        if (bap && CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, cap->activeCig->cigId) == CAP_CLIENT_BAP_STATE_INVALID)
        {
            /* traverse all the bap list and fill individual dummy bap with cis handles */
            while (bap)
            {
                uint8 i;
                /* It was assumed here number of cis for the non connected bap will be same as connected bap */
                bap->cisHandles = (uint16*)CsrPmemZalloc(sizeof(uint16) * (cisCount));

                /* Store the cis handles and later on when the device came into existence and ASE disocvery is done
                 * then map the handles to ASE's */
                for (i = offset; i < offset+cisCount; i++)
                {
                    bap->cisHandles[i - offset] = cfm->cisHandles[i];
                    CAP_CLIENT_INFO("\n(CAP)capHandleUnicastCigConfigureCfm: filling dummy handles %x offset %d \n", bap->cisHandles[i - offset], offset);
                }

                bap = bap->next;

                if (cfm->cisCount > offset)
                    offset += cisCount;
                else
                    break;
            }
            break;
        }
    }

    /*  When Cfm counter Hits Zero Send QOS configure Req*/

    if(inst->bapRequestCount == 0)
    {
        bap = (BapInstElement*)(cap->bapList.first);

        /*
         * If Co ordinated Set with multiple devices then send requests
         * else send only one. In this case if there is only one device then
         * for second iteration bap becomes NULL hence stopping the procedure
         *
         * This is taken care of internally by capSendBapUnicaseQosConfigReqSend
         * */

        capClientSendBapUnicastQosConfigReqSend(bap, cap, inst, &inst->bapRequestCount);
    }
}


void capClientSendUnicastClientConnectCfm(AppTask appTask,
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

    uint8 i = 0;
    BapInstElement* bap = NULL;

    CapClientProfileMsgQueueElem* msgElem = NULL;
    MAKE_CAP_CLIENT_MESSAGE(CapClientUnicastConnectCfm);

    message->deviceStatusLen = 0;
    message->groupId = inst->activeGroupId;
    message->deviceStatus = NULL;
    message->context = 0;
    message->cigId = CAP_CLIENT_INVALID_CIG_ID;
    if(gInst)
    {
        CAP_CLIENT_CLEAR_PENDING_OP(gInst->pendingOp);
        bap = (BapInstElement*)(gInst->bapList.first);
        message->context = gInst->useCase;
        message->numOfMicsConfigured = gInst->numOfSourceAses;
    }

    message->result = result;

    if(gInst)
    {
        if (result == CAP_CLIENT_RESULT_SUCCESS)
        {
            message->deviceStatusLen = inst->deviceCount;
            message->deviceStatus = (CapClientDeviceStatus*)
                CsrPmemZalloc(message->deviceStatusLen * sizeof(CapClientDeviceStatus));
            message->cigId = gInst->activeCig->cigId;

            for (i = 0; i < inst->deviceCount && bap; bap = bap->next)
            {
                message->deviceStatus[i].cid = bap->bapHandle;
                message->deviceStatus[i].result = bap->recentStatus;
                i++;
            }
            /* NOTE: Internal API which populates the above details needs to be tested with multiple Device
             *       i.e Standard LE
             */
            gInst->capState = CAP_CLIENT_STATE_UNICAST_CONNECTED;
            gInst->activeCig->state = CAP_CLIENT_STATE_UNICAST_CONNECTED;
            /*capGetAllProfileInstanceStatus(message->deviceCount, elem, &message->status, CAP_CLIENT_BAP);*/
        }
        else
        {
            /* UnicastConnect Was not successful,  hence find non connected CIG and remove it from the list */

            if (gInst->cigList.count)
            {
                CapClientCigElem* cig = (CapClientCigElem*)gInst->cigList.first;

                /* TODO: add a new search function to find CIG based on state and remove unintialized */
                while (cig)
                {
                    if ((cig->context == gInst->useCase)
                        && (cig->state == CAP_CLIENT_STATE_INVALID))
                        break;

                    cig = cig->next;
                }

                if(cig)
                    CAP_CLIENT_REMOVE_CIG(&gInst->cigList, (CsrCmnListElm_t*)cig);
            }
        }

    }

    CapClientMessageSend(appTask, CAP_CLIENT_UNICAST_CONNECT_CFM, message);

    /*
     * If the cfm was success and message queue is not
     * empty i.e msgElem is not NULL, handle the next
     * message
     *
     */
     /* Save CAP state and get next message */
    if (gInst)
        msgElem = capClientGetNextMsgElem(gInst);

    if (msgElem)
    {
        msgElem->handlerFunc(inst, (void*)msgElem, gInst);
    }
}

void capClientSendUnicastCigTestCfm(CAP_INST *inst,
                                   CapClientGroupInstance *gInst,
                                   CapClientResult result)
{
    CapClientProfileMsgQueueElem* msgElem = NULL;

    MAKE_CAP_CLIENT_MESSAGE(CapClientUnicastCigTestCfm);
    message->result = result;
    message->groupId = inst->activeGroupId;
    message->context = 0;

    if(gInst)
    {
        CAP_CLIENT_CLEAR_PENDING_OP(gInst->pendingOp);
        message->context = gInst->useCase;
    }

    CapClientMessageSend(inst->profileTask, CAP_CLIENT_UNICAST_CIG_TEST_CFM, message);

    /*
    * If the cfm was success and message queue is not
    * empty i.e msgElem is not NULL, handle the next
    * message
    *
    */
    /* Save CAP state and get next message */
     if (gInst)
        msgElem = capClientGetNextMsgElem(gInst);

     if (msgElem)
     {
          msgElem->handlerFunc(inst, (void*)msgElem, gInst);
     }
}

void capClientSendUnicastSetVsConfigDataCfm(CAP_INST *inst,
                                   CapClientGroupInstance *gInst,
                                   CapClientResult result)
{
    CapClientProfileMsgQueueElem* msgElem = NULL;

    MAKE_CAP_CLIENT_MESSAGE(CapClientUnicastSetVsConfigDataCfm);
    message->result = result;
    message->groupId = inst->activeGroupId;

    if(gInst)
    {
        CAP_CLIENT_CLEAR_PENDING_OP(gInst->pendingOp);
    }

    CapClientMessageSend(inst->profileTask, CAP_CLIENT_UNICAST_SET_VS_CONFIG_DATA_CFM, message);

    /*
    * If the cfm was success and message queue is not
    * empty i.e msgElem is not NULL, handle the next
    * message
    *
    */
    /* Save CAP state and get next message */
     if (gInst)
        msgElem = capClientGetNextMsgElem(gInst);

     if (msgElem)
     {
          msgElem->handlerFunc(inst, (void*)msgElem, gInst);
     }
}

void capClientSendRegisterTaskCfm(AppTask profileTask,
                            ServiceHandle groupId,
                            CapClientResult result)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientRegisterTaskCfm);
    message->result = result;
    message->groupId = groupId;

    CapClientMessageSend(profileTask, CAP_CLIENT_REGISTER_TASK_CFM, message);
}

void capClientSendDeRegisterTaskCfm(AppTask profileTask,
                              ServiceHandle groupId,
                              CapClientResult result)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientDeRegisterTaskCfm);
    message->result = result;
    message->groupId = groupId;

    CapClientMessageSend(profileTask, CAP_CLIENT_DEREGISTER_TASK_CFM, message);
}

/*************************************************************************************************************************/
/*****************************************************SET Param Req*******************************************************/

static void capClientSetUnicastConnectParam(CAP_INST* inst, CapClientInternalSetParamReq *req)
{
    AppTask appTask;
    CapClientResult result;
    CapClientProfileTaskListElem* task = NULL;
    CapClientGroupInstance* cap = NULL;

    /* if groupId is not same Switch the group
     *
     * Note: capSetNewActiveGroup sends CapActiveGroupChangeInd to the
     * application internally
     *
     * */
    cap = capClientSetNewActiveGroup(inst, req->profileHandle, FALSE);
    appTask = req->profileTask;

    if (((req->sinkConfig == CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN) &&
          (req->srcConfig == CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN)) ||
          req->numOfParamElems < CAP_CLIENT_MIN_PARAM_ELEMS ||
          ((req->paramType == CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT ||
          req->paramType == CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT_V1) &&
          req->numOfParamElems > 1))
    {
        CAP_CLIENT_INFO("\n(CAP)capClientSetUnicastConnectParam: Invalid Parameters \n");
        capClientSendSetParamCfm(req->profileTask, CAP_CLIENT_RESULT_INVALID_PARAMETER, req->profileHandle);
        return;
    }

    /* If Group Id does not match with the list of cap groupiDs
     * Send CAP_CLIENT_RESULT_INVALID_GROUPID
     * */
    if (cap == NULL)
    {
        capClientSendSetParamCfm(req->profileTask, CAP_CLIENT_RESULT_INVALID_GROUPID, req->profileHandle);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/
    task = (CapClientProfileTaskListElem*)
        CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

    if (task == NULL)
    {
        result = CAP_CLIENT_RESULT_TASK_NOT_REGISTERED;
        capClientSendSetParamCfm(req->profileTask, result, req->profileHandle);
        return;
    }

    task->sinkConfig = req->sinkConfig;
    task->srcConfig = req->srcConfig;

    /* Free the old values if API is called back to back */
    if (task->unicastParam)
    {
        if (task->unicastParam->vsConfigLen > 0)
        {
            CsrPmemFree(task->unicastParam->vsConfig);
            task->unicastParam->vsConfig = NULL;
        }

        CsrPmemFree(task->unicastParam);
        task->unicastParam = NULL;
    }

    /* if full unicast param is selected then assign the pointer as it is */
    if (req->paramType == CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT_V1)
    {
        task->unicastParam = (CapClientUnicastConnectParamV1*)req->paramElems;
    }
    else
    {
        /* CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT case
         * We need to allocate memory for complete set and assign it
         * free the memory for subset pointer
         */
         CapClientUnicastConnectParam *unicastParams =
             (CapClientUnicastConnectParam*)req->paramElems;
         task->unicastParam = CsrPmemZalloc(sizeof(CapClientUnicastConnectParamV1));

         /* Assign members in full set unicastParam*/
         task->unicastParam->rtnCtoP = unicastParams->rtnCtoP;
         task->unicastParam->rtnPtoC = unicastParams->rtnPtoC;
         task->unicastParam->sduSizeCtoP = unicastParams->sduSizeCtoP;
         task->unicastParam->sduSizePtoC = unicastParams->sduSizePtoC;
         task->unicastParam->codecBlocksPerSdu = unicastParams->codecBlocksPerSdu;
         task->unicastParam->phyCtoP = unicastParams->phy;
         task->unicastParam->phyPtoC = unicastParams->phy;
         task->unicastParam->maxLatencyPtoC = unicastParams->maxLatencyPtoC;
         task->unicastParam->maxLatencyCtoP = unicastParams->maxLatencyCtoP;
         task->unicastParam->sduIntervalCtoP = unicastParams->sduInterval;
         task->unicastParam->sduIntervalPtoC = unicastParams->sduInterval;
         task->unicastParam->sca = CAP_CLIENT_WORST_CASE_SCA;

         task->unicastParam->packing = CAP_CLIENT_PACKING_INTERLEAVED; /* default set for non-QHS mode*/
         task->unicastParam->framing = capClientGetFramingForCapability(req->sinkConfig);

         task->unicastParam->vsConfigLen = 0;
         task->unicastParam->vsConfig = NULL;

         /* Free the subset one as we have already allocated unicastParam */
         CsrPmemFree(unicastParams);
         unicastParams = NULL;
    }

    capClientSendSetParamCfm(appTask, CAP_CLIENT_RESULT_SUCCESS, req->profileHandle);
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */

void handleSetParamReq(CAP_INST* inst, const Msg msg)
{

    CapClientInternalSetParamReq* req = (CapClientInternalSetParamReq*)msg;

    switch (req->paramType)
    {
#ifdef INSTALL_LEA_UNICAST_CLIENT
        case CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT:
        case CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT_V1:
        {
            capClientSetUnicastConnectParam(inst, req);
        }
        break;
#endif
#ifdef INSTALL_LEA_BROADCAST_SOURCE
        case CAP_CLIENT_PARAM_TYPE_BCAST_CONFIG:
        {
            capClientBcastSrcSetConfigParam(inst, req);
        }
        break;
#endif
        default:
        {
            capClientSendSetParamCfm(req->profileTask, 
                                     CAP_CLIENT_RESULT_INVALID_PARAMETER, 
                                     req->profileHandle);
            CAP_CLIENT_INFO("\n(CAP) Invalid Param type\n");
        }
        break;
    }




}
