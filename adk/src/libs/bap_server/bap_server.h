/****************************************************************************
Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.


FILE NAME
    bap_server_lib.h
    
DESCRIPTION
    Header file for the Basic Audio Profile library.
*****************************************************************************/
#ifndef BAP_SERVER_H__
#define BAP_SERVER_H__

#include <message.h>
#include <library.h>
#include <bdaddr.h>
#include "gatt.h"
#include "service_handle.h"
#include "gatt_ascs_server.h"
#include "gatt_pacs_server.h"
#include "gatt_bass_server.h"

typedef connection_id_t ConnId;
/*!
    \brief BAP Profile handle type.
*/
typedef ServiceHandle bapProfileHandle;

/*!
    \brief BAP status code type.
*/
typedef uint16 bapStatus;

/*! { */
/*! Values for the BAP status code */
#define BAP_SERVER_STATUS_SUCCESS              (0x0000u)  /*!> Request was a success*/
#define BAP_SERVER_STATUS_IN_PROGRESS          (0x0001u)  /*!> Request in progress*/
#define BAP_SERVER_STATUS_INVALID_PARAMETER    (0x0002u)  /*!> Invalid parameter was supplied*/
#define BAP_SERVER_STATUS_NOT_ALLOWED          (0x0003u)  /*!> Request is not allowed*/
#define BAP_SERVER_STATUS_FAILED               (0x0004u)  /*!> Request has failed*/
#define BAP_SERVER_STATUS_BC_SOURCE_IN_SYNC    (0x0005u)  /*!> Request has failed, because the selected Broadcast Source is in sync with the PA and/or BIS/BIG*/
#define BAP_SERVER_STATUS_INVALID_SOURCE_ID    (0x0006u)  /*!> Request has failed, because an invalid source id was supplied*/
#define BAP_SERVER_STATUS_NO_EMPTY_BRS         (0x0007u)  /*!> Request has failed, because no source id was supplied and all the Broadcast Receive State Characteristics are full */
#define BAP_SERVER_STATUS_BRS_NOT_CHANGED      (0x0008u)  /*!> BRS contents are not changed and hence NTFs are not sent over GATT*/


#define BAP_SERVER_DATA_PATH_ID_RAW_STREAM_ENDPOINTS_ONLY  0x06
#define BAP_SERVER_DATA_PATH_DIRECTION_HOST_TO_CONTROLLER  0x00
#define BAP_SERVER_DATA_PATH_DIRECTION_CONTROLLER_TO_HOST  0x01

#define BAP_CODEC_ID_SIZE                          ((uint8)0x05)

typedef uint8 BapServerRole;
#define BAP_SERVER_UNICAST_ROLE                    (BapServerRole)0x01
#define BAP_SERVER_BROADCAST_ROLE                  (BapServerRole)0x02

typedef uint8 AseDirectionType;
#define ASE_DIRECTION_UNINITIALISED     (AseDirectionType)0x00
#define ASE_DIRECTION_AUDIO_SINK        (AseDirectionType)0x01
/*! Describes an ASE on which audio data travels from the client to the server */
#define ASE_DIRECTION_AUDIO_SOURCE      (AseDirectionType)0x02
/*! Describes an ASE on which audio data travels from the server to the client */

/*! \brief LE Audio context types as defined in  BAPS_Assigned_Numbers_v4 */
typedef uint16 AudioContextType;
#define AUDIO_CONTEXT_TYPE_UNKNOWN              (AudioContextType)0x0000
#define AUDIO_CONTEXT_TYPE_UNSPECIFIED          (AudioContextType)0x0001
#define AUDIO_CONTEXT_TYPE_COVERSATIONAL        (AudioContextType)0x0002
#define AUDIO_CONTEXT_TYPE_MEDIA                (AudioContextType)0x0004
#define AUDIO_CONTEXT_TYPE_GAME                 (AudioContextType)0x0008
#define AUDIO_CONTEXT_TYPE_INSTRUCTIONAL        (AudioContextType)0x0010
#define AUDIO_CONTEXT_TYPE_VOICE_ASSISTANT      (AudioContextType)0x0020
#define AUDIO_CONTEXT_TYPE_LIVE                 (AudioContextType)0x0040
#define AUDIO_CONTEXT_TYPE_SOUND_EFFECTS        (AudioContextType)0x0080
#define AUDIO_CONTEXT_TYPE_NOTIFICATIONS        (AudioContextType)0x0100
#define AUDIO_CONTEXT_TYPE_RINGTONE             (AudioContextType)0x0200
#define AUDIO_CONTEXT_TYPE_ALERTS               (AudioContextType)0x0400
#define AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM      (AudioContextType)0x0800


