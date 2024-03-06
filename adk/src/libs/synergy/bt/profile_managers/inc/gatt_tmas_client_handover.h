/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_TMAS_CLIENT_HANDOVER_H_
#define GATT_TMAS_CLIENT_HANDOVER_H_

#include "service_handle.h"
#include "csr_bt_profiles.h"

/***************************************************************************
NAME
    gattTmasClientHandoverVeto

DESCRIPTION
    Veto the handover of TMAS Client data

    @return TRUE if the module wishes to veto the handover attempt.
*/

CsrBool gattTmasClientHandoverVeto(ServiceHandle tmasClientHandle);

#endif
