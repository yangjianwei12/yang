/******************************************************************************
 Copyright (c) 2009-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_MAPC_MODULE

#include "csr_bt_profiles.h"
#include "csr_msg_transport.h"
#include "csr_bt_mapc_lib.h"

void CsrBtMapcMsgTransport(CsrSchedQid phandle, void *msg)
{
    CsrMsgTransport(phandle, CSR_BT_MAPC_PRIM, msg);
}

#endif /* EXCLUDE_CSR_BT_MAPC_MODULE */
