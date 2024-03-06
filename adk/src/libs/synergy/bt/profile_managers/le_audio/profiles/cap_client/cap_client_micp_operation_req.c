/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #
******************************************************************************/
#include "cap_client_util.h"
#include "cap_client_common.h"
#include "cap_client_init_stream_and_control_req.h"
#include "cap_client_csip_handler.h"
#include "cap_client_micp_handler.h"
#include "cap_client_debug.h"
#include "cap_client_init_optional_service_req.h"
#include "cap_client_micp_operation_req.h"
#include "gatt_service_discovery_lib.h"

#ifndef EXCLUDE_CSR_BT_MICP_MODULE

static void capClientDiscoverMicpService(CAP_INST *inst, MicpInstElement* micp)
{
    /* Peform the MICP service disocvery for all the devices in the group */
    while(micp)
    {
        GattServiceDiscoveryFindServiceRange(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                                             micp->cid,
                                             GATT_SD_MICS_SRVC);
        inst->discoveryRequestCount++;
        micp = micp->next;
    }
}

static void capClientInitializeMicpInstance(MicpInstElement* micp)
{
    MicpInitData clientInitParams;
    MicpHandles deviceData;

    /*
     * NOTE: At Present We do not Support AICS and VOCS instances
     * */
    deviceData.aicsHandleLength = 0;
    deviceData.aicsHandle = NULL;
    clientInitParams.cid = micp->cid;

    if (micp->micsData)
    {
        CsrMemCpy(&deviceData.micsHandle, micp->micsData, sizeof(GattMicsClientDeviceData));
        MicpInitReq(CSR_BT_CAP_CLIENT_IFACEQUEUE, &clientInitParams, &deviceData, FALSE);
    }
    else
    {
        MicpInitReq(CSR_BT_CAP_CLIENT_IFACEQUEUE, &clientInitParams, NULL, FALSE);
    }

    micp->recentStatus = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;
}

static void capClientSendMicpReq(CapClientGroupInstance* cap,
                                 CAP_INST* inst,
                                 CapClientMicpReqSender reqSender)
{
    CsipInstElement* csip = NULL;

    if (capClientIsGroupCoordinatedSet(cap)
           && !capClientIsGroupIntsanceLocked(cap))
    {
        csip = (CsipInstElement*)(cap->csipList.first);
        capClientSetCsisLockState(csip, &inst->csipRequestCount, TRUE);
    }
    else
    {
        CAP_CLIENT_INFO("\n capClientSendMicpReq: Set Locked or Not Coordinated Set! \n");
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
        capClientMicpServiceReqSend(cap, inst, CAP_CLIENT_OPTIONAL_SERVICE_MICP, reqSender);
#endif
    }
}
                                 
static void capClientSendSetMicpAttbuteHandlesCfm(AppTask appTask, 
                                       ServiceHandle groupId,
                                       MicpInstElement *micp,
                                       CapClientResult result)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientSetMicpAttribHandlesCfm);
    message->groupId = groupId;
    message->result = result;
    message->cid = 0;

    if (result == CAP_CLIENT_RESULT_SUCCESS && micp)
    {
        message->cid = micp->cid;
    }

    CapClientMessageSend(appTask,
                       CAP_CLIENT_SET_MICP_ATTRIB_HANDLES_CFM,
                       message);
}

void InitMicpList(CsrCmnListElm_t* elem)
{
    /* Initialize a MicpInst list element. This function is called every
     * time a new entry is made on the MICP Inst Element list */

    MicpInstElement* micpElem = (MicpInstElement*)elem;

    micpElem->cid = CAP_CLIENT_INVALID_CID;
    micpElem->micpHandle = CAP_CLIENT_INVALID_SERVICE_HANDLE;
    micpElem->groupId = CAP_CLIENT_INVALID_SERVICE_HANDLE;
    micpElem->recentStatus = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;
    micpElem->micsData = NULL;
    micpElem->micValue = 0x0;
}

