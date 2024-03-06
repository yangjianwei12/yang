/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup leabm
    \brief      LE Broadcast Manager interface with the BAP Scan Delegator role.
    @{
*/

#ifndef LE_BROADCAST_MANAGER_SCAN_DELEGATOR_H_
#define LE_BROADCAST_MANAGER_SCAN_DELEGATOR_H_

#include "le_broadcast_manager_data.h"

#include "gatt.h"
#include "scan_delegator_role.h"

/*! \brief Initialises the BAP Scan Delegator component.

    \param new_source The number of broadcast sources to support.
*/
void LeBroadcastManager_ScanDelegatorInit(uint8 number_broadcast_sources);

/*! \brief Removes the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    
    \return TRUE if the Remove Source was accepted. FALSE otherwise.
*/
le_bm_bass_status_t LeBroadcastManager_ScanDelegatorRemoveSourceState(uint8 source_id);

/*! \brief Gets if the Broadcast code is set for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    
    \return TRUE if the broadcast code is set. FALSE otherwise.
*/
bool LeBroadcastManager_ScanDelegatorIsBroadcastCodeSet(uint8 source_id);

/*! \brief Gets if PAST is available for the Broadcast Source.

    \param broadcast_source The Broadcast Source to test.
    
    \return TRUE if PAST is available. FALSE otherwise.
*/
bool LeBroadcastManager_ScanDelegatorIsPastAvailable(broadcast_source_state_t *broadcast_source);

/*! \brief Frees the memory contained within the scan_delegator_client_add_broadcast_source_t structure of the Add Source operation.

    \param client_add_source Pointer to the client add source command.
*/
void LeBroadcastManager_ScanDelegatorFreeSourceAddMemory(scan_delegator_client_add_broadcast_source_t *client_add_source);

/*! \brief Sets the PA Sync State for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    \param pa_sync_state The PA Sync State to set.
*/
void LeBroadcastManager_ScanDelegatorSetPaSyncState(uint8 source_id, scan_delegator_server_pa_sync_state_t pa_sync_state);

/*! \brief Gets the PA Sync State for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    
    \return The PA Sync State.
*/
scan_delegator_server_pa_sync_state_t LeBroadcastManager_ScanDelegatorGetPaSyncState(uint8 source_id);

/*! \brief Sets the BIS Sync State for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    \param bis_sync_state The BIS Sync State to set.
*/
void LeBroadcastManager_ScanDelegatorSetBisSyncState(uint8 source_id, const le_broadcast_manager_bis_sync_state_t *bis_sync_state);

/*! \brief Sets the BIS Sync Failed to Sync State for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
*/
void LeBroadcastManager_ScanDelegatorSetBisSyncStateFailedSync(uint8 source_id);

/*! \brief Sets the BIS Sync Not Synced State for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
*/
void LeBroadcastManager_ScanDelegatorSetBisSyncStateNoSync(uint8 source_id);

/*! \brief Gets the BIS Sync State for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    
    \return The BIS Sync State.
*/
uint32 LeBroadcastManager_ScanDelegatorGetBisSyncState(uint8 source_id);

/*! \brief Sets the BIG Encryption State for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    \param big_encryption The BIG Encryption State to set.
*/
void LeBroadcastManager_ScanDelegatorSetBigEncryptionState(uint8 source_id, scan_delegator_server_big_encryption_t big_encryption);

/*! \brief Gets the BIG Encryption State for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    
    \return The BIG Encryption State.
*/
scan_delegator_server_big_encryption_t LeBroadcastManager_ScanDelegatorGetBigEncryptionState(uint8 source_id);

/*! \brief Sets the BIG Encryption Bad Code condition for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
*/
void LeBroadcastManager_ScanDelegatorSetBigEncryptionBadCode(uint8 source_id);

/*! \brief Gets the address for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    
    \return The source address.
*/
typed_bdaddr LeBroadcastManager_ScanDelegatorGetSourceTypedBdaddr(uint8 source_id);

/*! \brief Gets the advertising SID for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    
    \return The advertising SID.
*/
uint8 LeBroadcastManager_ScanDelegatorGetSourceAdvSid(uint8 source_id);

/*! \brief Sets the advertising SID for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    \param source_adv_sid The advertising SID.
*/
void LeBroadcastManager_ScanDelegatorSetSourceAdvSid(uint8 source_id, uint8 source_adv_sid);

/*! \brief Gets the Broadcast ID for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    
    \return The Broadcast ID.
*/
uint32 LeBroadcastManager_ScanDelegatorGetSourceBroadcastId(uint8 source_id);

/*! \brief Sets the Broadcast ID for the Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    \param broadcast_id The Broadcast ID.
*/
void LeBroadcastManager_ScanDelegatorSetSourceBroadcastId(uint8 source_id, uint32 broadcast_id);

/*! \brief Write the Add Source sent by the client to a Broadcast Receive State characteristic with the supplied source ID.

    \param source_id Pointer which will be set to the source ID of the Broadcast Receive State characteristic that was added.
    \param new_source Pointer to the client Add Source command.

    \return The status of the operation.
*/
le_bm_bass_status_t LeBroadcastManager_ScanDelegatorWriteClientAddSource(uint8 *source_id, const scan_delegator_client_add_broadcast_source_t * new_source);

/*! \brief Write the Modify Source sent by the client to a Broadcast Receive State characteristic with the supplied source ID.

    \param source_id The source ID of the Broadcast Receive State characteristic.
    \param new_source Pointer to the client Modify Source command.
*/
void LeBroadcastManager_ScanDelegatorWriteClientModifySource(uint8 source_id, const scan_delegator_client_modify_broadcast_source_t * new_source);

/*! \brief Returns a copy of the Add Source command set by the client.

    \param add_broadcast_source The Add Source command to copy.
    
    \return Pointer to the copy of the Add Source command. The pointer memory must be freed after use.
*/
scan_delegator_client_add_broadcast_source_t * leBroadcastManager_ScanDelegatorCopyClientAddSource(const scan_delegator_client_add_broadcast_source_t * add_broadcast_source);

/*! \brief Gets the target PA/BIS sync state for the broadcast source identified by the source_id.

    The target PA/BIS sync state is the sync state that the device is aiming to acheive.
    This may be different from the PA or BIS sync state stored in the Broadcast Receive State characteristic
    if there is a pending operation.

    For example, a client might have told the server to sync to PA. The current PA sync will be 'no sync' until
    the sync is acheived, but the target PA sync would be updated to 'sync to PA'. If sync was acheived, only then would
    the Broadcast Receive State be updated to indicate 'synced to PA'.

    \param[in] source_id BASS source_id of the source to get the BIS sync state for.

    \return The requested BIS sync state of the source.
*/
scan_delegator_target_sync_state_t LeBroadcastManager_GetTargetSyncState(uint8 source_id);

/*! \brief Get the source state for broadcast source

    \param source_id Value of type uint8 to identify the broadcast source.
    \param source_state Pointer to the structure scan_delegator_server_get_broadcast_source_state_t to populate for the broadcast source.

    \return TRUE if source state was retrieved with success, FALSE otherwise.
*/
le_bm_bass_status_t leBroadcastManager_GetBroadcastSourceState(uint8 source_id, scan_delegator_server_get_broadcast_source_state_t * source_state);

/*! \brief Free the memory buffers used by a scan_delegator_source_state_t

    This function will only free any memory buffers pointed to by source_state.
    It will not attempt to free the source_state itself. That is the
    responsibility of the caller.

    \param source_state The source state to free memory for.
*/
void leBroadcastManager_FreeBroadcastSourceState(scan_delegator_server_get_broadcast_source_state_t * source_state);

#endif /* LE_BROADCAST_MANAGER_SCAN_DELEGATOR_H_ */
/*! @} */