typedef uint32 AudioLocationType;
#define AUDIO_LOCATION_MONO                    (AudioLocationType)0x00000000
#define AUDIO_LOCATION_FRONT_LEFT              (AudioLocationType)0x00000001
#define AUDIO_LOCATION_FRONT_RIGHT             (AudioLocationType)0x00000002
#define AUDIO_LOCATION_FRONT_CENTER            (AudioLocationType)0x00000004
#define AUDIO_LOCATION_LOW_FREQUENCY_EFFECT1   (AudioLocationType)0x00000008
#define AUDIO_LOCATION_BACK_LEFT               (AudioLocationType)0x00000010
#define AUDIO_LOCATION_BACK_RIGHT              (AudioLocationType)0x00000020
#define AUDIO_LOCATION_FRONT_LEFT_OF_CENTER    (AudioLocationType)0x00000040
#define AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER   (AudioLocationType)0x00000080
#define AUDIO_LOCATION_BACK_CENTER             (AudioLocationType)0x00000100
#define AUDIO_LOCATION_LOW_FREQUENCY_EFFECT2   (AudioLocationType)0x00000200
#define AUDIO_LOCATION_SIDE_LEFT               (AudioLocationType)0x00000400
#define AUDIO_LOCATION_SIDE_RIGHT              (AudioLocationType)0x00000800
#define AUDIO_LOCATION_TOP_FRONT_LEFT          (AudioLocationType)0x00001000
#define AUDIO_LOCATION_TOP_FRONT_RIGHT         (AudioLocationType)0x00002000
#define AUDIO_LOCATION_TOP_FRONT_CENTER        (AudioLocationType)0x00004000
#define AUDIO_LOCATION_TOP_CENTER              (AudioLocationType)0x00008000
#define AUDIO_LOCATION_TOP_BACK_LEFT           (AudioLocationType)0x00010000
#define AUDIO_LOCATION_TOP_BACK_RIGHT          (AudioLocationType)0x00020000
#define AUDIO_LOCATION_TOP_SIDE_LEFT           (AudioLocationType)0x00040000
#define AUDIO_LOCATION_TOP_SIDE_RIGHT          (AudioLocationType)0x00080000
#define AUDIO_LOCATION_TOP_SIDE_CENTER         (AudioLocationType)0x00100000
#define AUDIO_LOCATION_BOTTOM_FRONT_CENTER     (AudioLocationType)0x00200000
#define AUDIO_LOCATION_BOTTOM_FRONT_LEFT       (AudioLocationType)0x00400000
#define AUDIO_LOCATION_BOTTOM_FRONT_RIGHT      (AudioLocationType)0x00800000
#define AUDIO_LOCATION_FRONT_LEFT_WIDER        (AudioLocationType)0x01000000
#define AUDIO_LOCATION_FRONT_RIGHT_WIDER       (AudioLocationType)0x02000000
#define AUDIO_LOCATION_SURROUND_LEFT           (AudioLocationType)0x04000000
#define AUDIO_LOCATION_SURROUND_RIGHT          (AudioLocationType)0x08000000

typedef uint8 AseTargetLatency;
#define TARGET_LATENCY_TARGET_LOWER                    (AseTargetLatency)0x01
#define TARGET_LATENCY_TARGET_BALANCE_AND_RELIABLE     (AseTargetLatency)0x02
#define TARGET_LATENCY_TARGET_HIGHER_RELIABILITY       (AseTargetLatency)0x03


typedef uint8 AseTargetPhy;
#define TARGET_PHY_LE_1M_PHY                    (AseTargetPhy)0x01
#define TARGET_PHY_LE_2M_PHY                    (AseTargetPhy)0x02
#define TARGET_PHY_LE_CODEC_PHY                 (AseTargetPhy)0x03

typedef uint8 AseFraming;
#define FRAMING_UNFRAMED_ISOAL_PDU              (AseFraming)0x00
#define FRAMING_FRAMED_ISOAL_PDU                (AseFraming)0x01

typedef uint8 BapPacCodecIdType;
#define BAP_SERVER_PAC_CODEC_ID_UNKNOWN       (BapPacCodecIdType)0x00
#define BAP_SERVER_PAC_LC3_CODEC_ID           (BapPacCodecIdType)0x06   /* LC3 Codec id */
#define BAP_SERVER_PAC_VENDOR_CODEC_ID        (BapPacCodecIdType)0xFF    /* Vendor Codec id */

typedef uint8 BapServerBassPaSyncType;
#define BAP_SERVER_BASS_DONT_SYNC_TO_PA         (AseFraming)0x00
#define BAP_SERVER_BASS_SYNC_TO_PA              (AseFraming)0x01

/*! @brief The set of messages an application task can receive from the BAP Server library.
 */
typedef uint16 BapServerPrim;