void deInitMicpList(CsrCmnListElm_t* elem)
{
    MicpInstElement* micp = (MicpInstElement*)elem;
    CsrPmemFree(micp->micsData);
    micp->micsData = NULL;
}

void capClientInitMicpServiceHandler(CAP_INST *inst)
{
    CapClientGroupInstance *gInst = CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    MicpInstElement* micp = (MicpInstElement*)(gInst->micpList->first);

    for(; micp; micp = micp->next)
    {
        CAP_CLIENT_INFO("\n capClientInitMicpServiceHandler btconnID: %x\n", micp->cid);

        /* Initialiaze only those instances where MICP discovery is suscessfull and micp handles are not yet disocvered 
         * From remote, second cehck is added for dynmic addition of the devices as CAP need to skip micp init for those 
         * Which is already disocvered/added in the group */
        if (micp->recentStatus == CAP_CLIENT_RESULT_SUCCESS && 
            micp->micpHandle == CAP_CLIENT_INVALID_SERVICE_HANDLE)
        {
            capClientInitializeMicpInstance(micp);
            inst->micpRequestCount++;
        }
    }
}

void capClientMicpHandler(CAP_INST *inst, GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *cfm, uint8 i)
{
    CapClientGroupInstance* gInst = NULL;
    MicpInstElement* micp = NULL;
    bool micpSuccess = FALSE;
    CSR_UNUSED(i);

    gInst = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    /* Track the status, if at least one of the devices in the group for which service discovery is sucessfull
     * Then continue the init procedure for that service */
    if (cfm->result == GATT_SD_RESULT_SUCCESS)
    {
        micp = (MicpInstElement*)CAP_CLIENT_GET_MICP_ELEM_FROM_CID(gInst->micpList, cfm->cid);
        micp->recentStatus = CAP_CLIENT_RESULT_SUCCESS;
    }

    if (inst->discoveryRequestCount == 0)
    {
        CAP_CLIENT_INFO("\n (CAP)handleGattSrvcDiscMsg: optional Service Disocvery completed: %x\n", cfm->srvcInfo->srvcId);

         micp = (MicpInstElement*)(gInst->micpList->first);

         /* Check if the discovery is completed and there is micp service found at least for one of the device in the group */
         while (micp)
         {
             if (micp->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
             {
                 micpSuccess = TRUE;
                 break;
             }
             micp = micp->next;
         }
         
         /* Call the handler for function for performing the Init as MICP is found in remote */
         if (micpSuccess)
         {
             capClientInitMicpServiceHandler(inst);
         }    
         else
         {
            CAP_CLIENT_INFO("\n (CAP)handleGattSrvcDiscMsg: optional Service Disocvery completed failed: %x\n", cfm->srvcInfo->srvcId);

            /* Send error confirmation here as no remote device is supporting micp in a group */
            capClientSendOptionalServiceInitCfm(inst->appTask,
                inst->deviceCount,
                inst->activeGroupId,
                CAP_CLIENT_OPTIONAL_SERVICE_MICP,
                CAP_CLIENT_RESULT_FAILURE_MICP_ERR,
                gInst->micpList->first);
        }

        if (cfm->srvcInfoCount && cfm->srvcInfo)
            CsrPmemFree(cfm->srvcInfo);
    }
}

void capCLientSendReadMicStateCfm(AppTask appTask, 
                                       ServiceHandle groupId,
                                       CapClientResult result,
                                       MicpInstElement *micp,
                                       uint8 muteValue)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientReadMicStateCfm);
    message->groupId = groupId;
    message->result = result;

    message->micState = 0;
    message->cid = 0;

    if (result == CAP_CLIENT_RESULT_SUCCESS && micp)
    {
        message->micState = muteValue;
        message->cid = micp->cid;
    }

    CapClientMessageSend(appTask,
                       CAP_CLIENT_READ_MIC_STATE_CFM,
                       message);
}

