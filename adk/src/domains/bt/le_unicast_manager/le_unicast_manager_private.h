/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_unicast_manager
    \brief      Private types and functions for le_unicast_manager.
    @{
*/

#ifndef LE_UNICAST_MANAGER_PRIVATE_H_
#define LE_UNICAST_MANAGER_PRIVATE_H_

#include "le_unicast_manager.h"

#include "unicast_server_role.h"
#include "gatt_ascs_server.h"
#include "gatt_pacs_server.h"
#include "multidevice.h"
#include "unicast_server_role.h"
#include "ltv_utilities.h"
#include "pacs_utilities.h"
#include "kymera.h"
#include "bap_server_prim.h"

#define LE_UM_CIS_DIRECTION_DL      LE_AUDIO_ISO_DIRECTION_DL
#define LE_UM_CIS_DIRECTION_UL      LE_AUDIO_ISO_DIRECTION_UL
#define LE_UM_CIS_DIRECTION_BOTH    LE_AUDIO_ISO_DIRECTION_BOTH

/*! Number of Supported CIS's  */
#ifndef LE_UM_MAX_CIS
#define LE_UM_MAX_CIS               (2u)
#endif

/*! Number of Supported Sinks */
#define LE_MAX_SINK_ASES            (2u)

/*! Number of Supported Sources */
#define LE_MAX_SRC_ASES             (2u)

/*! Number of Supported Connections */
#define LE_MAX_UNICAST_INSTANCES (2)

/*! Unicast manager Log functionality */
#define UNICAST_MANAGER_LOG         DEBUG_LOG
#define UNICAST_MANAGER_WARN        DEBUG_LOG_WARN

/*! Codec set static parameters, used for testing codec configuration initiated from unicast server.*/
#define LE_UM_TEST_CODEC_SET_RETRANSMISSION_NUMBER                   ((uint8) 2)
#define LE_UM_TEST_CODEC_SET_TRANSPORT_LATENCY_MAX                   ((uint16) 0x0FA0)
#define LE_UM_TEST_CODEC_SET_PRESENTATION_DELAY_MIN_US               ((uint32) 10000)
#define LE_UM_TEST_CODEC_SET_PRESENTATION_DELAY_MAX_US               ((uint32) 40000)
#define LE_UM_TEST_CODEC_SET_PREFERRED_PRESENTATION_DELAY_MIN_US     ((uint32) 10000)
#define LE_UM_TEST_CODEC_SET_PREFERRED_PRESENTATION_DELAY_MAX_US     ((uint32) 40000)

/*! BAP Sampling Frequency used for testing codec configuration initiated from unicast server.*/
#define LE_UM_TEST_SAMPLING_FREQUENCY_8kHz                           ((uint8) 0x01)
#define LE_UM_TEST_SAMPLING_FREQUENCY_11_025kHz                      ((uint8) 0x02)
#define LE_UM_TEST_SAMPLING_FREQUENCY_16kHz                          ((uint8) 0x03)
#define LE_UM_TEST_SAMPLING_FREQUENCY_22_050kHz                      ((uint8) 0x04)
#define LE_UM_TEST_SAMPLING_FREQUENCY_24kHz                          ((uint8) 0x05)
#define LE_UM_TEST_SAMPLING_FREQUENCY_32kHz                          ((uint8) 0x06)
#define LE_UM_TEST_SAMPLING_FREQUENCY_44_1kHz                        ((uint8) 0x07)
#define LE_UM_TEST_SAMPLING_FREQUENCY_48kHz                          ((uint8) 0x08)
#define LE_UM_TEST_SAMPLING_FREQUENCY_88_200kHz                      ((uint8) 0x09)
#define LE_UM_TEST_SAMPLING_FREQUENCY_96kHz                          ((uint8) 0x0A)
#define LE_UM_TEST_SAMPLING_FREQUENCY_176_420kHz                     ((uint8) 0x0B)
#define LE_UM_TEST_SAMPLING_FREQUENCY_192kHz                         ((uint8) 0x0C)
#define LE_UM_TEST_SAMPLING_FREQUENCY_384kHz                         ((uint8) 0x0D)

/*! Bap defined Frame durations used for testing codec configuration initiated from unicast server */
#define LE_UM_TEST_FRAME_DURATION_7P5MS                              ((uint8) 0x00)
#define LE_UM_TEST_FRAME_DURATION_10MS                               ((uint8) 0x01)

/*! Max supported audio stream per direction */
#define LE_UM_SINK_AUDIO_STREAMS_MAX                    (2u)
#define LE_UM_SOURCE_AUDIO_STREAMS_MAX                  (2u)
#define LE_UM_SOURCE_AUDIO_STREAMS_SHARED               (1u)
#define LE_UM_SOURCE_AUDIO_NO_STREAMS                   (0u)

/*! Metadata static parameters, used for testing update metadata initiated from unicast server.*/
#define LE_UM_TEST_METADATA_LEN                        ((uint8) 0x04)
#define LE_UM_TEST_STREAMING_AUDIO_CONTEXT_LENGTH      ((uint8) 0x03)
#define LE_UM_TEST_STREAMING_AUDIO_CONTEXT_TYPE        ((uint8) 0x02)


#define LE_UM_PREFERED_MIN_FLUSH_TIMEOUT 0x1
#define LE_UM_PREFERED_MAX_FLUSH_TIMEOUT 0x4
#define LE_UM_PREFERED_MAX_BIT_RATE      0x96
#define LE_UM_PREFERED_ERR_RESILIENCE    0x00
#define LE_UM_PREFERED_LATENCY_MODE      0x0

#define LE_UM_ENABLE_REMOTE_STEREO_DOWNMIX 0x80

#define LE_UM_VENDOR_SPECIFIC_METADATA_TYPE 0xFF
#define LE_UM_FTINFO_VS_METADATA_LENGTH     0x0a
#define LE_UM_FTINFO_COMPANY_ID_QCOM_LOW    0x0A
#define LE_UM_FTINFO_COMPANY_ID_QCOM_HIGH   0x00
#define LE_UM_FTINFO_METDATA_LENGTH         0x06

#define LE_UM_METADATA_LTV_TYPE_FT_CURRENT_SETTINGS  0xFD
#define LE_UM_METADATA_LTV_TYPE_FT_REQUESTED_SETINGS 0xFE

#define LE_UM_APTX_DEFAULT_FRAME_DURATION   (10000)

#ifdef INCLUDE_STEREO
#define LE_UM_APTX_LITE_DEFAULT_FRAME_DURATION    (5000)
#else
#define LE_UM_APTX_LITE_DEFAULT_FRAME_DURATION    (6250)
#endif

/*! Messages used internally only in LE Unicast Manager. */
typedef enum
{
    /*! Invalid message */
    LE_UM_INTERNAL_INVALID_MSG = -1,

    /*! Message indicating CIS DISCONNECTED due to linkloss for instance 1*/
    LE_UM_INTERNAL_CIS_LINKLOSS_CONFIRMATION_INST1,

    /*! Message indicating CIS DISCONNECTED due to linkloss for instance 2*/
    LE_UM_INTERNAL_CIS_LINKLOSS_CONFIRMATION_INST2,
} le_um_internal_msg_t;

/*! ASE states that are going to be handled by the Unicast manager*/
typedef enum
{
    /*! ASE is in idle state */
    le_um_ase_state_idle,
    /*! ASE is in enabling state. ASE moves in this state when the unicast manager recieves ASE enable ind */
    le_um_ase_state_enabling,
    /*! ASE is in streaming state. ASE moves in this state when the cis is established for the ASE */
    le_um_ase_state_streaming,
    /*! ASE is in routed state. ASE moves in this state when the audio is routed / chain is setup for Sink ASE or ready_to recieve is recieved for Source ASE */
    le_um_ase_state_routed,
    /*! ASE is in disabling state. The source ASE moves in this state when the unicast manager recieves ASE disable ind */
    le_um_ase_state_disabling
} le_um_ase_state_t;

/*! states that represents what the Unicast manager is doing */
typedef enum
{
    le_um_state_idle,
    le_um_state_preparing,
    le_um_state_streaming,
    le_um_state_disabling
} le_um_state_t;

/*! Audio locations */
typedef enum
{
    le_um_audio_location_left_sink,
    le_um_audio_location_left_source,
    le_um_audio_location_right_sink,
    le_um_audio_location_right_source,
    le_um_audio_location_max
} le_um_audio_location_t;

/*! CIS Link states */
typedef enum
{
    le_um_cis_state_free,
    le_um_cis_state_established,
    le_um_cis_state_data_path_ready,
    le_um_cis_state_stale
} le_um_cis_state_t;

/*! Isochronous CIS Information */
typedef struct
{
    /*! CIS identifier */
    uint8                       cis_id;

    uint8                       dir;

    uint8                       pending_data_cfm;

    /*! Isochronous handle for this CIS */
    hci_connection_handle_t     cis_handle;

    /*! CIS state */
    le_um_cis_state_t           state;

#ifdef ENABLE_LEA_CIS_DELEGATION
    /*! Is CIS Delegated/Mirrored */
    bool                        is_cis_delegated:1;
    bool                        is_mirroring_attempted:1; /* Indicates if delegation was attempted */
#endif
} le_um_cis_t;

/*! The current state of an ASE. */
typedef struct
{
    /*! Identifier of the ASE in enabling/streaming state  */
    uint8                       ase_id;

    /*! ASE direction */
    uint8                       direction;

    /*! VS Codec Version */
    uint8                       codec_version;

    /*! Indicates status of Ready-To-Receive */
    bool                        rtr_status;

    /*! Present state of the ASE  */
    le_um_ase_state_t           state;

    /*! Claimed audio context */
    uint16                      audio_context;

    /*! Associated CIS information, valid only after CIS is established */
    le_um_cis_t                *cis_data;

    /*! Configured Codec information for this ASE  */
    const BapServerAseCodecInfo *codec_info;

    /*! Configured QoS information for this ASE  */
    const BapServerAseQosInfo   *qos_info;

    /*! Flush time out information */
    le_um_ft_info_t               *ft_info;

    /*! Content Control identifier */
    uint8                       ccid;
} le_um_ase_t;

/*! \brief Unicast manager context structure. */
typedef struct
{
    /*! GATT connection identifier */
    gatt_cid_t cid;

    /*! Current active audio context */
    AudioContextType            audio_context;

#ifdef INCLUDE_MIRRORING
    /*! Mirroring type */
    le_um_cis_mirror_type_t     mirror_type;
#endif

    /*! List of ASE's in Enabling/Streaming state */
    le_um_ase_t                 ase[le_um_audio_location_max];

    /*! List of Isochronous Connection information */
    le_um_cis_t                 cis[LE_UM_MAX_CIS];

    /*! The state of the Unicast audio source */
    source_state_t              source_state;
} le_um_instance_t;

/*! \brief Unicast manager task data. */
typedef struct
{
    /*! Task */
    TaskData                    task_data;

    /*! Current active audio context */
    uint16                      audio_context;

    /*! Anc mode before starting stereo recording */
    bool                        anc_mode_state_enabled;

    /*! Unicast Manager context information */
    le_um_instance_t             le_unicast_instances[LE_MAX_UNICAST_INSTANCES];

    /*! Audio interface state */
    source_state_t              audio_interface_state;
} le_um_task_data_t;


/*! Unicast Manager Task Data */
extern le_um_task_data_t le_unicast_taskdata;

/*! Returns the Unicast Manager taskdata */
#define LeUnicastManager_GetTaskData()         (&le_unicast_taskdata)

/*! Returns the Unicast Manager task */
#define LeUnicastManager_GetTask()            (&le_unicast_taskdata.task_data)

/*! Returns TRUE if the context is of audio (media/stereo recording) type */
#define LeUnicastManager_IsContextOfTypeMedia(context) (context != AUDIO_CONTEXT_TYPE_COVERSATIONAL && \
                                                        context != AUDIO_CONTEXT_TYPE_UNKNOWN)

