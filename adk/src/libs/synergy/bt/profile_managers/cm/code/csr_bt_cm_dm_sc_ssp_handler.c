/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "dmlib.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_rfc.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_prim.h"
#endif
#include "csr_bt_cm_l2cap.h"

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_cm_bnep.h"
#endif

#include "csr_bt_cm_dm_sc_ssp_handler.h"
#include "csr_bt_cm_events_handler.h"
#include "csr_bt_cm_le.h"

#ifdef INSTALL_CM_KEY_REQUEST_INDICATION
void CmSmKeyRequestRspHandler(cmInstanceData_t *cmData)
{
    CmSmKeyRequestRsp *cmPrim;

    cmPrim = (CmSmKeyRequestRsp*)cmData->recvMsgP;
    if (cmPrim->keyAvailable)
    {
        dm_sm_key_request_rsp(&cmPrim->tpAddrt,
                              cmPrim->secRequirements,
                              cmPrim->keyType,
                              cmPrim->key,
                              NULL);
    }
    else
    {
        dm_sm_key_request_neg_rsp(&cmPrim->tpAddrt, 
                                  cmPrim->keyType, 
                                  NULL);
    }
}
#endif /* INSTALL_CM_KEY_REQUEST_INDICATION */

/* Relay security request to SM/DM_SM */
#ifdef CSR_BT_LE_ENABLE
void CsrBtCmSmLeSecurityReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmLeSecurityReq *cmPrim;
    cmPrim = (CsrBtCmSmLeSecurityReq*)cmData->recvMsgP;
    dm_sm_security_req(CSR_BT_CM_IFACEQUEUE,
                       &cmPrim->addr,
                       cmPrim->l2caConFlags,
                       cmPrim->context,
                       cmPrim->securityRequirements,
                       NULL);
}
#endif

void CsrBtCmSmBondingReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmBondingReq *cmPrim = (CsrBtCmSmBondingReq *) cmData->recvMsgP;
    TYPED_BD_ADDR_T ad;

    cmData->dmVar.appHandle = CSR_BT_CM_SC_HANDLE(cmData);
    cmData->dmVar.operatingBdAddr = cmPrim->deviceAddr;

    if (!CmDuHandleAutomaticProcedure(cmData,
                                       CM_DU_AUTO_EVENT_SM_BONDING,
                                       (void *)cmPrim,
                                       &cmPrim->deviceAddr))
    {
        CsrBtCmDmLocalQueueHandler();
    }

    CSR_BT_CM_STATE_CHANGE(cmData->dmVar.state, CSR_BT_CM_DM_STATE_CONNECT);

    ad.addr = cmPrim->deviceAddr;
    ad.type = CSR_BT_ADDR_PUBLIC;
    dm_sm_bonding_req(&ad, NULL);
}

void CsrBtCmDmBondingCfm(cmInstanceData_t *cmData,
                         BD_ADDR_T *p_bd_addr, CsrUint8 status)
{
    CsrBtCmSmBondingCfm *p_prim;
    p_prim = CsrPmemAlloc(sizeof(CsrBtCmSmBondingCfm));
    p_prim->type = CSR_BT_CM_SM_BONDING_CFM;
    p_prim->phandle = CSR_BT_CM_IFACEQUEUE;
    p_prim->status = status;
    p_prim->flags = 0;

    CsrBtBdAddrCopy(&p_prim->addrt.addr, p_bd_addr);
    p_prim->addrt.type = CSR_BT_ADDR_PUBLIC;

    CsrSchedMessagePut(CSR_BT_CM_SC_HANDLE(cmData), CSR_BT_CM_PRIM, p_prim);
}

void CsrBtCmDmSmBondingScStateHandler(cmInstanceData_t * cmData)
{
    DM_SM_BONDING_CFM_T * prim;

    prim = (DM_SM_BONDING_CFM_T *) cmData->recvMsgP;

    if (prim != NULL &&
        CsrBtBdAddrEq(&cmData->dmVar.operatingBdAddr, &prim->addrt.addr) &&
        cmData->dmVar.state == CSR_BT_CM_DM_STATE_CONNECT)
    {
        CSR_BT_CM_STATE_CHANGE(cmData->dmVar.state, CSR_BT_CM_DM_STATE_NULL);
        cmData->dmVar.cancel = FALSE;
    }

    CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_BONDING_CFM);
    /* DM queue not locked during bonding, so do not unlock it here */
}

void CsrBtCmSmBondingCancelReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmBondingCancelReq    *prim;

    prim = (CsrBtCmSmBondingCancelReq *) cmData->recvMsgP;
    if (CsrBtBdAddrEq(&cmData->dmVar.cacheTargetDev, &prim->deviceAddr) &&
        cmData->dmVar.state != CSR_BT_CM_DM_STATE_NULL)
    {
        TYPED_BD_ADDR_T ad;
        ad.addr = prim->deviceAddr;
        ad.type = prim->addressType;
        dm_sm_bonding_cancel_req(&ad,
                                 prim->force,
                                 NULL);
        cmData->dmVar.cancel = TRUE;
    }
    else
    {
        /* ACL already up or unknown */
        ;
    }
}


#ifdef CSR_BT_INSTALL_CM_SC_MODE_CONFIG
void CsrBtCmSmSecModeConfigReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmSecModeConfigReq    *prim;

    prim = (CsrBtCmSmSecModeConfigReq *) cmData->recvMsgP;

    /* Only used in testing */
    dm_sm_init_req(DM_SM_INIT_WRITE_AUTH_ENABLE | DM_SM_INIT_CONFIG, /* options */
                   0, /* mode (not specified) */
                   0, /* security_level_default (not specified) */
                   prim->config,
                   prim->writeAuthEnable,
                   0, /* mode3_enc (not specified) */
                   NULL, /* sm_key_state */
                   0, /* sm_div_state*/                  
                   NULL);
}
#endif

#ifdef EXCLUDE_CSR_BT_SC_MODULE
static CsrBool cmSmHandleBond(cmInstanceData_t *cmData,
                              CsrBtDeviceAddr *addr,
                              CsrUint8 addrType,
                              CsrUint8 transportType,
                              CsrUint8 *authReq)
{
    CsrBtTypedAddr deviceAddr;
    CsrUint8 index, maxNumTransport = 2;
    aclTable *aclConnectionElement;
    leConnVar *conn;
    CsrBool disableBond = FALSE;

    CsrBtAddrCopyWithType(&deviceAddr, addrType, addr);

    if (authReq)
    {
        if (transportType == CSR_BT_TRANSPORT_LE)
        {
            if (!((*authReq) & DM_SM_SECURITY_BONDING))
            {
                disableBond = TRUE;
            }
        }
        else if (transportType == CSR_BT_TRANSPORT_BREDR)
        {
            if ((*authReq) == HCI_MITM_REQUIRED_NO_BONDING ||
                    (*authReq) == HCI_MITM_NOT_REQUIRED_NO_BONDING)
            {
                disableBond = TRUE;
            }
        }
    }

    for (index = 0; index < maxNumTransport; index++)
    {
        switch (transportType)
        {
            case CSR_BT_TRANSPORT_BREDR:
            {
                returnAclConnectionElement(cmData, (*addr), &aclConnectionElement);
                if (aclConnectionElement)
                {
                    /* If index != 0, it means that ACL connection does not exist for specified transport and
                     * keys are getting generated using CTKD In this scenario, bondRequired flag should not be updated
                     * and bondRequired value should be taken from the transport on which ACL established.
                     */
                    if (disableBond && !index && aclConnectionElement->bondRequired)
                    {
                        aclConnectionElement->bondRequired = FALSE;
                    }
                    return aclConnectionElement->bondRequired;
                }
                else
                {
                    /* ACL connection does not exist for BREDR transport, so BREDR keys might be getting generated 
                     * using CTKD. Set the transportType to LE to get the bond requirements.
                     */
                    transportType = CSR_BT_TRANSPORT_LE;
                }
            }
            break;

            case CSR_BT_TRANSPORT_LE:
            {
                conn = CsrBtCmLeFindConn(cmData, &deviceAddr);
                if (conn)
                {
                    /* If index != 0, it means that ACL connection does not exist for specified transport and
                     * keys are getting generated using CTKD. In this scenario, bondRequired flag should not be updated
                     * and bondRequired value should be taken from the transport on which ACL established.
                     */
                    if (disableBond && !index && conn->bondRequired)
                    {
                        conn->bondRequired = FALSE;
                    }
                   return conn->bondRequired;
                }
                else
                {
                    /* ACL connection does not exist for LE transport, so LE keys might be getting generated 
                     * using CTKD. Set the transportType to BREDR to get the bond requirements.
                     */
                    transportType = CSR_BT_TRANSPORT_BREDR;
                }
            }
            break;

            default:
            break;
        }
    }
    return TRUE;
}

