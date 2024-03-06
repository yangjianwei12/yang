/****************************************************************************
* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
* %%version

@file    bap_server_prim.h
@brief   Header file for the Basic Audio Profile Server library.

        This file provides documentation for the BAP Server library
        API (library name: bap_server_prim).
************************************************************************* ***/

#ifndef BAP_SERVER_PRIM_H
#define BAP_SERVER_PRIM_H

#include "service_handle.h"
#include "csr_bt_types.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_tasks.h"
#include "csr_pmem.h"
#include "csr_bt_addr.h"
#include "gatt_pacs_server.h"
#include "gatt_ascs_server.h"
#include "gatt_bass_server.h"


#define BAP_SRVR_PRIM             (SYNERGY_EVENT_BASE + BAP_SERVER_PRIM)

/*!
    \brief Connection id type.
*/

typedef connection_id_t ConnId;

/*!
    \brief BAP Profile handle type.
*/
typedef ServiceHandle bapProfileHandle;

/*!
    \brief BAP status code type as per Basic Audio profile specification.
     Note: Some of the status code value to be refered from Core Spec [Vol 1] Part F,
          Controller Error Codes as per documentation.
*/
typedef uint16 bapStatus;

/*! Values for the BAP status code */
#define BAP_SERVER_STATUS_SUCCESS              ((bapStatus)0x0000u)  /*! Request was a success*/
#define BAP_SERVER_STATUS_IN_PROGRESS          ((bapStatus)0x0001u)  /*! Request in progress*/
#define BAP_SERVER_STATUS_INVALID_PARAMETER    ((bapStatus)0x0002u)  /*! Invalid parameter was supplied*/
#define BAP_SERVER_STATUS_NOT_ALLOWED          ((bapStatus)0x0003u)  /*! Request is not allowed*/
#define BAP_SERVER_STATUS_FAILED               ((bapStatus)0x0004u)  /*! Request has failed*/
#define BAP_SERVER_STATUS_BC_SOURCE_IN_SYNC    ((bapStatus)0x0005u)  /*! Request has failed, because the selected Broadcast Source is in sync with the PA and/or BIS/BIG*/
#define BAP_SERVER_STATUS_INVALID_SOURCE_ID    ((bapStatus)0x0006u)  /*! Request has failed, because an invalid source id was supplied*/
#define BAP_SERVER_STATUS_NO_EMPTY_BRS         ((bapStatus)0x0007u)  /*!> Request has failed, because no source id was supplied and all the Broadcast Receive State Characteristics are full */
#define BAP_SERVER_STATUS_BRS_NOT_CHANGED      ((bapStatus)0x0008u)  /*!> BRS contents are not changed and hence NTFs are not sent over GATT*/

#define BAP_SERVER_DATA_PATH_ID_RAW_STREAM_ENDPOINTS_ONLY  0x06
#define BAP_SERVER_DATA_PATH_DIRECTION_HOST_TO_CONTROLLER  0x00
#define BAP_SERVER_DATA_PATH_DIRECTION_CONTROLLER_TO_HOST  0x01

#define BAP_CODEC_ID_SIZE                          ((uint8)0x05)

typedef GattAscsAseResultValue bapServerResponse;

/*! Values for the BAP server role */
typedef uint8 BapServerRole;
#define BAP_SERVER_UNICAST_ROLE                    ((BapServerRole)0x01)
#define BAP_SERVER_BROADCAST_ROLE                  ((BapServerRole)0x02)

/*! Values for ASE direction type */
typedef uint8 AseDirectionType;
#define ASE_DIRECTION_UNINITIALISED     ((AseDirectionType)0x00)
#define ASE_DIRECTION_AUDIO_SINK        ((AseDirectionType)0x01) /*! Describes an ASE on which audio data travels from the client to the server */
#define ASE_DIRECTION_AUDIO_SOURCE      ((AseDirectionType)0x02) /*! Describes an ASE on which audio data travels from the server to the client */

