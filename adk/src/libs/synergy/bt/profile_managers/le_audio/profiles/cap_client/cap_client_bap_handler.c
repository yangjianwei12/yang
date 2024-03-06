/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_bap_handler.h"
#include "cap_client_util.h"
#include "cap_client_init.h"
#include "cap_client_ase.h"
#include "cap_client_bap_pac_record.h"
#include "cap_client_init_stream_and_control_req.h"
#include "cap_client_add_new_dev.h"
#include "cap_client_available_audio_context_req.h"
#include "cap_client_discover_audio_capabilities_req.h"
#include "cap_client_start_stream_req.h"
#include "cap_client_remove_device_req.h"
#include "cap_client_unicast_connect_req.h"
#include "cap_client_update_audio_req.h"
#include "cap_client_vcp_operation_req.h"
#include "cap_client_stop_stream_req.h"
#include "cap_client_broadcast_assistant_periodic_scan.h"
#include "cap_client_broadcast_assistant_sync_to_adv.h"
#include "cap_client_broadcast_assistant_add_modify_src_req.h"
#include "cap_client_broadcast_assistant_remove_src_req.h"
#include "cap_client_broadcast_src.h"
#include "cap_client_debug.h"
#include "cap_client_csip_handler.h"
#include "cap_client_vcp_handler.h"
#include "cap_client_common.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
#define CAP_PACS_AUDIO_LOCATION_LENGTH 4
#define CAP_PACS_AUDIO_CONTEXT_LENGTH  4

static void capClientHandleBapinitCfm(CAP_INST* inst, const Msg msg, CapClientGroupInstance* gInst)
{
    BapInitCfm* cfm = (BapInitCfm*)msg;
    BapInstElement* bap = (BapInstElement*)
             CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(gInst->bapList, cfm->handle);
    bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);

    if (cfm->result != BAP_RESULT_INPROGRESS)
        inst->bapRequestCount--;

    CAP_CLIENT_INFO("\n(CAP) capClientHandleBapinitCfm: inst->bapRequestCount: %d cfm->result: %d\n", inst->bapRequestCount, cfm->result);

    if (inst->bapRequestCount == 0)
    {
        CsrCmnListElm_t* elem = (CsrCmnListElm_t*)((&gInst->bapList)->first);
        /* If Role is Commander Then Intialize VCP as well */

        /*Check if all instances errored */

        if (capClientManageError((BapInstElement*)elem, gInst->bapList.count))
        {
            capClientSendStreamControlInitCfm(inst,
                                             FALSE,
                                             CAP_CLIENT_RESULT_FAILURE_BAP_ERR,
                                             gInst->role,
                                             elem);
            return;
        }

        if (gInst->role & CAP_CLIENT_COMMANDER)
        {
            capClientInitializeVcpInstance(inst);
        }
        else
        {
            /* Send InitStreamAndControlCfm  to application */
            CAP_CLIENT_INFO("\n(CAP) Send InitStreamControlCfm\n");
            gInst->capState = CAP_CLIENT_STATE_INIT_STREAM_CTRL;

            capClientSendStreamControlInitCfm(inst,
                                             FALSE,
                                             CAP_CLIENT_RESULT_SUCCESS,
                                             gInst->role,
                                             elem);
            capClientCsipEnableCcd(inst, TRUE);
        }
    }
}

static void capClientHandleBapDiscoverRemoteAudioCapabilityInd(CAP_INST* inst,
                                BapPacsAudioCapabilityNotificationInd* ind,
                                CapClientGroupInstance* gInst)
{
    /* We need to parse PAC record and assign to
     * proper capability */
    uint8 i;
    MAKE_CAP_CLIENT_MESSAGE(CapClientPacRecordChangedInd);
    message->groupId = gInst->groupId;

    CapClientMessageSend(inst->appTask, CAP_CLIENT_PAC_RECORD_CHANGED_IND, message);

    CsrPmemFree(ind->pacRecords[0]);
    for (i = 0; i < ind->numPacRecords; i++)
    {
        ind->pacRecords[i] = NULL;
    }
}

