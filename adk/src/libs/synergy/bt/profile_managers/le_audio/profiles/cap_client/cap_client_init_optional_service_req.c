/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #
******************************************************************************/
#include "cap_client_util.h"
#include "cap_client_common.h"
#include "cap_client_init_optional_service_req.h"
#include "cap_client_micp_operation_req.h"
#include "cap_client_debug.h"
#include "cap_client_init.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void capClientSendOptionalServiceInitCfm(AppTask appTask,
                                  uint8 deviceCount,
                                  ServiceHandle groupId,
                                  uint32 services,
                                  CapClientResult result,
                                  CsrCmnListElm_t* elem)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientInitOptionalServicesCfm);
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    MicpInstElement* vElem = NULL;
#endif
    CapClientGroupInstance* gInst = NULL;
    uint8 i = 0;

    message->groupId = groupId;
    message->deviceStatusLen = 0x00;
    message->result = result;

    /* Clear the pending operation flag as operation is done */

    gInst = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(groupId);
    if (gInst)
        CAP_CLIENT_CLEAR_PENDING_OP(gInst->pendingOp);

    CAP_CLIENT_INFO("\n(CAP) capClientSendOptionalServiceInitCfm: Device Count: %d \n", deviceCount);

    if(elem)
    {
        message->deviceStatus = (CapClientDeviceStatus*)
               CsrPmemZalloc(deviceCount*sizeof(CapClientDeviceStatus));
        message->deviceStatusLen = deviceCount;
    }

    /* Currently only one serivce(MICP), in future need to extend as new optional service will come */
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    vElem = (MicpInstElement*)elem;

    /* If Cap result is success then and only then populate individual device status */
    if (result == CAP_CLIENT_RESULT_SUCCESS)
    {
        for (i = 0; i < deviceCount ; i++)
        {
            CAP_CLIENT_INFO("\n(CAP) capClientSendOptionalServiceInitCfm: Elem ptr handle : 0x%x \n", vElem->micpHandle);
            message->deviceStatus[i].cid = vElem->cid;
            message->deviceStatus[i].result =  vElem->recentStatus;

            /* Currently direct assignment but in future it will be bit mask of optional services */
            message->optServices = services;

            CAP_CLIENT_INFO("\n(CAP) capClientSendOptionalServiceInitCfm: btconnId : 0x%x, Status: 0x%x \n", message->deviceStatus[i].cid, message->deviceStatus[i].result);

            vElem = vElem->next;
        }
    }
#endif

    CapClientMessageSend(appTask, CAP_CLIENT_INIT_OPTIONAL_SERVICES_CFM, message);

}

void handleCapClientInitOptionalServicesReq(CAP_INST *inst, const Msg msg)
{
    CapClientGroupInstance *gInst = NULL;
    VcpInstElement* vcp = NULL;
    CapClientInternalInitOptionalServicesReq *req = (CapClientInternalInitOptionalServicesReq*)msg;

    /* Service discovery params need to enahnce in future for all bit mask as new services will come in future
     * currently its written for micp only (in future need to extract the services from req->servicesMask and do the disocvery for those)*/

    gInst = CAP_CLIENT_GET_GROUP_INST_DATA(req->groupId);

    if (gInst == NULL)
    {
        capClientSendOptionalServiceInitCfm(inst->appTask,
                                         0,
                                         req->groupId,
                                         CAP_CLIENT_OPTIONAL_SERVICE_MICP,
                                         CAP_CLIENT_RESULT_INVALID_GROUPID,
                                         NULL);
        return;
    }

    /* Check for the service mask if nothing is set then send error message */
    if (req->servicesMask == CAP_CLIENT_OPTIONAL_SERVICE_INVALID_SERVICE)
    {
        /* Send error confirmation here as no remote device is supporting micp in a group */
        capClientSendOptionalServiceInitCfm(inst->appTask,
                                         0,
                                         req->groupId,
                                         CAP_CLIENT_OPTIONAL_SERVICE_MICP,
                                         CAP_CLIENT_RESULT_FAILURE_MICP_ERR,
                                         gInst->micpList->first);
        return;
    }

    vcp = (VcpInstElement*)((&gInst->vcpList)->first);

    /* Check mandatory service is discovered or not , if not sent error and return */
    if (vcp == NULL)
    {
        /* Send error confirmation here as no remote device is supporting micp in a group */
        capClientSendOptionalServiceInitCfm(inst->appTask,
                                         0,
                                         req->groupId,
                                         CAP_CLIENT_OPTIONAL_SERVICE_INVALID_SERVICE,
                                         CAP_CLIENT_RESULT_FAILURE_DICOVERY_ERR,
                                         gInst->micpList->first);
        return;
    }

    /* Currently this is getting written for MICP and in future can be extened for other optional services also */
    /* Call the micp init handler, in future need to have other optional services init need to add similar functions */
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    capClientHandleMicpInit(inst, gInst, req);
#else
    /* If no optional service is enabled then send eror confirmation */
    capClientSendOptionalServiceInitCfm(inst->appTask,
                                     0,
                                     req->groupId,
                                     req->servicesMask,
                                     CAP_CLIENT_RESULT_INVALID_OPERATION,
                                     gInst->micpList->first);
#endif

}
#endif

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
void capClientInitInitializeMicpProfile(CapClientGroupInstance *const cap,
                                       GattMicsClientDeviceData *handles,
                                       uint32 cid)
{
    /*initialize service and profiles data*/
    MicpInstElement *micp = NULL;
    micp = (MicpInstElement*)CAP_CLIENT_GET_MICP_ELEM_FROM_CID(cap->micpList, cid);

    if (micp == NULL)
    {
        micp = (MicpInstElement*)CAP_CLIENT_ADD_MICP_INST(cap->micpList);
    }

    micp->cid = cid;
    micp->groupId = cap->groupId;

    if (handles == NULL)
    {
        micp->micsData = NULL;
        return;
    }

    /* Set the status to be success as for reconneciton micp handles information is already present */
    micp->recentStatus = CAP_CLIENT_RESULT_SUCCESS;

    micp->micsData = (GattMicsClientDeviceData*)
                              CsrPmemZalloc(sizeof(GattMicsClientDeviceData));
    CsrMemCpy(micp->micsData, handles, sizeof(GattMicsClientDeviceData));

}
#endif
