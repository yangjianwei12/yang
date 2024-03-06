/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_rfc.h"

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_cm_bnep.h"
#endif

#include "csr_bt_cm_l2cap.h"
#include "dm_prim.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_cm_dm_sc_lib.h"
#include "csr_bt_cm_dm_sc_ssp_handler.h"
#include "csr_bt_cm_le.h"

#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_prim.h"
#else
#include "csr_bt_gatt_private_lib.h"
#endif

void CsrBtCmGetSecurityConfIndSend(cmInstanceData_t *cmData,
                                   CsrUint8 lmpVersion)
{
#ifdef EXCLUDE_CSR_BT_SC_MODULE
    CsrBtCmDmSmInit(cmData);
    CSR_UNUSED(lmpVersion);
#else /* EXCLUDE_CSR_BT_SC_MODULE */
    CsrBtCmGetSecurityConfInd * prim;

    prim = (CsrBtCmGetSecurityConfInd*)CsrPmemAlloc(sizeof(CsrBtCmGetSecurityConfInd));
    prim->type = CSR_BT_CM_GET_SECURITY_CONF_IND;
    prim->lmpVersion = lmpVersion;
    CsrBtCmPutMessage(CSR_BT_CM_SC_HANDLE(cmData), prim);
#endif /* !EXCLUDE_CSR_BT_SC_MODULE */
}

void CsrBtCmDmSmAccessIndHandler(cmInstanceData_t * cmData)
{
    DM_SM_ACCESS_IND_T *prim;

    prim = (DM_SM_ACCESS_IND_T *) cmData->recvMsgP;

    if (!prim->conn_setup.incoming && prim->status != HCI_SUCCESS)
    {
        if (cmData->smVar.smInProgress)
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

                        if (CsrBtBdAddrEq(&prim->conn_setup.connection.addrt.addr,
                                          &(theLogicalLink->deviceAddr)))
                        {
                            cmData->dmVar.rebond.dmSmAccessIndStatus = prim->status;
                        }
                        else
                        {
                            /* Just ignore */
                            ;
                        }
                    }
                    else
                    {
                        /* just igore */
                        ;
                    }
                    break;
                }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
                case CSR_BT_CM_L2CA_CONNECT_REQ:
                {
                    cmL2caConnElement *connElement = CM_L2CA_ELEMENT_ACTIVE(cmData);

                    if (connElement && connElement->cmL2caConnInst)
                    {
                        cmL2caConnInstType *l2CaConnection = connElement->cmL2caConnInst;

                        if (CsrBtBdAddrEq(&prim->conn_setup.connection.addrt.addr,
                                          &(l2CaConnection->deviceAddr)))
                        {
                            cmData->dmVar.rebond.dmSmAccessIndStatus = prim->status;
                        }
                        else
                        { /* Just ignore */
                            ;
                        }
                    }
                    else
                    { /* just igore */
                        ;
                    }
                    break;
                }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */
#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
                case CSR_BT_CM_BNEP_CONNECT_REQ:
                {
                    bnepTable *bnepConnection;

                    bnepConnection = cmData->bnepVar.indexPtr;

                    if( CsrBtBdAddrEq(&prim->conn_setup.connection.addrt.addr,
                                   &(bnepConnection->deviceAddr)))
                    {
                        cmData->dmVar.rebond.dmSmAccessIndStatus = prim->status;
                    }
                    else
                    { /* Just ignore */
                        ;
                    }
                    break;
                }
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */
                default:
                { /* Just ignore */
                    break;
                }
            }
        }
        else
        { /* Just ignore */
            ;
        }
    }
    else
    { /* It is an incoming connect, just ignore */
        ;
    }

    CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_ACCESS_IND);
}

void CsrBtCmDmSmClearRebondData(cmInstanceData_t *cmData)
{
    cmData->dmVar.rebond.dmSmAccessIndStatus = HCI_SUCCESS;
    CsrBtBdAddrZero(&cmData->dmVar.rebond.keyMissingDeviceAddr);
}

CsrBool CsrBtCmDmSmRebondNeeded(cmInstanceData_t *cmData)
{
    hci_return_t status = cmData->dmVar.rebond.dmSmAccessIndStatus;
    CsrBool needRebond = FALSE;

    if (CsrBtBdAddrEqZero(&cmData->dmVar.rebond.keyMissingDeviceAddr))
    {
        switch (status)
        {
            case HCI_SUCCESS:/* Fall through */
            case HCI_ERROR_UNSUPP_LMP_PARAM:/* Fall through*/
            case HCI_ERROR_INSUFFICIENT_SECURITY:
                /* Insufficient Encryption key size */
                /* Do not initiate rebond */
                break;
            default:
                needRebond = TRUE;
                break;
        }
    }

    return needRebond;
}

#ifndef EXCLUDE_CSR_BT_SC_MODULE
void CsrBtCmSmDeleteStoreLinkKeyReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmDeleteStoreLinkKeyReq    *cmPrim;
    cmPrim = (CsrBtCmSmDeleteStoreLinkKeyReq *) cmData->recvMsgP;
    dm_hci_delete_stored_link_key(&(cmPrim->deviceAddr), cmPrim->flag, NULL);
}

void CsrBtCmSmAddDeviceReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmAddDeviceReq *cmPrim = (CsrBtCmSmAddDeviceReq *) cmData->recvMsgP;

    /* Suppressing false positive lint warning (437) for passing struct
     * to ellipsis, here union is being passed which is intended and determined 
     * by cmPrim->keys->present */

    /*lint -e(437) */
    dm_sm_add_device_req(NULL,
                         CSR_BT_CM_IFACEQUEUE,
                         &cmPrim->typedAddr,
                         cmPrim->trust,
                         cmPrim->keys->security_requirements,
                         cmPrim->keys->encryption_key_size,
                         cmPrim->keys->present,
                         cmPrim->keys->u[0],
                         cmPrim->keys->u[1],
                         cmPrim->keys->u[2],
                         cmPrim->keys->u[3],
                         cmPrim->keys->u[4]);
}

void CsrBtCmSmRemoveDeviceReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmRemoveDeviceReq *cmPrim;
    TYPED_BD_ADDR_T ad;

    cmPrim = (CsrBtCmSmRemoveDeviceReq *) cmData->recvMsgP;

    if (CsrBtBdAddrEqZero(&cmPrim->deviceAddr))
    { /* Remove all devices */
        ad.type = TBDADDR_INVALID;
        CsrMemSet(&ad.addr, 0xFF, sizeof(ad.addr));
    }
    else
    {
        ad.addr = cmPrim->deviceAddr;
        ad.type = cmPrim->addressType;
    }
    dm_sm_remove_device_req(CSR_BT_CM_IFACEQUEUE, &ad, NULL);
}
#endif /* EXCLUDE_CSR_BT_SC_MODULE */

void CsrBtCmSmCancelConnectReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmCancelConnectReq *cmPrim;
    aclTable * aclConnectionElement;

    cmPrim   = (CsrBtCmSmCancelConnectReq *) cmData->recvMsgP;

    returnAclConnectionElement(cmData, cmPrim->deviceAddr, &aclConnectionElement);

    if (aclConnectionElement == NULL &&
        cmData->dmVar.state != CSR_BT_CM_DM_STATE_NULL &&
        CsrBtBdAddrEq(&cmData->dmVar.cacheTargetDev, &cmPrim->deviceAddr))
    {
        if (cmData->dmVar.state == CSR_BT_CM_DM_STATE_CONNECT)
        {
            dm_hci_create_connection_cancel(&(cmPrim->deviceAddr), NULL);
        }
        else
        {
            cmData->dmVar.cancel = TRUE;
        }
    }
    else
    {
        /* ACL already up or unknown */
        ;
    }
}

#ifdef CSR_BT_INSTALL_SC_AUTHENTICATE
void CsrBtCmSmAuthenticateReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmAuthenticateReq    *cmPrim;

    cmPrim                      = (CsrBtCmSmAuthenticateReq *) cmData->recvMsgP;
    dm_sm_authenticate_req(&cmPrim->deviceAddr, NULL);
    /* ToDo: Should DM queue be unlocked here? */
    CsrBtCmDmLocalQueueHandler();
}
#endif
#ifdef CSR_BT_INSTALL_SC_ENCRYPTION
void CsrBtCmSmEncryptionReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmEncryptionReq    *cmPrim;
    cmPrim = (CsrBtCmSmEncryptionReq *) cmData->recvMsgP;

    cmData->dmVar.appHandle = cmPrim->appHandle;
    dm_sm_encrypt_req( &cmPrim->deviceAddr, cmPrim->encryptionMode, NULL);
}
#endif

#ifndef EXCLUDE_CSR_BT_SC_MODULE
#ifdef CSR_BT_INSTALL_SC_SECURITY_MODE
void CsrBtCmSmSetSecModeReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmSetSecModeReq    *cmPrim;

    /* Usually invoked by the SC after the application has set the
     * security level */
    cmPrim = (CsrBtCmSmSetSecModeReq *) cmData->recvMsgP;
    dm_sm_init_req(DM_SM_INIT_SECURITY_MODE | DM_SM_INIT_MODE3_ENC, /* options */
                   cmPrim->mode,
                   0, /* security_level_default (not specified) */
                   0, /* config (not specified) */
                   0, /* write_auth_enable (not specified) */
                   cmPrim->mode3Enc,
                   NULL, /* sm_key_state */
                   0, /* sm_div_state*/
                   NULL);
}
#endif

void CsrBtCmSmSetDefaultSecLevelReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmSetDefaultSecLevelReq    *cmPrim;

    cmPrim = (CsrBtCmSmSetDefaultSecLevelReq *) cmData->recvMsgP;

    dm_sm_init_req(DM_SM_INIT_SECURITY_LEVEL_DEFAULT, /* options */
                   0, /* mode  (not specified) */
                   cmPrim->seclDefault,
                   0, /* config (not specified) */
                   0, /* write_auth_enable (not specified) */
                   0, /* mode3_enc */
                   NULL, /* sm_key_state */
                   0, /* sm_div_state*/
                   NULL);

    CsrBtCmDmLocalQueueHandler();
}
#endif

void CsrBtCmSmUnRegisterReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmUnregisterReq    *cmPrim;

    cmPrim                      = (CsrBtCmSmUnregisterReq *) cmData->recvMsgP;
    dm_sm_unregister_req(CSR_BT_CM_IFACEQUEUE,
                         0, /* context */
                         cmPrim->protocolId, cmPrim->channel, NULL);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmSmPinRequestResHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmPinRequestRes    *cmPrim;
    cmPrim = (CsrBtCmSmPinRequestRes *) cmData->recvMsgP;
    dm_sm_pin_request_rsp(&cmPrim->deviceAddr, cmPrim->pinLength, cmPrim->pin, NULL);
}

void CsrBtCmSmRegisterReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmRegisterReq    *cmPrim;

    cmPrim                      = (CsrBtCmSmRegisterReq *) cmData->recvMsgP;
    dm_sm_service_register_req(CSR_BT_CM_IFACEQUEUE,
                               0, /* context */
                               cmPrim->protocolId,
                               cmPrim->channel,
                               cmPrim->outgoingOk,
                               cmPrim->securityLevel,
                               cmPrim->minEncKeySize,
                               NULL);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmScRejectedForSecurityReasonMsgSend(cmInstanceData_t *cmData,
                                               CsrBtDeviceAddr theAddr,
                                               CsrBool cancelInitiated)
{
    CsrBtCmConnectionRejSecurityInd * prim;
    prim = (CsrBtCmConnectionRejSecurityInd *) CsrPmemAlloc(sizeof(CsrBtCmConnectionRejSecurityInd));

    prim->type          = CSR_BT_CM_CONNECTION_REJ_SECURITY_IND;
    prim->cancelInitiated   = cancelInitiated;
    prim->deviceAddr        = theAddr;
    CsrBtCmPutMessage(CSR_BT_CM_SC_HANDLE(cmData), prim);
}

