/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_util.h"
#include "cap_client_common.h"
#include "cap_client_init_stream_and_control_req.h"
#include "cap_client_debug.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
static void capClientInitializeBapInstance(CapClientGroupInstance* gInst,
                                    uint32 cid,
                                    BapHandles *handles,
                                    BapRole bapRole)
{
    BapInitData bapInitData;
    bapInitData.cid = cid;
    bapInitData.role = bapRole;


    /* If the role is just BAP_ROLE_BROADCAST_ASSISTANT
     * There is no need for ASCS handles only PACS and BASS Handles are
     * enough */

    bapInitData.handles = handles;

    if(bapRole ==  BAP_ROLE_BROADCAST_ASSISTANT && handles)
    {

        CsrPmemFree(bapInitData.handles->ascsHandles);
        bapInitData.handles->ascsHandles = NULL;
    }

    BapInitReq(gInst->libTask, bapInitData);

}

void capClientSendStreamControlInitCfm(CAP_INST *inst,
                                   bool vcpInitialised,
                                   CapClientResult result,
                                   CapClientRole role,
                                   CsrCmnListElm_t *elem)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInitStreamControlCfm);
    BapInstElement* bElem = NULL;
    VcpInstElement* vElem = NULL;
    CapClientGroupInstance* gInst = NULL;
    uint8 i = 0;
    CapClientProfileMsgQueueElem* msgElem = NULL;
    uint8 deviceCount = inst->deviceCount;

    message->groupId = inst->activeGroupId;
    message->deviceStatusLen = 0x00;
    message->result = result;
    message->role = role;

    /* Clear the pending operation flag as operation is done */

    gInst = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);
    if (gInst)
    {
        CAP_CLIENT_CLEAR_PENDING_OP(gInst->pendingOp);
    }

    CAP_CLIENT_INFO("\n(CAP) InitStreamControlCfm: Device Count: %d \n", deviceCount);

    if (elem)
    {
        message->deviceStatus = (CapClientDeviceStatus*)
               CsrPmemZalloc(deviceCount*sizeof(CapClientDeviceStatus));
        message->deviceStatusLen = deviceCount;
    }

    if (vcpInitialised)
    {
        vElem = (VcpInstElement*)elem;
    }
    else
    {
        bElem = (BapInstElement*)elem;
    }

    /* If Cap result is success then and only then populate individual device status */

    if (result == CAP_CLIENT_RESULT_SUCCESS)
    {
        if (vcpInitialised)
        {
            for (i = 0; i < deviceCount && vElem; i++)
            {
                CAP_CLIENT_INFO("\n(CAP) InitStreamAndControlCfm: Elem ptr: 0x%p \n", vElem);
                message->deviceStatus[i].cid = vElem->cid;
                message->deviceStatus[i].result =  vElem->recentStatus;

                CAP_CLIENT_INFO("\n(CAP) InitStreamAndControlCfm: Device : 0x%x, Status: 0x%x \n", message->deviceStatus[i].cid, message->deviceStatus[i].result);

                    vElem = vElem->next;
            }

        }
        else
        {
            for (i = 0; i < deviceCount && bElem; i++)
            {
                CAP_CLIENT_INFO("\n(CAP) InitStreamAndControlCfm: Elem ptr: 0x%p \n", bElem);
                message->deviceStatus[i].cid = bElem->cid;
                message->deviceStatus[i].result = bElem->recentStatus;

                CAP_CLIENT_INFO("\n(CAP) InitStreamAndControlCfm: Device : 0x%x, Status: 0x%x \n", message->deviceStatus[i].cid, message->deviceStatus[i].result);

                bElem = bElem->next;
            }
        }
       
    }


    /* NOTE: Internal API which populates the above details needs to be tested with multiple Device
     *       i.e Standard LE
     */

    /* capGetAllProfileInstanceStatus(deviceCount, elem, &message->status, profile); */
    
    CapClientMessageSend(inst->appTask, CAP_CLIENT_INIT_STREAM_CONTROL_CFM, message);

    /*
     * If the cfm was success and message queue is not
     * empty i.e msgElem is not NULL, handle the next
     * message
     *
     */
     /* Save CAP state and get next message */
    if (gInst)
    {
        msgElem = capClientGetNextMsgElem(gInst);
    }

    if (msgElem)
    {
        msgElem->handlerFunc(inst, (void*)msgElem, gInst);
    }
}

