/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    adk_test_common Test APIs
\brief      Interface for common LE Audio specifc testing functions.
*/
#ifndef LE_AUDIO_TEST_H
#define LE_AUDIO_TEST_H

#ifdef INCLUDE_LE_AUDIO_UNICAST
#include "le_unicast_manager.h"
#include "le_unicast_manager_private.h"
#include "call_control_client.h"
#endif

#ifdef INCLUDE_LE_AUDIO_BROADCAST
#include "le_broadcast_manager_types.h"
#endif

/*! @{ */

/*! \brief Check if LE extended advertsing is enabled.

    Check the state of the le_advertising_manager to see if extended
    advertising is currently active.

    \return TRUE if extended advertising is active, FALSE othrwise.
*/
bool leAudioTest_IsExtendedAdvertisingActive(void);

/*! \brief Check if any LE Broadcast source is active.

    Active means are synced to, or in the process of syncing to,
    a PA and / or BIS.

    \return TRUE if any source is active; FALSE otherwise.
*/
bool leAudioTest_IsBroadcastReceiveActive(void);

/*! \brief Check if any LE Broadcast source is pa synced.

    \return TRUE if any source is pa synced; FALSE otherwise.
*/
bool leAudioTest_IsAnyBroadcastSourceSyncedToPa(void);

/*! \brief Check if any LE Broadcast source is synced to a BIS.

    \return TRUE if any source is BIS synced; FALSE otherwise.
*/
bool leAudioTest_IsAnyBroadcastSourceSyncedToBis(void);

/*! \brief Set the volume during LEA broadcast

    \param volume 0-255
    \return bool TRUE if the volume set request was initiated
                 else FALSE
*/
bool leAudioTest_SetVolumeForBroadcast(uint8 volume);

/*! \brief Set the mute state during LEA broadcast

    \param mute_state TRUE for mute, FALSE for unmute
    \return bool TRUE if the mute request was initiated
                 else FALSE
*/
bool leAudioTest_SetMuteForBroadcast(bool mute_state);

/*! \brief Pause receiving the broadcast stream
    \return bool TRUE if the request was initiated else FALSE.
*/
bool leAudioTest_PauseBroadcast(void);

/*! \brief Resume receiving the broadcast stream.
    \return bool TRUE if the request was initiated else FALSE.
*/
bool leAudioTest_ResumeBroadcast(void);

/*! \brief Query if the broadcast is paused.
    \return TRUE if paused.
*/
bool leAudioTest_IsBroadcastPaused(void);

/*! \brief Set the volume during LEA unicast music

    \param volume 0-255
    \return bool TRUE if the volume set request was initiated
                 else FALSE
*/
bool leAudioTest_SetVolumeForUnicastMusic(uint8 volume);

/*! \brief Set the mute state during LEA unicast music

    \param mute_state TRUE for mute, FALSE for unmute
    \return bool TRUE if the mute request was initiated
                 else FALSE
*/
bool leAudioTest_SetMuteForUnicastMusic(bool mute_state);

/*! \brief Set the volume during LEA unicast voice

    \param volume 0-255
    \return bool TRUE if the volume set request was initiated
                 else FALSE
*/
bool leAudioTest_SetVolumeForUnicastVoice(uint8 volume);

/*! \brief Set the mute state during LEA unicast voice

    \param mute_state TRUE for mute, FALSE for unmute
    \return bool TRUE if the mute request was initiated
                 else FALSE
*/
bool leAudioTest_SetMuteForUnicastVoice(bool mute_state);

/*! \brief Get the VCP volume of the Broadcast audio source

    \return VCP volume of Broadcast source, 0-255
*/
int leAudioTest_GetVolumeForBroadcast(void);

/*! \brief Get the VCP volume of the Unicast audio source

    \return VCP volume of Unicast audio source, 0-255
*/
int leAudioTest_GetVolumeForUnicastMusic(void);

/*! \brief Get the VCP volume of the Unicast Voice source

    \return VCP volume of Unicast voice source, 0-255
*/

int leAudioTest_GetVolumeForUnicastVoice(void);

#ifdef INCLUDE_LE_AUDIO_UNICAST

/*! \brief Increment the volume in steps during LEA unicast music
*/
void leAudioTest_UnicastAudioSinkVolumeUp(void);