#ifdef CSR_BT_INSTALL_SC_ENCRYPTION
static void sendCmReadEncryptionStatusCfm(CsrSchedQid     appHandle,
                                          CsrUint16       encrypted,
                                          CsrBtResultCode resultCode,
                                          CsrBtSupplier   resultSupplier)
{
    CsrBtCmReadEncryptionStatusCfm *cmPrim;

    cmPrim                  = (CsrBtCmReadEncryptionStatusCfm *)CsrPmemAlloc(sizeof(CsrBtCmReadEncryptionStatusCfm));
    cmPrim->type            = CSR_BT_CM_READ_ENCRYPTION_STATUS_CFM;
    cmPrim->encrypted       = encrypted;
    cmPrim->resultCode      = resultCode;
    cmPrim->resultSupplier  = resultSupplier;
    CsrBtCmPutMessage(appHandle, cmPrim);
}

void CsrBtCmReadEncryptionStatusReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmReadEncryptionStatusReq *cmPrim;
    aclTable                       * aclConnectionElement;

    cmPrim = (CsrBtCmReadEncryptionStatusReq *) cmData->recvMsgP;

    returnAclConnectionElement(cmData, cmPrim->deviceAddr, &aclConnectionElement);

    if (aclConnectionElement)
    {
        sendCmReadEncryptionStatusCfm(cmPrim->appHandle, aclConnectionElement->encryptType,
                                        CSR_BT_RESULT_CODE_CM_SUCCESS, CSR_BT_SUPPLIER_CM);
    }
    else
    {
        sendCmReadEncryptionStatusCfm(cmPrim->appHandle, FALSE,
                                CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER, CSR_BT_SUPPLIER_CM);
    }
    CsrBtCmDmLocalQueueHandler();
}
#endif

/*************************************************************************************
 CsrBtCmAutoAcceptForThisPsmAllowed:
************************************************************************************/
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
static void cmScMarkPeerAsAuthorised(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr, CsrUint32 psm)
{
    cmData->scVar.deviceAddr = *deviceAddr;
    cmData->scVar.psm = psm;
}

void CsrBtCmScCleanupVar(cmInstanceData_t *cmData)
{
    CsrBtBdAddrZero(&cmData->scVar.deviceAddr);
    cmData->scVar.psm = L2CA_PSM_INVALID;
}

static CsrBool CsrBtCmAutoAcceptForThisPsmAllowed(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr, CsrUint32 psm)
{
    cmL2caConnElement *currentElem;
    CsrBool allow = FALSE;

    for ( currentElem = CM_L2CA_GET_FIRST(cmData->l2caVar.connList);
         currentElem;
         currentElem = currentElem->next )
    {
        cmL2caConnInstType *cmL2caConn = currentElem->cmL2caConnInst;
        if (cmL2caConn != NULL)
         {
             if((cmL2caConn->btConnId != BTCONN_ID_EMPTY) &&
                (cmL2caConn->psm == psm) &&
                CsrBtBdAddrEq(&cmL2caConn->deviceAddr, deviceAddr))
             {
                if(cmL2caConn->authorised)
                {
                    allow = TRUE;
                    break;
                }
            }
        }
    }

    return allow;
}

/*************************************************************************************
 CsrBtCmAllowAutoAcceptForThisPsm:
************************************************************************************/
static void CsrBtCmAllowAutoAcceptForThisPsm(cmInstanceData_t *cmData, CsrBtDeviceAddr *deviceAddr, CsrUint32 psm)
{
    cmL2caConnElement *currentElem;

    for (currentElem = CM_L2CA_GET_FIRST(cmData->l2caVar.connList);
         currentElem;
         currentElem = currentElem->next)
     {
         cmL2caConnInstType *cmL2caConn = currentElem->cmL2caConnInst;
         if (cmL2caConn != NULL)
         {
             if((cmL2caConn->btConnId != BTCONN_ID_EMPTY) &&
                (cmL2caConn->psm == psm) &&
                CsrBtBdAddrEq(&cmL2caConn->deviceAddr, deviceAddr))
             {
                if(!cmL2caConn->authorised)
                {
                    cmL2caConn->authorised = TRUE;
                    break;
                }
             }
         }
     }
}
#endif

void CsrBtCmSmAuthoriseResHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmAuthoriseRes *cmPrim;
    CsrBtTypedAddr typedAddress;

    cmPrim = (CsrBtCmSmAuthoriseRes *) cmData->recvMsgP;
    typedAddress.addr = cmPrim->deviceAddr;
    typedAddress.type = cmPrim->addressType;

