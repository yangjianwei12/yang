
/*******************************************************************************

Copyright (C) 2021 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef CAP_CLIENT_PRIM_H
#define CAP_CLIENT_PRIM_H

#include "csr_list.h"
#include "bap_client_lib.h"
#include "vcp.h"
#include "csip.h"
#include "micp.h"
#include "csr_types.h"
#include "qbl_types.h"
#include "bluetooth.h"
#include "service_handle.h"

#define CAP_CLIENT_MAX_SUPPORTED_ASES 6

/* Cap Client Primitives */

typedef uint16 CapClientPrim;

#define CAP_CLIENT_PROFILE_PRIM             (SYNERGY_EVENT_BASE + CAP_CLIENT_PRIM)

/*CAP upstream Prims*/
#define CAP_CLIENT_INIT_CFM                                    ((CapClientPrim)0x0000u)
#define CAP_CLIENT_ADD_NEW_DEV_CFM                             ((CapClientPrim)0x0001u)
#define CAP_CLIENT_REMOVE_DEV_CFM                              ((CapClientPrim)0x0002u)
#define CAP_CLIENT_INIT_STREAM_CONTROL_CFM                     ((CapClientPrim)0x0003u)
#define CAP_CLIENT_DISCOVER_STREAM_CAPABILITIES_CFM            ((CapClientPrim)0x0004u)
#define CAP_CLIENT_DISCOVER_AVAILABLE_AUDIO_CONTEXT_CFM        ((CapClientPrim)0x0005u)
#define CAP_CLIENT_UNICAST_CONNECT_CFM                         ((CapClientPrim)0x0006u)
#define CAP_CLIENT_UNICAST_START_STREAM_CFM                    ((CapClientPrim)0x0007u)
#define CAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM                    ((CapClientPrim)0x0008u)
#define CAP_CLIENT_UNICAST_STOP_STREAM_CFM                     ((CapClientPrim)0x0009u)
#define CAP_CLIENT_CHANGE_VOLUME_CFM                           ((CapClientPrim)0x000Au)
#define CAP_CLIENT_MUTE_CFM                                    ((CapClientPrim)0x000Bu)
#define CAP_CLIENT_REGISTER_TASK_CFM                           ((CapClientPrim)0x000Cu)
#define CAP_CLIENT_BCAST_SRC_INIT_CFM                          ((CapClientPrim)0x000Du)
#define CAP_CLIENT_BCAST_SRC_CONFIG_CFM                        ((CapClientPrim)0x000Eu)
#define CAP_CLIENT_BCAST_SRC_START_STREAM_CFM                  ((CapClientPrim)0x000Fu)
#define CAP_CLIENT_BCAST_SRC_STOP_STREAM_CFM                   ((CapClientPrim)0x0010u)
#define CAP_CLIENT_BCAST_SRC_REMOVE_STREAM_CFM                 ((CapClientPrim)0x0011u)
#define CAP_CLIENT_BCAST_SRC_UPDATE_STREAM_CFM                 ((CapClientPrim)0x0012u)
#define CAP_CLIENT_BCAST_ASST_START_SRC_SCAN_CFM               ((CapClientPrim)0x0013u)
#define CAP_CLIENT_BCAST_ASST_START_SYNC_TO_SRC_CFM            ((CapClientPrim)0x0014u)
#define CAP_CLIENT_BCAST_ASST_CANCEL_SYNC_TO_SRC_CFM           ((CapClientPrim)0x0015u)
#define CAP_CLIENT_BCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM        ((CapClientPrim)0x0016u)
#define CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM                      ((CapClientPrim)0x0017u)
#define CAP_CLIENT_BCAST_ASST_MODIFY_SRC_CFM                   ((CapClientPrim)0x0018u)
#define CAP_CLIENT_BCAST_ASST_REMOVE_SRC_CFM                   ((CapClientPrim)0x0019u)
#define CAP_CLIENT_BCAST_ASST_REGISTER_NOTIFICATION_CFM        ((CapClientPrim)0x001Au)
#define CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_CFM           ((CapClientPrim)0x001Bu)
#define CAP_CLIENT_BCAST_ASST_STOP_SRC_SCAN_CFM                ((CapClientPrim)0x001Cu)
#define CAP_CLIENT_UNLOCK_COORDINATED_SET_CFM                  ((CapClientPrim)0x001Du)
#define CAP_CLIENT_AVAILABLE_AUDIO_CONTEXT_IND                 ((CapClientPrim)0x001Eu)
#define CAP_CLIENT_VOLUME_STATE_IND                            ((CapClientPrim)0x001Fu)
#define CAP_CLIENT_ACTIVE_GROUP_CHANGED_IND                    ((CapClientPrim)0x0020u)
#define CAP_CLIENT_UNICAST_START_STREAM_IND                    ((CapClientPrim)0x0021u)
#define CAP_CLIENT_PAC_RECORD_CHANGED_IND                      ((CapClientPrim)0x0022u)
#define CAP_CLIENT_BCAST_ASST_SRC_REPORT_IND                   ((CapClientPrim)0x0023u)
#define CAP_CLIENT_BCAST_ASST_BRS_IND                          ((CapClientPrim)0x0024u)
#define CAP_CLIENT_BCAST_ASST_SYNC_LOSS_IND                    ((CapClientPrim)0x0025u)
#define CAP_CLIENT_BCAST_ASST_SET_CODE_IND                     ((CapClientPrim)0x0026u)
#define CAP_CLIENT_UNICAST_LINK_LOSS_IND                       ((CapClientPrim)0x0027u)
#define CAP_CLIENT_DEREGISTER_TASK_CFM                         ((CapClientPrim)0x0028u)
#define CAP_CLIENT_CSIP_READ_CFM                               ((CapClientPrim)0x0029u)
#define CAP_CLIENT_READ_VOLUME_STATE_CFM                       ((CapClientPrim)0x002Au)
#define CAP_CLIENT_UNICAST_CIG_TEST_CFM                        ((CapClientPrim)0x002Bu)
#define CAP_CLIENT_UNICAST_DISCONNECT_CFM                      ((CapClientPrim)0x002Cu)
#define CAP_CLIENT_UNICAST_DISCONNECT_IND                      ((CapClientPrim)0x002Du)
#define CAP_CLIENT_BCAST_SRC_DEINIT_CFM                        ((CapClientPrim)0x002Eu)
#define CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_IND           ((CapClientPrim)0x002Fu)
#define CAP_CLIENT_SET_MICP_ATTRIB_HANDLES_CFM                 ((CapClientPrim)0x0030u)
#define CAP_CLIENT_INIT_OPTIONAL_SERVICES_CFM                  ((CapClientPrim)0x0031u)
#define CAP_CLIENT_READ_MIC_STATE_CFM                          ((CapClientPrim)0x0032u)
#define CAP_CLIENT_MIC_STATE_IND                               ((CapClientPrim)0x0033u)
#define CAP_CLIENT_SET_MIC_STATE_CFM                           ((CapClientPrim)0x0034u)
#define CAP_CLIENT_UPDATE_METADATA_IND                         ((CapClientPrim)0x0035u)
#define CAP_CLIENT_UNICAST_SET_VS_CONFIG_DATA_CFM              ((CapClientPrim)0x0036u)
#define CAP_CLIENT_SET_PARAM_CFM                               ((CapClientPrim)0x0037u)
#define CAP_CLIENT_LOCK_STATE_IND                              ((CapClientPrim)0x0038u)
#define CAP_CLIENT_AUDIO_LOCATION_CHANGE_IND                   ((CapClientPrim)0x0039u)
#define CAP_CLIENT_UNICAST_STOP_STREAM_IND                     ((CapClientPrim)0x003Au)


/* CAP Status Codes*/

