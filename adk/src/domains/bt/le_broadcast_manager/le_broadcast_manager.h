/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   leabm LE Audio Broadcast Manager
    @{
    \ingroup    bt_domain
    \brief      Manager for LE Broadcast Audio.
*/

#ifndef LE_BROADCAST_MANAGER_H_
#define LE_BROADCAST_MANAGER_H_

#include <device.h>

#include "le_broadcast_manager_types.h"
#include "audio_sources_list.h"
#include "scan_delegator_role.h"
#include "domain_message.h"

#define BASIC_AUDIO_ANNOUNCEMENT_SERVICE_UUID       0x1851
#define BROADCAST_AUDIO_ANNOUNCEMENT_SERVICE_UUID   0x1852

#define INVALID_LE_BM_BASS_CLIENT_ID 0

typedef enum le_broadcast_manager_messages
{
    LE_BROADCAST_MANAGER_STOP_CFM = LE_BROADCAST_MANAGER_MESSAGE_BASE,
    LE_BROADCAST_MANAGER_REMOVE_ALL_SOURCES_CFM,
    LE_BROADCAST_MANAGER_SELF_SCAN_STOP_ALL_CFM,
    LE_BROADCAST_MANAGER_MESSAGE_END
} le_broadcast_manager_messages_t;

#if defined(INCLUDE_LE_AUDIO_BROADCAST)

/*! \brief Initialises the LE Broadcast Manager

    Initialises the LE Broadcast manager, which initialises all components required for Broadcast LE Audio
 */
bool LeBroadcastManager_Init(Task init_task);

/*! \brief Stop the LE broadcast manager receiving a broadcast.
    \param requestor The task requesting the disconnection. May be set to NULL
    if the requestor does not require a response message.

    A LE_BROADCAST_MANAGER_STOP_CFM message is sent when all broadcast streams
    disconnected and the manager is stopped. The message is sent to the requestor
    even if the manager was already stopped when the function was called.

    If LeBroadcastManager_Stop is called by multiple requestors, each will
    receive a LE_BROADCAST_MANAGER_STOP_CFM.

    \note This function guarantees to send a LE_BROADCAST_MANAGER_STOP_CFM if
    the broadcast assistant is not connected. If the broadcast assistant is
    connected, it may interfere with the stopping procedure. For example, the
    assistant may request that the broadcast manager re-syncs to a BIS. This is
    not currently handled and the manager may not respond in such cases. To
    guarantee receiving the LE_BROADCAST_MANAGER_STOP_CFM ensure the broadcast
    assistant is disconnected prior to calling this function.
*/
void LeBroadcastManager_Stop(Task requestor);

/*! \brief Pause the LE broadcast manager.
    \param requestor The task requesting the pause. Must be set to a valid task.

    Pausing means the LE broadcast manager will stop sync to all BISes. But it
    will remain synchronised to periodic advertisements.

    When paused, LE broadcast manager will ignore any requests from a broadcast
    assistant to synchronise to a BIS. It will comply with requests to sync or
    unsync to the periodic advertisement train.
*/
void LeBroadcastManager_Pause(Task requestor);

/*! \brief Mute using syncronised start mute.
    \param seconds Number of seconds to mute for.

    If seconds is set to 0 the output will be un-muted
*/
void LeBroadcastManager_OutOfEarMute(uint8 seconds);

/*! \brief Unsync from all Broadcast Sources.

    Stop syncing to any Broadcast Sources in the local BASS server and reset
    the paused state of le_broadcast_maanger.

    If there is a peer earbud connected then this command will be forwarded
    to that as well.

    Note: The Broadcast Sources are not removed from the local BASS server.
*/
void LeBroadcastManager_Unsync(void);

/*! \brief Resume the LE broadcast manager.
    \param requestor The task requesting the resume. Must be set to a valid task.

    The LE broadcast manager will only resume when all tasks requesting pause
    have called this function.

    The LE broadcast manager will resume sync to a BIS the broadcast
    assistant has requested.
*/
void LeBroadcastManager_Resume(Task requestor);

/*! \brief Query if the LE broadcast manager is paused.
    \return TRUE if paused.
*/
bool LeBroadcastManager_IsPaused(void);

/*! \brief Query if the device is currently receiving a broadcast.
    \return TRUE if broadcast receive is active.
*/
bool LeBroadcastManager_IsBroadcastReceiveActive(void);

/*! \brief Check if any broadcast source is currently pa synced

    Check all possible broadcast sources and check if any are synced to a
    periodic advertisement.

    \return TRUE if at least one source is pa synced; FALSE otherwise.
*/
bool LeBroadcastManager_IsAnySourceSyncedToPa(void);

/*! \brief Check if any broadcast source is currently synced to a BIS

    Check all possible broadcast sources and check if any are synced to a
    BIS.

    \return TRUE if at least one source is BIS synced; FALSE otherwise.
*/
bool LeBroadcastManager_IsAnySourceSyncedToBis(void);


/*! \brief Get the device for the routed broadcast source.

    This function will only try to get the device for the broadcast source
    based on its address. It will not try to get the device of the broadcast
    assistant, even if there is one connected.

    \param source Audio source to get the device for.

    \return A valid device_t or NULL if no device for the broadcast source
            address could be found.
*/
device_t LeBroadcastManager_GetDeviceForAudioSource(audio_source_t source);

/*! \brief Find out if we have an active broadcast source which is from 
           the broadcast assistant (colocated)

    \note Use the function LeBroadcastSource_IsActiveSourceNonColocated
        to check for non co-located broadcasts. If this functio returns 
        FALSE, it does not mean that there is a non-colocated broadcast source

    \return TRUE if a co-located broadcast source was found.
*/
bool LeBroadcastSource_IsActiveSourceColocated(void);

/*! \brief Find out if we have an active broadcast source which is \b not 
           from the broadcast assistant (non-colocated)

    \note Use the function LeBroadcastSource_IsActiveSourceColocated
        to check for co-located broadcasts. A return value of FALSE
        does not mean that there is a colocated broadcast source

    \return TRUE if a non co-located broadcast source was found
*/
bool LeBroadcastSource_IsActiveSourceNonColocated(void);

/*! \brief Find the ISO interval of the current broadcast source

    Find the interval of packets for a broadcast source. 

    \return The packet interval in units of 1.25 ms. 0 if no active broadcast source.
*/
uint16 LeBroadcastSource_GetActiveSourceIsoInterval(void);

/*! \brief Find the source id of the current broadcast source

    \param src_id Source id returned in the variable.

    \return TRUE if the operation is successful.
*/
bool LeBroadcastManager_GetActiveSourceId(uint8 *src_id);

/*! \brief Remove all Broadcast Sources from the manager and the BASS server.

    Unsync from any active Broadcast Source and then reset the state of all
    Broadcast Sources in le_broadcast_manager and the GATT BASS server.

    \param requestor The task requesting to remove all sources. May be set to NULL
                     if the requestor does not require a response message.
*/
void LeBroadcastManager_RemoveAllSources(Task Requestor);

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)