/*! \brief Decrement the volume in steps during LEA unicast music
*/
void leAudioTest_UnicastAudioSinkVolumeDown(void);

/*! \brief Increment the volume in steps during LEA unicast voice
*/
void leAudioTest_UnicastVoiceSinkVolumeUp(void);

/*! \brief Decrement the volume in steps during LEA unicast voice
*/
void leAudioTest_UnicastVoiceSinkVolumeDown(void);

/*! \brief Send ASE release request

    \param cid The gatt connection id. If invalid CID is passed, then if any active BAP configuration associated,
               it will be used for search otherwise will return.
    \param ase_id The ASE id for which the ASE release request is to be sent.
*/
void leAudioTest_SendAseReleaseRequest(gatt_cid_t cid, uint8 ase_id);

/*! \brief Send ASE released request

    \param cid The gatt connection id. If invalid CID is passed, then if any active BAP configuration associated,
               it will be used for search otherwise will return.
    \param ase_id The ASE id for which the ASE released request is to be sent.
*/
void leAudioTest_SendAseReleasedRequest(gatt_cid_t cid, uint8 ase_id);

/*! \brief Send ASE Disable request

    \param cid The gatt connection id. If invalid CID is passed, then if any active BAP configuration associated,
               it will be used for search otherwise will return.
    \param ase_id The ASE id for which the ASE Disable request is to be sent.
*/
void leAudioTest_SendAseDisableRequest(gatt_cid_t cid, uint8 ase_id);

/*! \brief Send Ase Configure Codec Req

    \param cid The gatt connection id. If invalid CID is passed, then if any active BAP configuration associated,
               it will be used for search otherwise will return.
    \param ase_id The ASE id for which the ASE disable request is to be sent.
    \param config_set The config set \ref le_um_codec_config_set_t
*/
void leAudioTest_SendAseAseConfigureCodecReq(gatt_cid_t cid, uint8 ase_id, le_um_codec_config_set_t config_set);

/*! \brief Send Ase Metadata Update request

    \param cid The gatt connection id. If invalid CID is passed, then if any active BAP configuration associated,
               it will be used for search otherwise will return.
    \param ase_id The ASE id for which the ASE metadata update req is to be sent.
*/
void leAudioTest_SendAseMetadataUpdateReq(gatt_cid_t cid, uint8 ase_id);

/*! \brief Check if CCP is connected or not

    \return TRUE if CCP is connected
*/
bool leAudioTest_IsCcpConnected(void);

/*! \brief Check if Lea unicast is streaming
    \return True if Lea unicast is streaming.
*/
bool appTestIsLeaUnicastStreaming(void);

/*! \brief Get Lea unicast audio context
    \return Lea Audio context
*/
uint16 appTestLeaUnicastGetAudioContext(void);

/*! \brief Initiates request for reading CIS ISO link quality.

    \return TRUE if any request was initated
*/
bool leAudioTest_ReadUnicastCisLinkQuality(void);

/*! \brief Remove the isochronous data paths from a CIS
 *
 * \param cis_data CIS information for the isochronous data paths to be removed
*/
void leAudioTest_LeUnicastRemoveDataPaths(le_um_cis_t *cis_data);

#endif /* INCLUDE_LE_AUDIO_UNICAST */

/*! \brief Check if any handset is connected both BREDR and LE

    \return bool TRUE if a handset is found that has both a BREDR 
                 and LE connection
*/
bool leAudioTest_AnyHandsetConnectedBothBredrAndLe(void);

#ifdef INCLUDE_LE_AUDIO_BROADCAST

/*! \brief Get the PAST timeout.

    \return The PAST timeout in milliseconds.
*/
uint16 leAudioTest_GetPastTimeout(void);

/*! \brief Set the PAST timeout.

    \param timeout Timeout in milliseconds.
*/
void leAudioTest_SetPastTimeout(uint16 timeout);

/*! \brief Get the Find Trains timeout.

    The Find Trains, aka 'self scan', timeout is the timeout used when the
    device is scanning for PA trains from broadcast sources.

    \return The find trains timeout in milliseconds.
*/
uint16 leAudioTest_GetFindTrainsTimeout(void);

/*! \brief Set the Find Trains timeout.

    \param timeout Timeout in milliseconds.
*/
void leAudioTest_SetFindTrainsTimeout(uint16 timeout);