typedef uint16 CapClientResult;
#define CAP_CLIENT_RESULT_SUCCESS                             ((CapClientResult)0x0000)
#define CAP_CLIENT_RESULT_INVALID_OPERATION                   ((CapClientResult)0x0001)
#define CAP_CLIENT_RESULT_UNSUPPORTED_CAPABILITY              ((CapClientResult)0x0002)
#define CAP_CLIENT_RESULT_NOT_SUPPORTED                       ((CapClientResult)0x0003)
#define CAP_CLIENT_RESULT_INVALID_PARAMETER                   ((CapClientResult)0x0004)
#define CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR                 ((CapClientResult)0x0005)
#define CAP_CLIENT_RESULT_FAILURE_BAP_ERR                     ((CapClientResult)0x0006)
#define CAP_CLIENT_RESULT_FAILURE_CSIP_ERR                    ((CapClientResult)0x0007)
#define CAP_CLIENT_RESULT_FAILURE_VCP_ERR                     ((CapClientResult)0x0008)
#define CAP_CLIENT_RESULT_INVALID_ROLE                        ((CapClientResult)0x0009)
#define CAP_CLIENT_RESULT_STREAM_ALREADY_ACTIVATED            ((CapClientResult)0x000A)
#define CAP_CLIENT_RESULT_FAILURE_DICOVERY_ERR                ((CapClientResult)0x000B)
#define CAP_CLIENT_RESULT_NULL_INSTANCE                       ((CapClientResult)0x000C)
#define CAP_CLIENT_RESULT_INVALID_GROUPID                     ((CapClientResult)0x000D)
#define CAP_CLIENT_RESULT_INCORRECT_SIRK                      ((CapClientResult)0x000E)
#define CAP_CLIENT_RESULT_TASK_NOT_REGISTERED                 ((CapClientResult)0x000F)
#define CAP_CLIENT_RESULT_INSUFFICIENT_RESOURCES              ((CapClientResult)0x0010)
#define CAP_CLIENT_RESULT_INPROGRESS                          ((CapClientResult)0x0011)
#define CAP_CLIENT_RESULT_CONTEXT_UNAVAILABLE                 ((CapClientResult)0x0012)
#define CAP_CLIENT_RESULT_ALREADY_CONFIGURED                  ((CapClientResult)0x0013)
#define CAP_CLIENT_RESULT_NOT_CONFIGURED                      ((CapClientResult)0x0014)
#define CAP_CLIENT_RESULT_CAPABILITIES_NOT_DISCOVERED         ((CapClientResult)0x0015)
#define CAP_CLIENT_RESULT_PROFILES_NOT_INITIALIZED            ((CapClientResult)0x0016)
#define CAP_CLIENT_RESULT_STREAM_NOT_ACTIVE                   ((CapClientResult)0x0017)
#define CAP_CLIENT_RESULT_CAP_BUSY                            ((CapClientResult)0x0018)
#define CAP_CLIENT_RESULT_VOLUME_REQ_FLUSHED                  ((CapClientResult)0x0019)
#define CAP_CLIENT_RESULT_TIMEOUT_MISSING_NTF_FROM_REMOTE     ((CapClientResult)0x001A)
#define CAP_CLIENT_RESULT_SOURCE_STREAM_ALREADY_ACTIVATED     ((CapClientResult)0x001B)
#define CAP_CLIENT_RESULT_INVALID_BROADCAST_ASSISTANT_STATE   ((CapClientResult)0x001C)
#define CAP_CLIENT_RESULT_SUCCESS_DISCOVERY_ERR               ((CapClientResult)0x001D)
#define CAP_CLIENT_RESULT_FAILURE_MICP_ERR                    ((CapClientResult)0x001E)
#define CAP_CLIENT_RESULT_CSIP_LOCK_UNAVAILABLE               ((CapClientResult)0x001F)
#define CAP_CLIENT_RESULT_SUCCESS_SIRK_DECRYPT_ERR            ((CapClientResult)0x0020)


typedef uint8 CapClientCisHandleDirection;
#define CAP_CLIENT_DATAPATH_INPUT              ((CapClientCisHandleDirection)0x0001)
#define CAP_CLIENT_DATAPATH_OUTPUT             ((CapClientCisHandleDirection)0x0002)

typedef uint8 CapClientProfile;
#define CAP_CLIENT_PROFILE_PACS                ((CapClientProfile)(BAP_CLIENT_PROFILE_PACS))
#define CAP_CLIENT_PROFILE_ASCS                ((CapClientProfile)(BAP_CLIENT_PROFILE_ASCS))
#define CAP_CLIENT_PROFILE_BASS                ((CapClientProfile)(BAP_CLIENT_PROFILE_BASS))
#define CAP_CLIENT_PROFILE_VCP                 ((CapClientProfile)(BAP_CLIENT_PROFILE_VCP))
#define CAP_CLIENT_PROFILE_CSIP                ((CapClientProfile)(BAP_CLIENT_PROFILE_CSIP))
#define CAP_CLIENT_PROFILE_MICP                ((CapClientProfile)(0x06))

typedef uint8 CapClientIsoGroupType;
#define CAP_CLIENT_CIG                         ((CapClientIsoGroupType)(0x01))
#define CAP_CLIENT_BIG                         ((CapClientIsoGroupType)(0x02))

/* Bit mask to determine Which Configuration is supported/being configured*/
/* The configurations without codec Prefix
 * will be standard Settings and the vendor specfic settings will have
 * codec Prefix in the name. For more details refer BAP specification*/
typedef uint32 CapClientSreamCapability;
#define CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN   ((CapClientSreamCapability)0x00000000)
#define CAP_CLIENT_STREAM_CAPABILITY_8_1       ((CapClientSreamCapability)0x00000001)
#define CAP_CLIENT_STREAM_CAPABILITY_8_2       ((CapClientSreamCapability)0x00000002)
#define CAP_CLIENT_STREAM_CAPABILITY_16_1      ((CapClientSreamCapability)0x00000004)
#define CAP_CLIENT_STREAM_CAPABILITY_16_2      ((CapClientSreamCapability)0x00000008)
#define CAP_CLIENT_STREAM_CAPABILITY_24_1      ((CapClientSreamCapability)0x00000010)
#define CAP_CLIENT_STREAM_CAPABILITY_24_2      ((CapClientSreamCapability)0x00000020)
#define CAP_CLIENT_STREAM_CAPABILITY_32_1      ((CapClientSreamCapability)0x00000040)
#define CAP_CLIENT_STREAM_CAPABILITY_32_2      ((CapClientSreamCapability)0x00000080)
#define CAP_CLIENT_STREAM_CAPABILITY_441_1     ((CapClientSreamCapability)0x00000100)
#define CAP_CLIENT_STREAM_CAPABILITY_441_2     ((CapClientSreamCapability)0x00000200)
#define CAP_CLIENT_STREAM_CAPABILITY_48_1      ((CapClientSreamCapability)0x00000400)
#define CAP_CLIENT_STREAM_CAPABILITY_48_2      ((CapClientSreamCapability)0x00000800)
#define CAP_CLIENT_STREAM_CAPABILITY_48_3      ((CapClientSreamCapability)0x00001000)
#define CAP_CLIENT_STREAM_CAPABILITY_48_4      ((CapClientSreamCapability)0x00002000)
#define CAP_CLIENT_STREAM_CAPABILITY_48_5      ((CapClientSreamCapability)0x00004000)
#define CAP_CLIENT_STREAM_CAPABILITY_48_6      ((CapClientSreamCapability)0x00008000)

#define CAP_CLIENT_STREAM_CAPABILITY_96        ((CapClientSreamCapability)0x00020000)


/* Bit masks to represent Vendor Specific Codec. Here CODEC_NONE refers to standard Lc3 codec */
/* Here Most Significant byte of CapClientSreamCapability is used to represent Vendor Specific codec IDs */
/* Which is out of scope for application */
#define CAP_CLIENT_STREAM_CAPABILITY_LC3_EPC   ((CapClientSreamCapability)0x10000000)
#define CAP_CLIENT_STREAM_CAP_VS_APTX_LITE     ((CapClientSreamCapability)0x20000000)
#define CAP_CLIENT_STREAM_CAP_VS_APTX_ADAPTIVE ((CapClientSreamCapability)0x40000000)
#define CAP_CLIENT_CODEC_ID_MASK               ((CapClientSreamCapability)0xFF000000)
#define CAP_CLIENT_STREAM_CAP_CODEC_NONE       ((CapClientSreamCapability)0x00000000)


#define CAP_CLIENT_STREAM_CAPABILITY_APTX_LITE_48_1  (CAP_CLIENT_STREAM_CAP_VS_APTX_LITE | CAP_CLIENT_STREAM_CAPABILITY_48_1)
#define CAP_CLIENT_STREAM_CAPABILITY_APTX_LITE_16_1  (CAP_CLIENT_STREAM_CAP_VS_APTX_LITE | CAP_CLIENT_STREAM_CAPABILITY_16_1)

#define CAP_CLIENT_STREAM_CAPABILITY_APTX_ADAPTIVE_48_1  (CAP_CLIENT_STREAM_CAP_VS_APTX_ADAPTIVE | CAP_CLIENT_STREAM_CAPABILITY_48_1)
#define CAP_CLIENT_STREAM_CAPABILITY_APTX_ADAPTIVE_96    (CAP_CLIENT_STREAM_CAP_VS_APTX_ADAPTIVE | CAP_CLIENT_STREAM_CAPABILITY_96)

