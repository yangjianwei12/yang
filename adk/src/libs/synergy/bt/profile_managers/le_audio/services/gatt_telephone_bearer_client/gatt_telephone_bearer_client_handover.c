/******************************************************************************
 Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/
#include "csr_synergy.h"

#include "service_handle.h"
#include "gatt_telephone_bearer_client_init.h"
#include "gatt_telephone_bearer_client_handover.h"


CsrBool gattTelephoneBearerClientHandoverVeto(ServiceHandle tbsClientHandle)
{
    GTBSC *gattTbsClient = ServiceHandleGetInstanceData(tbsClientHandle);
    CsrBool veto = FALSE;

    if (!gattTbsClient || SynergySchedMessagesPendingForTask(CSR_BT_TBS_CLIENT_IFACEQUEUE, NULL) != 0)
    {
        /* Handover cannot proceed if we do not have an instance */
        veto = TRUE;
    }

    return veto;
}
