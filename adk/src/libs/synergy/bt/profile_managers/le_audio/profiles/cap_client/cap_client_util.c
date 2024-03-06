/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_bap_pac_record.h"
#include "cap_client_util.h"
#include "csr_pmem.h"
#include "cap_client_debug.h"
#include "cap_client_stop_stream_req.h"
#include "cap_client_start_stream_req.h"
#include "cap_client_unicast_connect_req.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
/* Currenty this timer value is added such that for EarBud total 6 ASE's can be configured
 * accroding to the spec we should wait till (noOFASE*1 = 6*1 seconds but cap is adding 1 more second extra so total
  (6+1)*second )
 * The minimum time period recommended by the cap for the Unicast Client to
 * consider reasonable to have received a notification of
 * the ASE Control Point characteristic from the Unicast Server is 1 second. */
#define CAP_CLIENT_NOTIFICATION_TIMER   10

#define CAP_CLIENT_SHIFT_OFFSET_NONE     0
#define CAP_CLIENT_SHIFT_OFFSET_WORD     16

#define CAP_GET_BIDIR_CISCOUNT_FROM_ASE(_ASECOUNT)   ((_ASECOUNT & 0x01) + (_ASECOUNT >> 1))

extern CapClientBroadcastSrcParams sourceParams;

static bool isConfigSupported(CapClientLocalStreamCapabilities* capabilities,
                              uint8 numOfCapabilities,
                              CapClientSreamCapability config,
                              uint8 *versionNum)
{
    uint8 i;
    bool result = FALSE;

    config &= ~CAP_CLIENT_CODEC_ID_MASK;

    for (i = 0; i < numOfCapabilities; i++)
    {
        if (capabilities[i].capability & config)
        {
            if (versionNum)
                *versionNum = capabilities[i].codecVersionNum;

            result = TRUE;
            break;
        }
    }
    return result;
}

