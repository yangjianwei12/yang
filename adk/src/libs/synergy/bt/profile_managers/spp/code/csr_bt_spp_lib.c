/******************************************************************************
 Copyright (c) 2009-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_SPP_MODULE
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_bt_spp_lib.h"
#include "csr_msg_transport.h"

void CsrBtSppMsgTransport(CsrSchedQid appHandle, void* msg)
{
    CsrMsgTransport(appHandle, CSR_BT_SPP_PRIM, msg);
}
#endif /* !EXCLUDE_CSR_BT_SPP_MODULE */