/*! Returns TRUE if the context is of conversational type */
#define LeUnicastManager_IsContextTypeConversational(context) (context == AUDIO_CONTEXT_TYPE_COVERSATIONAL)

/*! Returns TRUE if the context type is live audio */
#define LeUnicastManager_IsContextTypeLive(context) (context == AUDIO_CONTEXT_TYPE_LIVE)

/*! Returns TRUE if the context type is Gaming mode */
#define LeUnicastManager_IsContextTypeGaming(context) (context == AUDIO_CONTEXT_TYPE_GAME)

/*! Returns TRUE if CIS is either in established OR datapath created state */
#define LeUnicastManager_IsCisEstablished(cis_state)   ((cis_state == le_um_cis_state_established) || \
                                                        (cis_state == le_um_cis_state_data_path_ready))

#ifdef ENABLE_LEA_CIS_DELEGATION
#define leUnicastManager_IsDelegatedCis(cis_data) ((cis_data)->is_cis_delegated)
#else
#define leUnicastManager_IsDelegatedCis(cis_data) (FALSE)
#endif

/*! Returns TRUE if ASE is either in enabling state or streaming state */
#define LeUnicastManager_IsAseActive(ase)   (((ase)->state != le_um_ase_state_idle) && \
                                             ((ase)->state != le_um_ase_state_disabling))