#define CAP_CLIENT_VS_COMPANY_ID                                 BAP_VS_COMPANY_ID                       
#define CAP_CLIENT_VS_CODEC_ID_APTX                              BAP_VS_CODEC_ID_APTX
#define CAP_CLIENT_VS_CODEC_ID_APTX_ADAPTIVE                     BAP_VS_CODEC_ID_APTX_ADAPTIVE
#define CAP_CLIENT_VS_CODEC_ID_APTX_LITE                         BAP_VS_CODEC_ID_APTX_LITE

/* Bit mask to determine which BroadcastType to configure
 * CAP_CLIENT_NON_PUBLIC_BROADCAST is to be given by default and/or
 * App can give CAP_CLIENT_SQ_PUBLIC_BROADCAST or/and CAP_CLIENT_HQ_PUBLIC_BROADCAST 
 * based on broadcast preferences of SRC
*/

typedef uint32 CapClientBcastType;
#define CAP_CLIENT_NON_PUBLIC_BROADCAST        ((CapClientBcastType)(NON_PUBLIC_BROADCAST))
#define CAP_CLIENT_SQ_PUBLIC_BROADCAST         ((CapClientBcastType)(SQ_PUBLIC_BROADCAST))
#define CAP_CLIENT_HQ_PUBLIC_BROADCAST         ((CapClientBcastType)(HQ_PUBLIC_BROADCAST))
#define CAP_CLIENT_TMAP_BROADCAST              ((CapClientBcastType)(TMAP_BROADCAST))
#define CAP_CLIENT_GMAP_BROADCAST              ((CapClientBcastType)(GMAP_BROADCAST))

typedef uint16 CapClientBcastSrcLocation;
#define CAP_CLIENT_BCAST_SRC_COLLOCATED       ((CapClientBcastSrcLocation)0x0001)
#define CAP_CLIENT_BCAST_SRC_NON_COLLOCATED   ((CapClientBcastSrcLocation)0x0002)
#define CAP_CLIENT_BCAST_SRC_ALL              ((CapClientBcastSrcLocation)0x0003)
/* audio use cases*/

typedef BapAseType CapClientAseType;
#define CAP_CLIENT_ASE_SINK                    ((CapClientAseType)0x01)
#define CAP_CLIENT_ASE_SOURCE                  ((CapClientAseType)0x02)

typedef uint8 CapClientAseState;
#define CAP_CLIENT_STATE_DISABLING             ((CapClientAseState)0x00)
#define CAP_CLIENT_STATE_RELEASING             ((CapClientAseState)0x01)
#define CAP_CLIENT_ASE_STATE_INVALID           ((CapClientAseState)0xFF)


typedef uint8 CapClientPaSyncState;
#define CAP_CLIENT_PA_SYNC_NOT_SYNCHRONIZE         (0x00u)  /*!> Not synchronize to PA*/
#define CAP_CLIENT_PA_SYNC_SYNCHRONIZE_PAST        (0x01u)  /*!> Synchronize to PA - PAST available */
#define CAP_CLIENT_PA_SYNC_SYNCHRONIZE_NO_PAST     (0x02u)  /*!> Synchronize to PA - PAST no available*/
#define CAP_CLIENT_PA_SYNC_LOST                    (0x03u)

typedef uint32 CapClientAvailableContext;
typedef uint32 CapClientSupportedContext;

typedef uint16 CapClientContext;
#define CAP_CLIENT_CONTEXT_TYPE_PROHIBITED           ((CapClientContext)(0x0000))
#define CAP_CLIENT_CONTEXT_TYPE_UNSPECIFIED          ((CapClientContext)(0x0001))          /* Unspecified. Matches any audio content. */
#define CAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL       ((CapClientContext)(0x0002))   /* Phone Call, Conversation between humans */
#define CAP_CLIENT_CONTEXT_TYPE_MEDIA                ((CapClientContext)(0x0004))            /* Music, Radio, Podcast, Video Soundtrack or TV audio */
#define CAP_CLIENT_CONTEXT_TYPE_GAME                 ((CapClientContext)(0x0008))             /* Audio associated with gaming */
#define CAP_CLIENT_CONTEXT_TYPE_INSTRUCTIONAL        ((CapClientContext)(0x0010))    /* Satnav, User Guidance, Traffic Announcement */
#define CAP_CLIENT_CONTEXT_TYPE_VOICE_ASSISTANT      ((CapClientContext)(0x0020))  /* Virtual Assistant, Voice Recognition */
#define CAP_CLIENT_CONTEXT_TYPE_LIVE                 ((CapClientContext)(0x0040))             /* Live Audio */
#define CAP_CLIENT_CONTEXT_TYPE_SOUND_EFFECTS        ((CapClientContext)(0x0080))    /* Sound effects including keyboard and touch feedback;
                                                                                                                menu and user interface sounds; and other system sounds */
#define CAP_CLIENT_CONTEXT_TYPE_NOTIFICATIONS        ((CapClientContext)(0x0100))    /* Incoming Message Alert, Keyboard Click */
#define CAP_CLIENT_CONTEXT_TYPE_RINGTONE             ((CapClientContext)(0x0200))         /* Incoming Call */
#define CAP_CLIENT_CONTEXT_TYPE_ALERTS               ((CapClientContext)(0x0400))           /* Low Battery Warning, Alarm Clock, Timer Expired */
#define CAP_CLIENT_CONTEXT_TYPE_EMERGENCY_ALARM      ((CapClientContext)(0x0800))
#define CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC        ((CapClientContext)(0x8000))                            /* Vendor Specific Context: GAMING */

/* Discover Capability type */
typedef uint8 CapClientPublishedCapability;
#define CAP_CLIENT_PUBLISHED_CAPABILITY_PAC_RECORD         ((CapClientPublishedCapability)0x01)
#define CAP_CLIENT_PUBLISHED_CAPABILITY_AUDIO_LOC          ((CapClientPublishedCapability)0x02)
#define CAP_CLIENT_PUBLISHED_CAPABILITY_SUPPORTED_CONTEXT  ((CapClientPublishedCapability)0x04)
#define CAP_CLIENT_DISCOVER_ASE_STATE                      ((CapClientPublishedCapability)0x08)
#define CAP_CLIENT_PUBLISHED_CAPABILITY_ALL                ((CapClientPublishedCapability)0x0F)

typedef uint8 CapClientCsipType;
#define CAP_CLIENT_CSIP_LOCK                        ((CapClientCsipType)0x01)
#define CAP_CLIENT_CSIP_SIZE                        ((CapClientCsipType)0x02)
#define CAP_CLIENT_CSIP_SIRK                        ((CapClientCsipType)0x04)
#define CAP_CLIENT_CSIP_RANK                        ((CapClientCsipType)0x08)

typedef uint8 CapClientRole;
#define CAP_CLIENT_INITIATOR                        ((CapClientRole)0x01)
#define CAP_CLIENT_COMMANDER                        ((CapClientRole)0x02)

typedef uint8 CapClientTargetLatency;
#define CAP_CLIENT_TARGET_LOWER_LATENCY                     ((CapClientTargetLatency)(BAP_TARGET_LOWER_LATENCY))
#define CAP_CLIENT_TARGET_BALANCE_LATENCY_AND_RELIABILITY   ((CapClientTargetLatency)(BAP_TARGET_BALANCE_LATENCY_AND_RELIABILITY))
#define CAP_CLIENT_TARGET_HIGH_RELIABILITY                  ((CapClientTargetLatency)(BAP_TARGET_HIGHER_RELIABILITY))

#define CAP_BCAST_EXT_SCAN_AD_FLAGS_NO_FILTER     (DM_ULP_EXT_SCAN_AD_FLAGS_NO_FILTER)
#define CAP_BCAST_EXT_SCAN_AD_FLAGS_PRESENT       (DM_ULP_EXT_SCAN_AD_FLAGS_PRESENT)
#define CAP_BCAST_EXT_SCAN_AD_FLAGS_GEN_AND_LIM   (DM_ULP_EXT_SCAN_AD_FLAGS_GEN_AND_LIM)
#define CAP_BCAST_EXT_SCAN_AD_FLAGS_LIM           (DM_ULP_EXT_SCAN_AD_FLAGS_LIM)



