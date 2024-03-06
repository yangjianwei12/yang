/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_discover_audio_capabilities_req.h"
#include "cap_client_csip_handler.h"
#include "cap_client_bap_pac_record.h"
#include "cap_client_util.h"
#include "cap_client_common.h"
#include "cap_client_ase.h"
#include "cap_client_debug.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

static CapClientPublishedCapability config;

static void capClientReadBapAseInfoReq(BapInstElement *bap, 
                                BapAseType aseType, 
                                CAP_INST *inst, 
                                CapClientGroupInstance *gInst)
{
    if (gInst == NULL)
    {
        capClientSendDiscoverStreamCapabilitiesCfm(NULL, CAP_CLIENT_RESULT_NULL_INSTANCE, inst);
        CAP_CLIENT_ERROR("capReadBapAseInfoReq: gInst is NULL\n");
        return;
    }

    bool isSink = (aseType == BAP_ASE_SINK);

    if (capClientIsRecordPresent(gInst, isSink) && inst->bapRequestCount == 0)
    {
        /* If there are no sink/source Pac records then it is implied that
         * the peer device does not support corresponding role */
        if (gInst)
        {
            gInst->pendingCap = gInst->pendingCap & ~CAP_CLIENT_DISCOVER_ASE_STATE;
        }

        /* Discover all ASEs on all connected devices */
        while (bap)
        {
            if (bap->bapHandle)
            {
                CAP_CLIENT_INFO("\n(BAP)capClientReadBapAseInfoReq: Current BAP state : %x!! \n", bap->bapCurrentState);

                if (CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, CAP_CLIENT_NO_CIG_ID_MASK) == CAP_CLIENT_BAP_STATE_IDLE)
                {
                    /* Flush the aseId's stored inside bap instance and then discover*/
                    capClientFlushAseIdFromBapInstance(bap, aseType);

                    inst->bapRequestCount++;
                    BapUnicastClientReadAseInfoReq(bap->bapHandle, ASE_ID_ALL, aseType);
                    CAP_CLIENT_INFO("\n(BAP)capReadBapAseInfoReq: Reading Handle  !! \n");
                }
                else
                {
                    CAP_CLIENT_INFO("\n(BAP)capReadBapAseInfoReq: Already discovered!! \n");
                }
            }
            else
            {
                CAP_CLIENT_INFO("\n(BAP)capReadBapAseInfoReq: Invalid BAP Handle!! \n");
            }
            bap = bap->next;
        }
    }
    else if(config == CAP_CLIENT_DISCOVER_ASE_STATE && inst->bapRequestCount == 0)
    {
        gInst->pendingCap = gInst->pendingCap & ~CAP_CLIENT_DISCOVER_ASE_STATE;
        config = config & ~CAP_CLIENT_DISCOVER_ASE_STATE;
        capClientSendDiscoverStreamCapabilitiesCfm(gInst, CAP_CLIENT_RESULT_CAPABILITIES_NOT_DISCOVERED, inst);
        CAP_CLIENT_INFO("\n(BAP)capReadBapAseInfoReq: Invalid BAP Handle!! \n");
    }
}

static void capClientGetRemoteAudioLocationReq(BapInstElement *bap ,
                                        BapPacRecordType type,
                                        CAP_INST* inst)
{
    if (inst->bapRequestCount == 0)
    {
        while (bap)
        {
            if (bap->bapHandle)
            {
                CAP_CLIENT_INFO("\n(BAP)capClientGetRemoteAudioLocationReq: Current BAP state : %x!! \n", bap->bapCurrentState);

                if (CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, CAP_CLIENT_NO_CIG_ID_MASK) == CAP_CLIENT_BAP_STATE_IDLE)
                {
                    inst->bapRequestCount++;
                    BapGetRemoteAudioLocationReq(bap->bapHandle, type);
                }
                else
                {
                    CAP_CLIENT_INFO("\n(BAP)capGetRemoteAudioLocationReq: Already discovered!!  bap->bapHandle %x\n",bap->bapHandle);
                }
            }
            else
            {
                CAP_CLIENT_INFO("\n(BAP)capGetRemoteAudioLocationReq: Invalid BAP Handle!! \n");
            }
            bap = bap->next;
        }
    }
}