static CsrInt32 sortNode(CsipInstElement *csipElem1 , CsipInstElement *csipElem2)
{
    if (csipElem1->rank < csipElem2->rank)
    {
        return -1;
    }
    else if (csipElem1->rank > csipElem2->rank)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void capClientFreeCapInternalMsg(uint16 type, Msg msg)
{
    switch (type)
    {
        case CAP_CLIENT_INTERNAL_UNICAST_CONNECT_REQ:
        case CAP_CLIENT_INTERNAL_UNICAST_STOP_STREAM_REQ:
        case CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ:
        case CAP_CLIENT_INTERNAL_MUTE_REQ:
        case CAP_CLIENT_INTERNAL_UNICAST_DISCONNECT_REQ:
        case CAP_CLIENT_INTERNAL_UNICAST_CIG_TEST_CONFIG_REQ:
        case CAP_CLIENT_INTERNAL_INIT_STREAM_CONTROL_REQ:
        case CAP_CLIENT_INTERNAL_DISCOVER_STREAM_CAP_REQ:
        case CAP_CLIENT_INTERNAL_DISCOVER_AVAIL_AUDIO_CONTEXT_REQ:
            break;

        case CAP_CLIENT_INTERNAL_UNICAST_VS_SET_CONFIG_DATA_REQ:
        case CAP_CLIENT_INTERNAL_UNICAST_UPDATE_AUDIO_REQ:
        case CAP_CLIENT_INTERNAL_UNICAST_START_STREAM_REQ:
        {
            CapClientInternalUnicastCommonReq* req =
                       (CapClientInternalUnicastCommonReq*)msg;

            CAP_CLIENT_WARNING("\n(CAP)capClientFreeCapInternalMsg :req->metadataLen req->metadataParam : 0x%x %p\n", req->metadataLen, req->metadataParam);

            CapClientGroupInstance *cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(req->groupId);
            if (cap && cap->metadata)
            {
                cap->metadataLen = 0;
                CsrPmemFree(cap->metadata);
                cap->metadata = NULL;
            }
        }
        break;

        default:
        {
            CAP_CLIENT_WARNING("\n(CAP)capClientFreeCapInternalMsg : Invalid message type: 0x%x \n", type);
            return;
        }
    }

    SynergyMessageFree(CAP_CLIENT_PRIM, msg);
    msg = NULL;
}

CapClientBool capClientGetCapGroupFromCid(CsrCmnListElm_t* elem, void *value)
{
    CsrBtConnId cid = *(CsrBtConnId*)value;
    CapClientHandleElem *cElem = (CapClientHandleElem*)elem;
    CapClientGroupInstance *gInst = CAP_CLIENT_GET_GROUP_INST_DATA(cElem->profileHandle);

    /* Search for the group Intsance for CID*/
    if (gInst)
    {
        if(CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(gInst->bapList, cid))
            return TRUE;
        if(CAP_CLIENT_GET_VCP_ELEM_FROM_CID(gInst->vcpList, cid))
            return TRUE;
    }

    return FALSE;
}

CapClientBool capClientGetHandleElmFromGroupId(CsrCmnListElm_t* elem, void* value)
{
    ServiceHandle groupId = *(ServiceHandle*)value;
    CapClientHandleElem* handleElm = (CapClientHandleElem*)elem;
    return (handleElm->profileHandle == groupId) ? TRUE : FALSE;
}

BapInstElement* capClientGetBapInstanceFromCid(CsrCmnList_t list, uint32 cid)
{
    BapInstElement *bElem = NULL;
    CapClientGroupInstance *gElem = NULL;
    CapClientHandleElem* elem = (CapClientHandleElem*)
                   CAP_CLIENT_GET_GROUP_ELEM_FROM_CID(list, cid);

    if(elem)
        gElem = (CapClientGroupInstance*)
                  CAP_CLIENT_GET_GROUP_INST_DATA(elem->profileHandle);

    if(gElem)
        bElem = (BapInstElement*)
                      CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(gElem->bapList, cid);

    return bElem;
}

VcpInstElement* capClientGetVcpInstanceFromCid(CsrCmnList_t list, uint32 cid)
{
    VcpInstElement *vElem = NULL;
    CapClientGroupInstance *gElem = NULL;
    CapClientHandleElem* elem = (CapClientHandleElem*)
                                       CAP_CLIENT_GET_GROUP_ELEM_FROM_CID(list, cid);

    if(elem)
        gElem = (CapClientGroupInstance*)
                       CAP_CLIENT_GET_GROUP_INST_DATA(elem->profileHandle);

    if(gElem)
        vElem = (VcpInstElement*)
                     CAP_CLIENT_GET_VCP_ELEM_FROM_CID(gElem->vcpList, cid);

    return vElem;
}


CsipInstElement* capClientGetCsipInstanceFromCid(CsrCmnList_t list, uint32 cid)
{
    CsipInstElement *cElem = NULL;
    CapClientGroupInstance *gElem = NULL;
    CapClientHandleElem* elem = (CapClientHandleElem*)
                   CAP_CLIENT_GET_GROUP_ELEM_FROM_CID(list, cid);

    if(elem)
        gElem = (CapClientGroupInstance*)
                   CAP_CLIENT_GET_GROUP_INST_DATA(elem->profileHandle);

    if(gElem)
        cElem = (CsipInstElement*)
                   CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(gElem->csipList, cid);


    return cElem;
}

VcpInstElement* capClientGetVcpInstanceFromPhandle(CsrCmnList_t list, ServiceHandle srvcHndl)
{
    VcpInstElement *vElem = NULL;
    CapClientGroupInstance *gElem = NULL;
    CsrCmnListElm_t* elem = NULL;

    elem = (CsrCmnListElm_t*)(list.first);

    for(;elem; elem = elem->next)
    {
        CapClientHandleElem* hElem = (CapClientHandleElem*)elem;
        gElem = CAP_CLIENT_GET_GROUP_INST_DATA(hElem->profileHandle);

        if(gElem)
            vElem = (VcpInstElement*)CAP_CLIENT_GET_VCP_ELEM_FROM_PHANDLE(gElem->vcpList, srvcHndl);

        if(vElem)
            return vElem;
    }

    return NULL;
}


BapInstElement* capClientGetBapInstanceFromPhandle(CsrCmnList_t list, ServiceHandle srvcHndl)
{
    BapInstElement *bElem = NULL;
    CapClientGroupInstance *gElem = NULL;
    CsrCmnListElm_t* elem = NULL;

    elem = (CsrCmnListElm_t*)(list.first);

    for(;elem; elem = elem->next)
    {
        CapClientHandleElem* hElem = (CapClientHandleElem*)elem;
        gElem = (CapClientGroupInstance*)
                    CAP_CLIENT_GET_GROUP_INST_DATA(hElem->profileHandle);

        if(gElem)
            bElem = (BapInstElement*)
                       CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(gElem->bapList, srvcHndl);

        if(bElem)
            return bElem;
    }

    return NULL;
}


CsipInstElement* capClientGetCsipInstanceFromPhandle(CsrCmnList_t list, ServiceHandle srvcHndl)
{
    CapClientGroupInstance *gElem = NULL;
    CsrCmnListElm_t* elem = NULL;

    elem = (CsrCmnListElm_t*)(list.first);
    for(;elem; elem = elem->next)
    {
        CsipInstElement *cElem = NULL;
        CapClientHandleElem* hElem = (CapClientHandleElem*)elem;
        gElem = (CapClientGroupInstance*)
                      CAP_CLIENT_GET_GROUP_INST_DATA(hElem->profileHandle);

        if(gElem)
            cElem = (CsipInstElement*)
                       CAP_CLIENT_GET_CSIP_ELEM_FROM_PHANDLE(gElem->csipList, srvcHndl);

        if(cElem)
            return cElem;
    }

    return NULL;
}

CapClientBool capClientGetCsipFromPhandle(CsrCmnListElm_t* elem, void *value)
{
    ServiceHandle handle = *(ServiceHandle*)value;
    CsipInstElement* cElem = (CsipInstElement *)elem;
    return (cElem->csipHandle == handle) ? TRUE : FALSE;
}

CapClientBool capClientGetVcpFromPhandle(CsrCmnListElm_t* elem, void *value)
{
    ServiceHandle handle = *(ServiceHandle*)value;
    VcpInstElement* vElem = (VcpInstElement *)elem;
    return (vElem->vcpHandle == handle) ? TRUE : FALSE;
}

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
CapClientBool capClientGetMicpFromPhandle(CsrCmnListElm_t* elem, void* value)
{
    ServiceHandle handle = *(ServiceHandle*)value;
    MicpInstElement* micpElem = (MicpInstElement*)elem;
    return (micpElem->micpHandle == handle) ? TRUE : FALSE;
}
#endif

CapClientBool capClientGetBapFromPhandle(CsrCmnListElm_t* elem, void *value)
{
    CsrBtConnId cid = *(CsrBtConnId*)value;
    BapInstElement* bElem = (BapInstElement *)elem;
    return (bElem->bapHandle == cid) ? TRUE : FALSE;
}

CapClientBool capClientGetBapFromGroupId(CsrCmnListElm_t* elem, void *value)
{
    uint16 groupId = *(uint16*)value;
    BapInstElement* bElem = (BapInstElement *)elem;
    return (bElem->groupId == groupId && ((bElem->bapCurrentState & CAP_CLIENT_BAP_STATE_INVALID) == CAP_CLIENT_BAP_STATE_INVALID)) ? TRUE : FALSE;
}

CapClientBool capClientGetBapFromCid(CsrCmnListElm_t* elem, void* value)
{
    CsrBtConnId cid = *(CsrBtConnId*)value;
    BapInstElement* bElem = (BapInstElement*)elem;
    return (bElem->cid == cid) ? TRUE : FALSE;
}

CapClientBool capClientGetVcpFromCid(CsrCmnListElm_t* elem, void *value)
{
    CsrBtConnId cid = *(CsrBtConnId*)value;
    VcpInstElement* vElem = (VcpInstElement *)elem;
    return (vElem->cid == cid) ? TRUE : FALSE;
}

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
CapClientBool capClientGetMicpFromCid(CsrCmnListElm_t* elem, void *value)
{
    CsrBtConnId cid = *(CsrBtConnId*)value;
    MicpInstElement* micpElem = (MicpInstElement *)elem;
    return (micpElem->cid == cid) ? TRUE : FALSE;
}
#endif

CapClientBool capClientGetCsipFromCid(CsrCmnListElm_t* elem, void *value)
{
    CsrBtConnId cid = *(CsrBtConnId*)value;
    CsipInstElement* cElem = (CsipInstElement *)elem;
    return (cElem->cid == cid) ? TRUE : FALSE;
}

CsrInt32 capClientSortCsipProfileList(CsrCmnListElm_t *elem1, CsrCmnListElm_t *elem2)
{
    /* Sort csipElem so smallest rank is placed first */
    CsipInstElement *csipElem1 = (CsipInstElement *)elem1;
    CsipInstElement *csipElem2 = (CsipInstElement *)elem2;

    return (sortNode(csipElem1, csipElem2));
}


CsrInt32 capClientSortBapProfileList(CsrCmnListElm_t *elem1, CsrCmnListElm_t *elem2)
{
    BapInstElement *elemA = (BapInstElement *)elem1;
    BapInstElement *elemB = (BapInstElement *)elem2;

    CapClientGroupInstance *cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(elemA->groupId);

    /* Sort bapelem  according to csip rdered access */
    if (cap)
    {
        CsipInstElement *csipElem1 = CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(cap->csipList, elemA->cid);
        CsipInstElement *csipElem2 = CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(cap->csipList, elemB->cid);

        if (csipElem1 && csipElem2)
        {
            return (sortNode(csipElem1, csipElem2));
        }
    }
    return 0;
}

CsrInt32 capClientSortVcpProfileList(CsrCmnListElm_t *elem1, CsrCmnListElm_t *elem2)
{
    VcpInstElement *elemA = (VcpInstElement *)elem1;
    VcpInstElement *elemB = (VcpInstElement *)elem2;

    CapClientGroupInstance *cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(elemA->groupId);

    /* Sort Vcpelem  according to csip rdered access */
    if (cap)
    {
        CsipInstElement *csipElem1 = CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(cap->csipList, elemA->cid);
        CsipInstElement *csipElem2 = CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(cap->csipList, elemB->cid);

        if (csipElem1 && csipElem2)
        {
            return (sortNode(csipElem1, csipElem2));
        }
    }
    return 0;
}

CapClientBool capClientSearchGroupId(CsrCmnListElm_t* elem, void *value)
{
    ServiceHandle groupId = *(ServiceHandle*)value;
    CapClientHandleElem* hElem = (CapClientHandleElem *)elem;
    return (hElem->profileHandle == groupId) ? TRUE : FALSE;
}

void capClientMessageQueueRemove(CsrCmnList_t* list)
{
    CapClientProfileMsgQueueElem *qElem =
              (CapClientProfileMsgQueueElem*)CAP_CLIENT_MSG_QUEUE_GET_FRONT(list);

    if (qElem == NULL)
    {
        CAP_CLIENT_INFO("\n(CAP) capClientMessageQueueRemove: Queue empty \n");
        return;
    }

    capClientFreeCapInternalMsg(qElem->MessageType, qElem->capMsg);
    qElem->capMsg = NULL;

    CsrCmnListElementRemove(list, (CsrCmnListElm_t*)qElem);
}

CapClientBool capClientGetTaskElemFromAppHandle(CsrCmnListElm_t* elem, void* value)
{
    AppTask appTask = *(AppTask*)value;
    CapClientProfileTaskListElem* tElem = (CapClientProfileTaskListElem*)elem;
    return ((tElem->profileTask == appTask) ? TRUE : FALSE);
}

CapClientBool capClientGetCigFromContext(CsrCmnListElm_t* elem, void* value)
{
    CapClientContext context = *(CapClientContext*)value;
    CapClientCigElem* cigElem = (CapClientCigElem*)elem;
    return (cigElem->context == context) ? TRUE : FALSE;
}

CapClientBool capClientGetCigFromCigId(CsrCmnListElm_t* elem, void* value)
{
    uint8 cigId = *(uint8*)value;
    CapClientCigElem* cigElem = (CapClientCigElem*)elem;
    return (cigElem->cigId == cigId) ? TRUE : FALSE;
}

void capClientRemoveTaskFromList(CsrCmnList_t *list, AppTask appTask)
{
    CapClientProfileTaskListElem* lsElem = 
          CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(list, appTask);

    if (lsElem == NULL)
        return;

    CsrCmnListElementRemove(list, (CsrCmnListElm_t*)lsElem);
}

CapClientBool capClientIsGroupCoordinatedSet(CapClientGroupInstance* cap)
{
    return (cap->currentDeviceCount > 1);
}

void capClientGetAllProfileInstanceStatus(uint8 deviceCount,
                               CsrCmnListElm_t *elem,
                               CapClientDeviceStatus **status,
                               CapClientProfile profile)
{
    CSR_UNUSED(deviceCount);
    uint8 index;
    CAP_CLIENT_INFO("\n(CAP) capClientGetAllProfileInstanceStatus: Profile is 0x%x!! \n", profile);

    if (elem == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP) capClientGetAllProfileInstanceStatus: Elem is NULL!! \n");
        return;
    }


    if (*status == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP) capClientGetAllProfileInstanceStatus: Status is NULL!! \n");
        return;
    }

    for(index = 0; elem; elem = elem->next, index++)
    {
        if(profile == CAP_CLIENT_CSIP)
        {
            CsipInstElement *cElem = (CsipInstElement*)elem;
            status[index]->cid = cElem->cid;
            status[index]->result = cElem->recentStatus;

            CAP_CLIENT_INFO("\n(CAP) capClientGetAllProfileInstanceStatus: CSIP Status: 0x%x, BtConnId: 0x%x!! \n", (*status[index]).result ,(*status[index]).cid);
        }
        else if(profile == CAP_CLIENT_BAP)
        {
            BapInstElement *bElem = (BapInstElement*)elem;
            status[index]->cid = bElem->bapHandle;
            status[index]->result = bElem->recentStatus;

            CAP_CLIENT_INFO("\n(CAP) capClientGetAllProfileInstanceStatus: BAP Status: 0x%x, BtConnId: 0x%x!! \n", (*status[index]).result, (*status[index]).cid);
        }
        else if(profile == CAP_CLIENT_VCP)
        {
            VcpInstElement *vElem = (VcpInstElement*)elem;
            status[index]->cid = vElem->cid;
            status[index]->result = vElem->recentStatus;

            CAP_CLIENT_INFO("\n(CAP) capClientGetAllProfileInstanceStatus: VCP Status: 0x%x, BtConnId: 0x%x!! \n", (*status[index]).result, (*status[index]).cid);
        }
        else
        {
            CsrPmemFree(*status);
            *status = NULL;
            break;
        }
    }
}