typedef BapSamplingFrequency CapClientSamplingFrequency;
#define CAP_CLIENT_SAMPLING_FREQUENCY_RFU          ((CapClientSamplingFrequency)(BAP_SAMPLING_FREQUENCY_RFU))
#define CAP_CLIENT_SAMPLING_FREQUENCY_8kHz         ((CapClientSamplingFrequency)(BAP_SAMPLING_FREQUENCY_8kHz))
#define CAP_CLIENT_SAMPLING_FREQUENCY_11_025kHz    ((CapClientSamplingFrequency)(BAP_SAMPLING_FREQUENCY_11_025kHz))
#define CAP_CLIENT_SAMPLING_FREQUENCY_16kHz        ((CapClientSamplingFrequency)(BAP_SAMPLING_FREQUENCY_16kHz))
#define CAP_CLIENT_SAMPLING_FREQUENCY_22_05kHz     ((CapClientSamplingFrequency)(BAP_SAMPLING_FREQUENCY_22_05kHz))
#define CAP_CLIENT_SAMPLING_FREQUENCY_24kHz        ((CapClientSamplingFrequency)(BAP_SAMPLING_FREQUENCY_24kHz))
#define CAP_CLIENT_SAMPLING_FREQUENCY_32kHz        ((CapClientSamplingFrequency)(BAP_SAMPLING_FREQUENCY_32kHz))
#define CAP_CLIENT_SAMPLING_FREQUENCY_44_1kHz      ((CapClientSamplingFrequency)(BAP_SAMPLING_FREQUENCY_44_1kHz))
#define CAP_CLIENT_SAMPLING_FREQUENCY_48kHz        ((CapClientSamplingFrequency)(BAP_SAMPLING_FREQUENCY_48kHz))
#define CAP_CLIENT_SAMPLING_FREQUENCY_96kHz        ((CapClientSamplingFrequency)(BAP_SAMPLING_FREQUENCY_96kHz))

typedef BapFrameDuration CapClientFrameDuration;
#define CAP_CLIENT_SUPPORTED_FRAME_DURATION_NONE   ((CapClientFrameDuration)(BAP_SUPPORTED_FRAME_DURATION_NONE))
#define CAP_CLIENT_SUPPORTED_FRAME_DURATION_7P5MS  ((CapClientFrameDuration)(BAP_SUPPORTED_FRAME_DURATION_7P5MS))
#define CAP_CLIENT_SUPPORTED_FRAME_DURATION_10MS   ((CapClientFrameDuration)(BAP_SUPPORTED_FRAME_DURATION_10MS))
#define CAP_CLIENT_PREFERRED_FRAME_DURATION_7P5MS  ((CapClientFrameDuration)(BAP_PREFERRED_FRAME_DURATION_7P5MS))
#define CAP_CLIENT_PREFERRED_FRAME_DURATION_10MS   ((CapClientFrameDuration)(BAP_PREFERRED_FRAME_DURATION_10MS))

typedef BapAudioChannelCount CapClientAudioChannelCount;
#define CAP_CLIENT_AUDIO_CHANNEL_NONE              ((CapClientAudioChannelCount)(BAP_AUDIO_CHANNEL_NONE))
#define CAP_CLIENT_AUDIO_CHANNEL_1                 ((CapClientAudioChannelCount)(BAP_AUDIO_CHANNEL_1))
#define CAP_CLIENT_AUDIO_CHANNELS_2                ((CapClientAudioChannelCount)(BAP_AUDIO_CHANNELS_2))
#define CAP_CLIENT_AUDIO_CHANNELS_3                ((CapClientAudioChannelCount)(BAP_AUDIO_CHANNELS_3))
#define CAP_CLIENT_AUDIO_CHANNELS_4                ((CapClientAudioChannelCount)(BAP_AUDIO_CHANNELS_4))
#define CAP_CLIENT_AUDIO_CHANNELS_5                ((CapClientAudioChannelCount)(BAP_AUDIO_CHANNELS_5))
#define CAP_CLIENT_AUDIO_CHANNELS_6                ((CapClientAudioChannelCount)(BAP_AUDIO_CHANNELS_6))
#define CAP_CLIENT_AUDIO_CHANNELS_7                ((CapClientAudioChannelCount)(BAP_AUDIO_CHANNELS_7))
#define CAP_CLIENT_AUDIO_CHANNELS_8                ((CapClientAudioChannelCount)(BAP_AUDIO_CHANNELS_8))

typedef BapAudioLocation CapClientAudioLocation;
#define CAP_CLIENT_AUDIO_LOCATION_MONO             ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_MONO))
#define CAP_CLIENT_AUDIO_LOCATION_FL               ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_FL))
#define CAP_CLIENT_AUDIO_LOCATION_FR               ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_FR))
#define CAP_CLIENT_AUDIO_LOCATION_FC               ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_FC))
#define CAP_CLIENT_AUDIO_LOCATION_LFE1             ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_LFE1))
#define CAP_CLIENT_AUDIO_LOCATION_BL               ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_BL))
#define CAP_CLIENT_AUDIO_LOCATION_BR               ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_BR))
#define CAP_CLIENT_AUDIO_LOCATION_FLC              ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_FLC))
#define CAP_CLIENT_AUDIO_LOCATION_FRC              ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_FRC))
#define CAP_CLIENT_AUDIO_LOCATION_BC               ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_BC))
#define CAP_CLIENT_AUDIO_LOCATION_LFE2             ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_LFE2))
#define CAP_CLIENT_AUDIO_LOCATION_SIL              ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_SIL))
#define CAP_CLIENT_AUDIO_LOCATION_SIR              ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_SIR))
#define CAP_CLIENT_AUDIO_LOCATION_TPFL             ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_TPFL))
#define CAP_CLIENT_AUDIO_LOCATION_TPFR             ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_TPFR))
#define CAP_CLIENT_AUDIO_LOCATION_TPFC             ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_TPFC))
#define CAP_CLIENT_AUDIO_LOCATION_TPC              ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_TPC))
#define CAP_CLIENT_AUDIO_LOCATION_TPBL             ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_TPBL))
#define CAP_CLIENT_AUDIO_LOCATION_TPBR             ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_TPBR))
#define CAP_CLIENT_AUDIO_LOCATION_TPSIL            ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_TPSIL))
#define CAP_CLIENT_AUDIO_LOCATION_TPSIR            ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_TPSIR))
#define CAP_CLIENT_AUDIO_LOCATION_TPBC             ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_TPBC))
#define CAP_CLIENT_AUDIO_LOCATION_BTFC             ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_BTFC))
#define CAP_CLIENT_AUDIO_LOCATION_BTFL             ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_BTFL))
#define CAP_CLIENT_AUDIO_LOCATION_BTFR             ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_BTFR))
#define CAP_CLIENT_AUDIO_LOCATION_FLW              ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_FLW))
#define CAP_CLIENT_AUDIO_LOCATION_FRW              ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_FRW))
#define CAP_CLIENT_AUDIO_LOCATION_LS               ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_LS))
#define CAP_CLIENT_AUDIO_LOCATION_RS               ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_RS))
#define CAP_CLIENT_AUDIO_LOCATION_RFU              ((CapClientAudioLocation)(BAP_AUDIO_LOCATION_RFU))

typedef uint8 CapClientCigConfigMode;
#define CAP_CLIENT_CIG_CONFIG_MODE_DEFAULT ((CapClientCigConfigMode )0x00)
#define CAP_CLIENT_CIG_CONFIG_MODE_QHS     ((CapClientCigConfigMode )0x01) 
#define CAP_CLIENT_MODE_JOINT_STEREO       ((CapClientCigConfigMode )0x02) 

#ifdef CAP_CLIENT_IOP_TEST_MODE
#define CAP_CLIENT_IOP_TEST_CONFIG_MODE    ((CapClientCigConfigMode )0x04) 
#endif

typedef uint8 CapClientBigConfigMode;
#define CAP_CLIENT_BIG_CONFIG_MODE_DEFAULT      ((CapClientBigConfigMode )CAP_CLIENT_CIG_CONFIG_MODE_DEFAULT)
#define CAP_CLIENT_BIG_CONFIG_MODE_QHS          ((CapClientBigConfigMode )CAP_CLIENT_CIG_CONFIG_MODE_QHS)
#define CAP_CLIENT_BIG_CONFIG_MODE_JOINT_STEREO ((CapClientBigConfigMode )CAP_CLIENT_MODE_JOINT_STEREO)

typedef BapBigConfigParam CapClientBigConfigParam;
typedef BapSetupDataPath  CapClientBcastSetupDatapath;
typedef BapBroadcastSrcAdvParams CapClientBcastSrcAdvParams;
typedef BapBigParam CapClientBigParam;
typedef BapBigSubgroup CapClientBapBigSubgroup;

typedef uint8 CapClientPhy;
/* Defined values for Target PHY */
#define CAP_LE_1M_PHY                   ((CapClientPhy)BAP_LE_1M_PHY)
#define CAP_LE_2M_PHY                   ((CapClientPhy)BAP_LE_2M_PHY)
#define CAP_LE_CODED_PHY                ((CapClientPhy)BAP_LE_CODED_PHY)

