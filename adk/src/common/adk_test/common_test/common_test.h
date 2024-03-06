/*!
\copyright  Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    adk_test_common
\brief      Interface for common testing functions.
*/

/*! @{ */

#ifndef COMMON_TEST_H
#define COMMON_TEST_H

/*!
 * \brief Check if requested Handset is connected over QHS or not.
 * 
 * \param handset_bd_addr BT address of the device.
 * 
 * \return TRUE if requested Handset is connected over QHS.
 *         FALSE if it is not connected over QHS or NULL BT address supplied.
 */
bool appTestIsHandsetQhsConnectedAddr(const bdaddr* handset_bd_addr);

/*!
 * \brief Check if requested Handset's SCO is active or not.
 * 
 * \param handset_bd_addr BT address of the device.
 * 
 * \return TRUE if requested Handset's SCO is active.
 *         FALSE if its SCO is not active or NULL BT address supplied.
 */
bool appTestIsHandsetHfpScoActiveAddr(const bdaddr* handset_bd_addr);

/*!
 * \brief Check if requested Handset is connected.
 *
 * \param handset_bd_addr BT address of the device.
 *
 * \return TRUE if the requested Handset has at least one profile connected.
 *         FALSE if it has no connected profiles.
 */
bool appTestIsHandsetAddrConnected(const bdaddr* handset_bd_addr);

/*!
 * \brief Check if requested Handset is connected over LE.
 *
 *        If the remote device is using a resolvable random address (an RPA)
 *        then it must be bonded so that the RPA can be resolved
 *        to the public address given to this function.
 *
 * \param handset_bd_addr Public BT address of the device.
 *
 * \return TRUE if the requested Handset has a LE ACL connected.
 *         FALSE if it has no connected profiles.
 */
bool appTestIsHandsetAddrConnectedOverLE(const bdaddr* handset_bd_addr);

/*!
 * \brief Enable the common output chain feature if it's been compiled in
 */
void appTestEnableCommonChain(void);

/*!
 * \brief Disable the common output chain feature if it's been compiled in
 */
void appTestDisableCommonChain(void);

/*!
 * \brief Get current RSSI of device.
 *
 * \param tp_bdaddr typed BT address, specifying address and transport type.
 *
 * \return RSSI if the connection to the device exists, else zero
 */
int16 appTestGetRssiOfTpAddr(tp_bdaddr *tpaddr);

/*!
 * \brief Get RSSI of current BREDR connection.
 *
 * \return RSSI if a connection exists, else zero
 */
int16 appTestGetBredrRssiOfConnectedHandset(void);

/*!
 * \brief Get RSSI of current LE connection.
 *
 * \return RSSI if a connection exists, else zero
 */
int16 appTestGetLeRssiOfConnectedHandset(void);

/*!
 * \brief Enable connection barge-in functionality.
 *
 * \return TRUE for a successful barge-in enable operation, FALSE otherwise.
 */
bool appTestEnableConnectionBargeIn(void);

/*!
 * \brief Disable connection barge-in functionality.
 *
 * \return TRUE for a successful barge-in disable operation, FALSE otherwise.
 */
bool appTestDisableConnectionBargeIn(void);

/*! \brief Check if connection barge-in is enabled

    \return TRUE if enabled, FALSE if disabled.
*/
bool appTestIsConnectionBargeInEnabled(void);

/*! \brief Return if Earbud/Headset is in A2DP streaming mode with any connected handset

    \return bool TRUE Earbud is in A2DP streaming mode
                     FALSE Earbud is not in A2DP streaming mode
*/
bool appTestIsHandsetA2dpStreaming(void);

/*! \brief Return if Earbud/Headset is in A2DP streaming mode with the handset having specific bluetooth address

    \return bool TRUE Earbud is in A2DP streaming mode
                 FALSE Earbud is not in A2DP streaming mode
*/
bool appTestIsHandsetA2dpStreamingBdaddr(bdaddr* bd_addr);