CapClientResult capClientGetCapClientResult(uint16 result, CapClientProfile  profile)
{
    CapClientResult res = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;

    if(result == CSR_BT_GATT_RESULT_SUCCESS)
    {
        res = CAP_CLIENT_RESULT_SUCCESS;
    }
    else
    {
        switch(profile)
        {
            case CAP_CLIENT_BAP:
                res = CAP_CLIENT_RESULT_FAILURE_BAP_ERR;
                break;
            case CAP_CLIENT_CSIP:
                res = CAP_CLIENT_RESULT_FAILURE_CSIP_ERR;
                break;
            case CAP_CLIENT_VCP:
                res = CAP_CLIENT_RESULT_FAILURE_VCP_ERR;
                break;
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
            case CAP_CLIENT_MICP:
                res = CAP_CLIENT_RESULT_FAILURE_MICP_ERR;
                break;
#endif
            case CAP_CLIENT_SRVC_DSC:
                res = CAP_CLIENT_RESULT_FAILURE_DICOVERY_ERR;
                break;
            default:
                /* No other Profile Supported */
                break;
        }
    }
    return res;
}

uint32 capClientGetAudioLocationFromBitMask(uint32 *configuredLocation,
                                            CapClientCigConfigMode mode,
                                            uint8 channelCount)
{
    uint8 index = 0;
    uint32 audioLocation = 0;
    uint32 seed = 1;
    uint32 locationData = *configuredLocation;

    /* If Joint Stereo then Set bitMask on all the location */

    for (index = 0; index < 32; index++)
    {
        if (((mode & CAP_CLIENT_MODE_JOINT_STEREO)
                     == CAP_CLIENT_MODE_JOINT_STEREO) && 
                       capClientIsChannelCountSupported(channelCount, BAP_AUDIO_CHANNELS_2))
        {
            seed = 0xffffffff;
            audioLocation = locationData & seed;
            break;
        }

        audioLocation = seed << index;

        if ((locationData & audioLocation) == audioLocation)
        {
            locationData &= ~audioLocation;
            break;
        }
    }

    *configuredLocation = locationData;

    return audioLocation;
}