typedef uint8 CapClientMicState;

typedef uint32 CapClientOptionalServices;
/* Defined values for Optional Services, as on new services will come need to add bit mask for those here */
#define CAP_CLIENT_OPTIONAL_SERVICE_INVALID_SERVICE  ((CapClientOptionalServices)0x00000000)
#define CAP_CLIENT_OPTIONAL_SERVICE_MICP             ((CapClientOptionalServices)0x00000001)
#define CAP_CLIENT_OPTIONAL_SERVICE_ALL              ((CapClientOptionalServices)0xFFFFFFFF)

typedef uint8  CapClientParamType;
#define CAP_CLIENT_PARAM_TYPE_NONE              (CapClientParamType)0x00
#define CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT   (CapClientParamType)0x01
#define CAP_CLIENT_PARAM_TYPE_BCAST_CONFIG      (CapClientParamType)0x02
#define CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT_V1  (CapClientParamType)0x03
/* 04 - ff reserved for future use */

typedef uint8 CapClientDeviceLockStatus;
#define CAP_CLIENT_DEVICE_STATUS_UNLOCKED            ((CapClientDeviceLockStatus)0x01)
#define CAP_CLIENT_DEVICE_STATUS_LOCKED              ((CapClientDeviceLockStatus)0x02)

typedef struct
{
    uint8                      rtnCtoP;                        /* Unicast Retransmission Number*/
    uint16                     sduSizeCtoP;                    /* Max SDU size, double in case of Joint stereo */
    uint8                      rtnPtoC;                        /* Unicast Retransmission Number*/
    uint16                     sduSizePtoC;                    /* Max SDU size, double in case of Joint stereo */
    uint8                      codecBlocksPerSdu;
    uint8                      phy;
    uint16                     maxLatencyPtoC;                 /* Unicast Transport Latency in microseconds */
    uint16                     maxLatencyCtoP;                 /* Unicast Transport Latency in micro seconds */
    uint32                     sduInterval;                    /* Sdu Interval in milli seconds */
}CapClientUnicastConnectParam;

typedef struct
{
    uint32                     sduIntervalCtoP;               /* Time interval between the start of consecutive SDUs, in milli seconds */
    uint32                     sduIntervalPtoC;               /* Time interval between the start of consecutive SDUs, in milli seconds */
    uint16                     maxLatencyPtoC;                /* Unicast Transport Latency in microseconds */
    uint16                     maxLatencyCtoP;                /* Unicast Transport Latency in micro seconds */
    uint16                     sduSizeCtoP;                   /* Max SDU size, double in case of Joint stereo */
    uint16                     sduSizePtoC;                   /* Max SDU size, double in case of Joint stereo */
    uint8                      packing;                       /* Interleaved, Sequential placement of packets */
    uint8                      framing;                       /* Framed, Unframed */
    uint8                      sca;                           /* sleep clock accuracy */
    uint8                      rtnCtoP;                       /* Unicast Retransmission Number from central to peripheral*/
    uint8                      rtnPtoC;                       /* Unicast Retransmission Number peripheral to central*/
    uint8                      phyCtoP;                       /* PHY from central */
    uint8                      phyPtoC;                       /* PHY from peripheral */
    uint8                      codecBlocksPerSdu;             /* Supported Max Codec Frame Per SDU */
    uint8                      vsConfigLen;                   /* Total Length of Vendor specific Config pairs  */
    uint8                     *vsConfig;                     /* LTV pair(s) for any vendor or proprietary info  */
} CapClientUnicastConnectParamV1;

typedef struct
{
    uint8                      rtn;                    /* Unicast Retransmission Number*/
    uint16                     sduSize;                /* Max SDU size */
    uint8                      maxCodecFramesPerSdu;
    CapClientTargetLatency     targetLatency;
    uint16                     maxLatency;             /* Unicast Transport Latency */
    uint16                     phy;
    uint32                     sduInterval;            /* Sdu Interval */
    CapClientSreamCapability   subGroupConfig;
} CapClientBcastConfigParam;

typedef struct
{
    uint32 bisIndex;          /* Bit mask*/
    uint8  metadataLen;
    uint8* metadataValue;
}CapClientSubgroupInfo;

typedef struct
{
    uint32 cid;
    uint8  sourceId;
}CapClientDelegatorInfo;

typedef struct
{
    CapClientSreamCapability   config;                  /*<! Setting which needs to be configured per BIS*/
    uint32                     audioLocation;
    uint8                      targetLatency;
    uint8                      lc3BlocksPerSdu;         /*<! Default value = 1*/
}CapClientBisInfo;

typedef struct
{
    CapClientSreamCapability   config;                  /*<! Setting which needs to be configured*/
    uint8                      numBis;
    uint8                      targetLatency;           /*<! low latency or high reliability */
    uint8                      lc3BlocksPerSdu;         /*<! Default value = 1*/
    CapClientContext           useCase;                 /* Context type of Audio/Usecase */
    uint8                      metadataLen;             /* Total length of vendor metadata */
    uint8                      *metadata;                /* LTV format */
    CapClientBisInfo           bisInfo[BAP_MAX_NUMBER_BIS];
}CapClientBigSubGroup;

typedef struct {
    BroadcastType        broadcast;
    uint16               flags; /* Refer flags defines above */
    uint16               appearanceValue;
    uint8                bigNameLen;
    uint8*               bigName;
}CapClientBcastInfo;

typedef struct
{
    uint8                framing;
    uint8                phy;    /* PHY */
    uint8                rtn;    /* Retransmission number */
} CapClientQhsBigConfig;

typedef struct
{
    uint8            setSize;    /*!<  Size of the Co ordinated Set*/
    uint8            sirkType;   /*!< Type of SIRK used */
    uint8            sirk[CSIP_SIRK_SIZE];   /*!< SIRK */
} CapClientCoordinatedSetAttributes;

typedef struct
{
    uint32           cid;        /*!< btConnId */
    CapClientResult  result;     /*!< Device Status */
} CapClientDeviceStatus;

typedef struct
{
    BapPacsClientDeviceData  *pacsData;     /*!< PACS handles */
    BapAscsClientDeviceData  *ascsData;     /*!< ASCS handles */
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
    BapBassClientDeviceData  *bassData;     /*!< BASS handles */
#endif
    GattVcsClientDeviceData  *vcsData;      /*!< VCS handles */
    GattCsisClientDeviceData *csisData;     /*!< CSIS handles */
} CapClientHandleList;

typedef struct
{
    uint32                cid;               /*!< List of connection ID's*/
    CapClientHandleList   *handles;          /*!< Characateristic handles */
} CapClientInitData;

typedef struct
{
    CapClientAseType           direction;  /*!< Source/Sink */
    CapClientSreamCapability   capability; /*!< Bitmask indicating streaming capabilities */
    CapClientContext           context;    /*!< preferredAudioContext */
    uint16                     minOctetsPerCodecFrame;
    uint16                     maxOctetsPerCodecFrame;
    CapClientAudioChannelCount channelCount;
    uint8                      supportedMaxCodecFramesPerSdu;
    CapClientFrameDuration     frameDuaration;
    uint8                      metadataLen;
    uint8                      *metadata;
} CapClientStreamCapability;

typedef struct
{
    CapClientAseType           direction;       /*!< Source/Sink */
    uint8                      numAses;         /*!< No of ASE(s) for Source/Sink */
    CapClientResult            result;          /*!< CAP return status */
    uint32                     cid;             /*!< BtConnID */
    CapClientAudioLocation     audioLocation;   /*!< Audio Locations */
} CapClientDeviceInfo;

typedef struct
{
    uint32                      cid;         /*!< BtConnID */
    CapClientAvailableContext   context;     /*!< Audio Context */
    CapClientResult             result;      /*!< CAP return status */
} CapClientAvailableAudioContextInfo;

typedef struct
{
    uint16           maxSduCtoP;      /* Maximum SDU Size from master host */
    uint16           maxSduPtoC;      /* Maximum SDU Size from slave host */
    uint16           maxPduCtoP;      /* Maximum PDU Size from master host */
    uint16           maxPduPtoC;      /* Maximum PDU Size from slave host */
    uint8            nse;             /* Max no of sub events for each CIS */
    uint8            phyCtoP;         /* PHY from master */
    uint8            phyPtoC;         /* PHY from slave */
    uint8            bnCtoP;          /* Burst number from master to slave */
    uint8            bnPtoC;          /* Burst number from slave to master */
} CapClientCisTestConfig;

