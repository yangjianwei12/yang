/*!
  \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
              All Rights Reserved.
              Qualcomm Technologies International, Ltd. Confidential and Proprietary.
  \file
  \defgroup   leabm-sync LE Broadcast audio start synchronisation
  @{
  \ingroup    leabm
  \brief      Synchronise the state of LE broadcast audio between peer earbuds.

      This file contains the logic to synchronise the state of broadcast audio
      between two peer earbuds. For non-earbud devices this code is not needed
      and should compile to nothing.

      # Normal case

      The most common use-case is: both earbuds out of the case and connected
      to each other. The handset adds a broadcast source and the Primary adds
      the same source to the Secondary. Both earbuds then sync to the broadcast
      independently - so it can be at different times.

      In order to synchronise when the user hears the audio, the LE broadcast
      audio chain is started with the broadcast output muted. Then:
      - Both earbuds start a timer that will un-mute itself if the peer doesn't
        response in time.
      - When the Secondary establishes a BIS it must send the 'ready to start'
        message to the Primary.
      - When the Primary receives 'ready to start', it calculates a time for both
        earbuds to un-mute and sends a 'un-mute' message to the Secondary,
        as well as un-muting locally.

      # Edge cases

      - Only one earbud out of the case:
        - Primary will start the audio chain un-muted.
        - When the other earbuds is taken out of the case it will become the
          Secondary.
          - Secondary will start its audio chain muted and send 'read to start'
            to the Primary.
          - Primary sends the 'un-mute' message with a time of 'now'
      - Peer earbud disconnects before either the 'ready to start' or 'un-mute'
        message has been received.
        - Immediately un-mute the local audio chain; cancel the queued timeout.
      - Secondary loses sync to its BIS
        - If / when it re-establishes the BIS it starts the audio chain muted.
        - Secondary sends the 'ready to start' message
        - Primary sends the 'un-mute' message with a time of 'now'
      - Secondary establishes its BIS before the Primary
        - Secondary sends the 'ready to start' message
        - Primary records that Secondary is ready.
        - When Primary syncs to its BIS, it calculates a time to un-mute
          and sends the 'un-mute' message to the Secondary.
          - It does not have to wait for another 'ready to start'.
*/

#ifndef LE_BROADCAST_MANAGER_SYNC_H
#define LE_BROADCAST_MANAGER_SYNC_H

#if defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_MIRRORING)

#include "le_broadcast_manager_data.h"
#include "le_broadcast_manager_sync_typedef.h"

#include <rtime.h>


/*! \brief Initialise the LE broadcast synchronisation module.
*/
void LeBroadcastManager_SyncInit(void);

/*! \brief Send a message to the peer that this device is ready to un-mute.

    This should only be sent by the Secondary to tell the Primary when it
    has established a BIS and it has started the process of creating the
    broadcast audio chain.

    Note: the audio chain should be created with the broadcast output muted.

    \param source_id The broadcast source id in the gatt_bass_server.
*/
void LeBroadcastManager_SyncSendReadyToStart(uint8 source_id);

/*! \brief Tell the peer to un-mute its broadcast audio chain.

    This should only be sent by the Primary to tell the Secondary when
    to un-mute its broadcast audio chain output.

    Note: The time is converted to the BT clock during transmission and then
          converted to the peer device system time when it is received.

    \param source_id The broadcast source id in the gatt_bass_server.
    \param time The system time to un-mute the broadcast audio chain
                output.
*/
void LeBroadcastManager_SyncSendUnmuteInd(uint8 source_id, rtime_t time);

/*! \brief LeBroadcastManager_SyncCheckIfStartMuted

    Check if this device should start the broadcast audio chain muted so that
    both devices can synchronise un-muting.
*/
void LeBroadcastManager_SyncCheckIfStartMuted(broadcast_source_state_t *broadcast_source);

/*! \brief Handles the BROADCAST_MANAGER_INTERNAL_UNMUTE_TIMEOUT message.

*/
void LeBroadcastManager_SyncHandleInternalUnmuteTimeout(void);

/*! \brief Sends a le_broadcast_sync_command to the peer.
*/
void LeBroadcastManager_SyncSendCommandInd(le_broadcast_sync_command_t command);

/*! \brief Sends a le_broadcast_sync_pause_ind to the peer.
*/
void LeBroadcastManager_SyncSendPause(uint8 source_id);

/*! \brief Sends a le_broadcast_sync_resume_ind to the peer.
*/
void LeBroadcastManager_SyncSendResume(uint8 source_id);

/*! \brief Sends a le_broadcast_manager_sync_to_source to the peer.
*/
void LeBroadcastManager_SyncSendSyncToSource(uint8 source_id);

/*! \brief Add a source to the peer using the BASS 'Add Source' operation.
*/
void LeBroadcastManager_SyncAddSource(scan_delegator_client_add_broadcast_source_t * source);

/*! \brief Modify a source on the peer using the BASS 'Modify Source' operation.
*/
void LeBroadcastManager_SyncModifySource(scan_delegator_client_modify_broadcast_source_t * source);

/*! \brief Remove a source from the peer using the BASS 'Remove Source' operation.
*/
void LeBroadcastManager_SyncRemoveSource(uint8 source_id);

/*! \brief Set the Broadcast Code for a source on the peer using the BASS 'Set Broadcast Code' operation.
*/
void LeBroadcastManager_SyncSetBroadcastCode(uint8 source_id, uint8 *broadcast_code);

/*! \brief Configure a broadcast source to match advertised broadcasts by Bluetooth device address rather than by Broadcast ID */
void LeBroadcastManager_SyncSetSourceMatchAddress(uint8 source_id, typed_bdaddr *taddr);

#else /* defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_MIRRORING) */

#define LeBroadcastManager_SyncInit() /* Nothing to do */

#define LeBroadcastManager_SyncSendReadyToStart(source_id)  UNUSED(source_id)

#define LeBroadcastManager_SyncSendUnmuteInd(source_id, time)  do { UNUSED(source_id); UNUSED(time); } while(0)

#define LeBroadcastManager_SyncCheckIfStartMuted(broadcast_source)  UNUSED(broadcast_source)

#define LeBroadcastManager_SyncHandleInternalUnmuteTimeout()    /* Nothing to do */

#define LeBroadcastManager_SyncSendCommandInd(command)  UNUSED(command)

#define LeBroadcastManager_SyncSendPause(source_id)  UNUSED(source_id)

#define LeBroadcastManager_SyncSendResume(source_id)  UNUSED(source_id)

#define LeBroadcastManager_SyncSendSyncToSource(source_id)  UNUSED(source_id)

#define LeBroadcastManager_SyncAddSource(source) UNUSED(source)

#define LeBroadcastManager_SyncModifySource(source) UNUSED(source)

#define LeBroadcastManager_SyncRemoveSource(source_id) UNUSED(source_id)

#define LeBroadcastManager_SyncSetBroadcastCode(source_id, broadcast_code) (UNUSED(source_id), UNUSED(broadcast_code))

#define LeBroadcastManager_SyncSetSourceMatchAddress(source_id, taddr) (UNUSED(source_id), UNUSED(taddr))

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_MIRRORING) */

#endif /* LE_BROADCAST_MANAGER_SYNC_H */
/*! @} */