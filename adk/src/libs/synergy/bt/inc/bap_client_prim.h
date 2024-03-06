/*******************************************************************************

Copyright (C) 2022-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef BAP_CLIENT_PRIM_H
#define BAP_CLIENT_PRIM_H

#include "csr_types.h"
#include "qbl_types.h"
#include "bluetooth.h"

#include "gatt_pacs_client.h"
#include "gatt_ascs_client.h"
#include "gatt_bass_client.h"

#define BAP_CLIENT_PRIM    (SYNERGY_EVENT_BASE + BAP_PRIM)

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Primitive type for bap client*/
typedef uint16 BapClientPrim; /*!< Primitive identity */

typedef uint32 BapProfileHandle; /*!< Profile Handle */

#define BAP_PRIM_BASE   0x0F00    /*!< Reserved for BAP. */

#define BAP_PRIM_DOWN  (BAP_PRIM_BASE)
#define BAP_PRIM_UP    (BAP_PRIM_BASE | 0x0040)
#define BAP_PRIM_MAX   (BAP_PRIM_BASE | 0x00FF)

#define BAP_DIRECTION_FILTER_SINK_MASK   (1u)
#define BAP_DIRECTION_FILTER_SOURCE_MASK (2u)
#define BAP_DIRECTION_FILTER_ALL_MASK    (0u)

#define MAX_PAC_RECORD_ENTRIES (15)
#define MAX_ASE_INFO_ENTRIES (5)
#define MAX_SUPPORTED_CIS                   (0x08)

#define BAP_SAMPLING_FREQUENCY_LENGTH          3
#define BAP_SAMPLING_FREQUENCY_TYPE            0x01
#define BAP_BIT_RATE_LENGTH                    2
#define BAP_BIT_RATE_TYPE                      5
#define BAP_FRAME_DURATION_LENGTH              2
#define BAP_FRAME_DURATION_TYPE                0x0D
#define BAP_ALLOCATED_AUDIO_CHANNEL_LENGTH     2
#define BAP_ALLOCATED_AUDIO_CHANNEL_TYPE       0x0E
#define BAP_OCTET_PER_CODEC_FRAME_LENGTH       5
#define BAP_OCTET_PER_CODEC_FRAME_TYPE         0x0F

/* This is set to 3 for time being and minimum MTU size is 93 for 3 ASES */
/* This needs to be updated for EATT */
#define BAP_MAX_SUPPORTED_ASES (6)

#define ASE_ID_ALL                             0xFF

#define BAP_RECEIVER_START_READY               0x01
#define BAP_RECEIVER_STOP_READY                0x02


/*! \internal The current list of content types have been
 *  updated to match those defined in the BAP_ASCS_IOP test plan
 *  document (version 07r04). This is likely to change again
 *  in the future and should ultimately match whatever values
 *  end up in the SIGs assigned numbers list.
 */

/*! BAP Role
 */
typedef uint32 BapRole;
#define BAP_ROLE_UNICAST_CLIENT             ((BapRole)0x00000001)
#define BAP_ROLE_BROADCAST_SOURCE           ((BapRole)0x00000010)
#define BAP_ROLE_BROADCAST_ASSISTANT        ((BapRole)0x00000100)

#define INVALID_CONNID                       0xFFFFFFFFu

/*! BAP Result
 */
typedef uint16 BapResult;
#define BAP_RESULT_SUCCESS                  ((BapResult)0x0000)
#define BAP_RESULT_INVALID_OPERATION        ((BapResult)0x0001)
#define BAP_RESULT_INVALID_ASE_ID           ((BapResult)0x0002)
#define BAP_RESULT_TRUNCATED_OPERATION      ((BapResult)0x0003)
#define BAP_RESULT_INVALID_STATE            ((BapResult)0x0004)
#define BAP_RESULT_UNSUPPORTED_CAPABILITY   ((BapResult)0x0005)
#define BAP_RESULT_NOT_SUPPORTED            ((BapResult)0x0006)
#define BAP_RESULT_REJECTED_PARAMETER       ((BapResult)0x0007)
#define BAP_RESULT_INVALID_PARAMETER        ((BapResult)0x0008)
#define BAP_RESULT_INSUFFICIENT_RESOURCES   ((BapResult)0x0009)
#define BAP_RESULT_ERROR                    ((BapResult)0x000A)
#define BAP_RESULT_ARG_ERROR                ((BapResult)0x000B)
#define BAP_RESULT_INPROGRESS               ((BapResult)0x000C)

/*! BAP Audio Codec ID
 */
typedef uint8 BapCodecId;
#define BAP_CODEC_ID_UNKNOWN                ((BapCodecId)0x00)
#define BAP_CODEC_ID_LC3                    ((BapCodecId)0x06)
#define BAP_CODEC_ID_VENDOR_DEFINED         ((BapCodecId)0xff)


/*  Vendor specific defines from QBCESPEC */
#define BAP_COMPANY_ID_QUALCOMM             (0x000A) /* Qualcomm Technologies Intl. Ltd.*/
#define BAP_VS_CODEC_ID_APTX                (0x0001) /* AptX */
#define BAP_VS_CODEC_ID_APTX_ADAPTIVE       (0x0001) /* AptX adaptive*/
#define BAP_VS_CODEC_ID_APTX_LITE           (0x0002) /* AptX lite */


/*! BAP Audio frame duration
 */
typedef uint8 BapFrameDuration;
#define BAP_SUPPORTED_FRAME_DURATION_NONE   ((BapFrameDuration)0x00)
#define BAP_SUPPORTED_FRAME_DURATION_7P5MS  ((BapFrameDuration)0x01)
#define BAP_SUPPORTED_FRAME_DURATION_10MS   ((BapFrameDuration)0x02)
#define BAP_PREFERRED_FRAME_DURATION_7P5MS  ((BapFrameDuration)0x10)
#define BAP_PREFERRED_FRAME_DURATION_10MS   ((BapFrameDuration)0x20)

/*! BAP Audio Channels count
 */
typedef uint8 BapAudioChannelCount;
#define BAP_AUDIO_CHANNEL_NONE              ((BapAudioChannelCount)0x00)
#define BAP_AUDIO_CHANNEL_1                 ((BapAudioChannelCount)0x01)
#define BAP_AUDIO_CHANNELS_2                ((BapAudioChannelCount)0x02)
#define BAP_AUDIO_CHANNELS_3                ((BapAudioChannelCount)0x04)
#define BAP_AUDIO_CHANNELS_4                ((BapAudioChannelCount)0x08)
#define BAP_AUDIO_CHANNELS_5                ((BapAudioChannelCount)0x10)
#define BAP_AUDIO_CHANNELS_6                ((BapAudioChannelCount)0x20)
#define BAP_AUDIO_CHANNELS_7                ((BapAudioChannelCount)0x40)
#define BAP_AUDIO_CHANNELS_8                ((BapAudioChannelCount)0x80)

/*! BAP Audio Context values
 */
typedef uint16 BapAudioContext;
#define BAP_CONTEXT_TYPE_UNKNOWN            ((BapAudioContext)0x0001)   /*! Unspecified */
#define BAP_CONTEXT_TYPE_CONVERSATIONAL     ((BapAudioContext)0x0002)   /*! Phone Call, Conversation between humans */
#define BAP_CONTEXT_TYPE_MEDIA              ((BapAudioContext)0x0004)   /*! Music, Radio, Podcast, Video Soundtrack */
#define BAP_CONTEXT_TYPE_GAME               ((BapAudioContext)0x0008)   /*! Audio associated with video gaming, for example gaming media, gaming effects, music and in-game voice chat between participants; or a mix of all the above*/
#define BAP_CONTEXT_TYPE_INSTRUCTIONAL      ((BapAudioContext)0x0010)   /*! Satnav, User Guidance, Traffic Announcement */
#define BAP_CONTEXT_TYPE_VOICE_ASSISTANT    ((BapAudioContext)0x0020)   /*! Virtual Assistant, Voice Recognition */
#define BAP_CONTEXT_TYPE_LIVE               ((BapAudioContext)0x0040)   /*! Live audio as from a microphone where audio is perceived both through a direct acoustic path and through an LE Audio Stream*/
#define BAP_CONTEXT_TYPE_SOUND_EFFECTS      ((BapAudioContext)0x0080)   /*! Sound effects including keyboard and touch feedback; menu and user interface sounds; and other system sounds */
#define BAP_CONTEXT_TYPE_NOTIFICATIONS      ((BapAudioContext)0x0100)   /*! Notification and reminder sounds; attention-seeking audio, for example, in beeps signaling the arrival of a message */
#define BAP_CONTEXT_TYPE_RINGTONE           ((BapAudioContext)0x0200)   /*! Ringtone as in a call alert  */
#define BAP_CONTEXT_TYPE_ALERTS             ((BapAudioContext)0x0400)   /*! Low Battery Warning, Alarm Clock, Timer Expired */
#define BAP_CONTEXT_TYPE_EMERGENCY_ALARM    ((BapAudioContext)0x0800)   /*! Emergency Alerts like fire alarms  */


/*! BAP PAC Audio Context : Available and Supported
 */
typedef uint8 BapPacAudioContext;
#define BAP_PAC_AVAILABLE_AUDIO_CONTEXT     ((BapPacAudioContext)0x01)
#define BAP_PAC_SUPPORTED_AUDIO_CONTEXT     ((BapPacAudioContext)0x02)
#define BAP_PAC_SUPPORTED_VENDOR_CONTEXT    ((BapPacAudioContext)0xFF)


/*! BAP PAC Record Type : Audio Sink and Audio Source
 */
typedef uint8 BapPacRecordType;
#define BAP_AUDIO_SINK_RECORD               ((BapPacRecordType)0x01)
#define BAP_AUDIO_SOURCE_RECORD             ((BapPacRecordType)0x02)

/*! BAP PACS Notification type
 */
typedef uint8 BapPacsNotificationType;
#define BAP_PACS_NOTIFICATION_AVAILABLE_AUDIO_CONTEXT   ((BapPacsNotificationType)0x01)
#define BAP_PACS_NOTIFICATION_SUPPORTED_AUDIO_CONTEXT   ((BapPacsNotificationType)0x02)
#define BAP_PACS_NOTIFICATION_SINK_PAC_RECORD           ((BapPacsNotificationType)0x04)
#define BAP_PACS_NOTIFICATION_SINK_AUDIO_LOCATION       ((BapPacsNotificationType)0x08)
#define BAP_PACS_NOTIFICATION_SOURCE_PAC_RECORD         ((BapPacsNotificationType)0x10)
#define BAP_PACS_NOTIFICATION_SOURCE_AUDIO_LOCATION     ((BapPacsNotificationType)0x20)
#define BAP_PACS_NOTIFICATION_ALL                       ((BapPacsNotificationType)0x3F)

/*! BAP ASE Type
 */
typedef uint8 BapAseType;
#define BAP_ASE_SINK            ((BapAseType)0x01)
#define BAP_ASE_SOURCE          ((BapAseType)0x02)

/*! BAP Server Direction
 */
typedef uint8 BapServerDirection;
#define BAP_SERVER_DIRECTION_SINK           ((BapServerDirection)0x01)
#define BAP_SERVER_DIRECTION_SOURCE         ((BapServerDirection)0x02)

/*! BAP Audio Direction
 */
typedef uint8 BapAudioDirection;
#define BAP_AUDIO_DIRECTION_SINK            ((BapAudioDirection)0x01)
#define BAP_AUDIO_DIRECTION_SOURCE          ((BapAudioDirection)0x02)

/*! BAP Audio Capability
 */
typedef uint8 BapAudioCapability;
#define BAP_PAC_SERVER_AUDIO_SINK_CAPABILITY    ((BapAudioCapability)0x01)
#define BAP_PAC_SERVER_AUDIO_SOURCE_CAPABILITY  ((BapAudioCapability)0x02)

/*! BAP Audio Location
 */
typedef uint32 BapAudioLocation;
#define BAP_AUDIO_LOCATION_MONO             ((BapAudioLocation)0x00000000)  /* NOT Allowed, A non standndard value for not sending Audio location in codec configuration parameters */

#define BAP_AUDIO_LOCATION_FL               ((BapAudioLocation)0x00000001)
#define BAP_AUDIO_LOCATION_FR               ((BapAudioLocation)0x00000002)
#define BAP_AUDIO_LOCATION_FC               ((BapAudioLocation)0x00000004)
#define BAP_AUDIO_LOCATION_LFE1             ((BapAudioLocation)0x00000008)
#define BAP_AUDIO_LOCATION_BL               ((BapAudioLocation)0x00000010)
#define BAP_AUDIO_LOCATION_BR               ((BapAudioLocation)0x00000020)
#define BAP_AUDIO_LOCATION_FLC              ((BapAudioLocation)0x00000040)
#define BAP_AUDIO_LOCATION_FRC              ((BapAudioLocation)0x00000080)
#define BAP_AUDIO_LOCATION_BC               ((BapAudioLocation)0x00000100)
#define BAP_AUDIO_LOCATION_LFE2             ((BapAudioLocation)0x00000200)
#define BAP_AUDIO_LOCATION_SIL              ((BapAudioLocation)0x00000400)
#define BAP_AUDIO_LOCATION_SIR              ((BapAudioLocation)0x00000800)
#define BAP_AUDIO_LOCATION_TPFL             ((BapAudioLocation)0x00001000)
#define BAP_AUDIO_LOCATION_TPFR             ((BapAudioLocation)0x00002000)
#define BAP_AUDIO_LOCATION_TPFC             ((BapAudioLocation)0x00004000)
#define BAP_AUDIO_LOCATION_TPC              ((BapAudioLocation)0x00008000)
#define BAP_AUDIO_LOCATION_TPBL             ((BapAudioLocation)0x00010000)
#define BAP_AUDIO_LOCATION_TPBR             ((BapAudioLocation)0x00020000)
#define BAP_AUDIO_LOCATION_TPSIL            ((BapAudioLocation)0x00040000)
#define BAP_AUDIO_LOCATION_TPSIR            ((BapAudioLocation)0x00080000)
#define BAP_AUDIO_LOCATION_TPBC             ((BapAudioLocation)0x00100000)
#define BAP_AUDIO_LOCATION_BTFC             ((BapAudioLocation)0x00200000)
#define BAP_AUDIO_LOCATION_BTFL             ((BapAudioLocation)0x00400000)
#define BAP_AUDIO_LOCATION_BTFR             ((BapAudioLocation)0x00800000)
#define BAP_AUDIO_LOCATION_FLW              ((BapAudioLocation)0x01000000)
#define BAP_AUDIO_LOCATION_FRW              ((BapAudioLocation)0x02000000)
#define BAP_AUDIO_LOCATION_LS               ((BapAudioLocation)0x04000000)
#define BAP_AUDIO_LOCATION_RS               ((BapAudioLocation)0x08000000)
#define BAP_AUDIO_LOCATION_RFU              ((BapAudioLocation)0x10000000)


