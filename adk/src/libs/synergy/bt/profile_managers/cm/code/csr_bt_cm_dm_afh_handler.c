/******************************************************************************
 Copyright (c) 2008-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "csr_synergy.h"
#ifdef CSR_BT_INSTALL_CM_AFH

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_bits.h"
#include "csr_bt_cm_callback_q.h"
#include "csr_bt_cm_util.h"

#define HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE        (10) /* From HCI spec */
#define HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE_LE     (5)  /* From HCI spec */

static void csrBtCmSetAfhChannelClassCfmSend(CsrSchedQid appHandle,
                                             CsrBtResultCode result,
                                             CsrBtSupplier supplier)
{
    CsrBtCmSetAfhChannelClassCfm *cmPrim;
    cmPrim = (CsrBtCmSetAfhChannelClassCfm *) CsrPmemAlloc(sizeof(CsrBtCmSetAfhChannelClassCfm));
    cmPrim->type = CSR_BT_CM_SET_AFH_CHANNEL_CLASS_CFM;
    cmPrim->resultCode = result;
    cmPrim->resultSupplier = supplier;
    CsrBtCmPutMessage(appHandle, cmPrim);
}

void CsrBtCmDmHciSetAfhChannelClassCompleteHandler(cmInstanceData_t *cmInst)
{
    DM_HCI_SET_AFH_CHANNEL_CLASS_CFM_T * dmPrim;
    dmPrim = (DM_HCI_SET_AFH_CHANNEL_CLASS_CFM_T *) cmInst->recvMsgP;

    csrBtCmSetAfhChannelClassCfmSend(cmInst->dmVar.appHandle,
                                     (CsrBtResultCode)((dmPrim->status == HCI_SUCCESS)
                                                       ? CSR_BT_RESULT_CODE_CM_SUCCESS
                                                       : dmPrim->status),
                                     (CsrBtSupplier)((dmPrim->status == HCI_SUCCESS)
                                                     ? CSR_BT_SUPPLIER_CM
                                                     : CSR_BT_SUPPLIER_HCI));
    CsrBtCmDmLocalQueueHandler();
}

static void csrBtCmAfhMapsCollateFunc(CsrCmnListElm_t *elem, void *data)
{
    afhMapParms_t *afhMap = (afhMapParms_t *) elem;
    CsrUint8 *map = (CsrUint8 *) data;
    CsrUintFast8 idx;

    for (idx = 0; idx < HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE; ++idx)
    {
        map[idx] &= afhMap->map[idx];
    }
}

static void csrBtCmAfhGetRequestedMap(cmInstanceData_t *cmInst, CsrUint8 *map)
{
    /* Mark all as unknown */
    CsrMemSet(map, 0xFF, HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE);

    /* MSB is reserved (shall be set to zero, Vol 2, Part E, Sec. 7.3.46 */
    CSR_BT_BIT_CLEAR(map[HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE-1], 7);

    CsrCmnListIterate((CsrCmnList_t *) &cmInst->afhMaps,
                      csrBtCmAfhMapsCollateFunc,
                      map);
}

static void csrBtCmAfhSendChannelClassification(cmInstanceData_t *cmInst,
                                                CsrSchedQid appHandle)
{
    CsrUint8 map[HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE];

    CsrMemSet(map, 0xFF, HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE);
    cmInst->dmVar.appHandle = appHandle;
    csrBtCmAfhGetRequestedMap(cmInst, map);
    dm_hci_set_afh_channel_class(map, NULL);
}

#ifdef CSR_BT_LE_ENABLE
/* LE AFH set complete callback */
static void csrBtCmAfhLeSetHostChannelClassificationComplete(cmInstanceData_t *cmInst,
                                                             struct cmCallbackObjTag *object,
                                                             void *context,
                                                             void *event)
{
    DM_HCI_ULP_SET_HOST_CHANNEL_CLASSIFICATION_CFM_T *prim = (DM_HCI_ULP_SET_HOST_CHANNEL_CLASSIFICATION_CFM_T*)event;
    CsrBtCmSetAfhChannelClassReq *req = (CsrBtCmSetAfhChannelClassReq *) context;

    if(prim->status == HCI_SUCCESS)
    {
        csrBtCmAfhSendChannelClassification(cmInst, req->appHandle);
    }
    else
    {
        csrBtCmSetAfhChannelClassCfmSend(req->appHandle, prim->status, CSR_BT_SUPPLIER_HCI);
        CsrBtCmDmLocalQueueHandler();
    }
    
    CSR_UNUSED(object);
}