typedef struct
{
   uint16            isoIinterval;
   uint8             ftCtoP;
   uint8             ftPtoC;
   uint8             sca;
   uint8             packing;
   uint8             framing;
   uint24            sduIntervalCtoP;
   uint24            sduIntervalPtoC;
   CapClientCisTestConfig *cisTestConfig;
} CapClientCigTestConfig;

typedef struct
{
    uint8                          cisId;
    CapClientCisHandleDirection    direction;
} CapClientStreamInfo;

typedef struct
{
    uint8                       idCount;
    CapClientStreamInfo         *idInfo;
} CapClientGroupInfo;

/* Upstream Primitives */

typedef struct
{
    CapClientPrim               type;                  /*!< CAP_CLIENT_INIT_CFM */
    ServiceHandle               groupId;               /*!< CAP Handle */
    uint8                       deviceCount;           /*!< Current device Count */
    CapClientResult             result;                /*!< CAP Status */
    uint32                      cid;                   /*!< BtConnID */
    CapClientCoordinatedSetAttributes   *setAttrib;    /*!< Coordinated Set Property, Includes SIRK, SetSize and RANK*/
} CapClientInitCfm;

typedef struct
{
    CapClientPrim               type;                  /*!< CAP_CLIENT_BCAST_SRC_INIT_CFM */
    CapClientResult             result;
    uint32                      bcastSrcProfileHandle;    /*!< Broadcast Source Handle */
} CapClientBcastSrcInitCfm;

typedef struct
{
   CapClientPrim                type;                  /*!< CAP_CLIENT_REMOVE_DEVICE_CFM */
   ServiceHandle                groupId;               /*!< CAP Handle */
   CapClientResult              result;                /*!< CAP Status */
} CapClientRemoveDeviceCfm;

typedef struct
{
    CapClientPrim               type;                  /*!< CAP_CLIENT_ADD_NEW_DEV_CFM */
    ServiceHandle               groupId;               /*!< CAP Handle*/
    uint8                       deviceCount;           /*!< Current device Count */
    CapClientResult             result;                /*!< CAP Status */
} CapClientAddNewDevCfm;

typedef struct
{
    CapClientPrim               type;                  /*!< CAP_CLIENT_INIT_STREAM_CONTROL_CFM */
    ServiceHandle               groupId;               /*!< CAP Handle*/
    CapClientResult             result;                /*!< CAP Status */
    CapClientRole               role;                  /*!< CAP role */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;         /*!< Individual Device Status */
} CapClientInitStreamControlCfm;

typedef struct
{
    CapClientPrim               type;                  /*!< CAP_CLIENT_DISCOVER_AVAILABLE_AUDIO_CONTEXT_CFM */
    ServiceHandle               groupId;               /*!< CAP Handle */
    CapClientResult             status;                /*!< CAP Status */
    uint8                       deviceContextLen;
    CapClientAvailableAudioContextInfo    *deviceContext;  /*!< Available Contexts on all devices
                                                           (Ideally all devices needs to have same context) */
} CapClientDiscoverAvailableAudioContextCfm;

typedef struct
{
    CapClientPrim               type;                   /*!< CAP_CLIENT_DISCOVER_STREAM_CAPABILITIES_CFM */
    ServiceHandle               groupId;                /*!< CAP Handle */

    CapClientResult             result;                 /*!< CAP Status */
    CapClientSupportedContext   supportedContext;       /*!< Supported Audio Context */
    uint8                       streamCapCount;         /*!< Length of capability ptr */
    CapClientStreamCapability   *capability;            /*!< streaming capabilities */
    uint8                       deviceInfoCount;
    CapClientDeviceInfo         *deviceInfo;              /*!< Supported Audio Locations */
} CapClientDiscoverStreamCapabilitiesCfm;

typedef struct
{
    CapClientPrim               type;                  /*!< CAP_CLIENT_UNICAST_CONNECT_CFM */
    ServiceHandle               groupId;               /*!< CAP Handle */
    CapClientContext            context;               /*!< Audio Context */
    CapClientResult             result;                /*!< CAP Status */
    uint8                       numOfMicsConfigured;   /*!< Mics Configured */
    uint8                       cigId;
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;         /*!< Individual Device Status */
} CapClientUnicastConnectCfm;

typedef struct
{
    CapClientPrim               type;                  /*!< CAP_CLIENT_UNICAST_DISCONNECT_CFM */
    ServiceHandle               groupId;               /*!< CAP Handle */
    CapClientResult             result;                /*!< CAP Status */
} CapClientUnicastDisConnectCfm;

typedef struct
{
    CapClientPrim               type;                  /*!< CAP_CLIENT_UNICAST_COMMON_CFM */
    ServiceHandle               groupId;               /*!< CAP Handle */
    CapClientContext            context;               /*!< Audio Context */
    CapClientResult             result;                /*!< CAP Status */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;         /*!< Individual Device Status */
} CapClientUnicastCommonCfm;

typedef struct
{
    CapClientPrim               type;                 /*!< CAP_CLIENT_REGISTER_TASK_CFM */
    ServiceHandle               groupId;              /*!< CAP handle */
    CapClientResult             result;               /*!< CAP Status */
} CapClientRegisterTaskCfm;

typedef struct
{
    CapClientPrim               type;                 /*!< CAP_CLIENT_DEREGISTER_TASK_CFM */
    ServiceHandle               groupId;              /*!< CAP handle */
    CapClientResult             result;               /*!< CAP Status */
} CapClientDeRegisterTaskCfm;

typedef CapClientUnicastCommonCfm CapClientUnicastAudioUpdateCfm;

typedef struct
{
    CapClientPrim               type;                 /*!< CAP_CLIENT_READ_VOLUME_STATE_CFM */
    ServiceHandle               groupId;              /*!< CAP handle */
    CapClientResult             result;               /*!< CAP Status */
    uint32                      cid;                  /*!< btConnId */
    uint8                       volumeSetting;        /*!< current volume setting */
    uint8                       mute;                 /*!< Mute State */
    uint8                       changeCounter;        /*!< Change Counter */
} CapClientReadVolumeStateCfm;

typedef struct
{
    CapClientPrim               type;                 /*!< CAP_CLIENT_READ_MIC_STATE_CFM */
    ServiceHandle               groupId;              /*!< CAP handle */
    CapClientResult             result;               /*!< CAP Status */
    uint32                      cid;                  /*!< btConnId */
    CapClientMicState           micState;             /*!< Mic State */
} CapClientReadMicStateCfm;

typedef struct
{
    uint16                       cisHandle;      /*!< ISO handle  */
    CapClientCisHandleDirection  direction;      /*!< ISO Handle Direction */
    CapClientAudioLocation       audioLocation;  /*!< Configured Audio Location  */
} CapClientCisHandles;

typedef struct
{
    uint8                     sinkFrameDuaration;          /*<! codec frame duration */
    uint8                     sinkLc3BlocksPerSdu;
    uint8                     srcFrameDuaration;          /*<! codec frame duration */
    uint8                     srcLc3BlocksPerSdu;         /*<!  LC3 blocks per SDU */
    uint8                     codecId;                    /*<! Codec ID  */
    uint8                     codecVersionNum;            /*<! Codec Version Info  */
    uint16                    companyId;                  /*<! Comapany ID  */
    uint16                    vendorCodecId;              /*<! Vendor Codec ID  */
    uint16                    sinkOctetsPerFrame;          /*<! Bit rate */
    uint16                    sinkSamplingFrequency;       /*<! Sampling Frequency */
    uint16                    sinkPdelay;                 /*<! Sink ASE presentation delay */
    uint16                    srcSamplingFrequency;       /*<! Sampling Frequency */
    uint16                    srcOctetsPerFrame;          /*<! Bit rate */
    uint16                    srcPdelay;                  /*<! Source ASE presentation delay */
}CapClientAudioConfig;

typedef struct
{
    uint8                     srcVsMetadataLen;
    uint8                     sinkVsMetadataLen;
    uint8*                    srcVsMetadata;       /*!< Reciever/upper layer must free this pointer */
    uint8*                    sinkVsMetadata;      /*!< Reciever/upper layer must free this pointer */
}CapClientVsMetadata;

typedef struct
{
    CapClientPrim               type;            /*!< CAP_CLIENT_UNICAST_START_STREAM_IND */
    ServiceHandle               groupId;         /*!< handle of CAP */
    uint32                      cid;             /*!< BtConnID */ 
    CapClientResult             result;          /*!< CAP Status */
    CapClientAudioConfig        *audioConfig;
    CapClientCisHandles         *cishandles;      /*!< Unicast ISO handles */
    CapClientVsMetadata         *vsMetadata;      /*!< Reciever/upper layer must free this pointer */
    uint8                       cisCount;
    uint8                       cigId;
} CapClientUnicastStartStreamInd;

