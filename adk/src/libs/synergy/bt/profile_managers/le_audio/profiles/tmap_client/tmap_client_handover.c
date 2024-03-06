/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/
#include "gatt_tmas_client_handover.h"

#include "tmap_client_debug.h"
#include "tmap_client_handover.h"
#include "tmap_client_private.h"


bool TmapClientHandoverVeto(ServiceHandle tmapClientProfileHandle)
{
    TMAP *tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(tmapClientProfileHandle);

    if (tmapClientInst)
    {
        if (gattTmasClientHandoverVeto(tmapClientInst->tmasSrvcHndl))
        {
            return TRUE;
        }
    }

    return FALSE;
}