CsrBool CmSmIsBondEnabled(cmInstanceData_t *cmData,
                          CsrBtDeviceAddr *addr,
                          CsrUint8 addrType,
                          CsrUint8 transportType)
{
    /* Pass the authReq as NULL as just want to retrieve the bond requirement */
    return cmSmHandleBond(cmData,
                          addr,
                          addrType,
                          transportType,
                          NULL);
}
#endif /* EXCLUDE_CSR_BT_SC_MODULE */

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1
void CsrBtCmSmIoCapabilityRequestResHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmIoCapabilityRequestRes *prim;
    TP_BD_ADDR_T tpAddr;

    prim = (CsrBtCmSmIoCapabilityRequestRes *) cmData->recvMsgP;
    tpAddr.addrt.addr = prim->deviceAddr;
    tpAddr.addrt.type = prim->addressType;
    tpAddr.tp_type = prim->transportType;
    dm_sm_io_capability_request_rsp(&tpAddr,
                                    prim->ioCapability,
                                    prim->authenticationRequirements,
                                    prim->oobDataPresent,
                                    prim->oobHashC,
                                    prim->oobRandR,
                                    prim->keyDistribution,
                                    NULL);
    prim->oobHashC = NULL;
    prim->oobRandR = NULL;

#ifdef EXCLUDE_CSR_BT_SC_MODULE
    /* Update the bonding based on the authentication requirements provided by application */
    (void) cmSmHandleBond(cmData,
                          &prim->deviceAddr,
                          prim->addressType,
                          prim->transportType,
                          &prim->authenticationRequirements);
#endif
}

void CsrBtCmSmIoCapabilityRequestNegResHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmIoCapabilityRequestNegRes    *prim;
    TP_BD_ADDR_T tpAddr;

    prim = (CsrBtCmSmIoCapabilityRequestNegRes *) cmData->recvMsgP;
    tpAddr.addrt.addr = prim->deviceAddr;
    tpAddr.addrt.type = prim->addressType;
    tpAddr.tp_type = prim->transportType;
    dm_sm_io_capability_request_neg_rsp(&tpAddr,
                                        prim->reason,
                                        NULL);
}
#ifdef CSR_BT_INSTALL_CM_OOB
void CsrBtCmSmReadLocalOobDataReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmReadLocalOobDataReq *prim = (CsrBtCmSmReadLocalOobDataReq *) cmData->recvMsgP;
    dm_sm_read_local_oob_data_req(prim->transportType, NULL);
}
#endif
void CsrBtCmSmSendKeypressNotificationReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmSendKeypressNotificationReq    *prim;
    TP_BD_ADDR_T tpAddr;

    prim = (CsrBtCmSmSendKeypressNotificationReq *) cmData->recvMsgP;
    tpAddr.addrt.addr = prim->deviceAddr;
    tpAddr.addrt.type = prim->addressType;
    tpAddr.tp_type = prim->transportType;
    dm_sm_send_keypress_notification_req(&tpAddr, prim->notificationType, NULL);
}

