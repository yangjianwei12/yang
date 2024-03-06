/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    leabm
    \brief      Configuration parameters for the LE Broadcast Manager.
*/

#ifndef LE_BROADCAST_MANAGER_CONFIG_H_
#define LE_BROADCAST_MANAGER_CONFIG_H_

/*! The maximum number of Broadcast Sources that be handled simultaneously by the Broadcast Manager */
#define BROADCAST_MANAGER_MAX_BROADCAST_SOURCES             2
/*! The time to wait for the PAST procedure to complete */
#define BROADCAST_MANAGER_PAST_TIMEOUT_MS                   3000
/*! The time to wait to find the EA report for a Broadcast Source. */
#define BROADCAST_SINK_FIND_TRAINS_TIMEOUT_MS               4000
/*! The BIG create sync maximum sub events */
#define BROADCAST_MANAGER_BIG_SYNC_MSE                      0
/*! The BIG create sync timeout of BIS PDUs. Range: 0x000A to 0x4000. Time = N*10 ms. Time Range: 100ms to 163.84s */
#define BROADCAST_MANAGER_BIG_SYNC_TIMEOUT                  0x32
/*! The number of times to try and regain PA sync after link loss */
#define BROADCAST_MANAGER_PA_SYNC_LOST_RETRIES              2
/*! The time to wait before attempting another resync to PA, when sync was lost */
#define BROADCAST_MANAGER_TIME_BETWEEN_RESYNC_TO_PA_MS      5000

/*! The delay to add when synchronising the start of playback on both earbuds. */
#define LeBroadcastManager_ConfigSyncStartDelayMs() (120)

/*! The maximum time to wait for a response from the peer before un-muting the broadcast audio chain. */
#define LeBroadcastManager_ConfigSyncUnMuteTimeoutMs()  (500)

/*! Controls whether LE Broadcast is treated as a high priority BT bandwidth
    user that can cause other lower priority users, e.g. DFU, to be
    paused / disabled. */
#define LeBroadcastManager_IsHighPriorityBandwidthUser() (FALSE)

#endif /* LE_BROADCAST_MANAGER_CONFIG_H_ */
