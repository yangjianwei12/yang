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
#include "csr_bt_cm_sdc.h"
#include "csr_bt_cm_util.h"

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_cm_bnep.h"
#endif

#include "csr_bt_cm_events_handler.h"

#ifdef CSR_BT_INSTALL_CM_LINK_POLICY
static void insertDefaultLinkPolicySettings(cmInstanceData_t *cmData)
{
    CsrUintFast8 i;
    aclTable  *aclConnectionElement = NULL;

    for ( i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if (!CsrBtBdAddrEqZero(&(cmData->roleVar.aclVar[i].deviceAddr)))
        {
            aclConnectionElement                        = &(cmData->roleVar.aclVar[i]);
            aclConnectionElement->linkPolicySettings    = cmData->dmVar.defaultLinkPolicySettings;

#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(INSTALL_CM_INTERNAL_LPM)
            aclConnectionElement->sniffSettings         = cmData->dmVar.defaultSniffSettings;
#endif /* INSTALL_CM_DEVICE_UTILITY && INSTALL_CM_INTERNAL_LPM */
        }
    }
}

static void csrBtCmWriteLinkPolicyErrorIndMsgSend(CsrSchedQid appHandle,
                                                  CsrBtDeviceAddr deviceAddr,
                                                  CsrBtResultCode resultCode,
                                                  CsrBtSupplier resultSupplier)
{
    CsrBtCmWriteLinkPolicyErrorInd * prim;

    prim                 = (CsrBtCmWriteLinkPolicyErrorInd *)CsrPmemAlloc(sizeof(CsrBtCmWriteLinkPolicyErrorInd));
    prim->type           = CSR_BT_CM_WRITE_LINK_POLICY_ERROR_IND;
    prim->deviceAddr     = deviceAddr;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;
    CsrBtCmPutMessage(appHandle, prim);
}
#endif

void CsrBtCmDmHciWriteLpSettingsCompleteHandler(cmInstanceData_t *cmData)
{
#ifdef CSR_BT_INSTALL_CM_LINK_POLICY
    {
        DM_HCI_WRITE_LINK_POLICY_SETTINGS_CFM_T *dmPrim = (DM_HCI_WRITE_LINK_POLICY_SETTINGS_CFM_T*) cmData->recvMsgP;

        if (dmPrim->status != HCI_SUCCESS)
        {
            csrBtCmWriteLinkPolicyErrorIndMsgSend(cmData->dmVar.appHandle,
                                                  cmData->dmVar.operatingBdAddr,
                                                  dmPrim->status,
                                                  CSR_BT_SUPPLIER_HCI);
        }
    }
#endif /* CSR_BT_INSTALL_CM_LINK_POLICY */

    if (cmData->dmVar.lockMsg == CM_DM_WRITE_LINK_POLICY_REQ)
    {
        CsrBtCmDmLocalQueueHandler();
    }
}