#define BAP_SERVER_ASE_CODEC_CONFIGURED_IND     (BAP_SERVER_MESSAGE_BASE)
#define BAP_SERVER_ASE_CODEC_CONFIGURED_CFM     (BAP_SERVER_MESSAGE_BASE + 0x0001u)
#define BAP_SERVER_ASE_QOS_CONFIGURED_IND       (BAP_SERVER_MESSAGE_BASE + 0x0002u)
#define BAP_SERVER_ASE_ENABLED_IND              (BAP_SERVER_MESSAGE_BASE + 0x0003u)
#define BAP_SERVER_CIS_ESTABLISHED_IND          (BAP_SERVER_MESSAGE_BASE + 0x0004u)
#define BAP_SERVER_CIS_DISCONNECTED_IND         (BAP_SERVER_MESSAGE_BASE + 0x0005u)
#define BAP_SERVER_CIS_DISCONNECTED_CFM         (BAP_SERVER_MESSAGE_BASE + 0x0006u)
#define BAP_SERVER_ASE_RECEIVER_START_READY_IND (BAP_SERVER_MESSAGE_BASE + 0x0007u)
#define BAP_SERVER_ASE_DISABLED_IND             (BAP_SERVER_MESSAGE_BASE + 0x0008u)
#define BAP_SERVER_ASE_RECEIVER_STOP_READY_IND  (BAP_SERVER_MESSAGE_BASE + 0x0009u)
#define BAP_SERVER_ASE_UPDATE_METADATA_IND      (BAP_SERVER_MESSAGE_BASE + 0x000Au)
#define BAP_SERVER_ASE_RELEASED_IND             (BAP_SERVER_MESSAGE_BASE + 0x000Bu)
#define BAP_SERVER_SETUP_DATA_PATH_CFM          (BAP_SERVER_MESSAGE_BASE + 0x000Cu)
#define BAP_SERVER_REMOVE_DATA_PATH_CFM         (BAP_SERVER_MESSAGE_BASE + 0x000Du)
#define BAP_SERVER_BASS_SCANNING_STATE_IND      (BAP_SERVER_MESSAGE_BASE + 0x000Eu)
#define BAP_SERVER_BASS_ADD_SOURCE_IND          (BAP_SERVER_MESSAGE_BASE + 0x000Fu)
#define BAP_SERVER_BASS_MODIFY_SOURCE_IND       (BAP_SERVER_MESSAGE_BASE + 0x0010u)
#define BAP_SERVER_BASS_BROADCAST_CODE_IND      (BAP_SERVER_MESSAGE_BASE + 0x0011u)
#define BAP_SERVER_BASS_REMOVE_SOURCE_IND       (BAP_SERVER_MESSAGE_BASE + 0x0012u)
#define BAP_SERVER_ISOC_BIG_CREATE_SYNC_CFM     (BAP_SERVER_MESSAGE_BASE + 0x0013u)
#define BAP_SERVER_ISOC_BIG_TERMINATE_SYNC_IND  (BAP_SERVER_MESSAGE_BASE + 0x0014u)
#define BAP_SERVER_BIGINFO_ADV_REPORT_IND       (BAP_SERVER_MESSAGE_BASE + 0x0015u)

typedef GattAscsAseResultValue bapServerResponse;

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
typedef GattAscsServerUpdateMetadataInd  BapServerAseUpdateMetadataReq;

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

/*! @brief This structure is sent to the Profile/Application to indicate that a client has
 *         initiated updating metadata of given ASE
 */
typedef struct
{
    BapServerPrim    type;  /*! BAP_SERVER_ASE_UPDATE_METADATA_IND */
    ConnId        connectionId; /*! The connection on which ASEs metadata is being updated */
    AseMetadataType  aseMetadata;
} BapServerAseUpdateMetadataInd;

typedef struct
{
    uint32 cigSyncDelay;
    uint32 cisSyncDelay;
    uint32 transportLatencyMtoS;
    uint32 transportLatencyStoM;
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
} BapServerCisEstablishedInd;

typedef struct
{
    BapServerPrim  type; /*! BAP_SERVER_CIS_DISCONNECTED_IND */
    uint16         cisHandle; /*! CIS handle to disconnect */
    uint8          reason; /*! reason for CIS disconnection */
} BapServerCisDisconnectedInd;

typedef struct
{
    BapServerPrim  type;   /*! BAP_SERVER_CIS_DISCONNECTED_CFM */
    uint16         cisHandle; /*! CIS handle */
    bapStatus      status; /*! status in response to CIS disconnection */
} BapServerCisDisconnectedCfm;

/*! @brief This structure is sent to the Profile/Application to indicate that a client is ready
 *         to receive data for given ASE
 */
typedef struct
{
    BapServerPrim    type; /*! BAP_SERVER_ASE_RECEIVER_START_READY_IND */
    ConnId        connectionId; /*! The connection for which ASE receiver stop is ready */
    uint8            aseId; /*! The ASE id for which receiver start is ready*/
} BapServerAseReceiverStartReadyInd;

/*! @brief This structure is sent to the Profile/Application to indicate that a client
*          is disabling given ASE
 */
typedef struct
{
    BapServerPrim    type; /*! BAP_SERVER_ASE_DISABLED_IND */
    ConnId        connectionId; /*! The connection for which ASE is being disabled */
    uint8            aseId; /*! The ASE id which is being disabled */
} BapServerAseDisabledInd;

/*! @brief This structure is sent to the Profile/Application to indicate that a client is
 *          not ready to receive data for given ASE
 */
typedef struct
{
    BapServerPrim    type; /*! BAP_SERVER_ASE_RECEIVER_STOP_READY_IND */
    ConnId        connectionId; /*! The connection for which ASE receiver start is not ready */
    uint8            aseId; /*! The ASE id for which receiver stop is ready*/
} BapServerAseReceiverStopReadyInd;

typedef struct
{
    BapServerPrim    type; /*! BAP_SERVER_ASE_RELEASED_IND */
    ConnId        connectionId; /*! The connection for which ASE being released */
    uint8            aseId; /*! The ASE id which is being released */
} BapServerAseReleasedInd;

/*! @brief Request from the client to start or to stop scanning on behalf of the server.
 */
typedef struct
{
    BapServerPrim   type;  /*! BAP_SERVER_BASS_SCANNING_STATE_IND */
    ConnId          cid;
    bool            clientScanningState;
} BapServerBassScanningStateInd;


typedef GattBassServerReceiveState BapServerBassReceiveState;