/*! BAP Sampling Frequency
 */
typedef uint16 BapSamplingFrequency;
#define BAP_SAMPLING_FREQUENCY_RFU           ((BapSamplingFrequency)0x0000)
#define BAP_SAMPLING_FREQUENCY_8kHz          ((BapSamplingFrequency)0x0001)
#define BAP_SAMPLING_FREQUENCY_11_025kHz     ((BapSamplingFrequency)0x0002)
#define BAP_SAMPLING_FREQUENCY_16kHz         ((BapSamplingFrequency)0x0004)
#define BAP_SAMPLING_FREQUENCY_22_05kHz      ((BapSamplingFrequency)0x0008)
#define BAP_SAMPLING_FREQUENCY_24kHz         ((BapSamplingFrequency)0x0010)
#define BAP_SAMPLING_FREQUENCY_32kHz         ((BapSamplingFrequency)0x0020)
#define BAP_SAMPLING_FREQUENCY_44_1kHz       ((BapSamplingFrequency)0x0040)
#define BAP_SAMPLING_FREQUENCY_48kHz         ((BapSamplingFrequency)0x0080)
#define BAP_SAMPLING_FREQUENCY_88_2KHZ       ((BapSamplingFrequency)0x0100)
#define BAP_SAMPLING_FREQUENCY_96kHz         ((BapSamplingFrequency)0x0200)
#define BAP_SAMPLING_FREQUENCY_176_4kHz      ((BapSamplingFrequency)0x0400)
#define BAP_SAMPLING_FREQUENCY_192kHz        ((BapSamplingFrequency)0x0800)
#define BAP_SAMPLING_FREQUENCY_384kHz        ((BapSamplingFrequency)0x1000)


#define DATAPATH_DIRECTION_INPUT             (0x00)  /* Host to Controller */
#define DATAPATH_DIRECTION_OUTPUT            (0x01)  /* Controller to Host */

#define DATAPATH_ID_HCI                          (0x00)  /* HCI */
#define DATAPATH_ID_RAW_STREAM_ENDPOINTS_ONLY    (0x06)  /* for VM onchip solution */
#define DATAPATH_ID_DISABLE                      (0xFF)  /* Disable */

typedef uint8 BapAseState;
#define BAP_ASE_STATE_IDLE                  ((BapAseState)0x00)
#define BAP_ASE_STATE_CODEC_CONFIGURED      ((BapAseState)0x01)
#define BAP_ASE_STATE_QOS_CONFIGURED        ((BapAseState)0x02)
#define BAP_ASE_STATE_ENABLING              ((BapAseState)0x03)
#define BAP_ASE_STATE_STREAMING             ((BapAseState)0x04)
#define BAP_ASE_STATE_DISABLING             ((BapAseState)0x05)
#define BAP_ASE_STATE_RELEASING             ((BapAseState)0x06)
#define BAP_ASE_STATE_INVALID               ((BapAseState)0xFF)

/* Defined values for Target Latency */
#define BAP_TARGET_LOWER_LATENCY                      (0x01)
#define BAP_TARGET_BALANCE_LATENCY_AND_RELIABILITY    (0x02)
#define BAP_TARGET_HIGHER_RELIABILITY                 (0x03)

/* Defined values for Target PHY */
#define BAP_LE_1M_PHY                   (0x01)
#define BAP_LE_2M_PHY                   (0x02)
#define BAP_LE_CODED_PHY                (0x03)

/* Defined Values for LC3 Blocks per SDU */
#define BAP_DEFAULT_LC3_BLOCKS_PER_SDU                (0x01)

/*Defined values for metadata types in Assigned Number Document */
#define BAP_CLIENT_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS          0x01
#define BAP_CLIENT_METADATA_LTV_TYPE_STREAMING_AUDIO_CONTEXTS          0x02
#define BAP_CLIENT_METADATA_LTV_TYPE_PROGRAM_INFO                      0x03
#define BAP_CLIENT_METADATA_LTV_TYPE_LANGUAGE                          0x04
#define BAP_CLIENT_METADATA_LTV_TYPE_CCID_LIST                         0x05
#define BAP_CLIENT_METADATA_LTV_TYPE_PARENTAL_RATING                   0x06
#define BAP_CLIENT_METADATA_LTV_TYPE_PROGRAM_INFO_URI                  0x07
#define BAP_CLIENT_METADATA_LTV_TYPE_EXTENDED_METADATA                 0xFE 
#define BAP_CLIENT_METADATA_LTV_TYPE_VENDOR_SPECIFIC                   0xFF

/* Broadcast type*/
typedef uint32 BroadcastType;
#define NON_PUBLIC_BROADCAST            ((BroadcastType)0x00000001)
#define SQ_PUBLIC_BROADCAST             ((BroadcastType)0x00000002)
#define HQ_PUBLIC_BROADCAST             ((BroadcastType)0x00000004)
#define TMAP_BROADCAST                  ((BroadcastType)0x00000008)
#define GMAP_BROADCAST                  ((BroadcastType)0x00000010)

typedef uint8 BapClientProfile;

#define BAP_CLIENT_PROFILE_PACS                   ((BapClientProfile)0x01)
#define BAP_CLIENT_PROFILE_ASCS                   ((BapClientProfile)0x02)
#define BAP_CLIENT_PROFILE_BASS                   ((BapClientProfile)0x03)
#define BAP_CLIENT_PROFILE_VCP                    ((BapClientProfile)0x04)
#define BAP_CLIENT_PROFILE_CSIP                   ((BapClientProfile)0x05)


/* Element of the list of handles */
typedef GattBassClientDeviceData BapBassClientDeviceData;
typedef GattPacsClientDeviceData BapPacsClientDeviceData;
typedef GattAscsClientDeviceData BapAscsClientDeviceData;


typedef struct
{
    BapPacsClientDeviceData *pacsHandles;
    BapAscsClientDeviceData *ascsHandles;
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
    BapBassClientDeviceData *bassHandles;
#endif
}BapHandles;

typedef struct
{
    uint32    cid;     /*!< Connection Identifier */
    BapRole     role;    /*!< Supported BAP Role */
    BapHandles *handles;
} BapInitData;



/*! BAP Published Audio Capability (PAC) structure
 */
typedef struct
{
    BapCodecId  codecId;                            /*!< BAP Codec ID */
    uint16    companyId;                          /*!< Company_ID value of the BAP PAC record  */
    uint16    vendorCodecId;                      /*!< Vendor-specific codec_ID value of the PAC record. */

    uint16    samplingFrequencies;                 /*!< Bitfield representing the supported sampling frequencies */
    uint8     frameDuaration;                      /*!< Bitfield representing the supported frame duration */
    uint8     channelCount;                        /*!< Bitfield representing the value for channel count */
    uint16    minOctetsPerCodecFrame;
    uint16    maxOctetsPerCodecFrame;
    uint8     supportedMaxCodecFramesPerSdu;

    uint16    preferredAudioContext;
    uint16    streamingAudioContext;

    uint8     vendorSpecificConfigLen;
    uint8*    vendorSpecificConfig;

    uint8     vendorSpecificMetadataLen;
    uint8*    vendorSpecificMetadata;

} BapPacRecord;

typedef struct
{
    uint8            numPacRecords;  /*!< Number of BAP PAC records */
    BapPacRecord    *  pacRecords;     /*!< Pointer to BAP PAC records */
} BapPacRecords;

typedef struct
{
    uint16 sinkContext;
    uint16 sourceContext;
} BapPacAudioContextValue;

typedef struct
{
    uint8  codecId;          /*!< Codec Identifier */
    uint16 companyId;        /*!< Shall be set to 0 if codec_id is not 0xFF */
    uint16 vendorCodecId;    /*!< Shall be set to 0 if codec_id is not 0xFF */
} BapCodecIdInfo;

typedef struct
{
    BapCodecIdInfo   codecId;                            /*!< Codec Identifier */
    uint32         presentationDelayMinMicroseconds;   /*!< Minimum audio presentation delay for this codec */
    uint32         presentationDelayMaxMicroseconds;   /*!< Maximum audio presentation delay for this codec */
    uint8          numAudioLocations;                  /*!< Number of audio locations pointed to by 'audio_locations' */
    uint16*        audioLocations;                     /*!< Array of bitfields. Each element of the array represents
                                                          a set of audio locations that can be used simultaneously by this codec */
    uint16         channelMode;                        /*!< a single channel mode that is compatible with the supplied audio locations */
    uint8          samplingFrequencies;                /*!< Bitfield representing the supported sampling frequencies */
    uint8          numCodecSpecificParametersOctets;   /*!< Number of octets pointed to by 'codec_specific_parameters' */
    uint8*         codecSpecificParameters;            /*!< Additional parameters required by the codec identified by codec_id */
} BapCodecSubRecord;

typedef struct
{
    BapPacRecordType   recordType;    /*!< BAP PAC Sink or Source record type */
    BapCodecSubRecord  codecSubrecord;
} BapPacLocalRecord;

typedef struct
{
    uint16                    samplingFrequency;       /*<! Sampling Frequency */
    uint8                     frameDuaration;          /*<! codec frame duration */
    uint32                    audioChannelAllocation;  /*<! Audio Locations of the Audio Channels*/
    uint16                    octetsPerFrame;          /*<! Bit rate */
    uint8                     lc3BlocksPerSdu;         /*<! Number of blocks of LC3 codec frames per SDU */
} BapCodecConfiguration;

typedef struct
{
    uint8          aseId;       /*!< ASE ID */
    uint8          cisId;       /*!< CIS ID */
    BapResult        errorCode;   /*!< BAP error codes */
    BapAseState      aseState;    /*!< Current State of ASE */
} BapAseInfo;

typedef struct
{
    uint32 sduIntervalMtoS;                 /* qos: sdu_interval_microsecs M to S */
    uint32 sduIntervalStoM;                 /* qos: sdu_interval_microsecs S to M */
    uint16 sduSizeMtoS;                     /* qos: max_sdu_size*/
    uint16 sduSizeStoM;                     /* qos: max_sdu_size*/
    uint16 transportLatencyMtoS;            /* qos: transport_latency */
    uint16 transportLatencyStoM;            /* qos: transport_latency */
    uint8  packing;                         /* qos: packing */
    uint8  framing;                         /* qos: framing */
    uint8  rtnMtoS;                         /* qos: retransmission_effort */
    uint8  rtnStoM;                         /* qos: retransmission_effort */
    uint8  peerSleepClockAccuracy;          /* Peer Sleep Clock Accuracy */
    uint8  phyMtoS;                         /* 1PHY, 2PHY or coded PHY. Depends on the audio stream */
    uint8  phyStoM;                         /* 1PHY, 2PHY or coded PHY. Depends on the audio stream */
} BapIsocConfig;

typedef struct
{
    uint32 sduInterval;                 /* qos: sdu_interval_microsecs */
    uint8  framing;                     /* qos: framing */
    uint8  phy;                         /* 1PHY, 2PHY or coded PHY. Depends on the audio stream */
    uint16 sduSize;                     /* qos: max_sdu_size*/
    uint8  rtn;                         /* qos: retransmission_effort */
    uint16 transportLatency;            /* qos: transport_latency */
    uint32 presentationDelay;           /* qos: presentation_delay */
} BapAseQosConfig;

typedef struct
{
    uint8                    aseId;               /*!< ASE Identifier, Server assigned value */
    BapServerDirection         serverDirection;     /*!<  Server audio direction */
    uint8                    targetLatency;       /*!< Target Latency for the Codec config */
    uint8                    targetPhy;           /*!< Target PHY for the codec config */
    BapCodecIdInfo             codecId;             /*!<  Codec Identfier */
    BapCodecConfiguration      codecConfiguration;  /*!< Codec Configuration parameters */
} BapAseCodecConfiguration;

typedef struct
{
    uint8                    aseId;            /*!< ASE Identifier, Server assigned value */
    uint8                    cigId;            /*!< CIG ID value after CIG Confgure*/
    uint8                    cisId;            /*!< CIS ID , App can locally generate cis_id from the range <0x00-0xFE>*/
    uint16                   cisHandle;
    BapAseQosConfig            qosConfiguration; /*!< QOS Configuration parameters */
} BapAseQosConfiguration;

typedef struct
{
    uint8                    aseId;                   /*!< ASE Identifier */
    uint16                   streamingAudioContexts;  /*!< Bitmask of Audio Context type values */
    uint8                    metadataLen;             /* Total Metadata Len (Not to be Confused with 'L' of LTV) */
    uint8                    *metadata;               /* Can have CCID List LTV or vendor-specific metadata
                                                           NOTE: Metadata needs to be in LTV format*/
} BapAseEnableParameters;

typedef BapAseEnableParameters BapAseMetadataParameters;

typedef struct
{
    uint8                    aseId;   /*!< ASE ID */
} BapAseParameters;

typedef struct
{
    uint16                    isoHandle;          /*!< CIS or BIS handle */
    uint8                     dataPathDirection;  /*!< Direction of the audio datapath */
    uint8                     dataPathId;         /*!< Datapath id for HCI or Vendor specific Identifier*/
    uint32                    controllerDelay;    /*!< Controller Delay based on direction */
    BapCodecIdInfo              codecId;            /*!< Codec Identfier */
    BapCodecConfiguration       codecConfiguration; /*!< Codec Configuration parameters */
    uint8                     vendorDataLen;      /*!< vendorData length */
    uint8                     *vendorData;        /*!< vendor specific metadata which needs to be in LTV format */
}  BapSetupDataPath;


