/*!
\copyright  Copyright (c) 2019-2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       earbud_sm_config.h
\brief      Configuration related definitions for the state machine.
*/

#ifndef EARBUD_SM_CONFIG_H_
#define EARBUD_SM_CONFIG_H_


/*! Timeout for entering the case after selecting DFU from the user
    interface */
#define appConfigDfuTimeoutToPlaceInCaseMs()        (D_SEC(60))

/*! Timeout for starting DFU mode after a restart caused by an
    upgrade completing. The timeout will only apply if the device
    is out of the case.

    The timeout can be set to 0, in which case there is no limit.

    Note: Currently, this timeout timer is removed during DFU process because
    this can lead to rebooting the primary device if handset is not able to
    connect post DFU reboot, while the secondary device can becomes new primary
    and connect with Handset with the DFU'ed image.

    Maintaining the definition for now, if in case it get used for some other
    purpose.

    */
#define appConfigDfuTimeoutToStartAfterRestartMs()  (D_SEC(30))

/*! Timeout for DFU mode, entered after a reboot in DFU mode.

    This is the timeout for an abnormal restart. Restarts that occur
    as part of the normal upgrade process use
    appConfigDfuTimeoutToStartAfterRestartMs()

    The timeout can be set to 0, in which case there is no limit. */
#define appConfigDfuTimeoutToStartAfterRebootMs()   (D_SEC(15))

/*! Timeout for DFU mode, entered from UI

    This is the timeout for starting an upgrade when the user has
    requested DFU and then placed the device into the case.

    The timeout can be set to 0, in which case there is no limit. */
#define appConfigDfuTimeoutAfterEnteringCaseMs()    (D_SEC(50))

/*! Timeout for DFU mode, requested from GAIA

    This is the timeout for starting an upgrade after the GAIA
    upgrade protocol has been connected. Only applicable in the
    in case DFU mode.

    The timeout can be set to 0, in which case there is no limit.
*/
#define appConfigDfuTimeoutAfterGaiaConnectionMs()  (D_SEC(45))

/*! Timeout to detect ending a GAIA upgrade connection shortly after starting

    This can be used by a host application to check whether the upgrade
    feature is supported. This should not count as an upgrade connection.
 */
#define appConfigDfuTimeoutCheckForSupportMs()      (D_SEC(3))

/*! Time to wait for successful disconnection of links to peer and handset
 *  in terminating substate before shutdown/sleep. */
#define appConfigLinkDisconnectionTimeoutTerminatingMs() D_SEC(5)

/*! \brief Minimum time before resetting when deleting handsets
    The delay is added to allow for message to be sent to the peer
    if connected. */
#define appConfigHandsetDeleteMinimumTimeoutMs()        D_SEC(1)

/*! Timeout for audio when earbud removed from ear. */
#define appConfigOutOfEarAudioTimeoutSecs()      (2)

/*! Timeout within which audio will be automatically restarted
 *  if placed back in the ear. */
#define appConfigInEarAudioStartTimeoutSecs()    (10)

/*! Timeout for SCO audio when earbud removed from ear. */
#define appConfigOutOfEarScoTimeoutSecs()      (2)

/*! This timer is active in APP_STATE_OUT_OF_CASE_IDLE if set to a non-zero value.
    On timeout, the SM will allow sleep. */
#define appConfigOutOfCaseIdleTimeoutMs()   D_SEC(300)

/*! This timer is active in APP_STATE_IN_CASE_IDLE if set to a non-zero value.
    On timeout, the SM will attempt to enter dormant. This timeout should be
    long enough to allow any LED indication to show that charging has completed. */
#ifndef APP_CONFIG_IN_CASE_IDLE_TIMEOUT_MS
#define APP_CONFIG_IN_CASE_IDLE_TIMEOUT_MS D_SEC(0)
#endif
#define appConfigInCaseIdleTimeoutMs() APP_CONFIG_IN_CASE_IDLE_TIMEOUT_MS

/*! Should new connections be allowed when music is being played
    or when we are in a call.

    Selecting this option
    \li reduces power consumption slightly as the advertisements neccesary
    for a connection are relatively low power,
    \li stops any distortion from connections

    \note Existing connections are not affected by this option
 */
#define appConfigBleNewConnectionsWhenBusy()   (FALSE)

/*! \brief Timeout for remembering the peer was pairing when it went into the case

    If the primary device went into the case while pairing it informs the
    secondary. This is an unusual flow and hard to make sure the flag is cleared on
    all possible state transitions / code flows, especially when new states and 
    flows are added. As a fail safe cancel the flag on a timeout.
 */
#define appConfigPeerInCaseIndicationTimeoutMs() (5000)

/*! \brief Connect handset pause timeout.

    If A2DP starts streaming when the earbud is connecting other handsets, the
    connection attempt will be paused. If A2DP stops streaming before this
    timeout the connection attempt will be resumed. If this timeout fires,
    the connection will not be re-attempted.
 */
#define appConfigPauseHandsetConnectTimeout() D_SEC(30)

/*! \brief The delay in milliseconds before the factory reset message will be sent
    When sending the factory reset message to ourselves there will be a delay to ensure
    that we have enough time to forward the factory reset command to the peer.
 */
#define appConfigDelayBeforeFactoryResetMs() (300)

/*! \brief Connect handset pause timeout.

    If A2DP starts streaming when the earbud is connecting other handsets, the
    connection attempt will be paused. If A2DP stops streaming before this
    timeout the connection attempt will be resumed. If this timeout fires,
    the connection will not be re-attempted.
 */
#define appConfigPauseHandsetConnectTimeout() D_SEC(30)



#endif /* EARBUD_SM_CONFIG_H_ */
