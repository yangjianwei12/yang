/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/
#include "csr_synergy.h"

#include "service_handle.h"
#include "gatt_tmas_client_init.h"
#include "gatt_tmas_client_handover.h"


CsrBool gattTmasClientHandoverVeto(ServiceHandle tmasClientHandle)
{
    CSR_UNUSED(tmasClientHandle);
    return FALSE;
}