typedef struct
{
    uint8     cisId;                   /* Unique CIS identifier in given cis_id */
    uint16    maxSduMtoS;              /* Maximum SDU Size from master host */
    uint16    maxSduStoM;              /* Maximum SDU Size from slave host */
    uint8     phyMtoS;                 /* PHY from master */
    uint8     phyStoM;                 /* PHY from slave */
    uint8     rtnMtoS;                 /* Retransmission number from master to slave */
    uint8     rtnStoM;                 /* Retransmission number from slave to master */
} BapUnicastClientCisConfig;

typedef struct
{
    uint8                      cigId;                     /*!< Zero for new configuration, valid for re-configuration */
    uint32                     sduIntervalMtoS;           /*!< Time interval between the start of consecutive SDUs */
    uint32                     sduIntervalStoM;           /*!< Time interval between the start of consecutive SDUs */
    uint8                      packing;                   /*!< Interleaved, Sequential placement of packets */
    uint8                      framing;                   /*!< Indicates the format: framed or unframed */
    uint16                     maxTransportLatencyMtoS;   /*!< Maximum transport latency from master */
    uint16                     maxTransportLatencyStoM;   /*!< Maximum transport latency from slave */
    uint8                      sca;                       /*!< Sleep clock accuracy */
    uint8                      cisCount;                  /*!< Number of CIS config under CIG */
    BapUnicastClientCisConfig    *cisConfig;                /* Array of pointers to cis configuration */
} BapUnicastClientCigParameters;

typedef struct
{
    uint16    maxSduMtoS;             /* Maximum SDU Size from master host */
    uint16    maxSduStoM;             /* Maximum SDU Size from slave host */
    uint16    maxPduMtoS;             /* Maximum PDU Size from master host */
    uint16    maxPduStoM;             /* Maximum PDU Size from slave host */
    uint8     cisId;                  /* Unique CIS identifier in given cig_id */
    uint8     nse;                    /* Max no of sub events for each CIS */
    uint8     phyMtoS;                /* PHY from master */
    uint8     phyStoM;                /* PHY from slave */
    uint8     bnMtoS;                 /* Burst number from master to slave */
    uint8     bnStoM;                 /* Burst number from slave to master */
}BapUnicastClientCisTestConfig;

typedef struct
{
    uint32                           sduIntervalMtoS; /* Time interval between the start of consecutive SDUs */
    uint32                           sduIntervalStoM; /* Time interval between the start of consecutive SDUs */
    uint16                           isoInterval;     /* Time b/w two consecutive CIS anchor points */
    uint8                            cigId;           /* Zero for new configuration, valid for re-configuration */
    uint8                            ftMtoS;          /* Flush timeout at master side */
    uint8                            ftStoM;          /* Flush timeout at slave side */
    uint8                            sca;             /* Sleep clock accuracy */
    uint8                            packing;         /* Interleaved, Sequential placement of packets */
    uint8                            framing;         /* Indicates the format: framed or unframed */
    uint8                            cisCount;        /* Number of CIS under CIG */
    BapUnicastClientCisTestConfig      *cisTestConfig;  /* Array of pointers to cis test configuration */
}BapUnicastClientCigTestParameters;

#define BAP_MAX_NUMBER_BIS          0x02   /* TBD a proper value based on use case */
#define BAP_BROADCAST_CODE_SIZE     16

typedef struct
{
    uint16                    streamingAudioContext; /* Context type of Audio */
    uint16                    metadataLen;           /* Total length of vendor metadata */
    uint8                     *metadata;             /* LTV format */
} BapMetadata;

typedef struct
{
    uint8                      bisIndex;
    BapCodecConfiguration        codecConfigurations;  /*!< Codec Specific Configuration */
}BapBisInfo;

/* Level-2 (Subgroup Level) and Level-3 (BIS level) in Basic Audio Announcements */
typedef struct
{
    /* Num bises */
    uint8                      numBis;
    /* Codec Id */
    uint8                      codecId;                 /*!< Codec Identifier */
    uint16                     companyId;               /*!< Shall be set to 0 if codec_id is not 0xFF */
    uint16                     vendorCodecId;           /*!< Shall be set to 0 if codec_id is not 0xFF */
    /* Codec Specific Configurations */
    uint16                     samplingFrequency;       /*<! Sampling Frequency */
    uint8                      frameDuaration;          /*<! codec frame duration */
    uint32                     audioChannelAllocation;  /*<! Audio Locations of the Audio Channels*/
    uint16                     octetsPerFrame;          /*<! Bit rate */
    uint8                      lc3BlocksPerSdu;         /*<! Default value = 1*/
    /* Metadata */
    uint16                     streamingAudioContext;   /* Context type of Audio */
    uint16                     metadataLen;             /* Total length of vendor metadata */
    uint8                      *metadata;               /* LTV format */

    BapBisInfo                   bisInfo[BAP_MAX_NUMBER_BIS];
}BapBigSubgroups;

typedef struct
{
    uint32                     presentationDelay;    /*!<  Presentation delay */
    BapCodecIdInfo               codecId;              /*!<  Codec Identfier */
    BapCodecConfiguration        codecConfigurations;  /*!< Codec Specific Configuration */
    BapMetadata                  metadata;             /* !< Metadata */
}BapBigGroupInfo;

typedef struct
{
    BapCodecIdInfo               codecId;              /*!<  Codec Identfier */
    BapCodecConfiguration        codecConfigurations;  /*!< Codec Specific Configuration */
    BapMetadata                  metadata;
    uint8                      numBis;
    BapBisInfo                   bisInfo[BAP_MAX_NUMBER_BIS];
}BapBigSubgroupInfo;

typedef struct
{
    uint32    sduInterval;            /* Interval of Periodic SDUs*/
    uint16    maxSdu;                 /* Maximum size of an SDU */
    uint16    maxTransportLatency;    /* Max transport latency */
    uint8     rtn;                    /* Retransmission number */
    uint8     phy;                    /* Preferred PHY for transmission */
    uint8     packing;                /* Sequential or Interleaved */
    uint8     framing;                /* Framed or Unframed */
} BapBigConfigParam;

typedef struct
{
    uint24                 sduInterval;    /* Interval of Periodic SDUs*/
    uint16                 isoInterval;    /* ISO interval*/
    uint8                  nse;            /* NUmber of sub events */
    uint16                 maxSdu;         /* Maximum size of an SDU */
    uint16                 maxPdu;         /* Maximum size of an PDU */
    uint8                  phy;            /* Preferred PHY for transmission */
    uint8                  packing;        /* Sequential or Interleaved */
    uint8                  framing;        /* Framed or Unframed */
    uint8                  bn;             /* Burst number */
    uint8                  irc;            /* Repeated count of retransmission */
    uint8                  pto;            /* Pre transmission offset */
} BapBigTestConfigParam;

typedef struct
{
    uint24        transportLatencyBig;  /* Max time to tx SDUs of all BISes */
    uint16        maxPdu;               /* Maximum size of an PDU */
    uint16        isoInterval;          /* ISO interval */
    uint8         phy;                  /* PHY used */
    uint8         nse;                  /* Number of sub events */
    uint8         bn;                   /* Burst number */
    uint8         pto;                  /* Pre transmission offset */
    uint8         irc;                  /* Repeated count of retransmission */
} BapBigParam;

typedef struct BAP_BIG_SUBGROUP
{
    uint8                      numBis;
    BapCodecIdInfo               codecId;         /*!<  Codec Identfier */
    BapCodecConfiguration        codecConfigurations;  /*!< Codec Specific Configuration */
    BapMetadata                  metadata;
    /* Level 3 BIS  */
    struct BAP_BIS               *bisInfo;
}BapBigSubgroup;

typedef struct BAP_BIS
{
    uint8                      bisIndex; /* bisIndices */
    uint16                     bisHandle;
    BapCodecConfiguration        codecConfigurations;  /*!< Codec Specific Configuration */
}BapBis;

#define BAP_MAX_SUPPORTED_NUM_SUBGROUPS (4)

typedef struct
{
    uint32 bisSyncState;
    uint8  metadataLen;
    uint8  *metadataValue;
} BapSubgroupInfo;

/* flags field for Public Broadcast */
#define PUBLIC_BROADCAST_STREAM_ENCRYPTED  0x0001
#define PUBLIC_BROADCAST_METADATA_ALL      0x0002  /* Program_Info, Language, Parental_Rating */
typedef struct
{
    BroadcastType broadcast;
    uint16 flags; /* Refer flags defines above */
    uint16 appearanceValue;
} BapBroadcastInfo;

typedef enum bapPrimTag
{
    ENUM_BAP_INTERNAL_INIT_REQ = BAP_PRIM_DOWN,
    ENUM_BAP_INTERNAL_DEINIT_REQ,
    ENUM_BAP_INTERNAL_ADD_PAC_RECORD_REQ,
    ENUM_BAP_INTERNAL_REMOVE_PAC_RECORD_REQ,
    ENUM_BAP_INTERNAL_DISCOVER_AUDIO_ROLE_REQ,
    ENUM_BAP_INTERNAL_DISCOVER_REMOTE_AUDIO_CAPABILITY_REQ,
    ENUM_BAP_INTERNAL_REGISTER_PACS_NOTIFICATION_REQ,
    ENUM_BAP_INTERNAL_GET_REMOTE_AUDIO_LOCATION_REQ,
    ENUM_BAP_INTERNAL_SET_REMOTE_AUDIO_LOCATION_REQ,
    ENUM_BAP_INTERNAL_DISCOVER_AUDIO_CONTEXT_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_REGISTER_ASE_NOTIFICATION_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_READ_ASE_INFO_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_CODEC_CONFIGURE_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_CIG_CONFIGURE_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_CIG_TEST_CONFIGURE_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_CIG_REMOVE_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_QOS_CONFIGURE_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_ENABLE_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_CIS_CONNECT_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_CIS_DISCONNECT_REQ,
    ENUM_BAP_INTERNAL_SETUP_DATA_PATH_REQ,
    ENUM_BAP_INTERNAL_REMOVE_DATA_PATH_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_DISABLE_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_RELEASE_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_UPDATE_METEDATA_REQ,
    ENUM_BAP_INTERNAL_UNICAST_CLIENT_RECEIVER_READY_REQ,
    ENUM_BAP_INTERNAL_SET_CONTROL_POINT_OP_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_SRC_CONFIGURE_STREAM_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_TEST_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_SRC_DISABLE_STREAM_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_SRC_RELEASE_STREAM_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_SRC_UPDATE_METADATA_REQ,

    ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_START_SCAN_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_STOP_SCAN_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_CCC_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_ADD_SRC_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_MODIFY_SRC_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_REMOVE_SRC_REQ,
    ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_SET_CODE_RSP,
    ENUM_BAP_INTERNAL_BROADCAST_SRC_SET_BROADCAST_ID,

    ENUM_BAP_INIT_CFM = BAP_PRIM_UP,
    ENUM_BAP_DEINIT_CFM,
    ENUM_BAP_ADD_PAC_RECORD_CFM,
    ENUM_BAP_REMOVE_PAC_RECORD_CFM,
    ENUM_BAP_DISCOVER_AUDIO_ROLE_CFM,
    ENUM_BAP_DISCOVER_REMOTE_AUDIO_CAPABILITY_CFM,
    ENUM_BAP_REGISTER_PACS_NOTIFICATION_CFM,
    ENUM_BAP_GET_REMOTE_AUDIO_LOCATION_CFM,
    ENUM_BAP_SET_REMOTE_AUDIO_LOCATION_CFM,
    ENUM_BAP_PACS_NOTIFICATION_IND,
    ENUM_BAP_PACS_AUDIO_CAPABILITY_NOTIFICATION_IND,
    ENUM_BAP_DISCOVER_AUDIO_CONTEXT_CFM,
    ENUM_BAP_UNICAST_CLIENT_REGISTER_ASE_NOTIFICATION_CFM,
    ENUM_BAP_UNICAST_CLIENT_READ_ASE_INFO_CFM,
    ENUM_BAP_UNICAST_CLIENT_CODEC_CONFIGURE_IND,
    ENUM_BAP_UNICAST_CLIENT_CODEC_CONFIGURE_CFM,
    ENUM_BAP_UNICAST_CLIENT_CIG_CONFIGURE_CFM,
    ENUM_BAP_UNICAST_CLIENT_CIG_TEST_CONFIGURE_CFM,
    ENUM_BAP_UNICAST_CLIENT_CIG_REMOVE_CFM,
    ENUM_BAP_UNICAST_CLIENT_QOS_CONFIGURE_IND,
    ENUM_BAP_UNICAST_CLIENT_QOS_CONFIGURE_CFM,
    ENUM_BAP_UNICAST_CLIENT_ASE_CODEC_CONFIGURED_IND,
    ENUM_BAP_UNICAST_CLIENT_ENABLE_IND,
    ENUM_BAP_UNICAST_CLIENT_ENABLE_CFM,
    ENUM_BAP_UNICAST_CLIENT_CIS_CONNECT_IND,
    ENUM_BAP_UNICAST_CLIENT_CIS_CONNECT_CFM,
    ENUM_BAP_UNICAST_CLIENT_CIS_DISCONNECT_IND,
    ENUM_BAP_UNICAST_CLIENT_CIS_DISCONNECT_CFM,
    ENUM_BAP_CLIENT_SETUP_DATA_PATH_CFM,
    ENUM_BAP_CLIENT_REMOVE_DATA_PATH_CFM,
    ENUM_BAP_UNICAST_CLIENT_DISABLE_IND,
    ENUM_BAP_UNICAST_CLIENT_DISABLE_CFM,
    ENUM_BAP_UNICAST_CLIENT_DISABLED_IND,
    ENUM_BAP_UNICAST_CLIENT_RELEASE_IND,
    ENUM_BAP_UNICAST_CLIENT_ASE_RELEASE_IND,
    ENUM_BAP_UNICAST_CLIENT_RELEASE_CFM,
    ENUM_BAP_UNICAST_CLIENT_RELEASED_IND,
    ENUM_BAP_UNICAST_CLIENT_UPDATE_METADATA_IND,
    ENUM_BAP_UNICAST_CLIENT_UPDATE_METADATA_CFM,
    ENUM_BAP_UNICAST_CLIENT_RECEIVER_READY_IND,
    ENUM_BAP_UNICAST_CLIENT_RECEIVER_READY_CFM,
    ENUM_BAP_SET_CONTROL_POINT_OP_CFM,

    ENUM_BAP_BROADCAST_SRC_CONFIGURE_STREAM_CFM,
    ENUM_BAP_BROADCAST_SRC_ENABLE_STREAM_CFM,
    ENUM_BAP_BROADCAST_SRC_ENABLE_STREAM_TEST_CFM,
    ENUM_BAP_BROADCAST_SRC_DISABLE_STREAM_CFM,
    ENUM_BAP_BROADCAST_SRC_RELEASE_STREAM_CFM,
    ENUM_BROADCAST_SRC_UPDATE_METADATA_CFM,

    ENUM_BAP_BROADCAST_ASSISTANT_START_SCAN_CFM,
    ENUM_BAP_BROADCAST_ASSISTANT_STOP_SCAN_CFM,
    ENUM_BAP_BROADCAST_ASSISTANT_SRC_REPORT_IND,
    ENUM_BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_CFM,
    ENUM_BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_CFM,
    ENUM_BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_CFM,
    ENUM_BAP_BROADCAST_ASSISTANT_READ_BRS_CCC_CFM,
    ENUM_BAP_BROADCAST_ASSISTANT_READ_BRS_CFM,
    ENUM_BAP_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_CFM,
    ENUM_BAP_BROADCAST_ASSISTANT_BRS_IND,
    ENUM_BAP_BROADCAST_ASSISTANT_ADD_SOURCE_CFM,
    ENUM_BAP_BROADCAST_ASSISTANT_MODIFY_SOURCE_CFM,
    ENUM_BAP_BROADCAST_ASSISTANT_REMOVE_SOURCE_CFM,
    ENUM_BAP_BROADCAST_ASSISTANT_SET_CODE_IND,
    ENUM_BAP_BROADCAST_ASSISTANT_SYNC_LOSS_IND


} BapPrimTag;