/*! \brief LE Audio context types as defined in  BAPS_Assigned_Numbers_v4 */
typedef uint16 AudioContextType;
#define AUDIO_CONTEXT_TYPE_UNKNOWN              ((AudioContextType)0x0000)
#define AUDIO_CONTEXT_TYPE_UNSPECIFIED          ((AudioContextType)0x0001)
#define AUDIO_CONTEXT_TYPE_COVERSATIONAL        ((AudioContextType)0x0002)
#define AUDIO_CONTEXT_TYPE_MEDIA                ((AudioContextType)0x0004)
#define AUDIO_CONTEXT_TYPE_GAME                 ((AudioContextType)0x0008)
#define AUDIO_CONTEXT_TYPE_INSTRUCTIONAL        ((AudioContextType)0x0010)
#define AUDIO_CONTEXT_TYPE_VOICE_ASSISTANT      ((AudioContextType)0x0020)
#define AUDIO_CONTEXT_TYPE_LIVE                 ((AudioContextType)0x0040)
#define AUDIO_CONTEXT_TYPE_SOUND_EFFECTS        ((AudioContextType)0x0080)
#define AUDIO_CONTEXT_TYPE_NOTIFICATIONS        ((AudioContextType)0x0100)
#define AUDIO_CONTEXT_TYPE_RINGTONE             ((AudioContextType)0x0200)
#define AUDIO_CONTEXT_TYPE_ALERTS               ((AudioContextType)0x0400)
#define AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM      ((AudioContextType)0x0800)

/*! \brief LE Audio location types as defined in  spec */
typedef uint32 AudioLocationType;
#define AUDIO_LOCATION_MONO                    ((AudioLocationType)0x00000000)
#define AUDIO_LOCATION_FRONT_LEFT              ((AudioLocationType)0x00000001)
#define AUDIO_LOCATION_FRONT_RIGHT             ((AudioLocationType)0x00000002)
#define AUDIO_LOCATION_FRONT_CENTER            ((AudioLocationType)0x00000004)
#define AUDIO_LOCATION_LOW_FREQUENCY_EFFECT1   ((AudioLocationType)0x00000008)
#define AUDIO_LOCATION_BACK_LEFT               ((AudioLocationType)0x00000010)
#define AUDIO_LOCATION_BACK_RIGHT              ((AudioLocationType)0x00000020)
#define AUDIO_LOCATION_FRONT_LEFT_OF_CENTER    ((AudioLocationType)0x00000040)
#define AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER   ((AudioLocationType)0x00000080)
#define AUDIO_LOCATION_BACK_CENTER             ((AudioLocationType)0x00000100)
#define AUDIO_LOCATION_LOW_FREQUENCY_EFFECT2   ((AudioLocationType)0x00000200)
#define AUDIO_LOCATION_SIDE_LEFT               ((AudioLocationType)0x00000400)
#define AUDIO_LOCATION_SIDE_RIGHT              ((AudioLocationType)0x00000800)
#define AUDIO_LOCATION_TOP_FRONT_LEFT          ((AudioLocationType)0x00001000)
#define AUDIO_LOCATION_TOP_FRONT_RIGHT         ((AudioLocationType)0x00002000)
#define AUDIO_LOCATION_TOP_FRONT_CENTER        ((AudioLocationType)0x00004000)
#define AUDIO_LOCATION_TOP_CENTER              ((AudioLocationType)0x00008000)
#define AUDIO_LOCATION_TOP_BACK_LEFT           ((AudioLocationType)0x00010000)
#define AUDIO_LOCATION_TOP_BACK_RIGHT          ((AudioLocationType)0x00020000)
#define AUDIO_LOCATION_TOP_SIDE_LEFT           ((AudioLocationType)0x00040000)
#define AUDIO_LOCATION_TOP_SIDE_RIGHT          ((AudioLocationType)0x00080000)
#define AUDIO_LOCATION_TOP_SIDE_CENTER         ((AudioLocationType)0x00100000)
#define AUDIO_LOCATION_BOTTOM_FRONT_CENTER     ((AudioLocationType)0x00200000)
#define AUDIO_LOCATION_BOTTOM_FRONT_LEFT       ((AudioLocationType)0x00400000)
#define AUDIO_LOCATION_BOTTOM_FRONT_RIGHT      ((AudioLocationType)0x00800000)
#define AUDIO_LOCATION_FRONT_LEFT_WIDER        ((AudioLocationType)0x01000000)
#define AUDIO_LOCATION_FRONT_RIGHT_WIDER       ((AudioLocationType)0x02000000)
#define AUDIO_LOCATION_SURROUND_LEFT           ((AudioLocationType)0x04000000)
#define AUDIO_LOCATION_SURROUND_RIGHT          ((AudioLocationType)0x08000000)

