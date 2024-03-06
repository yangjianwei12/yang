/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
    All Rights Reserved.
    Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   gatt_service_discovery GATT Service Discovery
    @{
    \ingroup    gatt_common_domain
    \brief      Header file for Gatt Service Discovery
*/

#ifndef GATT_SERVICE_DISCOVERY_H
#define GATT_SERVICE_DISCOVERY_H

#include "gatt_client.h"
#ifdef USE_SYNERGY
#include "domain_message.h"
#include "bt_types.h"
#include "gatt_service_discovery_lib.h"
#endif /* USE_SYNERGY */

/*! Enumerated type for service discovery status */
typedef enum
{
    gsd_uninitialised,   /*! Uninitialised state */
    gsd_in_idle,         /*! Idle State - Ready to start dicovering */
    gsd_in_progress,     /*! Service discovery in progress */
    gsd_complete_failure /*! Service discovery failed */
}service_discovery_status;

/*! \brief Initialise the GATT Dervice Discovery component
    \param[in] init_task - unused
    \return bool TRUE
 */
bool GattServiceDiscovery_Init(Task init_task);

/*! Function to get the GATT Service discovery component status.

    \param none.

    \return Gatt service discovery component's present status.
*/
service_discovery_status GattServiceDiscovery_GetStatus(void);

#ifndef USE_SYNERGY
/*! Function to initiate the GATT service procedure.

    \param connection id in which the discovery procedure has to be started.

    \return TRUE if service discovery started successfully, FALSE otherwise.
*/
bool GattServiceDiscovery_StartDiscovery(gatt_cid_t cid);

/*! Function to remove the clients.

    \param cid Connection in which the Service discovery should start.

    \return TRUE if all the clients destroyed successfully, FALSE otherwise.
*/
bool GattServiceDiscovery_DestroyClients(gatt_cid_t cid);

#else /* USE_SYNERGY */

/*! \brief Events sent by GATT Service discovery to other modules. */
typedef enum
{
    /*! Service discovery complete confirmation */
    GATT_SERVICE_DISCOVERY_COMPLETE = GATT_LEA_DISCOVERY_MESSAGE_BASE,

    /*! This must be the final message */
    GATT_SERVICE_DISOVERY_MESSAGE_END
} gatt_service_discovery_msg_t;

/*! \brief Completion of the GATT Service discovery for Profiles */
typedef struct
{
    /*! GATT Connection identifier for which the Service discovery was complete. */
    gatt_cid_t cid;
} GATT_SERVICE_DISCOVERY_COMPLETE_T;

/*! \brief Destroying clients are done internally upon GATT disconnection */
#define GattServiceDiscovery_DestroyClients(cid) (FALSE)

/*! \brief Register a Task to receive notifications from Gatt Service Discovery.

    Once registered, #client_task will receive gatt service discovery messages

    \param client_task Task to register to receive gatt service discovery notifications.
*/
void GattServiceDiscovery_ClientRegister(Task client_task);

/*! \brief Un-register a Task that is receiving notifications from Gatt Service Discovery.

    If the task is not currently registered then nothing will be changed.

    \param client_task Task to un-register from Gatt Service Discovery notifications.
*/
void GattServiceDiscovery_ClientUnregister(Task client_task);

/*! \brief Register GATT services for discovery.
           GATT service discovery module will only discover the services registered with it.
    \param service_id Service ID of the service
*/
void GattServiceDiscovery_RegisterServiceForDiscovery(GattSdSrvcId service_id);

/*! \brief Start GATT services discovery again.
           GATT service discovery module will only discover the services registered with it.
    \param cid GATT connection identifier for which the service discovery has to be initiated

    Note: Starting service discovery is done internally upon GATT connection if the device
          corresponding to the cid is available at that time.
          In scenarios where device creation is delayed to due to some reason, this function
          can be used to manually trigger the service discovery.
          Before calling this function, ensure the BT Device is created in the database for
          the remote device with respect to the given GATT cid. 
*/
void GattServiceDiscovery_StartServiceDiscovery(gatt_cid_t cid);

/*! \brief Check if GATT service discovery has been completed for a GATT connection id.

    \param cid GATT connection identifier for which to check if service discovery has been completed.

    \return TRUE if service discovery has been completed; FALSE otherwise.
*/
bool GattServiceDisovery_IsServiceDiscoveryCompleted(gatt_cid_t cid);

#endif /* USE_SYNERGY */

#endif
/*! @} */