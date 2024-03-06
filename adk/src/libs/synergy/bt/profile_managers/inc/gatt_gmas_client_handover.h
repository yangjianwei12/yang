/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_GMAS_CLIENT_HANDOVER_H_
#define GATT_GMAS_CLIENT_HANDOVER_H_

#include "service_handle.h"
#include "csr_bt_profiles.h"

/***************************************************************************
NAME
    gattGmasClientHandoverVeto

DESCRIPTION
    Veto the handover of GMAS Client data

    @return TRUE if the module wishes to veto the handover attempt.
*/

CsrBool gattGmasClientHandoverVeto(ServiceHandle gmasClientHandle);

#endif
