/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup micp_server
    \brief   Implementations for the GATT MICP Server Handover interfaces.
*/

#if defined(ENABLE_LE_HANDOVER) && defined(INCLUDE_LE_AUDIO_UNICAST)

#include "gatt_mics_server_handover.h"
#include "micp_server_private.h"
#include <gatt_connect.h>
#include "handover_if.h"
#include "micp_server.h"
#include <panic.h>
#include <logging.h>
#include <bdaddr.h>

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*! Check whether to veto the handover */
static bool micpServer_Veto(void)
{
    /* Check message queue status */
    if (MessagesPendingForTask(MicpServer_GetTask(), NULL) != 0)
    {
        return TRUE;
    }

    return gattMicsServerHandoverVeto(MicpServer_GetMicsHandle());
}

/*! Marshal MICP Server data */
static bool micpServer_Marshal(const tp_bdaddr *tp_bd_addr,
                               uint8 *buf,
                               uint16 length,
                               uint16 *written)
{
    gatt_cid_t  cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);
    return gattMicsServerHandoverMarshal(MicpServer_GetMicsHandle(), cid, buf, length, written);
}

/*! Unmarshal MICP Server data during handover */
static bool micpServer_Unmarshal(const tp_bdaddr *tp_bd_addr,
                                 const uint8 *buf,
                                 uint16 length,
                                 uint16 *written)
{
    gatt_cid_t  cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);
    return gattMicsServerHandoverUnmarshal(MicpServer_GetMicsHandle(), cid, buf, length, written);
}

/*! Commit MICP Server data during handover.*/
static void micpServer_HandoverCommit(const tp_bdaddr *tp_bd_addr, bool is_primary)
{
    gatt_cid_t  cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);
    gattMicsServerHandoverCommit(MicpServer_GetMicsHandle(), cid, is_primary);
}

/*! Handle handover complete for MICP Server */
static void micpServer_HandoverComplete(bool is_primary)
{
    gattMicsServerHandoverComplete(MicpServer_GetMicsHandle(), is_primary );
}

/*! Procedure to handle handover abort for MICP Server */
static void micpServer_HandoverAbort(void)
{
    gattMicsServerHandoverAbort(MicpServer_GetMicsHandle());
}

/*! Handover interfaces for MICP Server */
const handover_interface micp_server_handover_if =
        MAKE_BLE_HANDOVER_IF(&micpServer_Veto,
                             &micpServer_Marshal,
                             &micpServer_Unmarshal,
                             &micpServer_HandoverCommit,
                             &micpServer_HandoverComplete,
                             &micpServer_HandoverAbort);

#endif /* defined(ENABLE_LE_HANDOVER) || defined(INCLUDE_LE_AUDIO_UNICAST)) */