/*! \brief Get the Sync to PA train timeout.

    \return The sync to train timeout in milliseconds.
*/
uint16 leAudioTest_GetSyncToTrainTimeout(void);

/*! \brief Set the Sync to PA train timeout.

    \param timeout Timeout in milliseconds.
*/
void leAudioTest_SetSyncToTrainTimeout(uint16 timeout);

/*! \brief Enable sending of broadcast metadata to registered clients.

    \param enable TRUE for enable, FALSE for disable
*/
void leAudioTest_EnableBroadcastMetadataNotification(bool enable);

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

/*! \brief Use data populated in le_audio_test_pa_data and le_audio_test_pa_data_len for updating PA data.

    \return TRUE if the operation was successfult

    \note Expected broadcast reciever as well as broadcast transmitter enabled to verify this functionality
*/
bool leAudioTest_UpdateBroadcastMetadata(void);

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

/*! \brief Add a broadcast source to the local BASS server.
 *
 * \param advertising_sid The Advertising Set ID of the broadcast source.
 * \param broadcast_id The Broadcast ID of the broadcast source.
 * \param pa_sync #le_bm_pa_sync_t value determining how the server shall attempt to
 *                synchronize to the Periodic Advertising train as defined in the
 *                Bluetooth Broadcast Audio Scan Service specification.
 *
 *  \return The Source ID of the new source or 0 if the Add Source operation failed.
*/
uint8 leAudioTest_BassAddSource(uint8 advertising_sid, uint32 broadcast_id, le_bm_pa_sync_t pa_sync);

/*! \brief Modify a broadcast source on the local BASS server.
 *
 * \param source_id Identifies the source for which the Broadcast Code is to be set.
 * \param pa_sync #le_bm_pa_sync_t value determining how the server shall attempt to
 *                synchronize to the Periodic Advertising train as defined in the
 *                Bluetooth Broadcast Audio Scan Service specification.
 *
 *  \return The status of the operation.
*/
le_bm_bass_status_t leAudioTest_BassModifySource(uint8 source_id, le_bm_pa_sync_t pa_sync);

/*!
 * \brief Sets the Broadcast Code to enable the server to decrypt an encrypted stream.
 *
 * \param source_id Identifies the source for which the Broadcast Code is to be set.
 * \param code_string A UTF-8 string representing the Broadcast Code.  The string shall
 *                    be at least 4 octets and not more than 16 octets as defined in
 *                    the Bluetooth Core Specification Volume 3, Part C, Section 3.2.6.1
 *
 *  \return The status of the operation.
 */
le_bm_bass_status_t leAudioTest_BassSetBroadcastCode(uint8 source_id, const char *code_string);

/*!
 * \brief Remove a broadcast source from the local BASS server.
 *
 * \param source_id Identifies the source which is to be removed.
 *
 *  \return The status of the operation.
 */
le_bm_bass_status_t leAudioTest_BassRemoveSource(uint8 source_id);

/*!
 * \brief Start a test self-scan for nearby Broadcast Sources
 *
 * Found sources are logged in the apps1 debug log.
 *
 * \param timeout Time period, in milliseconds, after which the scan will end.
 * \param broadcast_id Filter discovered sources that match this broadcast_id.
 *                     Not currently implemented.
 */
void leAudioTest_SelfScanStart(uint32 timeout, uint32 broadcast_id);

/*!
 * \brief Stop the test self-scan.
 *
 * If the test self-scan is not currently running then this does nothing.
 */
void leAudioTest_SelfScanStop(void);

/*!
 * \brief Stop all self-scans.
 */
void leAudioTest_SelfScanStopAll(void);

/*! \brief Configure a broadcast source to match advertised broadcasts by Bluetooth device
 *         address rather than by Broadcast ID
 *
 *  \param source_id The Source ID of the source to be configured.
 *  \param taddr The Bluetooth device address to match.
 *
 *  \return The status of the operation.
*/
le_bm_bass_status_t leAudioTest_BassSetSourceMatchAddress(uint8 source_id, typed_bdaddr *taddr);

/*!
 * \brief Send a request to get the current global LE scan parameters.
 */
void leAudioTest_GetGlobalLeScanParameters(void);

#endif /* INCLUDE_LE_AUDIO_BROADCAST */

/*! @} */

#endif // LE_AUDIO_TEST_H
