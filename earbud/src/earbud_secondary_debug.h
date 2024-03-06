/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file       earbud_secondary_debug.h
\brief      Earbud Application interface for testing le_debug_secondary module
*/

#ifndef EARBUD_SECONDARY_DEBUG_H
#define EARBUD_SECONDARY_DEBUG_H

#ifdef ENABLE_LE_DEBUG_SECONDARY
#include "earbud_sm.h"

#include <tws_topology.h>
#include <le_debug_secondary.h>

/*! \brief Enable Debug Mode

    This function shall be invoked on both the earbuds, so that it could start debug advertisements
    and accept BLE connection from Debug central device automatically on role update (to Secondary)
    On invocation of this function Following actions are taken based on #tws_topology_role.
    - tws_topology_role_none (PFR not started yet): just remembers that the debug mode is enabled.
    - tws_topology_role_primary :just remembers that the debug mode is enabled.
    - tws_topology_role_secondary (And if Peer is connected): Update the local IRK with modified 
      root keys and start advertisement on confirmation (#LE_DEBUG_SECONDARY_UPDATE_IRK_CFM)

    \note This functionality shall be used only for development purpose and shall NOT be invoked on an end product.
*/
void EarbudSecondaryDebug_Enable(void);

/*! \brief Disable Debug Mode
    This function shall be invoked on both the earbuds to disable the debug mode if it was enabled already

    \note This functionality shall be used only for development purpose and shall NOT be invoked on an end product.
*/
void EarbudSecondaryDebug_Disable(void);

/*! \brief Handle Update IRK Confirm
    \note This functionality shall be used only for development purpose and shall NOT be invoked on an end product.
*/
void EarbudSecondaryDebug_HandleUpdateIrkCfm(LE_DEBUG_SECONDARY_UPDATE_IRK_CFM_T *message);

/*! \brief Handle #TWS_TOPOLOGY_ROLE_CHANGED_IND
    This function handles TWS Topology Role transition and shall take respective actions like updating local IRK,
    Start/Stop Debug advertisement and Handle debug connection on secondary earbud.

    | old_role                    | new_role                   | Action                                               |
    | :----                       | :----                      | :----                                                |
    | tws_topology_role_primary   | tws_topology_role_none     | NA                                                   |
    | tws_topology_role_secondary | tws_topology_role_none     | Disconnect Link, Stop adverts,\n Restore primary IRK |
    | tws_topology_role_none      | tws_topology_role_none     | NA                                                   |
    | tws_topology_role_secondary | tws_topology_role_primary  | Disconnect Link, Stop adverts,\n Restore primary IRK |
    | tws_topology_role_primary   | tws_topology_role_primary  | NA                                                   |
    | tws_topology_role_none      | tws_topology_role_primary  | NA                                                   |
    | tws_topology_role_none      | tws_topology_role_secondary| Update local IRK, Start debug adverts                |
    | tws_topology_role_primary   | tws_topology_role_secondary| Update local IRK, Start debug adverts                |
    | tws_topology_role_secondary | tws_topology_role_secondary| Update local IRK, Start debug adverts                |

    \param new_role #tws_topology_role to which TWS topology role has transitioned to!
    \param old_role #tws_topology_role from which the TWS topology has transitioned to current role.
    \note This functionality shall be used only for development purpose and shall NOT be invoked on an end product.
*/
void EarbudSecondaryDebug_HandleTwsTopologyRoleChange(tws_topology_role new_role, tws_topology_role old_role);

/*! \brief Disconnect debug device and switch local IRK to primary while in debug mode
    This function shall be invoked on secondary earbud, to perform
    - Disconnect debug BLE link if connected.
    - Restore the local IRK to default primary IRK (if modified)
    - Stop debug advertisements on secondary earbud.

    \note This functionality shall be used only for development purpose and shall NOT be invoked on an end product.
*/
void EarbudSecondaryDebug_DisconnectOrStopAdvert(void);


#endif /* ENABLE_LE_DEBUG_SECONDARY */
#endif /* EARBUD_SECONDARY_DEBUG_H */