#ifdef INSTALL_CM_SM_REPAIR
void CsrBtCmSmRepairResHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmRepairRes    * prim;

    prim = (CsrBtCmSmRepairRes *) cmData->recvMsgP;

    if (prim->repairId == cmData->dmVar.rebond.keyMissingId && cmData->smVar.smInProgress)
    {
        switch (cmData->smVar.smMsgTypeInProgress)
        {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
            case CSR_BT_CM_CONNECT_REQ:
            case CSR_BT_CM_CONNECT_EXT_REQ:
            {
                cmRfcConnElement *connElement = CM_RFC_ELEMENT_ACTIVE(cmData);

                if (connElement && connElement->cmRfcConnInst)
                {
                    cmRfcConnInstType *theLogicalLink = connElement->cmRfcConnInst;
                    if (cmData->rfcVar.connectState == CM_RFC_SSP_REPAIR)
                    {
                        if (prim->accept)
                        { /* Repairing has been performed                   */
                            aclTable *aclConnectionElement;
                            CsrUint8 featIndex = LMP_FEATURES_SIMPLE_PAIRING_BIT/8;
                            CsrUint8 featOffsetBit = LMP_FEATURES_SIMPLE_PAIRING_BIT%8;

                            CSR_BT_CM_STATE_CHANGE(cmData->rfcVar.connectState, CM_RFC_IDLE);
                            returnAclConnectionElement(cmData, theLogicalLink->deviceAddr, &aclConnectionElement);

                            if ((aclConnectionElement && !CSR_BIT_IS_SET(aclConnectionElement->remoteFeatures[featIndex], featOffsetBit)) ||
                                !aclConnectionElement)
                            {
                                CsrBtCmRfcStartInitiateConnection(cmData, theLogicalLink);
                            }
                            else /* if (aclConnectionElement) */
                            {
                                CSR_BT_CM_STATE_CHANGE(theLogicalLink->state,
                                                       CSR_BT_CM_RFC_STATE_ACCESS);
                                CsrBtCmDmSmAccessReqMsgSend();
                            }
                        }
                        else
                        { /* Repairing has been rejected or fail            */
                            cmData->smVar.arg.result.code     = CSR_BT_RESULT_CODE_CM_REBOND_REJECTED_BY_APPLICATION;
                            cmData->smVar.arg.result.supplier = CSR_BT_SUPPLIER_CM;
                            CSR_BT_CM_STATE_CHANGE(cmData->rfcVar.connectState, CM_RFC_CANCELING);
                            CsrBtCmRfcCommonErrorHandler(cmData, theLogicalLink);
                        }
                    }
                    else
                    {
                        CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                                cmData->smVar.smMsgTypeInProgress,
                                                cmData->rfcVar.connectState,
                                                "");
                    }
                }
                else
                {
                    CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                            cmData->smVar.smMsgTypeInProgress,
                                            0,
                                            "No cmRfcConnElement in CsrBtCmSmRepairResHandler");
                }
                break;
            }
#endif

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
            case CSR_BT_CM_L2CA_CONNECT_REQ:
            {
                if (cmData->l2caVar.connectState == CM_L2CA_SSP_REPAIR)
                {
                    cmL2caConnElement *connElement = CM_L2CA_ELEMENT_ACTIVE(cmData);
                    if (connElement && connElement->cmL2caConnInst)
                    {
                        cmL2caConnInstType *l2CaConnection = connElement->cmL2caConnInst;

                        if (prim->accept)
                        { /* Repairing has been performed                   */
                            CSR_BT_CM_STATE_CHANGE(cmData->l2caVar.connectState, CM_L2CA_CONNECT);
                            L2CA_AutoConnectReq(L2CA_CID_INVALID, /* L2CA_CID_INVALID =create new */
                                                l2CaConnection->psm,
                                                &l2CaConnection->deviceAddr,
                                                l2CaConnection->remotePsm,
                                                CM_L2CA_CONNECT_INPROGRESS_CTX,
                                                CSR_BT_AMP_CONTROLLER_BREDR,
                                                CSR_BT_AMP_CONTROLLER_BREDR,
                                                l2CaConnection->conftabIter.size,
                                                CsrMemDup(l2CaConnection->conftabIter.block, l2CaConnection->conftabIter.size*sizeof(CsrUint16)));
                        }
                        else
                        { /* Repairing has been rejected or fail            */
                            CsrBtCmL2CaConnectCfmErrorHandler(cmData,
                                                              connElement,
                                                              CSR_BT_RESULT_CODE_CM_REBOND_REJECTED_BY_APPLICATION,
                                                              CSR_BT_SUPPLIER_CM);
                        }
                    }
                    else
                    {
                        CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                                cmData->smVar.smMsgTypeInProgress,
                                                0,
                                                "No cmL2caConnElement in CsrBtCmSmRepairResHandler");
                    }
                }
                else
                {
                    CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                            CSR_BT_CM_L2CA_CONNECT_REQ,
                                            cmData->l2caVar.connectState,
                                            "");
                }
                break;
            }
