/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file
    \ingroup    dfu
    \brief      DFU peer handover interfaces

*/
#ifdef INCLUDE_DFU_PEER
#include "handover_if.h"
#include "dfu_peer.h"
#include "dfu.h"

#include <logging.h>

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

static bool dfuPeer_HandoverVeto(void)
{
    return FALSE;
}

static bool dfuPeer_HandoverMarshal(const tp_bdaddr *tp_bd_addr,
                                    uint8 *buf,
                                    uint16 length,
                                    uint16 *written)
{
    UNUSED(tp_bd_addr);
    UNUSED(buf);
    UNUSED(length);
    UNUSED(written);

    DEBUG_LOG_INFO("dfuPeer_HandoverMarshal");

    return TRUE;
}

static bool dfuPeer_HandoverUnmarshal(const tp_bdaddr *tp_bd_addr,
                                      const uint8 *buf,
                                      uint16 length,
                                      uint16 *consumed)
{
    UNUSED(tp_bd_addr);
    UNUSED(buf);
    UNUSED(length);
    UNUSED(consumed);

    DEBUG_LOG_INFO("dfuPeer_HandoverUnmarshal");

    return TRUE;
}

static void dfuPeer_HandoverCommit(const tp_bdaddr *tp_bd_addr, bool is_primary)
{
    UNUSED(tp_bd_addr);

    DEBUG_LOG_INFO("dfuPeer_HandoverCommit is_primary %d", is_primary);
}

/*!
    \brief Component commits to the specified role

    The component should take any actions necessary to commit to the
    new role.

    \param[in] is_primary   TRUE if device role is primary, else secondary

*/
static void dfuPeer_HandoverComplete(bool is_primary)
{
    DEBUG_LOG_INFO("dfuPeer_HandoverComplete is_primary %d", is_primary);
    /* Disconeect DFU peer and upgrade transport to get ready for DFU resume
     * post handover */
    if(Dfu_IsUpgradeInProgress())
    {
        if(is_primary)
        {
            /* Called on the new primary to disconnect upgrade from the peer
             * L2CAP transport
             */
            UpgradeTransportDisconnectRequest();
        }
        else
        {
            DfuPeer_CancelPreviousDFUActivities();
        }

        /* Remove stale ctx information of old DFU if any. */
        UpgradePeerDeInit();

    }
}

static void dfuPeer_HandoverAbort(void)
{
    DEBUG_LOG_INFO("dfuPeer_HandoverAbort");
    return;
}

const handover_interface dfu_peer_handover_if =
        MAKE_HANDOVER_IF(&dfuPeer_HandoverVeto,
                         &dfuPeer_HandoverMarshal,
                         &dfuPeer_HandoverUnmarshal,
                         &dfuPeer_HandoverCommit,
                         &dfuPeer_HandoverComplete,
                         &dfuPeer_HandoverAbort);

#endif /* INCLUDE_DFU_PEER */
