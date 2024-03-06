/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_cm_bnep.h"
#endif

#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#include "csr_bt_cm_bccmd.h"
#endif

#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif

#define WRITE_SCAN_ENABLE_MAX_RETRY 10

#ifdef CSR_BT_INSTALL_CM_WRITE_COD
#define CSR_BT_CM_COD_STATE_SET(_dmVar, _state)     ((_dmVar).writingCod = (_state))
#define CSR_BT_CM_COD_STATE_IS_SET(_dmVar)          ((_dmVar).writingCod != FALSE)
#else
#define CSR_BT_CM_COD_STATE_SET(_dmVar, _state)
#define CSR_BT_CM_COD_STATE_IS_SET(_dmVar)          TRUE
#endif

#ifdef CSR_BT_INSTALL_CM_READ_COD
void CsrBtCmReadCodReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmReadCodReq  *prim;

    prim = (CsrBtCmReadCodReq *) cmData->recvMsgP;

    cmData->dmVar.appHandle = prim->appHandle;
    dm_hci_read_class_of_device(NULL);
}

void CsrBtCmDmHciReadClassOfDeviceCompleteHandler(cmInstanceData_t *cmData)
{ /* Read class of device complete */
    DM_HCI_READ_CLASS_OF_DEVICE_CFM_T * dmPrim = (DM_HCI_READ_CLASS_OF_DEVICE_CFM_T *) cmData->recvMsgP;
    CsrBtCmReadCodCfm                 * cmPrim = (CsrBtCmReadCodCfm *)CsrPmemAlloc(sizeof(CsrBtCmReadCodCfm));

    cmPrim->type            = CSR_BT_CM_READ_COD_CFM;
    cmPrim->classOfDevice   = dmPrim->dev_class;

    if(dmPrim->status == HCI_SUCCESS)
    {
        cmPrim->resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cmPrim->resultSupplier  = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        cmPrim->resultCode      = (CsrBtResultCode) dmPrim->status;
        cmPrim->resultSupplier  = CSR_BT_SUPPLIER_HCI;
    }
    CsrBtCmPutMessage(cmData->dmVar.appHandle, cmPrim);
    CsrBtCmDmLocalQueueHandler();
}
#endif

static void csrBtCmWriteScanEnableCfmMsgSend(CsrSchedQid appHandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtCmWriteScanEnableCfm *msg = (CsrBtCmWriteScanEnableCfm *)CsrPmemAlloc(sizeof(CsrBtCmWriteScanEnableCfm));
    msg->type                      = CSR_BT_CM_WRITE_SCAN_ENABLE_CFM;
    msg->resultCode                = resultCode;
    msg->resultSupplier            = resultSupplier;
    CsrBtCmPutMessage(appHandle, msg);
}

static CsrBool csrBtCmWriteScanEnableCompleteHandler(cmInstanceData_t *cmData,
                                                     CsrBtResultCode resultCode,
                                                     CsrBtSupplier resultSupplier)
{
    CsrBool restoreDmQueue = TRUE;
    if (resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
        {
            restoreDmQueue = FALSE;
        }
        else
        {
            csrBtCmWriteScanEnableCfmMsgSend(cmData->dmVar.appHandle, resultCode, resultSupplier);
        }
    }
    else if (cmData->globalState != CSR_BT_CM_STATE_NOT_READY)
    {
        if (cmData->dmVar.retryCounter++ < WRITE_SCAN_ENABLE_MAX_RETRY)
        {
            /* Fail try again */
            CsrUint8 mode = returnConnectAbleParameters(cmData);
            restoreDmQueue = FALSE;
            dm_hci_write_scan_enable(mode, NULL);
        }
        else
        {
            csrBtCmWriteScanEnableCfmMsgSend(cmData->dmVar.appHandle, resultCode, resultSupplier);
        }
    }

    return restoreDmQueue;
}

void CsrBtCmWriteScanEnableCompleteSwitch(cmInstanceData_t *cmData, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    if (!CmDuHandleCommandComplete(cmData, CM_DU_CMD_COMPLETE_SCAN))
    {
        /* The scan was requested externally and not by any automatic procedures caried out in device utility. */
        if (csrBtCmWriteScanEnableCompleteHandler(cmData, resultCode, resultSupplier))
        {
            CsrBtCmDmLocalQueueHandler();
        }
    }
}