/*! Returns TRUE if ASE is routed state */
#define LeUnicastManager_IsAseRouted(ase)   (((ase)->state == le_um_ase_state_routed))

/*! Returns TRUE if both source and sink ASE is either in enabling state or streaming state */
#define LeUnicastManager_IsBothSourceAndSinkAseActive(inst)  ((LeUnicastManager_IsAseActive(LeUnicastManager_InstanceGetLeftSinkAse((inst))) ||  \
                                                              LeUnicastManager_IsAseActive(LeUnicastManager_InstanceGetRightSinkAse((inst)))) && \
                                                             (LeUnicastManager_IsAseActive(LeUnicastManager_InstanceGetLeftSourceAse((inst))) || \
                                                              LeUnicastManager_IsAseActive(LeUnicastManager_InstanceGetRightSourceAse((inst)))))

#define LeUnicastManager_IsSourceAseActive(inst)  (LeUnicastManager_IsAseRouted(LeUnicastManager_InstanceGetLeftSourceAse((inst))) ||  \
                                                  LeUnicastManager_IsAseRouted(LeUnicastManager_InstanceGetRightSourceAse((inst))))

#define LeUnicastManager_IsBothSourceAseActive(inst)  (LeUnicastManager_IsAseActive(LeUnicastManager_InstanceGetLeftSourceAse((inst))) && \
                                                        LeUnicastManager_IsAseActive(LeUnicastManager_InstanceGetRightSourceAse((inst))))