typedef struct
{
    ConnId cid;
    uint8 paSync;
    typed_bdaddr advertiserAddress;
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
    BapServerPrim                    type;         /*! BAP_SERVER_BASS_ADD_SOURCE_IND */
    BapServerBassAddSourceType       source;
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
    BapServerPrim                       type;         /*! BAP_SERVER_BASS_MODIFY_SOURCE_IND */
    BapServerBassModifySourceType       source;
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
    BapServerPrim                      type;         /*! BAP_SERVER_BASS_BROADCAST_CODE_IND */
    BapServerBroadcastCodeType         code;
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
    BapServerPrim                      type;         /*! BAP_SERVER_BASS_REMOVE_SOURCE_IND */
    BapServerRemoveSourceType          source;
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
    BapServerPrim           type;
    bapStatus               status;
    uint16                  isoHandle;          /*!< CIS or BIS handle */
} BapServerSetupDataPathCfm;

typedef struct
{
    BapServerPrim           type;
    bapStatus               status;
    uint16                  isoHandle;          /*!< CIS or BIS handle */
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
    BapServerPrim           type;
    bapStatus               status;
    BapSereverBigParam      bigParams;    /* Confirmed Big Parameters */
    uint8                   bigHandle;    /* Host identifier of BIG */
    uint8                   numBis;       /* Number of BISes synchronized */
    uint16                  *bisHandles;  /* Connection handle of BISes */
}  BapServerIsocBigCreateSyncCfm;


typedef struct
{
    BapServerPrim          type;
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
    BapServerPrim           type;
    bapStatus               status;
    uint8                   bigHandle;    /* Host identifier of BIG */
}  BapServerIsocBigTerminateSyncInd;


typedef struct
{
    BapServerPrim           type;
    bapStatus               status;
    uint8                   bigHandle;    /* Host identifier of BIG */
} BapServerIsocBigTerminateSyncCfm;
/*!
    @brief This API is used to Get the maximum number of supported ASEs by BAP

    ASCS server
    @param void 

    @return Maximum number of supported ASEs
*/
uint8 BapServerUnicastGetMaxSupportedAses(void);

/*!
    @brief This API is used to Initialise the Basic Audio Profile Library for
    unicast Role.

    @param theAppTask The AppTask that will receive the messages sent from this BAP library.
    @param numAses The number of ASEs to allow on BAP Unicast server instance. 
    The value shall not exceed the value returned by BapServerUnicastGetMaxSupportedAses().

    @return The Profile handle for this BAP profile instance. On Success it 
            non-zero value otherwise it returns BAP_INVALID_HANDLE
*/
bapProfileHandle BapServerUnicastInit(Task appTask,
                                      uint8 numAses,
                                     const BapServerHandleRange *pacsHandles,
                                     const BapServerHandleRange *ascsHandles);


/*! \brief This API is used in response to BAP_SERVER_ASE_RECEIVER_START_READY_IND
    to signal that source ASE is ready to transmitting audio.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @param numAses the number of ases in the response
    @param aseIds pointer of aseIds
 */
void BapServerUnicastAseReceiveStartReadyResponse(bapProfileHandle profileHandle,
                                                  ConnId connectionId,
                                                  uint8 numAses,
                                                  const uint8 *aseIds);

/*! \brief This API is used to Release the ASE after CIS is disconnected.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @param numAses the number of ases in the response
    @param aseIds pointer of aseIds
    @param cacheCodecEnable Whether cacheCodecConfiguration is enabled or not
 */
void BapServerUnicastAseReleased(bapProfileHandle profileHandle,
                                 ConnId connectionId,
                                 uint8 numAses,
                                 const uint8 *aseIds,
                                 bool cacheCodecEnable);

/*!
    @brief This API is used to initiate the ASE Configure Codec procedure
    BAP_SERVER_ASE_CODEC_CONFIGURED_CFM message will be sent to the registered app Task.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param aseCodecInfo    A pointer to a BapServerAseConfigCodecReq structure
                           that describes the codec parameters for ASEs to be taken to the codec configured state
                           and the codec configuration data for each of those ASEs.
                           The Profile/Application is responsible for all memory associated
                           with BapServerAseConfigCodecReq, e.g. the Application/Profile
                           must free any codec_configuration data referenced in the
                           BapServerAseConfigCodecReq
    @return TRUE if success, FALSE otherwise
*/
bool BapServerUnicastAseConfigureCodecReq(bapProfileHandle profileHandle, 
                                          const BapServerAseConfigCodecReq *aseCodecInfo);

/*! \brief This API is used when sink ASE is ready to receive audio.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @param numAses the number of ases in the response
    @param aseIds pointer of aseIds

    @return TRUE if success, FALSE otherwise
 */
bool BapServerUnicastAseReceiveStartReadyReq(bapProfileHandle profileHandle,
                                             ConnId connectionId,
                                             uint8 numAses,
                                             const uint8 *aseIds);

/*! \brief This API is used to initiate Server autonomous disable ASEs procedure
    BAP_SERVER_ASE_DISABLED_IND message will be sent to the registered app Task.

    @param profileHandle The Profile handle of this BAP Server instance.
    \param connectionId connection id of the LE audio source
    \param numAses the number of ases in the request
    \param aseIds pointer of aseIds
    @param cisLoss This flag should be set to true when the CIS is lost. When
    the CIS is lost and the ASE is in the Streaming state or the Disabling
    state, the server shall immediately transition that ASE to the QoS
    Configured state

    @return TRUE if success, FALSE otherwise
 */