/* Downstream primitives */
#define BAP_INTERNAL_INIT_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_INIT_REQ)
#define BAP_INTERNAL_DEINIT_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_DEINIT_REQ)
#define BAP_INTERNAL_ADD_PAC_RECORD_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_ADD_PAC_RECORD_REQ)
#define BAP_INTERNAL_REMOVE_PAC_RECORD_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_REMOVE_PAC_RECORD_REQ)
#define BAP_INTERNAL_DISCOVER_AUDIO_ROLE_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_DISCOVER_AUDIO_ROLE_REQ)
#define BAP_INTERNAL_DISCOVER_REMOTE_AUDIO_CAPABILITY_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_DISCOVER_REMOTE_AUDIO_CAPABILITY_REQ)
#define BAP_INTERNAL_REGISTER_PACS_NOTIFICATION_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_REGISTER_PACS_NOTIFICATION_REQ)
#define BAP_INTERNAL_GET_REMOTE_AUDIO_LOCATION_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_GET_REMOTE_AUDIO_LOCATION_REQ)
#define BAP_INTERNAL_SET_REMOTE_AUDIO_LOCATION_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_SET_REMOTE_AUDIO_LOCATION_REQ)
#define BAP_INTERNAL_DISCOVER_AUDIO_CONTEXT_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_DISCOVER_AUDIO_CONTEXT_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_REGISTER_ASE_NOTIFICATION_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_REGISTER_ASE_NOTIFICATION_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_READ_ASE_INFO_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_READ_ASE_INFO_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_CODEC_CONFIGURE_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_CODEC_CONFIGURE_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_CIG_CONFIGURE_REQ  ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_CIG_CONFIGURE_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_CIG_TEST_CONFIGURE_REQ  ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_CIG_TEST_CONFIGURE_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_CIG_REMOVE_REQ  ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_CIG_REMOVE_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_QOS_CONFIGURE_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_QOS_CONFIGURE_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_ENABLE_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_ENABLE_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_CIS_CONNECT_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_CIS_CONNECT_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_CIS_DISCONNECT_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_CIS_DISCONNECT_REQ)
#define BAP_INTERNAL_SETUP_DATA_PATH_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_SETUP_DATA_PATH_REQ)
#define BAP_INTERNAL_REMOVE_DATA_PATH_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_REMOVE_DATA_PATH_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_DISABLE_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_DISABLE_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_RELEASE_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_RELEASE_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_UPDATE_METADATA_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_UPDATE_METEDATA_REQ)
#define BAP_INTERNAL_UNICAST_CLIENT_RECEIVER_READY_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_UNICAST_CLIENT_RECEIVER_READY_REQ)
#define BAP_INTERNAL_SET_CONTROL_POINT_OP_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_SET_CONTROL_POINT_OP_REQ)


#define BAP_INTERNAL_BROADCAST_SRC_CONFIGURE_STREAM_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_SRC_CONFIGURE_STREAM_REQ)
#define BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_REQ)
#define BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_TEST_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_TEST_REQ)
#define BAP_INTERNAL_BROADCAST_SRC_DISABLE_STREAM_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_SRC_DISABLE_STREAM_REQ)
#define BAP_INTERNAL_BROADCAST_SRC_RELEASE_STREAM_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_SRC_RELEASE_STREAM_REQ)
#define BAP_INTERNAL_BROADCAST_SRC_UPDATE_METADATA_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_SRC_UPDATE_METADATA_REQ)

#define BAP_INTERNAL_BROADCAST_ASSISTANT_START_SCAN_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_START_SCAN_REQ)
#define BAP_INTERNAL_BROADCAST_ASSISTANT_STOP_SCAN_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_STOP_SCAN_REQ)
#define BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_REQ)
#define BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_REQ)
#define BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_REQ)
#define BAP_INTERNAL_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_REQ)
#define BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_CCC_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_CCC_REQ)
#define BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_REQ)
#define BAP_INTERNAL_BROADCAST_ASSISTANT_ADD_SRC_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_ADD_SRC_REQ)
#define BAP_INTERNAL_BROADCAST_ASSISTANT_MODIFY_SRC_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_MODIFY_SRC_REQ)
#define BAP_INTERNAL_BROADCAST_ASSISTANT_REMOVE_SRC_REQ ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_REMOVE_SRC_REQ)
#define BAP_INTERNAL_BROADCAST_ASSISTANT_SET_CODE_RSP ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_ASSISTANT_SET_CODE_RSP)
#define BAP_INTERNAL_BROADCAST_SRC_SET_BROADCAST_ID ((BapClientPrim)ENUM_BAP_INTERNAL_BROADCAST_SRC_SET_BROADCAST_ID)


/* Upstream primitives */
#define BAP_INIT_CFM ((BapClientPrim)ENUM_BAP_INIT_CFM)
#define BAP_DEINIT_CFM ((BapClientPrim)ENUM_BAP_DEINIT_CFM)
#define BAP_ADD_PAC_RECORD_CFM ((BapClientPrim)ENUM_BAP_ADD_PAC_RECORD_CFM)
#define BAP_REMOVE_PAC_RECORD_CFM ((BapClientPrim)ENUM_BAP_REMOVE_PAC_RECORD_CFM)
#define BAP_DISCOVER_AUDIO_ROLE_CFM ((BapClientPrim)ENUM_BAP_DISCOVER_AUDIO_ROLE_CFM)
#define BAP_DISCOVER_REMOTE_AUDIO_CAPABILITY_CFM ((BapClientPrim)ENUM_BAP_DISCOVER_REMOTE_AUDIO_CAPABILITY_CFM)
#define BAP_REGISTER_PACS_NOTIFICATION_CFM ((BapClientPrim)ENUM_BAP_REGISTER_PACS_NOTIFICATION_CFM)
#define BAP_GET_REMOTE_AUDIO_LOCATION_CFM ((BapClientPrim)ENUM_BAP_GET_REMOTE_AUDIO_LOCATION_CFM)
#define BAP_SET_REMOTE_AUDIO_LOCATION_CFM ((BapClientPrim)ENUM_BAP_SET_REMOTE_AUDIO_LOCATION_CFM)
#define BAP_DISCOVER_AUDIO_CONTEXT_CFM ((BapClientPrim)ENUM_BAP_DISCOVER_AUDIO_CONTEXT_CFM)
#define BAP_PACS_NOTIFICATION_IND         ((BapClientPrim)ENUM_BAP_PACS_NOTIFICATION_IND)
#define BAP_PACS_AUDIO_CAPABILITY_NOTIFICATION_IND ((BapClientPrim)ENUM_BAP_PACS_AUDIO_CAPABILITY_NOTIFICATION_IND)
#define BAP_UNICAST_CLIENT_REGISTER_ASE_NOTIFICATION_CFM ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_REGISTER_ASE_NOTIFICATION_CFM)
#define BAP_UNICAST_CLIENT_READ_ASE_INFO_CFM ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_READ_ASE_INFO_CFM)
#define BAP_UNICAST_CLIENT_CODEC_CONFIGURE_IND ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_CODEC_CONFIGURE_IND)
#define BAP_UNICAST_CLIENT_CODEC_CONFIGURE_CFM ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_CODEC_CONFIGURE_CFM)
#define BAP_UNICAST_CLIENT_CIG_CONFIGURE_CFM  ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_CIG_CONFIGURE_CFM)
#define BAP_UNICAST_CLIENT_CIG_TEST_CONFIGURE_CFM  ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_CIG_TEST_CONFIGURE_CFM)
#define BAP_UNICAST_CLIENT_REMOVE_CIG_CFM  ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_CIG_REMOVE_CFM)
#define BAP_UNICAST_CLIENT_QOS_CONFIGURE_IND ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_QOS_CONFIGURE_IND)
#define BAP_UNICAST_CLIENT_QOS_CONFIGURE_CFM ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_QOS_CONFIGURE_CFM)
#define BAP_UNICAST_CLIENT_ASE_CODEC_CONFIGURED_IND ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_ASE_CODEC_CONFIGURED_IND)
#define BAP_UNICAST_CLIENT_ENABLE_IND ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_ENABLE_IND)
#define BAP_UNICAST_CLIENT_ENABLE_CFM ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_ENABLE_CFM)
#define BAP_UNICAST_CLIENT_CIS_CONNECT_IND ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_CIS_CONNECT_IND)
#define BAP_UNICAST_CLIENT_CIS_CONNECT_CFM ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_CIS_CONNECT_CFM)
#define BAP_UNICAST_CLIENT_CIS_DISCONNECT_IND ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_CIS_DISCONNECT_IND)
#define BAP_UNICAST_CLIENT_CIS_DISCONNECT_CFM ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_CIS_DISCONNECT_CFM)
#define BAP_CLIENT_SETUP_DATA_PATH_CFM ((BapClientPrim)ENUM_BAP_CLIENT_SETUP_DATA_PATH_CFM)
#define BAP_CLIENT_REMOVE_DATA_PATH_CFM ((BapClientPrim)ENUM_BAP_CLIENT_REMOVE_DATA_PATH_CFM)
#define BAP_UNICAST_CLIENT_DISABLE_IND ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_DISABLE_IND)
#define BAP_UNICAST_CLIENT_DISABLE_CFM ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_DISABLE_CFM)
#define BAP_UNICAST_CLIENT_RELEASE_IND ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_RELEASE_IND)
#define BAP_UNICAST_CLIENT_RELEASE_CFM ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_RELEASE_CFM)
#define BAP_UNICAST_CLIENT_RELEASED_IND ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_RELEASED_IND)
#define BAP_UNICAST_CLIENT_UPDATE_METADATA_IND ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_UPDATE_METADATA_IND)
#define BAP_UNICAST_CLIENT_UPDATE_METADATA_CFM ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_UPDATE_METADATA_CFM)
#define BAP_UNICAST_CLIENT_RECEIVER_READY_IND ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_RECEIVER_READY_IND)
#define BAP_UNICAST_CLIENT_RECEIVER_READY_CFM ((BapClientPrim)ENUM_BAP_UNICAST_CLIENT_RECEIVER_READY_CFM)
#define BAP_SET_CONTROL_POINT_OP_CFM ((BapClientPrim)ENUM_BAP_SET_CONTROL_POINT_OP_CFM)


#define BAP_BROADCAST_SRC_CONFIGURE_STREAM_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_SRC_CONFIGURE_STREAM_CFM)
#define BAP_BROADCAST_SRC_ENABLE_STREAM_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_SRC_ENABLE_STREAM_CFM)
#define BAP_BROADCAST_SRC_ENABLE_STREAM_TEST_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_SRC_ENABLE_STREAM_TEST_CFM)
#define BAP_BROADCAST_SRC_DISABLE_STREAM_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_SRC_DISABLE_STREAM_CFM)
#define BAP_BROADCAST_SRC_RELEASE_STREAM_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_SRC_RELEASE_STREAM_CFM)
#define BAP_BROADCAST_SRC_UPDATE_METADATA_CFM ((BapClientPrim)ENUM_BROADCAST_SRC_UPDATE_METADATA_CFM)

#define BAP_BROADCAST_ASSISTANT_START_SCAN_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_START_SCAN_CFM)
#define BAP_BROADCAST_ASSISTANT_STOP_SCAN_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_STOP_SCAN_CFM)
#define BAP_BROADCAST_ASSISTANT_SRC_REPORT_IND ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_SRC_REPORT_IND)
#define BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_CFM)
#define BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_CFM)
#define BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_CFM)
#define BAP_BROADCAST_ASSISTANT_READ_BRS_CCC_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_READ_BRS_CCC_CFM)
#define BAP_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_CFM)
#define BAP_BROADCAST_ASSISTANT_READ_BRS_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_READ_BRS_CFM)
#define BAP_BROADCAST_ASSISTANT_BRS_IND ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_BRS_IND)
#define BAP_BROADCAST_ASSISTANT_ADD_SOURCE_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_ADD_SOURCE_CFM)
#define BAP_BROADCAST_ASSISTANT_MODIFY_SOURCE_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_MODIFY_SOURCE_CFM)
#define BAP_BROADCAST_ASSISTANT_REMOVE_SOURCE_CFM ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_REMOVE_SOURCE_CFM)
#define BAP_BROADCAST_ASSISTANT_SET_CODE_IND ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_SET_CODE_IND)
#define BAP_BROADCAST_ASSISTANT_SYNC_LOSS_IND ((BapClientPrim)ENUM_BAP_BROADCAST_ASSISTANT_SYNC_LOSS_IND)