CapClientBool capClientIsConfigSupportedByServer(CapClientGroupInstance* cap,
                                    CapClientSreamCapability config,
                                    CapClientContext useCase,
                                    uint8 direction,
                                    uint8 *versionNum)
{
    CapClientLocalStreamCapabilities* records;
    uint8 recCount = 0;
    bool result = FALSE;
    uint8 i = 0;
    bool isSink = (direction == BAP_ASE_SINK);
    useCase = capClientMapCapContextWithCap(useCase);

    records = capClientGetRemotePacRecord(cap, &recCount, isSink, useCase);

    if (recCount == 0)
    {
        CsrPmemFree(records);
        records = NULL;
        return FALSE;
    }

    if (isConfigSupported(records, recCount, config, versionNum))
    {
        result = TRUE;
    }

    for(i = 0; i < recCount; i ++)
    {
        if (records[i].metadataLen)
           CsrPmemFree(records[i].metadata);
    }

    CsrPmemFree(records);
    records = NULL;

    return result;
}

CapClientContext capClientMapCapContextWithCap(CapClientContext useCase)
{
    CapClientContext result = useCase;

    if (useCase == CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC)
        result = CAP_CLIENT_CONTEXT_TYPE_GAME;

    return result;
}

CapClientBool capClientIsContextAvailable(CapClientContext useCase,
                             CapClientGroupInstance* cap,
                             CapClientBool isSink)
{
    BapInstElement* bap = (BapInstElement*)cap->bapList.first;
    CapClientBool result = FALSE;
    uint8 offset = isSink ? CAP_CLIENT_SHIFT_OFFSET_NONE : CAP_CLIENT_SHIFT_OFFSET_WORD;
    CapClientContext context ;

    while (bap)
    {
        context  = (CapClientContext)(((bap->availableAudioContext) >> offset) & 0x0000ffff);
        useCase = capClientMapCapContextWithCap(useCase);

        if ((useCase & context) == useCase)
        {
            result = TRUE;
            break;
        }

        bap = bap->next;
    }

    return result;
}

/* CAP state transitions validations for Unicast*/

