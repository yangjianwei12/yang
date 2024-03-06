/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/
#include "gatt_gmas_client_handover.h"

#include "gmap_client_debug.h"
#include "gmap_client_handover.h"
#include "gmap_client_private.h"


bool GmapClientHandoverVeto(ServiceHandle gmapClientProfileHandle)
{
    GMAP *gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(gmapClientProfileHandle);

    if (gmapClientInst)
    {
        if (gattGmasClientHandoverVeto(gmapClientInst->gmasSrvcHndl))
        {
            return TRUE;
        }
    }

    return FALSE;
}