/*
 * TODO: PAC records can no longer have multiple codec subrecords: include the (single)
 * codec subrecord in the BAP_REGISTER_PAC_RECORD_REQ.
 *
 * The BAP_REGISTER_PAC_RECORD_CFM includes a pac_record_id that is used associate
 * subsequent BAP_REGISTER_CODEC_SUBRECORD_REQ messages with this pac record.
 * The BAP_REGISTER_CODEC_SUBRECORD_REQ messages include the actual codec subrecord
 * data.
 */

/*! @brief BapInternalInitReq struct.
 */
 typedef struct
 {
     BapClientPrim               type;           /*!< BAP_INTERNAL_INIT_REQ */
     phandle_t                   phandle;        /*!< phandle for the response */
     BapInitData                 initData;       /*!< Initialization data */
 } BapInternalInitReq;

/*! @brief BapInternalDeinitReq struct.
 */
 typedef struct
 {
     BapClientPrim               type;           /*!< BAP_INTERNAL_DEINIT_REQ */
     BapProfileHandle            handle;         /*!< BAP Handle */
     BapRole                     role;           /*!< BAP Role */
 } BapInternalDeinitReq;

/*! @brief BapInternalAddPacRecordReq struct.
 */
 typedef struct
 {
     BapClientPrim               type;           /*!< BAP_INTERNAL_ADD_PAC_RECORD_REQ */
     phandle_t                   phandle;        /*!< phandle for the response */
     BapPacLocalRecord          *pacRecord;      /*!< BAP PAC record */
 } BapInternalAddPacRecordReq;

/*! @brief BapInternalRemovePacRecordReq struct.
 */
 typedef struct
 {
     BapClientPrim               type;           /*!< BAP_INTERNAL_REMOVE_PAC_RECORD_REQ */
     phandle_t                   phandle;        /*!< phandle for the response */
     uint16                    pacRecordId;    /*!< Audio PAC record ID */
 } BapInternalRemovePacRecordReq;

/*! @brief BapInternalDiscoverAudioRoleReq struct.
 */
 typedef struct
 {
     BapClientPrim               type;           /*!< BAP_INTERNAL_DISCOVER_AUDIO_ROLE_REQ */
     BapProfileHandle            handle;         /*!< BAP Handle */
     BapPacRecordType            recordType;     /*!< BAP PAC Sink or Source record type */
 } BapInternalDiscoverAudioRoleReq;

/*! @brief BapInternalDiscoverRemoteAudioCapabilityReq struct.
 */
 typedef struct
 {
     BapClientPrim               type;           /*!< BAP_INTERNAL_DISCOVER_REMOTE_AUDIO_CAPABILITY_REQ */
     BapProfileHandle            handle;         /*!< BAP Handle */
     BapPacRecordType            recordType;     /*!< BAP PAC Sink or Source record type */
 } BapInternalDiscoverRemoteAudioCapabilityReq;

 typedef struct
 {
     BapClientPrim                 type;          /*!< BAP_INTERNAL_REGISTER_PACS_NOTIFICATION_REQ */
     BapProfileHandle              handle;        /*!< BAP Handle */
     BapPacsNotificationType       notifyType;    /*!< BAP PACS Notification Type */
     bool                          notifyEnable;  /*!< Notification enable (TRUE) or disable (FALSE) */
 } BapInternalRegisterPacsNotificationReq;

/*! @brief BapInternalGetRemoteAudioLocationReq struct.
 */
 typedef struct
 {
     BapClientPrim               type;           /*!< BAP_INTERNAL_GET_REMOTE_AUDIO_LOCATION_REQ */
     BapProfileHandle            handle;         /*!< BAP Handle */
     BapPacRecordType            recordType;     /*!< BAP PAC Sink or Source record type */
 } BapInternalGetRemoteAudioLocationReq;

/*! @brief BapInternalSetRemoteAudioLocationReq struct.
 */
 typedef struct
 {
     BapClientPrim               type;           /*!< BAP_INTERNAL_SET_REMOTE_AUDIO_LOCATION_REQ */
     BapProfileHandle            handle;         /*!< BAP Handle */
     BapPacRecordType            recordType;     /*!< BAP PAC Sink or Source record type */
     BapAudioLocation            location;       /*!< Audio location */
 } BapInternalSetRemoteAudioLocationReq;

/*! @brief BapInternalDiscoverAudioContextReq struct.
 */
 typedef struct
 {
    BapClientPrim               type;           /*!< BAP_INTERNAL_DISCOVER_AUDIO_CONTEXT_REQ */
    BapProfileHandle            handle;         /*!< BAP Handle */
    BapPacAudioContext          context;        /*!< BAP PAC Audio Context  */
 } BapInternalDiscoverAudioContextReq;

/*! @brief BapInternalUnicastClientRegisterAseNotificationReq struct.
 */
typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_UNICAST_CLIENT_REGISTER_ASE_NOTIFICATION_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      aseId;         /*!< ASE Identifier */
    bool                         notifyEnable;  /*!< Notification enable (TRUE) or disable (FALSE) */
} BapInternalUnicastClientRegisterAseNotificationReq;

/*! @brief BapInternalUnicastClientReadAseInfoReq struct.
 */
typedef struct
{
    BapClientPrim                 type;          /*!< BAP_INTERNAL_UNICAST_CLIENT_READ_ASE_INFO_REQ */
    BapProfileHandle              handle;        /*!< BAP Handle */
    uint8                       aseId;         /*!< ASE Identifier */
    BapAseType                    aseType;       /*!< ASE type */
} BapInternalUnicastClientReadAseInfoReq;

/*! @brief BapInternalUnicastClientCodecConfigureReq struct.
 */
typedef struct
{
    BapClientPrim                type;                      /*!< BAP_INTERNAL_UNICAST_CLIENT_CODEC_CONFIGURE_REQ */
    BapProfileHandle             handle;                    /*!< BAP Handle */
    uint8                      numAseCodecConfigurations; /* < number of ase codec configureation */
    BapAseCodecConfiguration*    aseCodecConfigurations[BAP_MAX_SUPPORTED_ASES];
} BapInternalUnicastClientCodecConfigureReq;

/*! @brief BapInternalUnicastClientCigConfigureReq struct.
 */
typedef struct
{
    BapClientPrim                 type;                /*!< BAP_INTERNAL_UNICAST_CLIENT_CIG_CONFIGURE_REQ */
    phandle_t                     handle;              /*!< APP Handle */
    BapUnicastClientCigParameters cigParameters;
} BapInternalUnicastClientCigConfigureReq;

/*! @brief BapInternalUnicastClientCigTestConfigureReq struct.
 */
typedef struct
{
    BapClientPrim          type;                /*!< BAP_INTERNAL_UNICAST_CLIENT_CIG_TEST_CONFIGURE_REQ */
    phandle_t              handle;              /*!< APP Handle */
    BapUnicastClientCigTestParameters cigTestParameters;
} BapInternalUnicastClientCigTestConfigureReq;

/*! @brief BapInternalUnicastClientCigRemoveReq struct.
 */
typedef struct
{
    BapClientPrim          type;                /*!< BAP_INTERNAL_UNICAST_CLIENT_CIG_REMOVE_REQ */
    phandle_t              handle;              /*!< App Handle */
    uint8                cigId;              /* CIG Identifier to be removed */
} BapInternalUnicastClientCigRemoveReq;

/*! @brief BapInternalUnicastClientQosConfigureReq struct.
 */
typedef struct
{
    BapClientPrim                type;                    /*!< BAP_INTERNAL_UNICAST_CLIENT_QOS_CONFIGURE_REQ */
    BapProfileHandle             handle;                  /*!< BAP Handle */
    uint8                      numAseQosConfigurations; /* !< number of ase qos configureation */
    BapAseQosConfiguration*      aseQosConfigurations[BAP_MAX_SUPPORTED_ASES];
} BapInternalUnicastClientQosConfigureReq;

/*! @brief BapInternalUnicastClientEnableReq struct.
 */
typedef struct
{
    BapClientPrim                type;                   /*!< BAP_INTERNAL_UNICAST_CLIENT_ENABLE_REQ */
    BapProfileHandle             handle;                 /*!< BAP Handle */
    uint8                      numAseEnableParameters; /* !< number of ase enable parameters */
    BapAseEnableParameters*      aseEnableParameters[BAP_MAX_SUPPORTED_ASES];
} BapInternalUnicastClientEnableReq;

typedef struct
{
    uint8                 cisId;        /*!< cid id */
    uint16                cisHandle;    /* cis handles for isoc connection */
    TP_BD_ADDR_T            tpAddrt;      /* Transport bluetooth device address */
} BapUnicastClientCisConnection;

/*! @brief BapInternalUnicastClientCisConnectReq struct.
 */
typedef struct
{
    BapClientPrim                   type;                                  /*!< BAP_INTERNAL_UNICAST_CLIENT_CIS_CONNECT_REQ */
    BapProfileHandle                handle;                                /*!< BAP Handle */
    uint8                         cisCount;                              /* number of CIS connections */
    BapUnicastClientCisConnection   *cisConnParameters[MAX_SUPPORTED_CIS]; /* list of cis handle and addresses to connect */
} BapInternalUnicastClientCisConnectReq;

/*! @brief BapInternalUnicastClientCisDisconnectReq struct.
 */
typedef struct
{
    BapClientPrim               type;           /*!< BAP_INTERNAL_UNICAST_CLIENT_CIS_DISCONNECT_REQ */
    BapProfileHandle            handle;         /*!< BAP Handle */
    uint16                    cisHandle;      /*!< CIS handle for disconnection */
    uint8                     reason;         /*!< Reason for command */
} BapInternalUnicastClientCisDisconnectReq;

/*! @brief BapInternalSetupDataPathReq struct.
 */
typedef struct
{
    BapClientPrim               type;          /*!< BAP_INTERNAL_SETUP_DATA_PATH_REQ */
    BapProfileHandle            handle;        /*!< BAP Handle */
    BapSetupDataPath            dataPathParameter;
}  BapInternalSetupDataPathReq;

/*! @brief BapInternalRemoveDataPathReq struct.
 */
typedef struct
{
    BapClientPrim               type;              /*!< BAP_INTERNAL_REMOVE_DATA_PATH_REQ */
    BapProfileHandle            handle;            /*!< BAP Handle */
    uint16                    isoHandle;         /* CIS or BIS handle */
    uint8                     dataPathDirection; /* Direction of the path to be removed */
} BapInternalRemoveDataPathReq;

/*! @brief BapInternalUnicastClientDisableReq struct.
 */
typedef struct
{
    BapClientPrim                type;                      /*!< BAP_INTERNAL_UNICAST_CLIENT_DISABLE_REQ */
    BapProfileHandle             handle;                    /*!< BAP Handle */
    uint8                      numAseDisableParameters;   /* !< number of ase disable parameters */
    BapAseParameters*            aseDisableParameters[BAP_MAX_SUPPORTED_ASES];
} BapInternalUnicastClientDisableReq;

/*! @brief BapInternalUnicastClientReleaseReq struct.
 */
typedef struct
{
    BapClientPrim                type;                     /*!< BAP_INTERNAL_UNICAST_CLIENT_RELEASE_REQ */
    BapProfileHandle             handle;                   /*!< BAP Handle */
    uint8                      numAseReleaseParameters;  /* !< number of ase release parameters */
    BapAseParameters*            aseReleaseParameters[BAP_MAX_SUPPORTED_ASES];
} BapInternalUnicastClientReleaseReq;

/*! @brief BapInternalUnicastClientUpdateMetadataReq struct.
 */
typedef struct
{
    BapClientPrim                type;                      /*!< BAP_INTERNAL_UNICAST_CLIENT_UPDATE_METADATA_REQ */
    BapProfileHandle             handle;                    /*!< BAP Handle */
    uint8                      numAseMetadataParameters;  /* !< number of ase metadata parameters */
    BapAseMetadataParameters*    aseMetadataParameters[BAP_MAX_SUPPORTED_ASES];
} BapInternalUnicastClientUpdateMetadataReq;

/*! @brief BapInternalUnicastClientReceiverReadyReq struct.
 */
typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_UNICAST_CLIENT_RECEIVER_READY_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      readyType;     /*!< Ready type Start/Stop */
    uint8                      numAses;
    uint8*                     aseIds;
} BapInternalUnicastClientReceiverReadyReq;

/*! @brief BapInitCfm struct.
 */
typedef struct
{
    BapClientPrim               type;           /*!< BAP_INIT_CFM */
    BapResult                   result;         /*!< Result code. */
    BapProfileHandle            handle;         /*!< BAP Handle */
    BapRole                     role;           /*!< BAP Role */
} BapInitCfm;

/*! @brief BapDeinitCfm struct.
 */
typedef struct
{
    BapClientPrim               type;           /*!< BAP_DEINIT_CFM */
    BapResult                   result;         /*!< Result code. */
    BapProfileHandle            handle;         /*!< BAP Handle */
    BapRole                     role;           /*!< BAP Role */
    BapHandles                  *handles;
} BapDeinitCfm;

/*! @brief BapAddPacRecordCfm struct.
 */
typedef struct
{
    BapClientPrim               type;           /*!< BAP_ADD_PAC_RECORD_CFM */
    BapResult                   result;         /*!< Result code. */
    BapProfileHandle            handle;         /*!< BAP Handle */
    uint16                    pacRecordId;    /*!< Audio PAC record ID */
    BapPacRecordType            recordType;     /*!< BAP PAC Sink or Source record type */
} BapAddPacRecordCfm;

