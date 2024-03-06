/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_util.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"

#ifdef CSR_BT_LE_ENABLE
#include "csr_bt_cm_le.h"
#endif

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_cm_bnep.h"
#endif

#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_cm_events_handler.h"
#include "hci_prim.h"
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
#include "csr_bt_cm_cme.h"
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
cmRfcConnElement * returnReserveScoIndexToThisAddress(cmInstanceData_t *cmData, CsrBtDeviceAddr deviceAddr)
{
    /* Try to find the index that match with the given device address,
     * and where the Sco index is reserved. If no match it return
     * ERROR */
    return(CsrBtCmRfcFindRfcConnElementFromDeviceAddrScoHandle(cmData, &deviceAddr, SCOBUSY_ACCEPT));
}

#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
#endif /* EXCLUDE_CSR_BT_SCO_MODULE */

#if !defined (CSR_TARGET_PRODUCT_VM) && !defined (CSR_TARGET_PRODUCT_WEARABLE)
static CsrBool cmDmIsConnectable(cmInstanceData_t *cmData)
{
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    CsrBtConnId     btRfcConnId = CSR_BT_CONN_ID_INVALID;
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    CsrBtConnId     btL2caConnId  = BTCONN_ID_RESERVED;
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
    CsrUintFast8 i;
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    {
        cmRfcConnElement * theElement  = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId, &(btRfcConnId));
        if (theElement)
        {
            if (theElement->cmRfcConnInst->state != CSR_BT_CM_RFC_STATE_CANCEL_CONNECTABLE)
            {/* Do not enable scan page if we are cancelling it... */
                return TRUE;
            }
        }
    }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    if(CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromBtConnId, &btL2caConnId))
    {
        return TRUE;
    }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
    for ( i = 0; i < CSR_BT_MAX_NUM_OF_SIMULTANEOUS_BNEP_CONNECTIONS; i++ )
    { /* Goes through the bnep connection table */
        if(cmData->bnepVar.connectVar[i].id == ID_RESERVED)
        {
            return TRUE;
        }
    }
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */

    return FALSE;
}
#endif /* !CSR_TARGET_PRODUCT_VM/CSR_TARGET_PRODUCT_WEARABLE */

CsrUint8 returnConnectAbleParameters(cmInstanceData_t *cmData)
{
    /* Find out which connectable mode the device must be place into,
     * e.g HCI_SCAN_ENABLE_INQ_AND_PAGE, HCI_SCAN_ENABLE_PAGE,
     * HCI_SCAN_ENABLE_OFF or HCI_SCAN_ENABLE_INQ */
    CsrUint8 mode = HCI_SCAN_ENABLE_OFF;

    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
    {
        return CSR_BT_CM_DEFAULT_STARTUP_CONNECTABLE_MODE;
    }

    if (!cmData->dmVar.disableInquiryScan)
    {
        mode = HCI_SCAN_ENABLE_INQ;
    }

    if (cmData->dmVar.disablePageScan)
    {
        return mode;
    }
#if defined (CSR_TARGET_PRODUCT_VM) || defined (CSR_TARGET_PRODUCT_WEARABLE)
    else
    {
        /* For some product lines like V&M and Wearables, 
         * incoming connections for non Synergy services/profiles
         * are accepted/rejected by application. */
        mode = (CsrUint8)(mode | HCI_SCAN_ENABLE_PAGE);
    }
#else
    /* For some product lines (other than V&M and Wearables), page scan 
     * can only be enabled if the device is connectable. */
    if (cmDmIsConnectable(cmData))
    {
        mode = (CsrUint8)(mode | HCI_SCAN_ENABLE_PAGE);
    }
#endif /* CSR_TARGET_PRODUCT_VM/CSR_TARGET_PRODUCT_WEARABLE */
    return mode;
}

#ifdef CSR_BT_INSTALL_CM_WRITE_VOICE_SETTINGS
static void csrBtCmWriteVoiceSettingsCfmMsgSend(CsrSchedQid appHandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtCmWriteVoiceSettingsCfm   *prim = (CsrBtCmWriteVoiceSettingsCfm *) CsrPmemZalloc(sizeof(*prim));

    prim->type           = CSR_BT_CM_WRITE_VOICE_SETTINGS_CFM;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;
    CsrBtCmPutMessage(appHandle, prim);
}

void CsrBtCmWriteVoiceSettingsReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmWriteVoiceSettingsReq    * prim;

    prim = (CsrBtCmWriteVoiceSettingsReq *) cmData->recvMsgP;

    cmData->dmVar.appHandle = prim->phandle;

    dm_hci_write_voice_setting(prim->voiceSettings, NULL);
}
#endif

void CsrBtCmDmHciWriteVoiceSettingCompleteHandler(cmInstanceData_t *cmData)
{ /* Write voice setting complete. If success initialise RFCOMM */
    DM_HCI_WRITE_VOICE_SETTING_CFM_T    *dmPrim;

    dmPrim = (DM_HCI_WRITE_VOICE_SETTING_CFM_T *)cmData->recvMsgP;
#ifdef CSR_BT_INSTALL_CM_WRITE_VOICE_SETTINGS
    if (cmData->globalState != CSR_BT_CM_STATE_NOT_READY)
    {
        if (dmPrim->status == HCI_SUCCESS)
        {
            csrBtCmWriteVoiceSettingsCfmMsgSend(cmData->dmVar.appHandle,
                        CSR_BT_RESULT_CODE_CM_SUCCESS, CSR_BT_SUPPLIER_CM);
        }
        else
        {
            csrBtCmWriteVoiceSettingsCfmMsgSend(cmData->dmVar.appHandle,
                        (CsrBtResultCode) dmPrim->status, CSR_BT_SUPPLIER_HCI);
        }
        CsrBtCmDmLocalQueueHandler();
    }
    else
#endif
    {
        /* We are currently in the CM initialization process, we must
         * continue setting up the chip */
        CmInitSequenceHandler(cmData,
                              CM_INIT_SEQ_WRITE_VOICE_SETTING_CFM,
                              dmPrim->status,
                              CSR_BT_SUPPLIER_HCI);
    }
}

static void CsrBtCmDmWriteAuthPayloadTimeoutCfmMsgSend(CsrSchedQid appHandle,
                                                       CsrBtTpdAddrT    tpAddrt,
                                                       CsrBtResultCode resultCode,
                                                       CsrBtSupplier resultSupplier)
{
    CsrBtCmWriteAuthPayloadTimeoutCfm   *prim = (CsrBtCmWriteAuthPayloadTimeoutCfm *) CsrPmemZalloc(sizeof(*prim));

    /* Unlock DM queue */
    CsrBtCmDmLocalQueueHandler();

    prim->type               = CSR_BT_CM_WRITE_AUTH_PAYLOAD_TIMEOUT_CFM;
    prim->tpAddrt            = tpAddrt;
    prim->resultCode         = resultCode;
    if(resultCode == HCI_SUCCESS)
    {
        resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    prim->resultSupplier = resultSupplier;

    CsrBtCmPutMessage(appHandle, prim);
}

void CsrBtCmDmWriteAuthPayloadTimeoutReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmWriteAuthPayloadTimeoutReq * prim;
    aclTable *aclConnectionElement;

    prim = (CsrBtCmWriteAuthPayloadTimeoutReq *) cmData->recvMsgP;
    returnAclConnectionElement(cmData, prim->tpAddrt.addrt.addr, &aclConnectionElement);

    if (aclConnectionElement)
    {
        if (((prim->tpAddrt.tp_type == CSR_BT_TRANSPORT_BREDR) &&
             (aclConnectionElement->encryptType == DM_SM_ENCR_ON_BREDR_AES_CCM)) ||
            ((prim->tpAddrt.tp_type == CSR_BT_TRANSPORT_LE) &&
             (aclConnectionElement->encryptType == DM_SM_ENCR_ON_LE_AES_CCM)))
        {
            cmData->dmVar.appHandle = prim->phandle;
            dm_sm_write_authenticated_payload_timeout(&prim->tpAddrt,
                                                      prim->authPayloadTimeout,
                                                      prim->aptRoute,
                                                      NULL);
            return;
        }
    }

    CsrBtCmDmWriteAuthPayloadTimeoutCfmMsgSend(prim->phandle,
                                               prim->tpAddrt,
                                               CSR_BT_RESULT_CODE_CM_COMMAND_DISALLOWED,
                                               CSR_BT_SUPPLIER_CM);
}