/* When released is CAP_CLIENT_STATE_DISABLING it means Remote is in QOS config state.
 * Application can call start stream directly for the given use case o start.
 * If released state is CAP_CLIENT_STATE_RELEASING it means Remote is in Idle/Codec Config.
 * Application need to call unicast connect to start the use case again 
 * Other than these values Remote state is not known and App may release the given use case */
typedef struct
{
    CapClientPrim               type;            /*!< CAP_CLIENT_UNICAST_STOP_STREAM_IND */
    ServiceHandle               groupId;         /*!< handle of CAP */
    uint32                      cid;             /*!< BtConnID */
    uint16                      *cishandles;      /*!< Unicast ISO handles */
    uint8                       cisCount;
    CapClientAseState           released;        /*!< Refer CapClientAseState for values */
    uint8                       cigId;           /*!< CIG id for the stopped use case */
} CapClientUnicastStopStreamInd;


typedef struct
{
    CapClientPrim               type;            /*!< CAP_CLIENT_UNICAST_START_STREAM_CFM */
    ServiceHandle               groupId;         /*!< handle of CAP */
    CapClientResult             result;          /*!< CAP Status */
} CapClientUnicastStartStreamCfm;

typedef struct
{
    CapClientPrim               type;            /*!< CAP_CLIENT_UNICAST_STOP_STREAM_CFM */
    ServiceHandle               groupId;         /*!< handle of CAP */
    bool                        released;        /*!< If release Done */
    CapClientResult             result;          /*!< CAP Status */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;    /*!< Individual Device Status */
} CapClientUnicastStopStreamCfm;

typedef struct
{
    CapClientPrim               type;            /*!< CAP_CLIENT_VCP_COMMON_CFM */
    ServiceHandle               groupId;         /*!< handle of CAP */
    CapClientResult             result;          /*!< CAP Status */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;   /*!< Individual Device Status */
} CapClientVcpCommonCfm;

typedef CapClientVcpCommonCfm CapClientChangeVolumeCfm;
typedef CapClientVcpCommonCfm CapClientMuteCfm;

typedef struct
{
    uint8           framing;
    uint8           phyCtoP;    /* PHY from central */
    uint8           phyPtoC;    /* PHY from peripheral */
    uint8           rtnCtoP;    /* Retransmission number from central to peripheral */
    uint8           rtnPtoC;    /* Retransmission number from peripheral to central */
} CapClientQhsConfig;

typedef struct
{
    CapClientPrim               type;                     /*!< CAP_CLIENT_BCAST_SRC_CONFIG_CFM */
    CapClientResult             result;
    uint32                      bcastSrcProfileHandle;    /*!< Broadcast Source Handle */
} CapClientBcastSrcConfigCfm;

typedef struct
{
    uint8                     codecId;                    /*<! Codec ID  */
    uint16                    vendorCodecId;              /*<! Vendor Codec ID  */
    uint16                    companyId;                  /*<! Comapany ID  */
    uint16                    samplingFrequency;          /*<! codec frame duration */
    uint8                     frameDuaration;
    uint32                    audioChannelAllocation;     /*<! codec frame duration */
    uint16                    octetsPerFrame;             /*<!  LC3 blocks per SDU */
}CapClientBcastAudioConfig;

typedef struct
{
    uint8                       numBis;                      /* Number of BISes in subgroup */
    CapClientBcastAudioConfig   *audioConfig;                /* Audioconfig for each BIS's*/
    uint16                      *bisHandles;                 /* Connection handle of BISes */
    uint8                       metadataLen;                 /* Total length of vendor metadata */
    uint8                       *metadata;                   /* LTV format */
}CapClientBcastSubGroupInfo;

typedef struct
{
    CapClientPrim               type;
    CapClientResult             result;
    uint32                      bcastSrcProfileHandle;    /* Broadcast Source Handle */
    uint8                       bigId;
    uint32                      bigSyncDelay;             /* BIG Sync delay */
    CapClientBigParam           *bigParameters;
    uint8                       numSubGroup;              /* Number of subgroups for a given broadcast Source Handle */
    CapClientBcastSubGroupInfo  *subGroupInfo;
}CapClientBcastSrcStartStreamCfm;

typedef struct
{
    CapClientPrim               type;
    CapClientResult             result;                      /*!< CAP Status */
    uint32                      bcastSrcProfileHandle;       /*!< Broadcast Source Handle */
} CapClientBcastCommonCfm;

typedef CapClientBcastCommonCfm CapClientBcastSrcStopStreamCfm;
typedef CapClientBcastCommonCfm CapClientBcastSrcRemoveStreamCfm;
typedef CapClientBcastCommonCfm CapClientBcastSrcUpdateStreamCfm;
typedef CapClientBcastCommonCfm CapClientBcastSrcDeinitCfm;


typedef struct
{
    CapClientPrim               type;
    CapClientResult             result;                      /*!< CAP Status */
    uint32                      profileHandle;       /*!< Broadcast Source Handle/ GroupId */
} CapClientSetParamCfm;

typedef struct
{
    CapClientPrim               type;                     /*!< CAP_CLIENT_BCAST_SETUP_DATAPATH_CFM */
    uint16                      isoHandle;
    uint32                      bcastSrcProfileHandle;    /*!< Broadcast Source Handle */
    CapClientResult             result;
} CapClientBcastSetupDatapathCfm;

typedef struct
{
    CapClientPrim               type;          /*!< CAP_CLIENT_BCAST_ASST_START_SRC_SCAN_CFM */
    ServiceHandle               groupId;       /*!< CAP Handle */
    CapClientResult             result;        /*!< Result code. */
    uint16                      scanHandle;    /*!< scanHandle */
    uint8                       statusLen;
    CapClientDeviceStatus       *status;
} CapClientBcastAsstStartSrcScanCfm;

typedef struct
{
    CapClientPrim               type;          /*!< CAP_CLIENT_BCAST_ASST_START_SRC_SCAN_CFM */
    ServiceHandle               groupId;       /*!< CAP Handle */
    CapClientResult             result;        /*!< Result code. */
    uint8                       statusLen;
    CapClientDeviceStatus*      status;
} CapClientBcastAsstCommonMsg;

typedef CapClientBcastAsstCommonMsg CapClientBcastAsstStopSrcScanCfm;
typedef CapClientBcastAsstCommonMsg CapClientBcastAsstAddSrcCfm;
typedef CapClientBcastAsstCommonMsg CapClientBcastAsstModifySrcCfm;
typedef CapClientBcastAsstCommonMsg CapClientBcastAsstRemoveCfm;
typedef CapClientBcastAsstCommonMsg CapClientBcastAsstNotficationCfm;
typedef CapClientBcastAsstCommonMsg CapClientSetMicStateCfm;

typedef struct
{
    CapClientPrim               type;          /*!< CAP_CLIENT_BCAST_ASST_SYNC_TO_SRC_CFM */
    ServiceHandle               groupId;       /*!< CAP Handle */
    CapClientResult             result;        /*!< Result code. */
    uint16                      syncHandle;    /*!< Sync handle of the PA */
    uint8                       advSid;
    TYPED_BD_ADDR_T             addrt;
    uint8                       advPhy;
    uint16                      periodicAdvInterval;
    uint8                       advClockAccuracy;
} CapClientBcastAsstSyncToSrcStartCfm;

typedef struct
{
    CapClientPrim               type;          /*!< CAP_CLIENT_BCAST_ASST_SYNC_TO_SRC_TERMINATE_CFM */
    ServiceHandle               groupId;       /*!< CAP Handle */
    CapClientResult             result;        /*!< Result code. */
} CapClientBcastAsstSyncCommonCfm;

typedef struct
{
    CapClientPrim               type;          /*!< CAP_CLIENT_BCAST_ASST_SYNC_TO_SRC_TERMINATE_CFM */
    ServiceHandle               groupId;       /*!< CAP Handle */
    CapClientResult             result;        /*!< Result code. */
    uint16                      syncHandle;
} CapClientBcastAsstSyncToSrcTerminateCfm;

typedef CapClientBcastAsstSyncCommonCfm CapClientBcastAsstSyncToSrcCancelCfm;

typedef struct
{
    CapClientPrim               type;          /*!< CAP_CLIENT_BCAST_ASST_SYNC_TO_SRC_TERMINATE_CFM */
    ServiceHandle               groupId;       /*!< CAP Handle */
    CapClientResult             result;        /*!< Result code. */
    uint32                      cid;           /*!< BAP Handle */
} CapClientBcastAsstCommonCfm;

