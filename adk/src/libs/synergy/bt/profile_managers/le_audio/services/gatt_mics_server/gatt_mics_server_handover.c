/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include <stdlib.h>

#include "csr_bt_core_stack_pmalloc.h"
#include "gatt_mics_server_handover.h"
#include "gatt_mics_server_private.h"
#include "gatt_mics_server_common.h"
#include "gatt_mics_server_handover_mgr.h"

bool gattMicsServerHandoverMarshal(ServiceHandle serviceHandle,
                                  connection_id_t cid,
                                  uint8 *buf,
                                  uint16 length,
                                  uint16 *written)
{
    GMICS_T *micControlServer = (GMICS_T*)ServiceHandleGetInstanceData(serviceHandle);
    uint8 index = 0;
    bool res = FALSE;

    if (!micControlServer)
        return res;

    index = micsServerGetCidIndex(micControlServer, cid);

    if(index == GATT_MICS_SERVER_INVALID_CID_INDEX)
    {
        GATT_MICS_SERVER_ERROR("gattMicsServerHandoverMarshal called with not valid cid = %d\n", cid);
        /* Connection not found, nothing to marshal */
        *written = 0;
        res = TRUE;
    }
    else
    {

        if (!micControlServer->micsHandoverMgr)
            micControlServer->micsHandoverMgr = zpnew(MicsHandoverMgr); /* counters, bools etc. are initialised to zero */

        res = micsServerHandoverMgrMarshal(micControlServer->micsHandoverMgr,
                                          micControlServer->data,
                                          index,
                                          micControlServer->handoverStep,
                                          buf,
                                          length,
                                          written);

        if(res)
        {
            micControlServer->handoverStep++;
        }
    }

    return res;
}

bool gattMicsServerHandoverUnmarshal(ServiceHandle serviceHandle,
                                    connection_id_t cid,
                                    const uint8 *buf,
                                    uint16 length,
                                    uint16 *consumed)
{

    GMICS_T *micControlServer = (GMICS_T*)ServiceHandleGetInstanceData(serviceHandle);
    bool res = FALSE;

    if (!micControlServer)
    {
        return res;
    }

    if (!micControlServer->micsHandoverMgr)
    {
        micControlServer->micsHandoverMgr = zpnew(MicsHandoverMgr); /* counters, bools etc. are initialised to zero */
    }

    res = micsServerHandoverMgrUnmarshal(micControlServer->micsHandoverMgr,
                                        micControlServer->handoverStep,
                                        cid,
                                        buf,
                                        length,
                                        consumed);

    if(res)
    {
        micControlServer->handoverStep++;
    }

    return res;
}

void gattMicsServerHandoverCommit(ServiceHandle serviceHandle,
                                 connection_id_t cid,
                                 const bool newRole)
{
    (void) newRole;

    GMICS_T *micControlServer = (GMICS_T*)ServiceHandleGetInstanceData(serviceHandle);

    if (!micControlServer)
    {
        return ;
    }

    micsServerHandoverMgrCommit(micControlServer, cid);
}

static void gattMicsServerHandoverMgrCleanup(ServiceHandle serviceHandle)
{
    GMICS_T *micControlServer = (GMICS_T*)ServiceHandleGetInstanceData(serviceHandle);

    if (!micControlServer)
    {
        return ;
    }

    /*
     * At the moment the is no difference between handling a hand over 'abort' or a hand over 'complete':
     * they both ensure any 'leftover' hand over connections are freed.
     */
    micsServerHandoverMgrComplete(micControlServer->micsHandoverMgr);
    free(micControlServer->micsHandoverMgr);
    micControlServer->micsHandoverMgr = NULL;

    micControlServer->handoverStep = 0;
}

void gattMicsServerHandoverAbort(ServiceHandle serviceHandle)
{
    gattMicsServerHandoverMgrCleanup(serviceHandle);
}

void gattMicsServerHandoverComplete(ServiceHandle serviceHandle, const bool newRole)
{
    (void) newRole;
    gattMicsServerHandoverMgrCleanup(serviceHandle);
}

bool gattMicsServerHandoverVeto(ServiceHandle serviceHandle)
{
    GMICS_T *micControlServer = (GMICS_T*)ServiceHandleGetInstanceData(serviceHandle);

    if (!micControlServer|| SynergySchedMessagesPendingForTask(CSR_BT_MICS_SERVER_IFACEQUEUE, NULL) != 0)
    {
        /* Handover cannot proceed if we do not have a MICS instance */
        return TRUE;
    }

    return FALSE;
}