bool BapServerUnicastAseDisableReq(bapProfileHandle profileHandle,
                                   ConnId connectionId,
                                   uint8 numAses,
                                   const uint8 *aseIds,
                                   bool cisLoss);

/*! \brief This API is used to initiate Server autonomous release ASEs procedure
    BAP_SERVER_ASE_RELEASED_IND message will be sent to the registered app Task.

    @param profileHandle The Profile handle of this BAP Server instance.
    \param connectionId connection id of the LE audio source
    \param numAses the number of ases in the request
    @param aseIds pointer of aseIds

    @return TRUE if success, FALSE otherwise
 */
bool BapServerUnicastAseReleaseReq(bapProfileHandle profileHandle, 
                                   ConnId connectionId,
                                   uint8 numAses,
                                   const uint8 *aseIds);


/*!
    @brief This API is used to retrieve the Codec Configuration of an ASE.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId  The id of the connection that contains the ASE from
                         which the codec configuration is to be retrieved.
    @param aseId  The id of the ASE from which the codec configuration is to be retrieved.
*/
BapServerAseCodecInfo * BapServerUnicastReadAseCodecConfiguration(bapProfileHandle profileHandle,
                                                                  ConnId connectionId,
                                                                  uint8 aseId);

/*!
    @brief This API is used to retrieve the QOS Configuration of an ASE.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId  The id of the connection that contains the ASE from
                         which the QOS configuration is to be retrieved.
    @param aseId  The id of the ASE from which the QOS configuration is to be retrieved.
*/
BapServerAseQosInfo * BapServerUnicastReadAseQosConfiguration(bapProfileHandle profileHandle,
                                                              ConnId connectionId,
                                                              uint8 aseId);

/*!
    @brief This API is used to retrieve the ASE direction of given ASE ID.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId  The id of the connection that contains the ASE from which the QOS configuration is to be retrieved.
    @param aseId  The id of the ASE from which the QOS configuration is to be retrieved.
*/
AseDirectionType  BapServerUnicastReadAseDirection(bapProfileHandle profileHandle,
                                                   ConnId connectionId,
                                                   uint8 aseId);

/*! \brief This API is used to Add configuration for a paired peer device, 
    identified by its connectionId

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @config config Pointer to Connected Clinet characteristics configurations

    @return bapStatus status of the Add Configuration operation.

 */
bapStatus BapServerUnicastAddAscsConfig(bapProfileHandle profileHandle,
                                       ConnId connectionId,
                                       const BapAscsConfig * config);

/*! \brief This API is used to remove the configuration for a peer device,
    identified by its connectionId

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId connection id of the LE audio source

    @return BapAscsConfig  Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
 */
BapAscsConfig * BapServerUnicastRemoveAscsConfig(bapProfileHandle profileHandle,
                                                 ConnId connectionId);


/*! \brief This API is used to create an ISO data path between Host and
    the controller for the CIS or BIS identfied by isoHandle. 
    BAP_SERVER_SETUP_DATA_PATH_CFM message will be sent to the registered app Task.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param isoDataType ISO data for this datapath.
    @param dataPathParameters Iso data path parameters
 */
void BapServerSetupIsoDataPathReq(bapProfileHandle profileHandle,
                                  BapIsoDataType isoDataType,
                                  const BapServerSetupDataPathReq *dataPathParameters);

/*! \brief This API is used remove an ISO data path between Host and
    the controller for the CIS or BIS identfied by isoHandle. 
    BAP_SERVER_REMOVE_DATA_PATH_CFM message will be sent to the registered app Task.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param isoHandle     Iso handle of the CIS or BIS.
    @param isoDataType ISO data for this datapath.
    @param dataPathDirection Direction of the audio datapath.
 */
void BapServerRemoveIsoDataPathReq(bapProfileHandle profileHandle,
                                   uint16 isoHandle,
                                   BapIsoDataType isoDataType,
                                   uint8 dataPathDirection);

/*! \brief This API is use disconnect the CIS identfied by cisHandle. 
    BAP_SERVER_CIS_DISCONNECTED_CFM message will be sent to the registered app Task.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param cisHandle     cisHandle handle of the CIS.
    @param disconnectReason Reason for disconnection.

 */
void BapServerUnicastCisDisconnectReq(bapProfileHandle profileHandle,
                                      uint16 cisHandle,
                                      uint8 disconnectReason);

/*! \brief This API is for sending ASE Enable operation response.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @param numAses the number of ases in the response of bapServerAseResult
    @param bapServerAseResult pointer of BapServerAseResult has 'numAses' elements

    @return TRUE if success, FALSE otherwise
 */
bool BapServerUnicastAseEnableRsp(bapProfileHandle profileHandle,
                                  ConnId connectionId,
                                  uint8 numAses,
                                  const BapServerAseResult *bapServerAseResult);

/* PACS API */

/*!
    @brief This API is used to add a PAC record for SINK/SOURCE. 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacsDirection Direction specifies SINK/SOURCE PAC record.
    @param pacRecord A pointer to gatt_pacs_server_record_t of SINK/SOURCE PAC

    @return pac_record_handle if successful, errors mentioned in pacs_record_error_t otherwise.

*/
PacsRecordHandle BapServerAddPacRecord(bapProfileHandle profileHandle, 
                                       PacsDirectionType pacsDirection,
                                       const BapServerPacsRecord * pacRecord);