/* Local value to clear spec defined Audio Location */
#define AUDIO_LOCATION_CLEAR                   ((AudioLocationType)0xFFFFFFFF)

/*! Values for ASE target latency */
typedef uint8 AseTargetLatency;
#define TARGET_LATENCY_TARGET_LOWER                    ((AseTargetLatency)0x01)
#define TARGET_LATENCY_TARGET_BALANCE_AND_RELIABLE     ((AseTargetLatency)0x02)
#define TARGET_LATENCY_TARGET_HIGHER_RELIABILITY       ((AseTargetLatency)0x03)

/*! Values for ASE target phy */
typedef uint8 AseTargetPhy;
#define TARGET_PHY_LE_1M_PHY                    ((AseTargetPhy)0x01)
#define TARGET_PHY_LE_2M_PHY                    ((AseTargetPhy)0x02)
#define TARGET_PHY_LE_CODEC_PHY                 ((AseTargetPhy)0x03)

/*! Framing values of ASE */
typedef uint8 AseFraming;
#define FRAMING_UNFRAMED_ISOAL_PDU              ((AseFraming)0x00)
#define FRAMING_FRAMED_ISOAL_PDU                ((AseFraming)0x01)

#define BAP_SERVER_CODEC_ID_LC3                    (0x06)

/*! @brief The set of messages an application task can receive from the BAP Server library.
 */
typedef uint16 BapServerPrim;

#define BAP_SERVER_ASE_CODEC_CONFIGURED_IND     ((BapServerPrim)0)
#define BAP_SERVER_ASE_CODEC_CONFIGURED_CFM     ((BapServerPrim)1)
#define BAP_SERVER_ASE_QOS_CONFIGURED_IND       ((BapServerPrim)2)
#define BAP_SERVER_ASE_ENABLED_IND              ((BapServerPrim)3)
#define BAP_SERVER_CIS_ESTABLISHED_IND          ((BapServerPrim)4)
#define BAP_SERVER_CIS_DISCONNECTED_IND         ((BapServerPrim)5)
#define BAP_SERVER_CIS_DISCONNECTED_CFM         ((BapServerPrim)6)
#define BAP_SERVER_ASE_RECEIVER_START_READY_IND ((BapServerPrim)7)
#define BAP_SERVER_ASE_DISABLED_IND             ((BapServerPrim)8)
#define BAP_SERVER_ASE_RECEIVER_STOP_READY_IND  ((BapServerPrim)9)
#define BAP_SERVER_ASE_UPDATE_METADATA_IND      ((BapServerPrim)10)
#define BAP_SERVER_ASE_RELEASED_IND             ((BapServerPrim)11)
#define BAP_SERVER_SETUP_DATA_PATH_CFM          ((BapServerPrim)12)
#define BAP_SERVER_REMOVE_DATA_PATH_CFM         ((BapServerPrim)13)
#define BAP_SERVER_BASS_SCANNING_STATE_IND      ((BapServerPrim)14)
#define BAP_SERVER_BASS_ADD_SOURCE_IND          ((BapServerPrim)15)
#define BAP_SERVER_BASS_MODIFY_SOURCE_IND       ((BapServerPrim)16)
#define BAP_SERVER_BASS_BROADCAST_CODE_IND      ((BapServerPrim)17)
#define BAP_SERVER_BASS_REMOVE_SOURCE_IND       ((BapServerPrim)18)
#define BAP_SERVER_ISOC_BIG_CREATE_SYNC_CFM     ((BapServerPrim)19)
#define BAP_SERVER_ISOC_BIG_TERMINATE_SYNC_IND  ((BapServerPrim)20)
#define BAP_SERVER_BIGINFO_ADV_REPORT_IND       ((BapServerPrim)21)
#define BAP_SERVER_CONFIG_CHANGE_IND            ((BapServerPrim)22)
#define BAP_SERVER_AVAILABLE_AUDIO_CONTEXT_READ_IND   ((BapServerPrim)23)

