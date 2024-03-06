/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \defgroup   generic_broadcast_scan_server   Generic Broadcast Scan Server
    @{
        \ingroup    bt_domain
        \brief      Header file for the Generic Broadcast Scan Server module.

*/

#ifndef GENERIC_GENERIC_BROADCAST_SCAN_SERVER_H_
#define GENERIC_GENERIC_BROADCAST_SCAN_SERVER_H_

#ifdef INCLUDE_GBSS
#include "csr_bt_tasks.h"

#define NUM_GBSS_BRS (2)

typedef uint16 gbss_client_config;

typedef struct
{
    gbss_client_config gbss_scan_ccc;
    gbss_client_config gbss_rcv_state_ccc[NUM_GBSS_BRS];
    gbss_client_config gbss_scan_cp_ccc;
    gbss_client_config gbss_volume_state_ccc;
} gbss_config_t;


/*! \brief Initialise the GBSS Server.

    \param init_task    Task to send init completion message to

    \returns TRUE
*/
bool GenericBroadcastScanServer_Init(Task init_task);

status_t GenericBroadcastScanServer_AddConfig(connection_id_t cid,
                                           const gbss_config_t *config);

/*!
    \brief Remove the configuration for a peer device, identified by its
           Connection ID.

    This removes the configuration for that peer device, freeing the resources
    used for that config.
    This should only be done when the peer device is disconnecting.

    \param cid A Connection ID for the peer device.

    \return gbss_config_t Pointer to the peer device configuration
            data. It is the upper layer responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
            If the connection_id_t is not found, the function will return NULL.
*/
gbss_config_t* GenericBroadcastScanServer_RemoveConfig(connection_id_t  cid);

/*!
    \brief Send a notification with GBSS control point response
 */
void GenericBroadcastScanServer_NotifyGbssScanControlPointResponse(connection_id_t cid);
/*!
    \brief Send a notification with GBSS scan report
 */
void GenericBroadcastScanServer_NotifyGbssScanReport(void);

/*!
    \brief Send a notification with GBSS receiver state characteristic

    This is mirroring the NTF send by BASS
 */
void GenericBroadcastScanServer_NotifyReceiverState(uint8 source_id);

#endif /* INCLUDE_GBSS */
#endif /* GENERIC_GENERIC_BROADCAST_SCAN_SERVER_H_ */

/*! @} */