static void capClientHandleBapPacsNotificationInd(CAP_INST* inst,
                                  BapPacsNotificationInd* ind,
                                  CapClientGroupInstance* gInst)
{
    CSR_UNUSED(inst);
    BapInstElement* bap = (BapInstElement*)
        CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(gInst->bapList, ind->handle);

    CAP_CLIENT_INFO("\n(CAP) capClientHandleBapPacsNotificationInd: result: 0x%x, value_len: %d", ind->result, ind->valueLength);

    switch (ind->notifyType)
    {
        case BAP_PACS_NOTIFICATION_SUPPORTED_AUDIO_CONTEXT:
        {
            if (ind->result == BAP_RESULT_SUCCESS && ind->valueLength == CAP_PACS_AUDIO_CONTEXT_LENGTH)
            {
                CapClientSupportedContext context;
                context = *((CapClientSupportedContext*)ind->value);
                capClientCacheRemoteSupportedContext(context);
            }
        }
        break;

        case BAP_PACS_NOTIFICATION_SINK_AUDIO_LOCATION:
        {
            if (ind->result == BAP_RESULT_SUCCESS && ind->valueLength == CAP_PACS_AUDIO_LOCATION_LENGTH)
            {
                MAKE_CAP_CLIENT_MESSAGE(CapClientAudioLocationChangeInd);
                CapClientAudioLocation audioLocation = *((CapClientAudioLocation*)ind->value);
                capClientCacheRemoteAudioLocations(audioLocation, BAP_AUDIO_SINK_RECORD, ind->handle);

                message->groupId = inst->activeGroupId;
                message->audioLocation = audioLocation;
                message->sink = TRUE;
                message->cid = ind->handle;

                CapClientMessageSend(inst->appTask, CAP_CLIENT_AUDIO_LOCATION_CHANGE_IND, message);
            }
        }
        break;

        case BAP_PACS_NOTIFICATION_SOURCE_AUDIO_LOCATION:
        {
            if (ind->result == BAP_RESULT_SUCCESS && ind->valueLength == CAP_PACS_AUDIO_LOCATION_LENGTH)
            {
                MAKE_CAP_CLIENT_MESSAGE(CapClientAudioLocationChangeInd);
                CapClientAudioLocation audioLocation = *((CapClientAudioLocation*)ind->value);
                CAP_CLIENT_INFO("\n(CAP) capClientHandleBapPacsNotificationInd: Location: 0x%x", audioLocation);
                capClientCacheRemoteAudioLocations(audioLocation, BAP_AUDIO_SOURCE_RECORD, ind->handle);

                message->groupId = inst->activeGroupId;
                message->audioLocation = audioLocation;
                message->sink = FALSE;
                message->cid = ind->handle;

                CapClientMessageSend(inst->appTask, CAP_CLIENT_AUDIO_LOCATION_CHANGE_IND, message);
            }
        }
        break;

        case BAP_PACS_NOTIFICATION_AVAILABLE_AUDIO_CONTEXT:
        {
            if (ind->result == BAP_RESULT_SUCCESS && ind->valueLength == CAP_PACS_AUDIO_CONTEXT_LENGTH)
            {
                MAKE_CAP_CLIENT_MESSAGE(CapClientAvailableAudioContextInd);

                CapClientAvailableContext context = *((CapClientAvailableContext*)ind->value);
                CAP_CLIENT_INFO("\n(CAP) capClientHandleBapPacsNotificationInd: Available Context: 0x%x", context);
                bap->availableAudioContext = context;

                message->groupId = inst->activeGroupId;
                message->context = context;

                CapClientMessageSend(inst->appTask, CAP_CLIENT_AVAILABLE_AUDIO_CONTEXT_IND, message);
            }
        }
        break;

        default:
            CAP_CLIENT_WARNING("\n(CAP) capClientHandleBapPacsNotificationInd: Default Case invalid notify type");
            break;
    }

    if (ind->valueLength)
    {
        CsrPmemFree(ind->value);
        ind->value = NULL;
    }

}

