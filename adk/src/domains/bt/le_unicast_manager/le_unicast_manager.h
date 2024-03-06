/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   le_unicast_manager  LE Unicast Manager
    @{
    \ingroup    bt_domain
    \brief      Header file for the Unicast manager.
*/

#ifndef LE_UNICAST_MANAGER_H_
#define LE_UNICAST_MANAGER_H_

#include <hci.h>
#include "device.h"
#include "bt_types.h"

#if defined(INCLUDE_LE_AUDIO_UNICAST)

#include <device.h>

#include "audio_sources_list.h"
#include "voice_sources_list.h"
#include "ltv_utilities.h"
#include "le_audio_messages.h"

#ifdef ENABLE_LE_AUDIO_FT_UPDATE
#define LC3_DECODER_VERSION_ID_LE_UM                                             LC3_DECODER_VERSION_FT
#else
#define LC3_DECODER_VERSION_ID_LE_UM                                             LC3_DECODER_VERSION_EPC
#endif


/*! Invalid CIS ID value. */
#define LE_INVALID_CIS_ID           ((uint8) 0xFF)

/*! Invalid CIS handle value. */
#define LE_INVALID_CIS_HANDLE       ((hci_connection_handle_t) LE_AUDIO_INVALID_ISO_HANDLE)

/*! Invalid ASE ID value.*/
#define LE_INVALID_ASE_ID           ((uint8) 0x00)

#define LeUnicastManager_IsHighPriorityBandwidthUser()    TRUE

/*! Predefined codec configuration used for PTS tests involving codec configuration initiated by unicast server.*/
typedef enum 
{
    LE_UM_TEST_CODEC_CONFIG_SET_8_1,
    LE_UM_TEST_CODEC_CONFIG_SET_8_2,
    LE_UM_TEST_CODEC_CONFIG_SET_16_1,
    LE_UM_TEST_CODEC_CONFIG_SET_16_2,
    LE_UM_TEST_CODEC_CONFIG_SET_24_1,
    LE_UM_TEST_CODEC_CONFIG_SET_24_2,
    LE_UM_TEST_CODEC_CONFIG_SET_32_1,
    LE_UM_TEST_CODEC_CONFIG_SET_32_2,
    LE_UM_TEST_CODEC_CONFIG_SET_441_1,
    LE_UM_TEST_CODEC_CONFIG_SET_441_2,
    LE_UM_TEST_CODEC_CONFIG_SET_48_1,
    LE_UM_TEST_CODEC_CONFIG_SET_48_2,
    LE_UM_TEST_CODEC_CONFIG_SET_48_3,
    LE_UM_TEST_CODEC_CONFIG_SET_48_4,
    LE_UM_TEST_CODEC_CONFIG_SET_48_5,
    LE_UM_TEST_CODEC_CONFIG_SET_48_6,
    LE_UM_TEST_CODEC_CONFIG_SET_MAX
} le_um_codec_config_set_t;

#ifdef INCLUDE_MIRRORING

/*! Mirror/Delegation type */
typedef enum
{
    le_um_cis_mirror_type_mirror,
    le_um_cis_mirror_type_delegate,
    le_um_cis_mirror_type_delegate_with_left_src_shared,
    le_um_cis_mirror_type_delegate_with_right_src_shared,
    le_um_cis_mirror_type_delegate_with_left_snk_shared,
    le_um_cis_mirror_type_delegate_with_right_snk_shared,
    le_um_cis_mirror_type_invalid
} le_um_cis_mirror_type_t;

#endif /* INCLUDE_MIRRORING */

typedef struct
{
    uint8_t min_flush_timeout;      /*<! Minimum flush time out in FT units */
    uint8_t max_flush_timeout;      /*<! Maximum flush time out in FT units */
    uint8_t max_bit_rate;           /*<! Maximum bitrate */
    uint8_t err_resilience;         /*<! Error resilience & downmix */
    uint8_t latency_mode;           /*<! should be 0 (don't change) */
} le_um_ft_info_t;


/*! \brief Initialises the LE Unicast Manager

    Initialises the LE Unicast manager, which initialises all components required for Unicast LE Audio
 */
bool LeUnicastManager_Init(Task init_task);

/*! \brief Check if LE audio is active.
 *
 *  \param target_device If not null, returns target device pointer if active.
 *
 *  \return TRUE if LE audio is active
 */
bool LeUnicastManager_GetLeAudioDevice(device_t *target_device);

#ifdef INCLUDE_MIRRORING

/*! \brief Inform unicast manager on role change.
 *
 *   \param is_primary Indicates new role is primary or not.
 */
void LeUnicastManager_RoleChangeInd(bool is_primary);

/*! \brief Called by mirror profile when it knows delegate/mirroring status
 *
 *  \param cis_handle CIS-handle for which MIRROR status is reported.
 *  \param status Status of mirroing, TRUE means succes and FALSE means failed and will be tried again later.
 */
void LeUnicastManager_CisMirrorStatus(hci_connection_handle_t cis_handle, bool status);

#endif /* INCLUDE_MIRRORING */

/*! \brief Get the device_t associated with an LE audio source.

    Get the device_t associated with the given LE unicast audio source. Only
    devices that have connected over LE and configured one or more ASEs to the
    codec_configured state are considered as LE unicast capable.

    \param source Audio source to get the device for.

    \return A valid device_t or NULL if no LE capable one is associated with
            the audio source.
*/
device_t LeUnicastManager_GetDeviceForAudioSource(audio_source_t source);

/*! \brief Get the device_t associated with an LE voice source.

    Get the device_t associated with the given LE unicast voice source. Only
    devices that have connected over LE and configured one or more ASEs to the
    codec_configured state are considered as LE unicast capable.

    \param source Voice source to get the device for.

    \return A valid device_t or NULL if no LE capable one is associated with
            the voice source.
*/
device_t LeUnicastManager_GetDeviceForVoiceSource(voice_source_t source);

/*! \brief Check the unicast streaming active.

    \return True if Unicast streaming is active
*/
bool LeUnicastManager_IsStreamingActive(void);

/*! \brief Check if Linkloss occured while streaming.

    \param handset_addr which got linklossed

    \return True if linkloss occured while streaming for this handset
*/
bool LeUnicastManager_IsLinklossWhileStreaming(const tp_bdaddr *handset_addr);

/*! \brief Get the Le Audio Unicast context.

    \return Unicast context
*/
uint16 LeUnicastManager_GetUnicastAudioContext(void);

/*! \brief Get CIS handles.

    \param handle_list_size Size of handle array.
    \param handles Returns CIS handles. If NULL or handle_list_size is 0, handle is not filled

    \return Number of valid CIS handles.
*/
uint8 LeUnicastManager_GetCisHandles(uint8 handle_list_size, hci_connection_handle_t *handles);

/*! \brief Send ASE release request

    \param cid The gatt connection id. If invalid CID is passed, then if any active BAP configuration associated,
               it will be used for search otherwise will return.
    \param ase_id The ASE id for which the ASE release request is to be sent.
*/
void LeUnicastManager_SendAseReleaseRequest(gatt_cid_t cid, uint8 ase_id);

/*! \brief Send ASE released request

    \param cid The gatt connection id. If invalid CID is passed, then if any active BAP configuration associated,
               it will be used for search otherwise will return.
    \param ase_id The ASE id for which the ASE released request is to be sent.
*/
void LeUnicastManager_SendAseReleasedRequest(gatt_cid_t cid, uint8 ase_id);

/*! \brief Send ASE disable request

    \param cid The gatt connection id. If invalid CID is passed, then if any active BAP configuration associated,
               it will be used for search otherwise will return.
    \param ase_id The ASE id for which the ASE disable request is to be sent.
*/
void LeUnicastManager_SendAseDisableRequest(gatt_cid_t cid, uint8 ase_id);

/*! \brief Send Ase Configure Codec Req

    \param cid The gatt connection id. If invalid CID is passed, then if any active BAP configuration associated,
               it will be used for search otherwise will return.
    \param ase_id The ASE id for which the ASE disable request is to be sent.
    \param config_set The config set \ref le_um_codec_config_set_t
*/
void LeUnicastManager_SendAseConfigureCodecReq(gatt_cid_t cid, uint8 ase_id, le_um_codec_config_set_t config_set);

/*! \brief Send Metadata Update Request 

    \param cid The gatt connection id. If invalid CID is passed, then if any active BAP configuration associated,
               it will be used for search otherwise will return.
    \param ase_id The ASE id for which the ASE  Metadata Update Request is to be sent.
*/
void LeUnicastManager_SendAseMetadataUpdate(gatt_cid_t cid, uint8 ase_id);

/*! \Return TRUE if Le Audio Unicast context is media type, otherwise FALSE.*/
#define LeUnicastManager_IsUnicastMediaActive() (audio_context_type_media == LeUnicastManager_GetUnicastAudioContext())

/*! \Return TRUE if Le Audio Unicast context is conversational type, otherwise FALSE.*/
#define LeUnicastManager_IsUnicastVoiceActive() (audio_context_type_conversational == LeUnicastManager_GetUnicastAudioContext())

#if defined(ENABLE_LE_AUDIO_FT_UPDATE) || defined(INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE)
/*! \brief Send Metadata Update Request for FT update

    \param cid The gatt connection id. If invalid CID is passed, then if any active BAP configuration associated,
               it will be used for search otherwise will return.
    \param ase_id The ASE id for which the ASE  Metadata Update Request is to be sent.
    \param ft_info pointer to Flush Timeout information structure

    \return TRUE if successful
*/
bool LeUnicastManager_SendAseVSMetadataUpdate(gatt_cid_t cid, uint8 ase_id, le_um_ft_info_t *ft_info);
#endif


#ifdef ENABLE_LE_AUDIO_FT_UPDATE
/*! \brief Enable downmix of mono to stereo on the source device

    \param TRUE if downmix is to be set on the remote device
*/
void LeUnicastManager_EnableSourceMix(bool set_remote_mix);
#endif
#else /* INCLUDE_LE_AUDIO_UNICAST */

#define LeUnicastManager_GetDeviceForAudioSource(source)    ((device_t)NULL); UNUSED(source)

#define LeUnicastManager_GetDeviceForVoiceSource(source)    ((device_t)NULL); UNUSED(source)

#endif /* INCLUDE_LE_AUDIO_UNICAST */

#ifdef INCLUDE_LE_STEREO_RECORDING
/*! \brief Check if Le Stereo Recoding is active.

    \return TRUE if le stereo recording is active false otherwise
*/
bool LeUnicastManager_IsLeStereoRecordingActive(void);
#else
#define LeUnicastManager_IsLeStereoRecordingActive(void)    (FALSE)
#endif

/*! \brief Register the Unicast manager client component with the Device Database Serialiser */
void LeUnicastManager_RegisterAsPersistentDeviceDataUser(void);

#endif /* LE_UNICAST_MANAGER_H_ */
/*! @} */