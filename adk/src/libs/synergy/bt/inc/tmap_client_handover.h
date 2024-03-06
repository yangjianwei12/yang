/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef TMAP_CLIENT_HANDOVER_H_
#define TMAP_CLIENT_HANDOVER_H_

#include "service_handle.h"

/***************************************************************************
NAME
    TmapClientHandoverVeto

DESCRIPTION
    Veto the handover of TMAP Profile data.

    @return TRUE if the module wishes to veto the handover attempt.
*/

bool TmapClientHandoverVeto(ServiceHandle tmapClientProfileHandle);

#endif
