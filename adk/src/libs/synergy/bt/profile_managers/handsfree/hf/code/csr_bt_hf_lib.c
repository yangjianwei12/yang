/******************************************************************************
 Copyright (c) 2009-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_msg_transport.h"
#include "csr_bt_hf_lib.h"

#ifdef CSR_TARGET_PRODUCT_VM
#include "csr_bt_hf_util.h"
#endif /* CSR_TARGET_PRODUCT_VM */

void CsrBtHfMsgTransport(void *msg)
{
    CsrMsgTransport(CSR_BT_HF_IFACEQUEUE, CSR_BT_HF_PRIM, msg);
}

#ifdef CSR_TARGET_PRODUCT_VM
void CsrBtHfUpdateScoHandle(CsrBtConnId connId,
                            hci_connection_handle_t scoHandle)
{
    CsrBtDeviceAddr dummy_addr;
    CsrBtBdAddrZero(&dummy_addr);
    CsrBtHfSetScoHandle(&csrBtHfInstance, connId, dummy_addr, scoHandle);
}

void HfUpdateScoHandleWithAddr(CsrBtDeviceAddr addr, hci_connection_handle_t scoHandle)
{
    CsrBtHfSetScoHandle(&csrBtHfInstance, CSR_BT_CONN_ID_INVALID, addr, scoHandle);
}

CsrBtResultCode HfGetBdAddrFromConnectionId(CsrBtHfConnectionId connectionId, CsrBtDeviceAddr *deviceAddr)
{
    return HfUtilGetBdAddrFromConnectionId(&csrBtHfInstance, connectionId, deviceAddr);
}

CsrUint8 HfGetLocalHfServerChannel(void)
{
    HfMainInstanceData_t *instData = &csrBtHfInstance;

    return (instData->hfServerChannel);
}
#endif /* CSR_TARGET_PRODUCT_VM */