void CsrBtCmDmWriteAuthPayloadTimeoutCompleteHandler(cmInstanceData_t *cmData)
{
    DM_SM_WRITE_AUTHENTICATED_PAYLOAD_TIMEOUT_CFM_T    *dmPrim;
    dmPrim = (DM_SM_WRITE_AUTHENTICATED_PAYLOAD_TIMEOUT_CFM_T *)cmData->recvMsgP;

    CsrBtCmDmWriteAuthPayloadTimeoutCfmMsgSend(cmData->dmVar.appHandle,
                                               dmPrim->tp_addrt,
                                               dmPrim->status,
                                               CSR_BT_SUPPLIER_HCI);
}

#ifdef INSTALL_CM_READ_APT
void CmDmReadAuthPayloadTimeoutReqHandler(cmInstanceData_t *cmData)
{
    CmDmReadAuthPayloadTimeoutReq       *cmPrim = (CmDmReadAuthPayloadTimeoutReq *)cmData->recvMsgP;
    cmData->dmVar.appHandle = cmPrim->appHandle;
    dm_hci_read_authenticated_payload_timeout(&cmPrim->tpAddrt, NULL);
}

void CmDmReadAuthPayloadTimeoutCfmHandler(cmInstanceData_t *cmData)
{
    CmDmReadAuthPayloadTimeoutCfm                   *prim   = (CmDmReadAuthPayloadTimeoutCfm *)CsrPmemZalloc(sizeof(*prim));
    DM_HCI_READ_AUTHENTICATED_PAYLOAD_TIMEOUT_CFM_T *dmPrim = (DM_HCI_READ_AUTHENTICATED_PAYLOAD_TIMEOUT_CFM_T *)cmData->recvMsgP;

    prim->type = CM_DM_READ_AUTH_PAYLOAD_TIMEOUT_CFM;
    prim->tpAddrt            = dmPrim->tp_addrt;
    if(dmPrim->status == HCI_SUCCESS)
    {
        prim->authPayloadTimeout = dmPrim->authenticated_payload_timeout;
        prim->resultCode         = CSR_BT_RESULT_CODE_CM_SUCCESS;
        prim->resultSupplier     = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        prim->resultCode     = dmPrim->status;
        prim->resultSupplier = CSR_BT_SUPPLIER_HCI;
    }
    CsrBtCmPutMessage(cmData->dmVar.appHandle, prim);
    CsrBtCmDmLocalQueueHandler();
}
#endif /* INSTALL_CM_READ_APT */

void CmDmHciAuthPayloadTimeoutExpiredIndHandler(cmInstanceData_t *cmData)
{
    DM_HCI_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED_IND_T *dmPrim;
    CmDmHciAuthPayloadTimeoutExpiredInd *ind = (CmDmHciAuthPayloadTimeoutExpiredInd *)CsrPmemZalloc(sizeof(*ind));

    dmPrim = (DM_HCI_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED_IND_T *)cmData->recvMsgP;

    ind->type = CM_DM_HCI_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED_IND;
    ind->deviceAddr = dmPrim->tp_addrt.addrt.addr;

    CsrBtCmPutMessage(CSR_BT_CM_SC_HANDLE(cmData), ind);
}


#ifndef CSR_BT_EXCLUDE_HCI_READ_CONN_ACCEPT_TIMEOUT
void CmDmReadConnAcceptTimeoutReqHandler(cmInstanceData_t * cmData)
{
    CmDmReadConnAcceptTimeoutReq       *cmPrim = (CmDmReadConnAcceptTimeoutReq *)cmData->recvMsgP;
    cmData->dmVar.appHandle = cmPrim->appHandle;

    CSR_LOG_TEXT_INFO((CsrBtCmLto, CSR_BT_CM_LTSO_DM_QUEUE, "CmDmReadConnAcceptTimeoutReqHandler"));

    dm_hci_read_conn_accept_to(NULL);
}

void CmDmReadConnAcceptTimeoutCfmHandler(cmInstanceData_t * cmData)
{
    DM_HCI_READ_CONN_ACCEPT_TIMEOUT_CFM_T *dmPrim;
    CmDmReadConnAcceptTimeoutCfm *cfm = (CmDmReadConnAcceptTimeoutCfm*)CsrPmemZalloc(sizeof(*cfm));

    CSR_LOG_TEXT_INFO((CsrBtCmLto, CSR_BT_CM_LTSO_DM_QUEUE, "CmDmReadConnAcceptTimeoutCfmHandler"));

    dmPrim = (DM_HCI_READ_CONN_ACCEPT_TIMEOUT_CFM_T *)cmData->recvMsgP;

    cfm->type = CM_DM_READ_CONN_ACCEPT_TIMEOUT_CFM;
    cfm->status = dmPrim->status;
    cfm->connAcceptTimeout = dmPrim->conn_accept_timeout;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}