/* Channel collator for LE */
static void csrBtCmAfhLeMapsCollateFunc(CsrCmnListElm_t *elem, void *data)
{
    afhMapParms_t *afhMap = (afhMapParms_t *) elem;
    CsrUint8 *map = (CsrUint8 *) data;
    CsrUintFast8 idx;

    for (idx = 0; idx < HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE_LE; idx++)
    {
        /* Little endian. Pick every other bit. */
        map[idx] &= (CsrUint8) (((afhMap->map[idx * 2 + 1] & 0x01) >> 0) |
                                ((afhMap->map[idx * 2 + 1] & 0x04) >> 1) |
                                ((afhMap->map[idx * 2 + 1] & 0x10) >> 2) |
                                ((afhMap->map[idx * 2 + 1] & 0x40) >> 3) |
                                ((afhMap->map[idx * 2] & 0x01) << 4) |
                                ((afhMap->map[idx * 2] & 0x04) << 3) |
                                ((afhMap->map[idx * 2] & 0x10) << 2) |
                                ((afhMap->map[idx * 2] & 0x40) << 1));
    }
}

static void csrBtCmAfhLeGetRequestedMap(cmInstanceData_t *cmInst, CsrUint8 *map)
{
    CsrMemSet(map, 0xFF, HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE_LE);
    CSR_BT_BIT_CLEAR(map[HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE_LE-1], 7);
    CSR_BT_BIT_CLEAR(map[HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE_LE-1], 6);
    CSR_BT_BIT_CLEAR(map[HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE_LE-1], 5);
    CsrCmnListIterate((CsrCmnList_t *) &cmInst->afhMaps,
                      csrBtCmAfhLeMapsCollateFunc,
                      map);
}
#endif

void CsrBtCmSetAfhChannelClassReqHandler(cmInstanceData_t *cmInst)
{
    afhMapParms_t *afhMap;
    CsrBtCmSetAfhChannelClassReq *cmPrim = (CsrBtCmSetAfhChannelClassReq *) cmInst->recvMsgP;

    /* MSB is reserved (shall be set to zero, Vol 2, Part E, Sec. 7.3.46 */
    CSR_BT_BIT_CLEAR(cmPrim->map[HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE-1], 7);

    afhMap = (afhMapParms_t *) CsrCmnListSearchOffsetUint16((CsrCmnList_t *) &cmInst->afhMaps,
                                                            CsrOffsetOf(afhMapParms_t,
                                                                        appHandle),
                                                            cmPrim->appHandle);

    if (!afhMap)
    {
        afhMap = (afhMapParms_t *) CsrCmnListElementAddLast((CsrCmnList_t *) &cmInst->afhMaps,
                                                            sizeof(afhMapParms_t));
        afhMap->appHandle = cmPrim->appHandle;
    }
    SynMemCpyS(afhMap->map, sizeof(afhMap->map), cmPrim->map, sizeof(afhMap->map));

#ifdef CSR_BT_LE_ENABLE
    if (HCI_FEATURE_IS_SUPPORTED(LMP_FEATURES_LE_SUPPORTED_CONTROLLER_BIT, cmInst->dmVar.lmpSuppFeatures))
    {
        DM_UPRIM_T *dmPrim;
        CsrUint8 leMap[HCI_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE_LE];
        csrBtCmAfhLeGetRequestedMap(cmInst, leMap);
        dm_hci_ulp_set_host_channel_classification_req(leMap, &dmPrim);
        CsrBtCmCallbackSendSimpleBlockDm(cmInst,
                                         DM_HCI_ULP_SET_HOST_CHANNEL_CLASSIFICATION_CFM,
                                         cmPrim, 
                                         dmPrim,
                                         csrBtCmAfhLeSetHostChannelClassificationComplete);
        cmInst->recvMsgP = NULL;
    }
    else
#endif
    {
        csrBtCmAfhSendChannelClassification(cmInst, cmPrim->appHandle);
    }

}


