/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_unicast_manager
    \brief      Interfaces for the Unicast manager
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST)

#include "le_unicast_manager_instance.h"
#include "le_unicast_manager_private.h"
#include "le_unicast_manager_task.h"
#include "le_unicast_music_source.h"
#include "le_unicast_voice_source.h"
#include "le_unicast_media_control_interface.h"

#include <logging.h>

#include "bt_device.h"
#include "bt_types.h"
#include "device.h"
#include "device_db_serialiser.h"
#include "device_properties.h"
#include "gatt_connect.h"
#include "le_audio_messages.h"
#include "ltv_utilities.h"
#include "pacs_utilities.h"
#include "pddu_map.h"
#include "unicast_server_role.h"
#include "le_bap_profile.h"
#include "kymera.h"
#include "phy_state.h"
#include "anc_state_manager.h"
#include "bandwidth_manager.h"
#include "handset_ble_context.h"

#include <stream.h>
#include <operators.h>
#include <panic.h>
#include <ui.h>

#include "kymera_adaptation.h"
#include "kymera_adaptation_voice_protected.h"
#include "volume_types.h"
#include "voice_sources.h"
#include "mirror_profile.h"
#include "gatt.h"
#include "timestamp_event.h"

#include "telephony_messages.h"
#ifdef USE_SYNERGY
#include "media_control_client.h"
#include "call_control_client.h"
#include "tmap_server_role.h"
#include "tmap_client_sink.h"
#include "micp_server.h"
#endif
#include <qualcomm_connection_manager.h>
#include "audio_router.h"


/*! Codec configurable parameters for testing codec configuration initiated from unicast server */
typedef struct
{
    uint8 sampling_freq;
    uint8 frame_duration;
    uint8 supported_octet_per_codec_frame;
} le_um_test_codec_param_t;

#define LE_UM_AUDIO_LOCATION_STEREO     (AUDIO_LOCATION_FRONT_LEFT | AUDIO_LOCATION_FRONT_RIGHT)

#define leUnicastManager_IsAudioLocationFrontLeft(audio_location)   \
            ((audio_location) == AUDIO_LOCATION_FRONT_LEFT)

#define leUnicastManager_IsAudioLocationMonoFront(audio_location)   \
            ((audio_location) == AUDIO_LOCATION_FRONT_LEFT ||   \
             (audio_location) == AUDIO_LOCATION_FRONT_RIGHT ||  \
             (audio_location) == AUDIO_LOCATION_MONO)
        
#define leUnicastManager_IsAudioLocationStereoFront(audio_location) \
            ((audio_location) == LE_UM_AUDIO_LOCATION_STEREO)

#ifdef INCLUDE_LE_AUDIO_STEREO_CONFIG
#define leUnicastManager_IsAudioLocationSupported(audio_location)           \
            (leUnicastManager_IsAudioLocationStereoFront(audio_location) ||      \
             leUnicastManager_IsAudioLocationMonoFront(audio_location))

#else
#define leUnicastManager_IsAudioLocationSupported(audio_location)   \
            (leUnicastManager_IsAudioLocationMonoFront(audio_location))
#endif

/*! \brief ASE params to pass into leUnicastManager_AddEnabledAseToInstance */
typedef struct
{
    uint16 ase_id;
    uint16 audio_context;
    uint8 direction;
    const BapServerAseCodecInfo *codec_data;
    const BapServerAseQosInfo *qos_data;
    uint32 audio_location;
    uint8 ccid;
} le_um_add_enabled_ase_params_t;

/*! Informs registered clients with LE Audio/Voice connected message */
static void leUnicastManager_UpdateAudioConnected(le_um_instance_t *inst);

/*! Retrieve ASE information from unicast manager instance based on ASE identifier */
static le_um_ase_t * leUnicastManager_GetAse(gatt_cid_t cid, uint8 aseId);

/*! Routine that attempts to create data path */
static void leUnicastManager_EstablishDataPath(le_um_instance_t *inst, le_um_cis_t *cis_data);

/*! Routine to check if all cis are disconnected */
static bool leUnicastManager_IsAllCisDisconnected(le_um_instance_t *inst);

/*! \brief Retrieve cis information from unicast manager instance using cis_id */
static le_um_cis_t* leUnicastManager_GetCisById(le_um_instance_t *inst, uint8 cis_id)
{
    le_um_cis_t *cis = NULL;

    ARRAY_FOREACH(cis, inst->cis)
    {
        if (cis->state != le_um_cis_state_free &&
            cis->state != le_um_cis_state_stale &&
            cis->cis_id == cis_id)
        {
            return cis;
        }
    }

    return NULL;
}

/*! \brief Retrieve cis information from unicast manager instance using Cis Handle */
static le_um_cis_t* leUnicastManager_GetCisByHandle(le_um_instance_t *inst, hci_connection_handle_t cis_handle)
{
    le_um_cis_t *cis;

    ARRAY_FOREACH(cis, inst->cis)
    {
        if (cis->state != le_um_cis_state_free && cis->cis_handle == cis_handle)
        {
            return cis;
        }
    }

    return NULL;
}

uint8 LeUnicastManager_GetCisHandles(uint8 handle_list_size, hci_connection_handle_t *handles)
{
    le_um_instance_t *inst = LeUnicastManager_GetInstance();
    le_um_cis_t *cis;
    uint8 cis_count = 0;

    ARRAY_FOREACH(cis, inst->cis)
    {
        if (cis->state != le_um_cis_state_free)
        {
            if (handles != NULL && cis_count < handle_list_size)
            {
                handles[cis_count] = cis->cis_handle;
            }

            cis_count++;
        }
    }

    return cis_count;
}

static void leUnicastManager_UpdateAudioContext(le_um_instance_t *inst,
                                                uint16 audio_context)
{
    if (inst->audio_context == AUDIO_CONTEXT_TYPE_UNKNOWN)
    {
        inst->audio_context = (audio_context & AUDIO_CONTEXT_TYPE_COVERSATIONAL) ? AUDIO_CONTEXT_TYPE_COVERSATIONAL : audio_context ;
    }
}

static void leUnicastManager_ClaimAudioContext(le_um_instance_t *inst,
                                               GattAscsAseDirection direction,
                                               uint16 audio_context)
{
    switch (direction)
    {
        case GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK:
        {
            if (LeUnicastManager_IsAseActive(LeUnicastManager_InstanceGetLeftSinkAse(inst)) &&
                LeUnicastManager_IsAseActive(LeUnicastManager_InstanceGetRightSinkAse(inst)) &&
                LeBapPacsUtilities_IsSinkAudioContextAvailable(audio_context))
            {
                LeUnicastManager_ClaimSinkAudioContext(audio_context);
            }
        }
        break;

        case GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE:
        {
            if (LeUnicastManager_IsAseActive(LeUnicastManager_InstanceGetLeftSourceAse(inst)) &&
                LeUnicastManager_IsAseActive(LeUnicastManager_InstanceGetRightSourceAse(inst)) &&
                LeBapPacsUtilities_IsSourceAudioContextAvailable(audio_context))
            {
                LeUnicastManager_ClaimSourceAudioContext(audio_context);
            }
        }
        break;

        default:
        {
            Panic();
        }
        break;
    }
}

/*! \brief Function that begins a unicast audio sesson when ASE's are enabled. */
static void leUnicastManager_StartUnicastSession(le_um_instance_t *inst)
{
#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT
    bdaddr bd_addr = {0};
#endif
    TimestampEvent(TIMESTAMP_EVENT_LE_UNICAST_ASCS_ENABLE);

    UNICAST_MANAGER_LOG("leUnicastManager_StartUnicastSession cid=0x%x", inst->cid);

    /* Enable bandwith manager feature for Voice and Gaming mode with VBC use cases */
    if (LeUnicastManager_IsBothSourceAndSinkAseActive(inst))
    {
        BandwidthManager_FeatureStart(BANDWIDTH_MGR_FEATURE_LE_UNICAST);
    }

#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT
    if (GattConnect_GetPublicAddrFromConnectionId(inst->cid, &bd_addr))
    {
        appDeviceUpdateMruDevice(&bd_addr);
    }
#endif
}

/*! \brief Function that ends a unicast audio sesson when ASE's are disabled. */
static void leUnicastManager_EndUnicastSession(le_um_instance_t *inst)
{
    uint32 end_time;

    if (!LeUnicastManager_IsAnyAseEnabled(inst))
    {
        TimestampEvent(TIMESTAMP_EVENT_LE_UNICAST_ASCS_DISABLE);
        end_time = TimestampEvent_Delta(TIMESTAMP_EVENT_LE_UNICAST_ASCS_RECEIVER_START_READY,
                                        TIMESTAMP_EVENT_LE_UNICAST_ASCS_DISABLE);
        UNICAST_MANAGER_LOG("leUnicastManager_EndUnicastSession cid 0x%x, end_time = (%d)", inst->cid, end_time);

        if (LeUnicastManager_IsContextTypeConversational(inst->audio_context) ||
            LeUnicastManager_IsContextTypeGaming(inst->audio_context))
        {
            BandwidthManager_FeatureStop(BANDWIDTH_MGR_FEATURE_LE_UNICAST);
        }

        inst->audio_context = AUDIO_CONTEXT_TYPE_UNKNOWN;

        /* Clear the CID if all the cis are disconnected */
        if (leUnicastManager_IsAllCisDisconnected(inst))
        {
            inst->cid = INVALID_CID;
        }
    }
}

/*! \brief Try to turn on the audio subsystem early when ASE's gets enabled. */
static void leUnicastManager_TurnOnAudioSubsystem(void)
{
    /* Turn on the local audio subsystem irrespective of side */
    UNICAST_MANAGER_LOG("leUnicastManager_TurnOnAudioSubsystem powering on AudioSS");
    appKymeraProspectiveDspPowerOn(KYMERA_POWER_ACTIVATION_MODE_ASYNC);
}

/*! \brief Try to find an source/sink ase in idle state for mono audio location */
static le_um_ase_t * leUnicastManager_FindAseForMonoLocation(le_um_ase_t *left_ase, le_um_ase_t *right_ase)
{
    le_um_ase_t *ase = NULL;
    multidevice_side_t side = Multidevice_GetSide();

    switch (side)
    {
        case multidevice_side_left:
        case multidevice_side_both:
        default:
        {
            /* for side left, try to pick left sink ase first if it is idle.*/
            ase = left_ase;

            if (LeUnicastManager_IsAseActive(ase))
            {
                ase = right_ase;
            }
        }
        break;

        case multidevice_side_right:
        {
            /* for side right,try to pick right sink ase first, if it is idle.*/
            ase = right_ase;

            if (LeUnicastManager_IsAseActive(ase))
            {
                ase = left_ase;
            }
        }
        break;
    }

    return ase;
}

static void leUnicastManager_SwapAseContent(le_um_ase_t *ase1, le_um_ase_t *ase2)
{
    le_um_ase_t temp_ase = *ase1;

    *ase1 = *ase2;
    *ase2 = temp_ase;
}

/*! \brief Get a ASE based on the audio location and direction */
static le_um_ase_t * leUnicastManager_GetFreeAseByAudioLocation(le_um_instance_t *inst,
                                                                uint32 location,
                                                                uint8 direction,
                                                                const BapServerAseQosInfo *qos_data)
{
    le_um_ase_t *ase = NULL;
    le_um_ase_t *left_ase = NULL, *right_ase = NULL;
    bool swap_source_ases = FALSE;

    if (direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK)
    {
        switch (location)
        {
            case LE_UM_AUDIO_LOCATION_STEREO:
            case AUDIO_LOCATION_MONO:
                ase = leUnicastManager_FindAseForMonoLocation(LeUnicastManager_InstanceGetLeftSinkAse(inst),
                                                              LeUnicastManager_InstanceGetRightSinkAse(inst));
            break;

            case AUDIO_LOCATION_FRONT_LEFT:
                ase = LeUnicastManager_InstanceGetLeftSinkAse(inst);
            break;

            case AUDIO_LOCATION_FRONT_RIGHT:
                ase = LeUnicastManager_InstanceGetRightSinkAse(inst);
            break;

            default:
            break;
        }

        if (ase != NULL)
        {
            /* Cross check cis-id with source ASEs */
            left_ase = LeUnicastManager_InstanceGetLeftSourceAse(inst);
            right_ase = LeUnicastManager_InstanceGetRightSourceAse(inst);
            if (LeUnicastManager_IsAseActive(left_ase) && LeUnicastManager_IsCisMatches(left_ase->qos_info, qos_data))
            {
                if (LeUnicastManager_InstanceGetLeftSinkAse(inst) != ase)
                {
                    swap_source_ases = TRUE;
                }
            }
            else if (LeUnicastManager_IsAseActive(right_ase) && LeUnicastManager_IsCisMatches(right_ase->qos_info, qos_data))
            {
                if (LeUnicastManager_InstanceGetRightSinkAse(inst) != ase)
                {
                    swap_source_ases = TRUE;
                }
            }
        }
    }
    else if (direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE)
    {
        switch (location)
        {
            case LE_UM_AUDIO_LOCATION_STEREO:
            case AUDIO_LOCATION_MONO:
                ase = leUnicastManager_FindAseForMonoLocation(LeUnicastManager_InstanceGetLeftSourceAse(inst),
                                                              LeUnicastManager_InstanceGetRightSourceAse(inst));

                /* Cross check cis-id with sink ASEs */
                left_ase = LeUnicastManager_InstanceGetLeftSinkAse(inst);
                right_ase = LeUnicastManager_InstanceGetRightSinkAse(inst);
                if (LeUnicastManager_IsAseActive(left_ase) && LeUnicastManager_IsCisMatches(left_ase->qos_info, qos_data))
                {
                    if (LeUnicastManager_InstanceGetLeftSourceAse(inst) != ase)
                    {
                        swap_source_ases = TRUE;
                        ase = LeUnicastManager_InstanceGetLeftSourceAse(inst);
                    }
                }
                else if (LeUnicastManager_IsAseActive(right_ase) && LeUnicastManager_IsCisMatches(right_ase->qos_info, qos_data))
                {
                    if (LeUnicastManager_InstanceGetRightSourceAse(inst) != ase)
                    {
                        swap_source_ases = TRUE;
                        ase = LeUnicastManager_InstanceGetRightSourceAse(inst);
                    }
                }
            break;

            case AUDIO_LOCATION_FRONT_LEFT:
                ase = LeUnicastManager_InstanceGetLeftSourceAse(inst);
            break;

            case AUDIO_LOCATION_FRONT_RIGHT:
                ase = LeUnicastManager_InstanceGetRightSourceAse(inst);
            break;

            default:
            break;
        }
    }

    if (swap_source_ases)
    {
        leUnicastManager_SwapAseContent(LeUnicastManager_InstanceGetLeftSourceAse(inst), LeUnicastManager_InstanceGetRightSourceAse(inst));
    }

    if (ase != NULL && LeUnicastManager_IsAseActive(ase))
    {
        ase = NULL;
    }

    return ase;
}

