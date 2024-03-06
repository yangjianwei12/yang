/******************************************************************************
 Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "vsc_lib.h"
#include "vsc_main.h"

#define MAKE_VSC_PRIM(MSG, CFM) \
    VSDM_##MSG##_T *vsdmPrim = (VSDM_##MSG##_T *)vscData->recvMsgP; \
    CFM *prim = (CFM*)CsrPmemZalloc(sizeof (CFM)); \
    prim->type = VSC_##MSG


static void SendPrim(CsrCmnListElm_t *elem, void *data)
{
    VscApplicationInstanceElement *vscHandle = (VscApplicationInstanceElement *)elem;

    VSC_PUT_MESSAGE(vscHandle->handle, data);
}

void VscArrivalHandler(vscInstanceData_t  *vscData){
    VscUprim *vsdmPrim = (VscUprim *)vscData->recvMsgP;

    if(vsdmPrim->type == VSDM_REGISTER_CFM)
    {
        VscRegisterCfmHandler(vscData);
    }
    else if(vscData->vscVar.appHandle != VSC_SCHED_QID_INVALID && vscData->InstanceList.first != NULL)
    {
        switch(vsdmPrim->type)
        {
        case VSDM_QLM_CONNECTION_COMPLETE_IND:
            VscQlmConnectionCompleteIndHandler(vscData);
            break;
        case VSDM_QCM_PHY_CHANGE_IND:
            VscQcmPhyChangeIndHandler(vscData);
            break;
        case VSDM_INCOMING_PAGE_IND:
            VscIncomingPageIndHandler(vscData);
            break;
        case VSDM_QLL_CONNECTION_COMPLETE_IND:
            VscQllConnectionCompleteIndHandler(vscData);
            break;
        default:
            /*It must be a confirmation*/
            VscHciCfmHandler(vscData);
            break;
        }
    }
    else
    {
        VSC_ERROR("Application has not registered for messages from VSC lib.");
    }
}

void VscHciCfmHandler(vscInstanceData_t *vscData) {
    VscUprim *vsdmPrim = (VscUprim *)vscData->recvMsgP;

    switch(vsdmPrim->type)
    {
    case VSDM_REGISTER_CFM:
        VscRegisterCfmHandler(vscData);
        break;
    case VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM:
        VscReadLocalQlmSuppFeaturesCfmHandler(vscData);
        break;
    case VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM:
        VscReadRemoteQlmSuppFeaturesCfmHandler(vscData);
        break;
    case VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM:
        VscWriteScHostSuppOverrideCfmHandler(vscData);
        break;
    case VSDM_READ_SC_HOST_SUPP_OVERRIDE_CFM:
        VscReadScHostSuppOverrideCfmHandler(vscData);
        break;
    case VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_CFM:
        VscWriteScHostSuppCodOverrideCfmHandler(vscData);
        break;
    case VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_CFM:
        VscReadScHostSuppCodOverrideCfmHandler(vscData);
        break;
    case VSDM_SET_QHS_HOST_MODE_CFM:
        VscSetQhsHostModeCfmHandler(vscData);
        break;
    case VSDM_SET_WBM_FEATURES_CFM:
        VscSetWbmFeaturesCfmHandler(vscData);
        break;
    case VSDM_CONVERT_RPA_TO_IA_CFM:
        VscConvertRpaToIaCfmHandler(vscData);
        break;
    case VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_CFM:
        VscWriteTruncatedPageScanEnableCfmHandler(vscData);
        break;
    case VSDM_SET_STREAMING_MODE_CFM:
        VscSetStreamingModeCfmHandler(vscData);
        break;
    case VSDM_READ_REMOTE_QLL_SUPP_FEATURES_CFM:
        VscReadRemoteQllSuppFeaturesCfmHandler(vscData);
        break;
    default:
        /*This is an invalid response.*/
        break;
    }
}

/*Arrival Handler functions*/
void VscQlmConnectionCompleteIndHandler(vscInstanceData_t *vscData)
{
    MAKE_VSC_PRIM(QLM_CONNECTION_COMPLETE_IND, VscQlmConnectionCompleteInd);

    prim->status = vsdmPrim->status;
    prim->handle = vsdmPrim->handle;
    prim->bdAddr = vsdmPrim->bd_addr;

    CsrCmnListIterate((CsrCmnList_t *) &vscData->InstanceList,
                    SendPrim,
                    prim);

    VscLocalQueueHandler();
}

void VscQcmPhyChangeIndHandler(vscInstanceData_t *vscData)
{
    MAKE_VSC_PRIM(QCM_PHY_CHANGE_IND, VscQcmPhyChangeInd);

    prim->status = vsdmPrim->status;
    prim->handle = vsdmPrim->handle;
    prim->bdAddr = vsdmPrim->bd_addr;
    prim->phy = vsdmPrim->phy;
    prim->source = vsdmPrim->source;

    CsrCmnListIterate((CsrCmnList_t *) &vscData->InstanceList,
                    SendPrim,
                    prim);

    VscLocalQueueHandler();
}

