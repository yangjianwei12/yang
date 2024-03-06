/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/
#include "csr_synergy.h"

#include "service_handle.h"
#include "gatt_gmas_client_init.h"
#include "gatt_gmas_client_handover.h"


CsrBool gattGmasClientHandoverVeto(ServiceHandle gmasClientHandle)
{
    CSR_UNUSED(gmasClientHandle);
    return FALSE;
}