/*! \brief Context is available. Add the ASE to unicast manager instance */
static le_um_ase_t * leUnicastManager_AddEnabledAseToInstance(le_um_instance_t *inst,
                                                             uint32 audio_location,
                                                             le_um_add_enabled_ase_params_t *ase_params)
{
    le_um_ase_t *ase_data;

    UNICAST_MANAGER_LOG("leUnicastManager_AddEnabledAseToInstance: instance=%p loc=0x%08x cid=0x%x ccid=0x%x",
                        inst, audio_location, inst->cid, ase_params->ccid);

    ase_data = leUnicastManager_GetFreeAseByAudioLocation(inst, audio_location,
                                                          ase_params->direction,
                                                          ase_params->qos_data);

    if (ase_data != NULL)
    {
        ase_data->audio_context = ase_params->audio_context;
        ase_data->ase_id = ase_params->ase_id;
        ase_data->direction = ase_params->direction;
        ase_data->codec_info = ase_params->codec_data;
        ase_data->qos_info = ase_params->qos_data;
        ase_data->state = le_um_ase_state_enabling;
        ase_data->ccid = ase_params->ccid;
    }

    return ase_data;
}

/*! \brief Get the ASEs (audio location data) based on the side, doesn't consider single microphone case*/
void leUnicastManager_GetAseFromSide(le_um_instance_t *inst, multidevice_side_t side,
                                     le_um_ase_t **sink_ase, le_um_ase_t **source_ase)
{
    switch (side)
    {
        case multidevice_side_left:
        case multidevice_side_both:
        default:
            *sink_ase = LeUnicastManager_InstanceGetLeftSinkAse(inst);
            *source_ase = LeUnicastManager_InstanceGetLeftSourceAse(inst);
        break;

        case multidevice_side_right:
            *sink_ase = LeUnicastManager_InstanceGetRightSinkAse(inst);
            *source_ase = LeUnicastManager_InstanceGetRightSourceAse(inst);
        break;
    }

    PanicFalse(*sink_ase != NULL && *source_ase != NULL);
}

/*! \brief Get multidevice side from ASE */
multidevice_side_t leUnicastManager_GetSideFromAse(le_um_instance_t *inst, le_um_ase_t *ase)
{
    if (ase == LeUnicastManager_InstanceGetLeftSinkAse(inst) ||
        ase == LeUnicastManager_InstanceGetLeftSourceAse(inst))
    {
        return multidevice_side_left;
    }

    return multidevice_side_right;
}

#ifdef ENABLE_LE_AUDIO_FT_UPDATE
static void leUnicastManager_AddDefaultFTInfo(le_um_ase_t *ase_data)
{
    if(NULL == ase_data->ft_info)
    {
        ase_data->ft_info = PanicUnlessMalloc(sizeof(le_um_ft_info_t));
    }
    ase_data->ft_info->latency_mode =LE_UM_PREFERED_LATENCY_MODE;
    ase_data->ft_info->min_flush_timeout = LE_UM_PREFERED_MIN_FLUSH_TIMEOUT;
    ase_data->ft_info->max_flush_timeout = LE_UM_PREFERED_MAX_FLUSH_TIMEOUT;
    ase_data->ft_info->max_bit_rate = LE_UM_PREFERED_MAX_BIT_RATE;
    ase_data->ft_info->err_resilience = LE_UM_PREFERED_ERR_RESILIENCE & ~LE_UM_ENABLE_REMOTE_STEREO_DOWNMIX;
}

/* Adds addition FT Update infomation to Metadata */
static void leUnicastManager_UpdateMetadataForASE(gatt_cid_t cid, le_um_ase_t *ase, uint8 * metadata, uint8 metadataLength)
{
    BapServerAseUpdateMetadataReq updateMetadataReq;
    uint8 * new_metadata = PanicUnlessMalloc(metadataLength + LE_UM_FTINFO_VS_METADATA_LENGTH + 1);

    if (metadata && metadataLength)
        memcpy(new_metadata, metadata, metadataLength);

    new_metadata[metadataLength++] = LE_UM_FTINFO_VS_METADATA_LENGTH;
    new_metadata[metadataLength++] = LE_UM_VENDOR_SPECIFIC_METADATA_TYPE;
    new_metadata[metadataLength++] = LE_UM_FTINFO_COMPANY_ID_QCOM_LOW;
    new_metadata[metadataLength++] = LE_UM_FTINFO_COMPANY_ID_QCOM_HIGH;
    new_metadata[metadataLength++] = LE_UM_FTINFO_METDATA_LENGTH;
    new_metadata[metadataLength++] = LE_UM_METADATA_LTV_TYPE_FT_REQUESTED_SETINGS;
    new_metadata[metadataLength++] = ase->ft_info->min_flush_timeout;
    new_metadata[metadataLength++] = ase->ft_info->max_flush_timeout;
    new_metadata[metadataLength++] = ase->ft_info->max_bit_rate;
    new_metadata[metadataLength++] = ase->ft_info->err_resilience;
    new_metadata[metadataLength++] = ase->ft_info->latency_mode;

    updateMetadataReq.cid = cid;
    updateMetadataReq.numAses = 1;

    updateMetadataReq.updateMetadataReqInfo->aseId = ase->ase_id;
    updateMetadataReq.updateMetadataReqInfo->metadataLength = metadataLength;
    updateMetadataReq.updateMetadataReqInfo->metadata = new_metadata;

    LeBapUnicastServer_AseUpdateMetadataRequest(&updateMetadataReq);
}


bool leUnicastManager_ValidateFTInfo(gatt_cid_t cid, uint8 ase_id, uint8* metadata, uint8 metadataLength)
{
    le_um_ase_t *ase = leUnicastManager_GetAse(cid, ase_id);
    le_um_ft_info_t ft_info;

    /* Look for settings supplied by the encoder, and store them.
       If there are no settings, then apply the default settings and update the metadata
     */
    if(BapServerLtvUtilitiesFindLtvValueFromVsMetadata(metadata, metadataLength, LE_UM_METADATA_LTV_TYPE_FT_CURRENT_SETTINGS, (uint8*)&ft_info, sizeof(le_um_ft_info_t)))
    {
        if (ase->ft_info != NULL)
        {
            free(ase->ft_info);
        }

        ase->ft_info=PanicUnlessMalloc(sizeof(le_um_ft_info_t));
        memcpy(ase->ft_info, &ft_info,sizeof(le_um_ft_info_t));
        return TRUE;
    }
    return FALSE;
}

void LeUnicastManager_EnableSourceMix(bool set_remote_mix)
{
    le_um_ase_t *ase;
    /* Get the active ASE */
    le_um_instance_t *inst = LeUnicastManager_GetInstance();

    UNICAST_MANAGER_LOG("LeUnicastManager_EnableSourceMix 0x%x", set_remote_mix);

    ARRAY_FOREACH(ase, inst->ase)
    {
        if (LeUnicastManager_IsAseActive(ase) && (ase->codec_info != NULL)
            && ( (LeUnicastManager_isVSAptXAdaptive(ase->codec_info) && ase->codec_version >=1)
               ||(!LeUnicastManager_isVSAptXAdaptive(ase->codec_info) && ase->codec_version >= LC3_DECODER_VERSION_FT))
           )
        {
            /* We are the version of aptX adaptive or LC3 that supports FT updates, so can can
               update the ft update structure and send that as a metadata update */

            if (ase->ft_info == NULL)
            {
                leUnicastManager_AddDefaultFTInfo(ase);
            }
            if (set_remote_mix)
            {
                ase->ft_info->err_resilience |= LE_UM_ENABLE_REMOTE_STEREO_DOWNMIX;
            }
            else
            {
                ase->ft_info->err_resilience &= ~LE_UM_ENABLE_REMOTE_STEREO_DOWNMIX;
            }

            UNICAST_MANAGER_LOG("LeUnicastManager_EnableSourceMix ft_info->err_res 0x%x", ase->ft_info->err_resilience);

            LeUnicastManager_SendAseVSMetadataUpdate(inst->cid, ase->ase_id, ase->ft_info);
        }
    }
}

#endif /* ENABLE_LE_AUDIO_FT_UPDATE */

#if defined(ENABLE_LE_AUDIO_FT_UPDATE) || defined(INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE)
void LeUnicastManager_UpdateVSFTMetadata(le_um_ft_info_t *ft_info)
{
    le_um_ase_t *ase;
    /* Get the active ASE */
    le_um_instance_t *inst = LeUnicastManager_GetInstance();

    UNICAST_MANAGER_LOG("LeUnicastManager_UpdateVSFTMetadata()");

    ARRAY_FOREACH(ase, inst->ase)
    {
        if (LeUnicastManager_IsAseActive(ase) && (ase->codec_info != NULL) && 
           (LeUnicastManager_isVSAptXAdaptive(ase->codec_info) || LeUnicastManager_isVSAptxLite(ase->codec_info)))
        {
            /* We are using VS Aptx Lite or aptX Adaptive that supports FT Metadata updates, so can can
               update the ft update structure and send that as a metadata update */

            ase->ft_info->max_flush_timeout = ft_info->max_flush_timeout;
            ase->ft_info->min_flush_timeout = ft_info->min_flush_timeout;
            ase->ft_info->max_bit_rate = ft_info->max_bit_rate;
            ase->ft_info->latency_mode = ft_info->latency_mode;
            ase->ft_info->err_resilience = ft_info->err_resilience;

            UNICAST_MANAGER_LOG("LeUnicastManager_UpdateVSFTMetadata max_flush_timeout 0x%x, min_flush_timeout 0x%x", ase->ft_info->max_flush_timeout, ase->ft_info->min_flush_timeout);

            LeUnicastManager_SendAseVSMetadataUpdate(inst->cid, ase->ase_id, ase->ft_info);
        }
    }
}
#endif

uint8 leUnicastManger_DetermineCodecVersion(const BapServerAseCodecInfo *codec_info)
{
    uint8 version = 0;
#ifdef ENABLE_LE_AUDIO_WBM
    if (codec_info->codecId.codingFormat == PACS_LC3_CODEC_ID)
    {
        if (LeBapPacsUtilities_Lc3EpcLicenseCheck())
        {
            //ase_data->lc3_version = LtvUtilities_FindLc3EncoderVersionFromVsMetadata(ase_metadata->metadata, ase_metadata->metadataLength);
            /* @TODO AG is not sending this metadata, so force LC3-BER when license check is available (or FT Update support when enabled) */
            version = LC3_DECODER_VERSION_ID_LE_UM;
        }
    }
#ifdef INCLUDE_LE_APTX_ADAPTIVE
    else if (LeUnicastManager_isVSAptXAdaptive(codec_info))
    {
        version = APTX_DECODER_VERSION_ID;
    }
#endif
    else if (LeUnicastManager_isVSAptxLite(codec_info))
    {
        version = 0;
    }
#else /* ENABLE_LE_AUDIO_WBM */
    UNUSED(codec_info);
#endif /* ENABLE_LE_AUDIO_WBM */

    return version;
}