void VscIncomingPageIndHandler(vscInstanceData_t *vscData)
{
    MAKE_VSC_PRIM(INCOMING_PAGE_IND, VscIncomingPageInd);

    prim->bdAddr = vsdmPrim->bd_addr;
    prim->classOfDevice = vsdmPrim->class_of_device;

    CsrCmnListIterate((CsrCmnList_t *) &vscData->InstanceList,
                    SendPrim,
                    prim);

    VscLocalQueueHandler();
}

void VscQllConnectionCompleteIndHandler(vscInstanceData_t *vscData)
{
    /* Check application is registered with lib before allocating memory. */
    if(vscData->InstanceList.first != NULL)
    {
        MAKE_VSC_PRIM(QLL_CONNECTION_COMPLETE_IND, VscQllConnectionCompleteInd);

        prim->status  = vsdmPrim->status;
        prim->handle  = vsdmPrim->handle;

        prim->tpAddrt.type = vsdmPrim->tp_addrt.tp_type;
        prim->tpAddrt.addr.lap = vsdmPrim->tp_addrt.addrt.addr.lap;
        prim->tpAddrt.addr.uap = vsdmPrim->tp_addrt.addrt.addr.uap;
        prim->tpAddrt.addr.nap = vsdmPrim->tp_addrt.addrt.addr.nap;

        CsrCmnListIterate((CsrCmnList_t *) &vscData->InstanceList,
                        SendPrim,
                        prim);
    }

    VscLocalQueueHandler();
}

void VscRegisterCfmHandler(vscInstanceData_t *vscData)
{
    VSDM_REGISTER_CFM_T *vsdmPrim = (VSDM_REGISTER_CFM_T *)vscData->recvMsgP;

    if(vsdmPrim->result == VSC_RESULT_FAIL)
    {
        VSC_PANIC("VSDM registration failed\n");
    }

    VscLocalQueueHandler();
}

void VscReadLocalQlmSuppFeaturesCfmHandler(vscInstanceData_t *vscData)
{
    CsrUint8 i;
    MAKE_VSC_PRIM(READ_LOCAL_QLM_SUPP_FEATURES_CFM, VscReadLocalQlmSuppFeaturesCfm);

    prim->status = vsdmPrim->status;

    if(vsdmPrim->status == VSC_STATUS_SUCCESS)
    {
        for(i = 0; i < VSC_QLM_SUPP_FET_SIZE; i++)
        {
            prim->qlmpSuppFeatures[i] = vsdmPrim->qlmp_supp_features[i];
        }
    }

    VscLocalQueueHandler();
    VSC_PUT_MESSAGE(vscData->vscVar.appHandle, prim);
}

void VscReadRemoteQlmSuppFeaturesCfmHandler(vscInstanceData_t *vscData)
{
    CsrUint8 i;
    MAKE_VSC_PRIM(READ_REMOTE_QLM_SUPP_FEATURES_CFM, VscReadRemoteQlmSuppFeaturesCfm);

    prim->status = vsdmPrim->status;
    prim->bdAddr = vsdmPrim->bd_addr;

    if(vsdmPrim->status == VSC_STATUS_SUCCESS)
    {
        for(i = 0; i < VSC_QLM_SUPP_FET_SIZE; i++)
        {
            prim->qlmpSuppFeatures[i] = vsdmPrim->qlmp_supp_features[i];
        }
    }

    VscLocalQueueHandler();
    VSC_PUT_MESSAGE(vscData->vscVar.appHandle, prim);
}

void VscWriteScHostSuppOverrideCfmHandler(vscInstanceData_t *vscData)
{
    MAKE_VSC_PRIM(WRITE_SC_HOST_SUPP_OVERRIDE_CFM, VscWriteScHostSuppOverrideCfm);

    prim->status = vsdmPrim->status;

    VscLocalQueueHandler();
    VSC_PUT_MESSAGE(vscData->vscVar.appHandle, prim);
}


void VscReadScHostSuppOverrideCfmHandler(vscInstanceData_t *vscData)
{
    CsrUint8 i;
    MAKE_VSC_PRIM(READ_SC_HOST_SUPP_OVERRIDE_CFM, VscReadScHostSuppOverrideCfm);

    prim->status = vsdmPrim->status;

    if(vsdmPrim->status == VSC_STATUS_SUCCESS)
    {
        prim->numCompIDs = vsdmPrim->num_compIDs;

        if(prim->numCompIDs < VSC_MAX_NO_OF_COMPIDS)
        {
            for(i = 0; i < VSC_MAX_NO_OF_COMPIDS; i++)
            {
                prim->compID[i] = vsdmPrim->compID[i];
                prim->minLmpVersion[i] = vsdmPrim->min_lmpVersion[i];
                prim->minLmpSubVersion[i] = vsdmPrim->min_lmpSubVersion[i];
            }
        }
        else
        {
            VSC_PANIC("Too many Comp Ids\n");
        }
    }

    VscLocalQueueHandler();
    VSC_PUT_MESSAGE(vscData->vscVar.appHandle, prim);
}