#ifdef EXCLUDE_CSR_BT_SC_MODULE
    if (cmPrim->authorisation == CSR_BT_CM_AUTHORISE_FOREVER)
    {
        CsrBtTdDbBredrKey bredrKey;

        if (CsrBtTdDbGetBredrKey(cmPrim->addressType,
                                 &cmPrim->deviceAddr,
                                 &bredrKey) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
        {
            bredrKey.authorised = TRUE;
            CsrBtTdDbSetBredrKey(cmPrim->addressType,
                                 &cmPrim->deviceAddr,
                                 &bredrKey);
        }
    }
#endif /* EXCLUDE_CSR_BT_SC_MODULE */

    dm_sm_authorise_rsp(&typedAddress,
                        cmPrim->protocolId,
                        cmPrim->channel,
                        cmPrim->incoming,
                        cmPrim->authorisation,
                        NULL);

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    if (((cmPrim->authorisation == DM_SM_AUTHORISE_ACCEPT_ONCE) ||
         (cmPrim->authorisation == DM_SM_AUTHORISE_ACCEPT_TWICE) ||
         (cmPrim->authorisation == DM_SM_AUTHORISE_ACCEPT_LIFE_OF_ACL)) && (cmPrim->protocolId == CSR_BT_SC_PROTOCOL_L2CAP))
    {
        cmScMarkPeerAsAuthorised(cmData, &cmPrim->deviceAddr, cmPrim->channel);
    }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    if (((cmPrim->authorisation == DM_SM_AUTHORISE_REJECT_ONCE) ||
         (cmPrim->authorisation == DM_SM_AUTHORISE_REJECT_TWICE) ||
         (cmPrim->authorisation == DM_SM_AUTHORISE_REJECT_LIFE_OF_ACL)) && (cmPrim->protocolId == CSR_BT_SC_PROTOCOL_RFCOMM))
    {
        cmRfcConnElement *rfcElement = CsrBtCmRfcFindRfcConnElementFromDeviceAddrState1OrState2(cmData,
                                                                                                &cmPrim->deviceAddr,
                                                                                                CSR_BT_CM_RFC_STATE_CONNECT_ACCEPT,
                                                                                                CSR_BT_CM_RFC_STATE_CONNECT_ACCEPT_FINAL);
        if (rfcElement)
        {
           cmRfcConnInstType *theLogicalLink   = rfcElement->cmRfcConnInst;
           CsrBtCmDisconnectReqSend(theLogicalLink->btConnId);
        }
    }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
}


static CsrBool CsrBtCmDmAuthoriseIndRejectHandler(cmInstanceData_t *cmData)
{
    DM_SM_AUTHORISE_IND_T *prim;
    CsrUint8 theIndex = CM_ERROR;

    prim = (DM_SM_AUTHORISE_IND_T *) cmData->recvMsgP;

    if (prim->cs.connection.service.protocol_id == CSR_BT_SC_PROTOCOL_L2CAP)
    {
#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
        if ((psm_t)prim->cs.connection.service.channel == CSR_BT_PAN_BNEP_PSM)
        {
            theIndex = returnReserveBnepIdIndex(cmData);
        }
        else
#endif
        {
#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
            cmL2caConnElement *theElement = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromReserveBtConnIdPsm, &prim->cs.connection.service.channel);

            if (theElement)
            {
                theIndex = theElement->elementId;
            }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */
        }
    }
    else
    {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
        cmRfcConnElement *theElement = CsrBtCmRfcFindRfcConnElementFromServerState(cmData,
                                                                                   (CsrUint8) prim->cs.connection.service.channel,
                                                                                   CSR_BT_CM_RFC_STATE_CONNECT_ACCEPT);
        if (theElement)
        {
            theIndex = theElement->elementId;
        }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
    }

    if (prim->cs.incoming && theIndex == CM_ERROR)
    {/* We don't have any servers for this incoming connection. Simply reject it. */
        dm_sm_authorise_rsp(&prim->cs.connection.addrt, prim->cs.connection.service.protocol_id, prim->cs.connection.service.channel,
                            prim->cs.incoming, DM_SM_AUTHORISE_REJECT_ONCE, NULL);

        return TRUE;
    }

    return FALSE;
}

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
static CsrBool CsrBtCmDmAuthoriseIndAutoAcceptHandler(cmInstanceData_t *cmData)
{
    DM_SM_AUTHORISE_IND_T *prim;

    prim = (DM_SM_AUTHORISE_IND_T *) cmData->recvMsgP;

    if (prim && prim->cs.incoming &&
        prim->cs.connection.service.protocol_id == CSR_BT_SC_PROTOCOL_L2CAP &&
        CsrBtCmAutoAcceptForThisPsmAllowed(cmData, &prim->cs.connection.addrt.addr, prim->cs.connection.service.channel))
    {/* Auto accept allowed, so it accept it. */
        dm_sm_authorise_rsp(&prim->cs.connection.addrt, prim->cs.connection.service.protocol_id, prim->cs.connection.service.channel,
                            prim->cs.incoming, DM_SM_AUTHORISE_ACCEPT_ONCE, NULL);

        CsrBtCmAllowAutoAcceptForThisPsm(cmData, &prim->cs.connection.addrt.addr, prim->cs.connection.service.channel);

        return TRUE;
    }

    return FALSE;
}
#else
#define CsrBtCmDmAuthoriseIndAutoAcceptHandler(cmData) FALSE
#endif

void CsrBtCmDmAuthoriseIndHandler(cmInstanceData_t *cmData)
{
#ifdef EXCLUDE_CSR_BT_SC_MODULE
    DM_SM_AUTHORISE_IND_T *ind = (DM_SM_AUTHORISE_IND_T*) cmData->recvMsgP;

    if (CsrBtScDeviceAuthorised(ind->cs.connection.addrt.type,
                                &ind->cs.connection.addrt.addr))
    {
        CsrBtCmScDmAuthoriseRes(ind->cs.connection.addrt.addr,
                                ind->cs.incoming,
                                DM_SM_AUTHORISE_ACCEPT_LIFE_OF_ACL,
                                ind->cs.connection.service.channel,
                                ind->cs.connection.service.protocol_id,
                                ind->cs.connection.addrt.type);
    }
    else
#endif /* EXCLUDE_CSR_BT_SC_MODULE */
    {
        if (!CsrBtCmDmAuthoriseIndRejectHandler(cmData) &&
            !CsrBtCmDmAuthoriseIndAutoAcceptHandler(cmData))
        {
            /* Allow SC to process this message */
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_AUTHORISE_IND);
        }
    }
}