#ifdef CSR_BT_INSTALL_CM_LINK_POLICY
void CmDmWriteLinkPolicyReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmWriteLinkPolicyReq *cmPrim = (CsrBtCmWriteLinkPolicyReq *) cmData->recvMsgP;

    if (CsrBtBdAddrEqZero(&cmPrim->deviceAddr))
    {
        link_policy_settings_t policy = cmPrim->linkPolicySetting & (ENABLE_SNIFF | ENABLE_MS_SWITCH);

        if (cmPrim->setupLinkPolicySetting)
        {
            /* cmData->dmVar.defaultLinkPolicySettings is ONLY use to control 
             * which low Power modes are supported */
            cmData->dmVar.defaultLinkPolicySettings = cmPrim->linkPolicySetting;
        }

        /* Check if device utility wants to process anything extra for the automatic procedures.*/
        (void)CmDuHandleAutomaticProcedure(cmData,
                                           CM_DU_AUTO_EVENT_WRITE_DEFAULT_LP,
                                           (void *)cmData->recvMsgP,
                                           &cmPrim->deviceAddr);

        insertDefaultLinkPolicySettings(cmData);
        cmData->dmVar.appHandle = cmPrim->appHandle;
        cmData->dmVar.operatingBdAddr = cmPrim->deviceAddr;

        /* This sets both DM and HCI default policy */
        dm_set_default_link_policy_req(policy, policy, NULL);

        /* Wait for the default lp settings command to complete.*/
        return;
    }
    else
    {
        aclTable *aclConnectionElement;

        if (returnAclConnectionElement(cmData, cmPrim->deviceAddr, &aclConnectionElement) != CM_ERROR)
        {
            cmData->dmVar.appHandle = cmPrim->appHandle;
            cmData->dmVar.operatingBdAddr = cmPrim->deviceAddr;

            if (cmPrim->setupLinkPolicySetting)
            {
                aclConnectionElement->linkPolicySettings = cmPrim->linkPolicySetting;
            }

            /* Check if device utility wants to process anything extra for the automatic procedures.*/
            (void)CmDuHandleAutomaticProcedure(cmData,
                                               CM_DU_AUTO_EVENT_WRITE_LP,
                                               (void *)cmData->recvMsgP,
                                               &cmPrim->deviceAddr);
            dm_hci_write_lp_settings(&cmPrim->deviceAddr, aclConnectionElement->linkPolicySettings, NULL);

            /* Wait for the lp settings command to complete.*/
            return;
        }
        else
        {
            csrBtCmWriteLinkPolicyErrorIndMsgSend(cmPrim->appHandle,
                                                  cmPrim->deviceAddr,
                                                  CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER,
                                                  CSR_BT_SUPPLIER_CM);
        }
    }

    CsrBtCmDmLocalQueueHandler();
}

#ifdef CSR_BT_INSTALL_CM_READ_LP
void CmDmReadLinkPolicyReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmReadLinkPolicyReq        * cmPrim;
    CsrBtCmReadLinkPolicyCfm        * prim;

    cmPrim                          = (CsrBtCmReadLinkPolicyReq *) cmData->recvMsgP;
    prim                            = (CsrBtCmReadLinkPolicyCfm *)CsrPmemZalloc(sizeof(CsrBtCmReadLinkPolicyCfm));
    prim->type                      = CSR_BT_CM_READ_LINK_POLICY_CFM;
    prim->deviceAddr                = cmPrim->deviceAddr;
    prim->resultSupplier            = CSR_BT_SUPPLIER_CM;

    if (CsrBtBdAddrEqZero(&cmPrim->deviceAddr))
    {
        prim->resultCode            = CSR_BT_RESULT_CODE_CM_SUCCESS;
        prim->actualMode            = CSR_BT_ACTIVE_MODE;
        prim->linkPolicySetting     = cmData->dmVar.defaultLinkPolicySettings;
#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(INSTALL_CM_INTERNAL_LPM)
        prim->sniffSettings         = cmData->dmVar.defaultSniffSettings;
#endif /* INSTALL_CM_DEVICE_UTILITY && INSTALL_CM_INTERNAL_LPM */
    }
    else
    {
        aclTable *aclConnectionElement;

        if (returnAclConnectionElement(cmData, cmPrim->deviceAddr, &aclConnectionElement) != CM_ERROR)
        {
            prim->resultCode        = CSR_BT_RESULT_CODE_CM_SUCCESS;
            prim->actualMode        = CmDmGetActualMode(cmData, &cmPrim->deviceAddr);
            prim->linkPolicySetting = aclConnectionElement->linkPolicySettings;
#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(INSTALL_CM_INTERNAL_LPM)
            prim->sniffSettings     = aclConnectionElement->sniffSettings;
#endif /* INSTALL_CM_DEVICE_UTILITY && INSTALL_CM_INTERNAL_LPM */
        }
        else
        {
            prim->resultCode        = CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER;
            prim->actualMode        = CSR_BT_ACTIVE_MODE;
            prim->linkPolicySetting = cmData->dmVar.defaultLinkPolicySettings;
#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(INSTALL_CM_INTERNAL_LPM)
            prim->sniffSettings     = cmData->dmVar.defaultSniffSettings;
#endif /* INSTALL_CM_DEVICE_UTILITY && INSTALL_CM_INTERNAL_LPM */
        }
    }
    CsrBtCmPutMessage(cmPrim->appHandle, prim);
    CsrBtCmDmLocalQueueHandler();
}
#endif /* CSR_BT_INSTALL_CM_READ_LP */
#endif /* CSR_BT_INSTALL_CM_LINK_POLICY */

