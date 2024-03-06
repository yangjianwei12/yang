/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup handset_service
    @{
    \brief      Configurable values for the Handset Service.
*/

#ifndef HANDSET_SERVICE_CONFIG_H_
#define HANDSET_SERVICE_CONFIG_H_

#include "handset_service.h"
#include "rtime.h"

/*! Maximum permitted number of BR/EDR ACL connections
*/
#define HANDSET_SERVICE_MAX_PERMITTED_BREDR_CONNECTIONS 2

/*! The delay period after the maximum number of BREDR ACLs being connected before the
    Handset Service shall attempt to enable Truncated Page Scan. This delay is designed
    to allow time for the Handset that has caused us to hit the maximum number of BREDR
    ACLs threshold to connect all its profiles before we allow another handset to
    barge-in. */
#ifndef HANDSET_SERVICE_PROFILE_CONNECTION_GUARD_PERIOD_MS
#define HANDSET_SERVICE_PROFILE_CONNECTION_GUARD_PERIOD_MS    30000
#endif


#define HANDSET_SERVICE_FAST_PAGE_SCAN_TIMEOUT_MS 5000

/*! Time to stop advertising for when a device connects that could pair

    The advertising suspension will finish earlier if pairing completes.
 */
#define handsetService_AdvertisingSuspensionForPairingMs() (5000)

/*! Time delay between each handset ACL connect retry.

    The delay is in ms.
*/
#define handsetService_BredrAclConnectRetryDelayMs() (500)

/*! Maximum allowed duration for directed advertising.
    If the targeted device did not connect within this duration,
    directed advertising will be stopped.

    The delay is in ms.
*/
#define HANDSET_SERVICE_DIRECTED_ADVERTS_TIMEOUT_MS 15000

/*! Maximum number of handset ACL connect requests to attempt.

    This is the maximum number of times the BR/EDR ACL connection to the
    handset will be attempted for a single client handset connect request.

    After this the connection request will be completed with a fail status.
*/
uint8 handsetService_BredrAclConnectAttemptLimit(void);

/*! \brief Set the maximum number of handset ACL connect requests to attempt.

    \param num_attempts the maximum number of connection attemtpts
*/
bool handsetService_SetBredrAclConnectAttemptLimit(uint8 num_attempts);

/*! Maximum permitted number of BR/EDR ACL connections

    Can be modified using HandsetService_Configure.
*/
uint8 handsetService_BredrAclMaxConnections(void);

/*! \brief Check if connection barge-in is enabled

    \return TRUE if enabled, FALSE if disabled.
*/
bool handsetService_IsConnectionBargeInEnabled(void);

/*! Handle creation of the SELF device

  This is the time when data can be stored in the device database.
*/
void handsetService_HandleSelfCreated(void);

/*! Handle update of handset_service_config property
    It is expected to be called on the secondary earbud.
*/
void handsetService_HandleConfigUpdate(void);

/*! Maximum permitted number of LE ACL connections
*/
uint8 handsetService_LeAclMaxConnections(void);

/*! \brief Set whether Handset Service connection barge-in functionality is enabled or not.
 *
 *  \param enabled - set TRUE to enable connection barge-in, FALSE to disable.
 *  \return TRUE for a successful change of the barge-in enable state, FALSE otherwise.
 *               If the state was already configured as requested, returns TRUE.
 */
bool handsetService_SetConnectionBargeInEnable(bool enabled);

/*! \brief Test whether unlimited ACL reconnection attempts are currently enabled.
 *
 *  \return TRUE if unlimited ACL reconnections are enabled, FALSE otherwise.
 */
bool handsetService_IsUnlimitedAclReconnectionEnabled(void);

/*! \brief Get the interval between page scan attempts that the Handset Service is using for
 *         normal ACL reconnections.
 *
 *  \return the page interval in ms.
 */
uint32 handsetService_GetInitialReconnectionPageInterval(void);

/*! \brief Get the currently configured page timeout used for initial reconnection.
 *
 *  \return the initial reconnection page timeout in ms.
 */
uint16 handsetService_GetInitialReconnectionPageTimeout(void);

/*! \brief Get the interval between page scan attempts that the Handset Service is using for
 *         unlimited ACL reconnections.
 *
 *  \return the page interval in ms.
 */
uint32 handsetService_GetUnlimitedReconnectionPageInterval(void);

/*! \brief Get the currently configured page timeout the Handset Service shall use when
 *         performing infinite link loss reconnection.
 *
 *  \return the page timeout in ms, for unlimited reconnection.
 */
uint16 handsetService_GetUnlimitedReconnectionPageTimeout(void);

/*! Initialise handset service configuration to defaults
*/
void HandsetServiceConfig_Init(void);

uint32 HandsetService_CalculatePageIntervalMsValue(uint8 page_interval_config);

#endif /* HANDSET_SERVICE_CONFIG_H_ */
/*! @} */