void CsrBtCmDmHciReadAfhChannelAssesModeCompleteHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmReadAfhChannelAssessmentModeCfm * cmPrim;
    DM_HCI_READ_AFH_CHANNEL_CLASS_M_CFM_T * dmPrim;

    cmPrim = (CsrBtCmReadAfhChannelAssessmentModeCfm *) CsrPmemAlloc(sizeof(CsrBtCmReadAfhChannelAssessmentModeCfm));
    dmPrim = (DM_HCI_READ_AFH_CHANNEL_CLASS_M_CFM_T *) cmInst->recvMsgP;

    cmPrim->type      = CSR_BT_CM_READ_AFH_CHANNEL_ASSESSMENT_MODE_CFM;
    cmPrim->classMode = dmPrim->class_mode;

    if (dmPrim->status == HCI_SUCCESS)
    {
        cmPrim->resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cmPrim->resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        cmPrim->resultCode     = (CsrBtResultCode) dmPrim->status;
        cmPrim->resultSupplier = CSR_BT_SUPPLIER_HCI;
    }

    CsrBtCmPutMessage(cmInst->dmVar.appHandle, cmPrim);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmReadAfhChannelAssesModeReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmReadAfhChannelAssessmentModeReq * cmPrim;
    cmPrim = (CsrBtCmReadAfhChannelAssessmentModeReq *) cmInst->recvMsgP;
    cmInst->dmVar.appHandle = cmPrim->appHandle;
    dm_hci_read_afh_channel_class_m(NULL);
}


void CsrBtCmDmHciWriteAfhChannelAssesModeCompleteHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmWriteAfhChannelAssessmentModeCfm * cmPrim;
    DM_HCI_WRITE_AFH_CHANNEL_CLASS_M_CFM_T * dmPrim;

    cmPrim = (CsrBtCmWriteAfhChannelAssessmentModeCfm *) CsrPmemAlloc(sizeof(CsrBtCmWriteAfhChannelAssessmentModeCfm));
    dmPrim = (DM_HCI_WRITE_AFH_CHANNEL_CLASS_M_CFM_T *) cmInst->recvMsgP;

    cmPrim->type    = CSR_BT_CM_WRITE_AFH_CHANNEL_ASSESSMENT_MODE_CFM;

    if (dmPrim->status == HCI_SUCCESS)
    {
        cmPrim->resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cmPrim->resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        cmPrim->resultCode     = (CsrBtResultCode) dmPrim->status;
        cmPrim->resultSupplier = CSR_BT_SUPPLIER_HCI;
    }
    CsrBtCmPutMessage(cmInst->dmVar.appHandle, cmPrim);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmWriteAfhChannelAssesModeReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmWriteAfhChannelAssessmentModeReq * cmPrim;

    cmPrim                  = (CsrBtCmWriteAfhChannelAssessmentModeReq *) cmInst->recvMsgP;
    cmInst->dmVar.appHandle = cmPrim->appHandle;
    dm_hci_write_afh_channel_class_m(cmPrim->classMode, NULL);
}

void CsrBtCmReadAfhChannelMapCfmHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmReadAfhChannelMapCfm * cmPrim;
    DM_HCI_READ_AFH_CHANNEL_MAP_CFM_T * dmPrim;

    cmPrim = (CsrBtCmReadAfhChannelMapCfm *) CsrPmemAlloc(sizeof(CsrBtCmReadAfhChannelMapCfm));
    dmPrim = (DM_HCI_READ_AFH_CHANNEL_MAP_CFM_T *) cmInst->recvMsgP;

    cmPrim->type    = CSR_BT_CM_READ_AFH_CHANNEL_MAP_CFM;
    cmPrim->mode    = dmPrim->mode;
    cmPrim->bd_addr = dmPrim->bd_addr;
    SynMemCpyS(cmPrim->afhMap, 10, dmPrim->map, 10);

    if (dmPrim->status == HCI_SUCCESS)
    {
        cmPrim->resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cmPrim->resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        cmPrim->resultCode     = (CsrBtResultCode) dmPrim->status;
        cmPrim->resultSupplier = CSR_BT_SUPPLIER_HCI;
    }
    CsrBtCmPutMessage(cmInst->dmVar.appHandle, cmPrim);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmReadAfhChannelMapReqHandler(cmInstanceData_t *cmInst)
{
    CsrBtCmReadAfhChannelMapReq * cmPrim;

    cmPrim                  = (CsrBtCmReadAfhChannelMapReq *) cmInst->recvMsgP;
    cmInst->dmVar.appHandle = cmPrim->appHandle;
    dm_hci_read_afh_channel_map(&cmPrim->bd_addr, NULL);
}
#endif