/*! \brief Get the source state for broadcast source

    \param source_id Value of type uint8 to identify the broadcast source.

    \param source_state Pointer to the structure scan_delegator_server_get_broadcast_source_state_t to populate for the broadcast source.

    \return TRUE if source state was retrieved with success, FALSE otherwise.
*/
le_bm_bass_status_t LeBroadcastManager_BassGetBroadcastSourceState(uint8 source_id, scan_delegator_server_get_broadcast_source_state_t * source_state);

/*! \brief Free the memory buffers used by a scan_delegator_source_state_t

    This function will only free any memory buffers pointed to by source_state.
    It will not attempt to free the source_state itself. That is the
    responsibility of the caller.

    \param source_state The source state to free memory for.
*/
void LeBroadcastManager_BassFreeBroadcastSourceState(scan_delegator_server_get_broadcast_source_state_t * source_state);

/*!
 * \brief Add a Broadcast Source to the server
 *
 * \param source_id returns the Source_ID assigned by the server to the new Broadcast Source
 * \param add_source_info a #le_bm_add_source_info_t structure containing information regarding the Broadcast Source
 *
 * \return TRUE if the Broadcast Source was successfully added, FALSE otherwise
 */
le_bm_bass_status_t LeBroadcastManager_BassAddSource(uint8 *source_id, const le_bm_add_source_info_t *add_source_info);


/*!
 * \brief Add or update Metadata for the Broadcast
 *        Source, and/or request the server to synchronize to, or to stop synchronization
 *        to, a PA and/or a BIS
 *
 * \param source_id Identifies the source to be modified
 * \param modify_source_info a #le_bm_modify_source_info_t structure containing information regarding the Broadcast Source
 *
 * \return TRUE if the Broadcast Source was successfully modified, FALSE otherwise
 */
le_bm_bass_status_t LeBroadcastManager_BassModifySource(uint8 source_id, const le_bm_modify_source_info_t *modify_source_info);


/*!
 * \brief Remove a Broadcast Source from the server
 *
 * \param source_id Identifies the source to be removed
 *
 * \return TRUE if the Broadcast Source was successfully removed, FALSE otherwise
 */
le_bm_bass_status_t LeBroadcastManager_BassRemoveSource(uint8 source_id);