/*!
    @brief This API is used to get a registered PAC record corresponds to the pacRecordHandle. 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacRecordHandle Returned as part of BapServerAddPacRecord() for PAC record

    @return A const pointer to BapServerPacsRecord if success, NULL otherwise

*/
const BapServerPacsRecord * BapServerGetPacRecord(bapProfileHandle profileHandle,
                                                  uint16 pacRecordHandle);

/*!
    @brief This API is used to remove a registered PAC record corresponds to the pacRecordHandle. 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacRecordHandle Returned as part of BapServerAddPacRecord() for PAC record

    @return TRUE if success, FALSE otherwise

*/
bool BapServerRemovePacRecord(bapProfileHandle profileHandle,
                              PacsRecordHandle pacRecordHandle);

/*!
    @brief This API is used to add Audio Location of SINK/SOURCE PAC

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacsDirection Direction specifies SINK/SOURCE Audio location.
    @param audioLocations Device wide bitmap of supported Audio Location values
           for all PAC records where the server supports reception/transmission
           of audio data

    @return TRUE if success, FALSE otherwise
*/
bool BapServerAddPacAudioLocation(bapProfileHandle profileHandle,
                                  PacsDirectionType pacsDirection,
                                  AudioLocationType audioLocations);

/*!
    @brief This API is used to get Audio Location set for SINK or SOURCE configuration. 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param direction Direction specifies the SINK or SOURCE audio locations.

    @return Audio Location if success, 0 otherwise

*/
AudioLocationType BapServerGetPacAudioLocation(bapProfileHandle profileHandle,
                                               PacsDirectionType pacsDirection);

/*!
    @brief This API is used to remove Audio Location of SINK/SOURCE PAC

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacsDirection Direction specifies SINK/SOURCE Audio location.
    @param audioLocations Device wide bitmap of supported Audio Location values
           for all PAC records where the server supports reception/transmission
           of audio data

    @return TRUE if success, FALSE otherwise

*/
bool BapServerRemovePacAudioLocation(bapProfileHandle profileHandle,
                                     PacsDirectionType pacsDirection,
                                     AudioLocationType audioLocations);

/*!
    @brief This API is used to add Supported Audio Contexts for SINK and SOURCE contexts.
           The API is also used to add Available Audio Contexts for SINK and SOURCE contexts
           at any point from Supported Audio Contexts list.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacsDirection Direction specifies the SINK or SOURCE audio contexts.
    @param audioContext Bitmask of audio data Context Type values for transmission/reception.
    @param contexts Supported or Available audio context defined by pacs_server_audio_context_t

    @return TRUE if success, FALSE otherwise
*/
bool BapServerAddPacAudioContexts(bapProfileHandle profileHandle,
                                  PacsDirectionType pacsDirection,
                                  AudioContextType audioContext,
                                  PacAudioContextType contexts);

/*!
    @brief This API is used to get Available Audio Contexts for SINK or SOURCE configuration 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacsDirection Direction specifies the SINK or SOURCE supported audio contexts.

    @return Available Audio Contexts if success, 0 otherwise

*/
AudioContextType BapServerGetPacAvailableContexts(bapProfileHandle profileHandle,
                                                  PacsDirectionType pacsDirection);

/*!
    @brief This API is used to remove Supported or Available Audio Contexts for SINK or SOURCE
           configuration 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacsDirection Direction specifies SINK/SOURCE Audio location.
    @param audioContext Bitmask of audio data Context Type values for transmission/reception.
    @param contexts Supported or Available audio context defined by pacs_server_audio_context_t


    @return TRUE if success, FALSE otherwise

*/
bool BapServerRemovePacAudioContexts(bapProfileHandle profileHandle,
                                     PacsDirectionType pacsDirection,
                                     AudioContextType audioContext,
                                     PacAudioContextType contexts);

/*! \brief This API is used to Add configuration for a paired peer device, 
    identified by its connectionId

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @config config Pointer to Connected Clinet characteristics configurations

    @return bapStatus status of the Add Configuration operation.

 */
 
bapStatus BapServerAddPacsConfig(bapProfileHandle profileHandle,
                                ConnId connectionId,
                                BapPacsConfig * config);

/*! \brief Remove the configuration for a peer device, identified by its connectionId

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId connection id of the LE audio source

    @return BapPacsConfig  Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
 */
BapPacsConfig * BapServerRemovePacsConfig(bapProfileHandle profileHandle,
                                          ConnId connectionId);

/*! \brief This API is used to Add configuration for a paired peer device, 
    identified by its connectionId

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @config config Pointer to Connected Clinet characteristics configurations

    @return bapStatus status of the Add Configuration operation.

 */
bapStatus BapServerAddBassConfig(bapProfileHandle profileHandle,
                                ConnId connectionId,
                                BapBassConfig * config);

/*! \brief Remove the configuration for a peer device, identified by its connectionId

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId connection id of the LE audio source

    @return BapBassConfig  Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
 */
BapBassConfig * BapServerRemoveBassConfig(bapProfileHandle profileHandle,
                                          ConnId connectionId);

/* Advertisement API */

/*! \brief Request to get number of Advertising elements

    @param ServerRole  Role of the Server for Advertising

    @return uint16  Number of Advertising data items.
 */
uint16 BapServerGetNumberOfAdvertisingItems( BapServerRole ServerRole);

/*! \brief Request to get Advertising data

    @param ServerRole  Role of the Server for Advertising

    @return leAdvDataItem  Pointer to LE Advertising data.
 */