void CsrBtCmDmHciWriteScanEnableCompleteHandler(cmInstanceData_t *cmData)
{
    DM_HCI_WRITE_SCAN_ENABLE_CFM_T    *dmPrim;

    dmPrim = (DM_HCI_WRITE_SCAN_ENABLE_CFM_T *)cmData->recvMsgP;

    if(dmPrim->status == HCI_SUCCESS)
    {
        cmData->dmVar.currentChipScanMode = cmData->dmVar.pendingChipScanMode;
    }

#ifdef CSR_BT_INSTALL_CM_DUT_MODE
    if (cmData->dmVar.deviceUnderTest)
    {
        if (dmPrim->status == HCI_SUCCESS)
        {
            dm_hci_enable_device_ut_mode(NULL);
        }
        else
        {
            CsrBtCmSendDeviceUnderTestComplete(cmData->dmVar.appHandle, dmPrim->status, 2);
        }
    }
    else
#endif
    {
        if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
        {
            /* Currently we are in initialization phase, handle this using init sequence handler. */
            CmInitSequenceHandler(cmData,
                                  CM_INIT_SEQ_WRITE_SCAN_ENABLE_CFM,
                                  (CsrBtResultCode) dmPrim->status,
                                  CSR_BT_SUPPLIER_HCI);
        }

        if (dmPrim->status == HCI_SUCCESS)
        {
            CsrBtCmWriteScanEnableCompleteSwitch(cmData, CSR_BT_RESULT_CODE_CM_SUCCESS, CSR_BT_SUPPLIER_CM);
        }
        else
        {
            CsrBtCmWriteScanEnableCompleteSwitch(cmData, (CsrBtResultCode) dmPrim->status, CSR_BT_SUPPLIER_HCI);
        }
    }
}

#ifdef CSR_BT_INSTALL_CM_WRITE_COD
static void csrBtCmWriteClassOfDeviceCfmMsgSend(CsrSchedQid appHandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtCmWriteCodCfm *msg = (CsrBtCmWriteCodCfm *)CsrPmemAlloc(sizeof(CsrBtCmWriteCodCfm));
    msg->type               = CSR_BT_CM_WRITE_COD_CFM;
    msg->resultCode         = resultCode;
    msg->resultSupplier     = resultSupplier;
    CsrBtCmPutMessage(appHandle, msg);
}

void CsrBtCmWriteCodReqHandler(cmInstanceData_t *cmData)
{ /* Find the actual Class Of Device and send the command to the hostcontroller */
    CsrUint24 classOfDevice;
    CsrBtCmWriteCodReq *cmPrim = (CsrBtCmWriteCodReq*) cmData->recvMsgP;

    CSR_BT_CM_COD_STATE_SET(cmData->dmVar, TRUE);
    cmData->dmVar.appHandle = cmPrim->appHandle;

    if (cmPrim->updateFlags & CSR_BT_CM_WRITE_COD_UPDATE_FLAG_MAJOR_MINOR_CLASS)
    {
        cmData->dmVar.majorCod = cmPrim->majorClassOfDevice;
        cmData->dmVar.minorCod = cmPrim->minorClassOfDevice;
    }
    if (cmPrim->updateFlags & CSR_BT_CM_WRITE_COD_UPDATE_FLAG_SERVICE_CLASS)
    {
        cmData->dmVar.serviceCod = cmPrim->serviceClassOfDevice;
    }

    classOfDevice = CsrBtCmReturnClassOfdevice(cmData);

    if (classOfDevice != cmData->dmVar.codWrittenToChip)
    {
        cmData->dmVar.pendingCod = classOfDevice;
        dm_hci_write_class_of_device(classOfDevice, NULL);
    }
    else
    {
        CSR_BT_CM_COD_STATE_SET(cmData->dmVar, FALSE);
        csrBtCmWriteClassOfDeviceCfmMsgSend(cmData->dmVar.appHandle,
                                            CSR_BT_RESULT_CODE_CM_SUCCESS,
                                            CSR_BT_SUPPLIER_CM);
        CsrBtCmDmLocalQueueHandler();
    }
}
#endif /* CSR_BT_INSTALL_CM_WRITE_COD */