/*!
 * \brief Sets the Broadcast Code to enable the server to decrypt an encrypted stream.
 *
 * \param source_id Identifies the source for which the Broadcast Code is to be set
 * \param broadcast_code Pointer to the 16 octets of the Broadcast Code as defined in the
 *                       Bluetooth Core Specification Volume 3, Part C, Section 3.2.6.1
 *
 * \return TRUE if the Broadcast Code was successfully set, FALSE otherwise
 */
le_bm_bass_status_t LeBroadcastManager_BassSetBroadcastCode(uint8 source_id, uint8 *broadcast_code);


/*!
 * \brief Data structure which contains the functions to be used for the client callback interface.
 * Client needs to decide which callback functions need to be implemented to handle the event or events
 * associated with the callbacks.
 */
typedef struct
{
    /*!
    * \brief callback function to be implemented by the client to handle broadcast source state change notifications.
    * \param[in] source_id. Non-zero value to identify the broadcast source for which the notification is sent.
    */
    void (*NotifySourceStateChanged)(uint8 source_id);

}le_broadcast_manager_source_state_client_if_t;

/*!
 * \brief API function to be called by the client to register for broadcast manager BASS notifications.
 *
 * \param[in] client_callback_if
 *  Pointer to the data structure which contains the client callback functions.
 *
 * \return Valid non-zero client ID if register operation is successful, 0 otherwise
 */
uint32 LeBroadcastManager_BassClientRegister(le_broadcast_manager_source_state_client_if_t * client_callback_if);

/*!
 * \brief API function to be called by the client to deregister for broadcast manager BASS notifications.
 *
 * \param[in] client_id
 *  Valid non-zero client ID returned previously when the client registered the first time.
 */
void LeBroadcastManager_BassClientDeregister(uint32 client_id);

/*! \brief Configure a broadcast source to match advertised broadcasts by Bluetooth device address rather than by Broadcast ID

    \param[in] source_id The Source ID of the source to be configured.
    \param[in] taddr The Bluetooth device address to match.

    \return TRUE if the broadcast source was successfully updated, FALSE otherwise.
*/
le_bm_bass_status_t LeBroadcastManager_BassSetSourceMatchAddress(uint8 source_id, typed_bdaddr *taddr);

/*! \brief Stop all active Self-Scan procedures.

    LE_BROADCAST_MANAGER_SELF_SCAN_STATUS_IND will be sent to each client Task.
    LE_BROADCAST_MANAGER_SELF_SCAN_STOP_ALL_CFM will be sent to the calling task.
*/
void LeBroadcastManager_SelfScanStopAll(Task task);

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN) */

#else /* INCLUDE_LE_AUDIO_BROADCAST */

#define LeBroadcastManager_IsBroadcastReceiveActive() FALSE

#define LeBroadcastManager_Stop(requestor) MessageSend((requestor), LE_BROADCAST_MANAGER_STOP_CFM, NULL)

#define LeBroadcastManager_IsAnySourceSyncedToPa() FALSE

#define LeBroadcastManager_IsAnySourceSyncedToBis() FALSE

#define LeBroadcastManager_Pause(requestor)

#define LeBroadcastManager_Resume(requestor)

#define LeBroadcastManager_GetDeviceForAudioSource(source) (UNUSED(source), (device_t) NULL)

#define LeBroadcastManager_IsPaused() FALSE

#define LeBroadcastManager_Unsync()

#define LeBroadcastManager_RemoveAllSources(task)   UNUSED(task)

#define LeBroadcastManager_BassGetBroadcastSourceState(source_id, source_state) (UNUSED(source_id), UNUSED(source_state), le_bm_bass_status_failed)

#define LeBroadcastManager_BassFreeBroadcastSourceState(source_state) (UNUSED(source_state))

#define LeBroadcastManager_BassAddSource(source_id, add_source_info) (UNUSED(source_id), UNUSED(add_source_info), le_bm_bass_status_failed)

#define LeBroadcastManager_BassModifySource(source_id, modify_source_info) (UNUSED(source_id), UNUSED(modify_source_info), le_bm_bass_status_failed)

#define LeBroadcastManager_BassSetBroadcastCode(source_id, broadcast_code) (UNUSED(source_id), UNUSED(broadcast_code), le_bm_bass_status_failed)

#define LeBroadcastManager_BassClientRegister(client_callback_if) (UNUSED(client_callback_if), 0)

#define LeBroadcastManager_BassClientDeregister(client_id)  (UNUSED(client_id) )

#define LeBroadcastManager_SelfScanStopAll(task) MessageSend((task), LE_BROADCAST_MANAGER_SELF_SCAN_STOP_ALL_CFM, NULL)

#endif /* INCLUDE_LE_AUDIO_BROADCAST */

#endif /* LE_BROADCAST_MANAGER_H_ */

/*! @} */
