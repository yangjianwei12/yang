/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   gatt_service_discovery_synergy   Synergy
    @{
    \ingroup    gatt_service_discovery
    \brief      Private types and defines for GATT Service discovery module. Synergy.
*/
#ifndef GATT_SERVICE_DISCOVERY_CONTEXT_H
#define GATT_SERVICE_DISCOVERY_CONTEXT_H

#include <task_list.h>
#include "gatt_service_discovery.h"

/*! \brief gatt service discovery state information of legacy clients */
typedef struct
{
    client_id_t current_client_id;
    uint16 current_service_index;
}gatt_service_discovery_legacy_state_t;

/*! \brief Gatt Service Discovery context information. */
typedef struct
{
    /*! GATT Service discovery task */
    TaskData task_data;

    /*! List of client tasks registered for notifications */
    task_list_t *client_tasks;

    /*! Connection ID of the device which is being discovered */
    gatt_cid_t cid_in_discovery;

    /*! Connection ID of the device pending for service discovery (if any) */
    gatt_cid_t cid_pending_to_discover;

    /*! GATT services registered for discovery */
    GattSdSrvcId services_to_discover;

    /*! GATT services registered for discovery and handles */
    GattSdSrvcId services_with_handle_to_discover;

    /* Service discovery status */
    service_discovery_status gsd_status;

    /*! Legacy gatt clients discovery state */
    gatt_service_discovery_legacy_state_t legacy_services_discovery_state;
} gatt_service_discovery_task_data_t;

/*! \brief Initialise the Legacy GATT Dervice Discovery clients
    \param[in] init_task - unused
    \return bool TRUE
 */
bool gattServiceDiscovery_InitLegacyClients(void);

#endif // GATT_SERVICE_DISCOVERY_CONTEXT_H

/*! @} */