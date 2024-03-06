/******************************************************************************
 Copyright (c) 2009-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm_sc_lib.h"
#include "csr_bt_cm_lib.h"

void CsrBtCmScDmDeleteStoredLinkKeyReq(CsrBtDeviceAddr deviceAddr, delete_all_flag_t flag)
{
    CsrBtCmSmDeleteStoreLinkKeyReq *prim = (CsrBtCmSmDeleteStoreLinkKeyReq *) CsrPmemAlloc(sizeof(CsrBtCmSmDeleteStoreLinkKeyReq));
    prim->type = CSR_BT_CM_SM_DELETE_STORE_LINK_KEY_REQ;
    prim->flag = flag;
    prim->deviceAddr = deviceAddr;
    CsrBtCmPutMessageDownstream(prim);
}

void CsrBtCmScDmAddDeviceReq(CsrBtTypedAddr typedAddr,
                             DM_SM_TRUST_T  trust,
                             DM_SM_KEYS_T   *keys)
{
    CsrBtCmSmAddDeviceReq *prim = (CsrBtCmSmAddDeviceReq *) CsrPmemAlloc(sizeof(CsrBtCmSmAddDeviceReq));

    prim->type = CSR_BT_CM_SM_ADD_DEVICE_REQ;
    prim->typedAddr = typedAddr;
    prim->trust = trust;
    prim->keys = keys;
    CsrBtCmPutMessageDownstream(prim);
}

void CsrBtCmScDmRemoveDeviceReq(CsrBtDeviceAddr deviceAddr,
                                CsrBtAddressType addressType)
{
    CsrBtCmSmRemoveDeviceReq *prim = (CsrBtCmSmRemoveDeviceReq *) CsrPmemAlloc(sizeof(CsrBtCmSmRemoveDeviceReq));
    prim->type = CSR_BT_CM_SM_REMOVE_DEVICE_REQ;
    prim->deviceAddr = deviceAddr;
    prim->addressType = addressType;
    CsrBtCmPutMessageDownstream(prim);
}

#ifdef CSR_BT_INSTALL_SC_AUTHENTICATE
void CsrBtCmScDmAuthenticateReq(CsrBtDeviceAddr deviceAddr)
{
    CsrBtCmSmAuthenticateReq *prim = (CsrBtCmSmAuthenticateReq *) CsrPmemAlloc(sizeof(CsrBtCmSmAuthenticateReq));
    prim->type = CSR_BT_CM_SM_AUTHENTICATE_REQ;
    prim->deviceAddr = deviceAddr;
    CsrBtCmPutMessageDownstream(prim);
}
#endif

#ifdef CSR_BT_INSTALL_SC_ENCRYPTION
void CsrBtCmScDmEncryptionReq(CsrSchedQid appHandle, CsrBtDeviceAddr deviceAddr, CsrBool encMode)
{
    CsrBtCmSmEncryptionReq *prim = (CsrBtCmSmEncryptionReq *) CsrPmemAlloc(sizeof(CsrBtCmSmEncryptionReq));
    prim->type = CSR_BT_CM_SM_ENCRYPTION_REQ;
    prim->appHandle = appHandle;
    prim->deviceAddr = deviceAddr;
    prim->encryptionMode = encMode;
    CsrBtCmPutMessageDownstream(prim);
}
#endif

void CsrBtCmScDmAuthoriseRes(CsrBtDeviceAddr deviceAddr,
                             CsrBool incoming,
                             CsrUint16 authorisation,
                             CsrUint16 channel,
                             dm_protocol_id_t protocolId,
                             CsrBtAddressType addressType)
{
    CsrBtCmSmAuthoriseRes *prim = (CsrBtCmSmAuthoriseRes *) CsrPmemAlloc(sizeof(CsrBtCmSmAuthoriseRes));
    prim->type = CSR_BT_CM_SM_AUTHORISE_RES;
    prim->authorisation = authorisation;
    prim->channel = channel;
    prim->deviceAddr = deviceAddr;
    prim->incoming = incoming;
    prim->protocolId = protocolId;
    prim->addressType = addressType;
    CsrBtCmPutMessageDownstream(prim);
}

void CsrBtCmScDmPinRequestRes(CsrBtDeviceAddr deviceAddr, CsrUint8 pinLength, CsrUint8 *pin)
{
    CsrUint8 pinLen = CSRMIN(pinLength, CSR_BT_PASSKEY_MAX_LEN);
    CsrBtCmSmPinRequestRes *prim = (CsrBtCmSmPinRequestRes *) CsrPmemAlloc(sizeof(CsrBtCmSmPinRequestRes));
    prim->type = CSR_BT_CM_SM_PIN_REQUEST_RES;
    prim->deviceAddr = deviceAddr;
    CsrMemSet(prim->pin,0,CSR_BT_PASSKEY_MAX_LEN);
    if ((pin != NULL) && (pinLen > 0))
    {
        SynMemCpyS(prim->pin, CSR_BT_PASSKEY_MAX_LEN, pin, pinLen);
    }
    prim->pinLength = pinLen;
    CsrBtCmPutMessageDownstream(prim);
}

void CsrBtCmScDmPinRequestNegRes(CsrBtDeviceAddr deviceAddr)
{
    CsrBtCmScDmPinRequestRes(deviceAddr, 0, NULL);
}

void CsrBtCmScDmRegisterReq( dm_protocol_id_t protocolId, CsrUint16 channel, CsrBool outgoingOk,
                        dm_security_level_t securityLevel, CsrUint8 minEncKeySize)
{
    CsrBtCmSmRegisterReq *prim = (CsrBtCmSmRegisterReq *) CsrPmemAlloc(sizeof(CsrBtCmSmRegisterReq));
    prim->type = CSR_BT_CM_SM_REGISTER_REQ;
    prim->channel = channel;
    prim->outgoingOk = outgoingOk;
    prim->protocolId = protocolId;
    prim->securityLevel = securityLevel;
    prim->minEncKeySize = minEncKeySize;
    CsrBtCmPutMessageDownstream(prim);
}
