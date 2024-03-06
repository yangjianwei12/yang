/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/
#include "csr_synergy.h"

#include "service_handle.h"
#include "gatt_mcs_client_init.h"
#include "gatt_mcs_client_handover.h"


CsrBool gattMcsClientHandoverVeto(ServiceHandle mcsClientHandle)
{
    GMCSC *gattMcsClient = ServiceHandleGetInstanceData(mcsClientHandle);
    CsrBool veto = FALSE;

    if (gattMcsClient)
    {
        if (gattMcsClient->writeCccCount)
        { /* CCCD write for characteristics is in process */
            veto = TRUE;
        }
    }

    return veto;
}