void capClientInitializeVcpInstance(CAP_INST *inst)
{
    CapClientGroupInstance *gInst =
                 (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);
    CsrCmnListElm_t *elem;
    BapInstElement *bap;

    if (gInst == NULL)
    {
        CAP_CLIENT_INFO("capinitializeVcpInstance: gInst is NULL");
        return;
    }

    elem = (CsrCmnListElm_t*)(gInst->bapList.first);
    bap = (BapInstElement*)elem;

    for(;bap; bap = bap->next)
    {
        VcpInstElement *vcp = NULL;

        vcp = (VcpInstElement*)
                 CAP_CLIENT_GET_VCP_ELEM_FROM_CID(gInst->vcpList, bap->bapHandle);

        if (vcp && vcp->vcpHandle == CAP_CLIENT_INVALID_SERVICE_HANDLE)
        {
            if (bap->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
            {
                VcpInitData clientInitParams;
                VcpHandles deviceData;

                /*
                 * NOTE: At Present We do not Support AICS and VOCS instances
                 * */

                deviceData.aicsHandleLength = 0;
                deviceData.aicsHandle = NULL;
                deviceData.vocsHandleLength = 0;
                deviceData.vocsHandle = NULL;

                clientInitParams.cid = bap->bapHandle;

                if (vcp->vcsData)
                {
                    CsrMemCpy(&deviceData.vcsHandle, vcp->vcsData, sizeof(GattVcsClientDeviceData));
                    VcpInitReq(gInst->libTask, &clientInitParams, &deviceData, FALSE);
                }
                else
                {
                    VcpInitReq(gInst->libTask, &clientInitParams, NULL, FALSE);
                }
                inst->vcpRequestCount++;
                bap->recentStatus = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;

            }
        }
    }
}

static void capClientInitializeStreamControlReqHandler(CAP_INST* inst,
                                          void* msg,
                                          CapClientGroupInstance* gInst)
{
    BapRole role = 0;
    BapInstElement *bap = (BapInstElement*) gInst->bapList.first;

    if (gInst->role & CAP_CLIENT_INITIATOR)
        role |= BAP_ROLE_UNICAST_CLIENT;

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
    if (gInst->role & CAP_CLIENT_COMMANDER)
        role |= BAP_ROLE_BROADCAST_ASSISTANT;
#endif
    /* Initialize  BAP in Unicast and Broadcast roles based on CAP role requirements */

    /* First initialize  BAP ins BROADCAST ASSISTANT ROLE
     * and then Do VCP init.
     *
     * Note: If CAP is initialized as both Initiator/Commander roles,
     * only VCP shall to be initialized as BAP assistant  will already be
     * initialized
     * */

    while (bap && bap->cid != CAP_CLIENT_INVALID_CID)
    {
        if (bap->bapHandle == CAP_CLIENT_INVALID_CID)
        {
            /* Continue but the Success cfm will only go to the valid cids */
            /* increment the baprequestcount */
            inst->bapRequestCount++;

            bap->bapHandle = bap->cid;

            /* Initialize BAP ASE list */
            /* If result is false update recent status of the BAP Instance with Error */
            capClientInitializeBapInstance(gInst, bap->cid, bap->bapData, role);
        }
        bap = bap->next;
    }

    CSR_UNUSED(msg);
}


void handleCapClientInitializeStreamControlReq(CAP_INST *inst, const Msg msg)
{
    /* If CAP is acting as commander the BAP as Broadcast Assistant and VCP profiles are required
     * If CAP is in initiator role then Broadcast Source/PACS/ASCS and additionally Broadcast
     * Assistant Roles are required
     * */

     CapClientGroupInstance *gInst = NULL;
     CapClientInternalInitStreamControlReq *req = (CapClientInternalInitStreamControlReq*)msg;
     CapClientProfileMsgQueueElem* msgElem = NULL;
     CapClientBool isQueueEmpty = FALSE;
     CapClientProfileTaskListElem* task = NULL;

     gInst = capClientSetNewActiveGroup(inst, req->groupId, FALSE);
     /* gInst->currentDeviceCount = cfm->numOfDevices; */

     if (gInst == NULL)
     {
         /* NULL Instance Error */
         capClientSendStreamControlInitCfm(inst,
                                       FALSE,
                                       CAP_CLIENT_RESULT_INVALID_GROUPID,
                                       0,
                                       NULL);
         return;
     }

     /* Add Check the message queue is empty
      *
      *  If the queue is empty, add the message to queue and
      *  proceed to process the request, else add the message and return.
      *  Queued message will be processed once current message being processed
      *  receives the cfm from lower layers
      */

     isQueueEmpty = CAP_CLIENT_IS_MSG_QUEUE_EMPTY(gInst->capClientMsgQueue);

     msgElem = CapClientMsgQueueAdd(&gInst->capClientMsgQueue, (void*)req, 0,
                                   req->type, capClientInitializeStreamControlReqHandler, task);

     if (isQueueEmpty)
     {
         capClientInitializeStreamControlReqHandler(inst, (void*)msgElem, gInst);
     }
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */
