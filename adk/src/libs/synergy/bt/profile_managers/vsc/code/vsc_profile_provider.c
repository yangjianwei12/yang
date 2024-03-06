/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#include "vsc_main.h"
#include "qbl_adapter_scheduler.h"


static const SignalHandlerType vscProviderHandler[VSC_PRIM_DOWNSTREAM_COUNT + 1] =
{
    VscSendRegisterReq,
    VscSendReadLocalQlmSuppFeaturesReq,
    VscSendReadRemoteQlmSuppFeaturesReq,
    VscSendWriteScHostSuppOverridReq,
    VscSendReadScHostSuppOverrideReq,
    VscSendWriteScHostSuppCodOverridReq,
    VscSendReadScHostSuppCodOverrideReq,
    VscSendSetQhsHostModeReq,
    VscSendSetWbmFeaturesReq,
    VscSendConvertRpaToIaReq,
    VscSetStreamingModeReq,
    VscRestoreQueueHandler,
};


void VscProfileProvider(vscInstanceData_t *vscData)
{
    CsrUint16 id = *(CsrUint16*)vscData->recvMsgP;
    if(VSC_QUEUE_LOCKED(&vscData->vscVar))
    {
        CsrMessageQueuePush(&vscData->vscVar.saveQueue, BT_VSDM_PRIM, vscData->recvMsgP);
        vscData->recvMsgP = NULL;
    }
    else
    {
        if(id >= VSC_PRIM_DOWN &&
           id <= VSC_PRIM_DOWNSTREAM_HIGHEST &&
           vscProviderHandler[(CsrUint16)(id - VSC_PRIM_DOWN)] != NULL)
        {
            VscLockQueue(vscData);
            vscProviderHandler[(CsrUint16)(id - VSC_PRIM_DOWN)](vscData);
        }
        else
        {
            /* The prim should not be here */
        }
    }
}


void VscSendRegisterReq(vscInstanceData_t *vscData)
{
    VSDM_REGISTER_REQ_T *prim = (VSDM_REGISTER_REQ_T*) vscData->recvMsgP;
    vscBtData.vscVar.appHandle = prim->phandle;
    prim->phandle = CSR_BT_VSDM_IFACEQUEUE;

    VSC_PutMsg(prim);
}

void VscSendReadLocalQlmSuppFeaturesReq(vscInstanceData_t *vscData)
{
    VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ_T *prim = (VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ_T*) vscData->recvMsgP;
    vscBtData.vscVar.appHandle = prim->phandle;
    prim->phandle = CSR_BT_VSDM_IFACEQUEUE;

    VSC_PutMsg(prim);
}

void VscSendReadRemoteQlmSuppFeaturesReq(vscInstanceData_t *vscData)
{
    VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ_T *prim = (VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ_T*)vscData->recvMsgP;
    vscData->vscVar.bdAddr = prim->bd_addr;
    vscBtData.vscVar.appHandle = prim->phandle;
    prim->phandle = CSR_BT_VSDM_IFACEQUEUE;

    VSC_PutMsg(prim);
}

void VscSendWriteScHostSuppOverridReq(vscInstanceData_t *vscData)
{
   VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ_T *prim = (VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ_T*)vscData->recvMsgP;
   vscBtData.vscVar.appHandle = prim->phandle;
   prim->phandle = CSR_BT_VSDM_IFACEQUEUE;

   VSC_PutMsg(prim);
}


void VscSendReadScHostSuppOverrideReq(vscInstanceData_t *vscData)
{
   VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ_T *prim = (VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ_T*)vscData->recvMsgP;
   vscBtData.vscVar.appHandle = prim->phandle;
   prim->phandle = CSR_BT_VSDM_IFACEQUEUE;

   VSC_PutMsg(prim);
}


void VscSendWriteScHostSuppCodOverridReq(vscInstanceData_t *vscData)
{
   VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ_T *prim = (VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ_T*)vscData->recvMsgP;
   vscBtData.vscVar.appHandle = prim->phandle;
   prim->phandle = CSR_BT_VSDM_IFACEQUEUE;

   VSC_PutMsg(prim);
}


void VscSendReadScHostSuppCodOverrideReq(vscInstanceData_t *vscData)
{
    VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ_T *prim = (VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ_T*)vscData->recvMsgP;
    vscBtData.vscVar.appHandle = prim->phandle;
    prim->phandle = CSR_BT_VSDM_IFACEQUEUE;

    VSC_PutMsg(prim);
}


void VscSendSetQhsHostModeReq(vscInstanceData_t *vscData)
{
   VSDM_SET_QHS_HOST_MODE_REQ_T *prim = (VSDM_SET_QHS_HOST_MODE_REQ_T*)vscData->recvMsgP;
   vscBtData.vscVar.appHandle = prim->phandle;
   prim->phandle = CSR_BT_VSDM_IFACEQUEUE;

   VSC_PutMsg(prim);
}


void VscSendSetWbmFeaturesReq(vscInstanceData_t *vscData)
{
   VSDM_SET_WBM_FEATURES_REQ_T *prim = (VSDM_SET_WBM_FEATURES_REQ_T*)vscData->recvMsgP;
   vscBtData.vscVar.appHandle = prim->phandle;
   prim->phandle = CSR_BT_VSDM_IFACEQUEUE;

   VSC_PutMsg(prim);
}


void VscSendConvertRpaToIaReq(vscInstanceData_t *vscData)
{

   VSDM_CONVERT_RPA_TO_IA_REQ_T *prim = (VSDM_CONVERT_RPA_TO_IA_REQ_T*)vscData->recvMsgP;
   vscBtData.vscVar.appHandle = prim->phandle;
   prim->phandle = CSR_BT_VSDM_IFACEQUEUE;

   VSC_PutMsg(prim);
}

void VscSetStreamingModeReq(vscInstanceData_t *vscData)
{
    VSDM_SET_STREAMING_MODE_REQ_T *prim = (VSDM_SET_STREAMING_MODE_REQ_T*)vscData->recvMsgP;
    vscBtData.vscVar.appHandle = prim->phandle;
    prim->phandle = CSR_BT_VSDM_IFACEQUEUE;

    VSC_PutMsg(prim);
}
