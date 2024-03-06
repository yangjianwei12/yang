/******************************************************************************
 Copyright (c) 2008-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_prim.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm_sc_ssp_lib.h"
#include "dm_prim.h"
#include "csr_types.h"
#include "bluetooth.h"

#ifdef CSR_BT_INSTALL_CM_SC_MODE_CONFIG
void CsrBtCmScDmSecModeConfigReq(CsrUint16 writeAuthEnable, CsrUint16 config)
{
    CsrBtCmSmSecModeConfigReq    *prim;

    prim                  = (CsrBtCmSmSecModeConfigReq *) CsrPmemZalloc(sizeof(*prim));
    prim->type            = CSR_BT_CM_SM_SEC_MODE_CONFIG_REQ;
    prim->writeAuthEnable = writeAuthEnable;
    prim->config          = config;
    CsrBtCmPutMessageDownstream(prim);
}
#endif

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1

void CsrBtCmScDmIoCapabilityRequestRes(CsrBtDeviceAddr deviceAddr,
                                       CsrBtAddressType addressType,
                                       CsrBtTransportType transportType,
                                       CsrUint8 ioCapability,
                                       CsrUint8 authenticationRequirements,
                                       CsrUint8 oobDataPresent,
                                       CsrUint8 *oobHashC,
                                       CsrUint8 *oobRandR,
                                       CsrUint16 keyDistribution)
{
    CsrBtCmSmIoCapabilityRequestRes *prim = (CsrBtCmSmIoCapabilityRequestRes *) CsrPmemZalloc(sizeof(*prim));
    prim->type = CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_RES;
    prim->deviceAddr = deviceAddr;
    prim->addressType = addressType;
    prim->transportType = transportType;
    prim->ioCapability = ioCapability;
    prim->authenticationRequirements = authenticationRequirements;
    prim->oobDataPresent = oobDataPresent;
    prim->oobHashC = oobHashC;
    prim->oobRandR = oobRandR;
    prim->keyDistribution = keyDistribution;
    CsrBtCmPutMessageDownstream(prim);
}

void CsrBtCmScDmIoCapabilityRequestNegRes(CsrBtDeviceAddr deviceAddr,
                                          CsrBtAddressType addressType,
                                          CsrBtTransportType transportType,
                                          hci_error_t reason)
{
    CsrBtCmSmIoCapabilityRequestNegRes    *prim;

    prim = (CsrBtCmSmIoCapabilityRequestNegRes *) CsrPmemZalloc(sizeof(*prim));
    prim->type = CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_NEG_RES;
    prim->deviceAddr = deviceAddr;
    prim->addressType = addressType;
    prim->transportType = transportType;
    prim->reason = reason;

    CsrBtCmPutMessageDownstream(prim);
}

void CsrBtCmScDmUserConfirmationRequestNegRes(CsrBtDeviceAddr deviceAddr,
                                              CsrBtAddressType addressType,
                                              CsrBtTransportType transportType)
{
    CsrBtCmSmUserConfirmationRequestNegRes    *prim;

    prim             = (CsrBtCmSmUserConfirmationRequestNegRes *) CsrPmemZalloc(sizeof(*prim));
    prim->type       = CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_NEG_RES;
    prim->deviceAddr = deviceAddr;
    prim->reason     = HCI_ERROR_AUTH_FAIL; /* user rejected */
    prim->addressType = addressType;
    prim->transportType = transportType;
    CsrBtCmPutMessageDownstream(prim);
}

void CsrBtCmScDmUserPasskeyRequestNegRes(CsrBtDeviceAddr deviceAddr,
                                         CsrBtAddressType addressType,
                                         CsrBtTransportType transportType)
{
    CsrBtCmSmUserPasskeyRequestNegRes    *prim;

    prim             = (CsrBtCmSmUserPasskeyRequestNegRes *) CsrPmemZalloc(sizeof(*prim));
    prim->type       = CSR_BT_CM_SM_USER_PASSKEY_REQUEST_NEG_RES;
    prim->deviceAddr = deviceAddr;
    prim->numericValue = 0;
    prim->addressType = addressType;
    prim->transportType = transportType;
    CsrBtCmPutMessageDownstream(prim);
}

#ifdef INSTALL_CM_SM_REPAIR
void CsrBtCmSmSendRepairRes(CsrBtDeviceAddr deviceAddr,
                            CsrBool accept,
                            CsrUint16 repairId,
                            CsrBtAddressType addressType)
{
    CsrBtCmSmRepairRes    *prim;

    prim                   = (CsrBtCmSmRepairRes *) CsrPmemZalloc(sizeof(*prim));
    prim->type             = CSR_BT_CM_SM_REPAIR_RES;
    prim->deviceAddr       = deviceAddr;
    prim->accept           = accept;
    prim->repairId         = repairId;
    prim->addressType = addressType;
    CsrBtCmPutMessageDownstream(prim);
}
#endif /* INSTALL_CM_SM_REPAIR */

#endif