void capClientSendMicMuteCfm(AppTask appTask,
                   ServiceHandle groupId,
                   CapClientGroupInstance* cap,
                   uint32 cid,
                   CapClientResult result)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientSetMicStateCfm);
    message->groupId = groupId;
    message->result = result;
    message->statusLen = 0;
    message->status = NULL;

    CapClientGroupInstance* gInst = NULL;
    MicpInstElement* micp = NULL;

    gInst = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(groupId);
    if (gInst)
        CAP_CLIENT_CLEAR_PENDING_OP(gInst->pendingOp);

    if (result == CAP_CLIENT_RESULT_SUCCESS)
    {
        micp = (MicpInstElement*)cap->micpList->first;
        message->statusLen = cap->micpList->count;

        if (cid)
        {
            micp = (MicpInstElement*)
                CAP_CLIENT_GET_MICP_ELEM_FROM_CID(cap->micpList, cid);
            message->statusLen = 1;
        }

        if (message->statusLen)
            message->status = (CapClientDeviceStatus*)
                     CsrPmemZalloc(sizeof(CapClientDeviceStatus) * message->statusLen);

        if (micp && message->status)
        {
            uint8 i = 0;

            do
            {
                message->status[i].cid = micp->cid;
                message->status[i].result = micp->recentStatus;

                if (micp->recentStatus != CAP_CLIENT_RESULT_SUCCESS)
                    result = micp->recentStatus;

                micp = micp->next;
                i++;
            } while (micp && cid == 0);
        }
        else
        {
            CsrPmemFree(message->status);
            message->status = NULL;
            message->statusLen = 0;
            message->result = CAP_CLIENT_RESULT_FAILURE_MICP_ERR;
        }
    }

    CapClientMessageSend(appTask,
                      CAP_CLIENT_SET_MIC_STATE_CFM,
                      message);
}

void capClientMicMuteReqSend(MicpInstElement* micp,
                                    CAP_INST *inst,
                                    uint32 cid)
{
    do {
        inst->micpRequestCount++;
        CAP_CLIENT_INFO("\n capClientMicMuteReqSend: Counter :%d \n",
                                        inst->micpRequestCount);

        MicpSetMuteValueReq(micp->micpHandle, micp->micValue);

        micp = micp->next;
    } while (micp && (cid == 0));
}

void handleCapClientSetMicpProfileAttribHandlesReq(CAP_INST *inst, const Msg msg)
{
    CapClientGroupInstance *gInst = NULL;
    MicpInstElement* micp = NULL;
    CapClientInternalSetMicpAttribHandlesReq *req = (CapClientInternalSetMicpAttribHandlesReq*)msg;
    CapClientProfileTaskListElem* task = NULL;

    gInst = CAP_CLIENT_GET_GROUP_INST_DATA(req->groupId);

    if (gInst == NULL)
    {
        capClientSendSetMicpAttbuteHandlesCfm(req->profileTask,
                                       req->groupId,
                                       NULL,
                                       CAP_CLIENT_RESULT_INVALID_GROUPID);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/
    task = (CapClientProfileTaskListElem*)
        CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&gInst->profileTaskList, req->profileTask);

    /* Reject the api call if the call is not from registered app*/
    if (task == NULL)
    {
        capClientSendSetMicpAttbuteHandlesCfm(req->profileTask,
                                       inst->activeGroupId,
                                       NULL,
                                       CAP_CLIENT_RESULT_TASK_NOT_REGISTERED);
        return;
    }

    if (gInst->micpList == NULL)
    {
        gInst->micpList = (CsrCmnList_t*)CsrPmemZalloc(sizeof(CsrCmnList_t));

        /* Initialize the micp list */
        CsrCmnListInit(gInst->micpList, 0, InitMicpList, deInitMicpList);
    }

    capClientInitInitializeMicpProfile(gInst, req->micsHandles, req->cid);

    capClientSendSetMicpAttbuteHandlesCfm(req->profileTask,
                                       inst->activeGroupId,
                                       micp,
                                       CAP_CLIENT_RESULT_SUCCESS);

}