static void capClientHandleBapDiscoverRemoteAudioCapabilityCfm(CAP_INST *inst,
                                    BapDiscoverRemoteAudioCapabilityCfm* cfm,
                                    CapClientGroupInstance *gInst)
{
    /* We need to parse PAC record and assign to
     * proper capability */
    int i = 0;
    BapInstElement *bap = (BapInstElement*)
                  CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(gInst->bapList, cfm->handle);
    bool isSink = (cfm->recordType == BAP_AUDIO_SINK_RECORD);

    bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);

    /* Check for the error if its BAP error then check if its genuine error or can be ignored as remote don't have
     * record type */
    if (cfm->result == BAP_RESULT_ERROR)
    {
        /* Check for the PACS role first */
        bool pacsRole;
        pacsRole = BapDiscoverPacsAudioRoleReq(bap->bapHandle, cfm->recordType);

        /* Remote dont have required pacs role so change the status to success as its genuine case */
        if (!pacsRole)
            bap->recentStatus = CAP_CLIENT_RESULT_SUCCESS;
    }

    if (cfm->result == BAP_RESULT_SUCCESS || cfm->result == BAP_RESULT_NOT_SUPPORTED)
    {
        CAP_CLIENT_INFO("\n(CAP)capHandleBapDiscoverRemoteAudioCapabilityCfm: Read PAC record result: 0x%x", cfm->result);
        capClientInterpretPacRecord(gInst, cfm->pacRecords, cfm->numPacRecords, cfm->recordType);
    }
    else
    {
        CAP_CLIENT_INFO("\n(CAP -> APP) :PAC record discovery error : %s \n", isSink ? "SinkPAC" : "SourcePAC");

        if (bap->next != NULL)
        {
            /* Moving onto next device as source was not found so update bap state for the current device */
            if (!isSink && CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, CAP_CLIENT_NO_CIG_ID_MASK) == CAP_CLIENT_BAP_STATE_IDLE)
            {
                setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_DISCOVER_COMPLETE, CAP_CLIENT_BAP_STATE_INVALID);
            }
            
            bap = bap->next;
            BapDiscoverRemoteAudioCapabilityReq(bap->bapHandle, cfm->recordType);
            return;
        }
        else if (!isSink && CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, CAP_CLIENT_NO_CIG_ID_MASK) == CAP_CLIENT_BAP_STATE_IDLE)
        {
            /* Last device in the group, source not found so the procedure will end here */
            setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_DISCOVER_COMPLETE, CAP_CLIENT_BAP_STATE_INVALID);
        }

        gInst->pendingCap &= ~CAP_CLIENT_PUBLISHED_CAPABILITY_PAC_RECORD;
        gInst->pendingCap &= ~CAP_CLIENT_PUBLISHED_CAPABILITY_AUDIO_LOC;
        gInst->pendingCap &= ~CAP_CLIENT_DISCOVER_ASE_STATE;

        CAP_CLIENT_INFO("\n(CAP)capHandleBapDiscoverRemoteAudioCapabilityCfm: Error Reading PAC record result: 0x%x", cfm->result);
    }


   if(!cfm->moreToCome)
   {
       gInst->pendingCap = gInst->pendingCap & ~CAP_CLIENT_PUBLISHED_CAPABILITY_PAC_RECORD;
       gInst->pendingCap &= ~(capClientIsRecordPresent(gInst, isSink) ? 0x00 :
                                 (CAP_CLIENT_PUBLISHED_CAPABILITY_AUDIO_LOC | CAP_CLIENT_DISCOVER_ASE_STATE));

       /* update bap status to Error only if both Sink and source discovery fails  */

       if (capClientIsRecordPresent(gInst, TRUE) ||
              (capClientIsRecordPresent(gInst, FALSE) &&
                              !inst->discoverSource))
       {
           bap->recentStatus = CAP_CLIENT_RESULT_SUCCESS;
       }
       else
       {
           bap->recentStatus = CAP_CLIENT_RESULT_CAPABILITIES_NOT_DISCOVERED;
       }

       capClientSendDiscoverCapabilityReq(gInst->pendingCap, bap, isSink, inst, gInst->setSize, gInst);
   }
   else
   {
       CAP_CLIENT_INFO("\n(CAP)capHandleBapDiscoverRemoteAudioCapabilityCfm: More Pac records to come");
   }

   for (i = 0; i < cfm->numPacRecords; i++)
   {
       if (cfm->pacRecords[i]->vendorSpecificMetadata &&
                  cfm->pacRecords[i]->vendorSpecificMetadataLen)
       {
           CsrPmemFree(cfm->pacRecords[i]->vendorSpecificMetadata);
           cfm->pacRecords[i]->vendorSpecificMetadata = NULL;
       }

       if (cfm->pacRecords[i]->vendorSpecificConfig &&
                  cfm->pacRecords[i]->vendorSpecificConfigLen)
       {
           CsrPmemFree(cfm->pacRecords[i]->vendorSpecificConfig);
           cfm->pacRecords[i]->vendorSpecificConfig = NULL;
       }
   }

   CsrPmemFree(cfm->pacRecords[0]);

   for (i = 0; i < cfm->numPacRecords; i++)
   {
       cfm->pacRecords[i] = NULL;
   }
}

static void capClientHandleRemoteAudioLocationCfm(CAP_INST *inst,
                                    BapGetRemoteAudioLocationCfm* cfm,
                                    CapClientGroupInstance *gInst)
{
    BapInstElement *bap = (BapInstElement*)
                  CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(gInst->bapList, cfm->handle);
    bool isSink = (cfm->recordType == BAP_AUDIO_SINK_RECORD);
    bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);
    inst->bapRequestCount--;
    inst->isSink = isSink;

    if(cfm->result == BAP_RESULT_SUCCESS)
    {
         capClientCacheRemoteAudioLocations(cfm->location, cfm->recordType, cfm->handle);
    }

    /* Check for the error if its BAP error then check if its genuine error or can be ingored ascs_error_response_ccidremote dont have
     * record type */
    if (cfm->result == BAP_RESULT_ERROR)
    {
        /* Check for the PACS role first */
        bool pacsRole;
        pacsRole = BapDiscoverPacsAudioRoleReq(bap->bapHandle, cfm->recordType);
        
        /* Remote dont have required pacs role so change the status to success as its genuine case */
        if (!pacsRole)
            bap->recentStatus = CAP_CLIENT_RESULT_SUCCESS;
    }

    if(inst->bapRequestCount == 0)
    {
        gInst->pendingCap = gInst->pendingCap & ~CAP_CLIENT_PUBLISHED_CAPABILITY_AUDIO_LOC;
        capClientSendDiscoverCapabilityReq(gInst->pendingCap, bap, isSink, inst, gInst->setSize, gInst);
    }
}