typedef struct
{
    BapClientPrim               type;           /*!< BAP_ADD_PAC_RECORD_CFM */
    BapResult                   result;         /*!< Result code. */
    uint16                    pacRecordId;   /*!< Audio PAC record ID */
} BapRemovePacRecordCfm;

/*! @brief BapDiscoverAudioRoleCfm struct.
 */
typedef struct
{
    BapClientPrim               type;           /*!< BAP_DISCOVER_AUDIO_ROLE_CFM */
    BapResult                   result;         /*!< Result code. */
    BapProfileHandle            handle;         /*!< BAP Handle */
    BapPacRecordType            recordType;    /*!< BAP PAC Sink or Source record type */
} BapDiscoverAudioRoleCfm;

/*! @brief BapDiscoverRemoteAudioCapabilityCfm struct.
 */
typedef struct
{
    BapClientPrim               type;                                 /*!< BAP_DISCOVER_REMOTE_AUDIO_CAPABILITY_CFM */
    BapResult                   result;                               /*!< Result code. */
    BapProfileHandle            handle;                               /*!< BAP Handle */
    bool                        moreToCome;
    BapPacRecordType            recordType;                           /*!< BAP PAC Sink or Source record type */
    uint8                     numPacRecords;                        /*!< Number of BAP PAC records */
    BapPacRecord*               pacRecords[MAX_PAC_RECORD_ENTRIES];   /*!< Pointer to BAP PAC records */
} BapDiscoverRemoteAudioCapabilityCfm;

/*! @brief BapRegisterPacsNotificationCfm struct.
 */
typedef struct
{
    BapClientPrim               type;           /*!< BAP_REGISTER_PACS_NOTIFICATION_CFM */
    BapResult                   result;         /*!< Result code. */
    BapProfileHandle            handle;         /*!< BAP Handle */
} BapRegisterPacsNotificationCfm;

/*! @brief BapPacsNotificationInd struct.
 */
typedef struct
{
    BapClientPrim               type;           /*!< BAP_PACS_NOTIFICATION_IND */
    BapResult                   result;         /*!< Result code. */
    BapProfileHandle            handle;         /*!< BAP Handle */
    BapPacsNotificationType     notifyType;     /*!< BAP PACS Notification Type */
    uint8                     valueLength;    /*!< Notification Value Length */
    uint8*                    value;          /*!< Notification Value */
} BapPacsNotificationInd;

/*! @brief BapPacsAudioCapabilityNotificationInd struct.
 */
typedef struct
{
    BapClientPrim               type;                                    /*!< BAP_PACS_AUDIO_CAPABILITY_NOTIFICATION_IND */
    BapResult                   result;                                  /*!< Result code. */
    BapProfileHandle            handle;                                  /*!< BAP Handle */
    BapPacRecordType            recordType;                              /*!< BAP PACS Notification Type */
    uint8                     numPacRecords;                           /*!< Number of PACS record */
    BapPacRecord*               pacRecords[MAX_PAC_RECORD_ENTRIES];      /*!< Pointer to BAP Record  */
} BapPacsAudioCapabilityNotificationInd;

/*! @brief BapGetRemoteAudioLocationCfm struct.
 */
typedef struct
{
    BapClientPrim               type;           /*!< BAP_GET_REMOTE_AUDIO_LOCATION_CFM */
    BapResult                   result;         /*!< Result code. */
    BapProfileHandle            handle;         /*!< BAP Handle */
    BapPacRecordType            recordType;     /*!< PAC Sink or Source record */
    BapAudioLocation            location;       /*!< Audio location */
} BapGetRemoteAudioLocationCfm;

/*! @brief BapSetRemoteAudioLocationCfm struct.
 */
typedef struct
{
    BapClientPrim               type;           /*!< BAP_SET_REMOTE_AUDIO_LOCATION_CFM */
    BapResult                   result;         /*!< Result code. */
    BapProfileHandle            handle;         /*!< BAP Handle */
} BapSetRemoteAudioLocationCfm;

/*! @brief BapDiscoverAudioContextCfm struct.
 */
typedef struct
{
    BapClientPrim               type;           /*!< BAP_DISCOVER_AUDIO_CONTEXT_CFM */
    BapResult                   result;         /*!< Result code. */
    BapProfileHandle            handle;         /*!< BAP Handle */
    BapPacAudioContextValue     contextValue;   /*!< Audio context value */
    BapPacAudioContext          context;        /*!< BAP Audio context */
} BapDiscoverAudioContextCfm;

/*! @brief BapUnicastClientRegisterAseNotificationCfm struct.
 */
typedef struct
{
    BapClientPrim                type;         /*!< BAP_UNICAST_CLIENT_REGISTER_ASE_NOTIFICATION_CFM */
    BapResult                    result;       /*!< Result code. */
    uint8                      ase_id;       /*!< ASE Identifier */
    BapProfileHandle             handle;       /*!< BAP Handle */
} BapUnicastClientRegisterAseNotificationCfm;

/*! @brief BapUnicastClientReadAseInfoCfm struct.
 */
typedef struct
{
    BapClientPrim                type;         /*!< Always BAP_UNICAST_CLIENT_READ_ASE_INFO_CFM */
    BapResult                    result;       /*!< Result code. */
    phandle_t                    phandle;      /*!< Destination phandle. */
    BapProfileHandle             handle;       /*!< BAP Handle */
    uint8                      numAses;      /*!< Size of ase_ids pointer */
    uint8*                     aseIds;       /*!< ASE Identifier */
    uint8                      aseState;     /*!< ASE State */
    BapAseType                   aseType;
    uint8                      aseInfoLen;   /*!< Size of ase Parameters based on State */
    uint8                      *aseInfo;     /*!< Ase additional Parameters based on State*/
} BapUnicastClientReadAseInfoCfm;

/*! @brief BapUnicastClientCodecConfigureInd struct.
 */
typedef struct
{
    BapClientPrim                type;                 /*!< BAP_UNICAST_CLIENT_CODEC_CONFIGURE_IND */
    BapResult                    result;               /*!< Result code. */
    BapProfileHandle             handle;               /*!< BAP Handle */
    uint8                      aseId;                /*!< ASE Identifier */
    BapAseState                  aseState;             /*!< ASE State */
    uint8                      framing;              /* qos: framing */
    uint8                      phy;                  /* 1PHY, 2PHY or coded PHY. Depends on the audio stream */
    uint8                      rtn;                  /* qos: retransmission_effort */
    uint16                     transportLatency;     /* qos: transport_latency */
    uint32                     presentationDelayMin; /*!< Presentation delay min */
    uint32                     presentationDelayMax; /*!< Presentation delay min */
    BapCodecConfiguration        codecConfiguration;
    bool                         clientInitiated;      /*!< Client initiated operation */
} BapUnicastClientCodecConfigureInd;

/*! @brief BapUnicastClientCodecConfigureCfm struct.
 */
typedef struct
{
    BapClientPrim                type;         /*!< BAP_UNICAST_CLIENT_CODEC_CONFIGURE_CFM */
    BapResult                    result;       /*!< Result code. */
    BapProfileHandle             handle;       /*!< BAP Handle */
} BapUnicastClientCodecConfigureCfm;

/*! @brief BapUnicastClientCigConfigureCfm struct.
 */
typedef struct
{
    BapClientPrim              type;            /*!< BAP_UNICAST_CLIENT_CIG_CONFIGURE_CFM */
    BapResult                  result;          /*!< Result code. */
    BapProfileHandle           handle;          /*!< BAP Handle */
    uint8                    cigId;           /*!< CIG identifier */
    uint8                    cisCount;        /*!< number of cis configured */
    uint16                   cisHandles[MAX_SUPPORTED_CIS];
                                                /*!< cis handles for cigId, contains cisCount valid elements */
} BapUnicastClientCigConfigureCfm;

/*! @brief BapUnicastClientCigTestConfigureCfm struct.
 */
typedef struct
{
    BapClientPrim              type;            /*!< BAP_UNICAST_CLIENT_CIG_TEST_CONFIGURE_CFM */
    BapResult                  result;          /*!< Result code. */
    uint8                    cigId;           /*!< CIG identifier */
    uint8                    cisCount;        /*!< number of cis configured */
    uint16                   cisTestHandles[MAX_SUPPORTED_CIS];
                                                /*!< cis handles for cigId,
                                                 *!< contains cisCount valid elements */
} BapUnicastClientCigTestConfigureCfm;

/*! @brief BapUnicastClientRemoveCigCfm struct.
 */
typedef struct
{
    BapClientPrim              type;          /*!< BAP_UNICAST_CLIENT_REMOVE_CIG_CFM */
    BapResult                  result;        /*!< Result code. */
    BapProfileHandle           handle;        /*!< BAP Handle */
    uint8                    cigId;         /* removed CIG Identifier */
} BapUnicastClientRemoveCigCfm;

/*! @brief BapUnicastClientQosConfigureInd struct.
 */
typedef struct
{
    BapClientPrim                type;                          /*!< BAP_UNICAST_CLIENT_QOS_CONFIGURE_IND */
    BapResult                    result;                        /*!< Result code. */
    BapProfileHandle             handle;                        /*!< BAP Handle */
    uint8                      aseId;                         /*!< ASE Identifier */
    uint8                      cisId;                         /*!< CIS ID */
    BapAseState                  aseState;
    uint32                     presentationDelayMicroseconds; /*!< Presentation delay */
} BapUnicastClientQosConfigureInd;

/*! @brief BapUnicastClientQosConfigureCfm struct.
 */
typedef struct
{
    BapClientPrim                type;         /*!< BAP_UNICAST_CLIENT_QOS_CONFIGURE_CFM */
    BapResult                    result;       /*!< Result code. */
    BapProfileHandle             handle;       /*!< BAP Handle */
} BapUnicastClientQosConfigureCfm;

/*! @brief BapUnicastClientEnableInd struct.
 */
typedef struct
{
    BapClientPrim                type;               /*!< BAP_UNICAST_CLIENT_ENABLE_IND */
    BapResult                    result;             /*!< Result code. */
    BapProfileHandle             handle;             /*!< BAP Handle */
    uint8                        aseId;              /*!< ASE Identifier */
    uint8                        cisId;              /*!< CIS ID */
    BapAseState                  aseState;
    uint8                        metadataLength;     /*!< metadata length */
    uint8*                       metadata;           /*!< metadata in LTV format and can hve vendor specific data as well */
} BapUnicastClientEnableInd;

/*! @brief BapUnicastClientEnableCfm struct.
 */
typedef struct
{
    BapClientPrim                type;         /*!< BAP_UNICAST_CLIENT_ENABLE_CFM */
    BapResult                    result;       /*!< Result code. */
    BapProfileHandle             handle;       /*!< BAP Handle */
} BapUnicastClientEnableCfm;

typedef struct
{
    uint32 cigSyncDelay;
    uint32 cisSyncDelay;
    uint16 transportLatencyMtoS;
    uint16 transportLatencyStoM;
    uint16 maxPduMtoS;
    uint16 maxPduStoM;
    uint16 isoInterval;
    uint8  phyMtoS;
    uint8  phyStoM;
    uint8  nse;
    uint8  bnMtoS;
    uint8  bnStoM;
    uint8  ftMtoS;
    uint8  ftStoM;
}BapUnicastClientCisParam;

/*! @brief BapUnicastClientCisConnectInd struct.
 */
typedef struct
{
    BapClientPrim                type;             /*!< BAP_UNICAST_CLIENT_CIS_CONNECT_IND */
    BapResult                    result;           /*!< Result code. */
    BapProfileHandle             handle;           /*!< BAP Handle */
    uint16                     cisHandle;        /*!< CIS handle for cis establishment */
    BapUnicastClientCisParam     cisParams;        /*!< CIS parameters agreed during cis establishment */
    bool                         clientInitiated;  /*!< Client initiated operation */
} BapUnicastClientCisConnectInd;

/*! @brief BapUnicastClientCisConnectCfm struct.
 */
typedef struct
{
    BapClientPrim                type;          /*!< BAP_UNICAST_CLIENT_CIS_CONNECT_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
} BapUnicastClientCisConnectCfm;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_UNICAST_CLIENT_CIS_DISCONNECT_IND */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint16                     cisHandle;     /*!< CIS handle for disconnection */
    uint16                     reason;        /*!< HCI Reason code for disconnection */
} BapUnicastClientCisDisconnectInd;

/*! @brief BapUnicastClientCisDisconnectCfm struct.
 */
typedef struct
{
    BapClientPrim                type;          /*!< BAP_UNICAST_CLIENT_CIS_DISCONNECT_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint16                     cisHandle;     /*!< CIS handle for disconnection */
} BapUnicastClientCisDisconnectCfm;

/*! @brief BapSetupDataPathCfm struct.
 */
typedef struct
{
    BapClientPrim              type;          /*!< BAP_CLIENT_SETUP_DATA_PATH_CFM */
    BapResult                  result;        /*!< Result code. */
    BapProfileHandle           handle;        /*!< BAP Handle */
    uint16                   isoHandle;     /*!< CIS or BIS handle */
}BapSetupDataPathCfm;

/*! @brief BapRemoveDataPathCfm struct.
 */
typedef struct
{
    BapClientPrim              type;          /*!< BAP_CLIENT_REMOVE_DATA_PATH_CFM */
    BapResult                  result;        /*!< Result code. */
    BapProfileHandle           handle;        /*!< BAP Handle */
    uint16                   isoHandle;     /*!< CIS or BIS handle */
} BapRemoveDataPathCfm;

/*! @brief BapUnicastClientUpdateMetadataInd struct.
 */
typedef struct
{
    BapClientPrim                type;                    /*!< BAP_UNICAST_CLIENT_UPDATE_METADATA_IND */
    BapResult                    result;                  /*!< Result code. */
    BapProfileHandle             handle;                  /*!< BAP Handle */
    uint8                        aseId;                   /*!< ASE Identifier */
    uint16                       streamingAudioContexts;  /*!< Bitmask of Audio Context type values */
    uint8                        metadataLength;          /*!< metadata length */
    uint8*                       metadata;                /*!< metadata LTV format and can hve vendor specific data as well */
} BapUnicastClientUpdateMetadataInd;