/*! \brief Check and claim instance for the enabled ASE and add the ASE to unicast manager instance */
bool leUnicastManager_AseEnabled(gatt_cid_t cid, const AseMetadataType *ase_metadata, AudioContextType audio_context)
{
    bool enable_ase = FALSE;
    const BapServerAseCodecInfo *codec_info = NULL;
    const BapServerAseQosInfo *qos_info = NULL;
    uint8 direction = 0; /* 0 is direction uninitialised */
    uint32 audio_location = 0;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCidOrCreate(cid);
    le_um_ase_t *ase_data;
    uint8 *ccid_list = NULL;
    uint8 ccid_list_length = 0;
    uint8 ase_ccid = 0;

    /* Validate the Connection Id */
    if (!inst)
    {
        UNICAST_MANAGER_LOG("leUnicastManager_AseEnabled invalid cid=0x%x ase=0x%x", cid, ase_metadata->aseId);
        return FALSE;
    }

    direction = LeBapUnicastServer_GetAseDirection(cid, ase_metadata->aseId);
    codec_info = LeBapUnicastServer_GetCodecParameters(cid, ase_metadata->aseId);
    qos_info = LeBapUnicastServer_GetQoSParameters(cid, ase_metadata->aseId);

    if (codec_info == NULL || qos_info == NULL)
    {
        UNICAST_MANAGER_LOG("leUnicastManager_AseEnabled Bad codec/qos info ase=0x%x codec_info=%p qos_info=%p",
                            ase_metadata->aseId, codec_info, qos_info);
        return FALSE;
    }

    /* Get the audio location allocated for this ASE */
    audio_location = LeUnicastManager_GetAudioLocation(codec_info);

    if (!leUnicastManager_IsAudioLocationSupported(audio_location))
    {
        UNICAST_MANAGER_LOG("leUnicastManager_AseEnabled Unsupported Audio Location 0x%x", audio_location);
        return FALSE;
    }

    ccid_list = LtvUtilities_FindContentControlIdList(ase_metadata->metadata, ase_metadata->metadataLength, &ccid_list_length);

    if (ccid_list_length != 1)
    {
        UNICAST_MANAGER_WARN("leUnicastManager_AseEnabled: unexpected CCID list length %u", ccid_list_length);
    }

    if (ccid_list && ccid_list_length)
    {
        ase_ccid = ccid_list[0];
    }

    leUnicastManager_UpdateAudioContext(inst, audio_context);

    le_um_add_enabled_ase_params_t add_ase_params = {
        .ase_id = ase_metadata->aseId,
        .audio_context = audio_context,
        .direction = direction,
        .codec_data = codec_info,
        .qos_data = qos_info,
        .ccid = ase_ccid
    };
    ase_data = leUnicastManager_AddEnabledAseToInstance(inst,
                                                       audio_location,
                                                       &add_ase_params);

    if (ase_data != NULL)
    {
        DEBUG_LOG_INFO("leUnicastManager_AseEnabled cid=0x%x ase=0x%x, metadata length= %d", cid, ase_metadata->aseId, ase_metadata->metadataLength);

        leUnicastManager_TurnOnAudioSubsystem();
        ase_data->codec_version = leUnicastManger_DetermineCodecVersion(codec_info);

#ifdef ENABLE_LE_AUDIO_FT_UPDATE
        if (((codec_info->codecId.codingFormat == PACS_LC3_CODEC_ID) && ase_data->codec_version >= 2)
             || (LeUnicastManager_isVSAptXAdaptive(codec_info) && ase_data->codec_version >=1 ))
        {
            /* Validate the optional Metadata LTVS for Flush Timeout Value*/
            if (!leUnicastManager_ValidateFTInfo(cid, ase_metadata->aseId ,ase_metadata->metadata, ase_metadata->metadataLength))
            {
                /* if we have no FT update data stored for these versions, we must apply default values */
                leUnicastManager_AddDefaultFTInfo(ase_data);
                leUnicastManager_UpdateMetadataForASE(cid, ase_data ,ase_metadata->metadata, ase_metadata->metadataLength);
            }
        }
#endif /*ENABLE_LE_AUDIO_FT_UPDATE*/
        UNICAST_MANAGER_LOG("Codec version %d, audio context enum:AudioContextType:%d", ase_data->codec_version, audio_context);

        leUnicastManager_StartUnicastSession(inst);
        /* Associate CIS if already established */
        ase_data->cis_data = leUnicastManager_GetCisById(inst, qos_info->cisId);
        leUnicastManager_ClaimAudioContext(inst, ase_data->direction, audio_context);
        enable_ase = TRUE;
    }
    else
    {
        UNICAST_MANAGER_LOG("leUnicastManager_AseEnabled Duplicate Audio Location");
    }

    return enable_ase;
}

/*! \brief Update the ASE context for CIS data for which CIS-Id matches */
static bool leUnicastManager_UpdateAseForCisData(le_um_instance_t *inst, le_um_cis_t *cis_data)
{
    bool found = FALSE;
    le_um_ase_t *ase;
    le_um_ase_t *sink_ase = NULL;
#ifdef ENABLE_LEA_CIS_DELEGATION
    multidevice_side_t our_side = Multidevice_GetSide();
#endif

    /* Find the ASE with a matching CIS identifier and update the CIS data pointer */
    ARRAY_FOREACH(ase, inst->ase)
    {
        if (LeUnicastManager_IsAseActive(ase) && ase->qos_info->cisId == cis_data->cis_id)
        {
            UNICAST_MANAGER_LOG("leUnicastManager_UpdateAseForCisData: Update ASE ID %d with CIS Data", ase->ase_id);
            ase->cis_data = cis_data;
            
#ifdef ENABLE_LEA_CIS_DELEGATION
            cis_data->is_cis_delegated = leUnicastManager_GetSideFromAse(inst, ase) != our_side;
#endif
            if (ase->direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK)
            {
                /* Only single Sink ASE should be associated with a single CIS */
                PanicNotNull(sink_ase);
                sink_ase = ase;
                sink_ase->state = le_um_ase_state_streaming;
                leUnicastManager_ExecuteReadyToReceiveIfReady(inst, sink_ase);
            }

            found = TRUE;
        }
    }

    return found;
}

static bool leUnicastManager_IsVsLc3Available(le_um_instance_t *inst)
{
    bool vs_lc3_present = FALSE;
    le_um_ase_t *ase;

    /* Check if any of ASE is in enabled/streaming state */
    ARRAY_FOREACH(ase, inst->ase)
    {
        if (LeUnicastManager_IsAseActive(ase) &&
            ase->cis_data != NULL &&
            ase->cis_data->cis_handle == inst->cis->cis_handle &&
            ase->codec_version &&
            !LeUnicastManager_isVSAptXAdaptive(ase->codec_info))

        {
            vs_lc3_present = TRUE;
            break;
        }
    }
    return vs_lc3_present;
}

/*! \brief Update registered clients with an audio/voice connected message. */
static void leUnicastManager_UpdateAudioConnected(le_um_instance_t *inst)
{
    PanicFalse(inst->audio_context != AUDIO_CONTEXT_TYPE_UNKNOWN);

    UNICAST_MANAGER_LOG("leUnicastManager_UpdateAudioConnected: Sending Unicast Audio/Voice Connected (is_media: %d)",
                        LeUnicastManager_IsContextOfTypeMedia(inst->audio_context));

    if (LeUnicastManager_IsContextOfTypeMedia(inst->audio_context))
    {
        bdaddr handset_bdaddr;

        if(GattConnect_GetPublicAddrFromConnectionId(inst->cid, &handset_bdaddr))
        {
            {
                LeAudioMessages_SendUnicastMediaConnected(audio_source_le_audio_unicast_1, inst->audio_context);
            }
        }
    }
    else
    {
        LeAudioMessages_SendUnicastVoiceConnected(voice_source_le_audio_unicast_1);
        Telephony_NotifyCallAudioConnected(voice_source_le_audio_unicast_1);
    }
}
#ifdef ENABLE_LEA_CIS_DELEGATION

static bool leUnicastManager_IsSingleMicrophoneConfig(le_um_instance_t *inst)
{
    le_um_ase_t *our_sink_ase, *our_source_ase;
    le_um_ase_t *pair_sink_ase, *pair_source_ase;

    LeUnicastManager_GetAsesForGivenSide(inst, Multidevice_GetSide(), &our_sink_ase, &our_source_ase);
    LeUnicastManager_GetAsesForGivenSide(inst, Multidevice_GetPairSide(), &pair_sink_ase, &pair_source_ase);

    UNICAST_MANAGER_LOG("leUnicastManager_IsSingleMicrophoneConfig our_source_ase: %p, pair_source_ase: %p", our_source_ase, pair_source_ase);

    return our_source_ase != NULL && our_source_ase == pair_source_ase;
}

#else /* ENABLE_LEA_CIS_DELEGATION */

#define leUnicastManager_IsSingleMicrophoneConfig(inst)     (UNUSED(inst), FALSE)

#endif /* ENABLE_LEA_CIS_DELEGATION */

static void leUnicastManager_EstablishDataPath(le_um_instance_t *inst, le_um_cis_t *cis_data)
{
    uint8 dir = cis_data->dir;
    bool is_cis_delegated;

    cis_data->pending_data_cfm = 0;

    if (dir & LE_AUDIO_ISO_DIRECTION_UL)
    {
        cis_data->pending_data_cfm += 1;
        is_cis_delegated = leUnicastManager_IsDelegatedCis(cis_data);

        if (leUnicastManager_IsSingleMicrophoneConfig(inst))
        {
            /* The remote might have configured to use only the secondary Mic. If secondary goes away,
             * the primary should be able to send data using its own mic.In such scenario, if the CIS carrying
             * the mic data is delegated, the datapath has to be created as "RAW Stream" endpoints on the primary.
             */
            is_cis_delegated = FALSE;
        }

        UNICAST_MANAGER_LOG("leUnicastManager_EstablishDataPath cis_id: %d, dir: UL, is_cis_delegated: %d",
                            cis_data->cis_id, is_cis_delegated);

        LeBapUnicastServer_CreateDataPath(LeUnicastManager_GetTask(), cis_data->cis_handle, TRUE, FALSE, is_cis_delegated);
    }

    if (dir & LE_AUDIO_ISO_DIRECTION_DL)
    {
        cis_data->pending_data_cfm += 1;
        is_cis_delegated = leUnicastManager_IsDelegatedCis(cis_data);

        UNICAST_MANAGER_LOG("leUnicastManager_EstablishDataPath cis_id: %d, dir: DL, is_cis_delegated: %d",
                            cis_data->cis_id, is_cis_delegated);
        LeBapUnicastServer_CreateDataPath(LeUnicastManager_GetTask(), cis_data->cis_handle, FALSE, TRUE, is_cis_delegated);
    }
}

/*! \brief Check if it is special case where 2 CIS are used for our side */
static bool leUnicastManager_IsDualCisForOurSide(le_um_instance_t *inst)
{
    le_um_ase_t* ase_1;
    le_um_ase_t* ase_2;

    if (Multidevice_IsDeviceStereo())
    {
        /* In case of headset, check needs to be done if 2 CISes are used to carry left and right sinks */
        ase_1 = LeUnicastManager_InstanceGetLeftSinkAse(inst);
        ase_2 = LeUnicastManager_InstanceGetRightSinkAse(inst);

        if (ase_1->state != le_um_ase_state_idle && ase_2->state != le_um_ase_state_idle &&
            ase_1->qos_info->cisId != ase_2->qos_info->cisId)
        {
            return TRUE;
        }
    }

    leUnicastManager_GetAseFromSide(inst, Multidevice_GetSide(), &ase_1, &ase_2);

    return ase_1->state != le_um_ase_state_idle && ase_2->state != le_um_ase_state_idle &&
           ase_1->qos_info->cisId != ase_2->qos_info->cisId;
}

static multidevice_side_t leUnicastManager_GetSideForCis(le_um_instance_t *inst, le_um_cis_t *cis_data)
{
    le_um_ase_t* ase;
    multidevice_side_t side = multidevice_side_left;

    ARRAY_FOREACH(ase, inst->ase)
    {
        if (ase->state != le_um_ase_state_idle && ase->cis_data == cis_data)
        {
            if (leUnicastManager_IsAudioLocationStereoFront(LeUnicastManager_GetAudioLocation(ase->codec_info)))
            {
                side = multidevice_side_both;
                break;
            }
            else if (leUnicastManager_IsAudioLocationFrontLeft(LeUnicastManager_GetAudioLocation(ase->codec_info)))
            {
                side = multidevice_side_left;
            }
            else
            {
                side = multidevice_side_right;
            }
        }
    }

    return side;
}

/*! \brief Publish CIS connected to registered clients */
static void leUnicastManager_PublishCisConnected(le_um_instance_t *inst, le_um_cis_t *cis_data)
{
    multidevice_side_t side = leUnicastManager_GetSideForCis(inst, cis_data);
    LeAudioMessages_SendUnicastAudioCisConnected(side, cis_data->cis_id, cis_data->cis_handle, cis_data->dir);
}

