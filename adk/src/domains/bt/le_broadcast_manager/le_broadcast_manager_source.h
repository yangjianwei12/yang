/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup leabm
    \brief      Manager for LE Broadcast Audio Sources.
    @{
*/

#ifndef LE_BROADCAST_MANAGER_SOURCE_H_
#define LE_BROADCAST_MANAGER_SOURCE_H_

#include "scan_delegator_role.h"
#include "le_broadcast_manager_data.h"

#include <connection.h>

#ifdef USE_SYNERGY
#include "bap_server_lib.h"
#else
#include "bap_server.h"
#endif

typedef enum
{
    /*! Timeout for PAST procedure */
    BROADCAST_MANAGER_INTERNAL_MSG_PAST_TIMEOUT,
    /*! Check of sync for all added broadcast sources */
    BROADCAST_MANAGER_INTERNAL_MSG_SOURCES_SYNC_CHECK,
    /*! Queued Add Source operation */
    BROADCAST_MANAGER_INTERNAL_MSG_ADD_SOURCE,
    /*! Resync to lost PA */
    BROADCAST_MANAGER_INTERNAL_MSG_RESYNC_TO_LOST_PA,
    /*! Un-mute the broadcast audio source. Used to synchronise when audio is heard on both earbuds. */
    BROADCAST_MANAGER_INTERNAL_UNMUTE_TIMEOUT
} broadcast_manager_internal_msg_id_t;


/*! \brief Initialises the LE Broadcast Manager handling of broadcast sources.

    Initialises the LE Broadcast Manager message handlers and the initial state of the stored broadcast sources.
 */
void LeBroadcastManager_SourceInit(void);

/*! \brief Handles the received broadcast source Add Source operation.

    The Add Source operation is received from a GATT client to provide information regarding a broadcast source.
    This operation is defined in the Broadcast Audio Scan Service (BASS).
    If the Add Source operation can be accepted then this function will call the LeBapScanDelegator_AddBroadcastSourceState API,
    to modify a Broadcast Receive State characteristic.
    If the Add Source operation cannot be accepted, then no Broadcast Receive State characteristic will be modified.

    \param new_source The broadcast source state to add.
    
*/
void LeBroadcastManager_SourceAdd(scan_delegator_client_add_broadcast_source_t * new_source);

/*! \brief Handles the received broadcast source Modify Source operation.

    The Modify Source operation is received from a GATT client to request to add or update Metadata for the Broadcast Source identified by the Source_ID,
    and to request the server to synchronize to, or to stop synchronization to, a PA and/or a BIS.
    This operation is defined in the Broadcast Audio Scan Service (BASS).
    If the Modify Source operation can be accepted then this function will update the Metadata in the Broadcast Receive State characteristic,
    or attempt to synchronize to, or to stop synchronization to, a PA and/or a BIS. 
    The LeBapScanDelegator_ModifyBroadcastSourceState API will be called to modify a Broadcast Receive State characteristic.
    If the Modify Source operation cannot be accepted, then no Broadcast Receive State characteristic will be modified.

    \param new_source The broadcast source state to modify.

    \return Status of the operation.
*/
le_bm_bass_status_t LeBroadcastManager_SourceModify(scan_delegator_client_modify_broadcast_source_t * new_source);

/*! \brief Handles the received broadcast source Remove Source operation.

    The Remove Source operation is received from a GATT client to request the server to remove information for a Broadcast Source identified 
    by the Source_ID in a Broadcast Receive State characteristic.
    This operation is defined in the Broadcast Audio Scan Service (BASS).
    If the Remove Source operation can be accepted then the LeBapScanDelegator_RemoveBroadcastSourceState API would be called,
    to clear the values in a Broadcast Receive State characteristic.
    If the Remove Source operation cannot be accepted, then no Broadcast Receive State characteristic will be modified.

    \param new_source The broadcast source state to remove.
    
*/
void LeBroadcastManager_SourceRemove(scan_delegator_client_remove_broadcast_source_t * new_source);

/*! \brief Called when the sync to PA procedure is completed.

    This function can be called:
    - When the PA sync procedure has completed due to the PAST procedure.
    - When a locally initiated attempt to sync to PA has been successful.

    \param sync The PA sync information.
    
*/
void LeBroadcastManager_SourceSyncedToPA(scan_delegator_periodic_sync_t * sync);

/*! \brief Called when a locally initiated attempt to sync to PA has failed.
    
*/
void LeBroadcastManager_SourceFailedSyncToPA(void);

/*! \brief Called when a locally initiated attempt to sync to PA has been cancelled.
    
*/
void LeBroadcastManager_SourceCancelledSyncToPA(bool success);

/*! \brief Handles the received broadcast source Set Broadcast Code operation.

    The Set Broadcast Code is received from a GATT client to provide a Broadcast_Code to the server to enable the server to decrypt an encrypted BIS.
    This operation is defined in the Broadcast Audio Scan Service (BASS).
    If the Broadcast Code for the specified broadcast source was expected, then the BIG_Encryption field of the Broadcast Receive State characteristic
    will be updated.

    \param code The broadcast code information.
*/
void LeBroadcastManager_SourceBroadcastCode(scan_delegator_client_broadcast_code_t * code);


/*! \brief Gets the BIS handle associated with an active Broadcast Audio stream.

    When synchronized to BIS, a BIS handle will exist for the synchronization.
    This function will return that BIS handle, but if no synchronization exists it will be a zero value.

    \param loc BIS location within source.

    \return The BIS handle associated with an active Broadcast Audio stream. 0 if no BIS handle exists.
 */
uint16 LeBroadcastManager_SourceGetBisStreamHandle(uint8 bis_loc);

/*! \brief Returns if a synchronization to BIS exists.
    
    \return TRUE if a synchronization to BIS exists. FALSE otherwise.
*/
#define LeBroadcastManager_SourceIsBisSync() (LeBroadcastManager_SourceGetBisStreamHandle(broadcast_manager_bis_location_left_or_stereo) != 0)

/*! \brief Called on receiving a PA report when synchronized to PA.
    
    \param sync_handle The PA sync handle.
    \param data_length_adv The length of the PA advert data.
    \param data_adv The PA advert data.
*/
void LeBroadcastManager_SourcePaReportReceived(uint16 sync_handle, uint16 data_length_adv, const uint8 * data_adv);

/*! \brief Called on receiving a BIGInfo report when synchronized to PA.
    
    \param sync_handle The PA sync handle.
    \param max_sdu The maximum size of an SDU.
    \param encryption The encryption status.
    \param iso_interval The interval for the audio stream (ISO)
*/
void LeBroadcastManager_SourceBigInfoReportReceived(uint16 sync_handle, uint16 max_sdu, 
                                                    uint8 encryption, uint16 iso_interval);

/*! \brief Called when synchronization to PA has been lost.
    
    \param sync_handle The PA sync handle.
*/
void LeBroadcastManager_SourcePaSyncLoss(uint16 sync_handle);

/*! \brief Called when synchronization to BIG has been lost.
    
    \param big_handle The BIG handle.
    \param bad_code TRUE indicates the sync was lost due to a bad broadcast code. FALSE otherwise.
*/
void LeBroadcastManager_SourceBigSyncLoss(uint8 big_handle, bool bad_code);

/*! \brief Returns the sample rate associated with an active audio stream.

    The sample rate will be retrieved from the BASE structure in the PA report when synchronized to PA.
    When BIS synchronization also exists, this function will return the sample rate for the broadcast associated with the
    active stream. But if no synchronization exists the sample rate will be a zero value.
    
    \return The sample rate associated with an active audio stream. 0 if no sample rate found.
*/
uint16 LeBroadcastManager_SourceGetAudioStreamSampleRate(void);

/*! \brief Returns the frame duration associated with an active audio stream.

    The frame duration will be retrieved from the BASE structure in the PA report when synchronized to PA.
    When BIS synchronization also exists, this function will return the frame duration for the broadcast associated with the active stream. But if no synchronization exists the frame duration will be a zero value.
    
    \return The frame duration associated with an active audio stream. 0 if no frame duration found.
*/
uint16 LeBroadcastManager_SourceGetAudioStreamFrameDuration(void);

/*! \brief Returns the octets per frame associated with an active audio stream.

    The octets per frame will be retrieved from the BASE structure in the PA report when synchronized to PA.
    When BIS synchronization also exists, this function will return the octets per frame for the broadcast associated with the active stream. But if no synchronization exists the SDU size will be a zero value.
    
    \return The SDU size associated with an active audio stream. 0 if no octets per frame found.
*/
uint16 LeBroadcastManager_SourceGetAudioStreamOctetsPerFrame(void);

/*! \brief Returns the presentation delay associated with an active audio stream.

    The presentation delay will be retrieved from the BASE structure in the PA report when synchronized to PA.
    When BIS synchronization also exists, this function will return the presentation delay for the broadcast associated with the active stream. But if no synchronization exists the presentation delay will be a zero value.
    
    \return The presentation delay associated with an active audio stream. 0 if no presentation delay found.
*/
uint32 LeBroadcastManager_SourceGetAudioStreamPresentationDelay(void);

/*! \brief Returns the codec frame values per SDU associated with an active audio stream.

    The codec frame values per SDU will be retrieved from the BASE structure in the PA report when synchronized to PA.
    When BIS synchronization also exists, this function will return the codec frame values per SDU for the broadcast associated with the active stream. But if no synchronization exists the codec frame values per SDU will be a zero value.
    
    \return The codec frame values per SDU associated with an active audio stream. 0 if no codec frame values per SDU found.
*/
uint8 LeBroadcastManager_SourceGetAudioStreamCodecFrameBlocksPerSdu(void);

/*! \brief Handles when audio or voice becomes connected.

*/
void LeBroadcastManager_SourceHandleAudioOrVoiceConnectedInd(void);

/*! \brief Handles when audio or voice becomes disconnected.

*/
void LeBroadcastManager_SourceHandleAudioOrVoiceDisconnectedInd(void);

/*! \brief Handles when an incoming call starts.

*/
void LeBroadcastManager_SourceHandleIncomingCallStarted(void);

/*! \brief Handles when an incoming call ends.

*/
void LeBroadcastManager_SourceHandleIncomingCallEnded(void);

/*! \brief Handles the BAP_SERVER_SETUP_DATA_PATH_CFM message.

    \param cfm Pointer to the BapServerSetupDataPathCfm message structure.

*/
void LeBroadcastManager_HandleSetupIsoDataPathCfm(const BapServerSetupDataPathCfm *cfm);

/*! \brief Handles the BAP_SERVER_ISOC_BIG_CREATE_SYNC_CFM message.

    \param cfm Pointer to the BapServerIsocBigCreateSyncCfm message structure.

*/
void LeBroadcastManager_HandleIsocBigCreateSyncCfm(const BapServerIsocBigCreateSyncCfm *cfm);

/*! \brief Handles the CL_DM_BLE_PERIODIC_SCAN_SYNC_TERMINATE_CFM message.

    \param cfm Pointer to the CL_DM_BLE_PERIODIC_SCAN_SYNC_TERMINATE_CFM message structure.

*/
void LeBroadcastManager_SourcePaSyncTerminateCfm(const CL_DM_BLE_PERIODIC_SCAN_SYNC_TERMINATE_CFM_T *cfm);

/*! \brief Handles the BROADCAST_MANAGER_INTERNAL_MSG_PAST_TIMEOUT message.

*/
void LeBroadcastManager_HandleInternalMsgPastTimeout(void);

/*! \brief Handles the BROADCAST_MANAGER_INTERNAL_MSG_SOURCES_SYNC_CHECK message.

*/
void LeBroadcastManager_HandleInternalMsgSourcesSyncCheck(void);

/*! \brief Handles the BROADCAST_MANAGER_INTERNAL_MSG_RESYNC_TO_LOST_PA message.

*/
void LeBroadcastManager_HandleInternalMsgResyncToLostPa(void);

/*! \brief Gets the source of the currently active Bis.

    \return Pointer to the currently active Bis. Null if none found.
*/
broadcast_source_state_t * LeBroadcastManager_GetSourceOfActiveBis(void);

/*! \brief Handles a pause received from the peer device.

    \param source_id ID assigned by the server to a Broadcast Receive State characteristic.

*/
void LeBroadcastManager_HandleReceivedPauseSource(uint8 source_id);

/*! \brief Handles a resume received from the peer device.

    \param source_id ID assigned by the server to a Broadcast Receive State characteristic.

*/
void LeBroadcastManager_HandleReceivedResumeSource(uint8 source_id);

/*! \brief Handles a sync to source received from the peer device.

    \param source_id ID assigned by the server to a Broadcast Receive State characteristic.

*/
void LeBroadcastManager_HandleReceivedSyncToSource(uint8 source_id);

/*! \brief Handles a stop confirm message from .the broadcast manager

*/
void LeBroadcastManager_HandleStopConfirm(void);


/*! \brief Get the typed Bluetooth address for a broadcast source 

    \param broadcast_source Source to retrieve address for

    \return The typed Bluetooth address
 */
typed_bdaddr leBroadcastManager_GetSourceTypedBdaddr(broadcast_source_state_t * broadcast_source);

/*! \brief Get the broadcast source state for a given source_id.

    \param source_id The BASS source Id of the source.
    \return The broadcast source state or NULL if no match found.
*/
broadcast_source_state_t * LeBroadcastManager_GetSourceById(uint8 source_id);

/*! \brief Initiates the procedure to check all added broadcast sources to see if syncing can be handled.

    Syncing operations are only handled for one broadcast source at a time.
    When the syncing operation completes, this function can be called to
    initiate the procedure to look if syncing can occur for another added broadcast sources.

 */
void LeBroadcastManager_InitiateSyncCheckAllSources(void);

/*! \brief Checks if the Adv SID and Broadcast ID match the ones from the active broadcast source.

    \param adv_sid The advertising SID to match
    \param broadcast_id The broadcast ID to match

    \return TRUE if both the adv_sid and broadcast_id match the ones from the active broadcast source. FALSE otherwise.
 */
bool LeBroadcastManager_DoesSidAndBroadcastIdMatchSource(broadcast_source_state_t *broadcast_source, uint8 adv_sid, uint32 broadcast_id);

/*! \brief Checks if the Adv SID and Bluetooth device address match the ones from the active broadcast source.

    \param adv_sid The advertising SID to match
    \param taddr The Bluetooth device address to match

    \return TRUE if both the adv_sid and Bluetooth device address match the ones from the active broadcast source. FALSE otherwise.
 */
bool LeBroadcastManager_DoesSidAndBdaddrMatchSource(broadcast_source_state_t *broadcast_source, uint8 adv_sid, const typed_bdaddr *taddr);

/*! \brief Stop scanning for periodic adverts being sent by a broadcast source.

    \param source_id Source ID to use when scanning for PA
    \param sync_to_pa Set to TRUE if sync to PA should be attempted after scanning has stopped. FALSE if no syncing should occur.

 */
void LeBroadcastManager_StopScanForPaSource(uint8 source_id, bool sync_to_pa);

/*! \brief Start sync to periodic advert using the supplied source ID.

    \param source_id Source ID to use when syncing to the periodic advert

 */
void LeBroadcastManager_StartSyncToPaSource(uint8 source_id);

/*! \brief Stores the current broadcast source address.

    \param current_addr The broadcast source address to store

 */
void LeBroadcastManager_StoreCurrentSourceAddr(const typed_bdaddr *current_addr);

/*! \brief Returns the current broadcast source address.

    \param broadcast_source Broadcast source to retrieve the address for
    
    \return The current broadcast source address

 */
typed_bdaddr LeBroadcastManager_GetCurrentSourceAddr(broadcast_source_state_t *broadcast_source);

/*! \brief Clears the current broadcast source address.

    \param broadcast_source Broadcast source to clear the address for

 */
void LeBroadcastManager_ClearCurrentSourceAddr(broadcast_source_state_t *broadcast_source);

/*! \brief Get the first broadcast source that is currently active.

    A broadcast source is considered 'active' if:
    - It is valid (i.e. it exists in the BASS server)
    - It is synced or syncing to PA.
    - It is synced or syncing to a BIS.

    \return Pointer to an active broadcast source or NULL if there are none.
*/
broadcast_source_state_t * LeBroadcastManager_SourceGetFirstActiveSource(void);

/*! \brief Ask the broadcast manager to switch to the given broadcast source.

    \param[in] broadcast_source The broadcast source to switch to.
*/
void LeBroadcastManager_SwitchToBroadcastSource(broadcast_source_state_t *broadcast_source);

/*! \brief Enables sending of PA train metadata to the clients registered in \ref LeAudioMessages_ClientRegister.
           PA metadata will be indicated using message \ref LE_AUDIO_BROADCAST_METADATA_PAYLOAD.
*/
void LeBroadcastManager_EnableMetadataNotification(bool enable);

/*! \brief Ask the broadcast manager to add a broadcast source.

    \param[out] source_id The Source ID of the added source.
    \param[in] new_source Information regarding the new broadcast source as defined in the
                          Bluetooth Broadcast Audio Scan Service specification.

    \return The status of the operation, #le_bm_bass_status_t
*/
le_bm_bass_status_t LeBroadcastManager_AddSource(uint8 *source_id, scan_delegator_client_add_broadcast_source_t *new_source);

/*! \brief Ask the broadcast manager to remove a broadcast source.

    \param[in] source_id The Source ID of the source to be removed.

    \return The status of the operation, #le_bm_bass_status_t
*/
le_bm_bass_status_t LeBroadcastManager_RemoveSource(uint8 source_id);

/*! \brief Configure a broadcast source to match advertised broadcasts by Bluetooth device address rather than by Broadcast ID

    \param[in] source_id The Source ID of the source to be configured.
    \param[in] taddr The Bluetooth device address to match.

    \return The status of the operation, #le_bm_bass_status_t
*/
le_bm_bass_status_t LeBroadcastManager_SourceSetMatchAddress(uint8 source_id, const typed_bdaddr *taddr);


#endif /* LE_BROADCAST_MANAGER_SOURCE_H_ */
/*! @} */