static void capClientPopulateRemoteAudioLocations(CapClientDeviceInfo *deviceInfo, uint8 *index,
                                            CapClientGroupInstance *cap, uint8 direction,
                                            CapClientAudioLocationInfo *loc, uint8 count)
{
    uint8 i;
    BapInstElement *bap;

    for(i = 0 ; i < count; i++)
    {
        CAP_CLIENT_INFO("\n(CAP)capClientPopulateRemoteAudioLocations: Index: %d \n", i);
        deviceInfo[*index].direction = direction;
        deviceInfo[*index].audioLocation = loc[i].location;
        deviceInfo[*index].cid = loc[i].cid;
        bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, loc[i].cid);
        deviceInfo[*index].result = bap->recentStatus;
        deviceInfo[*index].numAses = (direction == BAP_ASE_SINK? ((bap->sinkAseList).count):((bap->sourceAseList).count));
        bap->recentStatus = CAP_CLIENT_RESULT_SUCCESS;
        (*index)++;
    }
}

static void capClientPopulateCapability(CapClientLocalStreamCapabilities *record,
                                  uint8 *index,
                                  uint8 count,
                                  CapClientStreamCapability *capability,
                                  uint8 direction)
{
    uint8 i;
    for(i = 0; i < count; i++)
    {
        capability[*index].direction = direction;
        capability[*index].capability = record[i].capability;
        capability[*index].context = record[i].context;
        capability[*index].channelCount = record[i].channelCount;
        capability[*index].frameDuaration = record[i].frameDuaration;
        capability[*index].minOctetsPerCodecFrame = record[i].minOctetsPerCodecFrame;
        capability[*index].maxOctetsPerCodecFrame = record[i].maxOctetsPerCodecFrame;
        capability[*index].supportedMaxCodecFramesPerSdu = record[i].supportedMaxCodecFramesPerSdu;
        capability[*index].metadataLen = 0;
        capability[*index].metadata = NULL;

        if (record[i].metadataLen && record[i].metadata)
        {
            capability[*index].metadataLen = record[i].metadataLen;
            capability[*index].metadata = record[i].metadata;
            record[i].metadata = NULL;
        }

        (*index)++;
    }
}
void capClientSendDiscoverCapabilityReq(CapClientPublishedCapability capability,
                                  BapInstElement *bap,
                                  bool isSink,
                                  CAP_INST *inst,
                                  uint8 setSize,
                                  CapClientGroupInstance *gInst)
{
    BapPacRecordType type = isSink ? BAP_AUDIO_SINK_RECORD : BAP_AUDIO_SOURCE_RECORD;
    BapAseType aseType = isSink ? BAP_ASE_SINK: BAP_ASE_SOURCE;

    bap = (BapInstElement*)(gInst->bapList.first);

    CAP_CLIENT_INFO("\n capSendDiscoverCapabilityReq: Disocvering for BtConnId: 0x%x \n", bap->bapHandle);

    if(bap->bapHandle == CAP_CLIENT_INVALID_SERVICE_HANDLE)
    {
        CAP_CLIENT_INFO("\n capSendDiscoverCapabilityReq: Invalid Service Handle \n");
        return;
    }

    if(capability & CAP_CLIENT_PUBLISHED_CAPABILITY_PAC_RECORD)
    {
        /* First Delete all the Cached records and then discover */
        capClientFlushPacRecordsFromList(gInst, type);
        CAP_CLIENT_INFO("\n(CAP -> APP) :PAC record discovery %s \n", isSink ? "SinkPAC": "SourcePAC");
        BapDiscoverRemoteAudioCapabilityReq(bap->bapHandle, type);
    }
    else if(capability & CAP_CLIENT_PUBLISHED_CAPABILITY_AUDIO_LOC)
    {
        CAP_CLIENT_INFO("\n(CAP -> APP) :BAP Audio Location Discovery\n");
        capClientGetRemoteAudioLocationReq(bap, type, inst);
    }
    else if(capability & CAP_CLIENT_PUBLISHED_CAPABILITY_SUPPORTED_CONTEXT)
    {
        CAP_CLIENT_INFO("\n(CAP -> APP) :BAP Supported Audio Context Discovery\n");
        BapDiscoverAudioContextReq(bap->bapHandle, BAP_PAC_SUPPORTED_AUDIO_CONTEXT);
    }
    else if(capability & CAP_CLIENT_DISCOVER_ASE_STATE)
    {
        CAP_CLIENT_INFO("\n(CAP -> APP) :BAP Sink ASE Discovery\n");

        /* If the Group is not co ordinated instance then there will be only one
         * BAP instance in the group hence only one reuest is sent
         * */
        capClientReadBapAseInfoReq(bap, aseType, inst, gInst);

    }
    else
    {
        /* All discovery done. release the Lock*/

        CAP_CLIENT_CLEAR_PENDING_OP(gInst->pendingCap);

        if (inst->discoverSource)
        {
            /* Do discovery for Source */
            /* During Sink Discovery itself Supported audio context will already be discovered
             * Unset the bit mask*/

            CAP_CLIENT_INFO("\n(CAP -> APP) : Source discovery started... \n");

            gInst->pendingCap = config;
            gInst->pendingCap &= ~CAP_CLIENT_PUBLISHED_CAPABILITY_SUPPORTED_CONTEXT;

            inst->isSink = FALSE;
            inst->discoverSource = FALSE;
            capClientSendDiscoverCapabilityReq(gInst->pendingCap, bap, FALSE, inst, setSize, gInst);
        }
        else
        {
            if (inst->bapRequestCount == 0)
            {
                capClientSendDiscoverStreamCapabilitiesCfm(gInst, CAP_CLIENT_RESULT_SUCCESS, inst);
            }
        }
    }
}

