/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "vsc_main.h"
#include "vsc_lib.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_core_stack_pmalloc.h"


vscInstanceData_t vscBtData;
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtVscLto);


void VscInit(void **gash)
{
    vscInstanceData_t *vscData;

    *gash = &vscBtData;
    vscData = (vscInstanceData_t *) *gash;
    VscInitInstData(vscData);

    /* Register with CM to receive CSR_BT_CM_BLUECORE_INITIALIZED_IND */
    CsrBtCmSetEventMaskReqSend(CSR_BT_VSDM_IFACEQUEUE,
                               CSR_BT_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED,
                               CSR_BT_CM_EVENT_MASK_COND_ALL);
    /* Register Synergy logging handle */
    VSC_TEXT_REGISTER;
}

void VscHandler(void **gash)
{
    vscInstanceData_t *VscData;
    CsrUint16 eventClass = 0;
    void *msg = NULL;

    VscData = (vscInstanceData_t *) (*gash);
    CsrSchedMessageGet(&eventClass, &msg);
    VscData->recvMsgP = msg;
    VscData->vscVar.appHandle = vscBtData.vscVar.appHandle;
    VscPrim id = *(VscPrim*)VscData->recvMsgP;

    if(eventClass == VSDM_PRIM)
    {
        if(id >= VSC_PRIM_UPSTREAM_LOWEST)
        {
           /* Deals with confirmations and indications */
           VscArrivalHandler(VscData);
        }
        else
        {
            /* Deals with requests when cleaning the queue */
            VscProfileProvider(VscData);
        }
    }
    else
    {
        CsrPrim type = *(CsrPrim *)msg;
        if( id == VSC_HOUSE_CLEANING)
        {
            /* Queue cleaning handler */
            VscRestoreQueueHandler(VscData);
        }
        if(type == CSR_BT_CM_BLUECORE_INITIALIZED_IND )
        {
            /* Ready to register with Bluestack */
            VscData->vscVar.lockMsg = VSC_QUEUE_UNLOCK;
            VSC_INFO("Registering Vsc Library");
            VscRegisterReqSend(0);
        }
    }

    if(eventClass == VSDM_PRIM)
    {
        bluestack_msg_free(eventClass, msg);
    }
    else
    {
        SynergyMessageFree(eventClass, msg);
    }
    VscData->recvMsgP = NULL;
}

CsrBool VscFindHandle(CsrCmnListElm_t *elem, void *value)
{
    VscApplicationInstanceElement *conn = (VscApplicationInstanceElement *)elem;
    CsrSchedQid     phandle   = *(CsrSchedQid *) value;
    return ((conn->handle == phandle) ? TRUE : FALSE);
}