void handleCapClientSetMicStateReq(CAP_INST* inst, const Msg msg)
{
    MicpInstElement* micp =  NULL;
    CapClientInternalSetMicStateReq* req = (CapClientInternalSetMicStateReq*)msg;
    CapClientGroupInstance* cap = NULL;
    CapClientProfileTaskListElem* task = NULL;
    CapClientResult result = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (cap == NULL)
    {
        capClientSendMicMuteCfm(req->profileTask,
                       req->groupId,
                       cap,
                       req->cid,
                       CAP_CLIENT_RESULT_INVALID_GROUPID);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/
    task = (CapClientProfileTaskListElem*)
        CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

    /* Reject the api call if the call is not from registered app*/
    if (task == NULL)
    {
        capClientSendMicMuteCfm(req->profileTask,
                            req->groupId,
                            cap,
                            req->cid,
                            CAP_CLIENT_RESULT_TASK_NOT_REGISTERED);
        return;
    }

    result = capClientValidateCapState(cap->capState, req->type);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n (CAP) handleCapClientSetMicStateReq: Invalid state Trasition \n");
        capClientSendMicMuteCfm(req->profileTask,
                             req->groupId,
                             cap,
                             req->cid,
                             result);
        return;
    }

    if ((cap->role & CAP_CLIENT_COMMANDER) != CAP_CLIENT_COMMANDER)
    {
        CAP_CLIENT_INFO("\n(CAP) handleCapClientSetMicStateReq: Role Not Supported \n");
        result = CAP_CLIENT_RESULT_INVALID_ROLE;
        capClientSendMicMuteCfm(req->profileTask,
                            req->groupId,
                            cap,
                            req->cid,
                            result);
        return;
    }

    if (cap->pendingOp != CAP_CLIENT_OP_NONE)
    {
        result = CAP_CLIENT_RESULT_CAP_BUSY;
        CAP_CLIENT_INFO("\n handleCapClientSetMicStateReq: Invalid State\n");
        capClientSendMicMuteCfm(req->profileTask,
                            req->groupId,
                            cap,
                            req->cid,
                            result);
        return;
    }
    /* co ordinated set?
    *
    * Based on if co ordinated Set or not decide number of ASEs required
    * and then start BAP procedures
    *
    * */

    cap->requestCid = req->cid;
    inst->micpProfileTask = req->profileTask;

    if (req->cid)
    {
        micp = (MicpInstElement*)CAP_CLIENT_GET_MICP_ELEM_FROM_CID(cap->micpList, req->cid);

        if (micp == NULL)
        {
            result = CAP_CLIENT_RESULT_INVALID_PARAMETER;
            CAP_CLIENT_INFO("\n handleCapClientSetMicStateReq: UNKNOWN CID \n");
            capClientSendMicMuteCfm(req->profileTask,
                            req->groupId,
                            cap,
                            req->cid,
                            result);
            return;
        }
    }

    cap->pendingOp = CAP_CLIENT_MICP_MUTE;

    micp = (MicpInstElement*)(cap->micpList->first);

    /* Store the mic value which App has set */
    while( micp)
    {
        micp->micValue = req->micState;
        micp =micp->next;
    }

    capClientSendMicpReq(cap, inst, capClientMicMuteReqSend);

}

