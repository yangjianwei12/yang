/******************************************************************************
 Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "gatt_telephone_bearer_client_handover.h"

#include "ccp_common.h"
#include "ccp_debug.h"
#include "ccp_handover.h"


bool CcpHandoverVeto(ServiceHandle ccpProfileHandle)
{
    CCP *ccpInst = FIND_CCP_INST_BY_PROFILE_HANDLE(ccpProfileHandle);

    if (ccpInst)
    {
        if (gattTelephoneBearerClientHandoverVeto(ccpInst->ccp_srvc_hdl) ||
            SynergySchedMessagesPendingForTask(CSR_BT_CCP_IFACEQUEUE, NULL) != 0)
        {
            CCP_DEBUG("CcpHandoverVeto");
            return TRUE;
        }
    }

    return FALSE;
}