#ifndef EXCLUDE_CSR_BT_SC_MODULE
void CsrBtCmGetSecurityConfResHandler(cmInstanceData_t *cmData)
{
    CsrBtCmGetSecurityConfRes * prim = (CsrBtCmGetSecurityConfRes *) cmData->recvMsgP;
    DM_SM_KEY_STATE_T ks;

    CsrMemSet(&ks, 0, sizeof(DM_SM_KEY_STATE_T));
#ifdef CSR_BT_LE_ENABLE
    if (prim->leEr && prim->leErCount == 8)
    {
        SynMemCpyS(ks.er, sizeof(CsrUint16)*8, prim->leEr, sizeof(CsrUint16)*8);
    }

    if (prim->leIr && prim->leIrCount == 8)
    {
        SynMemCpyS(ks.ir, sizeof(CsrUint16)*8, prim->leIr, sizeof(CsrUint16)*8);
    }
#endif

    /* The SC has fetched the security setup */
    dm_sm_init_req_le(prim->options,
                      prim->securityMode,
                      prim->securityLevelDefault,
                      prim->config,
                      prim->writeAuthEnable,
                      prim->mode3enc,
                      &ks, /* sm_key_state */
                      prim->leSmDivState,
                      prim->leSmSignCounter,
                      NULL);
}
#endif /* !EXCLUDE_CSR_BT_SC_MODULE */

/* CM start-up handler for initial DM_SM_INIT_CFM */
void CsrBtCmDmSmInitCfmHandler(cmInstanceData_t *cmData)
{
    DM_SM_INIT_CFM_T *dmPrim = (DM_SM_INIT_CFM_T*) cmData->recvMsgP;
#ifdef EXCLUDE_CSR_BT_SC_MODULE
    /* Low Energy requires a few persistent local keys. Write these now */
#ifdef CSR_BT_LE_ENABLE
    CsrBtCmLeUpdateLocalDbKeys(dmPrim);
#endif
#else /* EXCLUDE_CSR_BT_SC_MODULE */
    CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_INIT_CFM);
#endif /* !EXCLUDE_CSR_BT_SC_MODULE */

    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
    {
        /* We are currently in CM initialization phase, continue with the sequence. */
        CmInitSequenceHandler(cmData,
                              CM_INIT_SEQ_SM_INIT_CFM,
                              dmPrim->status,
                              CSR_BT_SUPPLIER_HCI);
#ifdef CSR_BT_LE_ENABLE
        /* Generate local random address as per the cmake configure flag's value */
        CsrBtCmLeConfigureRandomAddress(cmData);
#endif
    }
    else
    {
        CsrBtCmDmLocalQueueHandler();
    }
}

void CsrBtCmDmSmRemoveDeviceCfmHandler(cmInstanceData_t *cmData)
{
    CsrBool unlockDm = (cmData->dmVar.lockMsg == CSR_BT_CM_SM_REMOVE_DEVICE_REQ ? TRUE : FALSE) ;
#ifdef EXCLUDE_CSR_BT_SC_MODULE
    switch (cmData->dmVar.lockMsg)
    {
        case CM_DM_REMOVE_DEVICE_KEY_REQ:
        {
             DM_SM_REMOVE_DEVICE_CFM_T *cfm = (DM_SM_REMOVE_DEVICE_CFM_T *)cmData->recvMsgP;

            /* Prepare device key confirmation and send. */
            CmDmRemoveDeviceKeyConfirmSend(cmData,
                                           &cfm->addrt.addr,
                                           cfm->addrt.type,
                                           (cfm->status == HCI_SUCCESS ? CSR_BT_RESULT_CODE_CM_SUCCESS : cfm->status),
                                           (cfm->status == HCI_SUCCESS ? CSR_BT_SUPPLIER_CM : CSR_BT_SUPPLIER_HCI));

            unlockDm = TRUE;
        }
        break;

        case CM_DM_REMOVE_DEVICE_OPTIONS_REQ:
        {
            DM_SM_REMOVE_DEVICE_CFM_T *cfm = (DM_SM_REMOVE_DEVICE_CFM_T *)cmData->recvMsgP;
            
            /* Decrement the deviceIndex counter on receiving confirmation from Bluestack */ 
            if (cmData->scVar.deviceIndex)
            {
                cmData->scVar.deviceIndex--;
            }

            /* Once all the device removal requests have been handled, send the confirmation to the application  and unlock the queue */
            if (cmData->scVar.deviceIndex == 0)
            {
                /* If operatingBdAddr was set to all 0s and address type received in confirmation is not TBDADDR_INVALID,
                 * it means that all the remove device requests have been processed now.
                 * This will be considered as a success scenario.
                 */
                if (CsrBtBdAddrEqZero(&cmData->dmVar.operatingBdAddr) && cfm->addrt.type != TBDADDR_INVALID)
                {
                    cfm->addrt.type = TBDADDR_INVALID;
                    cfm->status = HCI_SUCCESS;
                }

                /* Prepare device remove confirmation and send. */
                CmDmRemoveDeviceOptionsConfirmSend(cmData,
                                                   &cmData->dmVar.operatingBdAddr,
                                                   cfm->addrt.type,
                                                   (cfm->status == HCI_SUCCESS ? CSR_BT_RESULT_CODE_CM_SUCCESS : cfm->status),
                                                   (cfm->status == HCI_SUCCESS ? CSR_BT_SUPPLIER_CM : CSR_BT_SUPPLIER_HCI));

                unlockDm = TRUE;
            }
        }
        break;

        default:
        {
             /* Ignore */
        }
        break;
    }
#else /* !EXCLUDE_CSR_BT_SC_MODULE */
    /* Send this to SC module.*/
    CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_REMOVE_DEVICE_CFM);