typedef uint8 BapServerConfigType;

#define BAP_SERVER_CONFIG_PACS         ((BapServerConfigType)0x01)
#define BAP_SERVER_CONFIG_ASCS         ((BapServerConfigType)0x02)
#define BAP_SERVER_CONFIG_BASS         ((BapServerConfigType)0x03)

/*! Structure which has handle ranges of BAP Server */
typedef struct
{
    uint16      startHandle;
    uint16      endHandle;
} BapServerHandleRange;

/*!
    @brief The fields in this structure are supplied by the profile/application in
           response to an GATT_ASCS_SERVER_CONFIGURE_CODEC_IND.
*/
typedef GattAscsServerConfigureCodecServerInfo BapServerAseCodecParameters;

typedef GattAscsServerConfigureCodecReq  BapServerAseConfigCodecReq;
typedef GattAscsServerUpdateMetadataReq  BapServerAseUpdateMetadataReq;
/*! @brief This structure contains all the information provided by the client for a particular
 *         ASE during the Codec Configuration procedure.
 */
typedef struct
{
    uint8 aseId; /*! The ASE id that is being configured */
    AseDirectionType direction; /*! The ASE direction provided by the client during the Configure Codec Procedure */
} AseCodecConfigType;

/*! @brief This structure is sent to the Profile/Application to indicate that a client has
 *         initiated the Configure Codec procedure.
 */
typedef struct
{
    BapServerPrim      type;  /*! BAP_SERVER_ASE_CODEC_CONFIGURED_IND */
    ConnId          connectionId; /*! The connection on which ASEs are being configured. */
    AseCodecConfigType aseCodecConfig;
} BapServerAseCodecConfiguredInd;

/*! @brief This structure contains all the information provided by the client for a particular
 *         ASE during the QoS Configuration procedure.
 */
typedef struct
{
    uint8 aseId; /*! The ASE id that is being configured */
    uint8 cisId; /*! Identifier for a CIS that has been assigned by the client host */
    uint32 sampleRate; /*! Sampling rate value */
    uint16 sduSize;     /*!  The time in milliseconds between first transmission of an SDU on this ASE, to the end of
                         *   the last attempt to receive that SDU by the peer device. Range: 0x0005 to 0x0FA0 */
    uint16 frameDuration; /*! frame duration value */
    uint32 presentationDelay; /*! Presentation delay for this ASE in [microseconds] Range: 0x00000000 to 0xFFFFFFFF  */
} AseQosconfigType;

/*! @brief This structure is sent to the Profile/Application to indicate that a client has
 *         initiated the QoS Configure procedure.
 */
typedef struct
{
    BapServerPrim    type;  /*! BAP_SERVER_ASE_QOS_CONFIGURED_IND */
    ConnId        connectionId; /*! The connection on which ASEs are being QoS configured. */
    AseQosconfigType aseQosConfig;
} BapServerAseQosConfiguredInd;

typedef struct
{
    uint8   aseId; /*! The ASE id */
    uint8   metadataLength; /*! The Length of the metadata */
    uint8 * metadata; /*! Metadata will contain LTV fields defined in BAP or by higher layer specifications.
                          The server retains ownership of the memory pointed to by 'metadata';
                          this memory must not be freed by the profile/app */
} AseMetadataType;

typedef GattAscsServerEnableIndInfo BapServerEnableIndInfo;
typedef GattAscsAseResult BapServerAseResult;