/*! \brief Update CIS details in unicast manager instance and process the CIS established notification */
void leUnicastManager_CisEstablished(gatt_cid_t cid, uint8 cis_id, hci_connection_handle_t cis_handle, uint8 dir)
{
    le_um_cis_t *cis_data;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCid(cid);
    uint32 start_time;

    TimestampEvent(TIMESTAMP_EVENT_LE_UNICAST_CIS_ESTABLISH);
    start_time = TimestampEvent_Delta(TIMESTAMP_EVENT_LE_UNICAST_ASCS_ENABLE,
                                      TIMESTAMP_EVENT_LE_UNICAST_CIS_ESTABLISH);

    DEBUG_LOG_INFO("leUnicastManager_CisEstablished cid 0x%x cis 0x%x handle 0x%x start_time (%d)", cid, cis_id, cis_handle, start_time);
    DEBUG_LOG_INFO("leUnicastManager_CisEstablished instance 0x%x ", inst);

    if (inst)
    {
        /* Find a free slot for storing CIS data in unicast manager instance */
        ARRAY_FOREACH(cis_data, inst->cis)
        {
            if (cis_data->state == le_um_cis_state_free)
            {
                cis_data->cis_id = cis_id;
                cis_data->dir = dir;
                cis_data->cis_handle = cis_handle;
                cis_data->state = le_um_cis_state_established;

                if (!leUnicastManager_UpdateAseForCisData(inst, cis_data))
                {
                    /* No associated ASE, need not establish data path now */
                    return;
                }

                leUnicastManager_PublishCisConnected(inst, cis_data);
                break;
            }
        }

        PanicFalse(cis_data <= &inst->cis[LE_UM_MAX_CIS]);
        leUnicastManager_CheckAndEstablishDataPath(inst, FALSE);
    }
}

void leUnicastManager_CheckAndEstablishDataPath(le_um_instance_t *inst, bool publish_cis_connected)
{
    le_um_cis_t *cis_data;

    if (publish_cis_connected)
    {
        ARRAY_FOREACH(cis_data, inst->cis)
        {
            if (cis_data->state == le_um_cis_state_established)
            {
                leUnicastManager_PublishCisConnected(inst, cis_data);
            }
        }
    }

    if (LeUnicastManager_IsAllCisConnected(inst, FALSE))
    {
#ifdef ENABLE_LEA_CIS_DELEGATION
        /* First establish data path for non owned CIS */
        ARRAY_FOREACH(cis_data, inst->cis)
        {
            if (cis_data->state == le_um_cis_state_established && cis_data->is_cis_delegated)
            {
                leUnicastManager_EstablishDataPath(inst, cis_data);
            }
        }
#endif

        ARRAY_FOREACH(cis_data, inst->cis)
        {
            if (cis_data->state == le_um_cis_state_established && cis_data->pending_data_cfm == 0)
            {
                leUnicastManager_EstablishDataPath(inst, cis_data);
            }
        }
    }
}

static void leUnicastManager_SetMruFromCid(gatt_cid_t cid)
{
    bdaddr addr;

    if (GattConnect_GetPublicAddrFromConnectionId(cid, &addr))
    {
        DEBUG_LOG("leUnicastManager_SetMruFromCid: cid=0x%x addr=%04x %02x %06x", cid, addr.nap, addr.uap, addr.lap);
        appDeviceUpdateMruDevice(&addr);
    }
    else
    {
        DEBUG_LOG_WARN("leUnicastManager_SetMruFromCid: no address for cid=0x%x", cid);
    }
}


/*! \brief Update the state of the CIS to data path ready */
void leUnicastManager_DataPathCreated(hci_connection_handle_t cis_handle)
{
    le_um_cis_t *cis_data = NULL;
    le_um_cis_t *other_cis = NULL;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCisHandle(cis_handle);

    DEBUG_LOG_INFO("leUnicastManager_DataPathCreated instance %p cid 0x%x cis_handle 0x%x",
                   inst, inst ? inst->cid : INVALID_CID, cis_handle);

    if (inst)
    {
        ARRAY_FOREACH(cis_data, inst->cis)
        {
            if (cis_data->state == le_um_cis_state_established && cis_data->cis_handle == cis_handle)
            {
                cis_data->pending_data_cfm--;
                if (cis_data->pending_data_cfm == 0)
                {
                    cis_data->state = le_um_cis_state_data_path_ready;

                    if(leUnicastManager_IsVsLc3Available(inst))
                    {
                        QcomConManagerSetWBMFeature(cis_handle, TRUE);
                    }
                }
            }
            else if (cis_data->state == le_um_cis_state_data_path_ready)
            {
                other_cis = cis_data;
            }
        }

        /* In case 2 CIS are used for same side, then audio connected should be sent only if both CISes data path is setup */
        if ((!leUnicastManager_IsDualCisForOurSide(inst) || other_cis != NULL) && LeUnicastManager_IsAllCisConnected(inst, TRUE))
        {
            leUnicastManager_UpdateAudioConnected(inst);
            leUnicastManager_SetMruFromCid(inst->cid);
        }
    }
}

void leUnicastManager_DataPathRemoved(hci_connection_handle_t cis_handle)
{
    le_um_cis_t *cis = NULL;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCisHandle(cis_handle);

    if (inst)
    {
        ARRAY_FOREACH(cis, inst->cis)
        {
            if (cis->cis_handle == cis_handle)
            {
                cis->state = le_um_cis_state_established;
            }
        }
    }
}

/*! Executes Ready To Recieve procedure for sink ase if required condition is met */
void leUnicastManager_ExecuteReadyToReceiveIfReady(le_um_instance_t *inst, le_um_ase_t *sink_ase)
{
    bool can_send_rtr = TRUE;

#ifdef INCLUDE_MIRRORING
    le_um_ase_t *sink_ase_l = LeUnicastManager_InstanceGetLeftSinkAse(inst);
    le_um_ase_t *sink_ase_r = LeUnicastManager_InstanceGetRightSinkAse(inst);
    le_um_ase_t *other_ase = sink_ase == sink_ase_l ? sink_ase_r : sink_ase_l;

    if (sink_ase->rtr_status && (!LeUnicastManager_IsAseActive(other_ase) || other_ase->rtr_status))
    {
        /* No action required as RTR is already sent */
        return;
    }

    if (other_ase->state != le_um_ase_state_enabling)
    {
        can_send_rtr = !LeUnicastManager_IsAseActive(other_ase) ||  /* Only one ase active */
                       (other_ase->cis_data != NULL && (other_ase->cis_data->is_mirroring_attempted || sink_ase->cis_data->is_mirroring_attempted)); /* Delegation attempted */
       if (other_ase->cis_data != NULL)
       {
           UNICAST_MANAGER_LOG("leUnicastManager_ExecuteReadyToReceiveIfReady is_mirroring_attempted (%d)", other_ase->cis_data->is_mirroring_attempted);
       }
    }

    UNICAST_MANAGER_LOG("leUnicastManager_ExecuteReadyToReceiveIfReady can_send_rtr (%d)", can_send_rtr);

    if (sink_ase->rtr_status)
    {
        sink_ase = other_ase;
    }
#endif

    if (can_send_rtr)
    {
        uint32 start_time;
        
        TimestampEvent(TIMESTAMP_EVENT_LE_UNICAST_ASCS_RECEIVER_START_READY);
        start_time = TimestampEvent_Delta(TIMESTAMP_EVENT_LE_UNICAST_ASCS_ENABLE, TIMESTAMP_EVENT_LE_UNICAST_ASCS_RECEIVER_START_READY);
        UNICAST_MANAGER_LOG("leUnicastManager_ExecuteReadyToReceiveIfReady sent for ase_id : 0x%x, start_time (%d)", sink_ase->ase_id, start_time);

        PanicNotZero(sink_ase->rtr_status);
        sink_ase->rtr_status = TRUE;


        /* Execute a reciever ready request for the sink ase */
        LeBapUnicastServer_AseReceiveReadyRequest(inst->cid, 1, &sink_ase->ase_id);
    }
}

/*! \brief Remote is ready to receive. Move Source ASE to streaming state by sending response back */
void leUnicastManager_AseReceiverReady(gatt_cid_t cid, uint8 aseId)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCid(cid);
    uint32 start_time;

    TimestampEvent(TIMESTAMP_EVENT_LE_UNICAST_ASCS_RECEIVER_START_READY);
    start_time = TimestampEvent_Delta(TIMESTAMP_EVENT_LE_UNICAST_ASCS_ENABLE,
                                      TIMESTAMP_EVENT_LE_UNICAST_ASCS_RECEIVER_START_READY);

    UNICAST_MANAGER_LOG("leUnicastManager_AseReceiverReady instance %p cid=0x%x ase=0x%x, start_time = (%d)",
                        inst, cid, aseId, start_time);

    if (inst)
    {
        le_um_ase_t *left_src = LeUnicastManager_InstanceGetLeftSourceAse(inst);
        le_um_ase_t *right_src = LeUnicastManager_InstanceGetRightSourceAse(inst);

        if (left_src->ase_id == aseId && left_src->state == le_um_ase_state_enabling)
        {
            left_src->state = le_um_ase_state_streaming;
        }

        if (right_src->ase_id == aseId && right_src->state == le_um_ase_state_enabling)
        {
            right_src->state = le_um_ase_state_streaming;
        }

        if (left_src->ase_id == aseId || right_src->ase_id == aseId)
        {
            LeBapUnicastServer_AseReceiveReadyResponse(inst->cid, 1, &aseId);
            UNICAST_MANAGER_LOG("leUnicastManager_AseReceiverReady Success");
        }
        else
        {
            UNICAST_MANAGER_LOG("leUnicastManager_AseReceiverReady Bad ASE-ID or duplicate RecvReadyReq");
        }
    }
    else
    {
        UNICAST_MANAGER_LOG("leUnicastManager_AseReceiverReady Failure unknown CID %d", cid);
    }
}

static le_um_ase_t * leUnicastManager_GetAse(gatt_cid_t cid, uint8 aseId)
{
    le_um_ase_t *ase;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCid(cid);

    if (!inst)
    {
        DEBUG_LOG_WARN("leUnicastManager_GetAse No instance found for cid 0x%x", cid);
        return NULL;
    }

    /* Retrieve ASE data for the matching ASE ID */
    ARRAY_FOREACH(ase, inst->ase)
    {
        if (ase->ase_id == aseId)
        {
            return ase;
        }
    }

    UNICAST_MANAGER_LOG("leUnicastManager_GetAse No ASE found for Id=0x%x", aseId);
    return NULL;
}

#ifdef INCLUDE_LE_STEREO_RECORDING
bool LeUnicastManager_IsLeStereoRecordingActive(void)
{
    bool active = FALSE;
    le_um_instance_t *inst = NULL;

    ARRAY_FOREACH(inst, LeUnicastManager_GetTaskData()->le_unicast_instances)
    {
        if (LeUnicastManager_InstanceIsValid(inst))
        {
            if (LeUnicastManager_IsContextTypeLive(LeUnicastManager_InstanceGetAudioContext(inst)))
            {
                active = TRUE;
                break;
            }
        }
    }

    /* If stereo recording is active on primary or secondary */
    return (active || LeUnicastManager_IsContextTypeLive(MirrorProfile_GetLeAudioUnicastContext()));
}
#endif

bool LeUnicastManager_IsAnyAseEnabled(le_um_instance_t *inst)
{
    le_um_ase_t *ase;

    /* Check if any of ASE is in enabled/streaming state */
    ARRAY_FOREACH(ase, inst->ase)
    {
        if (LeUnicastManager_IsAseActive(ase))
        {
            return TRUE;
        }
    }

    return FALSE;
}

bool LeUnicastManager_IsAllCisConnected(le_um_instance_t *inst, bool data_path_required)
{
    le_um_ase_t *ase;

    /* Check if any of ASE is in enabled/streaming state */
    ARRAY_FOREACH(ase, inst->ase)
    {
        if (LeUnicastManager_IsAseActive(ase) &&
            (ase->cis_data == NULL || (data_path_required && ase->cis_data->pending_data_cfm != 0)))
        {
            /* Not all CISes are established or not all data path setup confirmations are received from established CISes */
            return FALSE;
        }
    }

    return TRUE;
}

static bool leUnicastManager_IsAllCisDisconnected(le_um_instance_t *inst)
{
    le_um_cis_t *cis = NULL;

    ARRAY_FOREACH(cis, inst->cis)
    {
        if (cis->cis_handle != LE_INVALID_CIS_HANDLE)
        {
            return FALSE;
        }
    }

    return TRUE;
}

le_um_state_t LeUnicastManager_GetState(le_um_instance_t *inst)
{
    le_um_ase_t *ase;
    le_um_cis_t *cis_data;
    uint8 cis_count = 0;
    bool is_streaming = FALSE;

    /* Check if any of ASE is in state */
    ARRAY_FOREACH(ase, inst->ase)
    {
        if (ase->state == le_um_ase_state_enabling || ase->state == le_um_ase_state_disabling)
        {
            return le_um_state_preparing;
        }
        else if (ase->state == le_um_ase_state_streaming || ase->state == le_um_ase_state_routed)
        {

            if (ase->cis_data == NULL || ase->cis_data->state != le_um_cis_state_data_path_ready)
            {
                return le_um_state_preparing;
            }

            is_streaming = TRUE;
        }
    }

    if (!is_streaming)
    {
        /* If there are CIS's present and ASE's are not streaming, return disabling */
        ARRAY_FOREACH(cis_data, inst->cis)
        {
            if (cis_data->state != le_um_cis_state_free)
            {
                cis_count++;
                return le_um_state_disabling;
            }
        }
    }
    /* only one CIS connected. This could be a local CIS and we would not have informed the lower layers to
     * "no delegate/no mirror" this CIS. In such conditions, activities such as handover should not be allowed. So
     * return that we are in a preparing state.
     */
    if (cis_count == 1)
    {
        return le_um_state_preparing;
    }

    return is_streaming ? le_um_state_streaming : le_um_state_idle;
}

