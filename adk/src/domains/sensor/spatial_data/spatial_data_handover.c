/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       spatial_data_handover.c
    \ingroup    spatial_data
    \brief      Provides handover support during spatial data
*/

#ifdef INCLUDE_SPATIAL_DATA

/* Handover only needed for earbuds - when the spatial data (control and orientation) is exchanged between earbuds. */
#ifdef INCLUDE_SENSOR_PROFILE

#define DEBUG_LOG_MODULE_NAME spatial_data
#include "spatial_data_private.h"
#include "app_handover_if.h"
#include "state_proxy.h"

#ifdef INCLUDE_HIDD_PROFILE
#include "hidd_profile.h"
#endif

/* Handover functions */
static bool spatialData_Veto(void);
static void spatialData_Commit(bool is_primary);

REGISTER_HANDOVER_INTERFACE_NO_MARSHALLING(SPATIAL_DATA, spatialData_Veto, spatialData_Commit);

/*!
    \brief Handle Veto check during spatial data
    \return TRUE to veto spatial data.
*/
static bool spatialData_Veto(void)
{
    DEBUG_LOG_DEBUG("%s",__func__);

    if (spatial_data.sensor_profile_connected &&
        spatial_data.pending_cfm_from_secondary)
    {
        /* TODO: Veto if the primary is waiting for a spatial_data related cfm from secondary.*/
        DEBUG_LOG_WARN("%s:Handover initiated when waiting for spatial data enable/disable cfm from secondary", __func__);
    }

    /* Veto handover if we are sending reports AND both earbuds are out of case */
    if ((spatial_data.enabled_status != spatial_data_disabled) &&
        StateProxy_IsOutOfCase() && StateProxy_IsPeerOutOfCase())
    {
        DEBUG_LOG_DEBUG(
            "%s: Handover Vetoed.  IsInEar %d, IsPeerInEar %d",
            __func__, StateProxy_IsInEar(), StateProxy_IsPeerInEar());
        return TRUE;
    }

    return FALSE;
}

/*!
    \brief Component commits to the specified role

    The component should take any actions necessary to commit to the
    new role.

    \param[in] is_primary   TRUE if new role is primary, else secondary

*/
static void spatialData_Commit(bool is_primary)
{
    DEBUG_LOG_DEBUG("%s",__func__);

#ifdef INCLUDE_HIDD_PROFILE
    if (is_primary)
    {
        /* Update the HID connection status after a handover in primary. */

        /* TODO: this needs to be modified when adding multi-point support in HIDD. */
        /* Update the sink/wallclock handler for the HID connnection. */
        if (HiddProfile_IsConnected())
        {
            uint32 conn_id = HiddProfile_GetConnectionId();

            if (conn_id)
            {
                /* Create a wall-clock time conversion handle for the L2CAP Sink for the HID connection. */
                spatial_data.hidd_wallclock_enabled = RtimeWallClockEnable(&spatial_data.hidd_wallclock_state, 
                                                                                 StreamL2capSink(conn_id));
            }
            else
            {
                /* Invalid connection ID? It shouldn't have happened as the HID is connected!! */
                DEBUG_LOG_WARN("%s:Invalid HID connecton_id:%d sink handle:%d", __func__,
                                                       conn_id,
                                                       StreamL2capSink(conn_id));
            }
        }
        else
        {
            if (spatial_data.enabled_status == spatial_data_enabled_remote)
            {
                DEBUG_LOG_WARN("%s:HID not connected in the new primary after handover when spatial data is enabled for remote processing!");
            }
        }
    }
#else
    UNUSED(is_primary);
#endif /* INCLUDE_HIDD_PROFILE */

#if defined(INCLUDE_ATTITUDE_FILTER) && defined(INCLUDE_SENSOR_PROFILE)
    /* No orientation value rxed just after a handover. */
    spatial_data.orientation_rxed = FALSE;
#endif /* INCLUDE_ATTITUDE_FILTER && INCLUDE_SENSOR_PROFILE */

}

#endif /* INCLUDE_SENSOR_PROFILE */

#endif /* INCLUDE_SPATIAL_DATA */
