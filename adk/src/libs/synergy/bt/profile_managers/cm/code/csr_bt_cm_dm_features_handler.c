/******************************************************************************
 Copyright (c) 2008-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_events_handler.h"
#include "csr_bt_cm_util.h"

#ifndef CSR_BT_EXCLUDE_HCI_READ_LOCAL_EXT_FEATURES
extern void dm_hci_read_local_ext_features(CsrUint8 page_num, DM_UPRIM_T **pp_prim );
#endif
static void csrBtCmReadRemoteExtFeaturesCfmSend(CsrSchedQid            appHandle,
                                                CsrUint8          pageNum,
                                                CsrUint8          maxPageNum,
                                                CsrUint16         lmpFeatures[4],
                                                CsrBtDeviceAddr   deviceAddr,
                                                CsrBtResultCode   resultCode,
                                                CsrBtSupplier     resultSupplier)
{
    CsrBtCmReadRemoteExtFeaturesCfm * cmPrim;
    cmPrim                 = (CsrBtCmReadRemoteExtFeaturesCfm *)CsrPmemAlloc(sizeof(CsrBtCmReadRemoteExtFeaturesCfm));
    cmPrim->type           = CSR_BT_CM_READ_REMOTE_EXT_FEATURES_CFM;
    cmPrim->bd_addr        = deviceAddr;
    cmPrim->pageNum        = pageNum;
    cmPrim->maxPageNum     = maxPageNum;
    cmPrim->resultCode     = resultCode;
    cmPrim->resultSupplier = resultSupplier;
    SynMemCpyS(cmPrim->extLmpFeatures, 4*sizeof(CsrUint16), lmpFeatures, 4*sizeof(CsrUint16));
    CsrBtCmPutMessage(appHandle, cmPrim);
}

static void csrBtCmSaveRemoteExtFeaturesHandler(cmInstanceData_t *cmData,
                                                CsrBtDeviceAddr  deviceAddr,
                                                CsrUint16        lmpFeatures[4],
                                                CsrUint8         hciStatus)
{
    if (hciStatus == HCI_SUCCESS)
    {
        aclTable *aclConnectionElement;
        returnAclConnectionElement(cmData, deviceAddr, &aclConnectionElement);

        if (aclConnectionElement)
        {
            aclConnectionElement->remoteFeaturesValid = TRUE;
            SynMemCpyS(aclConnectionElement->remoteFeatures, sizeof(aclConnectionElement->remoteFeatures), 
                       lmpFeatures, sizeof(aclConnectionElement->remoteFeatures));
        }
    }
}

void CsrBtCmDmHciReadRemoteFeaturesCompleteHandler(cmInstanceData_t *cmData)
{
    DM_HCI_READ_REMOTE_SUPP_FEATURES_CFM_T * dmPrim;
    dmPrim = (DM_HCI_READ_REMOTE_SUPP_FEATURES_CFM_T *) cmData->recvMsgP;

    if (CSR_BT_CM_DM_QUEUE_LOCKED(&cmData->dmVar) &&
        CsrBtBdAddrEq(&cmData->dmVar.cacheTargetDev, &dmPrim->bd_addr))
    {
        if (cmData->dmVar.appHandle != CSR_BT_CM_IFACEQUEUE)
        {
            CsrBtResultCode   resultCode;
            CsrBtSupplier     resultSupplier;


            if (dmPrim->status == HCI_SUCCESS)
            {
                resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
                resultSupplier = CSR_BT_SUPPLIER_CM;
            }
            else
            {
                resultCode      = (CsrBtResultCode) dmPrim->status;
                resultSupplier  = CSR_BT_SUPPLIER_HCI;
                CsrMemSet(dmPrim->features, 0, 4*sizeof(CsrUint16));
            }

            csrBtCmReadRemoteExtFeaturesCfmSend(cmData->dmVar.appHandle,
                                                0,
                                                0,
                                                dmPrim->features,
                                                dmPrim->bd_addr,
                                                resultCode,
                                                resultSupplier);
        }
        else
        { /* Just restore queue as it were the CM which send the request    */
            csrBtCmSaveRemoteExtFeaturesHandler(cmData,
                                                dmPrim->bd_addr,
                                                dmPrim->features,
                                                dmPrim->status);
        }

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_REMOTE_FEATURES
        CsrBtCmPropgateEvent(cmData,
                             CsrBtCmPropgateReadRemoteFeatureEvents,
                             CSR_BT_CM_EVENT_MASK_SUBSCRIBE_REMOTE_FEATURES,
                             dmPrim->status,
                             dmPrim,
                             NULL);
#endif

        CsrBtCmDmLocalQueueHandler();
    }
    else
    {
        /* do nothing */
    }
}

void CsrBtCmReadRemoteFeaturesReqHandler(cmInstanceData_t * cmData)
{
    CsrBtCmReadRemoteFeaturesReq * cmPrim;

    cmPrim = (CsrBtCmReadRemoteFeaturesReq *) cmData->recvMsgP;

    cmData->dmVar.appHandle      = cmPrim->appHandle;
    cmData->dmVar.cacheTargetDev = cmPrim->deviceAddr;
    dm_hci_read_remote_features(&cmPrim->deviceAddr, NULL);
}

