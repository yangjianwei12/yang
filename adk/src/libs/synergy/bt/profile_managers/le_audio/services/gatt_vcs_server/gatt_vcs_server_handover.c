/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#include <stdlib.h>

#include "csr_bt_core_stack_pmalloc.h"
#include "gatt_vcs_server_handover.h"
#include "gatt_vcs_server_private.h"
#include "gatt_vcs_server_common.h"
#include "gatt_vcs_server_handover_mgr.h"

bool gattVcsServerHandoverMarshal(ServiceHandle serviceHandle,
                                  connection_id_t cid,
                                  uint8 *buf,
                                  uint16 length,
                                  uint16 *written)
{
    GVCS *volumeControlServer = (GVCS*)ServiceHandleGetInstanceData(serviceHandle);
    uint8 index = 0;
    bool res = FALSE;

    if (!volumeControlServer)
        return res;

    index = vcsServerGetCidIndex(volumeControlServer, cid);

    if(index == GATT_VCS_SERVER_INVALID_CID_INDEX)
    {
        GATT_VCS_SERVER_ERROR("gattVcsServerHandoverMarshal called with not valid cid = %d\n", cid);
        /* Connection not found, nothing to marshal */
        *written = 0;
        res = TRUE;
    }
    else
    {

        if (!volumeControlServer->vcsHandoverMgr)
            volumeControlServer->vcsHandoverMgr = zpnew(VcsHandoverMgr); /* counters, bools etc. are initialised to zero */

        res = vcsServerHandoverMgrMarshal(volumeControlServer->vcsHandoverMgr,
                                          volumeControlServer->data,
                                          index,
                                          volumeControlServer->handoverStep,
                                          buf,
                                          length,
                                          written);

        if(res)
        {
            volumeControlServer->handoverStep++;
        }
    }

    return res;
}

bool gattVcsServerHandoverUnmarshal(ServiceHandle serviceHandle,
                                    connection_id_t cid,
                                    const uint8 *buf,
                                    uint16 length,
                                    uint16 *consumed)
{

    GVCS *volumeControlServer = (GVCS*)ServiceHandleGetInstanceData(serviceHandle);
    bool res = FALSE;

    if (!volumeControlServer)
    {
        return res;
    }

    if (!volumeControlServer->vcsHandoverMgr)
    {
        volumeControlServer->vcsHandoverMgr = zpnew(VcsHandoverMgr); /* counters, bools etc. are initialised to zero */
    }

    res = vcsServerHandoverMgrUnmarshal(volumeControlServer->vcsHandoverMgr,
                                        volumeControlServer->handoverStep,
                                        cid,
                                        buf,
                                        length,
                                        consumed);

    if(res)
    {
        volumeControlServer->handoverStep++;
    }

    return res;
}

void gattVcsServerHandoverCommit(ServiceHandle serviceHandle,
                                 connection_id_t cid,
                                 const bool newRole)
{
    (void) newRole;

    GVCS *volumeControlServer = (GVCS*)ServiceHandleGetInstanceData(serviceHandle);

    if (!volumeControlServer)
    {
        return ;
    }

    vcsServerHandoverMgrCommit(volumeControlServer, cid);
}

static void gattVcsServerHandoverMgrCleanup(ServiceHandle serviceHandle)
{
    GVCS *volumeControlServer = (GVCS*)ServiceHandleGetInstanceData(serviceHandle);

    if (!volumeControlServer)
    {
        return ;
    }

    /*
     * At the moment the is no difference between handling a hand over 'abort' or a hand over 'complete':
     * they both ensure any 'leftover' hand over connections are freed.
     */
    vcsServerHandoverMgrComplete(volumeControlServer->vcsHandoverMgr);
    free(volumeControlServer->vcsHandoverMgr);
    volumeControlServer->vcsHandoverMgr = NULL;

    volumeControlServer->handoverStep = 0;
}

void gattVcsServerHandoverAbort(ServiceHandle serviceHandle)
{
    gattVcsServerHandoverMgrCleanup(serviceHandle);
}

void gattVcsServerHandoverComplete(ServiceHandle serviceHandle, const bool newRole)
{
    (void) newRole;
    gattVcsServerHandoverMgrCleanup(serviceHandle);
}

bool gattVcsServerHandoverVeto(ServiceHandle serviceHandle)
{
    GVCS *volumeControlServer = (GVCS*)ServiceHandleGetInstanceData(serviceHandle);

    if (!volumeControlServer)
    {
        /* Handover cannot proceed if we do not have a VCS instance */
        return TRUE;
    }

    return FALSE;
}