/*! \brief Reset given ASE context */
static void leUnicastManager_ResetAse(le_um_ase_t *ase)
{
    if (ase != NULL)
    {
        if (ase->ft_info != NULL)
            free(ase->ft_info);
        memset(ase, 0, sizeof(*ase));
    }
}

/*! \brief Reset given CIS data */
static void leUnicastManager_ResetCis(le_um_cis_t *cis_data)
{
    le_um_instance_t *inst = LeUnicastManager_GetInstance();

    if (cis_data != NULL && cis_data->state != le_um_cis_state_free)
    {
        multidevice_side_t side;

#ifdef ENABLE_LEA_CIS_DELEGATION
        side = cis_data->is_cis_delegated ? Multidevice_GetPairSide() : Multidevice_GetSide();
#else
        side = Multidevice_GetSide();
#endif

        LeAudioMessages_SendUnicastAudioCisDisconnected(side, cis_data->cis_id, cis_data->cis_handle);

        memset(cis_data, 0, sizeof(*cis_data));
        cis_data->cis_handle = LE_INVALID_CIS_HANDLE;
        cis_data->cis_id = LE_INVALID_CIS_ID;
    }

    /* Clear the CID if all the cis are disconnected */
    if (leUnicastManager_IsAllCisDisconnected(inst) && !LeUnicastManager_IsAnyAseEnabled(inst))
    {
        inst->cid = INVALID_CID;
    }
}

/*! \brief Function to invalidate the cis if no ase is associated with the CIS */
static void leUnicastManager_InvalidateCisIfNoAseAssociated(le_um_cis_t *cis_data)
{
    le_um_ase_t *ase;
    le_um_instance_t *inst = LeUnicastManager_GetInstance();

    ARRAY_FOREACH(ase, inst->ase)
    {
        if (ase->cis_data == cis_data)
        {
            return;
        }
    }

    /* No ASE associated with this CIS, set the CIS state as stale */
    cis_data->state = le_um_cis_state_stale;
}

/*! \brief Initiate cleanup depending on ASE's present state */
void leUnicastManager_AseDisabled(gatt_cid_t cid, uint8 aseId)
{
    multidevice_side_t side;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCid(cid);
    le_um_ase_t *sink_ase = NULL;
    le_um_ase_t *source_ase = NULL;
    le_um_ase_t *ase_data = NULL;
    le_um_cis_t *cis_data = NULL;
    uint16 audio_context = 0;

    if (!inst)
    {
        DEBUG_LOG_WARN("leUnicastManager_AseDisabled invalid cid 0x%x ase 0x%x", cid, aseId);
        return;
    }

    DEBUG_LOG_INFO("leUnicastManager_AseDisabled cid 0x%x ase 0x%x", cid, aseId);

    ase_data = leUnicastManager_GetAse(cid, aseId);

    if (ase_data != NULL && LeUnicastManager_IsAseActive(ase_data))
    {
        audio_context = ase_data->audio_context;

        side = leUnicastManager_GetSideFromAse(inst, ase_data);
        leUnicastManager_GetAseFromSide(inst, side, &sink_ase, &source_ase);
        LeUnicastManager_RestoreAudioContext(ase_data->direction, audio_context);
        cis_data = ase_data->cis_data;

        leUnicastManager_ResetAse(ase_data);

        if (cis_data != NULL)
        {
            leUnicastManager_InvalidateCisIfNoAseAssociated(cis_data);
        }

        if (!LeUnicastManager_IsAseActive(sink_ase) &&
            !LeUnicastManager_IsAseActive(source_ase))
        {
            leUnicastManager_EndUnicastSession(inst);

            if (LeUnicastManager_IsContextOfTypeMedia(audio_context))
            {
                {
                    LeAudioMessages_SendUnicastMediaDisconnected(audio_source_le_audio_unicast_1, audio_context);
                }
            }
            else if (LeUnicastManager_IsContextTypeConversational(audio_context))
            {
                LeAudioMessages_SendUnicastVoiceDisconnected(voice_source_le_audio_unicast_1);
                Telephony_NotifyCallAudioDisconnected(voice_source_le_audio_unicast_1);
            }
        }
    }
    else
    {
        UNICAST_MANAGER_LOG("leUnicastManager_AseDisabled invalid state/ase-id ase_data: %p", ase_data);
    }
}

/*! \brief Initiate cleanup depending on ASE's present state */
void leUnicastManager_CidDisconnected(gatt_cid_t cid)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCid(cid);
    le_um_ase_t *ase;
    le_um_cis_t *cis_data;
    bool link_loss = FALSE;
    uint8 ase_to_release[le_um_audio_location_max];
    uint8 released_ase_count = 0;
    gatt_connect_disconnect_reason_t reason_code = GattConnect_GetDisconnectReasonCode(cid);
    device_t device;

    DEBUG_LOG_FN_ENTRY("leUnicastManager_CidDisconnected cid 0x%x, reasoncode 0x%x", cid, reason_code);

#ifdef ENABLE_LE_HANDOVER
    /* If the link is transferred(handed over), just return */
    if (reason_code == gatt_connect_disconnect_reason_link_transferred)
    {
        return;
    }
#endif

    if (!inst)
    {
        DEBUG_LOG_WARN("leUnicastManager_CidDisconnected no instance for cid 0x%x", cid);
        return;
    }

    /* Iterate through ASE list and find if any of the ASE's is still in
     * streaming/enabling state.
     */
    ARRAY_FOREACH(ase, inst->ase)
    {
        if (LeUnicastManager_IsAseActive(ase))
        {
            link_loss = TRUE;
            ase_to_release[released_ase_count] = ase->ase_id;
            released_ase_count++;

            /* Disconnect audio/voice & provide the context back to PACS service.
             * Also reset this ASE information in unicast manager context.
             */
            leUnicastManager_AseDisabled(cid, ase->ase_id);
        }
    }

    /* Clean the CIS information always on a LE-ACL closure.*/
    ARRAY_FOREACH(cis_data, inst->cis)
    {
        leUnicastManager_ResetCis(cis_data);
    }

    /* If this is a link loss,inform BAP to release the ASE's and set device property as linkloss while streaming */
    if (link_loss)
    {
        device = GattConnect_GetBtDevice(inst->cid);
        if (device != NULL)
        {
            DeviceProperties_SetHandsetBleContext(device, handset_ble_context_link_loss_streaming);
        }
        /* TODO BAP should give a single API to cleanup the ASE's */
        LeBapUnicastServer_AseReleaseRequest(cid, released_ase_count, ase_to_release);
        LeBapUnicastServer_AseReleasedRequest(cid, released_ase_count, ase_to_release, TRUE);
    }
}

/*! \brief Initiate cleanup depending on ASE's present state */
void leUnicastManager_CisDisconnected(uint16 cis_handle)
{
    le_um_ase_t *ase;
    le_um_cis_t *cis_data;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCisHandle(cis_handle);
    uint8 ase_id_to_disable = 0;
    BapServerReleasingAseInfo *releasing_ase_info = NULL;

    DEBUG_LOG_INFO("leUnicastManager_CisDisconnected cis_handle 0x%x", cis_handle);
    DEBUG_LOG_INFO("leUnicastManager_CisDisconnected instance 0x%x", inst);

    if (inst)
    {
        /* Iterate through ASE list and find if any of the ASE's managed by
         * this CIS is still in streaming/enabling state.
         */
        ARRAY_FOREACH(ase, inst->ase)
        {
            cis_data = ase->cis_data;

            if (LeUnicastManager_IsAseActive(ase) && cis_data != NULL && cis_data->cis_handle == cis_handle)
            {
                /* Preserve the ASE Id, so it can be used later to initate a
                 * disable operation.
                 */
                ase_id_to_disable = ase->ase_id;

                {
                    /* Probably CIS Link Loss (BAP_SERVER_CIS_DISCONNECTED_IND). Inform remote
                     * client, that the ASE managed by this CIS is disabling. For the CIS Loss case
                     * BAP will inform ASCS to move this ASE state to "QOS Configured".
                     * Note: Ordinarily, a server initiated Disable (where the CIS has not been lost),
                     * causes ASCS to move a sink ASE into the "Qos Configured" state and a
                     * source ASE into the "disabling" state.
                    */
                    LeBapUnicastServer_AseDisableRequest(inst->cid, 1, &ase_id_to_disable, TRUE /* CIS loss */);

                     /* Disconnect audio/voice, provide the claimed context back to PACS service.
                     * Reset the ASE information in unicast manager instance.
                     */
                    leUnicastManager_AseDisabled(inst->cid, ase_id_to_disable);
                }
            }
        }

        cis_data = leUnicastManager_GetCisByHandle(inst, cis_handle);
        if (cis_data != NULL)
        {
            /* Release the ASEs in releasing state for this cis */
            releasing_ase_info = LeBapUnicastServer_ReadReleasingAseIdsByCisId(inst->cid, cis_data->cis_id);
            if (releasing_ase_info != NULL)
            {
                UNICAST_MANAGER_LOG("leUnicastManager_CisDisconnected releasing ases");
                LeBapUnicastServer_AseReleasedRequest(inst->cid, releasing_ase_info->numAses, releasing_ase_info->aseIds, TRUE);
                pfree(releasing_ase_info);
            }

            /* Reset the CIS information stored in unicast manager instance.*/
            leUnicastManager_ResetCis(cis_data);
        }
    }
}

/*! \brief Initiate cleanup depending on ASE's present state */
void leUnicastManager_AseReleased(gatt_cid_t cid, uint8 aseId)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCid(cid);
    bool is_cis_connected = FALSE;

    UNICAST_MANAGER_LOG("leUnicastManager_AseReleased cid 0x%x aseId=0x%x instance %p", cid, aseId, inst);

    if (inst)
    {
        const BapServerAseQosInfo *qos_info = LeBapUnicastServer_GetQoSParameters(cid, aseId);

        le_um_ase_t *ase_data = leUnicastManager_GetAse(cid, aseId);

        if (ase_data != NULL && LeUnicastManager_IsAseActive(ase_data))
        {
            UNICAST_MANAGER_LOG("leUnicastManager_AseReleased called in Streaming/Enabling state for aseId=0x%x", aseId);
            /* Cleanup the Unicast manager context for this ASE, disconnect audio/voice and restore
             * context back to PACS service.
             */

            if (ase_data->cis_data != NULL)
            {
                is_cis_connected = TRUE;
            }

            leUnicastManager_AseDisabled(cid, ase_data->ase_id);
        }
        else if (qos_info != NULL)
        {
            if (leUnicastManager_GetCisById(inst, qos_info->cisId) != NULL)
            {
                is_cis_connected = TRUE;
            }
        }
    }

    if (!is_cis_connected)
    {
        /* BAP had already moved this ASE state to "Releasing" and would have autonomously
         * notified client already.Application has to send a ASE Release response to BAP,
         * so BAP informs the ASCS service to move the ASE which is already in "Releasing"
         * state to "Codec Configured" OR "Idle" state.
         */
        UNICAST_MANAGER_LOG("leUnicastManager_AseReleased cis already disconnected for this ase, Release ase : %d,", aseId);
        LeBapUnicastServer_AseReleasedRequest(cid, 1, &aseId, TRUE);
    }
}

void leUnicastManager_AseReceiverStop(gatt_cid_t cid, uint8 aseId)
{
    uint32 end_time;

    TimestampEvent(TIMESTAMP_EVENT_LE_UNICAST_ASCS_RECEIVER_STOP_READY);
    end_time = TimestampEvent_Delta(TIMESTAMP_EVENT_LE_UNICAST_ASCS_DISABLE,
                                    TIMESTAMP_EVENT_LE_UNICAST_ASCS_RECEIVER_STOP_READY);

    UNICAST_MANAGER_LOG("leUnicastManager_AseReceiverStop cid=0x%x ase=0x%x, end_time = (%d)", cid, aseId, end_time);

    /* Client must have already disabled its sink ASE's, before executing a ASE receiver stop
     * operation. BAP already should have moved the client Sink ASE's state to "Qos Configured"
     * So we don't need to do anything here
     */
}

bool leUnicastManager_UpdateMetadata(gatt_cid_t cid, const AseMetadataType *ase_metadata)
{
    UNICAST_MANAGER_LOG("leUnicastManager_UpdateMetadata cid=0x%x ase_metadata=%p ase=0x%x",
                        cid, ase_metadata, ase_metadata ? ase_metadata->aseId : 0);

    if (ase_metadata && ase_metadata->metadata)
    {
        DEBUG_LOG_DATA_VERBOSE(ase_metadata->metadata, ase_metadata->metadataLength);
    }

    // @ TODO Need to implement functionality to handle meta data change.
    // Reject any update of metadata, for us it would mean a change of usecase and therefore change of audio graph,
    // and this can't currently be done on the fly
    return FALSE;
}