/*!
 *  @brief This indication is sent to the profile/application to indicate that the client is
 *         enabling the specified ASEs and to provide any metadata that is applicable to
 *         those ASEs.
 */
typedef struct
{
    BapServerPrim    type;  /*! BAP_SERVER_ASE_ENABLED_IND */
    ConnId        connectionId; /*! The connection on which ASEs are being enabled. */
    /*! The number of ASES being Enabled */
    uint8        numAses;
    /*! The array of BapServerEnableIndInfos has 'numAses' elements.
     *  There is one BapServerEnableIndInfo per ASE
     *
     *  The memory allocated to store the BapServerEnableIndInfo structure is
     *  one contiguous block of memory that is large enough to store the appropriate number of
     *  BapServerEnableIndInfos (i.e. the number specified by the numAses field).
     *  This means that, if numAses is greater than 1, then the memory allocated to store
     *  the BapServerAseEnabledInd is greater than sizeof(BapServerAseEnabledInd).
     */
    BapServerEnableIndInfo bapServerEnableIndInfo[1];
} BapServerAseEnabledInd;

typedef GattAscsServerUpdateMetadataInfo BapServerUpdateMetadataInfo;

/*! @brief This structure is sent to the Profile/Application to indicate that a client has
 *         initiated updating metadata of give ASE
 */
typedef struct
{
    BapServerPrim    type;  /*! BAP_SERVER_ASE_UPDATE_METADATA_IND */
    ConnId        connectionId; /*! The connection on which ASEs metadata to be updated */
    uint8        numAses;
    /*! There is one BapServerUpdateMetadataInfo per ASE
     *
     *  The memory allocated to store the BapServerUpdateMetadataInfo structure is
     *  one contiguous block of memory that is large enough to store the appropriate number of
     *  BapServerUpdateMetadataInfos (i.e. the number specified by the numAses field).
     *  This means that, if numAses is greater than 1, then the memory allocated to store
     *  the bapServerUpdateMetadataInfo is greater than sizeof(BapServerUpdateMetadataInfo).
     */
    BapServerUpdateMetadataInfo bapServerUpdateMetadataInfo[1];
} BapServerAseUpdateMetadataInd;


typedef struct
{
    uint24 cigSyncDelay;
    uint24 cisSyncDelay;
    uint24 transportLatencyMtoS;
    uint24 transportLatencyStoM;
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
}BapServerCisParam;

typedef struct
{
    BapServerPrim      type; /*! BAP_SERVER_CIS_ESTABLISHED_IND */
    uint8              cisId; /*! CIS ID for cis establishment */
    uint16             cisHandle; /*! CIS handle for cis establishment */
    BapServerCisParam  cisParams;
    ConnId             connectionId; /*! The connection for which cis established */
} BapServerCisEstablishedInd;

typedef struct
{
    BapServerPrim  type; /*! BAP_SERVER_CIS_DISCONNECTED_IND */
    uint16         cisHandle; /*! CIS handle to disconnect */
    uint8          reason; /*! reason for CIS disconnection, ref Core Spec [Vol 1] Part F, Controller Error Codes */
} BapServerCisDisconnectedInd;

typedef struct
{
    BapServerPrim  type;   /*! BAP_SERVER_CIS_DISCONNECTED_CFM */
    uint16         cisHandle; /*! CIS handle */
    bapStatus      status; /*! status in response to CIS disconnection, ref Core Spec [Vol 1] Part F, Controller Error Codes */
} BapServerCisDisconnectedCfm;

/*! @brief This structure is sent to the Profile/Application to indicate that a client is ready
 *         to receive data for given ASE
 */
typedef struct
{
    BapServerPrim    type; /*! BAP_SERVER_ASE_RECEIVER_START_READY_IND */
    ConnId           connectionId; /*! The connection for which ASE receiver stop is ready */
    uint8            aseId; /*! The ASE id for which receiver start is ready*/
} BapServerAseReceiverStartReadyInd;

/*! @brief This structure is sent to the Profile/Application to indicate that a client
*          is disabling given ASE
 */