void CsrBtCmDmHciWriteDefaultLinkPolicySettingsCompleteHandler(cmInstanceData_t *cmData)
{
    DM_HCI_WRITE_DEFAULT_LINK_POLICY_SETTINGS_CFM_T *dmPrim;
    dmPrim = (DM_HCI_WRITE_DEFAULT_LINK_POLICY_SETTINGS_CFM_T*)cmData->recvMsgP;

    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
    {
        if (dmPrim->status != HCI_SUCCESS)
        {
            /* Can't do much but throw an exception and continue */
            CsrBtCmGeneralException(DM_PRIM,
                                    dmPrim->type,
                                    0,
                                    "HCI command failed");
        }

        /* We are currently in CM initialization phase, continue with the sequence. */
        CmInitSequenceHandler(cmData,
                              CM_INIT_SEQ_WRITE_DEFAULT_LINK_POLICY_SETTINGS_CFM,
                              dmPrim->status,
                              CSR_BT_SUPPLIER_HCI);
    }
    else
    {
        /* Normal CM operation. Send error indication to application
         * in case the HCI command failed, otherwise keep quiet as
         * there is no CFM for this */
#ifdef CSR_BT_INSTALL_CM_LINK_POLICY
        if((dmPrim->status != HCI_SUCCESS) &&
           (cmData->smVar.appHandle != CSR_BT_CM_IFACEQUEUE))
        {
            csrBtCmWriteLinkPolicyErrorIndMsgSend(cmData->smVar.appHandle,
                                                  cmData->smVar.operatingBdAddr,
                                                  dmPrim->status,
                                                  CSR_BT_SUPPLIER_HCI);
        }
#endif
        if (cmData->dmVar.lockMsg == CM_DM_WRITE_LINK_POLICY_REQ)
        {
            CsrBtCmDmLocalQueueHandler();
        }
    }
}

#ifdef CSR_BT_INSTALL_CM_SWITCH_ROLE_PUBLIC
void CsrBtCmAlwaysMasterDevicesReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmAlwaysMasterDevicesReq * cmPrim = (CsrBtCmAlwaysMasterDevicesReq *) cmData->recvMsgP;
    cmData->dmVar.appHandle                = cmPrim->phandle; 
    cmData->dmVar.operatingBdAddr          = cmPrim->deviceAddr; 
    dm_lp_write_always_master_devices_req(cmPrim->operation,
                                          &cmPrim->deviceAddr,
                                          NULL);
}

void CsrBtCmAlwaysMasterDevicesCfmHandler(cmInstanceData_t *cmData)
{
    DM_LP_WRITE_ALWAYS_MASTER_DEVICES_CFM_T *dmPrim = (DM_LP_WRITE_ALWAYS_MASTER_DEVICES_CFM_T*) cmData->recvMsgP;

    CsrBtCmAlwaysMasterDevicesCfm *cmPrim = (CsrBtCmAlwaysMasterDevicesCfm *)
                                             CsrPmemAlloc(sizeof(CsrBtCmAlwaysMasterDevicesCfm));


    cmPrim->type            = CSR_BT_CM_ALWAYS_MASTER_DEVICES_CFM;
    cmPrim->deviceAddr      = cmData->dmVar.operatingBdAddr;
    cmPrim->resultSupplier  = CSR_BT_SUPPLIER_CM;
    cmPrim->resultCode      = CSR_BT_RESULT_CODE_CM_SUCCESS;  
    
    if(dmPrim->status != HCI_SUCCESS)
    {
        cmPrim->resultSupplier = CSR_BT_SUPPLIER_HCI;
        cmPrim->resultCode     = (CsrBtResultCode) dmPrim->status;
    }
    CsrBtCmPutMessage(cmData->dmVar.appHandle, cmPrim);
    CsrBtCmDmLocalQueueHandler();
}
#endif /* CSR_BT_INSTALL_CM_SWITCH_ROLE_PUBLIC */