static pdd_size_t leUnicastManager_GetDeviceDataLength(device_t device)
{
    void *config = NULL;
    size_t config_size = 0;

    if (Device_GetProperty(device, device_property_le_audio_unicast_config, &config, &config_size) == FALSE)
    {
        config_size = 0;
    }
    return config_size;
}

static void leUnicastManager_SerialisetDeviceData(device_t device, void *buf, pdd_size_t offset)
{
    UNUSED(offset);
    void *config = NULL;
    size_t config_size = 0;

    if (Device_GetProperty(device, device_property_le_audio_unicast_config, &config, &config_size))
    {
        memcpy(buf, config, config_size);
    }
}

static void leUnicastManager_DeserialisetDeviceData(device_t device, void *buf, pdd_size_t data_length, pdd_size_t offset)
{
    UNUSED(offset);

    Device_SetProperty(device, device_property_le_audio_unicast_config, buf, data_length);
}

void * leUnicastManager_RetrieveClientConfig(gatt_cid_t cid)
{
    void * device_config = NULL;
    device_t device = GattConnect_GetBtDevice(cid);

    if (device)
    {
        size_t size;

        if (!Device_GetProperty(device, device_property_le_audio_unicast_config, &device_config, &size))
        {
            device_config = NULL;
        }
    }

    return device_config;
}

void leUnicastManager_StoreClientConfig(gatt_cid_t cid, void * config, uint8 size)
{
    device_t device = GattConnect_GetBtDevice(cid);

    if (device)
    {
        if (!Device_SetProperty(device, device_property_le_audio_unicast_config, config, size))
        {
            UNICAST_MANAGER_LOG("leUnicastManager_StoreClientConfig Failed");
        }
    }
}

void LeUnicastManager_GetAsesForGivenSide(le_um_instance_t *inst, multidevice_side_t side, le_um_ase_t **sink_ase, le_um_ase_t **source_ase)
{
    le_um_ase_t *temp_ase;
    multidevice_side_t other_side = side == multidevice_side_left ? multidevice_side_right : multidevice_side_left;

    leUnicastManager_GetAseFromSide(inst, side, sink_ase, source_ase);

#ifdef INCLUDE_CIS_MIRRORING 
    if (!LeUnicastManager_IsAseActive(*sink_ase) && inst->mirror_type == le_um_cis_mirror_type_mirror)
    {
        *sink_ase = NULL;

        leUnicastManager_GetAseFromSide(inst, other_side, sink_ase, &temp_ase);

        if (!LeUnicastManager_IsAseActive(*sink_ase))
        {
            *sink_ase = NULL;
        }
    }
#endif

    /* For case of microphone path wherein only one CIS used, then pass on secondary CIS/ASE info so that if secondary
     * bud not available, primary still continues to send data.
     */
    if (!LeUnicastManager_IsAseActive(*source_ase))
    {
        *source_ase = NULL;

        if (Multidevice_GetType() == multidevice_type_pair)
        {
            leUnicastManager_GetAseFromSide(inst, other_side, &temp_ase, source_ase);
            if (!LeUnicastManager_IsAseActive(*source_ase))
            {
                *source_ase = NULL;
            }
        }
    }
}

static void leUnicastManager_RegisterAudioVoiceInterfaces(void)
{
    /* This routine will be called whenever there is a role change Ensure the interfaces
     * that are already registered are not registered again.
     */
    LeUnicastMusicSource_Init();
    LeUnicastMediaControlInterface_Init();
    LeUnicastVoiceSource_Init();
}

#ifdef INCLUDE_MIRRORING

bool leUnicastManager_UpdateMirrorType(le_um_instance_t *inst)
{
    le_um_ase_t *ase;
    uint8 snk_stream_count, src_stream_count, cis_scan, cis_count;
    uint8 cis_list[LE_UM_MAX_CIS] = {LE_INVALID_CIS_ID, LE_INVALID_CIS_ID};
    le_um_cis_mirror_type_t mirror_type = le_um_cis_mirror_type_mirror;

    if (inst == NULL)
    {
        return FALSE;
    }

    snk_stream_count = src_stream_count = cis_count = 0;
    ARRAY_FOREACH(ase, inst->ase)
    {
        if (LeUnicastManager_IsAseActive(ase))
        {
            ase->direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK ? snk_stream_count++
                                                                           : src_stream_count++;

            /* Check if the CIS managed by this ASE is in our list */
            for (cis_scan = 0; cis_scan < LE_UM_MAX_CIS; cis_scan++)
            {
                if (cis_list[cis_scan] == ase->qos_info->cisId)
                {
                    break;
                }
                else if (cis_list[cis_scan] == LE_INVALID_CIS_ID)
                {
                    cis_count++;
                    cis_list[cis_scan] = ase->qos_info->cisId;
                    break;
                }
            }

            if (cis_scan == LE_UM_MAX_CIS)
            {
                mirror_type = le_um_cis_mirror_type_invalid;
                break;
            }
        }
    }

    if (mirror_type != le_um_cis_mirror_type_invalid &&
        snk_stream_count <= LE_UM_SINK_AUDIO_STREAMS_MAX &&
        src_stream_count <= LE_UM_SOURCE_AUDIO_STREAMS_MAX)
    {
        if (src_stream_count == LE_UM_SOURCE_AUDIO_NO_STREAMS)
        {
            mirror_type = snk_stream_count == LE_UM_SINK_AUDIO_STREAMS_MAX ? le_um_cis_mirror_type_delegate
                                                                           : le_um_cis_mirror_type_mirror;
        }
        else if (snk_stream_count == LE_UM_SOURCE_AUDIO_NO_STREAMS)
        {
            mirror_type = src_stream_count == LE_UM_SOURCE_AUDIO_STREAMS_MAX ? le_um_cis_mirror_type_delegate
                                                                             : le_um_cis_mirror_type_mirror;
        }
        else
        {
            if (snk_stream_count == LE_UM_SOURCE_AUDIO_STREAMS_SHARED &&
                src_stream_count == LE_UM_SOURCE_AUDIO_STREAMS_SHARED)
            {
                mirror_type = le_um_cis_mirror_type_mirror;
            }
            else if (src_stream_count == LE_UM_SOURCE_AUDIO_STREAMS_SHARED)
            {
                mirror_type = LeUnicastManager_IsAseActive(&inst->ase[le_um_audio_location_left_source]) ?
                                                    le_um_cis_mirror_type_delegate_with_left_src_shared :
                                                    le_um_cis_mirror_type_delegate_with_right_src_shared;
            }
            else if (snk_stream_count == LE_UM_SOURCE_AUDIO_STREAMS_SHARED)
            {
                mirror_type = LeUnicastManager_IsAseActive(&inst->ase[le_um_audio_location_left_sink]) ?
                                                    le_um_cis_mirror_type_delegate_with_left_snk_shared :
                                                    le_um_cis_mirror_type_delegate_with_right_snk_shared;
            }
            else
            {
                mirror_type = le_um_cis_mirror_type_delegate;
            }
        }
    }
    else
    {
        mirror_type = le_um_cis_mirror_type_invalid;
    }

    DEBUG_LOG_INFO("leUnicastManager_UpdateMirrorType snk_stream_count: %d, src_stream_count: %d, enum:le_um_cis_mirror_type_t:%d",
                   snk_stream_count, src_stream_count, mirror_type);

    inst->mirror_type = mirror_type;

    return mirror_type != le_um_cis_mirror_type_invalid;
}

void LeUnicastManager_CisMirrorStatus(hci_connection_handle_t cis_handle, bool status)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCisHandle(cis_handle);

    if (inst)
    {
        le_um_cis_t *cis_data = leUnicastManager_GetCisByHandle(inst, cis_handle);

        PanicFalse(cis_data != NULL || !status);

        if (cis_data != NULL && cis_data->state != le_um_cis_state_stale)
        {
            le_um_ase_t *ase;

            ARRAY_FOREACH(ase, inst->ase)
            {
                if (ase->cis_data == cis_data)
                {
                    ase->cis_data->is_mirroring_attempted = TRUE;
                    if (status)
                    {
                        ase->cis_data->state = le_um_cis_state_data_path_ready;
                        ase->state = le_um_ase_state_routed;
                    }

                    if (ase->direction == ASE_DIRECTION_AUDIO_SINK)
                    {
                        UNICAST_MANAGER_LOG("LeUnicastManager_CisMirrorStatus Execute ready to receive %d",ase->ase_id);
                        leUnicastManager_ExecuteReadyToReceiveIfReady(inst, ase);
                    }
                }
            }
        }
        else
        {
            UNICAST_MANAGER_LOG("LeUnicastManager_CisMirrorStatus Race b/w CIS disconnect & delegation");
        }
    }
    else
    {
        UNICAST_MANAGER_LOG("LeUnicastManager_CisMirrorStatus No instance found for cis_handle 0x%x", cis_handle);
    }
}

void LeUnicastManager_RoleChangeInd(bool is_primary)
{
    if (is_primary)
    {
        leUnicastManager_RegisterAudioVoiceInterfaces();
    }
}

#endif /* INCLUDE_MIRRORING */

bool LeUnicastManager_GetLeAudioDevice(device_t *target_device)
{
    le_um_instance_t *inst = LeUnicastManager_GetInstance();

    /* @TODO: When multipoint is supported, focus needs to checked either here or during enable */
    if (inst->audio_context != AUDIO_CONTEXT_TYPE_UNKNOWN &&
        LeUnicastManager_IsAllCisConnected(inst, FALSE))
    {
        if (target_device != NULL)
        {
            *target_device = GattConnect_GetBtDevice(inst->cid);
            UNICAST_MANAGER_LOG("LeUnicastManager_GetLeAudioDevice: cid: %x, *target_device: %p",
                                inst->cid, *target_device);
        }

        return TRUE;
    }
    else if (target_device != NULL)
    {
        /* Fill MRU device for mirroring as CIS is not active */
        *target_device = BtDevice_GetMruDevice();
    }

    return FALSE;
}

static bool leUnicastManager_IsCisInExpectedStateForAllCises(le_um_instance_t *inst, le_um_cis_state_t state)
{
    bool result = FALSE;

    le_um_cis_t *cis;

    ARRAY_FOREACH(cis, inst->cis)
    {
        if (cis->state == state)
        {
            result = TRUE;
        }
        else
        {
            result = FALSE;
            break;
        }
    }

    return result;
}

static bool leUnicastManager_IsDataPathReadyForAllCises(le_um_instance_t *inst)
{
    bool result = leUnicastManager_IsCisInExpectedStateForAllCises(inst, le_um_cis_state_data_path_ready);

    DEBUG_LOG_V_VERBOSE("leUnicastManager_IsDataPathReadyForAllCises=%d", result);

    return result;
}

static bool leUnicastManager_IsCisEstablishedForAllCises(le_um_instance_t *inst)
{
    bool result = leUnicastManager_IsCisInExpectedStateForAllCises(inst, le_um_cis_state_established);

    DEBUG_LOG_V_VERBOSE("leUnicastManager_IsCisEstablishedForAllCises=%d", result);

    return result;

}

static void leUnicastManager_MediaStateChanged(uint8 media_state, gatt_cid_t cid)
{
    le_um_instance_t * inst = LeUnicastManager_InstanceGetByCid(cid);

    DEBUG_LOG_V_VERBOSE("leUnicastManager_MediaStateChanged state[%d] inst[0x%x], cid[0x%x]", media_state, inst, cid);

    if(inst
       && (media_state == GATT_MCS_CLIENT_PLAY)
       && (LeUnicastManager_IsAllCisConnected(inst, TRUE))
       && (LeUnicastManager_IsAnyAseEnabled(inst)))
    {
        if(leUnicastManager_IsDataPathReadyForAllCises(inst))
        {
            DEBUG_LOG_V_VERBOSE("leUnicastManager_MediaStateChanged data path ready for all CISes");

            LeAudioMessages_SendUnicastMediaDataPathReady(audio_source_le_audio_unicast_1, inst->audio_context);
        }
        else if(leUnicastManager_IsCisEstablishedForAllCises(inst))
        {
            DEBUG_LOG_V_VERBOSE("leUnicastManager_MediaStateChanged CIS established for all CISes");

            leUnicastManager_SetMruFromCid(cid);
            LeAudioMessages_SendUnicastMediaConnected(audio_source_le_audio_unicast_1, inst->audio_context);
        }
    }
}
static media_control_client_callback_if callback_if =
{
  .media_state_change_callback = leUnicastManager_MediaStateChanged
};

bool LeUnicastManager_Init(Task init_task)
{
    UNUSED(init_task);

    UNICAST_MANAGER_LOG("LeUnicastManager_Init");
    LeUnicastManagerTask_Init();
    LeUnicastManager_InstanceInit();
    LeTmapServer_Init();

#ifndef INCLUDE_MIRRORING
    leUnicastManager_RegisterAudioVoiceInterfaces();
#endif

#ifdef USE_SYNERGY
    TmapClientSink_Init(NULL);

#ifndef DISABLE_LE_AUDIO_MEDIA
    MediaControlClient_Init();
#endif

    mediaControlClient_RegisterForMediaStateChangeIndications(&callback_if);

#ifndef DISABLE_LE_AUDIO_VOICE
    CallControlClient_Init();
#endif

#endif /* USE_SYNERGY */

    if (LeUnicastManager_IsHighPriorityBandwidthUser())
    {
        PanicFalse(BandwidthManager_RegisterFeature(BANDWIDTH_MGR_FEATURE_LE_UNICAST, high_bandwidth_manager_priority, NULL));
    }

    return TRUE;
}