typedef struct
{
    BapServerPrim    type; /*! BAP_SERVER_ASE_DISABLED_IND */
    ConnId           connectionId; /*! The connection for which ASE is being disabled */
    uint8            aseId; /*! The ASE id which is being disabled */
} BapServerAseDisabledInd;

/*! @brief This structure is sent to the Profile/Application to indicate that a client is
 *          not ready to receive data for given ASE
 */
typedef struct
{
    BapServerPrim    type; /*! BAP_SERVER_ASE_RECEIVER_STOP_READY_IND */
    ConnId           connectionId; /*! The connection for which ASE receiver start is not ready */
    uint8            aseId; /*! The ASE id for which receiver stop is ready*/
} BapServerAseReceiverStopReadyInd;

typedef struct
{
    BapServerPrim    type; /*! BAP_SERVER_ASE_RELEASED_IND */
    ConnId           connectionId; /*! The connection for which ASE being released */
    uint8            aseId; /*! The ASE id which is being released */
} BapServerAseReleasedInd;

typedef struct
{
    BapServerPrim   type;  /*! BAP_SERVER_BASS_SCANNING_STATE_IND */
    ConnId          cid; /*! Connection identifier */
    bool            clientScanningState; /*! Scanning state True or False*/
} BapServerBassScanningStateInd;

typedef GattBassServerReceiveState BapServerBassReceiveState;

typedef struct
{
    ConnId cid;
    uint8 paSync;
    CsrBtTypedAddr advertiserAddress;
    uint32 broadcastId;
    uint8 sourceAdvSid;
    uint16 paInterval;
    uint8 numSubGroups;
    GattBassServerSubGroupsData *subGroupsData;
} BapServerBassAddSourceType;

/*! @brief Request from the client to add a source.
 */
typedef struct
{
    BapServerPrim                    type; /*! BAP_SERVER_BASS_ADD_SOURCE_IND */
    BapServerBassAddSourceType       source; /*! Source identifier parameters for add Source */
} BapServerBassAddSourceInd;

typedef struct
{
    ConnId cid;
    uint8 sourceId;
    uint8 paSyncState;
    uint16 paInterval;
    uint8 numSubGroups;
    GattBassServerSubGroupsData *subGroupsData;
} BapServerBassModifySourceType;

/*! @brief Request from the client to modify a source.
 */
typedef struct
{
    BapServerPrim                       type; /*! BAP_SERVER_BASS_MODIFY_SOURCE_IND */
    BapServerBassModifySourceType       source; /*! Source identifier parameters for modify */
} BapServerBassModifySourceInd;

typedef struct
{
    ConnId cid;
    uint8  sourceId;
    uint8 *broadcastCode;
} BapServerBroadcastCodeType;

/*! @brief Broadcast code sent by a client to attempt to decrypt or encrypt  BIS.
 */
typedef struct
{
    BapServerPrim                      type; /*! BAP_SERVER_BASS_BROADCAST_CODE_IND */
    BapServerBroadcastCodeType         code; /*! Broadcast code of the BIG */
} BapServerBassBroadcastCodeInd;

typedef struct
{
    ConnId cid;
    uint8 sourceId;
} BapServerRemoveSourceType;

/*! @brief Request from the client to remove a source.
 */
typedef struct
{
    BapServerPrim                      type; /*! BAP_SERVER_BASS_REMOVE_SOURCE_IND */
    BapServerRemoveSourceType          source; /*! Source identifier parameters for remove */
} BapServerBassRemoveSourceInd;


/*! \brief Data structure for the individual data items */
typedef struct
{
    uint16 size;
    const uint8 * data;
}leAdvDataItem;


typedef GattAscsClientConfig  BapAscsConfig;

typedef GattPacsServerConfigType BapPacsConfig;

typedef GattBassServerConfig BapBassConfig;

typedef uint8 BapPacCodecIdType;