#define LeUnicastManager_IsCisMatches(qos_info1, qos_info2) ((qos_info1)->cigId == (qos_info2)->cigId && (qos_info1)->cisId == (qos_info2)->cisId)

#define LeUnicastManager_GetAudioLocation(codec_info) \
    BapServerLtvUtilitiesGetAudioChannelAllocation((codec_info)->infoFromServer.codecConfiguration, \
                                                   (codec_info)->infoFromServer.codecConfigurationLength)

#define LeUnicastManager_GetSampleRate(codec_info) \
    BapServerLtvUtilitiesGetSampleRate((codec_info)->infoFromServer.codecConfiguration, \
                                                (codec_info)->infoFromServer.codecConfigurationLength)

#define LeUnicastManager_GetCodecFrameBlocksPerSdu(codec_info) \
    BapServerLtvUtilitiesGetCodecFrameBlocksPerSdu((codec_info)->infoFromServer.codecConfiguration, \
                                                   (codec_info)->infoFromServer.codecConfigurationLength)

#define LeUnicastManager_GetFramelength(max_sdu_size, blocks_per_sdu, audio_location_mask) \
    LeBapPacsUtilities_Lc3GetFrameLength(max_sdu_size, blocks_per_sdu, audio_location_mask)

/*! Returns TRUE if Audio Context has unspecified bit set */
#define LeUnicastManager_AudioContextHasUnspecifiedContext(context) (context & AUDIO_CONTEXT_TYPE_UNSPECIFIED)

/*! Returns TRUE if Audio Context is Unknown */
#define LeUnicastManager_IsContextOfTypeUnknown(context) (context == AUDIO_CONTEXT_TYPE_UNKNOWN)

/*! Returns TRUE if Audio Context has more then one context */
#define LeUnicastManager_AudioContextHasMoreThanOneContext(context) (context & (context - 1))

/*! Returns the CIS link loss confirmation timeout */
#define LeUnicastManager_CisLinklossConfirmationTimeout()       (5000u)

#ifdef INCLUDE_LE_APTX_ADAPTIVE
#define LeUnicastManager_isVSAptXAdaptive(info)  ((info->codecId.codingFormat == 0xff) && \
                                                  (info->codecId.companyId == VS_METADATA_COMPANY_ID_QUALCOMM) &&  \
                                                  ((info->codecId.vendorSpecificCodecId == APTX_ADAPTIVE_VS_CODEC_ID) ))
#else
#define LeUnicastManager_isVSAptXAdaptive(info)   (FALSE)
#endif

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
#define LeUnicastManager_isVSAptxLite(info)  ((info->codecId.codingFormat == PACS_VENDOR_CODEC_ID) && \
                                              (info->codecId.companyId == VS_METADATA_COMPANY_ID_QUALCOMM) && \
                                              (info->codecId.vendorSpecificCodecId == APTX_LITE_VS_CODEC_ID))
#else
#define LeUnicastManager_isVSAptxLite(info)   (FALSE)
#endif

