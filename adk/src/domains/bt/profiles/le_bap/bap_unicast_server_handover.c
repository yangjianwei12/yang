/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_bap
    \brief   Implementations for the LE BAP Handover interfaces.
*/

#if defined(ENABLE_LE_HANDOVER) && (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST))

#include "bap_server_handover.h"
#include "handover_if.h"

#include <panic.h>
#include <logging.h>
#include <bdaddr.h>

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*! Check whether to veto the handover */
static bool bapServer_Veto(void)
{
    return BapServerHandoverVeto();
}

/*! Marshal BAP Profile data */
static bool bapServer_Marshal(const tp_bdaddr *tp_bd_addr, uint8 *buf, uint16 length, uint16 *written)
{
    return BapServerHandoverMarshal(tp_bd_addr, buf, length, written);
}

/*! Unmarshal BAP Profile data during handover */
static bool bapServer_Unmarshal(const tp_bdaddr *tp_bd_addr, const uint8 *buf, uint16 length, uint16 *written)
{
    return BapServerHandoverUnmarshal(tp_bd_addr, buf, length, written);
}

/*! Commit BAP Profile data during handover.*/
static void bapServer_HandoverCommit(const tp_bdaddr *tp_bd_addr, bool is_primary)
{
    BapServerHandoverCommit(tp_bd_addr, is_primary);
}

/*! Handle handover complete for BAP */
static void bapServer_HandoverComplete(bool is_primary)
{
    BapServerHandoverComplete(is_primary);
}

/*! Procedure to handle handover abort for BAP */
static void bapServer_HandoverAbort(void)
{
    BapServerHandoverAbort();
}

/*! Handover interfaces for BAP */
const handover_interface bap_handover_if =
        MAKE_BLE_HANDOVER_IF(&bapServer_Veto,
                             &bapServer_Marshal,
                             &bapServer_Unmarshal,
                             &bapServer_HandoverCommit,
                             &bapServer_HandoverComplete,
                             &bapServer_HandoverAbort);

#endif /* defined(ENABLE_LE_HANDOVER) && (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)) */