leAdvDataItem *BapServerGetAdvertisingData( BapServerRole ServerRole);

/*! \brief Request to free the Advertising data

    @param ServerRole  Role of the Server for Advertising
 */
void BapServerReleaseAdvertisingItems( BapServerRole ServerRole);

/*! \brief Request to get Advertising data size

    \param server_role  Role of the Server for Advertising

    \return The size of the advertising data.
 */
unsigned BapServerGetAdvertisingDataSize(BapServerRole server_role);


/*!
    @brief Initialises the Basic Audio Profile Library for Broadcast Role.

    @param theAppTask The AppTask that will receive the messages sent from this BAP library.
    @param numberBroadcastSources The number of Broadcast sources to allow on BAP broadcast server instance
    @param pacsHandles pointer to Start handle and End handle of PACS service
    @param bassHandles pointer to Start handle and End handle of BASS service

    @return The Profile handle for this BAP profile instance.

*/
bapProfileHandle BapServerBroadcastInit(Task appTask,
                                        uint8 numberBroadcastSources,
                                        const BapServerHandleRange *pacsHandles,
                                        const BapServerHandleRange *bassHandles);

/*!
    @brief Get the number and the values of the source IDs of all the Broadcast Receive State characteristics.

    @param profileHandle The BAP instance profile handle.
    @param sourceIdNum Pointer to the variable in which the library will put the number of Source IDs.

    @return uint8 * pointer to the list of values of the Source IDs. It's responsibility of the application
                    to free the memory associated to this pointer.
*/
uint8 * BapServerGetSourceIdsReq(bapProfileHandle profileHandle,
                                 uint16 *sourceIdNum);

/*!
    @brief Add a broadcast source.

    @param profileHandle The BAP instance profile handle.
    @param sourceId     ID of the specific broadcast source.
    @param sourceInfo   Pointer to the structure containing the info of the broadcast source to set
                         in the Receive State characteristic.
                         It is the application's responsibility to free the memory of this pointer.

    @return bapStatus Result of the operation.

*/
bapStatus  BapServerAddBroadcastSourceReq(bapProfileHandle profileHandle,
                                          uint8* sourceId,
                                          BapServerBassReceiveState *sourceInfo);


/*!
    @brief Get the info of a specific broadcast source.

    @param profileHandle The BAP instance profile handle.
    @param sourceId     ID of the specific broadcast source.
    @param state         Pointer to the structure containing all the info.
                         The gatt_bass_server_receive_state_t structure contains a pointer (metadata):
                         it is the application's responsibility to free the memory of the this pointer.

    @return gatt_bass_server_status_tbapStatus Result of the operation.

*/
bapStatus BapServerGetBroadcastReceiveStateReq(bapProfileHandle profileHandle,
                                               uint8 sourceId,
                                               BapServerBassReceiveState *state);

/*!
    @brief Modify a broadcast source.

    @param profileHandle The BAP instance profile handle.
    @param sourceId     ID of the specific broadcast source to modify.
    @param sourceInfo   Pointer to the structure containing the info of the broadcast source to modify
                         in the Receive State characteristic.
                         It is the application responsibility to free the memory of this pointer.

    @return bapStatus Result of the operation.

*/
bapStatus  BapServerModifyBroadcastSourceReq(bapProfileHandle profileHandle,
                                             uint8 sourceId,
                                             BapServerBassReceiveState *sourceInfo);

/*!
    @brief Remove a broadcast source.

    @param profileHandle The BAP instance profile handle.
    @param sourceId     ID of the specific broadcast source to remove.

    @return bapStatus Result of the operation.

*/
bapStatus  BapServerRemoveBroadcastSourceReq(bapProfileHandle profileHandle,
                                             uint8 sourceId);

/*!
    @brief Get the Broadcast Code of a specific broadcast source.

    @param profileHandle The BAP instance profile handle.
    @param sourceId     ID of the specific broadcast source.

    @return Broadcast Code for that Broadcast Source.

*/
uint8* BapServerGetBroadcastCodeReq(bapProfileHandle profileHandle,
                                    uint8 sourceId);

/*!
    @brief Set the Broadcast Code of a specific broadcast source.

    @param profileHandle  The BAP instance profile handle.
    @param sourceId      ID of the specific broadcast source.
    @param broadcastCode The pointer to broadcast code to set.

    @return bapStatus Result of the operation.

*/
bapStatus BapServerSetBroadcastCodeReq(bapProfileHandle profileHandle,
                                       uint8 sourceId,
                                       uint8 *broadcastCode);


/*! \brief Query if the server has any client connections.
    @param srvcHndl The GATT service instance handle
    .
    @param profileHandle  The BAP instance profile handle.
    @return TRUE if any clients are connected to the server.
*/
bool BapServerIsAnyClientConnected(bapProfileHandle profileHandle);

