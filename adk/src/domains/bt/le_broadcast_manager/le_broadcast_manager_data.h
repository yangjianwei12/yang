/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup leabm
    \brief      Handles the data associated with the LE Broadcast Manager.
    @{
*/

#ifndef LE_BROADCAST_MANAGER_DATA_H_
#define LE_BROADCAST_MANAGER_DATA_H_


#include "le_broadcast_manager_config.h"
#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
#include "le_broadcast_manager_periodic_scan.h"
#endif
#include "ltv_utilities.h"
#include "scan_delegator_role.h"

#include <logging.h>
#include <task_list.h>

#include <stdlib.h>
#include <stdio.h>

#define BROADCAST_MANAGER_DATA_LOG      DEBUG_LOG
#define BIS_SYNC_NO_PREFERENCE          0xFFFFFFFFUL

#define BROADCAST_MANAGER_AUDIO_LOCATION_NOT_SET    0
#define BROADCAST_MANAGER_AUDIO_ACTIVE_STATE_NO_PRESENT 0xFF

/* ! BIS location mapping and Maximum number of BIS supported */
typedef enum
{
    broadcast_manager_bis_location_left_or_stereo = 0,
    broadcast_manager_bis_location_right,
    broadcast_manager_bis_location_max
} broadcast_manager_bis_location_t;

/*! The Broadcast Manager synchronization state used when syncing to PA and BIS.
    This ensures that only one synchronization producedure occurs at a time. */
typedef enum
{
    /*! No sync to PA or BIS procedure in progress */
    broadcast_manager_sync_none,
    /*! Waiting for PA sync information to arrive from Broadcast Assistant */
    broadcast_manager_sync_waiting_for_sync_to_pa,
    /*! Currently scan for PA */
    broadcast_manager_sync_scanning_for_pa,
    /*! Cancelling scan for PA */
    broadcast_manager_sync_cancelling_scan_for_pa,
    /*! Stop scan then sync to PA */
    broadcast_manager_sync_cancelling_scan_sync_to_pa,
    /*! Currently syncing to PA */
    broadcast_manager_sync_syncing_to_pa,
    /*! Cancelling sync to PA */
    broadcast_manager_sync_cancelling_sync_to_pa,
    /*! Currently syncing to BIS */
    broadcast_manager_sync_syncing_to_bis,
    /*! Currently stopping sync to PA */
    broadcast_manager_sync_stopping_sync_to_pa,
    /*! Currently stopping sync to BIS */
    broadcast_manager_sync_stopping_sync_to_bis,
    /*! Currently synced to BIS and creating ISO data path */
    broadcast_manager_sync_synced_to_bis_create_iso,
} broadcast_manager_sync_state_t;

/*! The state of the Broadcast assistant remote scanning. */
typedef enum
{
    /*! Broadcast assistant remote scanning is inactive */
    broadcast_manager_assistant_scan_inactive,
    /*! Broadcast assistant remote scanning is active */
    broadcast_manager_assistant_scan_active
} broadcast_manager_assistant_scan_t;

typedef struct
{
    uint16 sample_rate;
    uint16 frame_duration;
    uint16 octets_per_frame;
    uint8 codec_frame_blocks_per_sdu;
} broadcast_source_codec_config_t;

typedef struct
{
    uint8 num_subgroups;
    uint32 *bis_sync;
}le_broadcast_manager_bis_sync_state_t;

typedef struct
{
    uint8 is_stereo_bis; /* Indicates BIS is carrying both left and right */
    uint8 subgroup;
    uint32 index;
}le_broadcast_manager_bis_index_t;

/*!  */
typedef struct
{
    /*! Indicates if pa sync lost with this Broadcast Source. */
    bool sync_lost;
    /*! Number of retries to reestablish sync. */
    uint8 retries;
} broadcast_pa_sync_lost_t;

typedef struct
{
    le_broadcast_manager_bis_index_t    base_bis_index;
    uint16                              bis_handle;
} le_broadcast_manager_bis_info_t;

/*! The Broadcast State stored for each Broadcast Source */
typedef struct
{
    TaskData task;
    uint8 source_id;
    uint16 sync_handle;
    le_bm_pa_sync_t target_pa_sync_state;

    /* The BIS sync state set by the assistant in the most recent Add or Modify operation. */
    le_broadcast_manager_bis_sync_state_t requested_bis_sync_state;

    /* The current desired BIS sync state for this source. */
    le_broadcast_manager_bis_sync_state_t target_bis_sync_state;

    le_broadcast_manager_bis_info_t bis_info[broadcast_manager_bis_location_max];
    uint8 big_handle;
    broadcast_source_codec_config_t codec_config;
    uint32 presentation_delay;
    uint8 big_encryption;
    uint16 remove_pending;
    uint16 iso_interval;
    bool dont_sync_bis;
    uint8 num_subgroups;
    broadcast_pa_sync_lost_t pa_sync_lost;
    audio_context_t streaming_audio_context;
    bool sync_to_bis_no_preference;

    /*! The assistant address for this source. (Should be public)*/
    typed_bdaddr assistant_address;

    /*! The source address for this source. */
    typed_bdaddr current_taddr;

    /*! Address to match broadcast sources in preference to broadcast ID */
    typed_bdaddr source_match_address;

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
    le_bm_periodic_scan_handle handle;
#endif
} broadcast_source_state_t;


/*! The Broadcast Manager State */
typedef struct
{
    TaskData task;
    task_list_t *stop_tasks;
    task_list_t *pause_tasks;
    task_list_t *remove_tasks;
    broadcast_manager_sync_state_t sync_state;
    uint8 sync_source_id;
    uint8 paused_bis_source_id;
    le_broadcast_manager_bis_sync_state_t paused_bis_sync_state;
    le_broadcast_manager_bis_sync_state_t pending_bis_sync_state;
    broadcast_source_state_t broadcast_source_receive_state[BROADCAST_MANAGER_MAX_BROADCAST_SOURCES];
    broadcast_manager_assistant_scan_t assistant_scan;
    
    /*! Is an incoming call active. */
    bool is_incoming_call_active;

    /*! Should the audio chain be started with the output muted. */
    bool start_muted;
    
    /*! Source ID that should be used to sync to BIS. */
    uint8 target_bis_source_id;

    /* Source ID to start up after a stop is complete */
    uint8 source_to_start;

    /*! Reference count of interested metadata clients */
    uint8 pa_metadata_client_ref_count;

    /*! How long the application will wait for PAST to succeed before timing out. */
    uint16 past_timeout;
    /*! Application timeout used when scanning for the EA of a Broadcast Source. */
    uint16 find_trains_timeout;


} le_broadcast_manager_t;

extern le_broadcast_manager_t le_broadcast_manager;


#define LeBroadcastManager_SourceGetTask(void)  (&le_broadcast_manager.task)

/*! \brief Updates the Broadcast Assistant scanning state.

    The Remote Scan Start and the Remote Scan Stop operations are received from a GATT client to inform the server that the client is scanning/not scanning on behalf of the server
    This operation is defined in the Broadcast Audio Scan Service (BASS).
    The function is called to store if a Broadcast Assistant is scanning or not scanning.
    
    \param scan The Broadcast Assistant scanning state.
*/
#define LeBroadcastManager_SourceSetAssistantScanningState(scan) le_broadcast_manager.assistant_scan = scan

/*! \brief Sets the current synchronization state for syncing to PA and BIS.

    The Broadcast Manager synchronization state is set when syncing to PA and BIS,
    or stopping sync to PA and BIS.
    Only one synchronization producedure should occur at a time, so this is used to indicate with which
    broadcast source syncing is active.

    \param source_id The broadcast source ID with which syncing is active. Use SCAN_DELEGATOR_SOURCE_ID_INVALID if syncing is not active.
    \param state The current synchronization state.
*/
void leBroadcastManager_SetSyncState(uint8 source_id, broadcast_manager_sync_state_t state);
            
/*! \brief Gets the current synchronization state for syncing to PA and BIS.

    \return The current synchronization state for syncing to PA and BIS.
*/
#define leBroadcastManager_GetSyncState() le_broadcast_manager.sync_state

/*! \brief Gets the broadcast source ID associated with the current synchronization state for syncing to PA and BIS.

    \return The broadcast source ID if synchronization is active. Shall be SCAN_DELEGATOR_SOURCE_ID_INVALID if not active.
*/
#define leBroadcastManager_GetSyncStateSourceId() le_broadcast_manager.sync_source_id

/*! \brief Use to set the BIS sync state while syncing to BIS in in progress.

    The pending BIS sync state shall be set before syncing to BIS, so it can be retrieved after the sync is complete.
    As the BIS sync state could be updated while the sync is in progress, this stores which BIS were used in the syncing procedure.

    \param state The BIS sync state to set.
*/
void leBroadcastManager_SetPendingBisSyncState(const le_broadcast_manager_bis_sync_state_t *bis_sync_state);

/*! \brief Resets the pending BIS sync state.

*/
void leBroadcastManager_ResetPendingBisSyncState(void);

/*! \brief Gets the BIS sync state used for syncing to BIS.

    \return The pending BIS sync state.
*/
le_broadcast_manager_bis_sync_state_t *leBroadcastManager_GetPendingBisSyncState(void);

/*! \brief Sets the PA sync handle if synced to PA, or 0 if not synced to PA.

    \param broadcast_source The broadcast source to use to set the PA sync handle.
    \param handle The PA sync handle. Shall use 0 if no handle exists.
*/
#define leBroadcastManager_SetPaSyncHandle(broadcast_source, handle) broadcast_source->sync_handle = handle

/*! \brief Gets the PA sync handle.

    \param broadcast_source The broadcast source to use to get the PA sync handle.

    \return The PA sync handle.
*/
#define leBroadcastManager_GetPaSyncHandle(broadcast_source) broadcast_source->sync_handle

/*! \brief Sets the BIS handle if synced to BIS, or 0 if not synced to BIS.

    \param broadcast_source The broadcast source to use to set the BIS sync handle.
    \param handle The BIS handle.
    \param bis_loc The BIS location.
*/
#define leBroadcastManager_SetBisHandle(broadcast_source, handle, bis_loc) \
    do { \
        broadcast_source->bis_info[bis_loc].bis_handle = handle; \
        BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetBisHandle broadcast_source=%p bis_handle=0x%x index=0x%x", \
                                   broadcast_source, handle, bis_loc); \
    } while (0)

/*! \brief Gets the BIS handle.

    \param broadcast_source The broadcast source to use to get the BIS handle.
    \param bis_loc The BIS location.

    \return The BIS handle.
*/
#define leBroadcastManager_GetBisHandle(broadcast_source, bis_loc) \
            (broadcast_source->bis_info[bis_loc].bis_handle)

/*! \brief Checks the BIS carries joint stereo.

    \param broadcast_source The broadcast source to use to get the BIS handle.

    \return TRUE if BIS carries carries both left and right channels.
*/
#define leBroadcastManager_IsStereoBis(broadcast_source) (broadcast_source != NULL && broadcast_source->bis_info[broadcast_manager_bis_location_left_or_stereo].base_bis_index.is_stereo_bis)

/*! \brief Sets the BIG encryption value.

    The BIG encyption value is retrieved from the BIGInfo report and indicate if the BIG is encrypted.

    \param broadcast_source The broadcast source to use to set the BIG encryption value.
    \param encryption The BIG encryption value.
*/
#define leBroadcastManager_SetBigEncryption(broadcast_source, encryption) \
    do { \
        broadcast_source->big_encryption = encryption; \
        BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetBigEncryption broadcast_source=%p big_encryption=0x%x", broadcast_source, broadcast_source->big_encryption); \
    } while (0)

/*! \brief Gets the BIG encryption value.

    \param broadcast_source The broadcast source to use to get the BIG encryption value.

    \return The BIG encryption value.
*/
#define leBroadcastManager_GetBigEncryption(broadcast_source) broadcast_source->big_encryption

/*! \brief Use to set the target PA sync state.

    The target PA sync state is the PA sync that should occur. The command may have come from a Broadcast Assistant,
    so this keeps track of what PA sync state is the goal. It may not be possible to sync immediately because of 
    another sync procedure, or activity with another Broadcast Source.

    \param broadcast_source The broadcast source to use to set the target PA sync state.
    \param pa_sync_state The target PA sync state.
*/
#define leBroadcastManager_SetTargetPaSyncState(broadcast_source, pa_sync_state) \
    do { \
        broadcast_source->target_pa_sync_state = pa_sync_state; \
        BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetTargetPaSyncState source_id=%u pa_sync_state enum:le_bm_pa_sync_t:%u", \
                                    (broadcast_source)->source_id, pa_sync_state); \
    } while (0)

/*! \brief Gets the target PA sync state.

    \param broadcast_source The broadcast source to use to get the target PA sync state.

    \return The target PA sync state.
*/
#define leBroadcastManager_GetTargetPaSyncState(broadcast_source) broadcast_source->target_pa_sync_state

/*! \brief Use to set the target BIS sync state.

    The target BIS sync state is the BIS sync that should occur. The command may have come from a Broadcast Assistant,
    so this keeps track of what BIS sync state is the goal. It may not be possible to sync immediately because of 
    another sync procedure, or activity with another Broadcast Source.

    \param broadcast_source The broadcast source to use to set the target BIS sync state.
    \param num_subgroups The number of subgroups that the BIS sync state has been set.
    \param bis_sync_state The target BIS sync state for each subgroup.
*/
void leBroadcastManager_SetTargetBisSyncState(broadcast_source_state_t *broadcast_source, uint8 num_subgroups, uint32 *bis_sync_state);

/*! \brief Resets the target BIS sync state back to initial state.

    \param broadcast_source The broadcast source to use to reset the target BIS sync state.
*/
void leBroadcastManager_ResetTargetBisSyncState(broadcast_source_state_t *broadcast_source);

/*! \brief Use to set the target BIS sync state back to no synchronization.

    \param broadcast_source The broadcast source to use to set the target BIS sync state back to no synchronization.
*/
void leBroadcastManager_SetTargetBisSyncStateNoSync(broadcast_source_state_t *broadcast_source);

/*! \brief Use to set the target BIS sync state to no preference.

    \param broadcast_source The broadcast source to use to set the target BIS sync state to no preference.
*/
void leBroadcastManager_SetTargetBisSyncStateNoPreference(broadcast_source_state_t *broadcast_source);

/*! \brief Gets the target BIS sync state as a single uint32 value.

    \param broadcast_source The broadcast source to use to get the target PA sync state.

    \return The target BIS sync state as a single uint32 value.
*/
uint32 leBroadcastManager_GetTargetBisSyncStateValue(broadcast_source_state_t *broadcast_source);

/*! \brief Gets the target BIS sync state in the subgroup structure.

    \param broadcast_source The broadcast source to use to get the target PA sync state.

    \return The target BIS sync state in the subgroup structure.
*/
le_broadcast_manager_bis_sync_state_t *leBroadcastManager_GetTargetBisSyncState(broadcast_source_state_t *broadcast_source);

/*! \brief Gets the BIS sync state as a single uint32 value.

    \param bis_sync_state The BIS sync state represented in subgroup form.

    \return The BIS sync state as a single uint32 value.
*/
uint32 leBroadcastManager_GetBisSyncStateValue(const le_broadcast_manager_bis_sync_state_t *bis_sync_state);

/*! \brief Sets the codec specific configuration for the broadcast source.

    The codec specific configuration will be read from the PA BASE structure and set using this function
    for the specific broadcast source.

    \param broadcast_source The broadcast source to set the codec specific configuration for.
    \param config Pointer to broadcast_source_codec_config_t type.

*/
#define leBroadcastManager_SetBroadcastSourceCodecConfig(broadcast_source, config) broadcast_source->codec_config = *config


/*! \brief Gets the sample rate for the broadcast stream.

    \param broadcast_source The broadcast source to use to get the sample rate.

    \return The sample rate.
*/
#define leBroadcastManager_GetBroadcastSourceSampleRate(broadcast_source) broadcast_source->codec_config.sample_rate

/*! \brief Gets the frame duration for the broadcast stream.

    \param broadcast_source The broadcast source to use to get the frame duration.

    \return The frame duration.
*/
#define leBroadcastManager_GetBroadcastSourceFrameDuration(broadcast_source) broadcast_source->codec_config.frame_duration

/*! \brief Sets the presentation delay for the broadcast stream.

    The presentation delay is retrieved from the BASE information in the PA report which is used to describe the available broadcast stream.

    \param broadcast_source The broadcast source to use to set the sample rate.
    \param delay The presentation delay.
*/
#define leBroadcastManager_SetBroadcastSourcePresentationDelay(broadcast_source, delay) broadcast_source->presentation_delay = delay

/*! \brief Gets the presentation delay for the broadcast stream.

    \param broadcast_source The broadcast source to use to get the presentation delay.

    \return The presentation delay.
*/
#define leBroadcastManager_GetBroadcastSourcePresentationDelay(broadcast_source) broadcast_source->presentation_delay

/*! \brief Sets the application specific BIG handle when creating sync to BIS.

    \param broadcast_source The broadcast source to use to set the BIG handle.
    \param handle The BIG handle.
*/
#define leBroadcastManager_SetBigHandle(broadcast_source, handle) \
    do { \
        broadcast_source->big_handle = handle; \
        BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetBigHandle broadcast_source=%p big_handle=0x%x", broadcast_source, handle); \
    } while (0)

/*! \brief Gets the application specific BIG handle used when creating sync to BIS.

    \param broadcast_source The broadcast source to use to get the BIG handle.

    \return The BIG handle.
*/
#define leBroadcastManager_GetBigHandle(broadcast_source) broadcast_source->big_handle

/*! \brief Gets the number of octets per codec frame read from the most recent PA report's BASE structure.

    \param broadcast_source The broadcast source to read the octets per codec frame from.

    \return The number of octets per codec frame.
*/
#define leBroadcastManager_GetBroadcastSourceOctetsPerFrame(broadcast_source) broadcast_source->codec_config.octets_per_frame

/*! \brief Gets the codec frame blocks per SDU read from the most recent PA report's BASE structure.

    \param broadcast_source The broadcast source to read the codec frame blocks per SDU from.

    \return The codec frame blocks per SDU.
*/
#define leBroadcastManager_GetBroadcastSourceCodecFrameBlocksPerSdu(broadcast_source) ((broadcast_source->codec_config.codec_frame_blocks_per_sdu == 0) ? 1 : broadcast_source->codec_config.codec_frame_blocks_per_sdu)

/*! \brief Sets the paused state when the LeBroadcastManager_Pause API is called.

    When the LeBroadcastManager_Pause API is called it will pause any active broadcast stream.
    The paused state is stored so that when the LeBroadcastManager_Resume is called,
    the previous sync to BIS state can be resumed.

    \param source_id The paused broadcast source ID.
    \param state The BIS sync state to store.
*/
void leBroadcastManager_SetPausedState(uint8 source_id, const le_broadcast_manager_bis_sync_state_t *state);

/*! \brief Resets the stored paused state.

*/
void leBroadcastManager_ResetPausedState(void);

/*! \brief Check if PA sync was lost unexpectedly.

    \param broadcast_source Pointer to a broadcast_source_state_t to check.

    \return TRUE if any broadcast source unexpectedly lost PA sync, FALSE otherwise.
*/
#define leBroadcastManager_PaSyncHasBeenLost(broadcast_source)  ((broadcast_source)->pa_sync_lost.sync_lost)

/*! \brief Check if a broadcast_source_state_t represents a valid broadcast source.

    A valid source is one that exists in the BASS server. Currently
    the source_id is used to check if this is true.

    \param broadcast_source Pointer to a broadcast_source_state_t to check.
    \return TRUE if the broadcast source is valid; FALSE otherwise.
*/
#define leBroadcastManager_IsSourceValid(broadcast_source) (SCAN_DELEGATOR_SOURCE_ID_INVALID != (broadcast_source)->source_id)

/*! \brief Get whether a non-broadcast audio or voice source is active.

    \return TRUE if a non-broadcast audio or voice source is active. FALSE otherwise.
*/
bool leBroadcastManager_IsNonBroadcastAudioVoiceActive(void);

/*! \brief Set whether an incoming call has become active.

    \param active TRUE if an incoming call is active. FALSE otherwise.
*/
#define leBroadcastManager_SetIncomingCallActive(active) \
    do { \
        BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetIncomingCallActive active %d", (active)); \
        le_broadcast_manager.is_incoming_call_active = (active); \
    } while (0)
        
/*! \brief Get whether an incoming call is active.

    \return TRUE if an incoming call is active. FALSE otherwise.
*/
#define leBroadcastManager_IsIncomingCallActive()    (le_broadcast_manager.is_incoming_call_active)

/*! \brief Get whether an incoming call or non-broadcast audio source is active.

    \return TRUE if an incoming call or non-broadcast audio source is active. FALSE otherwise.
*/
#define leBroadcastManager_IsNonBroadcastAudioVoiceOrIncomingCallActive()    (leBroadcastManager_IsIncomingCallActive() || leBroadcastManager_IsNonBroadcastAudioVoiceActive())

/*! \brief Sets the number of subgroups read from the most recent PA report's BASE structure.
    
    \param broadcast_source The broadcast source associated with the PA report.
    \param num_subgroups The number of subgroups.
*/
#define leBroadcastManager_SetBroadcastSourceNumberOfSubgroups(broadcast_source, number_subgroups) (broadcast_source->num_subgroups = number_subgroups)

/*! \brief Gets the number of subgroups read from the most recent PA report's BASE structure.
    
    \param broadcast_source The broadcast source associated with the PA report.
    
    \return The number of subgroups read from the most recent PA report's BASE structure.
*/
#define leBroadcastManager_GetBroadcastSourceNumberOfSubgroups(broadcast_source) (broadcast_source->num_subgroups)

/*! \brief Sets when No Preference should be used for syncing to BIS.

    This can be used to specify that No Preference should be used when syncing to BIS.
    It is useful in cases where there is termination of the sync, as normally a locally terminated sync
    will cause the target BIS sync state to be set so that is doesn't sync back to the BIS again.
    By using this flag it can override the default behaviour so it will resync back to the BIS as soon as it is available.

    \param broadcast_source The broadcast source to use for the BIS sync.
    \param sync Set to TRUE to use No Preference for BIS sync. If FALSE there is no requirement to use No Preference.
*/
#define leBroadcastManager_SetSyncToBisNoPreference(broadcast_source, sync) (broadcast_source->sync_to_bis_no_preference = sync)

/*! \brief Gets if No Preference should be used for syncing to BIS.

    \param broadcast_source The broadcast source associated with the BIS sync.
    
    \return TRUE if No Preference should be used for BIS sync. If FALSE there is no requirement to use No Preference.
*/
#define leBroadcastManager_GetSyncToBisNoPreference(broadcast_source)    (broadcast_source->sync_to_bis_no_preference)

/*! \brief Sets if the broadcast audio chain should start muted.

    \param muted Set TRUE to start muted; FALSE otherwise.
*/
#define leBroadcastManager_SetStartMuted(muted) \
    do { \
        BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetStartMuted muted %d", (muted)); \
        le_broadcast_manager.start_muted = (muted); \
    } while (0)

/*! \brief Gets if the broadcast audio chain should start muted.

    \return TRUE to start muted; FALSE otherwise.
*/
#define leBroadcastManager_GetStartMuted()  (le_broadcast_manager.start_muted)

/*! \brief Sets the source ID to use for syncing to BIS.

    \param source_id The source ID to use for syncing to BIS.
*/
#define leBroadcastManager_SetTargetBisSourceId(source_id) \
    do { \
        BROADCAST_MANAGER_DATA_LOG("leBroadcastManager_SetTargetBisSourceId source_id %d", (source_id)); \
        le_broadcast_manager.target_bis_source_id = (source_id); \
    } while (0)

/*! \brief Gets the source ID to use for syncing to BIS.

    \return The source ID to use for syncing to BIS.
*/
#define leBroadcastManager_GetTargetBisSourceId()  (le_broadcast_manager.target_bis_source_id)

/*! \brief Set the address of the broadcast assistant.

    \param broadast_source The broadcast source to use for the assistant cid.
    \param tpaddr The resolved public typed_bdaddr of the broadcast assistant.
*/
#define LeBroadcastManager_SetAssistantAddress(broadcast_source, taddr) \
    do { \
        broadcast_source->assistant_address = (taddr); \
        BROADCAST_MANAGER_DATA_LOG("LeBroadcastManager_SetAssistantAddress broadcast_source=%p addr[%x %x:%x:%lx]", \
                broadcast_source, (taddr).type, \
                (taddr).addr.nap, (taddr).addr.uap, (taddr).addr.lap); \
    } while (0)

/*! \brief Get the address of the broadcast assistant.

    On the first connection the assistant address may be a random address
    if the assistant is using a RPA.

    \param broadcast_source The broadcast source to use for the assistant address.
    \return The typed_bdaddr of the broadcast assistant. This may be a random address.
*/
#define LeBroadcastManager_GetAssistantAddress(broadcast_source) broadcast_source->assistant_address

/*! \brief Reset the requested BIS sync state for a source.

    \param broadcast_source Source to reset the requested BIS sync state for.
*/
void LeBroadcastManager_ResetRequestedBisSyncState(broadcast_source_state_t *broadcast_source);

/*! \brief Set the requested BIS sync state for a source.

    \param broadcast_source Source to set the requested BIS sync state for.
    \param bis_sync_state BIS sync state to store.
*/
void LeBroadcastManager_SetRequestedBisSyncState(broadcast_source_state_t *broadcast_source, const le_broadcast_manager_bis_sync_state_t *bis_sync_state);

/*! \brief Get the requested BIS sync state for a source.

    \param broadcast_source Source to get the requested BIS sync state for.
    \return Pointer to the requested BIS sync state.
*/
const le_broadcast_manager_bis_sync_state_t *LeBroadcastManager_GetRequestedBisSyncState(broadcast_source_state_t *broadcast_source);

/*! \brief Get the requested BIS sync state for a source as a bitfield.

    \param broadcast_source Source to get the requested BIS sync state for.
    \return Bitfield representation of BIS sync state.
*/
uint32 LeBroadcastManager_GetRequestedBisSyncStateValue(broadcast_source_state_t *broadcast_source);

/*! \brief Get the PAST timeout.

    \return The timeout in milliseconds.
*/
#define LeBroadcastManager_GetPastTimeoutMs() (le_broadcast_manager.past_timeout)

/*! \brief Set the PAST timeout.

    \param timeout The timeout in milliseconds.
*/
#define LeBroadcastManager_SetPastTimeoutMs(timeout) (le_broadcast_manager.past_timeout = (timeout))

/*! \brief Get the Find Trains timeout.

    The Find Trains, aka 'self scan', timeout is the timeout used when the
    device is scanning for PA trains from broadcast sources.

    \return The find trains timeout in milliseconds.
*/
#define LeBroadcastManager_GetFindTrainsTimeout() (le_broadcast_manager.find_trains_timeout)

/*! \brief Set the Find Trains timeout.

    \param timeout Timeout in milliseconds.
*/
#define LeBroadcastManager_SetFindTrainsTimeout(timeout) (le_broadcast_manager.find_trains_timeout = (timeout))

/*! \brief Get the BASS source id for a broadcast source

    \param broadcast_source Pointer to the broadcast source state object to get the BASS source id for.

    \return The BASS source id, or SCAN_DELEGATOR_SOURCE_ID_INVALID if the broadcast source state pointer is not valid.
*/
#define LeBroadcastManager_SourceGetSourceId(broadcast_source)    ((broadcast_source) ? (broadcast_source)->source_id : SCAN_DELEGATOR_SOURCE_ID_INVALID)

/*! Set the periodic scan handle for a source. */
#define LeBroadcastManager_SourceGetPeriodicScanHandle(source)    ((source)->handle)

/*! Get the periodic scan handle for a source. */
#define LeBroadcastManager_SourceSetPeriodicScanHandle(source, handle)    ((source)->handle = handle)

#endif /* LE_BROADCAST_MANAGER_DATA_H_ */
/*! @} */