typedef CapClientBcastAsstCommonCfm CapClientBdcastAsstStopReceptionCfm;
typedef CapClientBcastAsstCommonCfm CapClientSetMicpAttribHandlesCfm;

typedef struct
{
    CapClientPrim               type;           /*!< CAP_CLIENT_UNLOCK_COORDINATED_SET_CFM */
    ServiceHandle               groupId;        /*!< CAP Handle */
    CapClientResult             result;         /*!< CAP Status */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;   /*!< Individual Device Status */
} CapClientUnlockCoordinatedSetCfm;

typedef struct
{
    CapClientPrim                type;
    ServiceHandle                groupId;                /*!< CAP Handle */
    CapClientResult              result;                 /*!< CAP Status */
    uint32                       csipChar;
    uint16                       csipSizeValue;          /*! Value size*/
    uint8*                       csipValue;              /*! Read value */
}CapClientCsipReadCfm;

/* This message will be sent to app in case when lock status of any device changes.
 * If the attempt to lock a device fails while performing any procedure,
 * app will get the respective procedure cfm with result as CAP_CLIENT_RESULT_CSIP_LOCK_UNAVAILABLE.
 * Later when the lock status ind is received by app with status as unlocked, it can try the same procedure again */

typedef struct
{
    CapClientPrim               type;            /*!< CAP_CLIENT_LOCK_STATE_IND */
    ServiceHandle               groupId;         /*!< CAP handle */
    uint32                      cid;             /*!< btConnId */
    CapClientDeviceLockStatus   lockState;       /*!< Lock State */
} CapClientLockStateInd;

typedef struct
{
    CapClientPrim                type;
    ServiceHandle                groupId;                /*!< CAP Handle */
    CapClientContext             context;                /*!< Audio Context */
    CapClientResult              result;                 /*!< CAP Status */
} CapClientUnicastCigTestCfm;

 typedef struct
 {
     CapClientPrim                type;
     ServiceHandle                groupId;                /*!< CAP Handle */
     CapClientResult              result;                 /*!< CAP Status */
 } CapClientUnicastSetVsConfigDataCfm;


 /* Notifications */

typedef struct
{
    CapClientPrim               type;            /*!< CAP_CLIENT_AVAILABLE_AUDIO_CONTEXT_IND */
    ServiceHandle               groupId;         /*!< CAP handle */
    CapClientAvailableContext   context;         /*!< Available Audio use case */
} CapClientAvailableAudioContextInd;

typedef struct
{
    CapClientPrim               type;            /*!< CAP_CLIENT_AUDIO_LOCATION_CHANGE_IND */
    ServiceHandle               groupId;         /*!< CAP handle */
    uint32                      cid;             /*!< BTconnId */
    CapClientAudioLocation      audioLocation;   /*!< New Audio Location */
    bool                        sink;            /*!< Sink or Source Audio Location */
} CapClientAudioLocationChangeInd;

typedef struct
{
    CapClientPrim               type;            /*!< CAP_CLIENT_VOLUME_STATE_IND */
    ServiceHandle               groupId;         /*!< CAP handle */
    uint8                       volumeState;
    uint8                       mute;
    uint8                       changeCounter;
    uint32                      cid;             /*!< btConnId */
} CapClientVolumeStateInd;

typedef struct
{
    CapClientPrim               type;                  /*!< CAP_CLIENT_INIT_OPTIONAL_SERVICES_CFM */
    ServiceHandle               groupId;               /*!< CAP Handle*/
    CapClientResult             result;                /*!< CAP Status */
    CapClientOptionalServices   optServices;           /*!< Optional services bit mask */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;         /*!< Individual Device Status */
} CapClientInitOptionalServicesCfm;

typedef struct
{
    CapClientPrim               type;            /*!< CAP_CLIENT_MIC_STATE_IND */
    ServiceHandle               groupId;         /*!< CAP handle */
    uint32                      cid;             /*!< btConnId */
    CapClientMicState           micState;        /*!< Mic State */
} CapClientMicStateInd;

typedef struct
{
    CapClientPrim               type;            /*!< CAP_CLIENT_ACTIVE_GROUP_CHANGED_IND */
    ServiceHandle               activeGroupId;   /*!< CAP handle */
    ServiceHandle               previousGroupId;
} CapClientActiveGroupChangedInd;

/* Indicate to client that cis link loss disconnection happened */

typedef struct
{
    CapClientPrim               type;            /*!< CAP_CLIENT_UNICAST_LINK_LOSS_IND */
    ServiceHandle               activeGroupId;   /*!< CAP handle */
    uint16                      cisHandle;
} CapClientUnicastLinkLossInd;


/* Indicate to client that cis link disconnection happened and app will receive 
 * cis disconnect reason as well */

typedef struct
{
    CapClientPrim               type;            /*!< CAP_CLIENT_UNICAST_DISCONNECT_IND */
    ServiceHandle               activeGroupId;   /*!< CAP handle */
    uint16                      cisHandle;
    uint16                      reason;          /* DISCONNECT_REASON */
} CapClientUnicastDisconnectInd;


/* Indicate to server that PAC records have been changed on remote side to app
 * Discover Audio capabilities needs to be re excercised by app when this message
 * arrives at application*/

typedef struct
{
    CapClientPrim               type;             /*!< CAP_CLIENT_PAC_RECORD_CHANGED_IND */
    ServiceHandle               groupId;          /*!< CAP handle*/
} CapClientPacRecordChangedInd;

typedef struct
{
    CapClientPrim                type;                     /*!< CAP_CLIENT_UPDATE_METADATA_IND */
    CapClientResult              result;                   /*!< Result code. */
    uint16                       streamingAudioContexts;   /*!< Bitmask of Audio Context type values */
    ServiceHandle                groupId;                  /*!< Cap Group Handle */
    uint32                       cid;
    CsrBool                      isSink;
    uint8                        metadataLen;
    uint8                        *metadata;
} CapClientUpdateMetadataInd;

typedef BapBroadcastAssistantSrcReportInd CapClientBcastAsstSrcReportInd;

typedef struct
{
    BapClientPrim               type;            /*!< CAP_CLIENT_BCAST_ASST_BRS_IND */
    ServiceHandle               groupId;         /*!< CAP Handle */
    uint32                      cid;             /*!< BAP Handle */
    uint8                       sourceId;        /*! Source_id of the Broadcast
                                                    Receive State characteristic */
    BD_ADDR_T                   sourceAddress;
    uint8                       advertiseAddType;
    uint8                       advSid;
    CapClientPaSyncState        paSyncState;
    uint8                       bigEncryption;
    uint32                      broadcastId;    /*!< Broadcast ID */
    uint8                       *badCode;
    uint8                       numSubGroups;
    CapClientSubgroupInfo       *subGroupInfo;
} CapClientBcastAsstBrsInd;

typedef struct
{
    CapClientPrim               type;           /*!< CAP_CLIENT_BCAST_ASST_SYNC_LOSS_IND */
    ServiceHandle               groupId;        /*!< CAP Handle */
    uint32                      cid;            /*!< BAP Handle */
    uint16                      syncHandle;
} CapClientBcastAsstSyncLossInd;

typedef struct
{
    CapClientPrim               type;           /*!< CAP_CLIENT_BCAST_ASST_SET_CODE_IND */
    ServiceHandle               groupId;        /*!< CAP Handle */
    uint32                      cid;            /*!< BAP Handle */
    uint8                       sourceId;       /*! Source_id of the Broadcast
                                                   Receive State characteristic */
    uint8                       flags;          /*! See flags above */
} CapClientBcastAsstSetCodeInd;

typedef struct
{
    CapClientPrim               type;           /*!< CAP_CLIENT_BCAST_ASST_READ_BRS_IND */
    ServiceHandle               groupId;        /*!< CAP Handle */
    CapClientResult             result;         /*!< Result code. */
    uint32                      cid;            /*!< BAP Handle */
    uint8                       sourceId;       /*! Source_id of the Broadcast
                                                    Receive State characteristic */
    BD_ADDR_T                   sourceAddress;
    uint8                       advertiseAddType;
    uint8                       advSid;
    uint8                       paSyncState;
    uint8                       bigEncryption;
    uint32                      broadcastId;     /*!< Broadcast ID */
    uint8                       *badCode;
    uint8                       numSubGroups;
    CapClientSubgroupInfo       *subGroupInfo;
} CapClientBcastAsstReadReceiveStateInd;

typedef CapClientBcastAsstCommonCfm CapClientBcastAsstReadReceiveStateCfm;

#endif /* CAP_CLIENT_PRIM_H */