#define BAP_SERVER_PAC_CODEC_ID_UNKNOWN             (BapPacCodecIdType)0x00
#define BAP_SERVER_PAC_LC3_CODEC_ID                 (BapPacCodecIdType)0x06   /* LC3 Codec id */
#define BAP_SERVER_PAC_VENDOR_CODEC_ID              (BapPacCodecIdType)0xFF    /* Vendor Codec id */


typedef uint8 PacsDirectionType;
#define PACS_DIRECTION_AUDIO_SINK        (PacsDirectionType)0x01 /*! Server is capable of receiving Audio data */
#define PACS_DIRECTION_AUDIO_SOURCE      (PacsDirectionType)0x02 /*! Server is capable of transmitting Audio data */

typedef uint8 PacAudioContextType;
#define PACS_SUPPORTED_AUDIO_CONTEXTS    (PacAudioContextType)0x01
#define PACS_AVAILABLE_AUDIO_CONTEXTS    (PacAudioContextType)0x02

typedef uint8 BapIsoDataType;
#define BAP_ISO_UNICAST                 (BapIsoDataType)0x01
#define BAP_ISO_BROADCAST               (BapIsoDataType)0x02

typedef uint16 PacsRecordHandle;

typedef GattPacsServerRecordType BapServerPacsRecord;
typedef GattPacsServerVSPacRecord BapServerVSPacsRecord;

typedef struct
{
    /*! The coding format value as defined in the Bluetooth Assigned Numbers.
     *
     *  A value of 0xFF specifies a Vendor Specific coding format.*/
    uint8 codingFormat;
    /*! For Vendor Specific coding formats this field specifies the Company ID as defined in Bluetooth Assigned Numbers.
     *
     *  For coding formats listed in the Bluetooth Assigned Numbers this field shall be 0x0000. */
    uint16 companyId;
    /*! For Vendor Specific coding formats this field specifies the Vendor specific Codec Id.
     *
     *  For coding formats listed in the Bluetooth Assigned Numbers this field shall be 0x0000. */
    uint16 vendorSpecificCodecId;
} AseCodecId;

typedef GattAscsServerConfigureCodecInfo BapServerAseCodecInfo;

typedef GattAscsServerConfigureQosInfo BapServerAseQosInfo;

typedef struct
{
    uint16                   isoHandle;          /*!< CIS or BIS handle */
    uint8                    dataPathDirection;  /*!< Direction of the audio datapath */
    uint8                    dataPathId;         /*! < Datapath id for HCI or Vendor specific Identifier*/
    uint32                   controllerDelay;    /*!<  Controller Delay based on direction */
    uint8                    codecId[BAP_CODEC_ID_SIZE];            /*!<  Codec Identfier */
    uint8                    codecConfigLength;  /*!< Codec Configuration parameters length*/
    uint8                    *codecConfigParams;  /*!< Codec Configuration parameters */
}  BapServerSetupDataPathReq;

typedef struct
{
    BapServerPrim           type;  /*! BAP_SERVER_SETUP_DATA_PATH_CFM */
    bapStatus               status; /*! Data path creation status , ref Core Spec [Vol 1] Part F, Controller Error Codes */
    uint16                  isoHandle;  /*!< CIS or BIS handle */
} BapServerSetupDataPathCfm;

typedef struct
{
    BapServerPrim           type;  /*! BAP_SERVER_REMOVE_DATA_PATH_CFM */
    bapStatus               status; /*! Data path removal status , ref Core Spec [Vol 1] Part F, Controller Error Codes */
    uint16                  isoHandle;  /*!< CIS or BIS handle */
}  BapServerRemoveDataPathCfm;

typedef struct
{
    uint32        transportLatencyBig;/* Max time to tx SDUs of all BISes */
    uint16        maxPdu;              /* Maximum size of an PDU */
    uint16        isoInterval;         /* ISO interval */
    uint8         phy;                  /* PHY used */
    uint8         nse;                  /* Number of sub events */
    uint8         bn;                   /* Burst number */
    uint8         pto;                  /* Pre transmission offset */
    uint8         irc;                  /* Repeated count of retransmission */
} BapSereverBigParam;