void handleCapClientReadMicStateReq(CAP_INST* inst, const Msg msg)
{
    CapClientInternalReadMicStateReq* req = (CapClientInternalReadMicStateReq*)msg;
    CapClientGroupInstance* cap = NULL;
    CapClientProfileTaskListElem* task = NULL;
    CapClientResult result = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;
    MicpInstElement* micp = NULL;

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (cap == NULL)
    {
        capCLientSendReadMicStateCfm(req->profileTask,
                                     req->groupId,
                                     CAP_CLIENT_RESULT_INVALID_GROUPID,
                                     NULL,
                                     0);
        return;
    }

    /* Reject the api call if the task is not found in the registered Task list*/
    task = (CapClientProfileTaskListElem*)
        CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, req->profileTask);

    if (task == NULL)
    {
        capCLientSendReadMicStateCfm(req->profileTask,
                                     req->groupId,
                                     CAP_CLIENT_RESULT_TASK_NOT_REGISTERED,
                                     NULL,
                                     0);
        return;
    }

    /* Reject the API call if the Stream and control init is not done*/
    result = capClientValidateCapState(cap->capState, req->type);

    if (result != CAP_CLIENT_RESULT_SUCCESS)
    {
        CAP_CLIENT_INFO("\n(CAP) handleCapClientReadMicStateReq: invalid state transition \n");
        capCLientSendReadMicStateCfm(req->profileTask,
                                     req->groupId,
                                     result,
                                     NULL,
                                     0);
        return;
    }

    if ((cap->role & CAP_CLIENT_COMMANDER) != CAP_CLIENT_COMMANDER)
    {
        CAP_CLIENT_INFO("\n(CAP) handleCapClientReadMicStateReq: Role Not Supported \n");
        result = CAP_CLIENT_RESULT_INVALID_ROLE;
        capCLientSendReadMicStateCfm(req->profileTask,
                                     req->groupId,
                                     result,
                                     NULL,
                                     0);

        return;
    }

    micp = (MicpInstElement*)CAP_CLIENT_GET_MICP_ELEM_FROM_CID(cap->micpList, req->cid);

    /* If MICP instance is null, just return sending NULL instance Error*/
    if (micp == NULL)
    {
        CAP_CLIENT_INFO("\n(CAP) handleCapClientReadMicStateReq: Micp instance NULL \n");
        result = CAP_CLIENT_RESULT_NULL_INSTANCE;
        capCLientSendReadMicStateCfm(req->profileTask,
                                     req->groupId,
                                     result,
                                     NULL,
                                     0);
        return;
    }

    inst->micpProfileTask = req->profileTask;

    MicpReadMuteValueReq(micp->micpHandle);
}

void capClientHandleMicpInit(CAP_INST *inst, CapClientGroupInstance* cap, CapClientInternalInitOptionalServicesReq *req)
{
    MicpInstElement* micp = NULL;
    VcpInstElement* vcp = NULL;

    /* Check for the service mask and perform the respective services init procedures 
     * Currently its for MICP and in future as new services will come need to extend */
    if ((req->servicesMask & CAP_CLIENT_OPTIONAL_SERVICE_MICP) == CAP_CLIENT_OPTIONAL_SERVICE_MICP)
    {
        vcp = (VcpInstElement*)((&cap->vcpList)->first);

        if (cap->micpList == NULL)
        {
            cap->micpList = (CsrCmnList_t*)CsrPmemZalloc(sizeof(CsrCmnList_t));

            /* Initialize the micp list */
            CsrCmnListInit(cap->micpList, 0, InitMicpList, deInitMicpList);
        }

        /* Traverse either bap/vcp list in cap and populate the micp list, vcp is choosen as vcp
         * is mandatory service and micp and vcp is having similar functionality */
        while (vcp)
        {
            GattMicsClientDeviceData* micsHandles = NULL;
            micp = (MicpInstElement*)CAP_CLIENT_GET_MICP_ELEM_FROM_CID((cap->micpList), vcp->cid);

            if (micp && micp->micsData)
            {
                micsHandles = micp->micsData;
            }

            capClientInitInitializeMicpProfile(cap, micsHandles, vcp->cid);
            vcp = vcp->next;
        }

        micp = (MicpInstElement*)cap->micpList->first;

        if (micp->micsData == NULL)
        {
            capClientDiscoverMicpService(inst, micp);
        }
        else
            capClientInitMicpServiceHandler(inst);
    }
}
#endif