#endif /* !CSR_BT_EXCLUDE_HCI_READ_CONN_ACCEPT_TIMEOUT */


#ifndef CSR_BT_EXCLUDE_HCI_WRITE_CONN_ACCEPT_TIMEOUT
void CmDmWriteConnAcceptTimeoutReqHandler(cmInstanceData_t * cmData)
{
    CmDmWriteConnAcceptTimeoutReq       *cmPrim = (CmDmWriteConnAcceptTimeoutReq *)cmData->recvMsgP;
    cmData->dmVar.appHandle = cmPrim->appHandle;
    CSR_LOG_TEXT_INFO((CsrBtCmLto, CSR_BT_CM_LTSO_DM_QUEUE, "CmDmWriteConnAcceptTimeoutReqHandler"));

    dm_hci_write_conn_accept_to(cmPrim->connAcceptTimeout, NULL);
}

void CmDmWriteConnAcceptTimeoutCfmHandler(cmInstanceData_t * cmData)
{
    DM_HCI_WRITE_CONN_ACCEPT_TIMEOUT_CFM_T *dmPrim;
    CmDmWriteConnAcceptTimeoutCfm *cfm = (CmDmWriteConnAcceptTimeoutCfm *)CsrPmemZalloc(sizeof(*cfm));

    CSR_LOG_TEXT_INFO((CsrBtCmLto, CSR_BT_CM_LTSO_DM_QUEUE, "CmDmWriteConnAcceptTimeoutCfmHandler"));

    dmPrim = (DM_HCI_WRITE_CONN_ACCEPT_TIMEOUT_CFM_T *)cmData->recvMsgP;

    cfm->type = CM_DM_WRITE_CONN_ACCEPT_TIMEOUT_CFM;
    cfm->status= dmPrim->status;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}
#endif /* !CSR_BT_EXCLUDE_HCI_WRITE_CONN_ACCEPT_TIMEOUT */



void CsrBtCmDmSmEncryptionChangeHandler(cmInstanceData_t *cmData)
{
    aclTable *aclConnectionElement;
    DM_SM_ENCRYPTION_CHANGE_IND_T *prim = (DM_SM_ENCRYPTION_CHANGE_IND_T*) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE
    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropagateEncryptIndStatusEvents,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE,
                         HCI_SUCCESS,
                         prim,
                         NULL);
#endif

    returnAclConnectionElement(cmData,
                               prim->tp_addrt.addrt.addr,
                               &aclConnectionElement);

    if (aclConnectionElement)
    {
        aclConnectionElement->encryptType = prim->encrypt_type;
#if (CSR_BT_DEFAULT_LINK_SUPERVISION_TIMEOUT != CSR_BT_HCI_DEFAULT_LSTO)
        if (aclConnectionElement->role == CSR_BT_MASTER_ROLE &&
            aclConnectionElement->lsto != CSR_BT_DEFAULT_LINK_SUPERVISION_TIMEOUT)
        {
            CsrBtCmWriteDmLinkSuperVisionTimeoutHandler(cmData,
                                                        CSR_BT_CM_IFACEQUEUE,
                                                        CSR_BT_DEFAULT_LINK_SUPERVISION_TIMEOUT,
                                                        prim->addrt.addr);
        }
#endif
    }

#ifndef EXCLUDE_CSR_BT_SC_MODULE
    CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_ENCRYPTION_CHANGE_IND);
#endif /* !EXCLUDE_CSR_BT_SC_MODULE */
}

#ifdef CSR_BT_INSTALL_SC_ENCRYPTION
void CsrBtCmDmSmEncryptCfmHandler(cmInstanceData_t *cmData)
{
    CsrUint8 status = HCI_SUCCESS;
    DM_SM_ENCRYPT_CFM_T *prim = (DM_SM_ENCRYPT_CFM_T *) cmData->recvMsgP;

    if (prim->success)
    {
        aclTable *aclConnectionElement;

        returnAclConnectionElement(cmData,
                                   prim->bd_addr,
                                   &aclConnectionElement);

        if (aclConnectionElement)
        {
            aclConnectionElement->encryptType = prim->encrypt_type;
        }
    }
    else
    {
        status = HCI_ERROR_UNSPECIFIED;
    }

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE
    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropagateEncryptCfmStatusEvents,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE,
                         status,
                         prim,
                         NULL);
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE */

#ifndef EXCLUDE_CSR_BT_SC_MODULE
    prim->phandle = cmData->dmVar.appHandle;
    CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_ENCRYPT_CFM);
#endif /* !EXCLUDE_CSR_BT_SC_MODULE */

    CsrBtCmDmLocalQueueHandler();
}
#endif /* CSR_BT_INSTALL_SC_ENCRYPTION */

static void csrBtCmSmSppRepairIndDelaySend(CsrUint8 dummy, cmInstanceData_t *cmData)
{
    if (cmData->dmVar.rebond.keyMissingTimerId != 0)
    {
        CsrBtCmSmRepairInd * prim;

        prim = (CsrBtCmSmRepairInd *) CsrPmemAlloc(sizeof(CsrBtCmSmRepairInd));

        cmData->dmVar.rebond.keyMissingTimerId = 0;
        cmData->dmVar.rebond.keyMissingId++;

        prim->type          = CSR_BT_CM_SM_REPAIR_IND;
        prim->deviceAddr    = cmData->dmVar.rebond.keyMissingDeviceAddr;
        prim->repairId      = cmData->dmVar.rebond.keyMissingId;
        prim->addressType   = CSR_BT_ADDR_PUBLIC;
        CsrBtCmPutMessage(CSR_BT_CM_SC_HANDLE(cmData), prim);
    }
    else
    {
        ;
    }
    CSR_UNUSED(dummy);
}

void CsrBtCmSmSppRepairIndSend(cmInstanceData_t *cmData, CsrBtDeviceAddr deviceAddr)
{
    cmData->dmVar.rebond.keyMissingDeviceAddr  = deviceAddr;
    cmData->dmVar.rebond.keyMissingTimerId     = CsrSchedTimerSet(SSP_REPAIR_DELAY,
                    (void (*) (CsrUint16, void *)) csrBtCmSmSppRepairIndDelaySend, 0, (void *) cmData);
}

void CsrBtCmSmCancelSppRepairInd(cmInstanceData_t *cmData)
{
    if (cmData->dmVar.rebond.keyMissingTimerId != 0)
    {
        CsrSchedTimerCancel(cmData->dmVar.rebond.keyMissingTimerId, NULL, NULL);
        cmData->dmVar.rebond.keyMissingTimerId = 0;
    }
    else
    {
        ;
    }
    cmData->dmVar.rebond.keyMissingId++;
}

void CsrBtCmLogicalChannelTypeHandler(cmInstanceData_t *cmData)
{/* Update the RFC or L2CAP table and, if needed, the ACL table as well.
    Issue event if needed. */
    CsrBtCmLogicalChannelTypeReq * prim = (CsrBtCmLogicalChannelTypeReq *)cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
    if (CSR_BT_CONN_ID_IS_RFC(prim->btConnId))
    {/* RFC connection */
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
        cmRfcConnElement * theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId, &(prim->btConnId));
        if (theElement)
        { /* Update RFC table */
            theElement->cmRfcConnInst->logicalChannelData = (prim->logicalChannelTypeMask & CSR_BT_ACTIVE_DATA_CHANNEL) ?
                            TRUE : FALSE;
            theElement->cmRfcConnInst->logicalChannelControl = (prim->logicalChannelTypeMask & CSR_BT_ACTIVE_CONTROL_CHANNEL) ?
                            TRUE : FALSE;
        }
#endif
    }
    else
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE */
    {/* L2CAP */
#ifndef  EXCLUDE_CSR_BT_L2CA_MODULE
        if (CSR_BT_CONN_ID_IS_L2CA(prim->btConnId))
        {
            cmL2caConnElement *theElement = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromBtConnId,
                                                                 &(prim->btConnId));
            if (theElement)
            { /* Update L2CAP table */
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
                theElement->cmL2caConnInst->logicalChannelData = (prim->logicalChannelTypeMask & CSR_BT_ACTIVE_DATA_CHANNEL) ?
                                TRUE : FALSE;
                theElement->cmL2caConnInst->logicalChannelControl = (prim->logicalChannelTypeMask & CSR_BT_ACTIVE_CONTROL_CHANNEL) ?
                                TRUE : FALSE;
#endif

#if defined(CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE) || defined(CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_A2DP_BIT_RATE)
                theElement->cmL2caConnInst->logicalChannelStream = (prim->logicalChannelTypeMask & CSR_BT_ACTIVE_STREAM_CHANNEL) ?
                                TRUE : FALSE;
#else
                CSR_UNUSED(theElement);
#endif
            }
        }