static void capClientHandleRemoteSupportedContextCfm(CAP_INST *inst,
          BapDiscoverAudioContextCfm* cfm,
          CapClientGroupInstance *gInst)
{
    uint32 context;
    BapInstElement *bap = (BapInstElement*)
                  CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(gInst->bapList, cfm->handle);

    bool supportedContext =(cfm->context == BAP_PAC_SUPPORTED_AUDIO_CONTEXT);
    bap->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_BAP);

    gInst->pendingCap = gInst->pendingCap & ~CAP_CLIENT_PUBLISHED_CAPABILITY_SUPPORTED_CONTEXT;

    if(supportedContext && cfm->result == BAP_RESULT_SUCCESS)
    {
        context = ((cfm->contextValue.sinkContext & 0x0000FFFF) |
                           ((cfm->contextValue.sourceContext << 16) & 0xFFFF0000));
        capClientCacheRemoteSupportedContext(context);
    }

    capClientSendDiscoverCapabilityReq(gInst->pendingCap, bap, inst->isSink, inst, gInst->setSize, gInst);

}

static void capClientHandleUnicastMsg(CAP_INST* inst, const Msg msg)
{
    CsrBtGattPrim* prim = (CsrBtGattPrim*)msg;
    CapClientGroupInstance* gInst =
        (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (gInst == NULL)
    {
        /* Ignore Unicast Client Messages that arrive after groupInstance is destroyed */
        CAP_CLIENT_INFO("capClientHandleUnicastMsg: gInst is NULL");
        return;
    }

    switch (*prim)
    {
        case BAP_INIT_CFM:
        {
            capClientHandleBapinitCfm(inst, msg, gInst);
        }
        break;

        case BAP_REGISTER_PACS_NOTIFICATION_CFM:
        {
            /* Consume it here */
            /* Nothing to do with this message*/
            CAP_CLIENT_INFO("\n(CAP) capClientHandleBapMsg: PACS client Notification Enabled!!\n");
        }
        break;

        case BAP_DISCOVER_REMOTE_AUDIO_CAPABILITY_CFM:
        {
            BapDiscoverRemoteAudioCapabilityCfm *cfm =
                          (BapDiscoverRemoteAudioCapabilityCfm*)msg;
            capClientHandleBapDiscoverRemoteAudioCapabilityCfm(inst, cfm, gInst);
        }
        break;

        case BAP_GET_REMOTE_AUDIO_LOCATION_CFM:
        {
            BapGetRemoteAudioLocationCfm *cfm =
                          (BapGetRemoteAudioLocationCfm*)msg;
            capClientHandleRemoteAudioLocationCfm(inst, cfm, gInst);
        }
        break;

        case BAP_DISCOVER_AUDIO_CONTEXT_CFM:
        {
            BapDiscoverAudioContextCfm *cfm =
                          (BapDiscoverAudioContextCfm*)msg;

            if(cfm->context == BAP_PAC_SUPPORTED_AUDIO_CONTEXT)
            {
               capClientHandleRemoteSupportedContextCfm(inst, cfm, gInst);
            }
            else if(cfm->context == BAP_PAC_AVAILABLE_AUDIO_CONTEXT)
            {
                capClientHandleRemoteAvailableContextCfm(inst, cfm, gInst);
            }
        }
        break;

        case BAP_UNICAST_CLIENT_READ_ASE_INFO_CFM:
        {
            BapUnicastClientReadAseInfoCfm *cfm =
                          (BapUnicastClientReadAseInfoCfm*)msg;
            capClientHandleReadAseInfoCfm(inst, cfm, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_CODEC_CONFIGURE_IND:
        {
             BapUnicastClientCodecConfigureInd *ind =
                         (BapUnicastClientCodecConfigureInd*)msg;

             capClientHandleUnicastCodecConfigureInd(inst, ind, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_CODEC_CONFIGURE_CFM:
        {
            BapUnicastClientCodecConfigureCfm *cfm =
                        (BapUnicastClientCodecConfigureCfm*)msg;

            capClientHandleUnicastCodecConfigureCfm(inst, cfm, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_CIG_CONFIGURE_CFM:
        {
            BapUnicastClientCigConfigureCfm *cfm =
                         (BapUnicastClientCigConfigureCfm*)msg;
            capClientHandleUnicastCigConfigureCfm(inst, cfm, gInst);

        }
        break;

        case BAP_UNICAST_CLIENT_CIG_TEST_CONFIGURE_CFM:
        {
            BapUnicastClientCigTestConfigureCfm *cfm =
                         (BapUnicastClientCigTestConfigureCfm*)msg;
            capClientHandleUnicastCigTestConfigureCfm(inst, cfm, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_REMOVE_CIG_CFM:
        {
            BapUnicastClientRemoveCigCfm* cfm = (BapUnicastClientRemoveCigCfm*)msg;
            /* In case ignoreMessage flag is set then no need to handle the confirms           *
             * This flag will only be set once the removeDevReq for complete CAP is issue.
             * Flag is reset in CapClient   */

            CAP_CLIENT_INFO("\n(CAP)BapUnicastClientRemoveCigCfm Result: %d, CIG ID: %d", cfm->result, cfm->cigId);
            capClientHandleRemoveCigCfm(inst, cfm, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_QOS_CONFIGURE_IND:
        {
            BapUnicastClientQosConfigureInd *ind =
                         (BapUnicastClientQosConfigureInd*)msg;

            capClientHandleUnicastQosConfigureInd(inst, ind, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_QOS_CONFIGURE_CFM:
        {
            BapUnicastClientQosConfigureCfm *cfm =
                         (BapUnicastClientQosConfigureCfm*)msg;

            capClientHandleUnicastQosConfigureCfm(inst, cfm, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_ENABLE_IND:
        {
            BapUnicastClientEnableInd *ind =
                         (BapUnicastClientEnableInd*)msg;

            capClientHandleUnicastAseEnableInd(inst, ind, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_ENABLE_CFM:
        {
            BapUnicastClientEnableCfm *cfm =
                         (BapUnicastClientEnableCfm*)msg;

            capClientHandleUnicastAseEnableCfm(inst, cfm, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_DISABLE_IND:
        {
            BapUnicastClientDisableInd *ind =
                         (BapUnicastClientDisableInd*)msg;
            capClientHandleUnicastBapAseDisableInd(inst, ind, gInst);

        }
        break;

        case BAP_UNICAST_CLIENT_DISABLE_CFM:
        {
            BapUnicastClientDisableCfm *cfm =
                         (BapUnicastClientDisableCfm*)msg;

            capClientHandleUnicastAseDisableCfm(inst, cfm, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_RELEASED_IND:
        {
            BapUnicastClientReleasedInd* ind =
                        (BapUnicastClientReleasedInd*)msg;

            capClientHandleBapAseReleasedInd(inst, ind, gInst);
        }
        break;

        case BAP_PACS_AUDIO_CAPABILITY_NOTIFICATION_IND:
        {
            BapPacsAudioCapabilityNotificationInd* ind =
                (BapPacsAudioCapabilityNotificationInd*)msg;

            capClientHandleBapDiscoverRemoteAudioCapabilityInd(inst, ind, gInst);

        }
        break;

        case BAP_PACS_NOTIFICATION_IND:
        {
            BapPacsNotificationInd* ind =
                               (BapPacsNotificationInd*)msg;
            capClientHandleBapPacsNotificationInd(inst, ind, gInst);

        }
        break;
        case BAP_UNICAST_CLIENT_RELEASE_IND:
        {
            BapUnicastClientReleaseInd *ind =
                         (BapUnicastClientReleaseInd*)msg;
            capClientHandleBapAseReleaseInd(inst, ind, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_RELEASE_CFM:
        {
            BapUnicastClientReleaseCfm *cfm =
                         (BapUnicastClientReleaseCfm*)msg;

            capClientHandleUnicastAseReleaseCfm(inst, cfm, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_CIS_CONNECT_IND:
        {
            BapUnicastClientCisConnectInd *ind =
                         (BapUnicastClientCisConnectInd*)msg;
            capClientHandleUnicastCisConnectInd(inst, ind, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_CIS_CONNECT_CFM:
        {
            BapUnicastClientCisConnectCfm *cfm =
                         (BapUnicastClientCisConnectCfm*)msg;
            capClientHandleUnicastCisConnectCfm(inst, cfm, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_CIS_DISCONNECT_IND:
        {
            BapUnicastClientCisDisconnectInd* ind =
                (BapUnicastClientCisDisconnectInd*)msg;

            capClientHandleCisDisconnectInd(inst, ind, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_CIS_DISCONNECT_CFM:
        {
            BapUnicastClientCisDisconnectCfm *cfm =
                         (BapUnicastClientCisDisconnectCfm*)msg;
            capClientHandleCisDisconnectCfm(inst, cfm, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_RECEIVER_READY_IND:
        {
            BapUnicastClientReceiverReadyInd *ind =
                         (BapUnicastClientReceiverReadyInd*)msg;

            if(ind->readyType == BAP_RECEIVER_START_READY)
            {
                capClientHandleUnicastRecieverStartReadyInd(inst, ind, gInst);
            }
            else
            {
                /* Reciever Stop ready handling */
                capClientHandleBapAseRecieverStopReadyInd(inst, ind, gInst);
            }
        }
        break;

        case BAP_UNICAST_CLIENT_RECEIVER_READY_CFM:
        {
            BapUnicastClientReceiverReadyCfm *cfm =
                         (BapUnicastClientReceiverReadyCfm*)msg;

            if(cfm->readyType == BAP_RECEIVER_START_READY)
            {
                capClientHandleUnicastRecieverStartReadycfm(inst, cfm, gInst);
            }
            else
            {
                capClientHandleBapAseRecieverStopReadyCfm(inst, cfm, gInst);
            }
        }
        break;

        case BAP_CLIENT_REMOVE_DATA_PATH_CFM:
        {
            BapRemoveDataPathCfm *cfm =
                             (BapRemoveDataPathCfm*)msg;

            capClientHandleDatapathRemoveCfm(inst, cfm, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_UPDATE_METADATA_IND:
        {
            BapUnicastClientUpdateMetadataInd *ind =
                          (BapUnicastClientUpdateMetadataInd*)msg;

            capClientHandleUpdateMetadataInd(inst, ind, gInst);
        }
        break;

        case BAP_UNICAST_CLIENT_UPDATE_METADATA_CFM:
        {
            BapUnicastClientUpdateMetadataCfm *cfm =
                          (BapUnicastClientUpdateMetadataCfm*)msg;

            capClientHandleUpdateMetadataCfm(inst, cfm, gInst);
        }
        break;

        case BAP_CLIENT_SETUP_DATA_PATH_CFM:
        {
            BapSetupDataPathCfm* cfm = (BapSetupDataPathCfm*)msg;
            capClientHandleSetupDataPathCfm(inst, cfm, gInst);
        }
        break;

        default:
        {
            CAP_CLIENT_INFO("capHandleBapMsg: Invalid prim: %d", *prim);
        }
        break;
    }
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
static void capClientHandleBroadcastAssistantSetCodeInd(CAP_INST *inst, 
                                  BapBroadcastAssistantSetCodeInd* ind)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastAsstSetCodeInd);
    message->groupId = inst->activeGroupId;
    message->cid = ind->handle;
    message->flags = ind->handle;
    message->sourceId = ind->sourceId;

    CapClientMessageSend(inst->profileTask, CAP_CLIENT_BCAST_ASST_SET_CODE_IND, message);
}

static void capClientHandleBroadcastAssistantMsg(CAP_INST* inst, const Msg msg)
{
    CsrBtGattPrim* prim = (CsrBtGattPrim*)msg;
    CapClientGroupInstance* gInst =
        (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);


    if (gInst == NULL)
    {
        /* Ignore Broadcast Assistant Messages that arrive after groupInstance is destroyed */
        CAP_CLIENT_INFO("capClientHandleBroadcastAssistantMsg: gInst is NULL");
        return;
    }

    switch (*prim)
    {
        case BAP_BROADCAST_ASSISTANT_START_SCAN_CFM:
        {
            capClientHandleBroadcastAssistantStartScanCfm(inst, msg, gInst);
        }
        break;

        case BAP_BROADCAST_ASSISTANT_STOP_SCAN_CFM:
        {
            capClientHandleBroadcastAssistantStopScanCfm(inst, msg, gInst);
        }
        break;

        case BAP_BROADCAST_ASSISTANT_SRC_REPORT_IND:
        {
             CAP_CLIENT_INFO("(CAP) : BapBroadcastAssistantSrcReportInd: New Report Indication \n");
             capClientHandleBroadcastAssistantSrcReportInd(inst, msg, gInst);
        }
        break;

        case BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_CFM:
        {
            BapBroadcastAssistantSyncToSrcStartCfm* cfm =
                             (BapBroadcastAssistantSyncToSrcStartCfm*)msg;
            capClientHandleBroadcastAssistantSyncToSrcStartCfm(inst, cfm, gInst);
        }
        break;

        case BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_CFM:
        {
            BapBroadcastAssistantSyncToSrcTerminateCfm* cfm =
                             (BapBroadcastAssistantSyncToSrcTerminateCfm*)msg;
            capClientHandleBroadcastAssistantSyncToSrcTerminateCfm(inst, cfm, gInst);
        }
        break;

        case BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_CFM:
        {
            BapBroadcastAssistantSyncToSrcCancelCfm* cfm =
                             (BapBroadcastAssistantSyncToSrcCancelCfm*)msg;
            capClientHandleBroadcastAssistantSyncToSrcCancelCfm(inst, cfm, gInst);
        }
        break;

        case BAP_BROADCAST_ASSISTANT_ADD_SOURCE_CFM:
        {
            capClientHandleBroadcastAssistantAddModifySrcCfm(inst, msg, gInst,
                                                     CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM);
        }
        break;

        case BAP_BROADCAST_ASSISTANT_MODIFY_SOURCE_CFM:
        {

            capClientHandleBroadcastAssistantAddModifySrcCfm(inst, msg, gInst,
                                                CAP_CLIENT_BCAST_ASST_MODIFY_SRC_CFM);
        }
        break;

        case BAP_BROADCAST_ASSISTANT_REMOVE_SOURCE_CFM:
        {

            capClientHandleBroadcastAssistantRemoveSrcCfm(inst, msg, gInst);
        }
        break;

        case BAP_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_CFM:
        {
            capClientHandleBroadcastAssistantRegisterNotificationCfm(inst, msg, gInst);
        }
        break;

        case BAP_BROADCAST_ASSISTANT_BRS_IND:
        {
            BapBroadcastAssistantBrsInd* ind =
                                (BapBroadcastAssistantBrsInd*)msg;
            capClientHandleBassBrsInd(inst, ind, gInst);
        }
        break;

        case BAP_BROADCAST_ASSISTANT_SYNC_LOSS_IND:
        {
            BapBroadcastAssistantSyncLossInd* ind =
                                 (BapBroadcastAssistantSyncLossInd*)msg;
            capClientHandleBassSyncLossInd(inst, ind);

        }
        break;

        case BAP_BROADCAST_ASSISTANT_READ_BRS_CFM:
        {
            BapBroadcastAssistantReadBrsCfm* cfm =
                          (BapBroadcastAssistantReadBrsCfm*)msg;
            capClientHandleBroadcastAssistantReadBrsCfm(inst, cfm, gInst);
        }
        break;

        case BAP_BROADCAST_ASSISTANT_SET_CODE_IND:
        {
            BapBroadcastAssistantSetCodeInd* ind =
                                  (BapBroadcastAssistantSetCodeInd*)msg;
            capClientHandleBroadcastAssistantSetCodeInd(inst, ind);
        }
        break;

        default:
        {
            CAP_CLIENT_INFO("capClientHandleBroadcastAssistantMsg: Invalid prim: %d", *prim);
        }
        break;
    }

}
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

void capClientHandleBapMsg(CAP_INST *inst, const Msg msg)
{
    CsrBtGattPrim *prim = (CsrBtGattPrim *)msg;

    switch(*prim)
    {

        case BAP_INIT_CFM:
        {
#ifdef INSTALL_LEA_BROADCAST_SOURCE
            BapInitCfm *cfm = (BapInitCfm*)msg;

            CAP_CLIENT_INFO("\n(CAP) BapInitCfm \n");

            if(cfm->role & BAP_ROLE_BROADCAST_SOURCE)
            {
                capClientHandleBapBroadcastInitCfm(inst, cfm);
            }
#endif /* INSTALL_LEA_BROADCAST_SOURCE */
#ifdef INSTALL_LEA_UNICAST_CLIENT
            else
            {
                capClientHandleUnicastMsg(inst, msg);
            }
#endif /* INSTALL_LEA_UNICAST_CLIENT */
        }
        break;

        case BAP_DEINIT_CFM:
        {
            BapDeinitCfm* cfm = (BapDeinitCfm*)msg;
            CAP_CLIENT_INFO("\n(CAP) BapDeinitCfm \n");

            if (cfm->role & BAP_ROLE_BROADCAST_SOURCE)
            {
                capClientRemoveBcastSrcInst(inst, msg);
            }
#ifdef INSTALL_LEA_UNICAST_CLIENT
            else
            {
                capClientRemoveGroup(msg, inst, CAP_CLIENT_BAP);
            }
#endif /* INSTALL_LEA_UNICAST_CLIENT */
        }
        break;

#ifdef INSTALL_LEA_UNICAST_CLIENT
        case BAP_REGISTER_PACS_NOTIFICATION_CFM:
        case BAP_DISCOVER_REMOTE_AUDIO_CAPABILITY_CFM:
        case BAP_GET_REMOTE_AUDIO_LOCATION_CFM:
        case BAP_DISCOVER_AUDIO_CONTEXT_CFM:
        case BAP_UNICAST_CLIENT_READ_ASE_INFO_CFM:
        case BAP_UNICAST_CLIENT_CODEC_CONFIGURE_IND:
        case BAP_UNICAST_CLIENT_CODEC_CONFIGURE_CFM:
        case BAP_UNICAST_CLIENT_CIG_CONFIGURE_CFM:
        case BAP_UNICAST_CLIENT_CIG_TEST_CONFIGURE_CFM:
        case BAP_UNICAST_CLIENT_REMOVE_CIG_CFM:
        case BAP_UNICAST_CLIENT_QOS_CONFIGURE_IND:
        case BAP_UNICAST_CLIENT_QOS_CONFIGURE_CFM:
        case BAP_UNICAST_CLIENT_ENABLE_IND:
        case BAP_UNICAST_CLIENT_ENABLE_CFM:
        case BAP_UNICAST_CLIENT_DISABLE_IND:
        case BAP_UNICAST_CLIENT_DISABLE_CFM:
        case BAP_UNICAST_CLIENT_RELEASED_IND:
        case BAP_PACS_AUDIO_CAPABILITY_NOTIFICATION_IND:
        case BAP_PACS_NOTIFICATION_IND:
        case BAP_UNICAST_CLIENT_RELEASE_IND:
        case BAP_UNICAST_CLIENT_RELEASE_CFM:
        case BAP_UNICAST_CLIENT_CIS_CONNECT_IND:
        case BAP_UNICAST_CLIENT_CIS_CONNECT_CFM:
        case BAP_UNICAST_CLIENT_CIS_DISCONNECT_IND:
        case BAP_UNICAST_CLIENT_CIS_DISCONNECT_CFM:
        case BAP_UNICAST_CLIENT_RECEIVER_READY_IND:
        case BAP_UNICAST_CLIENT_RECEIVER_READY_CFM:
        case BAP_UNICAST_CLIENT_UPDATE_METADATA_IND:
        case BAP_UNICAST_CLIENT_UPDATE_METADATA_CFM:
        case BAP_CLIENT_REMOVE_DATA_PATH_CFM:
        {
            capClientHandleUnicastMsg(inst, msg);
        }
        break;
#endif /* INSTALL_LEA_UNICAST_CLIENT */

        case BAP_CLIENT_SETUP_DATA_PATH_CFM:
        {
#ifdef INSTALL_LEA_BROADCAST_SOURCE
            BapSetupDataPathCfm* cfm = (BapSetupDataPathCfm*)msg;

            /* Check for the profile handle whether it is broadcast or unicast profile handle 
             * and accordingly route the message */
            BroadcastSrcInst* bcastSrc = CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(inst->capClientBcastSrcList, cfm->handle);

            if(bcastSrc)
            {
                capClientHandleBroadcastSourceMsg(inst, msg);
            }
#endif /* INSTALL_LEA_BROADCAST_SOURCE */
#ifdef INSTALL_LEA_UNICAST_CLIENT
            else
            {
                capClientHandleUnicastMsg(inst, msg);

            }
#endif /* INSTALL_LEA_UNICAST_CLIENT */
        }
        break;

#ifdef INSTALL_LEA_BROADCAST_SOURCE
        case BAP_BROADCAST_SRC_CONFIGURE_STREAM_CFM:
        case BAP_BROADCAST_SRC_ENABLE_STREAM_CFM:
        case BAP_BROADCAST_SRC_DISABLE_STREAM_CFM:
        case BAP_BROADCAST_SRC_RELEASE_STREAM_CFM:
        case BAP_BROADCAST_SRC_UPDATE_METADATA_CFM:
        {
            capClientHandleBroadcastSourceMsg(inst, msg);
        }
        break;
#endif /* INSTALL_LEA_BROADCAST_SOURCE*/

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
        case BAP_BROADCAST_ASSISTANT_START_SCAN_CFM:
        case BAP_BROADCAST_ASSISTANT_STOP_SCAN_CFM:
        case BAP_BROADCAST_ASSISTANT_SRC_REPORT_IND:
        case BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_CFM:
        case BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_CFM:
        case BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_CFM:
        case BAP_BROADCAST_ASSISTANT_ADD_SOURCE_CFM:
        case BAP_BROADCAST_ASSISTANT_MODIFY_SOURCE_CFM:
        case BAP_BROADCAST_ASSISTANT_REMOVE_SOURCE_CFM:
        case BAP_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_CFM:
        case BAP_BROADCAST_ASSISTANT_BRS_IND:
        case BAP_BROADCAST_ASSISTANT_SYNC_LOSS_IND:
        case BAP_BROADCAST_ASSISTANT_READ_BRS_CFM:
        case BAP_BROADCAST_ASSISTANT_SET_CODE_IND:
        {
            capClientHandleBroadcastAssistantMsg(inst, msg);
        }
        break;
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

        default:
        {
            CAP_CLIENT_INFO("capClientHandleBapMsg: Invalid prim: %d", *prim);
        }
        break;
    }
}
