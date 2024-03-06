/******************************************************************************
 Copyright (c) 2009-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_bt_hfg_lib.h"
#include "csr_msg_transport.h"
#include "csr_bt_hfg_prim.h"

void CsrBtHfgMsgTransport(void* __msg)
{
    CsrMsgTransport(CSR_BT_HFG_IFACEQUEUE, CSR_BT_HFG_PRIM, __msg);
}

