/******************************************************************************
 Copyright (c) 2009-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_profiles.h"
#include "csr_msg_transport.h"
#include "csr_bt_hidd_lib.h"

void CsrBtHiddMsgTransport(CsrSchedQid phandle, void *msg)
{
    CsrMsgTransport(phandle, CSR_BT_HIDD_PRIM, msg);
}
