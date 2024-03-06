/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_TBS_CLIENT_HANDOVER_H_
#define GATT_TBS_CLIENT_HANDOVER_H_

#include "service_handle.h"
#include "csr_bt_profiles.h"

/***************************************************************************
NAME
    gattTelephoneBearerClientHandoverVeto

DESCRIPTION
    Veto the handover of TBS Client data

    @return TRUE if the module wishes to veto the handover attempt.
*/

CsrBool gattTelephoneBearerClientHandoverVeto(ServiceHandle tbsClientHandle);

#endif
