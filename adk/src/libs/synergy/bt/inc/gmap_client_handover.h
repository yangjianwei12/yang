/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GMAP_CLIENT_HANDOVER_H_
#define GMAP_CLIENT_HANDOVER_H_

#include "service_handle.h"

/***************************************************************************
NAME
    GmapClientHandoverVeto

DESCRIPTION
    Veto the handover of GMAP Profile data.

    @return TRUE if the module wishes to veto the handover attempt.
*/

bool GmapClientHandoverVeto(ServiceHandle gmapClientProfileHandle);

#endif