/*! \brief Get the ASEs based on the side. In case of single Microphone path, it will give data from other side

    \param inst        The unicast manager instance to get the ASEs for.
    \param side       The multidevice side from where we need to retrieve the Sink and Source ASE's.
    \param sink_ase   This will be pointed to the Sink ASE matching the side
    \param source_ase This will be pointed to the Source ASE if available. This can be for given side (dual microphone)
                      or other side (single microphone)
*/
void LeUnicastManager_GetAsesForGivenSide(le_um_instance_t *inst, multidevice_side_t side, le_um_ase_t **sink_ase, le_um_ase_t **source_ase);

#ifdef INCLUDE_CIS_MIRRORING
/*! \brief Get the Shared ASEs In case of CIS mirroring, it will give data from other side

    \param inst        The unicast manager instance to get the ASEs for.
    \param sink_ase   This will be pointed to the Sink ASE irrespective of side it is enabled on.
    \param source_ase This will be pointed to the Source ASE if available. This can be for given side (dual microphone)
                      or other side (single microphone)
*/
void LeUnicastManager_GetSharedAses(le_um_instance_t *inst, le_um_ase_t **sink_ase, le_um_ase_t **source_ase);
#endif

/*! \brief Get the ASEs (audio location data) based on the side, doesn't consider single microphone case

    \param inst        The unicast manager instance to get the ASEs for.
    \param side       The multidevice side from where we need to retrieve the Sink and Source ASE's.
    \param sink_ase   This will be pointed to the Sink ASE matching the side
    \param source_ase This will be pointed to the Source ASE matching the side.
*/
void leUnicastManager_GetAseFromSide(le_um_instance_t *inst, multidevice_side_t side, le_um_ase_t **sink_ase, le_um_ase_t **source_ase);

/*! \brief Checks if any ASE is enabling/streaming state

    \param inst  The unicast manager instance to check the ASE state for.

    \return bool  Returns TRUE if any of the ASE is enabled/streaming state.
*/
bool LeUnicastManager_IsAnyAseEnabled(le_um_instance_t *inst);

/*! \brief Checks if required CIS are in connected state

    \param inst  The unicast manager instance to check the CIS state.
    \param data_path_required     Additionally check if data path setup confirmations are received from all established CISes

    \return bool  Returns TRUE if required CIS are in connected state.
*/
bool LeUnicastManager_IsAllCisConnected(le_um_instance_t *inst, bool data_path_required);

/*! \brief Check if the Unicast manager is setting up a audio session now

    \param inst  The unicast manager instance to get the state for.

    \return bool  TRUE if a audio session is being setup else return FALSE(idle/streaming)
*/
le_um_state_t LeUnicastManager_GetState(le_um_instance_t *inst);

/*! \brief Restore audio context back to PACS service.

    \param direction     The direction for which the context has to be restored.
    \param audio_context The audio context that needs to be restored.
*/
void LeUnicastManager_RestoreAudioContext(GattAscsAseDirection direction, uint16 audio_context);

/*! \brief Claim sink audio context from PACS Service

    \param audio_context The sink audio context that needs to be claimed.
*/
void LeUnicastManager_ClaimSinkAudioContext(uint16 audio_context);

/*! \brief Restore sink audio context to PACS Service

    \param audio_context The sink audio context that needs to be restored.
*/
void LeUnicastManager_RestoreSinkAudioContext(uint16 audio_context);

/*! \brief Claim source audio context from PACS Service

    \param audio_context The source audio context that needs to be claimed.
*/
void LeUnicastManager_ClaimSourceAudioContext(uint16 audio_context);

/*! \brief Restore source audio context to PACS Service

    \param audio_context The source audio context that needs to be restored.
*/
void LeUnicastManager_RestoreSourceAudioContext(uint16 audio_context);

/*! Handler for BAP ASE enable */
bool leUnicastManager_AseEnabled(gatt_cid_t cid, const AseMetadataType *ase_metadata, AudioContextType audio_context);

/*! Handler for BAP CIS establishment*/
void leUnicastManager_CisEstablished(gatt_cid_t cid, uint8 cis_id, hci_connection_handle_t cis_handle, uint8 dir);

/*! Establishes data path if required conditions are met */
void leUnicastManager_CheckAndEstablishDataPath(le_um_instance_t *inst, bool publish_cis_connected);

/*! Handler for BAP CIS disconnection */
void leUnicastManager_CisDisconnected(uint16 cis_handle);

/*! Handler for BAP ASE release */
void leUnicastManager_AseReleased(gatt_cid_t cid, uint8 aseId);