/*!
    @brief This API is called to synchronize to  BIG described in the periodic advertising
          train specified by the SyncHandle parameter.
    BAP_SERVER_ISOC_BIG_CREATE_SYNC_CFM message will be sent to the registered app Task.

    @param appTask The AppTask that will receive the BIS messages sent from this BAP library.
                   This can be set to CSR_SCHED_QID_INVALID if appTask is same used in BapServerBroadcastInit()
    @param profileHandle  The BAP instance profile handle.
    @param syncHandle:         Identifies the associated periodic advertising train of the BIG
    @param bigSyncTimeout     Maximum permitted time between successful receptions of BIS PDUs
    @param bigHandle:          Identifier of the BIG
    @param mse                  maximum number of subevents that a Controller should use to receive
                                data payloads in each interval for a BIS.
    @param encryption:          Encryption mode of the BISes( 0- unencrypted 1-encrypted)
    @param broadcastCode:      Encryption key(size 16) for encrypting payloads of all BISes.
    @param numBis              Total number of BISes indices specified in the BIS[i] parameter
    @param bis                  List of indices corresponding to BIS(es) in the synchronized BIG

*/
void BapServerBroadcastBigCreateSyncReq(Task appTask,
                                        bapProfileHandle profileHandle,
                                        uint16 syncHandle,
                                        uint16 bigSyncTimeout,
                                        uint8 bigHandle,
                                        uint8 mse,
                                        uint8 encryption,
                                        uint8 *broadcastCode,
                                        uint8 numBis,
                                        uint8 *bis);

/*!
    @brief This API is called to terminate or cancel the syncronised BIS identified by BigHandle.
    BAP_SERVER_ISOC_BIG_TERMINATE_SYNC_IND message will be sent to the registered app Task.

    @param profileHandle  The BAP instance profile handle.
    @param bigHandle:          Identifier of the BIG

*/
void BapServerBroadcastBigTerminateSyncReq(bapProfileHandle profileHandle,
                                           uint8 bigHandle);

/*! \brief This API is used to find values of specific Types from the Audio Stream
     Endpoint LTV stucture.

    @param ltvData  Pointer to LTV data.
    @param ltvDataLength length of ltvData pointer
    @param type Type field of LTV stucture
    @param value Pointer to Value filed to be returned from LTV stucture
    @param valueLength length of value pointer

    @return TRUE if success, FALSE otherwise

 */
bool BapServerLtvUtilitiesFindLtvValue(uint8 * ltvData, uint8 ltvDataLength,
                                              uint8 type, uint8 * value,
                                              uint8 valueLength);

/*! \brief This API is used to get Sampling frequency value from LTV structure.

    @param config  Pointer to LTV data.
    @param configLength Length of config

    @return uint32  sampling frequency value.

 */
uint32 BapServerLtvUtilitiesGetSampleRate(uint8 * config, uint8 configLength);

/*! \brief This API is used to get Frame duration value from LTV structure.

    @param config  Pointer to LTV data.
    @param configLength Length of config

    @return uint16  Frame duration value.

 */
uint16 BapServerLtvUtilitiesGetFrameDuration(uint8 * config, uint8 configLength);

/*! \brief This API is used to get Streaming audio context from LTV structure.

    @param metadata  Pointer to LTV data.
    @param metadataLength Length of metadata

    @return AudioContextType  Streaming Audio context.

 */
AudioContextType BapServerLtvUtilitiesGetStreamingAudioContext(uint8 * metadata,
                                                         uint8 metadataLength);

/*! \brief This API is used to get Audio channel allocation value from LTV structure.

    @param config  Pointer to LTV data.
    @param configLength Length of config

    @return uint32  Audio channel allocation

 */
uint32 BapServerLtvUtilitiesGetAudioChannelAllocation(uint8 * config,
                                                      uint8 configLength);

/*! \brief This API is used to get Number of Octets per Frame value from LTV structure.

    @param config  Pointer to LTV data.
    @param configLength Length of config

    @return uint16  Number of Octets per Frame

 */
uint16 BapServerLtvUtilitiesGetOctetsPerCodecFrame(uint8 * config,
                                                    uint8 configLength);

/*! \brief This API is used to get Codec frame blocks per SDU value from LTV structure.

    @param config  Pointer to LTV data.
    @param configLength Length of config

    @return uint8  Number of Codec frame blocks per SDU value

 */
uint8 BapServerLtvUtilitiesGetCodecFrameBlocksPerSdu(uint8 * config,
                                                        uint8 configLength);

/*! \brief This API is used validate values range from LTV structure.

    @param metadata  Pointer to LTV data.
    @param metadataLength Length of metadata
    @param invalidMetadataType Pointer to Type value of metadata

    @return bapServerResponse  Result of validation

 */
bapServerResponse BapServerValidateMetadataLtvs(uint8* metadata,
                                                   uint8 metadataLength,
                                                   uint8* invalidMetadataType);

/*! \brief This API is used get validate Streaming Audio context from PACS available
     Audio context.

    @param profileHandle  The BAP instance profile handle.
    @param connectionId connection id of the LE audio source
    @param aseId ASE Id of Sink or Source ASE
    @param metadata  Pointer to LTV data.
    @param metadataLength Length of metadata

    @return TRUE if success, FALSE otherwise

 */
bool BapServerValidateStreamingContext(bapProfileHandle profileHandle, 
                                      ConnId connectionId,uint8 aseId,
                                      uint8 * metadata, uint8 *metadataLength);


/*! \brief This API is used to get offset of the LTV type from the ltvData.

    @param ltvData  Pointer to LTV data.
    @param ltvDataLength Length of config
    @param type Type field of LTV
    @param offset offset of the LTV type

    @return bool  True if LTV type present else false

 */
bool BapServerLtvUtilitiesFindLtvOffset(uint8 * ltvData, uint8 ltvDataLength,
                                                    uint8 type, uint8 * offset);

#endif