static void capClientDiscoverStreamCapabilitiesReqHandler(CAP_INST* inst,
                                          void* msg,
                                          CapClientGroupInstance* cap)
{
    BapInstElement *bap = NULL;

    CapClientProfileMsgQueueElem* msgElem = (CapClientProfileMsgQueueElem*)msg;
    CapClientInternalDiscoverStreamCapReq* req = (CapClientInternalDiscoverStreamCapReq*)(msgElem->capMsg);

    bap = (BapInstElement*) ((CsrCmnListElm_t*) (cap->bapList.first));

    if (bap == NULL)
    {
        /* BAP instance here can never be NULL */
        CAP_CLIENT_ERROR("\n(CAP) : handleCapClientDiscoverStreamCapabilitiesReq : NUll BAP instance!! \n");
    }

    /* Start Discovery of the Published Capabilities*/

    /* Note: At this point BAP service handle as well as CID shall
     * be valid*/

    capClientSendDiscoverCapabilityReq(req->attribute, bap, inst->isSink, inst, cap->setSize, cap);
}

void handleCapClientDiscoverStreamCapabilitiesReq(CAP_INST *inst,const Msg msg)
{
    /*
     * First Check if the Group is A co-ordinated Set
     * Note: Group is a co ordinated Set if the CSIS instance has
     * setSize of more than 1 and has a valid CSIP handle
     * */

     CapClientGroupInstance *cap = NULL;
     CapClientInternalDiscoverStreamCapReq *req =
                        (CapClientInternalDiscoverStreamCapReq*)msg;
     CapClientProfileTaskListElem* task = NULL;
     CapClientProfileMsgQueueElem* msgElem = NULL;
     CapClientBool isQueueEmpty = FALSE;
     inst->isSink         = TRUE;
     inst->discoverSource =  inst->isSink;

     /* if groupId is not same Switch the group
      *
      * Note: capClientSetNewActiveGroup sends CapActiveGroupChangeInd to the
      * application internally
      * */

     cap = capClientSetNewActiveGroup(inst, req->groupId, FALSE);

     /* If Group Id does not match with the list of cap groupiDs send
      * Send CAP_CLIENT_RESULT_INVALID_GROUPID
      * */

     if (cap == NULL)
     {
         capClientSendDiscoverStreamCapabilitiesCfm(cap, CAP_CLIENT_RESULT_INVALID_GROUPID, inst);
         return;
     }

     if (req->groupId != inst->activeGroupId)
     {
           capClientSendDiscoverStreamCapabilitiesCfm(cap, CAP_CLIENT_RESULT_INVALID_OPERATION, inst);
           return;
     }

     cap->pendingCap =  req->attribute;
     config = req->attribute;

     /* Add Check the message queue is empty
      *
      *  If the queue is empty, add the message to queue and
      *  proceed to process the request, else add the message and return.
      *  Queued message will be processed once current message being processed
      *  receives the cfm from lower layers
      */

     isQueueEmpty = CAP_CLIENT_IS_MSG_QUEUE_EMPTY(cap->capClientMsgQueue);

     msgElem = CapClientMsgQueueAdd(&cap->capClientMsgQueue, (void*)req, 0,
                                   req->type, capClientDiscoverStreamCapabilitiesReqHandler, task);

     if (isQueueEmpty)
     {
         capClientDiscoverStreamCapabilitiesReqHandler(inst, (void*)msgElem, cap);
     }
}

