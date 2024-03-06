/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_events_handler.h"

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_cm_bnep.h"
#endif

#include "csr_bt_cm_util.h"
#include "csr_bt_cm_sdc.h"

static void csrBtCmModeChangeCfmMsgSend(cmInstanceData_t *cmData, CsrUint16 interval, CsrUint8 mode,
                                        CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtCmModeChangeCfm    *cmPrim;

    cmPrim = (CsrBtCmModeChangeCfm *)CsrPmemAlloc(sizeof(CsrBtCmModeChangeCfm));

    cmPrim->type            = CSR_BT_CM_MODE_CHANGE_CFM;
    cmPrim->deviceAddr      = cmData->smVar.operatingBdAddr;
    cmPrim->interval        = interval;
    cmPrim->mode            = mode;
    cmPrim->resultCode      = resultCode;
    cmPrim->resultSupplier  = resultSupplier;
    CsrBtCmPutMessage(cmData->smVar.appHandle, cmPrim);
}

#ifdef CSR_BT_INSTALL_CM_LOW_POWER_CONFIG_PUBLIC
void CsrBtCmModeChangeConfigReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmModeChangeConfigReq *cmPrim = (CsrBtCmModeChangeConfigReq *) cmData->recvMsgP;
    if (!CmDuHandleAutomaticProcedure(cmData,
                                       CM_DU_AUTO_EVENT_MODE_CHANGE_CONFIG,
                                       (void *)cmPrim,
                                       &cmPrim->deviceAddr))
    {
        /* This feature is unsupported as device utility low power management is not enabled. In this case,
         * the low power handling is always carried out from application and CM doesn't control by itself.*/
        CsrBtCmModeChangeConfigCfm *cfm = (CsrBtCmModeChangeConfigCfm *)CsrPmemAlloc(sizeof(CsrBtCmModeChangeConfigCfm));

        cfm->type            = CSR_BT_CM_MODE_CHANGE_CONFIG_CFM;
        cfm->deviceAddr      = cmPrim->deviceAddr;
        cfm->resultCode      = CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE;
        cfm->resultSupplier  = CSR_BT_SUPPLIER_CM;
        CsrBtCmPutMessage(cmPrim->phandle, cfm);
    }
}

static void cmHciSnifffReqSend(cmInstanceData_t *cmData,
                               CsrBtSniffSettings sniffSettings,
                               CsrBtDeviceAddr *deviceAddr)
{
    aclTable *aclConnectionElement;

    returnAclConnectionElement(cmData, *deviceAddr, &aclConnectionElement);

    if (aclConnectionElement && sniffSettings.max_interval > aclConnectionElement->lsto)
    { /* The Sniff_Max_Interval shall be less than the Link Supervision Timeout */
        sniffSettings.max_interval = (CsrUint16) (aclConnectionElement->lsto - CM_HCI_SNIFF_DRAWBACK);
    }

    if (sniffSettings.max_interval > CM_HCI_MAX_SNIFF_INTERVAL)
    {
        sniffSettings.max_interval = CM_HCI_MAX_SNIFF_INTERVAL;
    }

    if (sniffSettings.min_interval > sniffSettings.max_interval)
    {
        sniffSettings.min_interval = (CsrUint16) (sniffSettings.max_interval - CM_HCI_SNIFF_DRAWBACK);
    }

    if (sniffSettings.min_interval < CM_HCI_MIN_SNIFF_INTERVAL)
    {
        sniffSettings.min_interval = CM_HCI_MIN_SNIFF_INTERVAL;
    }

    if (sniffSettings.attempt > CM_HCI_MAX_SNIFF_ATTEMPT)
    {
        sniffSettings.attempt = CM_HCI_MAX_SNIFF_ATTEMPT;
    }
    else if (sniffSettings.attempt < CM_HCI_MIN_SNIFF_ATTEMPT)
    {
        sniffSettings.attempt = CM_HCI_MIN_SNIFF_ATTEMPT;
    }

    if (sniffSettings.timeout > CM_HCI_MAX_SNIFF_TIMEOUT)
    {
        sniffSettings.timeout = CM_HCI_MAX_SNIFF_TIMEOUT;
    }

    /* Note only even values are valid */
    sniffSettings.max_interval = (sniffSettings.max_interval & CM_HCI_MAX_SNIFF_INTERVAL);
    sniffSettings.min_interval = (sniffSettings.min_interval & CM_HCI_MAX_SNIFF_INTERVAL);

    dm_hci_sniff_mode(deviceAddr,
                      sniffSettings.max_interval,
                      sniffSettings.min_interval,
                      sniffSettings.attempt,
                      sniffSettings.timeout, NULL);
}

void CsrBtCmModeChangeReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmModeChangeReq *cmPrim = (CsrBtCmModeChangeReq *) cmData->recvMsgP;
    aclTable *aclConnectionElement;

    cmData->dmVar.appHandle = cmPrim->phandle;

    if (returnAclConnectionElement(cmData, cmPrim->deviceAddr, &aclConnectionElement) != CM_ERROR)
    {
        if (cmPrim->mode == CSR_BT_SNIFF_MODE || cmPrim->mode == CSR_BT_ACTIVE_MODE)
        {
            switch (aclConnectionElement->mode)
            {
                case CSR_BT_ACTIVE_MODE:
                {
                    if (cmPrim->mode == CSR_BT_SNIFF_MODE)
                    {
                        /* Try to go to SNIFF mode */
                        cmHciSnifffReqSend(cmData, cmPrim->sniffSettings, &cmPrim->deviceAddr);

                        /* SM will be unlocked, once the mode change is completed.*/
                        return;
                    }
                    else
                    {
                        /* The ACL connection is already in active mode */
                        csrBtCmModeChangeCfmMsgSend(cmData,
                                                    aclConnectionElement->interval,
                                                    CSR_BT_ACTIVE_MODE,
                                                    CSR_BT_RESULT_CODE_CM_SUCCESS,
                                                    CSR_BT_SUPPLIER_CM);
                    }
                    break;
                }

                case CSR_BT_SNIFF_MODE:
                {
                    if (cmPrim->mode == CSR_BT_SNIFF_MODE && !cmPrim->forceSniffSettings)
                    {
                        /* The mode is the same as the requested mode and the
                         * application don't want to force new sniff setting.
                         * Inform the application */
                        csrBtCmModeChangeCfmMsgSend(cmData,
                                                    aclConnectionElement->interval,
                                                    CSR_BT_SNIFF_MODE,
                                                    CSR_BT_RESULT_CODE_CM_SUCCESS,
                                                    CSR_BT_SUPPLIER_CM);
                    }
                    else
                    {
                        /* Either the application has requested SNIFF mode with new
                         * setting that must be forced or it has requested ACTIVE mode.*/
                        if (cmPrim->mode == CSR_BT_SNIFF_MODE)
                        {
                            cmHciSnifffReqSend(cmData, cmPrim->sniffSettings, &cmPrim->deviceAddr);
                        }
                        else
                        {
                            dm_hci_exit_sniff_mode(&cmPrim->deviceAddr, NULL);
                        }

                        /* SM will be unlocked, once the mode change is completed.*/
                        return;
                    }
                    break;
                }

                default:
                {
                    csrBtCmModeChangeCfmMsgSend(cmData,
                                                aclConnectionElement->interval,
                                                aclConnectionElement->mode,
                                                CSR_BT_RESULT_CODE_CM_INTERNAL_ERROR,
                                                CSR_BT_SUPPLIER_CM);
                    break;
                }
            }
        }
        else
        {
            /* The mode requested by the application is invalid */
            csrBtCmModeChangeCfmMsgSend(cmData,
                                        aclConnectionElement->interval,
                                        aclConnectionElement->mode,
                                        CSR_BT_RESULT_CODE_CM_UNACCEPTABLE_PARAMETER,
                                        CSR_BT_SUPPLIER_CM);
        }
    }
    else
    {
        /* No ACL connection, return Error */
        csrBtCmModeChangeCfmMsgSend(cmData,
                                    0,
                                    CSR_BT_ACTIVE_MODE,
                                    CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER,
                                    CSR_BT_SUPPLIER_CM);
    }

    /* Reaching here means the command is unsuccesfull, restore the respective queue.*/
    CsrBtCmServiceManagerLocalQueueHandler(cmData);
}
#endif /* CSR_BT_INSTALL_CM_LOW_POWER_CONFIG_PUBLIC */

CsrUint8 CmDmGetActualMode(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr)
{
    aclTable *aclConnectionElement;

    if (returnAclConnectionElement(cmData, *deviceAddr, &aclConnectionElement) != CM_ERROR)
    {
        return aclConnectionElement->mode;
    }

    return CSR_BT_ACTIVE_MODE;
}

static void csrBtCmDmStoreActualMode(cmInstanceData_t *cmData)
{ /* Store the actual mode identify by the given device address */
    DM_HCI_MODE_CHANGE_EVENT_IND_T *dmPrim;
    aclTable *aclConnectionElement;

    dmPrim    = (DM_HCI_MODE_CHANGE_EVENT_IND_T *) cmData->recvMsgP;

    returnAclConnectionElement(cmData, dmPrim->bd_addr, &aclConnectionElement);

    if (aclConnectionElement)
    {
        aclConnectionElement->mode     = dmPrim->mode;
        aclConnectionElement->interval = dmPrim->length;
    }
}

void CsrBtCmDmHciModeChangeEventHandler(cmInstanceData_t *cmData)
{
    DM_HCI_MODE_CHANGE_EVENT_IND_T *dmPrim = (DM_HCI_MODE_CHANGE_EVENT_IND_T *) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE
    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropgateModeChangeEvents,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE,
                         dmPrim->status,
                         dmPrim,
                         NULL);
#endif

    /* Store the current mode */
    csrBtCmDmStoreActualMode(cmData);

    /* Check if device utility wants to perform any operation on this event.*/
    (void)CmDuHandleAutomaticProcedure(cmData,
                                       CM_DU_AUTO_EVENT_MODE_CHANGE,
                                       (void *) dmPrim,
                                       &dmPrim->bd_addr);

    if (cmData->smVar.smInProgress &&
        cmData->smVar.smMsgTypeInProgress == CSR_BT_CM_MODE_CHANGE_REQ)
    {
        csrBtCmModeChangeCfmMsgSend(cmData,
                                    dmPrim->length,
                                    dmPrim->mode,
                                    dmPrim->status == HCI_SUCCESS ? CSR_BT_RESULT_CODE_CM_SUCCESS: dmPrim->status,
                                    dmPrim->status == HCI_SUCCESS ? CSR_BT_SUPPLIER_CM: CSR_BT_SUPPLIER_HCI);
        CsrBtCmServiceManagerLocalQueueHandler(cmData);
    }
}