CapClientResult capClientValidateCapState(CapClientState state, CapClientPrim operation)
{
    CapClientResult result = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;

    switch (state)
    {
        case CAP_CLIENT_STATE_INIT:
        {
            switch (operation)
            {
                 case CAP_CLIENT_INTERNAL_INIT_REQ:
                 case CAP_CLIENT_INTERNAL_ADD_NEW_DEV_REQ:
                 case CAP_CLIENT_INTERNAL_INIT_STREAM_CONTROL_REQ:
                 case CAP_CLIENT_INTERNAL_CSIP_READ_REQ:
                 {
                     result = CAP_CLIENT_RESULT_SUCCESS;
                 }
                 break;

                 case CAP_CLIENT_INTERNAL_REMOVE_DEV_REQ:
                 {
                     result = CAP_CLIENT_RESULT_PROFILES_NOT_INITIALIZED;
                 }
                 break;

                 default:
                 {
                     result = CAP_CLIENT_RESULT_INVALID_OPERATION;
                 }
                 break;
            }
        }
        break;

        case CAP_CLIENT_STATE_INIT_STREAM_CTRL:
        {
            switch (operation)
            {
                 case CAP_CLIENT_INTERNAL_INIT_REQ:
                 case CAP_CLIENT_INTERNAL_REMOVE_DEV_REQ:
                 case CAP_CLIENT_INTERNAL_DISCOVER_STREAM_CAP_REQ:
                 case CAP_CLIENT_INTERNAL_DISCOVER_AVAIL_AUDIO_CONTEXT_REQ:
                 case CAP_CLIENT_INTERNAL_CSIP_READ_REQ:
                 case CAP_CLIENT_INTERNAL_READ_VOLUME_STATE_REQ:
                 case CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ:
                 case CAP_CLIENT_INTERNAL_MUTE_REQ:
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
                 case CAP_CLIENT_INTERNAL_SET_MIC_STATE_REQ:
                 case CAP_CLIENT_INTERNAL_READ_MIC_STATE_REQ:
#endif
                 {
                     result = CAP_CLIENT_RESULT_SUCCESS;
                 }
                 break;

                 case CAP_CLIENT_INTERNAL_UNICAST_CONNECT_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_START_STREAM_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_STOP_STREAM_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_UPDATE_AUDIO_REQ:
                 {
                     result = CAP_CLIENT_RESULT_CAPABILITIES_NOT_DISCOVERED;
                 }
                 break;

                 default:
                 {
                     result = CAP_CLIENT_RESULT_INVALID_OPERATION;
                 }
                 break;
            }
        }
        break;
        case CAP_CLIENT_STATE_DISCOVER_SUPPORTED_CAP:
        {
            switch (operation)
            {
                 case CAP_CLIENT_INTERNAL_INIT_REQ:
                 case CAP_CLIENT_INTERNAL_REMOVE_DEV_REQ:
                 case CAP_CLIENT_INTERNAL_DISCOVER_STREAM_CAP_REQ:
                 case CAP_CLIENT_INTERNAL_DISCOVER_AVAIL_AUDIO_CONTEXT_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_CONNECT_REQ:
                 case CAP_CLIENT_INTERNAL_CSIP_READ_REQ:
                 case CAP_CLIENT_INTERNAL_READ_VOLUME_STATE_REQ:
                 case CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ:
                 case CAP_CLIENT_INTERNAL_MUTE_REQ:
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
                 case CAP_CLIENT_INTERNAL_SET_MIC_STATE_REQ:
                 case CAP_CLIENT_INTERNAL_READ_MIC_STATE_REQ:
#endif
                 {
                     result = CAP_CLIENT_RESULT_SUCCESS;
                 }
                 break;

                 case CAP_CLIENT_INTERNAL_UNICAST_START_STREAM_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_STOP_STREAM_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_UPDATE_AUDIO_REQ:
                 {
                     result = CAP_CLIENT_RESULT_NOT_CONFIGURED;
                 }
                 break;

                 default:
                 {
                     result = CAP_CLIENT_RESULT_INVALID_OPERATION;
                 }
                 break;
            }
        }
        break;

        case CAP_CLIENT_STATE_UNICAST_CONNECTED:
        {
            switch (operation)
            {
                 case CAP_CLIENT_INTERNAL_REMOVE_DEV_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_START_STREAM_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_STOP_STREAM_REQ:
                 case CAP_CLIENT_INTERNAL_CSIP_READ_REQ:
                 case CAP_CLIENT_INTERNAL_READ_VOLUME_STATE_REQ:
                 case CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ:
                 case CAP_CLIENT_INTERNAL_MUTE_REQ:
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
                 case CAP_CLIENT_INTERNAL_READ_MIC_STATE_REQ:
                 case CAP_CLIENT_INTERNAL_SET_MIC_STATE_REQ:
#endif
                 case CAP_CLIENT_INTERNAL_UNICAST_CIG_TEST_CONFIG_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_CONNECT_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_DISCONNECT_REQ:
                 {
                     result = CAP_CLIENT_RESULT_SUCCESS;
                 }
                 break;

                 default:
                 {
                     result = CAP_CLIENT_RESULT_INVALID_OPERATION;
                 }
                 break;
            }
        }
        break;

        case CAP_CLIENT_STATE_STREAM_STARTED:
        {
            switch (operation)
            {
                 case CAP_CLIENT_INTERNAL_UNICAST_STOP_STREAM_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_UPDATE_AUDIO_REQ:
                 case CAP_CLIENT_INTERNAL_DISCOVER_AVAIL_AUDIO_CONTEXT_REQ:
                 case CAP_CLIENT_INTERNAL_CSIP_READ_REQ:
                 case CAP_CLIENT_INTERNAL_READ_VOLUME_STATE_REQ:
                 case CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ:
                 case CAP_CLIENT_INTERNAL_MUTE_REQ:
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
                 case CAP_CLIENT_INTERNAL_SET_MIC_STATE_REQ:
                 case CAP_CLIENT_INTERNAL_READ_MIC_STATE_REQ:
#endif
                 {
                     result = CAP_CLIENT_RESULT_SUCCESS;
                 }
                 break;

                 case CAP_CLIENT_INTERNAL_UNICAST_CONNECT_REQ: 
                 case CAP_CLIENT_INTERNAL_UNICAST_START_STREAM_REQ:
                 {
                     result = CAP_CLIENT_RESULT_STREAM_ALREADY_ACTIVATED;
                 }
                 break;

                 default:
                 {
                     result = CAP_CLIENT_RESULT_INVALID_OPERATION;
                 }
                 break;
            }
        }
        break;

        case CAP_CLIENT_STATE_STREAM_STOPPED:
        {
            switch (operation)
            {
                 case CAP_CLIENT_INTERNAL_INIT_REQ:
                 case CAP_CLIENT_INTERNAL_REMOVE_DEV_REQ:
                 case CAP_CLIENT_INTERNAL_DISCOVER_STREAM_CAP_REQ:
                 case CAP_CLIENT_INTERNAL_DISCOVER_AVAIL_AUDIO_CONTEXT_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_CONNECT_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_DISCONNECT_REQ:
                 case CAP_CLIENT_INTERNAL_CSIP_READ_REQ:
                 case CAP_CLIENT_INTERNAL_READ_VOLUME_STATE_REQ:
                 case CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_START_STREAM_REQ:
                 case CAP_CLIENT_INTERNAL_MUTE_REQ:
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
                 case CAP_CLIENT_INTERNAL_SET_MIC_STATE_REQ:
                 case CAP_CLIENT_INTERNAL_READ_MIC_STATE_REQ:
#endif
                 {
                     result = CAP_CLIENT_RESULT_SUCCESS;
                 }
                 break;

                 case CAP_CLIENT_INTERNAL_UNICAST_STOP_STREAM_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_UPDATE_AUDIO_REQ:
                 {
                     result = CAP_CLIENT_RESULT_NOT_CONFIGURED;
                 }
                 break;

                 default:
                 {
                     result = CAP_CLIENT_RESULT_INVALID_OPERATION;
                 }
                 break;
            }
        }
        break;

        case CAP_CLIENT_STATE_AUDIO_UPDATED:
        {
            switch (operation)
            {
                 case CAP_CLIENT_INTERNAL_REMOVE_DEV_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_UPDATE_AUDIO_REQ:
                 case CAP_CLIENT_INTERNAL_UNICAST_STOP_STREAM_REQ:
                 case CAP_CLIENT_INTERNAL_READ_VOLUME_STATE_REQ:
                 case CAP_CLIENT_INTERNAL_CSIP_READ_REQ:
                 case CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ:
                 case CAP_CLIENT_INTERNAL_MUTE_REQ:
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
                 case CAP_CLIENT_INTERNAL_SET_MIC_STATE_REQ:
                 case CAP_CLIENT_INTERNAL_READ_MIC_STATE_REQ:
#endif
                 {
                     result = CAP_CLIENT_RESULT_SUCCESS;
                 }
                 break;

                 default:
                 {
                     result = CAP_CLIENT_RESULT_INVALID_OPERATION;
                 }
                 break;
            }
        }
        break;

        default:
            break;
    }

    return result;
}

uint8 capClientGetAseCountForUseCase(BapInstElement* bap, CapClientContext useCase)
{
    uint8 count = 0;
    BapAseElement* ase = (BapAseElement*)bap->sinkAseList.first;

    while (ase)
    {
        if (ase->inUse && ase->useCase == useCase)
            count++;
        ase = ase->next;
    }

    ase = (BapAseElement*)bap->sourceAseList.first;

    while (ase)
    {
        if (ase->inUse && ase->useCase == useCase)
            count++;
        ase = ase->next;
    }
    return count;
}