void capClientSendDiscoverStreamCapabilitiesCfm(CapClientGroupInstance *cap, CapClientResult result, CAP_INST *inst)
{
    CapClientAudioLocationInfo* sourceLoc = NULL;
    CapClientAudioLocationInfo* sinkLoc = NULL;
    CapClientLocalStreamCapabilities* capabilities = NULL;
    uint8 sinkCount = 0, srcCount = 0;
    uint8 index = 0;
    CapClientProfileMsgQueueElem* msgElem = NULL;
    MAKE_CAP_CLIENT_MESSAGE(CapClientDiscoverStreamCapabilitiesCfm);

    message->groupId = inst->activeGroupId;

    if(result == CAP_CLIENT_RESULT_SUCCESS && cap)
    {
        /* Populate PAC records*/

        CAP_CLIENT_CLEAR_PENDING_OP(cap->pendingOp);
        cap->capState = CAP_CLIENT_STATE_DISCOVER_SUPPORTED_CAP;

        message->capability = (CapClientStreamCapability*)
                    CsrPmemZalloc(capClientGetPacRecordCount(cap)*sizeof(CapClientStreamCapability));

        capabilities = capClientGetRemotePacRecord(cap, &sinkCount, TRUE, CAP_CLIENT_CONTEXT_TYPE_PROHIBITED);
        capClientPopulateCapability(capabilities, &index, sinkCount, message->capability, BAP_ASE_SINK);
        CsrPmemFree(capabilities);

        capabilities = capClientGetRemotePacRecord(cap, &srcCount, FALSE, CAP_CLIENT_CONTEXT_TYPE_PROHIBITED);
        capClientPopulateCapability(capabilities, &index, srcCount, message->capability, BAP_ASE_SOURCE);
        CsrPmemFree(capabilities);
       
        capabilities = NULL;

        message->streamCapCount = index;

        /* Populate locations for both Sink and Source */
        index = 0;

        sinkLoc = capClientGetRemoteAudioLocationsInfo(&sinkCount, TRUE);

        sourceLoc = capClientGetRemoteAudioLocationsInfo(&srcCount, FALSE);

        message->deviceInfo = CsrPmemZalloc(sizeof(CapClientDeviceInfo)* (sinkCount + srcCount));

        if (sinkCount)
        {
            capClientPopulateRemoteAudioLocations(message->deviceInfo, &index, cap, BAP_ASE_SINK, sinkLoc, sinkCount);
            CsrPmemFree(sinkLoc);
        }


        if (srcCount)
        {
            capClientPopulateRemoteAudioLocations(message->deviceInfo, &index, cap, BAP_ASE_SOURCE, sourceLoc, srcCount);
            CsrPmemFree(sourceLoc);
        }

        message->deviceInfoCount = sinkCount + srcCount;
        message->supportedContext = capClientGetRemoteSupportedContext();
    }

    message->result = result;

    CapClientMessageSend(inst->appTask, CAP_CLIENT_DISCOVER_STREAM_CAPABILITIES_CFM, message);

    /*
     * If the cfm was success and message queue is not
     * empty i.e msgElem is not NULL, handle the next
     * message
     *
     */
     /* Save CAP state and get next message */
    if (cap)
    {
        msgElem = capClientGetNextMsgElem(cap);
    }

    if (msgElem)
    {
        msgElem->handlerFunc(inst, (void*)msgElem, cap);
    }
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */
