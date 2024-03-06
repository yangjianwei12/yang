/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include <stdlib.h>

#include "csr_bt_core_stack_pmalloc.h"
#include "gatt_bass_server_handover.h"
#include "gatt_bass_server_private.h"
#include "gatt_bass_server_handover_mgr.h"
#include "gatt_bass_server_common.h"
#include "gatt_bass_server_debug.h"

bool gattBassServerHandoverMarshal(ServiceHandle serviceHandle,
                                   connection_id_t cid,
                                   uint8 *buf,
                                   uint16 length,
                                   uint16 *written)
{
    GBASSSS *bassServer = (GBASSSS*)ServiceHandleGetInstanceData(serviceHandle);
    uint8 index = 0;
    bool res = FALSE;

    if (!bassServer)
        return FALSE;

    if(!bassServerFindCid(bassServer, cid, &index))
    {
        GATT_BASS_SERVER_ERROR("gattBassServerHandoverMarshal called with not valid cid = %d\n", cid);
        /* Connection not found, nothing to marshal */
        *written = 0;
        return TRUE;
    }

    if (!bassServer->bassHandoverMgr)
        bassServer->bassHandoverMgr = zpnew(BassHandoverMgr); /* counters, bools etc. are initialised to zero */

    res = bassServerHandoverMgrMarshal(bassServer->bassHandoverMgr,
                                       bassServer->data,
                                       index,
                                       bassServer->handoverStep,
                                       buf,
                                       length,
                                       written);

    if(res)
    {
        bassServer->handoverStep++;
    }

    return res;
}

bool gattBassServerHandoverUnmarshal(ServiceHandle serviceHandle,
                                     connection_id_t cid,
                                     const uint8 *buf,
                                     uint16 length,
                                     uint16 *consumed)
{
    GBASSSS *bassServer = (GBASSSS*)ServiceHandleGetInstanceData(serviceHandle);
    bool res = FALSE;

    if (!bassServer)
    {
        return FALSE;
    }

    if (!bassServer->bassHandoverMgr)
    {
        bassServer->bassHandoverMgr = zpnew(BassHandoverMgr); /* counters, bools etc. are initialised to zero */
    }

    res = bassServerHandoverMgrUnmarshal(bassServer->bassHandoverMgr,
                                         bassServer->handoverStep,
                                         cid,
                                         buf,
                                         length,
                                         consumed);

    if(res)
    {
        bassServer->handoverStep++;
    }

    return res;
}

void gattBassServerHandoverCommit(ServiceHandle serviceHandle, connection_id_t cid, const bool newRole)
{
    (void) newRole;

    GBASSSS *bassServer = (GBASSSS*)ServiceHandleGetInstanceData(serviceHandle);

    if (!bassServer)
    {
        return ;
    }

    if (bassServer->bassHandoverMgr != NULL)
    {
        bassServerHandoverMgrCommit(bassServer, cid);
    }
    else
    {
        GATT_BASS_SERVER_ERROR("Called gattBassServerHandoverCommit but no bassHandoverMgr allocated - has (un)marshal been called and no previous call to Complete() or Abort()?\n");
    }
}

static void gattBassServerHandoverMgrCleanup(ServiceHandle serviceHandle)
{
    GBASSSS *bassServer = (GBASSSS*)ServiceHandleGetInstanceData(serviceHandle);

    if (!bassServer)
    {
        return ;
    }

    /*
     * At the moment the is no difference between handling a hand over 'abort' or a hand over 'complete':
     * they both ensure any 'leftover' hand over connections are freed.
     */
    bassServerHandoverMgrComplete(bassServer->bassHandoverMgr);
    free(bassServer->bassHandoverMgr);
    bassServer->bassHandoverMgr = NULL;

    bassServer->handoverStep = 0;
    CsrMemSet(bassServer->isToBeNotified, FALSE, BASS_SERVER_BROADCAST_RECEIVE_STATE_NUM);
}

void gattBassServerHandoverAbort(ServiceHandle serviceHandle)
{
    gattBassServerHandoverMgrCleanup(serviceHandle);
}

void gattBassServerHandoverComplete(ServiceHandle serviceHandle, const bool newRole )
{
    (void) newRole;
    gattBassServerHandoverMgrCleanup(serviceHandle);
}

bool gattBassServerHandoverVeto(ServiceHandle serviceHandle)
{
    GBASSSS *bassServer = (GBASSSS*)ServiceHandleGetInstanceData(serviceHandle);

    if (!bassServer)
    {
        /* Handover cannot proceed if we do not have a BASS instance */
        return TRUE;
    }

    return FALSE;
}

bool gattBassServerHasValidConnection(ServiceHandle serviceHandle, connection_id_t cid)
{
    GBASSSS *bassServer = (GBASSSS*)ServiceHandleGetInstanceData(serviceHandle);
    uint8 index = 0;

    if (bassServer && bassServerFindCid(bassServer, cid, &index))
        return TRUE;

    return FALSE;
}
