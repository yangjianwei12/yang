/******************************************************************************
 Copyright (c) 2008-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

static void CsrBtCmReadLocalVersionCfmSend(cmInstanceData_t *cmData, CsrSchedQid phandle,
                      CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtCmReadLocalVersionCfm * prim;

    prim                    = (CsrBtCmReadLocalVersionCfm *)CsrPmemAlloc(sizeof(CsrBtCmReadLocalVersionCfm));
    prim->type              = CSR_BT_CM_READ_LOCAL_VERSION_CFM;
    prim->resultCode        = resultCode ;
    prim->resultSupplier    = resultSupplier ;
    prim->lmpVersion        = cmData->dmVar.lmpVersion;
    prim->hciVersion        = cmData->dmVar.hciVersion;
    prim->hciRevision       = cmData->dmVar.hciRevision;
    prim->manufacturerName  = cmData->dmVar.manufacturerName;
    prim->lmpSubversion     = cmData->dmVar.lmpSubversion;

    CsrBtCmPutMessage(phandle, prim);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmHciReadLocalVersionCompleteHandler(cmInstanceData_t *cmData)
{
    DM_HCI_READ_LOCAL_VER_INFO_CFM_T    *dmPrim;

    dmPrim = (DM_HCI_READ_LOCAL_VER_INFO_CFM_T *) cmData->recvMsgP;

    if (dmPrim->status == HCI_SUCCESS)
    {
        cmData->dmVar.lmpVersion       = dmPrim->lmp_version;
        cmData->dmVar.hciVersion       = dmPrim->hci_version;
        cmData->dmVar.hciRevision      = dmPrim->hci_revision;
        cmData->dmVar.manufacturerName = dmPrim->manuf_name;
        cmData->dmVar.lmpSubversion    = dmPrim->lmp_subversion;
    }
    else
    {
        /* We require Bluetooth 1.2 */
        cmData->dmVar.lmpVersion        = CSR_BT_BLUETOOTH_VERSION_1P2;
        cmData->dmVar.hciRevision       = 0xFFFF ;
        cmData->dmVar.lmpSubversion     = 0xFFFF ;
        cmData->dmVar.manufacturerName  = 0xFFFF ;
    }

    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
    {
        CmInitSequenceHandler(cmData,
                              CM_INIT_SEQ_READ_LOCAL_VER_INFO_CFM,
                              CSR_BT_RESULT_CODE_CM_SUCCESS,
                              CSR_BT_SUPPLIER_CM);
    }
    else
    {
        CsrBtCmReadLocalVersionCfmSend(cmData, cmData->dmVar.appHandle, dmPrim->status, CSR_BT_SUPPLIER_HCI);
    }
}

void CsrBtCmReadLocalVersionReqHandler(cmInstanceData_t *cmData)
{
   
    CsrBtCmReadLocalVersionReq * cmPrim;
    cmPrim = (CsrBtCmReadLocalVersionReq *) cmData->recvMsgP;

    if( (cmData->dmVar.hciRevision == 0xFFFF) &&
            (cmData->dmVar.lmpSubversion == 0xFFFF) &&
            (cmData->dmVar.manufacturerName == 0xFFFF))
    {
        cmData->dmVar.appHandle = cmPrim->appHandle;
        dm_hci_read_local_version(NULL);
    }
    else
    {
        CsrBtCmReadLocalVersionCfmSend(cmData, cmPrim->appHandle, HCI_SUCCESS, CSR_BT_SUPPLIER_HCI);
    }

}