#endif /* EXCLUDE_CSR_BT_SC_MODULE */

    /* dm_sm_remove_device_req() is also called internally from CM
     * eg. from CsrBtCmDatabaseReqHandler -> csrBtCmDbRemoveKeys().
     * These calls shall not unlock DM Lock.
     */
    if (unlockDm)
    {
        CsrBtCmDmLocalQueueHandler();
    }
}

void CsrBtCmDmSmAddDeviceCfmHandler(cmInstanceData_t *cmData)
{
#ifdef EXCLUDE_CSR_BT_SC_MODULE
    CsrBool dmUnlock = FALSE;

    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
    { /* Continue adding device records from persistent storage to SM DB */
        CsrBtCmSmDbAddDeviceIndex(cmData, cmData->scVar.deviceIndex + 1);
    }
    else
    {
        if (cmData->scVar.locked)
        {
            cmData->scVar.locked = FALSE;
            dmUnlock = TRUE;
        }

        if (cmData->dmVar.lockMsg == CM_DM_REMOVE_DEVICE_KEY_REQ)
        {
            DM_SM_ADD_DEVICE_CFM_T *cfm = (DM_SM_ADD_DEVICE_CFM_T *)cmData->recvMsgP;

            /* Prepare device key confirmation and send. */
            CmDmRemoveDeviceKeyConfirmSend(cmData,
                                           &cfm->addrt.addr,
                                           cfm->addrt.type,
                                           (cfm->status == HCI_SUCCESS ? CSR_BT_RESULT_CODE_CM_SUCCESS : cfm->status),
                                           (cfm->status == HCI_SUCCESS ? CSR_BT_SUPPLIER_CM : CSR_BT_SUPPLIER_HCI));
            dmUnlock = TRUE;
        }
    }

    if (dmUnlock)
    {
        CsrBtCmDmLocalQueueHandler();
    }
#else /* !EXCLUDE_CSR_BT_SC_MODULE */
    CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_ADD_DEVICE_CFM);
#endif /* EXCLUDE_CSR_BT_SC_MODULE */
}

void CsrBtCmDmSmSecurityCfmHandler(cmInstanceData_t *cmData)
{
    CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_SECURITY_CFM);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtDmSmKeyRequestIndHandler(cmInstanceData_t *cmData)
{
#ifdef INSTALL_CM_KEY_REQUEST_INDICATION
    CsrBtCmScMessagePut(cmData, CM_SM_KEY_REQUEST_IND);
#else
    DM_SM_KEY_REQUEST_IND_T *dmPrim = (DM_SM_KEY_REQUEST_IND_T *) cmData->recvMsgP;

    /* All device records are already shared with Bluestack during
     * initialization or pairing */
    dm_sm_key_request_neg_rsp(&dmPrim->addrt, dmPrim->key_type, NULL);
#endif /* INSTALL_CM_KEY_REQUEST_INDICATION */  
}

void CsrBtCmDmSmKeysIndHandler(cmInstanceData_t *cmData)
{
#ifdef EXCLUDE_CSR_BT_SC_MODULE
    DM_SM_KEYS_IND_T *dmPrim = (DM_SM_KEYS_IND_T *) cmData->recvMsgP;
    CsrBtTransportMask transportMask;
    CsrBtTypedAddr *addrt = &dmPrim->addrt;
    CsrBool bond;

    transportMask = ((dmPrim->keys.present & DM_SM_KEY_MASK) == DM_SM_KEY_ENC_BREDR) ? CSR_BT_TRANSPORT_TYPE_FLAG_BREDR : CSR_BT_TRANSPORT_TYPE_FLAG_LE;
    bond = CmSmIsBondEnabled(cmData, &dmPrim->addrt.addr, dmPrim->addrt.type, (transportMask >> 1));

    if (!CsrBtBdAddrEqZero(&dmPrim->id_addrt.addr))
    {
        addrt = &dmPrim->id_addrt;
    }

    if (transportMask == CSR_BT_TRANSPORT_TYPE_FLAG_BREDR)
    {
        /* Handle BR/EDR keys Indication */
        if (bond)
        {
            CsrBtCmBredrKeysHandler(cmData);
        }
    }
#ifdef CSR_BT_LE_ENABLE
    else
    {
        /* Handle LE keys indication */
        CmPropgateAddressMappedIndEvent(cmData,
                                        &dmPrim->addrt.addr,
                                        &dmPrim->id_addrt);
        if (bond)
        {
            CsrBtCmLeKeysHandler(cmData);
        }
#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
        /* Read CAR & RPA only characteristics for the connected device
         * if local controller supports privacy feature */
        if (cmData->leVar.llFeaturePrivacy)
        {
            CsrBtGattReadRemoteRpaOnlyCharReqSend(*addrt);
        }
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */
    }
#endif /* CSR_BT_LE_ENABLE */

    if (bond)
    {
        dmPrim->keys.present |= DM_SM_KEYS_UPDATE_EXISTING;
        CsrBtCmDmSmAddDevice(addrt, DM_SM_TRUST_UNCHANGED, &dmPrim->keys);
        /* Reset 'dmPrim->keys' to avoid freeing of the keys pointers.
         * DM will free the Keys after adding the keys in to SMDB. */
        CsrMemSet(&dmPrim->keys, 0x00, sizeof(dmPrim->keys));

        CsrBtCmPropgateSecurityEventIndEvent(cmData,
                                             transportMask,
                                             addrt->type,
                                             &addrt->addr,
                                             CSR_BT_CM_SECURITY_EVENT_BOND);
    }

#else /* EXCLUDE_CSR_BT_SC_MODULE */
    CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_KEYS_IND);
#endif /* !EXCLUDE_CSR_BT_SC_MODULE */
}