bool LeUnicastManager_IsStreamingActive(void)
{
    bool is_streaming = FALSE;

#ifdef ENABLE_LEA_CIS_DELEGATION
    if (!MirrorProfile_IsRolePrimary())
    {
        is_streaming = MirrorProfile_IsCisMirroringConnected();
    }
    else
#endif
    {
        le_um_instance_t *inst = LeUnicastManager_GetInstance();
        is_streaming = LeUnicastManager_IsAnyAseEnabled(inst);
    }
    return is_streaming;
}

bool LeUnicastManager_IsLinklossWhileStreaming(const tp_bdaddr *handset_addr)
{
    bool is_linkloss_while_streaming = FALSE;
    gatt_cid_t cid = GattConnect_GetConnectionIdFromTpaddr(handset_addr);
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCid(cid);
    le_um_internal_msg_t msg = LeUnicastManager_GetCisLinklossMessageForInst(inst);

    if (MessagePendingFirst(LeUnicastManager_GetTask(), msg, NULL))
    {
        MessageCancelFirst(LeUnicastManager_GetTask(), msg);
        is_linkloss_while_streaming = TRUE;
    }

    return is_linkloss_while_streaming;
}

uint16 LeUnicastManager_GetUnicastAudioContext(void)
{
    uint16 audio_context = 0;

#ifdef ENABLE_LEA_CIS_DELEGATION
    if (!MirrorProfile_IsRolePrimary())
    {
        audio_context = MirrorProfile_GetLeAudioUnicastContext();
    }
    else
#endif
    {
        le_um_instance_t *inst = LeUnicastManager_GetInstance();
        audio_context = LeUnicastManager_InstanceGetAudioContext(inst);
    }
    return audio_context;
}

device_t leUnicastManager_GetBtAudioDevice(audio_source_t source)
{
    device_t device;
    le_um_instance_t *inst;


   inst = LeUnicastManager_InstanceGetByAudioSource(source);
   device = (inst != NULL && inst->cid != INVALID_CID) ? GattConnect_GetBtDevice(inst->cid) : MediaClientControl_GetDeviceForAudioSource(source);

   return device;
}

static device_t leUnicastManager_GetUnicastOrMruDevice(void)
{
    device_t device = leUnicastManager_GetBtAudioDevice(audio_source_le_audio_unicast_1);

    if (device == NULL)
    {
        device = BtDevice_GetMruDevice();
    }

    return device;
}

device_t LeUnicastManager_GetDeviceForAudioSource(audio_source_t source)
{
    return (audio_source_le_audio_unicast_1 == source) ? leUnicastManager_GetUnicastOrMruDevice() : NULL;
}

device_t LeUnicastManager_GetDeviceForVoiceSource(voice_source_t source)
{
    return (voice_source_le_audio_unicast_1 == source) ? leUnicastManager_GetUnicastOrMruDevice() : NULL;
}

/* \brief Validates the given CID, if invalid assigns the CID from unicast manager context*/
static bool leUnicastManager_ValidateAndGetCid(gatt_cid_t *cid)
{
    if (*cid == INVALID_CID)
    {
        le_um_instance_t *inst = LeUnicastManager_GetInstance();
        *cid = inst->cid;
    }

    if (*cid == INVALID_CID)
    {
        DEBUG_LOG_INFO("leUnicastManager_ValidateAndGetCid Invalid CID");
        return FALSE;
    }

    return TRUE;
}

/* \brief Gets codec configuration for given config set */
static void leUnicastManager_GetTestCodecConfigParams(le_um_codec_config_set_t config_set, le_um_test_codec_param_t *codec_param)
{
    static const le_um_test_codec_param_t test_codec_param[LE_UM_TEST_CODEC_CONFIG_SET_MAX + 1] =
    {
        {LE_UM_TEST_SAMPLING_FREQUENCY_8kHz, LE_UM_TEST_FRAME_DURATION_7P5MS, 26},
        {LE_UM_TEST_SAMPLING_FREQUENCY_8kHz, LE_UM_TEST_FRAME_DURATION_10MS, 30},

        {LE_UM_TEST_SAMPLING_FREQUENCY_16kHz, LE_UM_TEST_FRAME_DURATION_7P5MS, 30},
        {LE_UM_TEST_SAMPLING_FREQUENCY_16kHz, LE_UM_TEST_FRAME_DURATION_10MS, 40},

        {LE_UM_TEST_SAMPLING_FREQUENCY_24kHz, LE_UM_TEST_FRAME_DURATION_7P5MS, 45},
        {LE_UM_TEST_SAMPLING_FREQUENCY_24kHz, LE_UM_TEST_FRAME_DURATION_10MS, 60},

        {LE_UM_TEST_SAMPLING_FREQUENCY_32kHz, LE_UM_TEST_FRAME_DURATION_7P5MS, 60},
        {LE_UM_TEST_SAMPLING_FREQUENCY_32kHz, LE_UM_TEST_FRAME_DURATION_10MS, 80},

        {LE_UM_TEST_SAMPLING_FREQUENCY_44_1kHz, LE_UM_TEST_FRAME_DURATION_7P5MS, 97},
        {LE_UM_TEST_SAMPLING_FREQUENCY_44_1kHz, LE_UM_TEST_FRAME_DURATION_10MS, 130},

        {LE_UM_TEST_SAMPLING_FREQUENCY_48kHz, LE_UM_TEST_FRAME_DURATION_7P5MS, 75},
        {LE_UM_TEST_SAMPLING_FREQUENCY_48kHz, LE_UM_TEST_FRAME_DURATION_10MS, 100},
        {LE_UM_TEST_SAMPLING_FREQUENCY_48kHz, LE_UM_TEST_FRAME_DURATION_7P5MS, 90},
        {LE_UM_TEST_SAMPLING_FREQUENCY_48kHz, LE_UM_TEST_FRAME_DURATION_10MS, 120},
        {LE_UM_TEST_SAMPLING_FREQUENCY_48kHz, LE_UM_TEST_FRAME_DURATION_7P5MS, 117},
        {LE_UM_TEST_SAMPLING_FREQUENCY_48kHz, LE_UM_TEST_FRAME_DURATION_10MS, 155},
        /* invalid entry */
        {0, 0, 0}
    };

    *codec_param = test_codec_param[config_set < LE_UM_TEST_CODEC_CONFIG_SET_MAX ? config_set : LE_UM_TEST_CODEC_CONFIG_SET_MAX];
}


/*! \brief Function to create codec info for config set */
static GattAscsServerConfigureCodecServerReqInfo *leUnicastManager_CreateCodecInfo(le_um_codec_config_set_t config_set)
{
    le_um_test_codec_param_t codec_param;

    leUnicastManager_GetTestCodecConfigParams(config_set, &codec_param);

    uint8 codec_config_data[] =
    {
        0x02, LC3_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY, codec_param.sampling_freq,
        0x02, LC3_CODEC_CONFIG_LTV_TYPE_FRAME_DURATION, codec_param.frame_duration,
        0x05, LC3_CODEC_CONFIG_LTV_TYPE_AUDIO_CHANNEL_ALLOCATION, 0x00, 0x00, 0x00, 0x04,
        0x03, LC3_CODEC_CONFIG_LTV_TYPE_OCTETS_PER_FRAME_CODEC, codec_param.supported_octet_per_codec_frame, 0x00,
        0x02, LC3_CODEC_CONFIG_LTV_TYPE_CODEC_FRAME_BLOCKS_PER_SDU, 0x02
    };

    GattAscsServerConfigureCodecServerReqInfo *codec_server_info =
        (GattAscsServerConfigureCodecServerReqInfo *) PanicNull(
            calloc(1,sizeof(GattAscsServerConfigureCodecServerReqInfo)));

    codec_server_info->retransmissionNumber = LE_UM_TEST_CODEC_SET_RETRANSMISSION_NUMBER;
    codec_server_info->phyPreference = GATT_ASCS_PHY_2M_BITS_PER_SECOND;
    codec_server_info->framing = GATT_ASCS_CODEC_CONFIGURED_FRAMING_UNFRAMED_ISOAL_PDUS_SUPPORTED;
    codec_server_info->transportLatencyMax = LE_UM_TEST_CODEC_SET_TRANSPORT_LATENCY_MAX;
    codec_server_info->presentationDelayMin = LE_UM_TEST_CODEC_SET_PRESENTATION_DELAY_MIN_US;
    codec_server_info->presentationDelayMax = LE_UM_TEST_CODEC_SET_PRESENTATION_DELAY_MAX_US;
    codec_server_info->preferredPresentationDelayMin = LE_UM_TEST_CODEC_SET_PREFERRED_PRESENTATION_DELAY_MIN_US;
    codec_server_info->preferredPresentationDelayMax = LE_UM_TEST_CODEC_SET_PREFERRED_PRESENTATION_DELAY_MAX_US;
    codec_server_info->codecConfigurationLength = sizeof(codec_config_data);
    codec_server_info->codecConfiguration = (uint8 *)PanicNull(
        calloc(1,codec_server_info->codecConfigurationLength));

    memcpy(codec_server_info->codecConfiguration, codec_config_data, codec_server_info->codecConfigurationLength);

    return codec_server_info;
}

void LeUnicastManager_SendAseConfigureCodecReq(gatt_cid_t cid, uint8 ase_id, le_um_codec_config_set_t config_set)
{
    bool is_success = FALSE;
    le_um_ase_t *ase;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCid(cid);
    GattAscsServerConfigureCodecReq *aseCodecInfo;

    if (!inst)
    {
        DEBUG_LOG_WARN("LeUnicastManager_SendAseConfigureCodecReq cid 0x%x invalid!", cid);
        return;
    }

    aseCodecInfo =
        (GattAscsServerConfigureCodecReq *) PanicNull(
            calloc(1,sizeof(GattAscsServerConfigureCodecReq)));
    aseCodecInfo->ase =
        (GattAscsServerConfigureCodecReqAse *) PanicUnlessMalloc(
            1 * sizeof(GattAscsServerConfigureCodecReqAse));

    aseCodecInfo->cid = cid;
    aseCodecInfo->numAses = 1;

    ARRAY_FOREACH(ase, inst->ase)
    {
        if (LeUnicastManager_IsAseActive(ase) && ase->ase_id == ase_id)
        {
            return;
        }
    }

    aseCodecInfo->ase[0].aseId = ase_id;
    aseCodecInfo->ase[0].codecId.codingFormat = PACS_LC3_CODEC_ID;
    aseCodecInfo->ase[0].codecId.companyId = 0x00;
    aseCodecInfo->ase[0].codecId.vendorSpecificCodecId = 0x00;
    aseCodecInfo->ase[0].gattAscsServerConfigureCodecServerInfo = leUnicastManager_CreateCodecInfo(config_set);

    is_success = LeBapUnicastServer_AseConfigureCodecReq(aseCodecInfo);

    DEBUG_LOG_INFO("LeUnicastManager_SendAseConfigureCodecReq le_um_codec_config_set_t enum:le_um_codec_config_set_t:%d, ase_id=0x%x, cid=0x%x, is_success=%d",
                   config_set, ase_id, cid, is_success);

    if (!is_success)
    {
        pfree(aseCodecInfo->ase->gattAscsServerConfigureCodecServerInfo->codecConfiguration);
        pfree(aseCodecInfo->ase->gattAscsServerConfigureCodecServerInfo);
    }
    pfree(aseCodecInfo->ase);
    pfree(aseCodecInfo);
}

void LeUnicastManager_SendAseDisableRequest(gatt_cid_t cid, uint8 ase_id)
{
    le_um_ase_t *ase;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCid(cid);

    DEBUG_LOG_INFO("LeUnicastManager_SendAseDisabledRequest instance %p cid 0x%x ase_id=0x%x",
                   inst, cid, ase_id);

    if (!inst)
    {
        DEBUG_LOG_WARN("LeUnicastManager_SendAseDisabledRequest cid is invalid!");
        return;
    }

    /* Iterate through ASE list and find if any of the ASE's managed by
     * this CIS is still in streaming/enabling state.
     */
    ARRAY_FOREACH(ase, inst->ase)
    {
        if (LeUnicastManager_IsAseActive(ase) && ase->ase_id == ase_id)
        {
            /* Send Disable ASE request*/
            LeBapUnicastServer_AseDisableRequest(cid, 1, &ase_id, FALSE);

            DEBUG_LOG_INFO("LeUnicastManager_SendAseDisableRequest ase_id=0x%x, cid=0x%x", ase_id, cid);
            /* Disconnect audio/voice, provide the claimed context back to PACS service.
            * Reset the ASE information in unicast manager context.
            */
            leUnicastManager_AseDisabled(cid, ase_id);
            return;
        }
    }
}