/*! @brief BapUnicastClientUpdateMetadataCfm struct.
 */
typedef struct
{
    BapClientPrim                type;          /*!< BAP_UNICAST_CLIENT_UPDATE_METADATA_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
} BapUnicastClientUpdateMetadataCfm;

/*! @brief BapUnicastClientDisableInd struct.
 */
typedef struct
{
    BapClientPrim                type;             /*!< BAP_UNICAST_CLIENT_DISABLE_IND */
    BapResult                    result;           /*!< Result code. */
    BapProfileHandle             handle;           /*!< BAP Handle */
    uint8                      aseId;            /*!< ASE Identifier */
    uint8                      cisId;            /*!< CIS ID */
    BapAseState                  aseState;         /*!< ASE State */
    bool                         clientInitiated;  /*!< Client initiated operation */
} BapUnicastClientDisableInd;

/*! @brief BapUnicastClientDisableCfm struct.
 */
typedef struct
{
    BapClientPrim                type;         /*!< BAP_UNICAST_CLIENT_DISABLE_CFM */
    BapResult                    result;       /*!< Result code. */
    BapProfileHandle             handle;       /*!< BAP Handle */
} BapUnicastClientDisableCfm;

/*! @brief BapUnicastClientReleaseInd struct.
 */
typedef struct
{
    BapClientPrim                type;             /*!< BAP_UNICAST_CLIENT_RELEASE_IND */
    BapResult                    result;           /*!< Result code. */
    BapProfileHandle             handle;           /*!< BAP Handle */
    uint8                      aseId;            /*!< ASE Identifier */
    uint8                      cisId;            /*!< CIS ID */
    BapAseState                  aseState;         /*!< ASE State */
    bool                         clientInitiated;  /*!< Client initiated operation */
} BapUnicastClientReleaseInd;

/*! @brief BapUnicastClientReleaseCfm struct.
 */
typedef struct
{
    BapClientPrim                type;         /*!< BAP_UNICAST_CLIENT_RELEASE_CFM */
    BapResult                    result;       /*!< Result code. */
    BapProfileHandle             handle;       /*!< BAP Handle */
} BapUnicastClientReleaseCfm;

/*! @brief BapUnicastClientReleasedInd struct.
 */
typedef struct
{
    BapClientPrim           type;         /*!< BAP_UNICAST_CLIENT_RELEASED_IND */
    BapProfileHandle        handle;       /*!< BAP Handle */
    uint8                 aseId;        /*!< ASE Identifier */
    uint8                 cisId;        /*!< CIS ID */
    BapAseState             aseState;     /*!< ASE State */
} BapUnicastClientReleasedInd;

/*! @brief BapUnicastClientReceiverReadyInd struct.
 */
typedef struct
{
    BapClientPrim      type;             /*!< BAP_UNICAST_CLIENT_RECEIVER_READY_IND */
    BapResult          result;           /*!< Result code. */
    BapProfileHandle   handle;           /*!< BAP Handle */
    uint8            readyType;        /*!< Ready type Start/Stop */
    uint8            aseId;            /*!< ASE Identifier */
    BapAseState        aseState;         /*!< ASE State */
    bool               clientInitiated;  /*!< Client initiated operation */
} BapUnicastClientReceiverReadyInd;

/*! @brief BapUnicastClientReceiverReadyCfm struct.
 */
typedef struct
{
    BapClientPrim                type;          /*!< BAP_UNICAST_CLIENT_RECEIVER_READY_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      readyType;     /*!< Ready type Start/Stop */
} BapUnicastClientReceiverReadyCfm;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_SRC_SET_BROADCAST_ID */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint32                       broadcastId;   /*!< BIG Broadcast ID */
} BapInternalBroadcastSrcSetBroadcastId;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_SRC_CONFIGURE_STREAM_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      bigId;
    uint8                      ownAddrType;
    uint32                     presentationDelay;
    uint8                      numSubgroup;
    BapBigSubgroup              *subgroupInfo;
    BapBroadcastInfo            *broadcastInfo;
    uint8                        bigNameLen;
    char*                        bigName;
} BapInternalBroadcastSrcConfigureStreamReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_SRC_CONFIGURE_STREAM_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
} BapBroadcastSrcConfigureStreamCfm;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      bigId;        /*!< BIG identifier */
    BapBigConfigParam           *bigConfigParameters;  /*!< BIG config parameters */
    uint8                      numBis;
    bool                         encryption;
    uint8                      *broadcastCode;
} BapInternalBroadcastSrcEnableStreamReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_SRC_ENABLE_STREAM_TEST_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      bigId;        /*!< BIG identifier */
    BapBigTestConfigParam       *bigTestConfigParameters;  /*!< BIG Test config parameters */
    uint8                      numBis;
    bool                         encryption;
    uint8                      *broadcastCode;
} BapInternalBroadcastSrcEnableStreamTestReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_SRC_ENABLE_STREAM_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      bigId;        /*!< BIG identifier */
    uint32                     bigSyncDelay; /*!< BIG Sync delay */
    BapBigParam                  bigParameters;
    uint8                      numBis;       /* Number of BISes in BIG */
    uint16                     *bisHandles;  /* Connection handle of BISes */
} BapBroadcastSrcEnableStreamCfm;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_SRC_ENABLE_STREAM_TEST_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      bigId;        /*!< BIG identifier */
    uint32                     bigSyncDelay; /*!< BIG Sync delay */
    BapBigParam                  bigParameters;
    uint8                      numBis;       /* Number of BISes in BIG */
    uint16                     *bisHandles;  /* Connection handle of BISes */
} BapBroadcastSrcEnableStreamTestCfm;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_SRC_DISABLE_STREAM_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      bigId;        /*!< BIG identifier */
} BapInternalBroadcastSrcDisableStreamReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_SRC_DISABLE_STREAM_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      bigId;        /*!< BIG identifier */
} BapBroadcastSrcDisableStreamCfm;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_SRC_RELEASE_STREAM_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      bigId;        /*!< BIG identifier */
} BapInternalBroadcastSrcReleaseStreamReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_SRC_UPDATE_METADATA_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      bigId;        /*!< BIG identifier */
    uint8                      numSubgroup;
    BapMetadata                  *subgroupMetadata;
} BapInternalBroadcastSrcUpdateMetadataStreamReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_SRC_RELEASE_STREAM_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      bigId;        /*!< BIG identifier */
} BapBroadcastSrcReleaseStreamCfm;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_SRC_UPDATE_METADATA_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      bigId;        /*!< BIG identifier */
} BapBroadcastSrcUpdateMetadataStreamCfm;

/*! Broadcast Assistant flags */
#define BROADCAST_SRC_COLLOCATED      0x01
#define BROADCAST_SRC_NON_COLLOCATED  0x02
#define BROADCAST_SRC_ALL             0x03

typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_ASSISTANT_START_SCAN_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      flags;         /*!< See flags above */
    uint16                     filterContext; /*!< Refer BAP Audio Context values */

    /* Below values are used when non-collocated scan is also enabled */
    uint8                      scanFlags;            /*!< Scanning flags */
    uint8                      ownAddressType;       /*!< Local address type */
    uint8                      scanningFilterPolicy; /*! < Scanning filter policy */
} BapInternalBroadcastAssistantStartScanReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_START_SCAN_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint16                     scanHandle;    /*!< scanHandle */
} BapBroadcastAssistantStartScanCfm;

typedef struct
{
    BapClientPrim                type;            /*!< BAP_BROADCAST_ASSISTANT_SRC_REPORT_IND */
    BapProfileHandle             handle;          /*!< BAP Handle */
    TYPED_BD_ADDR_T              sourceAddrt;     /*!< BT address of the Broadcast Source State */
    uint8                      advSid;          /*!< Advertising SID */
    uint8                      advHandle;       /*!< advHandle valid in case of collocated Broadcast
                                                      Source. For others it will be 0 */
    bool                         collocated;      /*!< Whether SRC is collocated(local) or not */
    uint32                     broadcastId;   /*!< Broadcast ID */
    /* Level 1 */
    uint8                      numSubgroup;

    /* Level 2 Sub Group and Level 3 */
    BapBigSubgroup              *subgroupInfo;
    uint8                        bigNameLen;
    char                        *bigName; 
    uint8                        serviceDataLen;  /*!< Length of service data */
    uint8                       *serviceData;     /*!< serviceData contains "Service Data 16-bit UUID" AD type of
                                                       services such as TMAS, GMAS and PBP etc. in LTV format  */
 }BapBroadcastAssistantSrcReportInd;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_ASSISTANT_STOP_SCAN_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint16                     scanHandle;    /*!< scanHandle returned in BAP_BROADCAST_ASSISTANT_START_SCAN_CFM */
} BapInternalBroadcastAssistantStopScanReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_STOP_SCAN_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
} BapBroadcastAssistantStopScanCfm;


typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    TYPED_BD_ADDR_T              addrt;
    uint8                      advSid;
} BapInternalBroadcastAssistantSyncToSrcStartReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_START_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint16                     syncHandle;   /*!< Sync handle of the PA */
    uint8                      advSid;
    TYPED_BD_ADDR_T              addrt;
    uint8                      advPhy;
    uint16                     periodicAdvInterval;
    uint8                      advClockAccuracy;
} BapBroadcastAssistantSyncToSrcStartCfm;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
} BapInternalBroadcastAssistantSyncToSrcCancelReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
} BapBroadcastAssistantSyncToSrcCancelCfm;


typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint16                     syncHandle;   /*!< Sync handle of the PA received in
                                                     BapBroadCastAssistantStartSyncToSrcCfm */
} BapInternalBroadcastAssistantSyncToSrcTerminateReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint16                     syncHandle;
} BapBroadcastAssistantSyncToSrcTerminateCfm;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_SYNC_LOSS_IND */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint16                     syncHandle;
} BapBroadcastAssistantSyncLossInd;


typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      sourceId;
    bool                         allSources;
    bool                         notificationsEnable;
} BapInternalBroadcastAssistantBrsRegisterForNotifcationReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_BRS_REGISTER_FOR_NOTIFICATION_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      sourceId;      /*!< Source_id of the Broadcast
                                                     Receive State characteristic */
} BapBroadcastAssistantBrsRegisterForNotifcationCfm;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_CCC_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      sourceId;
    bool                         allSources;
} BapInternalBroadcastAssistantReadBrsCccReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_READ_BRS_CCC_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      sourceId;      /*!< Source_id of the Broadcast
                                                     Receive State characteristic */
    uint16                     size_value;
    uint8                      *value;
} BapBroadcastAssistantReadBrsCccCfm;


typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_ASSISTANT_READ_BRS_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      sourceId;
    bool                         allSources;
} BapInternalBroadcastAssistantReadBrsReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_READ_BRS_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      sourceId;      /*! Source_id of the Broadcast
                                                    Receive State characteristic */
    BD_ADDR_T                    sourceAddress;
    uint8                      advertiseAddType;
    uint8                      advSid;
    uint8                      paSyncState;
    uint8                      bigEncryption;
    uint32                     broadcastId;   /*!< Broadcast ID */
    uint8                     *badCode;
    uint8                      numSubGroups;
    BapSubgroupInfo             *subGroupInfo;
} BapBroadcastAssistantReadBrsCfm;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_BRS_IND */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      sourceId;     /*! Source_id of the Broadcast
                                                    Receive State characteristic */
    BD_ADDR_T                    sourceAddress;
    uint8                      advertiseAddType;
    uint8                      advSid;
    uint8                      paSyncState;
    uint8                      bigEncryption;
    uint32                     broadcastId;   /*!< Broadcast ID */
    uint8                     *badCode;
    uint8                      numSubGroups;
    BapSubgroupInfo             *subGroupInfo;
} BapBroadcastAssistantBrsInd;


typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_ASSISTANT_ADD_SRC_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    BD_ADDR_T                    sourceAddrt;
    uint8                      advertiserAddressType;
    bool                         srcCollocated;
    uint16                     syncHandle;    /*!< SyncHandle of PA or advHandle of
                                                      collocated Broadcast src */
    uint8                      sourceAdvSid;  /*! Advertising SID */
    uint8                      paSyncState;   /*! PA Synchronization state */
    uint16                     paInterval;
    uint32                     broadcastId;   /*!< Broadcast ID */
    uint8                      numbSubGroups; /*! Number of subgroups */
    BapSubgroupInfo              *subgroupInfo[BAP_MAX_SUPPORTED_NUM_SUBGROUPS];

} BapInternalBroadcastAssistantAddSrcReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_ADD_SRC_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
} BapBroadcastAssistantAddSrcCfm;


typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_ASSISTANT_MODIFY_SRC_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      sourceId;      /*!< Advertising SID */
    bool                         srcCollocated;
    uint16                     syncHandle;    /*!< SyncHandle of PA or advHandle of
                                                      collocated Broadcast src */
    uint8                      sourceAdvSid;  /*!< Advertising SID */
    uint8                      paSyncState;   /*!< PA Synchronization state */
    uint16                     paInterval;
    uint8                      numbSubGroups; /*!< Number of subgroups */
    BapSubgroupInfo              *subgroupInfo[BAP_MAX_SUPPORTED_NUM_SUBGROUPS];
} BapInternalBroadcastAssistantModifySrcReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_MODIFY_SRC_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
} BapBroadcastAssistantModifySrcCfm;


typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_BROADCAST_ASSISTANT_REMOVE_SRC_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      sourceId;      /*!< Source_id of the Broadcast
                                                     Receive State characteristic */
} BapInternalBroadcastAssistantRemoveSrcReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_REMOVE_SRC_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
} BapBroadcastAssistantRemoveSrcCfm;


typedef struct
{
    BapClientPrim                type;             /*!< BAP_INTERNAL_BROADCAST_ASSISTANT_SET_CODE_RSP */
    BapProfileHandle             handle;           /*!< BAP Handle */
    uint8                      sourceId;         /*!< Source_id of the Broadcast
                                                        Receive State characteristic */
    uint8                      *broadcastCode;   /*! Value of Broadcast Code to set */
} BapInternalBroadcastAssistantSetCodeRsp;