uint8 capClientGetSrcAseCountForUseCase(BapInstElement* bap, CapClientContext useCase)
{
    uint8 count = 0;
    BapAseElement* ase = (BapAseElement*)bap->sourceAseList.first;

    while (ase)
    {
        if (ase->inUse && ase->useCase == useCase)
            count++;
        ase = ase->next;
    }
    return count;
}

uint8 capClientGetSinkAseCountForUseCase(BapInstElement* bap, CapClientContext useCase)
{
    uint8 count = 0;
    BapAseElement* ase = (BapAseElement*)bap->sinkAseList.first;

    while (ase)
    {
        if (ase->inUse && ase->useCase == useCase)
            count++;
        ase = ase->next;
    }
    return count;
}

/* Wrapper function for getting the profiles handle information from the respective profiles */
void* capClientGetHandlesInfo(uint32 cid, CapClientGroupInstance *cap, uint8 clntProfile)
{
    if (clntProfile == CAP_CLIENT_PROFILE_PACS || clntProfile == CAP_CLIENT_PROFILE_ASCS || clntProfile == CAP_CLIENT_PROFILE_BASS)
    {
         BapInstElement *bapInst =
                  (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cid);
         if (bapInst)
         {
             return BapGetAttributeHandles(bapInst->bapHandle, clntProfile);
         }
    }
    else if (clntProfile == CAP_CLIENT_PROFILE_VCP)
    {
        VcpInstElement *vcpInst = (VcpInstElement*)CAP_CLIENT_GET_VCP_ELEM_FROM_CID(cap->vcpList, cid);
        if (vcpInst)
        {
            return (void *)VcpGetAttributeHandles(vcpInst->vcpHandle);
        }
    }
    else if (clntProfile == CAP_CLIENT_PROFILE_CSIP)
    {
       CsipInstElement *csipInst = (CsipInstElement*)CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(cap->csipList, cid);
       if (csipInst)
       {
           return (void *)CsipGetAttributeHandles(csipInst->csipHandle);
       }
    }

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    else if (CAP_CLIENT_PROFILE_MICP)
    {
       MicpInstElement *micpInst = (MicpInstElement*)CAP_CLIENT_GET_MICP_ELEM_FROM_CID(cap->micpList, cid);
       if (micpInst)
       {
           return (void *)MicpGetAttributeHandles(micpInst->micpHandle);
       }
    }
#endif

    CAP_CLIENT_ERROR("CapClientGetHandlesInfo: Invalid Profile!!");
    return NULL;
}

CapClientProfileMsgQueueElem* CapClientMsgQueueAdd(CsrCmnList_t *list,
                                                  void* req,
                                                  int indCount,
                                                  CapClientPrim type,
                                                  CapClientMessageHandler handlerFunc,
                                                  CapClientProfileTaskListElem *task)
{
    CapClientProfileMsgQueueElem* msgElem = NULL;

    msgElem = (CapClientProfileMsgQueueElem*)CAP_CLIENT_MSG_QUEUE_ADD(list);
    msgElem->task = task;
    msgElem->capMsg = req;
    msgElem->MessageType = type;
    msgElem->confirmationRecieved = FALSE;
    msgElem->ExpectedIndCount = indCount;
    msgElem->handlerFunc = handlerFunc;

    return msgElem;
}

CapClientBool capClientAsesReleaseComplete(CapClientGroupInstance* cap)
{
    BapInstElement* bap = (BapInstElement*)cap->bapList.first;
    CapClientBool releaseComplete = TRUE;

    while (bap)
    {
        bap->asesInUse = capClientGetAseCountForUseCase(bap, cap->useCase);

        if (bap->asesInUse != bap->releasedAses)
        {
            releaseComplete = FALSE;
            break;
        }
        bap = bap->next;
    }

    return releaseComplete;
}

uint8 capClientGetSetBitCount(uint32 num)
{
    uint8 count = 0;
    while (num)
    {
        num &= (num - 1);
        count++;
    }

    return count;
}

CapClientCisDirection capClientGetCisDirectionFromConfig(CapClientSreamCapability sinkConfig,
                                                        CapClientSreamCapability srcConfig)
{
    CapClientCisDirection direction = CAP_CLIENT_CIS_DIR_INVALID;

    if (sinkConfig != CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN)
    {
        direction |= CAP_CLIENT_CIS_SINK_UNIDIRECTIONAL;
    }

    if (srcConfig != CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN)
    {
        direction |= CAP_CLIENT_CIS_SRC_UNIDIRECTIONAL;
    }

    return direction;
}

void capClientClearCounters(CAP_INST* inst)
{
    inst->bapRequestCount = 0;
    inst->csipRequestCount = 0;
    inst->vcpRequestCount = 0;
    inst->discoveryRequestCount = 0;
}

uint8 capClientGetCisCountPerBap(BapInstElement* bap, CapClientContext useCase)
{
    uint8 sinkCis = 0, srcCis = 0, cisCount = 0;
    sinkCis = capClientGetSinkAseCountForUseCase(bap, useCase);
    srcCis = capClientGetSrcAseCountForUseCase(bap, useCase);

    cisCount = CAP_CLIENT_GET_MAX(sinkCis, srcCis);
    return cisCount;
}

uint8 capClientGetTotalCisCount(BapInstElement* bap,
                               CapClientContext useCase,
                               CapClientCisDirection cisDir)
{
    uint8 cisCount = 0;
    CSR_UNUSED(cisDir);

    while (bap)
    {
        cisCount += capClientGetCisCountPerBap(bap, useCase);
        bap = bap->next;
    }

    return cisCount;
}

void capClientSendInternalPendingOpReq(CapClientPendingOp op)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInternalPendingOpReq);
    message->pendingOp = op;
    CapClientMessageSend(CSR_BT_CAP_CLIENT_IFACEQUEUE, CAP_CLIENT_INTERNAL_PENDING_OP_REQ, message);
}

