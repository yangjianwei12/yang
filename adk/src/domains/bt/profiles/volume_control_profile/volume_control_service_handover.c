/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    volume_profile
    \brief      Implementations for the Gatt vcs Handover interfaces.
*/

#if defined(ENABLE_LE_HANDOVER) && (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST))

#include "gatt_vcs_server_handover.h"
#include <gatt_connect.h>
#include "volume_renderer_role.h"
#include "handover_if.h"

#include <panic.h>
#include <logging.h>
#include <bdaddr.h>

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*! Check whether to veto the handover */
static bool VolumeControlServer_Veto(void)
{
    return gattVcsServerHandoverVeto(VolumeRenderer_GetVcsHandle());
}

/*! Marshal VCS Profile data */
static bool VolumeControlServer_Marshal(const tp_bdaddr *tp_bd_addr,
                                  uint8 *buf,
                                  uint16 length,
                                  uint16 *written)
{
    gatt_cid_t  cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);
    return gattVcsServerHandoverMarshal(VolumeRenderer_GetVcsHandle(), cid, buf, length, written);
}

/*! Unmarshal VCS Profile data during handover */
static bool VolumeControlServer_Unmarshal(const tp_bdaddr *tp_bd_addr,
                                    const uint8 *buf,
                                    uint16 length,
                                    uint16 *written)
{
    gatt_cid_t  cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);
    return gattVcsServerHandoverUnmarshal(VolumeRenderer_GetVcsHandle(), cid, buf, length, written);
}

/*! Commit VCS Profile data during handover.*/
static void VolumeControlServer_HandoverCommit(const tp_bdaddr *tp_bd_addr, bool is_primary)
{
    gatt_cid_t  cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);
    gattVcsServerHandoverCommit(VolumeRenderer_GetVcsHandle(), cid, is_primary);
}

/*! Handle handover complete for VCS */
static void VolumeControlServer_HandoverComplete(bool is_primary)
{
    gattVcsServerHandoverComplete(VolumeRenderer_GetVcsHandle(), is_primary );
}

/*! Procedure to handle handover abort for VCS */
static void VolumeControlServer_HandoverAbort(void)
{
    gattVcsServerHandoverAbort(VolumeRenderer_GetVcsHandle());
}

/*! Handover interfaces for VCS */
const handover_interface vcs_service_handover_if =
        MAKE_BLE_HANDOVER_IF(&VolumeControlServer_Veto,
                             &VolumeControlServer_Marshal,
                             &VolumeControlServer_Unmarshal,
                             &VolumeControlServer_HandoverCommit,
                             &VolumeControlServer_HandoverComplete,
                             &VolumeControlServer_HandoverAbort);

#endif /* defined(ENABLE_LE_HANDOVER) && (defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)) */