static const CsrUint8 cmScPrimSize[] =
{
    sizeof(DM_SM_PIN_REQUEST_IND_T),
    sizeof(DM_SM_IO_CAPABILITY_RESPONSE_IND_T),
    sizeof(DM_SM_IO_CAPABILITY_REQUEST_IND_T),
    sizeof(DM_SM_USER_CONFIRMATION_REQUEST_IND_T),
    sizeof(DM_SM_USER_PASSKEY_REQUEST_IND_T),
    sizeof(DM_SM_USER_PASSKEY_NOTIFICATION_IND_T),
    sizeof(DM_SM_KEYPRESS_NOTIFICATION_IND_T),
    sizeof(DM_SM_SIMPLE_PAIRING_COMPLETE_IND_T),
    sizeof(DM_SM_AUTHENTICATE_CFM_T),
    sizeof(DM_SM_SECURITY_IND_T),
    sizeof(DM_SM_CSRK_COUNTER_CHANGE_IND_T),
    sizeof(DM_SM_READ_LOCAL_OOB_DATA_CFM_T),
    sizeof(DM_SM_LOCAL_KEY_DELETED_IND_T),
    sizeof(DM_SM_SECURITY_CFM_T),
    sizeof(DM_SM_KEYS_IND_T),
    sizeof(DM_SM_BONDING_CFM_T),
    sizeof(DM_SM_ENCRYPTION_CHANGE_IND_T),
    sizeof(DM_SM_ENCRYPT_CFM_T),
    sizeof(DM_SM_ACCESS_IND_T),
    sizeof(DM_SM_AUTHORISE_IND_T),
    sizeof(DM_SM_INIT_CFM_T),
    sizeof(DM_SM_REMOVE_DEVICE_CFM_T),
    sizeof(DM_SM_ADD_DEVICE_CFM_T),
    sizeof(DM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_IND_T),
    sizeof(DM_SM_KEY_REQUEST_IND_T),
};

static const CsrUint8 cmHciPrimSize[] =
{
    sizeof(DM_HCI_CREATE_CONNECTION_CANCEL_CFM_T),
    sizeof(DM_HCI_DELETE_STORED_LINK_KEY_CFM_T),
    sizeof(DM_HCI_REFRESH_ENCRYPTION_KEY_IND_T),
};


void CsrBtCmScMessagePut(cmInstanceData_t *cmData, CsrBtCmPrim primId)
{
#if defined( CSR_TARGET_PRODUCT_VM ) || defined( CSR_TARGET_PRODUCT_WEARABLE ) 
    if (cmData->recvMsgP)
    {
        if (primId >= CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST &&
            primId <= CSR_BT_CM_SM_PRIM_UPSTREAM_HIGHEST)
{
            const CsrUint8 cmSmPrimId = primId- CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST;
            void *prim = CsrMemDup(cmData->recvMsgP, cmScPrimSize[cmSmPrimId]);

            *(CsrBtCmPrim*) prim = primId;

            CsrSchedMessagePut(CSR_BT_CM_SC_HANDLE(cmData),
                               CSR_BT_CM_PRIM,
                               prim);
        }
    }
#else
    if ((cmData->recvMsgP) != NULL)
    {
        CsrSchedMessagePut(cmData->scHandle, DM_PRIM, (cmData->recvMsgP));
    }
    cmData->recvMsgP = NULL;
#endif
}

void CsrBtCmHciMessagePut(cmInstanceData_t *cmData, CsrBtCmPrim primId)
{
#if defined( CSR_TARGET_PRODUCT_VM ) || defined( CSR_TARGET_PRODUCT_WEARABLE ) 
    if (cmData->recvMsgP)
    {
        if (primId >= CSR_BT_CM_HCI_PRIM_UPSTREAM_LOWEST &&
            primId <= CSR_BT_CM_HCI_PRIM_UPSTREAM_HIGHEST)
        {
            const CsrUint8 cmHciPrimId = primId- CSR_BT_CM_HCI_PRIM_UPSTREAM_LOWEST;
            void *prim = CsrMemDup(cmData->recvMsgP, cmHciPrimSize[cmHciPrimId]);

            *(CsrBtCmPrim*) prim = primId;

            CsrSchedMessagePut(CSR_BT_CM_SC_HANDLE(cmData),
                               CSR_BT_CM_PRIM,
                               prim);
        }
    }
#else
    if((cmData->recvMsgP) != NULL){
        CsrSchedMessagePut(cmData->scHandle, DM_PRIM, (cmData->recvMsgP));
    }
    cmData->recvMsgP = NULL;
#endif
}

void CmWriteScHostSupportOverrideReqSend(CsrSchedQid        appHandle,
                                         CsrBtDeviceAddr   *deviceAddr,
                                         CmScOverrideAction overrideAction)
{
    CmWriteScHostSupportOverrideReq *msg = (CmWriteScHostSupportOverrideReq*) CsrPmemAlloc(sizeof(*msg));

    msg->type           = CM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ;
    msg->deviceAddr     = *deviceAddr;
    msg->appHandle      = appHandle;
    msg->overrideAction = overrideAction;

    CsrBtCmMsgTransport(msg);
}

void CmReadScHostSupportOverrideMaxBdAddrReqSend(CsrSchedQid appHandle)
{
    CmReadScHostSupportOverrideMaxBdAddrReq *msg = (CmReadScHostSupportOverrideMaxBdAddrReq*) CsrPmemAlloc(sizeof(*msg));

    msg->type           = CM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_REQ;
    msg->appHandle      = appHandle;

    CsrBtCmMsgTransport(msg);
}

void CmWriteScHostSupportOverrideReqHandler(cmInstanceData_t *cmData)
{
    CmWriteScHostSupportOverrideReq *req = (CmWriteScHostSupportOverrideReq *)cmData->recvMsgP;

    cmData->dmVar.appHandle = req->appHandle;
    dm_write_sc_host_support_override_req(req->appHandle,
                                          &req->deviceAddr,
                                          req->overrideAction,
                                          NULL);
}