void setBapStatePerCigId(BapInstElement * bap, uint32 state, uint8 cigId)
{
    if (cigId == CAP_CLIENT_INVALID_USECASE)
    {
        bap->bapCurrentState = 0;
        bap->bapCurrentState = (bap->bapCurrentState & CAP_CLIENT_MASK_BYTE0) | state;
        bap->bapCurrentState = (bap->bapCurrentState & CAP_CLIENT_MASK_BYTE1) | (state << 8);
        bap->bapCurrentState = (bap->bapCurrentState & CAP_CLIENT_MASK_BYTE2) | (state << 16);
        bap->bapCurrentState = (bap->bapCurrentState & CAP_CLIENT_MASK_BYTE3) | (state << 24);
    }
    else
    {
        uint32 byteMask = CAP_CLIENT_USE_CASE_MASK;

        if (cigId == 0)
            byteMask = CAP_CLIENT_MASK_BYTE0;
        else if (cigId == 1)
            byteMask = CAP_CLIENT_MASK_BYTE1;
        else if (cigId == 2)
            byteMask = CAP_CLIENT_MASK_BYTE2;
        else if (cigId == 3)
            byteMask = CAP_CLIENT_MASK_BYTE3;

        bap->bapCurrentState = bap->bapCurrentState & byteMask;
        bap->bapCurrentState = bap->bapCurrentState | state << (8 *cigId);
    }
}

#ifdef CAP_CLIENT_NTF_TIMER
void capClientNtfTimerSet(CAP_INST *inst, CapClientGroupInstance* cap, BapInstElement *bap, uint8 capAseState)
{
    CAP_CLIENT_INFO("\n(CAP): capClientNtfTimerSet  timer for %d\n", capAseState);
    cap->capNtfTimeOut = FALSE;
    bap->ntfTimer = CsrSchedTimerSet(CAP_CLIENT_NOTIFICATION_TIMER*CSR_SCHED_SECOND,
    capclientTimerExpiryHandler,
    capAseState,
    inst);
}

void capClientNtfTimerReset(BapInstElement *bap)
{
    /* First check if timer is running and then cancel */
    if (bap->ntfTimer)
    {
        CsrSchedTimerCancel(bap->ntfTimer, NULL, NULL);
        bap->ntfTimer = (CsrSchedTid) CSR_SCHED_TID_INVALID;
        CAP_CLIENT_INFO("\n(CAP): capClientNtfTimerReset timer \n");
    }
}

void capclientTimerExpiryHandler(CsrUint16 capAseState, void* data)
{
    CAP_INST* inst= (CAP_INST*)data;
    CapClientGroupInstance *cap  = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    CAP_CLIENT_INFO("\n(CAP): capclientTimerExpiryHandler timer %d\n", capAseState);

    switch (capAseState)
    {
         case BAP_ASE_STATE_CODEC_CONFIGURED:
         case BAP_ASE_STATE_QOS_CONFIGURED:
         {
             capClientSendUnicastClientConnectCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_TIMEOUT_MISSING_NTF_FROM_REMOTE);
             break;
         }

         case BAP_ASE_STATE_ENABLING:
         case BAP_ASE_STATE_STREAMING:
         {
             capClientSendUnicastStartStreamCfm(inst->profileTask, inst, cap, CAP_CLIENT_RESULT_TIMEOUT_MISSING_NTF_FROM_REMOTE);
             break;
         }

         case BAP_ASE_STATE_DISABLING:
         case BAP_ASE_STATE_RELEASING:
         {
             capClientSendUnicastStopStreamCfm(inst->profileTask, inst, CAP_CLIENT_RESULT_TIMEOUT_MISSING_NTF_FROM_REMOTE,
                     TRUE, NULL, cap);
             break;
         }

         default:
            break;
    }

    if (cap)
    {
        /* Set the ntf flag to TRUE so that we should not send multiple confirmation to the upper layer 
         * if the rest of the notification is coming after timeout cap should avoid sending one more confrimation for 
         * the respective ASE operation.*/
        cap->capNtfTimeOut = TRUE;
        /* This need to verfiy/confirm for split cig case(either we need to reset the counter or decrement) */
        inst->bapRequestCount--;
    }
}
#endif
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_SOURCE
CapClientBool capClientGetBcastSrcFromBigId(CsrCmnListElm_t* elem, void* value)
{
    uint8 bigId = *(uint8*)value;
    BroadcastSrcInst* bcastSrc = (BroadcastSrcInst*)elem;
    return (bcastSrc->bigId == bigId) ? TRUE : FALSE;
}

CapClientBool capClientGetBcastSrcFromPhandle(CsrCmnListElm_t* elem, void* value)
{
    uint32 pHandle = *(uint32*)value;
    BroadcastSrcInst* bcastSrc = (BroadcastSrcInst*)elem;
    return (bcastSrc->bcastSrcProfileHandle == pHandle) ? TRUE : FALSE;
}

CapClientBool capClientGetBcastSrcFromAppTask(CsrCmnListElm_t* elem, void* value)
{
    AppTask appTask = *(AppTask*)value;
    BroadcastSrcInst* bcastSrc = (BroadcastSrcInst*)elem;
    return (bcastSrc->appTask == appTask) ? TRUE : FALSE;
}

void capClientDecrementOpCounter(CapClientGenericCounter *counter)
{
    CAP_CLIENT_INFO("\n(CAP) CapClientDecrementOpCounter : Counter Value :%d!!", counter->opReqCount);
    /* Decrement if the counter is proper */

    if ((counter->opReqCount > 0) 
        && !(counter->opReqCount & 0x80))
    {
        counter->opReqCount -= 1;
    }
    else
    {
        CAP_CLIENT_ERROR("\n(CAP) CapClientDecrementOpCounter : Counter underFlow detected!!");
        /* Reset Counter */
        counter->opReqCount = 0;
    }
}
#endif

#if defined(INSTALL_LEA_BROADCAST_SOURCE) || defined(INSTALL_LEA_BROADCAST_ASSISTANT)
void capClientIncrementOpCounter(CapClientGenericCounter* counter)
{
    CAP_CLIENT_INFO("\n(CAP) CapClientIncrementOpCounter : Counter Value :%d!!", counter->opReqCount);

    /* Increment if the counter is proper */

    if (!(counter->opReqCount & 0x80))
    {
        counter->opReqCount += 1;
    }
    else
    {
        CAP_CLIENT_ERROR("\n(CAP) CapClientDecrementOpCounter : Counter Overflow detected!!");
        /* Over ride the overflow value*/
        counter->opReqCount = 1;
    }
}
#endif

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
void capClientResetOpCounter(CapClientGroupInstance *cap)
{
    BapInstElement* bap = (BapInstElement*)cap->bapList.first;

    /* Increment if the counter is proper */
    while (bap)
    {
        CAP_CLIENT_INFO("\n(CAP) CapClientIncrementOpCounter : Err IV, Success IV, Req IV :%d, %d, %d!!",
                                                                           bap->operationCounter.errorCount, 
                                                                             bap->operationCounter.successCount,
                                                                                  bap->operationCounter.opReqCount);

        bap->operationCounter.errorCount = 0;
        bap->operationCounter.successCount = 0;
        bap->operationCounter.opReqCount = 0;


        bap = bap->next;
    }
}