void LeUnicastManager_SendAseReleaseRequest(gatt_cid_t cid, uint8 ase_id)
{
    if (!leUnicastManager_ValidateAndGetCid(&cid))
    {
        return;
    }

    leUnicastManager_AseDisabled(cid, ase_id);
    LeBapUnicastServer_AseReleaseRequest(cid, 1, &ase_id);
    DEBUG_LOG_INFO("LeUnicastManager_SendAseReleaseRequest ase_id=0x%x, cid=0x%x", ase_id, cid);
}

void LeUnicastManager_SendAseReleasedRequest(gatt_cid_t cid, uint8 ase_id)
{
    if (!leUnicastManager_ValidateAndGetCid(&cid))
    {
        return;
    }

    LeBapUnicastServer_AseReleasedRequest(cid, 1, &ase_id, TRUE);
    DEBUG_LOG_INFO("LeUnicastManager_SendAseReleasedRequest ase_id=0x%x, cid=0x%x", ase_id, cid);
}

#if defined(ENABLE_LE_AUDIO_FT_UPDATE) || defined(INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE)
bool LeUnicastManager_SendAseVSMetadataUpdate(gatt_cid_t cid, uint8 ase_id, le_um_ft_info_t *ft_info)
{
    const unsigned total_length = LE_UM_TEST_METADATA_LEN + LE_UM_FTINFO_VS_METADATA_LENGTH + 1;
    bool is_success = FALSE;
    BapServerAseUpdateMetadataReq aseMetadataInfo;
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCid(cid);
    AudioContextType audio_context = (inst == NULL)? 0: inst->audio_context;

    if (!leUnicastManager_ValidateAndGetCid(&cid))
    {
        return FALSE;
    }

    uint8 *metadata = (uint8 *) PanicUnlessMalloc(total_length);

    metadata[0] = LE_UM_TEST_STREAMING_AUDIO_CONTEXT_LENGTH;
    metadata[1] = LE_UM_TEST_STREAMING_AUDIO_CONTEXT_TYPE;
    metadata[2] = audio_context & 0x00ff;
    metadata[3] = audio_context >> 8;

    metadata[4] = LE_UM_FTINFO_VS_METADATA_LENGTH;
    metadata[5] = LE_UM_VENDOR_SPECIFIC_METADATA_TYPE;
    metadata[6] = LE_UM_FTINFO_COMPANY_ID_QCOM_LOW;
    metadata[7] = LE_UM_FTINFO_COMPANY_ID_QCOM_HIGH;
    metadata[8] = LE_UM_FTINFO_METDATA_LENGTH;
    metadata[9] = LE_UM_METADATA_LTV_TYPE_FT_REQUESTED_SETINGS;
    metadata[10] = ft_info->min_flush_timeout;
    metadata[11] = ft_info->max_flush_timeout;
    metadata[12] = ft_info->max_bit_rate;
    metadata[13] = ft_info->err_resilience;
    metadata[14] = ft_info->latency_mode;

    aseMetadataInfo.cid = cid;
    aseMetadataInfo.numAses = 1;
    aseMetadataInfo.updateMetadataReqInfo[0].aseId = ase_id;
    aseMetadataInfo.updateMetadataReqInfo[0].metadata = metadata;
    aseMetadataInfo.updateMetadataReqInfo[0].metadataLength = total_length;

    is_success = LeBapUnicastServer_AseUpdateMetadataRequest(&aseMetadataInfo);

    if (!is_success)
    {
        pfree(metadata);
    }

    DEBUG_LOG_INFO("LeUnicastManager_SendAseVSMetadataUpdate ase_id=0x%x, cid=0x%x, is_success = %d", ase_id, cid, is_success);
    return is_success;
}
#endif

void LeUnicastManager_SendAseMetadataUpdate(gatt_cid_t cid, uint8 ase_id)
{
    bool is_success = FALSE;
    BapServerAseUpdateMetadataReq aseMetadataInfo;

    if (!leUnicastManager_ValidateAndGetCid(&cid))
    {
        return;
    }

    uint8 *metadata = (uint8 *) PanicUnlessMalloc(LE_UM_TEST_METADATA_LEN);
    metadata[0] = LE_UM_TEST_STREAMING_AUDIO_CONTEXT_LENGTH;
    metadata[1] = LE_UM_TEST_STREAMING_AUDIO_CONTEXT_TYPE;
    metadata[2] = 0x04;
    metadata[3] = 0x00;

    aseMetadataInfo.cid = cid;
    aseMetadataInfo.numAses = 1;
    aseMetadataInfo.updateMetadataReqInfo[0].aseId = ase_id;
    aseMetadataInfo.updateMetadataReqInfo[0].metadataLength = LE_UM_TEST_METADATA_LEN;
    aseMetadataInfo.updateMetadataReqInfo[0].metadata = metadata;

    is_success = LeBapUnicastServer_AseUpdateMetadataRequest(&aseMetadataInfo);

    if (!is_success)
    {
        pfree(metadata);
    }

    DEBUG_LOG_INFO("LeUnicastManager_SendAseMetadataUpdate ase_id=0x%x, cid=0x%x, is_success = %d", ase_id, cid, is_success);
}

void LeUnicastManager_RestoreAudioContext(GattAscsAseDirection direction, uint16 audio_context)
{
    switch (direction)
    {
        case GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK:
        {
            LeUnicastManager_RestoreSinkAudioContext(audio_context);
        }
        break;

        case GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE:
        {
            LeUnicastManager_RestoreSourceAudioContext(audio_context);
        }
        break;

        default:
        {
            Panic();
        }
        break;
    }
}

appKymeraLeStreamType leUnicastManager_DetermineStreamType(le_um_ase_t *ase, le_um_ase_t *ase_r)
{
    appKymeraLeStreamType stream_type;
    uint32 audio_location = ase != NULL ? LeUnicastManager_GetAudioLocation(ase->codec_info) :
                           (ase_r != NULL ? LeUnicastManager_GetAudioLocation(ase_r->codec_info) : 0);

    if (Multidevice_IsDeviceStereo())
    {
        if (ase != NULL && ase_r != NULL)
        {
            /* Two ases used, check if it is on same CIS is part of same CIS (i.e. carries both L & R) */
            stream_type = ase->cis_data == ase_r->cis_data ? KYMERA_LE_STREAM_STEREO_USE_BOTH : KYMERA_LE_STREAM_DUAL_MONO;
        }
        else if (audio_location == LE_UM_AUDIO_LOCATION_STEREO)
        {
            /* Single CIS carrying both L & R */
            stream_type = KYMERA_LE_STREAM_STEREO_USE_BOTH;
        }
        else
        {
            /* Single CIS carrying mono */
            stream_type = KYMERA_LE_STREAM_MONO;
        }
    }
    else
    {
        if (audio_location == LE_UM_AUDIO_LOCATION_STEREO)
        {
            /* Not a supported config (single CIS carrying both L & R), but just use one of the sides */
            stream_type = Multidevice_GetSide() == multidevice_side_left ? KYMERA_LE_STREAM_STEREO_USE_LEFT : KYMERA_LE_STREAM_STEREO_USE_RIGHT;
        }
        else
        {
            /* Single CIS carrying mono */
            stream_type = KYMERA_LE_STREAM_MONO;
        }
    }

    return stream_type;
}

uint16 LeUnicastManager_GetFrameDuration(le_um_ase_t *ase_context)
{
#ifdef INCLUDE_LE_APTX_ADAPTIVE
    if (LeUnicastManager_isVSAptXAdaptive(ase_context->codec_info))
    {
        uint16 iso_int_ms = (ase_context->qos_info != NULL) ? ase_context->qos_info->sduInterval : LE_UM_APTX_DEFAULT_FRAME_DURATION;
        /* Frame duration not stored in the codecConfigration,
         * but will match the SDU interval  */
        UNICAST_MANAGER_LOG("LeUnicastManager_GetFrameDuration - aptX adaptive %d",iso_int_ms);
        return iso_int_ms;
    }
#endif

#ifdef INCLUDE_LE_AUDIO_GAMING_MODE_APTX_LITE
    if (LeUnicastManager_isVSAptxLite(ase_context->codec_info))
    {
        return LE_UM_APTX_LITE_DEFAULT_FRAME_DURATION;
    }
#endif

    return BapServerLtvUtilitiesGetFrameDuration(ase_context->codec_info->infoFromServer.codecConfiguration,
                                                 ase_context->codec_info->infoFromServer.codecConfigurationLength);
}

void LeUnicastManager_ClaimSinkAudioContext(uint16 audio_context)
{
    LeBapPacsUtilities_ClaimSinkAudioContext(audio_context);
    UNICAST_MANAGER_LOG("LeUnicastManager_ClaimSinkAudioContext Claim context 0x%04x, Available context: 0x%04x",
                        audio_context, LeBapPacsUtilities_GetSinkAudioContextAvailability());
}

void LeUnicastManager_RestoreSinkAudioContext(uint16 audio_context)
{
    LeBapPacsUtilities_RestoreSinkAudioContext(audio_context);
    UNICAST_MANAGER_LOG("LeUnicastManager_RestoreSinkAudioContext Restore context 0x%04x, Available context: 0x%04x",
                        audio_context, LeBapPacsUtilities_GetSinkAudioContextAvailability());
}

void LeUnicastManager_ClaimSourceAudioContext(uint16 audio_context)
{
    LeBapPacsUtilities_ClaimSourceAudioContext(audio_context);
    UNICAST_MANAGER_LOG("LeUnicastManager_ClaimSourceAudioContext Claim context 0x%04x, Available context: 0x%04x",
                        audio_context, LeBapPacsUtilities_GetSourceAudioContextAvailability());
}

void LeUnicastManager_RestoreSourceAudioContext(uint16 audio_context)
{
    LeBapPacsUtilities_RestoreSourceAudioContext(audio_context);
    UNICAST_MANAGER_LOG("LeUnicastManager_RestoreSourceAudioContext Restore context 0x%04x, Available context: 0x%04x",
                        audio_context, LeBapPacsUtilities_GetSourceAudioContextAvailability());
}

void LeUnicastManager_RegisterAsPersistentDeviceDataUser(void)
{
    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        PDDU_ID_LEA_UNICAST_MANAGER,
        leUnicastManager_GetDeviceDataLength,
        leUnicastManager_SerialisetDeviceData,
        leUnicastManager_DeserialisetDeviceData);

#ifdef USE_SYNERGY
    TmapClientSink_RegisterAsPersistentDeviceDataUser();
    CallControlClient_RegisterAsPersistentDeviceDataUser();
    MediaControlClient_RegisterAsPersistentDeviceDataUser();
    MicpServer_RegisterAsPersistentDeviceDataUser();
#endif
}

appKymeraLeAudioCodec leUnicastManager_GetCodecType(const BapServerAseCodecInfo *codec_info)
{
    appKymeraLeAudioCodec codec_type = KYMERA_LE_AUDIO_CODEC_LC3;

    if (codec_info->codecId.codingFormat == 0xff && codec_info->codecId.companyId == VS_METADATA_COMPANY_ID_QUALCOMM)
    {
        codec_type = codec_info->codecId.vendorSpecificCodecId == APTX_ADAPTIVE_VS_CODEC_ID ? KYMERA_LE_AUDIO_CODEC_APTX_ADAPTIVE : KYMERA_LE_AUDIO_CODEC_APTX_LITE;
    }

    UNICAST_MANAGER_LOG("leUnicastMusicSource_GetCodecType codec_type enum:appKymeraLeAudioCodec:%d", codec_type);

    return codec_type;
}

void LeUnicastManager_RemoveDataPaths(le_um_cis_t *cis_data)
{
    if (cis_data)
    {
        /* The bitmask values for the direction in cis_data->dir are the
           opposite way around to the bitmask expected by
           BapServerRemoveIsoDataPathReq. Do the conversion here.

           Note: BapServerRemoveIsoDataPathReq uses the same bitmask values as
           the underlying HCI command HCI_LE_Remove_ISO_Data_Path. */
        uint8 direction_mask = 0;

        direction_mask |= (cis_data->dir & LE_AUDIO_ISO_DIRECTION_DL) ? (1 << ISOC_DATA_PATH_DIRECTION_CONTROLLER_TO_HOST) : 0;
        direction_mask |= (cis_data->dir & LE_AUDIO_ISO_DIRECTION_UL) ? (1 << ISOC_DATA_PATH_DIRECTION_HOST_TO_CONTROLLER) : 0;

        UNICAST_MANAGER_LOG("LeUnicastManager_RemoveDataPaths: cis_id=0x%02x cis_dir=0x%01x direction_mask=0x%01x",
                            cis_data->cis_id, cis_data->dir, direction_mask);

        LeBapUnicastServer_RemoveDataPath(cis_data->cis_handle, direction_mask);
    }
}

void LeUnicastManager_RemoveAllDataPaths(le_um_instance_t *inst)
{
    le_um_cis_t *cis = NULL;

    ARRAY_FOREACH(cis, inst->cis)
    {
        switch (cis->state)
        {
        case le_um_cis_state_data_path_ready:
            {
                LeUnicastManager_RemoveDataPaths(cis);
            }
            break;

        default:
            break;
        }
    }
}


#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) */