#endif

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
            case CSR_BT_CM_BNEP_CONNECT_REQ:
            {
                if (cmData->bnepVar.connectState  == CM_BNEP_SSP_REPAIR)
                {
                    bnepTable *bnepConnection = cmData->bnepVar.indexPtr;
                    ETHER_ADDR remAddr = CsrBtBdAddrToEtherAddr(&(bnepConnection->deviceAddr));

                    if (prim->accept)
                    { /* Repairing has been performed                   */
                        CSR_BT_CM_STATE_CHANGE(cmData->bnepVar.connectState, CM_BNEP_CONNECT);
                        CsrBtBnepConnectReqSend(cmData->bnepVar.connectReqFlags, remAddr);
                    }
                    else
                    { /* Repairing has been rejected or fail            */
                        CsrBtCmBnepConnectErrorIndHandler(cmData, bnepConnection->deviceAddr,
                                              bnepConnection, ID_EMPTY, 0, 0, remAddr,
                                              CSR_BT_RESULT_CODE_CM_REBOND_REJECTED_BY_APPLICATION,
                                              CSR_BT_SUPPLIER_CM);
                    }
                }
                else
                {
                    CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                            CSR_BT_CM_BNEP_CONNECT_REQ,
                                            cmData->bnepVar.connectState,
                                            "");
                }
                break;
            }
#endif
            default:
            {
                CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                        cmData->smVar.smMsgTypeInProgress,
                                        cmData->globalState,
                                        "");
                break;
            }
        }
    }
    else
    { /* The Id don't match, just ignore this request   */
        CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                cmData->smVar.smMsgTypeInProgress,
                                cmData->globalState,
                                "");
    }
}
#endif /* INSTALL_CM_SM_REPAIR */

void CsrBtCmSmUserConfirmationRequestResHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmUserConfirmationRequestRes *prim;
    TP_BD_ADDR_T tpAddr;

    prim = (CsrBtCmSmUserConfirmationRequestRes *) cmData->recvMsgP;
    tpAddr.addrt.addr = prim->deviceAddr;
    tpAddr.addrt.type = prim->addressType;
    tpAddr.tp_type = prim->transportType;

    dm_sm_user_confirmation_request_rsp(&tpAddr, NULL);
}

void CsrBtCmSmUserConfirmationRequestNegResHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmUserConfirmationRequestNegRes *prim;
    TP_BD_ADDR_T tpAddr;

    prim = (CsrBtCmSmUserConfirmationRequestNegRes *) cmData->recvMsgP;
    tpAddr.addrt.addr = prim->deviceAddr;
    tpAddr.addrt.type = prim->addressType;
    tpAddr.tp_type = prim->transportType;

    dm_sm_user_confirmation_request_neg_rsp(&tpAddr, NULL);
}

void CsrBtCmSmUserPasskeyRequestResHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmUserPasskeyRequestRes *prim;
    TP_BD_ADDR_T tpAddr;

    prim = (CsrBtCmSmUserPasskeyRequestRes *) cmData->recvMsgP;
    tpAddr.addrt.addr = prim->deviceAddr;
    tpAddr.addrt.type = prim->addressType;
    tpAddr.tp_type = prim->transportType;

    dm_sm_user_passkey_request_rsp(&tpAddr, prim->numericValue, NULL);
}

void CsrBtCmSmUserPasskeyRequestNegResHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmUserPasskeyRequestNegRes *prim;
    TP_BD_ADDR_T tpAddr;

    prim = (CsrBtCmSmUserPasskeyRequestNegRes *) cmData->recvMsgP;
    tpAddr.addrt.addr = prim->deviceAddr;
    tpAddr.addrt.type = prim->addressType;
    tpAddr.tp_type = prim->transportType;

    dm_sm_user_passkey_request_neg_rsp(&tpAddr, NULL);
}

#endif