void capClientIncrementSuccessCounter(CapClientGenericCounter* counter)
{
    CAP_CLIENT_INFO("\n(CAP) CapClientIncrementOpCounter : Counter Value :%d!!", counter->successCount);

    /* Increment if the counter is proper */

    if (!(counter->successCount & 0x80))
    {
        counter->successCount += 1;
    }
    else
    {
        CAP_CLIENT_ERROR("\n(CAP) CapClientDecrementOpCounter : Counter Overflow detected!!");
        /* Over ride the overflow value*/
        counter->successCount = 1;
    }
}

CapClientBcastAssistState capClientBcastAsistantGetState(CapClientGroupInstance *cap)
{
    return cap->bcastAsstState;
}

void capClientBcastAsistantSetState(CapClientGroupInstance *cap,
    CapClientBcastAssistState state)
{
    cap->bcastAsstState = state;
}

void capClientIncrementErrorCounter(CapClientGenericCounter* counter)
{
    CAP_CLIENT_INFO("\n(CAP) CapClientIncrementOpCounter : Counter Value :%d!!", counter->errorCount);

    /* Increment if the counter is proper */

    if (!(counter->errorCount & 0x80))
    {
        counter->errorCount += 1;
    }
    else
    {
        CAP_CLIENT_ERROR("\n(CAP) CapClientDecrementOpCounter : Counter Overflow detected!!");
        /* Over ride the overflow value*/
        counter->errorCount = 1;
    }
}

CapClientBool capClientIsGroupIntsanceLocked(CapClientGroupInstance* cap)
{
    CsipInstElement* csip = (CsipInstElement*)cap->csipList.first;
    CapClientBool locked = TRUE;

    while (csip)
    {
        if (csip->lock == CAP_CLIENT_LOCK_STATE_DISABLED)
            locked &= FALSE;

        csip = csip->next;
    }

    return locked;
}

CapClientBool capClientBcastAsstOpComplete(CapClientGroupInstance* cap)
{
    CapClientBool result = TRUE;
    BapInstElement* bap = (BapInstElement*)cap->bapList.first;

    while (bap)
    {
        if (bap->operationCounter.opReqCount !=
            (bap->operationCounter.successCount +
                bap->operationCounter.errorCount))

            result &= FALSE;
        bap = bap->next;
    }

    return result;
}

void capClientBcastAsstResetState(CapClientGroupInstance* cap,
                                  CapClientBcastAssistState state)
{
    capClientResetOpCounter(cap);
    capClientBcastAsistantSetState(cap, state);
    CAP_CLIENT_CLEAR_PENDING_OP(cap->pendingOp);
}

void capClientFreeSourceParamsContent(void)
{
    uint8 i;

    for (i = 0; i < sourceParams.numbSubGroups; i++)
    {
        /* Free the subgroupinfo metadata value from previous call */
        if (sourceParams.subgroupInfo[i].metadataLen && 
            sourceParams.subgroupInfo[i].metadataValue)
        {
            CsrPmemFree(sourceParams.subgroupInfo[i].metadataValue);
            sourceParams.subgroupInfo[i].metadataValue = NULL;
        }
    }
    sourceParams.numbSubGroups = 0;

    for (i = 0; i < sourceParams.infoCount; i++)
    {
        if (sourceParams.infoCount && sourceParams.info)
        {
            CsrPmemFree(sourceParams.info);
            sourceParams.info = NULL;
        }
    }
    sourceParams.infoCount = 0;
}
#endif

CapClientResult capClientBroadcastSourceGetResultCode(BapResult result)
{
    uint8 code;

    switch (result)
    {
        case BAP_RESULT_SUCCESS:
        {
            code = CAP_CLIENT_RESULT_SUCCESS;
        }
        break;

        case BAP_RESULT_INPROGRESS:
        {
            code = CAP_CLIENT_RESULT_INPROGRESS;
        }
        break;

        case BAP_RESULT_INVALID_OPERATION:
        {
            code = CAP_CLIENT_RESULT_INVALID_OPERATION;
        }
        break;

        case BAP_RESULT_UNSUPPORTED_CAPABILITY:
        {
            code = CAP_CLIENT_RESULT_UNSUPPORTED_CAPABILITY;
        }
        break;

        case BAP_RESULT_NOT_SUPPORTED:
        {
            code = CAP_CLIENT_RESULT_NOT_SUPPORTED;
        }
        break;

        case BAP_RESULT_REJECTED_PARAMETER:
        {
            code = CAP_CLIENT_RESULT_INVALID_PARAMETER;
        }
        break;

        case BAP_RESULT_INSUFFICIENT_RESOURCES:
        {
            code = CAP_CLIENT_RESULT_INSUFFICIENT_RESOURCES;
        }
        break;

        default:
        {
            CAP_CLIENT_INFO("capClientBroadcastSourceGetResultCode result : %x\n", result);
            code = CAP_CLIENT_RESULT_FAILURE_BAP_ERR;
        }
        break;
    }

    return code;
}

uint8 capClientNumOfBitSet(uint32 num)
{
    uint8 count = 0;

    while(num)
    {
        num &= (num - 1);
        count++;
    }

    return count;
}

bool capClientUtilsFindLtvValue(uint8 * ltvData,
                          uint8 ltvDataLength,
                          uint8 type,
                          uint8 * value,
                          uint8 valueLength)
{
    bool ltvFound = FALSE;
    if(ltvData && ltvDataLength && value)
    {
        int ltvIndex = 0;
        while(ltvIndex < ltvDataLength && ltvFound == FALSE && ltvData[ltvIndex])
        {
            uint8 length = ltvData[ltvIndex];
            CAP_CLIENT_INFO("capClientUtilsFindLtvValue: index=%d length=%d type=%d",
                ltvIndex, ltvData[ltvIndex], ltvData[ltvIndex + 1]);

            if(ltvData[ltvIndex + 1] == type)
            {
                if(ltvData[ltvIndex] == (valueLength + 1))
                {
                    uint8 i;
                    for(i = 0; i < valueLength; i++)
                    {
                        value[i] = ltvData[ltvIndex + 2 + i];
                    }
                    ltvFound = TRUE;
                }
                else
                {
                    CAP_CLIENT_INFO("capClientUtilsFindLtvValue: Unexpected length");
                    break;
                }
            }
            else
            {
                ltvIndex += (1 + length);
            }
        }
    }
    else
    {
        CAP_CLIENT_INFO("capClientUtilsFindLtvValue: Invalid LTV data");
    }

    return ltvFound;
}