/*! \brief Configure parameters for the Handset Service to use when reconnecting link lossed handsets

    \param use_unlimited_reconnection_attempts - TRUE to attempt to reconnect, till the handset
                                                 is disconnected, either by an in-case event or
                                                 being connection barged-out.
    \param num_connection_attempts - the number of ACL connection attempts that shall be made,
                                     before starting to use link loss specific interval and timeout.
    \param reconnection_page_interval_ms - the interval at which to page for link loss handsets,
                                           after the usual connection attempts have expired.
    \param reconnection_page_timeout_ms - the timeout for each individual page attempt for a
                                          link loss handset, used after the usual connection
                                          attempts have expired.
 */
void appTestConfigureLinkLossReconnectionParameters(
        bool use_unlimited_reconnection_attempts,
        uint8 num_connection_attempts,
        uint32 reconnection_page_interval_ms,
        uint16 reconnection_page_timeout_ms);
		
/*! \brief Suspends A2dp Media
    \return TRUE Media suspended successfully
            FALSE Media suspend failed
*/
bool appTestHandsetA2dpMediaSuspend(void);

#ifdef INCLUDE_FAST_PAIR
/*! \brief Assign a Fast Pair Account Key to all handsets in the TDL

       For each handset which doesn't have an assigned account key already,
       this function will generate a new account key and bind it to that handset.

       Essentially, it will be as though the handset has undergone a Retroactive Fast Pairing.

    \return void.
*/
void appTestAssignFastPairAccountKeyToAllHandsets(void);

/*! \brief Disable connection switch for SASS
 *  \param void
    \return void
*/
void appTest_SassDisableConnectionSwitch(void);

/*! \brief Enable connection switch for SASS
 *  \param void
    \return void
*/
void appTest_SassEnableConnectionSwitch(void);

/*! \brief Check if connection switching for SASS is disabled
 *  \param void
    \return bool TRUE if connection switch for SASS is disabled, FALSE otherwise
*/
bool appTest_IsSassSwitchDisabled(void);
#endif /* INCLUDE_FAST_PAIR */

/*! \brief Configure the recommended settings for using infinite link loss reconnection for handsets.

    \param use_unlimited_reconnection_attempts - TRUE to attempt to reconnect indefinitely, till the handset
                                                 is disconnected, either by an in-case event or
                                                 being connection barged-out. FALSE and we shall only try
                                                 to reconnect the configured, finite, number of attempts.
 */
void appTestConfigureRecommendedInfiniteLinkLossReconnectionParameters(bool use_unlimited_reconnection_attempts);

#endif /* COMMON_TEST_H */

/*! \brief Block disconnection of handsets with DFU connection when processing ui_input_disconnect_lru_handset
 *  \param block TRUE to block, FALSE to allow
    \return TRUE if successful, otherwise FALSE
*/
bool appTestBlockDisconnectOfHandsetsWithDfuConnection(bool block);

/*! \brief Block disconnection of handsets with voice or audio focus when processing ui_input_disconnect_lru_handset
 *  \param block TRUE to block, FALSE to allow
    \return TRUE if successful, otherwise FALSE
*/
bool appTestBlockDisconnectOfHandsetsWithVoiceOrAudioFocus(bool block);

/*! \brief Cancel monitoring Aptx Voice packet to blacklist the handset in case no audio
 *  \param handset_bd_addr BT address of the handset device.
    \return TRUE if aptx Voice packet counter is disabled for the session
*/
bool appTestCancelAptxVoicePacketsCounter(const bdaddr* handset_bd_addr);


#ifdef INCLUDE_GATT_QSS_SERVER
/*! \brief Sets user description for QSS service.

    \param description Pointer to user description
    \param len Length of user description

    \returns TRUE if user description is being updated successfully, else FALSE
*/
bool appTest_SetQssUserDescription(const char *description, uint8 len);
#endif /* INCLUDE_GATT_QSS_SERVER */

/*! \brief Report the contents of the Device Database. 
*/
void appTest_DeviceDatabaseReport(void);

#ifdef ENABLE_BITRATE_STATISTIC
/*! \brief Utility API to retrive Streaming information statistics
 *  \param void
    \returns TRUE if streaming info is successfully retreived, else FALSE
*/
bool appTestGetStreamingInfo(void);
#endif /* ENABLE_BITRATE_STATISTIC */

/*! @} */