void CmReadScHostSupportOverrideMaxBdAddrReqHandler(cmInstanceData_t *cmData)
{
    CmReadScHostSupportOverrideMaxBdAddrReq *req = (CmReadScHostSupportOverrideMaxBdAddrReq *)cmData->recvMsgP;

    cmData->dmVar.appHandle = req->appHandle;
    dm_read_sc_host_support_override_max_bd_addr_req(req->appHandle, NULL);
}

void CmDmWriteScHostSupportOverrideCfmHandler(cmInstanceData_t *cmData)
{
    DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM_T *dmPrim = (DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM_T *)cmData->recvMsgP;
    CmWriteScHostSupportOverrideCfm *cfm = (CmWriteScHostSupportOverrideCfm *)CsrPmemAlloc(sizeof(*cfm));

    cfm->type                = CM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM;
    cfm->deviceAddr          = dmPrim->bd_addr;
    cfm->hostSupportOverride = dmPrim->host_support_override;
    cfm->status              = dmPrim->status;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}

void CmDmReadScHostSupportOverrideMaxBdAddrCfmHandler(cmInstanceData_t *cmData)
{
    DM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_CFM_T *dmPrim = (DM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_CFM_T *)cmData->recvMsgP;
    CmReadScHostSupportOverrideMaxBdAddrCfm *cfm = (CmReadScHostSupportOverrideMaxBdAddrCfm *)CsrPmemAlloc(sizeof(*cfm));

    cfm->type               = CM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_CFM;
    cfm->status             = dmPrim->status;
    cfm->maxOverrideBdAddr  = dmPrim->max_override_bdaddr;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}

void CmSmRefreshEncryptionKeyReqHandler(cmInstanceData_t *cmData)
{
    CmSmRefreshEncryptionKeyReq *req = (CmSmRefreshEncryptionKeyReq *)cmData->recvMsgP;
    dm_hci_refresh_encryption_key(&req->deviceAddr, NULL);
}

void CmSmGenerateCrossTransKeyRequestRspHandler(cmInstanceData_t *cmData)
{
    CmSmGenerateCrossTransKeyRequestRsp *rsp = (CmSmGenerateCrossTransKeyRequestRsp *)cmData->recvMsgP;

    dm_sm_generate_cross_trans_key_request_rsp(&rsp->tpAddr,
                                               rsp->identifier,
                                               rsp->flags,
                                               NULL);
}

void CmDmChangeConnectionLinkKeyReqHandler(cmInstanceData_t *cmData)
{
    CmDmChangeConnectionLinkKeyReq *req = (CmDmChangeConnectionLinkKeyReq *) cmData->recvMsgP;

    cmData->dmVar.appHandle = req->appHandle;
    dm_hci_change_link_key(&req->deviceAddr, NULL);
}

void CmDmChangeConnectionLinkKeyCfmHandler(cmInstanceData_t *cmData)
{
    DM_HCI_CHANGE_CONN_LINK_KEY_CFM_T *dmCfm = (DM_HCI_CHANGE_CONN_LINK_KEY_CFM_T *)cmData->recvMsgP;
    CmDmChangeConnectionLinkKeyCfm *cfm = (CmDmChangeConnectionLinkKeyCfm *)CsrPmemAlloc(sizeof(*cfm));

    cfm->type = CM_DM_CHANGE_CONNECTION_LINK_KEY_CFM;
    cfm->deviceAddr = dmCfm->bd_addr;

    if (dmCfm->status == HCI_SUCCESS)
    {
        cfm->resultCode     = CSR_BT_RESULT_CODE_CM_SUCCESS;
        cfm->resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        cfm->resultCode      = (CsrBtResultCode) dmCfm->status;
        cfm->resultSupplier  = CSR_BT_SUPPLIER_HCI;
    }

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
    CsrBtCmDmLocalQueueHandler();
}

#ifdef INSTALL_CM_SM_CONFIG
void CmSmConfigReqHandler(cmInstanceData_t *cmData)
{
    CmSmConfigReq *req = (CmSmConfigReq *) cmData->recvMsgP;
    cmPendingMsg_t *pendingMsg;

    CsrPCmnListAddLast(cmData->pendingMsgs, sizeof(cmPendingMsg_t), pendingMsg);
    pendingMsg->type = CM_PENDING_MSG_SM_CONFIG_PARAMS;
    pendingMsg->arg.commonParams.appHandle = req->appHandle;

    dm_sm_config_req(CSR_BT_CM_IFACEQUEUE, req->configMask, 0, NULL, NULL);
}

void CmSmConfigCompleteHandler(cmInstanceData_t *cmData)
{
    DM_SM_CONFIG_CFM_T *dmCfm = (DM_SM_CONFIG_CFM_T *) cmData->recvMsgP;
    cmPendingMsg_t *pendingMsg;
    CsrUint8 type = CM_PENDING_MSG_SM_CONFIG_PARAMS;

    pendingMsg = (cmPendingMsg_t *) CsrCmnListSearch((CsrCmnList_t *) cmData->pendingMsgs,
                                                     CmSearchPendingListByType,
                                                     (void *) &type);

    if (pendingMsg)
    {
        CmSmConfigCfm *cfm = (CmSmConfigCfm *) CsrPmemAlloc(sizeof(CmSmConfigCfm));

        cfm->type = CM_SM_CONFIG_CFM;

        if (dmCfm->status == DM_SM_CONFIG_STATUS_SUCCESS)
        {
            cfm->resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
            cfm->resultSupplier = CSR_BT_SUPPLIER_CM;
        }
        else
        {
            cfm->resultCode = dmCfm->status;
            cfm->resultSupplier = CSR_BT_SUPPLIER_DM;
        }

        CsrBtCmPutMessage(pendingMsg->arg.commonParams.appHandle, cfm);
        CsrPCmnListRemove(cmData->pendingMsgs, (CsrCmnListElm_t* ) pendingMsg);
    }
}
#endif /* INSTALL_CM_SM_CONFIG */