#endif
    }
    
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
    {
        aclTable *aclConnectionElement = NULL;

        /* find ACL link */
        returnAclConnectionElement(cmData,
                                   prim->deviceAddr,
                                   &aclConnectionElement);

        if (aclConnectionElement)
        {
            if (updateLogicalChannelTypeMaskAndNumber(cmData,
                                                      &aclConnectionElement->deviceAddr))
            {/* Changed: issue event */
                CsrBtCmPropgateEvent(cmData,
                                     CsrBtCmPropgateLogicalChannelTypeEvents,
                                     CSR_BT_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE,
                                     HCI_SUCCESS,
                                     (void *) aclConnectionElement,
                                     NULL);
            }
        }
    }
#endif
}

void CsrBtCmA2DPBitrateHandler(cmInstanceData_t *cmData)
{
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_A2DP_BIT_RATE
    CsrBtCmPropgateEvent(cmData,
                        CsrBtCmPropagateA2DPBitRateEvents,
                        CSR_BT_CM_EVENT_MASK_SUBSCRIBE_A2DP_BIT_RATE,
                        HCI_SUCCESS,
                        (void *)cmData->recvMsgP,
                        NULL);
#else
    CSR_UNUSED(cmData);

#endif
}

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
void CsrBtCmSetAvStreamInfoReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSetAvStreamInfoReq *prim = (CsrBtCmSetAvStreamInfoReq*)cmData->recvMsgP;

    CsrBool start = prim->start;
    CsrUint8 streamIdx = prim->streamIndex;
    if (streamIdx < CSR_BT_AV_MAX_NUM_STREAMS)
    {/* update av stream information */
        cmData->avStreamVar[streamIdx].start                = start;
        cmData->avStreamVar[streamIdx].aclHandle            = prim->aclHandle;
        cmData->avStreamVar[streamIdx].l2capConnectionId    = prim->l2capConnectionId;
        cmData->avStreamVar[streamIdx].bitRate              = prim->bitRate;
        cmData->avStreamVar[streamIdx].codecLocation        = prim->codecLocation;
        cmData->avStreamVar[streamIdx].codecType            = prim->codecType;
        cmData->avStreamVar[streamIdx].period               = prim->period;
        cmData->avStreamVar[streamIdx].role                 = prim->role;
        cmData->avStreamVar[streamIdx].samplingFreq         = prim->samplingFreq;
        cmData->avStreamVar[streamIdx].sduSize              = prim->sduSize;

        if (cmData->cmeServiceRunning)
        {/* CME service is running on-chip */
            if (start)
            {/* send cme a2dp start stream indication to chip */
                CsrBtCmCmeProfileA2dpStartIndSend(cmData, streamIdx);
            }
            else
            {/* send cme a2dp stop stream indication to chip */
                CsrBtCmCmeProfileA2dpStopIndSend(cmData, streamIdx);
            }
        }
    }
    else
    {
        /* wrong stream id */
    }
}
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

#ifdef CSR_TARGET_PRODUCT_VM
void CsrBtCmDmBadMessageIndHandler(cmInstanceData_t *cmData)
{
    DM_BAD_MESSAGE_IND_T *dmPrim = (DM_BAD_MESSAGE_IND_T*) cmData->recvMsgP;

    if ((dmPrim->request_type == L2CA_DISCONNECT_REQ) &&
        (dmPrim->reason == DM_BAD_MESSAGE_NO_PHANDLE))
    {
        /* ULCONV_TODO: Bluestack, in certain circumstances, sends DM_BAD_MESSAGE_IND
         * in response to L2CA_DISCONNECT_REQ instead of sending a L2CA_DISCONNECT_CFM.
         * So a dummy L2CA_DISCONNECT_CFM with a specific reason code is posted to CM. */
        L2CA_DISCONNECT_REQ_T *reqPrim = (L2CA_DISCONNECT_REQ_T*)dmPrim->message;
        L2CA_DISCONNECT_CFM_T *cfmPrim = CsrPmemZalloc(sizeof(L2CA_DISCONNECT_CFM_T));

        cfmPrim->type = L2CA_DISCONNECT_CFM;
        cfmPrim->phandle = dmPrim->phandle;
        cfmPrim->cid = reqPrim->cid;
        cfmPrim->result = L2CA_DISCONNECT_LINK_TRANSFERRED;

        CsrSchedMessagePut(CSR_BT_CM_IFACEQUEUE, L2CAP_PRIM, cfmPrim);
    }
}
#endif