#define BROADCAST_CODE_REQUESTED         0x01
#define BROADCAST_CODE_BAD_CODE          0x02
typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_SET_CODE_IND */
    BapProfileHandle             handle;        /*!< BAP Handle */
    uint8                      sourceId;      /*!< Source_id of the Broadcast
                                                     Receive State characteristic */
    uint8                      flags;         /*!< See flags above */
} BapBroadcastAssistantSetCodeInd;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_INTERNAL_SET_CONTROL_POINT_OP_REQ */
    BapProfileHandle             handle;        /*!< BAP Handle */
    bool                         controlOpResponse;
    bool                         longWrite;
} BapInternalSetControlPointOpReq;

typedef struct
{
    BapClientPrim                type;          /*!< BAP_BROADCAST_ASSISTANT_CONTROL_POINT_OP_CFM */
    BapResult                    result;        /*!< Result code. */
    BapProfileHandle             handle;        /*!< BAP Handle */
} BapSetControlPointOpCfm;

typedef struct
{
    uint16 advEventProperties; /* Advertising type.*/
    uint16 advIntervalMin; /* Minimum advertising interval N = 0x20 to 0xFFFFFF  (Time = N * 0.625 ms)*/
    uint16 advIntervalMax; /* Maximum advertising interval N = 0x20 to 0xFFFFFF  (Time = N * 0.625 ms)*/
    uint16 primaryAdvPhy; /* PHY for advertising paackets on Primary advertising channels
                           * 1 - LE 1M, 3 - LE Coded */
    uint8  primaryAdvChannelMap; /* Bit mask field (bit 0 = Channel 37, bit 1 = Channel 38 and bit 2 = Channel 39) */
    uint8 secondaryAdvMaxSkip; /* Maximum advertising events on the primary advertising
                                * channel that can be skipped before sending an AUX_ADV_IND.*/
    uint16 secondaryAdvPhy;  /* PHY for advertising paackets on Secondary advertising channels 
                              *  1 - LE 1M, 2 - LE 2M, 3 - LE Coded */
    uint16 advSid;     /* Advertsing set ID.
                        * CM_EXT_ADV_SID_INVALID            - For legacy advertising
                        * CM_EXT_ADV_SID_ASSIGNED_BY_STACK  - Stack will assign unique value
                        * 0 to 15                           - Application decides unique value
                        * CM_EXT_ADV_SID_SHARE + (0 to 15)  - More than 1 advertising set can have this value */
    uint16 periodicAdvIntervalMin;  /* Range: 0x0006 to 0xFFFF
                                       Time = N * 1.25 ms */
    uint16 periodicAdvIntervalMax;  /* Range: 0x0006 to 0xFFFF
                                       Time = N * 1.25 ms */
    int8 advertisingTransmitPower;  /* Advertising Tx Power
                                     * Range: -127 to 20 dbM
                                     * Default value 0x7F(Host has no preference) */
} BapBroadcastSrcAdvParams;


/*! \brief Union of all BAP primitives */
typedef union
{
    /* Common fields */
    BapClientPrim                                                 type;                                      /*!< Shared for all primitives */
    /* Downstream */
    BapInternalInitReq                                            bapInternalInitReq;
    BapInternalDeinitReq                                          bapInternalDeinitReq;
    BapInternalAddPacRecordReq                                    bapInternalAddPacRecordReq;
    BapInternalRemovePacRecordReq                                 bapInternalRemovePacRecordReq;
    BapInternalDiscoverAudioRoleReq                               bapInternalDiscoverAudioRoleReq;
    BapInternalDiscoverRemoteAudioCapabilityReq                   bapInternalDiscoverRemoteAudioCapabilityReq;
    BapInternalRegisterPacsNotificationReq                        bapInternalRegisterPacsNotificationReq;
    BapInternalGetRemoteAudioLocationReq                          bapInternalGetRemoteAudioLocationReq;
    BapInternalSetRemoteAudioLocationReq                          bapInternalSetRemoteAudioLocationReq;
    BapInternalDiscoverAudioContextReq                            bapInternalDiscoverAudioContextReq;

    BapInternalUnicastClientRegisterAseNotificationReq            bapInternalUnicastClientRegisterAseNotificationReq;
    BapInternalUnicastClientReadAseInfoReq                        bapInternalUnicastClientReadAseInfoReq;
    BapInternalUnicastClientCodecConfigureReq                     bapInternalUnicastClientCodecConfigureReq;
    BapInternalUnicastClientCigConfigureReq                       bapInternalUnicastClientCigConfigureReq;
    BapInternalUnicastClientCigTestConfigureReq                   bapInternalUnicastClientCigTestConfigureReq;
    BapInternalUnicastClientCigRemoveReq                          bapInternalUnicastClientCigRemoveReq;
    BapInternalUnicastClientQosConfigureReq                       bapInternalUnicastClientQosConfigureReq;
    BapInternalUnicastClientEnableReq                             bapInternalUnicastClientEnableReq;
    BapInternalUnicastClientCisConnectReq                         bapInternalUnicastClientCisConnectReq;
    BapInternalUnicastClientCisDisconnectReq                      bapInternalUnicastClientCisDisconnectReq;
    BapInternalSetupDataPathReq                                   bapInternalSetupDataPathReq;
    BapInternalRemoveDataPathReq                                  bapInternalRemoveDataPathReq;
    BapInternalUnicastClientDisableReq                            bapInternalUnicastClientDisableReq;
    BapInternalUnicastClientReleaseReq                            bapInternalUnicastClientReleaseReq;
    BapInternalUnicastClientUpdateMetadataReq                     bapInternalUnicastClientUpdateMetadataReq;
    BapInternalUnicastClientReceiverReadyReq                      bapInternalUnicastClientReceiverReadyReq;
    BapInternalSetControlPointOpReq                               bapInternalSetControlPointOpReq;

    BapInternalBroadcastSrcConfigureStreamReq                     bapInternalBroadcastSrcConfigureStreamReq;
    BapInternalBroadcastSrcEnableStreamReq                        bapInternalBroadcastSrcEnableStreamReq;
    BapInternalBroadcastSrcEnableStreamTestReq                    bapInternalBroadcastSrcEnableStreamTestReq;
    BapInternalBroadcastSrcDisableStreamReq                       bapInternalBroadcastSrcDisableStreamReq;
    BapInternalBroadcastSrcReleaseStreamReq                       bapInternalBroadcastSrcReleaseStreamReq;
    BapInternalBroadcastSrcUpdateMetadataStreamReq                bapInternalBroadcastSrcUpdateMetadataStreamReq;


    BapInternalBroadcastAssistantStartScanReq                     bapInternalBroadcastAssistantStartScanReq;
    BapInternalBroadcastAssistantStopScanReq                      bapInternalBroadcastAssistantStopScanReq;
    BapInternalBroadcastAssistantSyncToSrcStartReq                bapInternalBroadcastAssistantSyncToSrcStartReq;
    BapInternalBroadcastAssistantSyncToSrcCancelReq               bapInternalBroadcastAssistantSyncToSrcCancelReq;
    BapInternalBroadcastAssistantSyncToSrcTerminateReq            bapInternalBroadcastAssistantSyncToSrcTerminateReq;
    BapInternalBroadcastAssistantBrsRegisterForNotifcationReq     bapInternalBroadcastAssistantBrsRegisterForNotifcationReq;
    BapInternalBroadcastAssistantReadBrsCccReq                    bapInternalBroadcastAssistantReadBrsCccReq;
    BapInternalBroadcastAssistantReadBrsReq                       bapInternalBroadcastAssistantReadBrsReq;
    BapInternalBroadcastAssistantAddSrcReq                        bapInternalBroadcastAssistantAddSrcReq;
    BapInternalBroadcastAssistantModifySrcReq                     bapInternalBroadcastAssistantModifySrcReq;
    BapInternalBroadcastAssistantRemoveSrcReq                     bapInternalBroadcastAssistantRemoveSrcReq;
    BapInternalBroadcastAssistantSetCodeRsp                       bapInternalBroadcastAssistantSetCodeRsp;
    BapInternalBroadcastSrcSetBroadcastId                         bapInternalBroadcastSrcSetBroadcastId;

    /* Upstream */
    BapInitCfm                                                    bapInitCfm;
    BapDeinitCfm                                                  bapDeinitCfm;
    BapAddPacRecordCfm                                            bapAddPacRecordCfm;
    BapDiscoverAudioRoleCfm                                       bapDiscoverAudioRoleCfm;
    BapDiscoverRemoteAudioCapabilityCfm                           bapDiscoverRemoteAudioCapabilityCfm;
    BapRegisterPacsNotificationCfm                                bapRegisterPacsNotificationCfm;
    BapPacsNotificationInd                                        bapPacsNotificationInd;
    BapPacsAudioCapabilityNotificationInd                         bapPacsAudioCapabilityNotificationInd;
    BapGetRemoteAudioLocationCfm                                  bapGetRemoteAudioLocationCfm;
    BapSetRemoteAudioLocationCfm                                  bapSetRemoteAudioLocationCfm;
    BapDiscoverAudioContextCfm                                    bapDiscoverAudioContextCfm;

    BapUnicastClientRegisterAseNotificationCfm                    bapUnicastClientRegisterAseNotificationCfm;
    BapUnicastClientReadAseInfoCfm                                bapUnicastClientReadAseInfoCfm;
    BapUnicastClientCodecConfigureInd                             bapUnicastClientCodecConfigureInd;
    BapUnicastClientCodecConfigureCfm                             bapUnicastClientCodecConfigureCfm;
    BapUnicastClientQosConfigureInd                               bapUnicastClientQosConfigureInd;
    BapUnicastClientQosConfigureCfm                               bapUnicastClientQosConfigureCfm;
    BapUnicastClientCigConfigureCfm                               bapUnicastClientCigConfigureCfm;
    BapUnicastClientRemoveCigCfm                                  bapUnicastClientRemoveCigCfm;

    BapUnicastClientEnableInd                                     bapUnicastClientEnableInd;
    BapUnicastClientEnableCfm                                     bapUnicastClientEnableCfm;
    BapUnicastClientCisConnectInd                                 bapUnicastClientCisConnectInd;
    BapUnicastClientCisConnectCfm                                 bapUnicastClientCisConnectCfm;
    BapUnicastClientCisDisconnectInd                              bapUnicastClientCisDisconnectInd;
    BapUnicastClientCisDisconnectCfm                              bapUnicastClientCisDisconnectCfm;
    BapSetupDataPathCfm                                           bapSetupDataPathCfm;
    BapRemoveDataPathCfm                                          bapRemoveDataPathCfm;
    BapUnicastClientDisableInd                                    bapUnicastClientDisableInd;
    BapUnicastClientDisableCfm                                    bapUnicastClientDisableCfm;
    BapUnicastClientReleaseInd                                    bapUnicastClientReleaseInd;
    BapUnicastClientReleaseCfm                                    bapUnicastClientReleaseCfm;
    BapUnicastClientReleasedInd                                   bapUnicastClientReleasedInd;
    BapUnicastClientReceiverReadyInd                              bapUnicastClientReceiverReadyInd;
    BapUnicastClientReceiverReadyCfm                              bapUnicastClientReceiverReadyCfm;
    BapUnicastClientUpdateMetadataInd                             bapUnicastClientUpdateMetadataInd;
    BapUnicastClientUpdateMetadataCfm                             bapUnicastClientUpdateMetadataCfm;
    BapSetControlPointOpCfm                                       bapSetControlPointOpCfm;


    BapBroadcastSrcConfigureStreamCfm                             bapBroadcastSrcConfigureStreamCfm;
    BapBroadcastSrcEnableStreamCfm                                bapBroadcastSrcEnableStreamCfm;
    BapBroadcastSrcEnableStreamTestCfm                            bapBroadcastSrcEnableStreamTestCfm;
    BapBroadcastSrcDisableStreamCfm                               bapBroadcastSrcDisableStreamCfm;
    BapBroadcastSrcReleaseStreamCfm                               bapBroadcastSrcReleaseStreamCfm;
    BapBroadcastSrcUpdateMetadataStreamCfm                        bapBroadcastSrcUpdateMetadataCfm;

    BapBroadcastAssistantStartScanCfm                             bapBroadcastAssistantStartScanCfm;
    BapBroadcastAssistantStopScanCfm                              bapBroadcastAssistantStopScanCfm;
    BapBroadcastAssistantSrcReportInd                             bapBroadcastAssistantSrcReportInd;
    BapBroadcastAssistantSyncToSrcStartCfm                        bapBroadcastAssistantSyncToSrcStartCfm;
    BapBroadcastAssistantSyncToSrcCancelCfm                       bapBroadcastAssistantSyncToSrcCancelCfm;
    BapBroadcastAssistantSyncToSrcTerminateCfm                    bapBroadcastAssistantSyncToSrcTerminateCfm;
    BapBroadcastAssistantBrsRegisterForNotifcationCfm             bapBroadcastAssistantBrsRegisterForNotificationCfm;
    BapBroadcastAssistantReadBrsCccCfm                            bapBroadcastAssistantReadBrsCccCfm;
    BapBroadcastAssistantReadBrsCfm                               bapBroadcastAssistantReadBrsCfm;
    BapBroadcastAssistantBrsInd                                   bapBroadcastAssistantBrsInd;
    BapBroadcastAssistantAddSrcCfm                                bapBroadcastAssistantAddSrcCfm;
    BapBroadcastAssistantModifySrcCfm                             bapBroadcastAssistantModifySrcCfm;
    BapBroadcastAssistantRemoveSrcCfm                             bapBroadcastAssistantRemoveSrcCfm;
    BapBroadcastAssistantSetCodeInd                               bapBroadcastAssistantSetCodeInd;
    BapBroadcastAssistantSyncLossInd                              bapBroadcastAssistantSyncLossInd;

} BapUPrim;

#ifdef __cplusplus
}
#endif

#endif /* ifndef BAP_CLIENT_PRIM_H */