typedef struct
{
    BapServerPrim           type;   /*! BAP_SERVER_ISOC_BIG_CREATE_SYNC_CFM */
    bapStatus               status; /*! BIG Create Sync status , ref Core Spec [Vol 1] Part F, Controller Error Codes */
    BapSereverBigParam      bigParams;    /*! Confirmed Big Parameters */
    uint8                   bigHandle;    /*! Host identifier of BIG */
    uint8                   numBis;       /*! Number of BISes synchronized */
    uint16                  *bisHandles;  /*! Connection handle of BISes */
}  BapServerIsocBigCreateSyncCfm;

typedef struct
{
    BapServerPrim          type;    /*! BAP_SERVER_BIGINFO_ADV_REPORT_IND */
    uint16                 syncHandle;   /*! Sync handle of the PA */
    uint8                  numBis;       /*! Number of BISes in BIG */
    uint8                  nse;           /*! Number of sub events */
    uint16                 isoInterval;  /*! iso interval */
    uint8                  bn;            /*! Burst number */
    uint8                  pto;           /*! Pre transmission offset */
    uint8                  irc;           /*! repeated count of retransmission */
    uint16                 maxPdu;       /*! Maximum size of an PDU */
    uint32                 sduInterval;  /*! Interval of Periodic SDUs*/
    uint16                 maxSdu;       /*! Maximum size of an SDU */
    uint8                  phy;           /*! PHY for transmission */
    uint8                  framing;       /*! framed or unframed data */
    uint8                  encryption;    /*! data encryption status */
}  BapServerBigInfoAdvReportInd;

typedef struct
{
    BapServerPrim           type;   /*! BAP_SERVER_ISOC_BIG_TERMINATE_SYNC_IND */
    bapStatus               status; /*! BIG terminate Sync status , ref Core Spec [Vol 1] Part F, Controller Error Codes */
    uint8                   bigHandle;  /* Host identifier of BIG */
}  BapServerIsocBigTerminateSyncInd;


typedef struct
{
    BapServerPrim           type; /*! BAP_SERVER_ISOC_BIG_TERMINATE_SYNC_IND */
    bapStatus               status; /*! BIG terminate Sync status , ref Core Spec [Vol 1] Part F, Controller Error Codes */
    uint8                   bigHandle; /* Host identifier of BIG */
} BapServerIsocBigTerminateSyncCfm;

typedef struct
{
    GattAscsTargetLatency       targetLatency;
    PacsSamplingFrequencyType   samplingFreq;
    PacsFrameDurationType       frameDuration;
    uint32                      sinkLc3PdMin;
    uint32                      sourceLc3PdMin;
    uint32                      sinkVsAptxPdMin;
    uint32                      sourceVsAptxPdMin;
} BapServerCodecPdMin;

typedef struct
{
    uint16 lowMaxTransportLatancy; /* Supported Max tranport latency value for low target latency */
    uint16 balanceMaxTransportLatancy; /* Supported Max tranport latency value for Balance and reliability target latency */
    uint16 highMaxTransportLatancy; /* Supported Max tranport latency value for high target latency */
    uint8 preferredRtn;  /* Preferred retransmission number value Supported by Unicast Server */
} BapServerQosParams;

typedef GattAscsServerReleasingAseInfo BapServerReleasingAseInfo;

typedef struct
{
    BapServerPrim          type; /*! BAP_SERVER_ASE_CODEC_CONFIGURED_IND */
    ConnId                 connectionId; /*! The connection identifier */
    BapServerConfigType    configType; /* Can be anyone from "BapServerConfigType" defined above */
    bool                   configChangeComplete; /* will be TRUE if all CCCD of serverProfile are written once */
}  BapServerConfigChangeInd;

typedef struct
{
    BapServerPrim          type; /*! BAP_SERVER_AVAILABLE_AUDIO_CONTEXT_READ_IND */
    ConnId                 connectionId; /*! The connection identifier */
}  BapServerAvailableAudioContextReadInd;

typedef struct
{
    uint8 aseDataLength;
    uint8* aseData;
}BapServerAseData;

#endif