void CsrBtCmDmHciReadRemoteExtFeaturesCompleteHandler(cmInstanceData_t *cmData)
{
    DM_HCI_READ_REMOTE_EXT_FEATURES_CFM_T * dmPrim;

    dmPrim = (DM_HCI_READ_REMOTE_EXT_FEATURES_CFM_T *) cmData->recvMsgP;

    if (cmData->dmVar.lockMsg == CSR_BT_CM_READ_REMOTE_EXT_FEATURES_REQ &&
        CsrBtBdAddrEq(&cmData->dmVar.cacheTargetDev, &dmPrim->bd_addr))
    {
        if (dmPrim->status == HCI_SUCCESS)
        {
            csrBtCmReadRemoteExtFeaturesCfmSend(cmData->dmVar.appHandle,
                                                dmPrim->page_num,
                                                dmPrim->max_page_num,
                                                dmPrim->lmp_ext_features,
                                                dmPrim->bd_addr,
                                                CSR_BT_RESULT_CODE_CM_SUCCESS,
                                                CSR_BT_SUPPLIER_CM);
            CsrBtCmDmLocalQueueHandler();
        }
        else
        { /* Start sending an old supported features request    */
            dm_hci_read_remote_features(&dmPrim->bd_addr,NULL);
        }
    }
    else
    { /* Just ignore see D-5351                                 */
        ;
    }
}

#ifdef INSTALL_CM_REMOTE_EXT_FEATURES
void CsrBtCmReadRemoteExtFeaturesReqHandler(cmInstanceData_t *cmData)
{    /* This event indicates that the application desired to know the local
     device extended features. */
    CsrBtCmReadRemoteExtFeaturesReq *cmPrim;

    cmPrim                       = (CsrBtCmReadRemoteExtFeaturesReq *) cmData->recvMsgP;
    cmData->dmVar.appHandle      = cmPrim->appHandle;
    cmData->dmVar.cacheTargetDev = cmPrim->bd_addr;
    dm_hci_read_remote_ext_features(&cmPrim->bd_addr, cmPrim->pageNum, NULL);
}
#endif /* INSTALL_CM_REMOTE_EXT_FEATURES */

#ifdef CSR_BT_INSTALL_CM_READ_LOCAL_EXT_FEATURES
void CsrBtCmDmHciReadLocalExtFeaturesCompleteHandler(cmInstanceData_t *cmData)
{
    DM_HCI_READ_LOCAL_EXT_FEATURES_CFM_T * dmPrim;
    CsrBtCmReadLocalExtFeaturesCfm * cmPrim;

    dmPrim = (DM_HCI_READ_LOCAL_EXT_FEATURES_CFM_T *) cmData->recvMsgP;
    cmPrim = (CsrBtCmReadLocalExtFeaturesCfm *)CsrPmemAlloc(sizeof(CsrBtCmReadLocalExtFeaturesCfm));
    cmPrim->type = CSR_BT_CM_READ_LOCAL_EXT_FEATURES_CFM;

    if (dmPrim->status == HCI_SUCCESS)
    {
        cmPrim->resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cmPrim->resultSupplier = CSR_BT_SUPPLIER_CM;
        cmPrim->pageNum        = dmPrim->page_num;
        cmPrim->maxPageNum     = dmPrim->max_page_num;
        SynMemCpyS(cmPrim->extLmpFeatures, 8, dmPrim->lmp_ext_features, 8);
    }
    else
    {
        cmPrim->resultCode      = (CsrBtResultCode) dmPrim->status;
        cmPrim->resultSupplier  = CSR_BT_SUPPLIER_HCI;
        cmPrim->pageNum         = 0;
        cmPrim->maxPageNum      = 0;
        CsrMemSet(cmPrim->extLmpFeatures, 0, 8);
    }
    CsrBtCmPutMessage(cmData->dmVar.appHandle, cmPrim);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmReadLocalExtFeaturesReqHandler(cmInstanceData_t *cmData)
{    /* This event indicates that the application desired to know the local
     device extended features. */
    CsrBtCmReadLocalExtFeaturesReq *cmPrim;

    cmPrim                  = (CsrBtCmReadLocalExtFeaturesReq *) cmData->recvMsgP;
    cmData->dmVar.appHandle = cmPrim->appHandle;
    dm_hci_read_local_ext_features(cmPrim->pageNum, NULL);
}
#endif
void CsrBtCmDmHciReadSuppFeaturesCfmHandler(cmInstanceData_t *cmData)
{
    DM_HCI_READ_LOCAL_SUPP_FEATURES_CFM_T    *dmPrim;

    dmPrim = (DM_HCI_READ_LOCAL_SUPP_FEATURES_CFM_T *) cmData->recvMsgP;

    if (dmPrim->status == HCI_SUCCESS)
    {
        SynMemCpyS(cmData->dmVar.lmpSuppFeatures,
                  sizeof(cmData->dmVar.lmpSuppFeatures),
                  dmPrim->lmp_supp_features,
                  sizeof(cmData->dmVar.lmpSuppFeatures));
    }

    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
    {
        CmInitSequenceHandler(cmData,
                              CM_INIT_SEQ_READ_LOCAL_SUPP_FEATURES_CFM,
                              CSR_BT_RESULT_CODE_CM_SUCCESS,
                              CSR_BT_SUPPLIER_CM);
    }
}

