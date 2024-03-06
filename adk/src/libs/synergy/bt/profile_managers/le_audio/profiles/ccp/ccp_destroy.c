/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <gatt_telephone_bearer_client.h>

#include "ccp.h"
#include "ccp_private.h"
#include "ccp_destroy.h"
#include "ccp_common.h"
#include "ccp_debug.h"

void ccpSendDestroyCfm(CCP * ccp_inst, CcpStatus status)
{
    MAKE_CCP_MESSAGE(CcpDestroyCfm);

    message->id = CCP_DESTROY_CFM;
    message->status = status;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;

    CcpMessageSend(ccp_inst->appTask, message);
}

/******************************************************************************/
void CcpDestroyReq(CcpProfileHandle profileHandle)
{
    CCP * ccp_inst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (ccp_inst)
    {
        /* Send confirmation message with status in progress */
        ccpSendDestroyCfm(ccp_inst, CCP_STATUS_IN_PROGRESS);

        /* Terminate TBS Client */
        GattTelephoneBearerClientTerminateReq(ccp_inst->tbs_srvc_hdl);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}


/****************************************************************************/
void ccpDestroyProfileInst(CCP *ccp_inst)
{
    bool res = FALSE;
    AppTask appTask = ccp_inst->appTask;

    ccp_main_inst *ccpMainInst = ccpGetMainInstance();

    /* We can destroy all the list we have in the profile context */
    ccpDestroyReqAllSrvcHndlList(ccp_inst);
    ccpDestroyReqAllInstList(ccp_inst);

    /* Clear pending messages */
    /*MessageFlushTask((Task)&ccp_inst->lib_task);*/

    /* Send the confirmation message */
    MAKE_CCP_MESSAGE(CcpDestroyCfm);
    message->id = CCP_DESTROY_CFM;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;

     /* Free the profile instance memory and remove the element from main list */
    res = FREE_CCP_CLIENT_INST(ccp_inst->ccp_srvc_hdl);
    if(ccpMainInst)
        CCP_REMOVE_SERVICE_HANDLE(ccpMainInst->profile_handle_list, ccp_inst->ccp_srvc_hdl);

    if (res)
    {
        message->status = CCP_STATUS_SUCCESS;
    }
    else
    {
        message->status = CCP_STATUS_FAILED;
    }

    CcpMessageSend(appTask, message);
}



void ccpSendTbsTerminateCfm(CCP *ccp_inst,
                            CcpStatus status,
                            GattTelephoneBearerClientDeviceData handles)
{
    MAKE_CCP_MESSAGE(CcpTbsTerminateCfm);

    message->id = CCP_TBS_TERMINATE_CFM;
    message->status = status;
    message->prflHndl = ccp_inst->ccp_srvc_hdl;

    memcpy(&(message->tbsHandle), &handles, sizeof(GattTelephoneBearerClientDeviceData));

    CcpMessageSend(ccp_inst->appTask, message);
}


/****************************************************************************/
void ccpHandleTbsClientTerminateResp(CCP *ccp_inst,
                                     const GattTelephoneBearerClientTerminateCfm * message)
{
    if(message->status == GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS)
    {
        /* TBS Client has been terminated successfully. */

        /* Send the tbs characteristic handles to the application */
        ccpSendTbsTerminateCfm(ccp_inst,
                               CCP_STATUS_SUCCESS,
                               message->deviceData);

        /* As gTBS only is currently supported CCP instance can be destroyed */
        ccpDestroyProfileInst(ccp_inst);
    }
    else
    {
        ccpSendDestroyCfm(ccp_inst, CCP_STATUS_FAILED);
    }
}