void VscWriteScHostSuppCodOverrideCfmHandler(vscInstanceData_t *vscData)
{
    MAKE_VSC_PRIM(WRITE_SC_HOST_SUPP_COD_OVERRIDE_CFM, VscWriteScHostSuppCodOverrideCfm);

    prim->status = vsdmPrim->status;

    VscLocalQueueHandler();
    VSC_PUT_MESSAGE(vscData->vscVar.appHandle, prim);
}


void VscReadScHostSuppCodOverrideCfmHandler(vscInstanceData_t *vscData)
{
    MAKE_VSC_PRIM(READ_SC_HOST_SUPP_COD_OVERRIDE_CFM, VscReadScHostSuppCodOverrideCfm);

    prim->status = vsdmPrim->status;
    prim->bitNumber = vsdmPrim->bit_number;
    prim->value = vsdmPrim->value;

    VscLocalQueueHandler();
    VSC_PUT_MESSAGE(vscData->vscVar.appHandle, prim);
}


void VscSetQhsHostModeCfmHandler(vscInstanceData_t *vscData)
{
    MAKE_VSC_PRIM(SET_QHS_HOST_MODE_CFM, VscSetQhsHostModeCfm);

    prim->status = vsdmPrim->status;

    VscLocalQueueHandler();
    VSC_PUT_MESSAGE(vscData->vscVar.appHandle, prim);
}


void VscSetWbmFeaturesCfmHandler(vscInstanceData_t *vscData)
{
    MAKE_VSC_PRIM(SET_WBM_FEATURES_CFM, VscSetWbmFeaturesCfm);

    prim->status = vsdmPrim->status;

    VscLocalQueueHandler();
    VSC_PUT_MESSAGE(vscData->vscVar.appHandle, prim);
}


void VscConvertRpaToIaCfmHandler(vscInstanceData_t *vscData)
{
    MAKE_VSC_PRIM(CONVERT_RPA_TO_IA_CFM, VscConvertRpaToIaCfm);

    prim->status = vsdmPrim->status;
    prim->identityAddress = vsdmPrim->identity_address;
    prim->priv_mode = vsdmPrim->priv_mode;

    VscLocalQueueHandler();
    VSC_PUT_MESSAGE(vscData->vscVar.appHandle, prim);
}


void VscWriteTruncatedPageScanEnableCfmHandler(vscInstanceData_t *vscData)
{
    MAKE_VSC_PRIM(WRITE_TRUNCATED_PAGE_SCAN_ENABLE_CFM, VscWriteTruncatedPageScanEnableCfm);

    prim->status = vsdmPrim->status;

    VscLocalQueueHandler();
    VSC_PUT_MESSAGE(vscData->vscVar.appHandle, prim);
}

void VscSetStreamingModeCfmHandler(vscInstanceData_t *vscData)
{
    MAKE_VSC_PRIM(SET_STREAMING_MODE_CFM, VscSetStreamingModeCfm);

    prim->status = vsdmPrim->status;
    prim->tp_addrt = vsdmPrim->tp_addrt;

    VscLocalQueueHandler();
    VSC_PUT_MESSAGE(vscData->vscVar.appHandle, prim);
}

void VscReadRemoteQllSuppFeaturesCfmHandler(vscInstanceData_t *vscData)
{
    CsrUint8 i;
    MAKE_VSC_PRIM(READ_REMOTE_QLL_SUPP_FEATURES_CFM, VscReadRemoteQllSuppFeaturesCfm);

    prim->status  = vsdmPrim->status;

    prim->tpAddrt.type = vsdmPrim->tp_addrt.tp_type;
    prim->tpAddrt.addr.lap = vsdmPrim->tp_addrt.addrt.addr.lap;
    prim->tpAddrt.addr.uap = vsdmPrim->tp_addrt.addrt.addr.uap;
    prim->tpAddrt.addr.nap = vsdmPrim->tp_addrt.addrt.addr.nap;

    if(vsdmPrim->status == VSC_STATUS_SUCCESS)
    {
        for(i = 0; i < VSC_QLL_SUPP_FET_SIZE; i++)
        {
            prim->qllSuppFeatures[i] = vsdmPrim->qll_supp_features[i];
        }
    }

    VscLocalQueueHandler();
    VSC_PUT_MESSAGE(vscData->vscVar.appHandle, prim);
}