/*! Handler for BAP to execute "Receiver ready" for a source ASE */
void leUnicastManager_AseReceiverReady(gatt_cid_t cid, uint8 aseId);

/*! Handler for BAP ASE disable */
void leUnicastManager_AseDisabled(gatt_cid_t cid, uint8 aseId);

/*! Handler for BAP ASE to execute "Receiver stop ready" for a source ASE */
void leUnicastManager_AseReceiverStop(gatt_cid_t cid, uint8 aseId);

/*! Handler for BAP Metadata update when ASE in "enabling" OR "streaming" */
bool leUnicastManager_UpdateMetadata(gatt_cid_t cid, const AseMetadataType *ase_metadata);

/*! Handler for BAP Client configuration information retrieval from NVM storage */
void * leUnicastManager_RetrieveClientConfig(gatt_cid_t cid);

/*! Handler for BAP Client configuration information preservation in NVM storage */
void leUnicastManager_StoreClientConfig(gatt_cid_t cid, void * config, uint8 size);

/*! Handler for disconnecting BAP LE-ACL connection  */
void leUnicastManager_CidDisconnected(gatt_cid_t cid);

/*! Handler for BAP isochronous data paths creation for a Cis Link */
void leUnicastManager_DataPathCreated(hci_connection_handle_t cis_handle);

/*! Handler for BAP isochronous data paths removal for a CIS Link */
void leUnicastManager_DataPathRemoved(hci_connection_handle_t cis_handle);

/*! Utility to determine the associated stream type on device type and ase configuration */
appKymeraLeStreamType leUnicastManager_DetermineStreamType(le_um_ase_t *ase, le_um_ase_t *ase_r);

/*! Utility to determine the associated codec type based on ase configuration */
appKymeraLeAudioCodec leUnicastManager_GetCodecType(const BapServerAseCodecInfo *codec_info);

/*! \brief Validate the flush timeout settings coming from the source device
    \param cid The gatt connection id
    \param ase_id The ASE id for which the ASE  Metadata Update Request is to be sent.
    \param metadata Metadata structure received from the source
    \param metadataLength Length of metadata structure
    \return TRUE if metadata valid and stored
    */
bool leUnicastManager_ValidateFTInfo(gatt_cid_t cid, uint8 ase_id, uint8* metadata, uint8 metadataLength);

uint8 leUnicastManger_DetermineCodecVersion(const BapServerAseCodecInfo *codec_info);

/*! \brief Get BT device from the source */
device_t leUnicastManager_GetBtAudioDevice(audio_source_t source);

/*! \brief Get multidevice side from ASE

    \param inst The unicast manager instance that the ASE belongs to.
    \param ase ASE whose side is to be determined.

    \return multidevice_side_t, side with which this ASE is associated.
*/
multidevice_side_t leUnicastManager_GetSideFromAse(le_um_instance_t *inst, le_um_ase_t *ase);

/*! \brief Executes receiver ready request for given sink ASE if conditions are met

    \param inst The unicast manager instance that the ASEs belong to.
    \param sink_ase Sink ASE to be checked for RTR sending.
*/
void leUnicastManager_ExecuteReadyToReceiveIfReady(le_um_instance_t *inst, le_um_ase_t *sink_ase);

/*! \brief Get the Frame duration from the ASE codec info */
uint16 LeUnicastManager_GetFrameDuration(le_um_ase_t *ase_context);


/*! \brief Remove the isochronous data paths from a CIS

    \param cis_data The CIS that the data paths belong to.
*/
void LeUnicastManager_RemoveDataPaths(le_um_cis_t *cis_data);

/*! \brief Remove all the data paths for a unicast instance.

    \param inst The unicast Manager instance to remove all data paths for.
*/
void LeUnicastManager_RemoveAllDataPaths(le_um_instance_t *inst);

#ifdef INCLUDE_MIRRORING

/*! \brief Get mirror type based on the configuration

    \param inst The unicast manager instance that the ASE belongs to.

    \return TRUE if mirror type was determined.
*/
bool leUnicastManager_UpdateMirrorType(le_um_instance_t *inst);

#endif /* INCLUDE_MIRRORING */

#if defined(ENABLE_LE_AUDIO_FT_UPDATE) || defined(INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE)
/*! \brief Update and send Flush Timeout VS Metadata info */
void LeUnicastManager_UpdateVSFTMetadata(le_um_ft_info_t *ft_info);
#endif

#endif /* LE_UNICAST_MANAGER_PRIVATE_H_ */
/*! @} */