void CsrBtCmDmSmSimplePairingCompleteHandler(cmInstanceData_t *cmData)
{
    DM_SM_SIMPLE_PAIRING_COMPLETE_IND_T *prim = (DM_SM_SIMPLE_PAIRING_COMPLETE_IND_T*) cmData->recvMsgP;

    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropagateSimplePairingIndStatusEvents,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SIMPLE_PAIRING_COMPLETE,
                         prim->status,
                         prim,
                         NULL);
}

#ifdef INSTALL_CM_DM_CONFIGURE_DATA_PATH
void CmDmConfigureDataPathReqHandler(cmInstanceData_t *cmData)
{
    CmDmConfigureDataPathReq *cmPrim = (CmDmConfigureDataPathReq *)cmData->recvMsgP;

    cmData->dmVar.appHandle = cmPrim->appHandle;
    dm_hci_configure_data_path_req(cmPrim->dataPathDirection,
                                   cmPrim->dataPathId,
                                   cmPrim->vendorSpecificConfigLen,
                                   cmPrim->vendorSpecificConfig,
                                   NULL);
}

void CmDmConfigureDataPathCfmHandler(cmInstanceData_t *cmData)
{
    CmDmConfigureDataPathCfm *prim   = (CmDmConfigureDataPathCfm *)CsrPmemZalloc(sizeof(*prim));
    DM_HCI_CONFIGURE_DATA_PATH_CFM_T *dmPrim = (DM_HCI_CONFIGURE_DATA_PATH_CFM_T *)cmData->recvMsgP;

    prim->type = CM_DM_HCI_CONFIGURE_DATA_PATH_CFM;
    prim->resultCode = dmPrim->status;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, prim);
    CsrBtCmDmLocalQueueHandler();
}
#endif /* INSTALL_CM_DM_CONFIGURE_DATA_PATH */

#ifdef INSTALL_CM_DM_LE_READ_CHANNEL_MAP
void CmDmLeReadChannelMapReqHandler(cmInstanceData_t *cmData)
{
    CmDmLeReadChannelMapReq *cmPrim = (CmDmLeReadChannelMapReq *)cmData->recvMsgP;

    cmData->dmVar.appHandle = cmPrim->appHandle;
    dm_hci_ulp_read_channel_map_req(&(cmPrim->peerAddr), NULL);
}

void CmDmLeReadChannelMapCfmHandler(cmInstanceData_t *cmData)
{
    CmDmLeReadChannelMapCfm *prim   = (CmDmLeReadChannelMapCfm *)CsrPmemZalloc(sizeof(*prim));
    DM_HCI_ULP_READ_CHANNEL_MAP_CFM_T *dmPrim = (DM_HCI_ULP_READ_CHANNEL_MAP_CFM_T *)cmData->recvMsgP;

    prim->type = CM_DM_LE_READ_CHANNEL_MAP_CFM;

    if(dmPrim->status == HCI_SUCCESS)
    {
        prim->resultCode     = CSR_BT_RESULT_CODE_SUCCESS;
        prim->resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        prim->resultCode     = dmPrim->status;
        prim->resultSupplier = CSR_BT_SUPPLIER_HCI;
    }

    CsrBtAddrCopy(&(prim->peerAddr), &(dmPrim->addrt));
    CsrMemCpy(prim->channelMap, dmPrim->ulp_channel_map, CM_DM_LE_CHANNEL_MAP_LEN);

    CsrBtCmPutMessage(cmData->dmVar.appHandle, prim);
    CsrBtCmDmLocalQueueHandler();
}
#endif /* INSTALL_CM_DM_LE_READ_CHANNEL_MAP */