void CsrBtCmDmHciWriteClassOfDeviceCompleteHandler(cmInstanceData_t *cmData)
{ /* Write class of device complete */
#if defined(CSR_BT_INSTALL_CM_WRITE_COD) || defined(CSR_BT_INSTALL_CM_PRI_IAC)
    CsrBtResultCode                    resultCode;
    CsrBtSupplier                resultSupplier;
    DM_HCI_WRITE_CLASS_OF_DEVICE_CFM_T *dmPrim = (DM_HCI_WRITE_CLASS_OF_DEVICE_CFM_T *) cmData->recvMsgP;

    if(dmPrim->status == HCI_SUCCESS)
    {
        cmData->dmVar.codWrittenToChip = cmData->dmVar.pendingCod;
        resultCode                     = CSR_BT_RESULT_CODE_CM_SUCCESS;
        resultSupplier                 = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        resultCode                     = (CsrBtResultCode) dmPrim->status;
        resultSupplier                 = CSR_BT_SUPPLIER_HCI;
    }
#endif /* defined(CSR_BT_INSTALL_CM_WRITE_COD) || defined(CSR_BT_INSTALL_CM_PRI_IAC) */

#ifdef CSR_BT_INSTALL_CM_WRITE_COD
    if (CSR_BT_CM_COD_STATE_IS_SET(cmData->dmVar) &&
        cmData->globalState == CSR_BT_CM_STATE_IDLE)
    {
        CSR_BT_CM_COD_STATE_SET(cmData->dmVar, FALSE);
        csrBtCmWriteClassOfDeviceCfmMsgSend(cmData->dmVar.appHandle, resultCode, resultSupplier);
        CsrBtCmDmLocalQueueHandler();
    }
    else
#endif /* CSR_BT_INSTALL_CM_WRITE_COD */
    {
        if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
        {
            CmInitSequenceHandler(cmData,
                                  CM_INIT_SEQ_WRITE_COD_CFM,
                                  (CsrBtResultCode) dmPrim->status,
                                  CSR_BT_SUPPLIER_HCI);
        }
    }
}

#ifdef CSR_BT_INSTALL_CM_READ_SCAN_EANBLE
void CsrBtCmDmHciReadScanEnableCompleteHandler(cmInstanceData_t *cmData)
{
    CsrBtCmReadScanEnableCfm * cmPrim = (CsrBtCmReadScanEnableCfm *) CsrPmemAlloc(sizeof(CsrBtCmReadScanEnableCfm));
    DM_HCI_READ_SCAN_ENABLE_CFM_T * dmPrim = (DM_HCI_READ_SCAN_ENABLE_CFM_T *) cmData->recvMsgP;

    cmPrim->type       = CSR_BT_CM_READ_SCAN_ENABLE_CFM;
    cmPrim->scanEnable = dmPrim->scan_enable;

    if (dmPrim->status == HCI_SUCCESS)
    {
        cmPrim->resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cmPrim->resultSupplier  = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        cmPrim->resultCode      = (CsrBtResultCode) dmPrim->status;
        cmPrim->resultSupplier  = CSR_BT_SUPPLIER_HCI;
    }

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cmPrim);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmReadScanEnableReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmReadScanEnableReq *cmPrim = (CsrBtCmReadScanEnableReq*) cmData->recvMsgP;

    cmData->dmVar.appHandle = cmPrim->appHandle;
    dm_hci_read_scan_enable(NULL);
}
#endif /* CSR_BT_INSTALL_CM_READ_SCAN_EANBLE */

void CsrBtCmWriteScanEnableReqHandler(cmInstanceData_t *cmData)
{
    CsrUint8 mode;
    CsrBtCmWriteScanEnableReq *cmPrim = (CsrBtCmWriteScanEnableReq*) cmData->recvMsgP;

    cmData->dmVar.appHandle = cmPrim->appHandle;
    cmData->dmVar.disableInquiryScan = cmPrim->disableInquiryScan;
    cmData->dmVar.disablePageScan = cmPrim->disablePageScan;
    cmData->dmVar.retryCounter = 0;

    mode = returnConnectAbleParameters(cmData);

    if (mode != cmData->dmVar.currentChipScanMode)
    {
        cmData->dmVar.pendingChipScanMode = mode;
        dm_hci_write_scan_enable(mode, NULL);
    }
    else
    {
        CsrBtCmWriteScanEnableCompleteSwitch(cmData,
                                             CSR_BT_RESULT_CODE_CM_SUCCESS,
                                             CSR_BT_SUPPLIER_CM);
    }
}

