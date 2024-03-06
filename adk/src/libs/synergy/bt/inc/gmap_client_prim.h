/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************

FILE NAME
    gmap_client_prim.h
    
DESCRIPTION
    Header file for the telephony and media audio profile (GMAP) primitives.
*/

#ifndef GMAP_CLIENT_PRIM_H
#define GMAP_CLIENT_PRIM_H

#include "csr_bt_gatt_prim.h"
#include "service_handle.h"
#include "gatt_gmas_client.h"

#include "cap_client_prim.h"
#include "cap_client_lib.h"

#define GMAP_CLIENT_PROFILE_PRIM             (SYNERGY_EVENT_BASE + GMAP_CLIENT_PRIM)


/*!
    \brief Profile handle type.
*/
typedef ServiceHandle GmapClientProfileHandle;

/*!
    @brief GMAP Client handles.

*/
typedef struct
{
    GattGmasClientDeviceData *gmasClientHandle;
}GmapClientHandles;

/*!
    @brief Initialisation parameters.
*/
typedef struct
{
    GmapClientConnectionId cid;
}GmapClientInitData;

/*!
   @GMAP Client role values
*/
#define GMAP_ROLE_UNICAST_GAME_GATEWAY                       (0x01)
#define GMAP_ROLE_UNICAST_GAME_TERMINAL                      (0x02)
#define GMAP_ROLE_BROADCAST_GAME_SENDER                      (0x04)
#define GMAP_ROLE_BROADCAST_GAME_RECEIVER                    (0x08)

/*!
    \brief GMAP Client status code type.
*/
typedef uint16                           GmapClientStatus;
#define GMAP_CLIENT_STATUS_SUCCESS                      ((GmapClientStatus) 0x0000)  /*!> Request was a success*/
#define GMAP_CLIENT_STATUS_NOT_SUPPORTED                ((GmapClientStatus) 0x0001) /*!>  Not supported by remote device*/
#define GMAP_CLIENT_STATUS_COMMAND_INCOMPLETE           ((GmapClientStatus) 0x0002)  /*!> Command requested could not be completed*/
#define GMAP_CLIENT_STATUS_DISCOVERY_ERR                ((GmapClientStatus) 0x0003)  /*!> Error in discovery of one of the services*/
#define GMAP_CLIENT_STATUS_FAILED                       ((GmapClientStatus) 0x0004)  /*!> Request has failed*/
#define GMAP_CLIENT_STATUS_IN_PROGRESS                  ((GmapClientStatus) 0x0005)  /*!> Request in progress*/
#define GMAP_CLIENT_STATUS_INVALID_PARAMETER            ((GmapClientStatus) 0x0006)  /*!> Invalid parameter was supplied */
#define GMAP_CLIENT_STATUS_ACTIVE_CONN_PRESENT          ((GmapClientStatus) 0x0007)  /*!> Active connections are still present */
#define GMAP_CLIENT_STATUS_SUCCESS_GMAS_SRVC_NOT_FOUND  ((GmapClientStatus) 0x0008)  /*!> Request was success but GMAS Srvc was not found */


/*! @brief Messages a gmap client task may receive from the profile library.
 */

typedef uint16                                   GmapClientMessageId;
#define GMAP_CLIENT_INIT_CFM                                ((GmapClientMessageId) 0x0001)
#define GMAP_CLIENT_DESTROY_CFM                             ((GmapClientMessageId) 0x0002)
#define GMAP_CLIENT_READ_ROLE_CFM                           ((GmapClientMessageId) 0x0003)
#define GMAP_CLIENT_READ_UNICAST_FEATURES_CFM               ((GmapClientMessageId) 0x0004)
#define GMAP_CLIENT_READ_BROADCAST_FEATURES_CFM             ((GmapClientMessageId) 0x0005)

typedef uint8 GmapClientServices;
#define GMAP_CLIENT_GMAP_SUPPORTED             ((GmapClientServices) 0x01)   /* SIG GMAP */ 
#define GMAP_CLIENT_QGMAP_SUPPORTED            ((GmapClientServices) 0x02)   /* Qualcomm GMAP */


#define GMAP_CLIENT_REGISTER_CAP_CFM                            ((GmapClientMessageId) 0x0006)
#define GMAP_CLIENT_UNICAST_CONNECT_CFM                         ((GmapClientMessageId) 0x0007)
#define GMAP_CLIENT_UNICAST_START_STREAM_CFM                    ((GmapClientMessageId) 0x0008)
#define GMAP_CLIENT_UNICAST_START_STREAM_IND                    ((GmapClientMessageId) 0x0009)
#define GMAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM                    ((GmapClientMessageId) 0x000A)
#define GMAP_CLIENT_UNICAST_UPDATE_METADATA_IND                 ((GmapClientMessageId) 0x000B)
#define GMAP_CLIENT_UNICAST_STOP_STREAM_CFM                     ((GmapClientMessageId) 0x000C)
#define GMAP_CLIENT_UNICAST_DISCONNECT_CFM                      ((GmapClientMessageId) 0x000D)
#define GMAP_CLIENT_ABS_VOLUME_CFM                              ((GmapClientMessageId) 0x000E)
#define GMAP_CLIENT_MUTE_CFM                                    ((GmapClientMessageId) 0x000F)
#define GMAP_CLIENT_VOLUME_STATE_IND                            ((GmapClientMessageId) 0x0010)
#define GMAP_CLIENT_BROADCAST_SRC_INIT_CFM                      ((GmapClientMessageId) 0x0011)
#define GMAP_CLIENT_BROADCAST_SRC_CONFIG_CFM                    ((GmapClientMessageId) 0x0012)
#define GMAP_CLIENT_BROADCAST_SRC_START_STREAM_CFM              ((GmapClientMessageId) 0x0013)
#define GMAP_CLIENT_BROADCAST_SRC_UPDATE_STREAM_CFM             ((GmapClientMessageId) 0x0014)
#define GMAP_CLIENT_BROADCAST_SRC_STOP_STREAM_CFM               ((GmapClientMessageId) 0x0015)
#define GMAP_CLIENT_BROADCAST_SRC_REMOVE_STREAM_CFM             ((GmapClientMessageId) 0x0016)
#define GMAP_CLIENT_BROADCAST_SRC_DEINIT_CFM                    ((GmapClientMessageId) 0x0017)
#define GMAP_CLIENT_BROADCAST_ASST_START_SRC_SCAN_CFM           ((GmapClientMessageId) 0x0018)
#define GMAP_CLIENT_BROADCAST_ASST_SRC_REPORT_IND               ((GmapClientMessageId) 0x0019)
#define GMAP_CLIENT_BROADCAST_ASST_STOP_SRC_SCAN_CFM            ((GmapClientMessageId) 0x001A)
#define GMAP_CLIENT_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM    ((GmapClientMessageId) 0x001B)
#define GMAP_CLIENT_BROADCAST_ASST_READ_BRS_IND                 ((GmapClientMessageId) 0x001C)
#define GMAP_CLIENT_BROADCAST_ASST_READ_BRS_CFM                 ((GmapClientMessageId) 0x001D)
#define GMAP_CLIENT_BROADCAST_ASST_BRS_IND                      ((GmapClientMessageId) 0x001E)
#define GMAP_CLIENT_BROADCAST_ASST_START_SYNC_TO_SRC_CFM        ((GmapClientMessageId) 0x001F)
#define GMAP_CLIENT_BROADCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM    ((GmapClientMessageId) 0x0020)
#define GMAP_CLIENT_BROADCAST_ASST_CANCEL_SYNC_TO_SRC_CFM       ((GmapClientMessageId) 0x0021)
#define GMAP_CLIENT_BROADCAST_ASST_ADD_SRC_CFM                  ((GmapClientMessageId) 0x0022)
#define GMAP_CLIENT_BROADCAST_ASST_MODIFY_SRC_CFM               ((GmapClientMessageId) 0x0023)
#define GMAP_CLIENT_BROADCAST_ASST_REMOVE_SRC_CFM               ((GmapClientMessageId) 0x0024)
#define GMAP_CLIENT_BROADCAST_ASST_SET_CODE_IND                 ((GmapClientMessageId) 0x0025)
#define GMAP_CLIENT_BROADCAST_SRC_BIG_TEST_CONFIG_CFM           ((GmapClientMessageId) 0x0026)
#define GMAP_CLIENT_SET_PARAMS_CFM                              ((GmapClientMessageId) 0x0027)
#define GMAP_CLIENT_DEREGISTER_CAP_CFM                          ((GmapClientMessageId) 0x0028)


#define GMAP_CLIENT_CCID_LTV_LEN 0x04
#define GMAP_CLIENT_CCID_LIST_LENGTH  0x03
#define GMAP_CLIENT_CCID_LIST_TYPE  0x05

typedef CapClientBigConfigParam                  GmapClientBigConfigParam;
typedef CapClientBcastInfo                       GmapClientBcastInfo;
typedef CapClientQhsConfig                       GmapClientQhsConfig;
typedef CapClientQhsBigConfig                    GmapClientQhsBigConfig;
typedef CapClientBcastSrcAdvParams               GmapClientBcastSrcAdvParams;
typedef CapClientSubgroupInfo                    GmapClientSubgroupInfo;

typedef uint16 GmapClientContext;
#define GMAP_CLIENT_CONTEXT_TYPE_UNSPECIFIED          ((GmapClientContext)CAP_CLIENT_CONTEXT_TYPE_UNSPECIFIED)     /* Unspecified. Matches any audio content. */
#define GMAP_CLIENT_CONTEXT_TYPE_GAME                 ((GmapClientContext)CAP_CLIENT_CONTEXT_TYPE_GAME)            /* Audio associated with gaming */

typedef uint8 GmapClientTargetLatency;
#define GMAP_CLIENT_TARGET_LOWER_LATENCY                     ((CapClientTargetLatency)(CAP_CLIENT_TARGET_LOWER_LATENCY))
#define GMAP_CLIENT_TARGET_BALANCE_LATENCY_AND_RELIABILITY   ((CapClientTargetLatency)(CAP_CLIENT_TARGET_BALANCE_LATENCY_AND_RELIABILITY))
#define GMAP_CLIENT_TARGET_HIGH_RELIABILITY                  ((CapClientTargetLatency)(CAP_CLIENT_TARGET_HIGH_RELIABILITY))


typedef uint32 GmapClientStreamCapability;
#define GMAP_CLIENT_STREAM_CAPABILITY_UNKNOWN         ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN)
#define GMAP_CLIENT_STREAM_CAPABILITY_8_1             ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_8_1)
#define GMAP_CLIENT_STREAM_CAPABILITY_8_2             ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_8_2)
#define GMAP_CLIENT_STREAM_CAPABILITY_16_1            ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_16_1)
#define GMAP_CLIENT_STREAM_CAPABILITY_16_2            ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_16_2)
#define GMAP_CLIENT_STREAM_CAPABILITY_24_1            ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_24_1)
#define GMAP_CLIENT_STREAM_CAPABILITY_24_2            ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_24_2)
#define GMAP_CLIENT_STREAM_CAPABILITY_32_1            ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_32_1)
#define GMAP_CLIENT_STREAM_CAPABILITY_32_2            ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_32_2)
#define GMAP_CLIENT_STREAM_CAPABILITY_441_1           ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_441_1)
#define GMAP_CLIENT_STREAM_CAPABILITY_441_2           ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_441_2)
#define GMAP_CLIENT_STREAM_CAPABILITY_48_1            ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_48_1)
#define GMAP_CLIENT_STREAM_CAPABILITY_48_2            ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_48_2)
#define GMAP_CLIENT_STREAM_CAPABILITY_48_3            ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_48_3)
#define GMAP_CLIENT_STREAM_CAPABILITY_48_4            ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_48_4)
#define GMAP_CLIENT_STREAM_CAPABILITY_48_5            ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_48_5)
#define GMAP_CLIENT_STREAM_CAPABILITY_48_6            ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_48_6)
#define GMAP_CLIENT_STREAM_CAPABILITY_LC3_EPC         ((GmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_LC3_EPC)
#define GMAP_CLIENT_CODEC_ID_MASK                     ((GmapClientStreamCapability)CAP_CLIENT_CODEC_ID_MASK)

typedef uint8 GmapClientCigConfigMode;
#define GMAP_CLIENT_CIG_CONFIG_MODE_DEFAULT           ((GmapClientCigConfigMode) CAP_CLIENT_CIG_CONFIG_MODE_DEFAULT)
#define GMAP_CLIENT_CIG_CONFIG_MODE_QHS               ((GmapClientCigConfigMode) CAP_CLIENT_CIG_CONFIG_MODE_QHS)
#define GMAP_CLIENT_MODE_JOINT_STEREO                 ((GmapClientCigConfigMode) CAP_CLIENT_MODE_JOINT_STEREO)

typedef uint16 GmapClientBroadcastSrcType;
#define GMAP_CLIENT_BROADCAST_SRC_COLLOCATED       ((GmapClientBroadcastSrcType)CAP_CLIENT_BCAST_SRC_COLLOCATED)
#define GMAP_CLIENT_BROADCAST_SRC_NON_COLLOCATED   ((GmapClientBroadcastSrcType)CAP_CLIENT_BCAST_SRC_NON_COLLOCATED)
#define GMAP_CLIENT_BROADCAST_SRC_ALL              ((GmapClientBroadcastSrcType)CAP_CLIENT_BCAST_SRC_ALL)

typedef uint8 GmapClientBigConfigMode;
#define GMAP_CLIENT_BIG_CONFIG_MODE_DEFAULT           ((GmapClientBigConfigMode )CAP_CLIENT_BIG_CONFIG_MODE_DEFAULT)
#define GMAP_CLIENT_BIG_CONFIG_MODE_QHS               ((GmapClientBigConfigMode )CAP_CLIENT_BIG_CONFIG_MODE_QHS)
#define GMAP_CLIENT_BIG_CONFIG_MODE_JOINT_STEREO      ((GmapClientBigConfigMode )CAP_CLIENT_BIG_CONFIG_MODE_JOINT_STEREO)

typedef uint8  GmapClientParamsType;
#define GMAP_CLIENT_PARAMS_TYPE_NONE                 ((GmapClientParamsType)CAP_CLIENT_PARAM_TYPE_NONE)
#define GMAP_CLIENT_PARAMS_TYPE_UNICAST_CONNECT      ((GmapClientParamsType)CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT)
#define GMAP_CLIENT_PARAMS_TYPE_BROADCAST_CONFIG     ((GmapClientParamsType)CAP_CLIENT_PARAM_TYPE_BCAST_CONFIG)

/*!
    @brief Profile library message sent as a result of calling the GmapClientInitReq API.
*/
typedef struct
{
    GmapClientMessageId       id;
    GmapClientStatus          status;          /*! Status of the initialisation attempt*/
    GmapClientProfileHandle   prflHndl;        /*! GMAP Client profile handle */
    GmapClientServices        gmasServices;    /*! GMAS Services supported by remote */
} GmapClientInitCfm;

/*! @brief Contents of the GmapClientReadRoleCfm message that is sent by the library,
    as response to a reading of a role of remote.
*/
typedef struct
{
    GmapClientMessageId       id;
    GmapClientProfileHandle   prflHndl;         /*! GMAP Client profile handle*/
    GmapClientStatus          status;           /*! Status of the reading attempt */
    uint8                     role;             /*! Role Value */
} GmapClientReadRoleCfm;


/*! @brief Contents of the GmapClientReadUnicastFeaturesCfm message that is sent by the library,
    as response to a reading of a unicast features of remote.
*/
typedef struct
{
    GmapClientMessageId       id;
    GmapClientProfileHandle   prflHndl;         /*! GMAP Client profile handle*/
    GmapClientStatus          status;           /*! Status of the reading attempt */
    uint8                     unicastFeatures;  /*! Unicast Features Value */
} GmapClientReadUnicastFeaturesCfm;

/*! @brief Contents of the GmapClientReadBroadcastFeaturesCfm message that is sent by the library,
    as response to a reading of a broadcast features of remote.
*/
typedef struct
{
    GmapClientMessageId       id;
    GmapClientProfileHandle   prflHndl;             /*! GMAP Client profile handle*/
    GmapClientStatus          status;               /*! Status of the reading attempt */
    uint8                     broadcastFeatures;    /*! Broadcast Features Value */
} GmapClientReadBroadcastFeaturesCfm;

/*!
    @brief Profile library message sent as a result of calling the GmapClientDestroyReq API.

    This message will send at first with the value of status of GMAP_CLIENT_STATUS_IN_PROGRESS.
    Another GMAP_CLIENT_DESTROY_CFM message will be sent with the final status (success or fail),
*/
typedef struct
{
    GmapClientMessageId       id;
    GmapClientProfileHandle   prflHndl;     /*! GMAP Client profile handle*/
    GmapClientStatus          status;       /*! Status of the initialisation attempt*/
} GmapClientDestroyCfm;

typedef struct
{
    GmapClientMessageId         type;                 /*!< GMAP_CLIENT_REGISTER_CAP_CFM */
    ServiceHandle               groupId;              /*!< Group Id */
    CapClientResult             result;               /*!< CAP Status */
} GmapClientRegisterTaskCfm;

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
}GmapClientUnicastConnectParams;

typedef struct
{
    uint8                      rtn;                    /* Unicast Retransmission Number*/
    uint16                     sduSize;                /* Max SDU size */
    uint8                      maxCodecFramesPerSdu;
    GmapClientTargetLatency    targetLatency;
    uint16                     maxLatency;             /* Unicast Transport Latency */
    uint16                     phy;
    uint32                     sduInterval;            /* Sdu Interval */
    GmapClientStreamCapability subGroupConfig;
} GmapClientBroadcastConfigParams;


typedef struct
{
    GmapClientMessageId         type;
    CapClientResult             result;
    uint32                      profileHandle;       /*!< Broadcast Source Handle/ GroupId */
} GmapClientSetParamsCfm;

typedef struct
{
    GmapClientMessageId         type;                  /*!< GMAP_CLIENT_UNICAST_CONNECT_CFM */
    ServiceHandle               groupId;               /*!< Group Id */
    GmapClientContext           context;               /*!< Audio Context */
    CapClientResult             result;                /*!< CAP Status */
    uint8                       numOfMicsConfigured;   /*!< Mics Configured */
    uint8                       cigId;                 /*!< cig Id */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;         /*!< Individual Device Status */
} GmapClientUnicastConnectCfm;

typedef struct
{
    GmapClientMessageId         type;             /*!< GMAP_CLIENT_UNICAST_START_STREAM_IND */
    ServiceHandle               groupId;          /*!< Group Id */
    uint32                      cid;              /*!< BtConnID */ 
    CapClientResult             result;           /*!< CAP Status */
    CapClientAudioConfig        *audioConfig;
    CapClientCisHandles         *cishandles;      /*!< Unicast ISO handles */
    uint8                       cisCount;
    uint8                       cigId;
} GmapClientUnicastStartStreamInd;

typedef struct
{
    GmapClientMessageId         type;            /*!< GMAP_CLIENT_UNICAST_START_STREAM_CFM */
    ServiceHandle               groupId;         /*!< Group Id */
    CapClientResult             result;          /*!< CAP Status */
} GmapClientUnicastStartStreamCfm;

typedef struct
{
     GmapClientMessageId        type;                   /*!< GMAP_CLIENT_UNICAST_UPDATE_METADATA_IND */
     CapClientResult            result;                 /*!< CAP Status. */
     uint16                     streamingAudioContexts; /*!< Bitmask of Audio Context type values */
     ServiceHandle              groupId;                /*!< Group Id */
     uint32                     cid;
     bool                       isSink;
     uint8                      metadataLen;
     uint8                      *metadata;
}GmapClientUnicastUpdateMetadataInd;

typedef struct
{
    GmapClientMessageId         type;                  /*!< GMAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM */
    ServiceHandle               groupId;               /*!< CAP Handle */
    GmapClientContext           context;               /*!< Group Id */
    CapClientResult             result;                /*!< CAP Status */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;         /*!< Individual Device Status */
} GmapClientUnicastUpdateAudioCfm;

typedef struct
{
    GmapClientMessageId         type;            /*!< GMAP_CLIENT_UNICAST_STOP_STREAM_CFM */
    ServiceHandle               groupId;         /*!< Group Id */
    bool                        released;        /*!< If release Done */
    CapClientResult             result;          /*!< CAP Status */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;    /*!< Individual Device Status */
} GmapClientUnicastStopStreamCfm;

typedef struct
{
    GmapClientMessageId         type;                  /*!< GMAP_CLIENT_UNICAST_DISCONNECT_CFM */
    ServiceHandle               groupId;               /*!< CAP Handle */
    CapClientResult             result;                /*!< CAP Status */
} GmapClientUnicastDisconnectCfm;

typedef struct
{
    GmapClientMessageId         type;                 /*!< GMAP_CLIENT_DEREGISTER_CAP_CFM */
    ServiceHandle               groupId;              /*!< Group Id */
    CapClientResult             result;               /*!< CAP Status */
} GmapClientDeRegisterTaskCfm;

typedef struct
{
    GmapClientMessageId         type;
    ServiceHandle               groupId;         /*!< Group Id */
    CapClientResult             result;          /*!< CAP Status */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;   /*!< Individual Device Status */
} GmapClientVolumeCommonCfm;

typedef GmapClientVolumeCommonCfm                 GmapClientAbsVolumeCfm;
typedef GmapClientVolumeCommonCfm                 GmapClientMuteCfm;

typedef struct
{
    GmapClientMessageId         type;            /*!< GMAP_CLIENT_VOLUME_STATE_IND */
    ServiceHandle               groupId;         /*!< CAP handle */
    uint8                       volumeState;
    uint8                       mute;
    uint8                       changeCounter;
} GmapClientVolumeStateInd;

typedef struct
{
    GmapClientStreamCapability config;                  /*<! Setting which needs to be configured per BIS*/
    uint32                     audioLocation;
    uint8                      targetLatency;
    uint8                      lc3BlocksPerSdu;
}GmapClientBisInfo;

typedef struct
{
    GmapClientStreamCapability config;                  /*<! Setting which needs to be configured*/
    uint8                      targetLatency;           /*<! low latency or high reliability */
    uint8                      lc3BlocksPerSdu;
    GmapClientContext          useCase;                 /* Context type of Audio/Usecase */
    uint8                      metadataLen;             /* Total length of vendor metadata */
    uint8                      *metadata;                /* LTV format */
    uint8                      numBis;
    GmapClientBisInfo         *bisInfo;                /* The array of TmapClientBisInfo has 'numBis' elements.*/
}GmapClientBigSubGroup;

typedef struct
{
    GmapClientMessageId         type;                     /*!< GMAP_CLIENT_BROADCAST_SRC_INIT_CFM */
    CapClientResult             result;
    GmapClientProfileHandle     bcastSrcProfileHandle;    /*!< Broadcast Source Handle */
} GmapClientBroadcastSrcInitCfm;

typedef struct
{
    GmapClientMessageId         type;                     /*!< GMAP_CLIENT_BROADCAST_SRC_CONFIG_CFM */
    CapClientResult             result;
    GmapClientProfileHandle     bcastSrcProfileHandle;    /*!< Broadcast Source Handle */
} GmapClientBroadcastSrcConfigCfm;

typedef struct
{
    GmapClientMessageId         type;                     /*!<GMAP_CLIENT_BROADCAST_SRC_START_STREAM_CFM */
    CapClientResult             result;
    GmapClientProfileHandle     bcastSrcProfileHandle;    /* Broadcast Source Handle */
    uint8                       bigId;
    uint32                      bigSyncDelay;             /* BIG Sync delay */
    CapClientBigParam           *bigParameters;
    uint8                       numSubGroup;              /* Number of subgroups for a given broadcast Source Handle */
    CapClientBcastSubGroupInfo  *subGroupInfo;
}GmapClientBroadcastSrcStartStreamCfm;

typedef struct
{
    GmapClientMessageId         type;
    CapClientResult             result;                   /*!< CAP Status */
    GmapClientProfileHandle     bcastSrcProfileHandle;    /*!< Broadcast Source Handle */
} GmapClientBroadcastSrcCommonCfm;

typedef GmapClientBroadcastSrcCommonCfm GmapClientBroadcastSrcUpdateStreamCfm;
typedef GmapClientBroadcastSrcCommonCfm GmapClientBroadcastSrcStopStreamCfm;
typedef GmapClientBroadcastSrcCommonCfm GmapClientBroadcastSrcRemoveStreamCfm;
typedef GmapClientBroadcastSrcCommonCfm GmapClientBroadcastSrcDeinitCfm;

typedef struct
{
    uint32 cid;
    uint8  sourceId;
}GmapClientBroadcastSinkInfo;

typedef struct
{
    GmapClientMessageId    type;        /*!< GMAP_CLIENT_BROADCAST_ASST_START_SRC_SCAN_CFM */
    ServiceHandle          groupId;     /*!< CAP Handle */
    CapClientResult        result;      /*!< Result code. */
    uint16                 scanHandle;  /*!< scanHandle */
    uint8                  statusLen;
    CapClientDeviceStatus  *status;
}GmapClientBroadcastAsstStartSrcScanCfm;

typedef struct
{
    GmapClientMessageId         type;          /*!< GMAP_CLIENT_BROADCAST_ASST_SRC_REPORT_IND */
    uint32                      cid;           /*!< BAP Handle */
    TYPED_BD_ADDR_T             sourceAddrt;   /*! BT address of the Broadcast Source State */
    uint8                       advSid;        /*! Advertising SID */
    uint8                       advHandle;     /*! advHandle valid in case of collocated Broadcast Source. Else it will be 0 */
    bool                        collocated;    /*! Whether SRC is collocated(local) or not */
    uint32                      broadcastId;   /*!< Broadcast ID */
    /* Level 1 */
    uint8                       numSubgroup;
   /* Level 2 Sub Group and Level 3 */
    BapBigSubgroup              *subgroupInfo;
    uint8                       bigNameLen;
    char                        *bigName;
}GmapClientBroadcastAsstSrcReportInd;

typedef struct
{
    GmapClientMessageId    type;      /*!< GMAP_CLIENT_BROADCAST_ASST_STOP_SRC_SCAN_CFM */
    ServiceHandle          groupId;   /*!< CAP Handle */
    CapClientResult        result;    /*!< Result code. */
    uint8                  statusLen;
    CapClientDeviceStatus  *status;
}GmapClientBroadcastAsstStopSrcScanCfm;

typedef struct
{
    GmapClientMessageId    type;            /* GMAP_CLIENT_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM */           
    ServiceHandle          groupId;         /*!< CAP Handle */
    CapClientResult        result;          /*!< CAP Status */
    uint8                  statusLen;
    CapClientDeviceStatus  *status;
}GmapClientBroadcastAsstRegisterNotificationCfm;

typedef struct
{
    GmapClientMessageId         type;             /*!< GMAP_CLIENT_BROADCAST_ASST_READ_BRS_IND */
    ServiceHandle               groupId;          /*!< CAP Handle */
    CapClientResult             result;           /*!< Result code. */
    uint32                      cid;              /*!< BAP Handle */
    uint8                       sourceId;         /*! Source_id of the Broadcast Receive State characteristic */
    BD_ADDR_T                   sourceAddress;
    uint8                       advertiseAddType;
    uint8                       advSid;
    uint8                       paSyncState;
    uint8                       bigEncryption;
    uint32                      broadcastId;      /*!< Broadcast ID */
    uint8                       *badCode;
    uint8                       numSubGroups;
    GmapClientSubgroupInfo      *subGroupInfo;
}GmapClientBroadcastAsstReadBrsInd;

typedef struct
{
    GmapClientMessageId         type;             /*!< GMAP_CLIENT_BROADCAST_ASST_READ_BRS_CFM */
    ServiceHandle               groupId;          /*!< CAP Handle */
    CapClientResult             result;           /*!< Result code. */
    uint32                      cid;              /*!< Connection Id*/
}GmapClientBroadcastAsstReadBrsCfm;

typedef struct
{
    GmapClientMessageId         type;             /*!< GMAP_CLIENT_BROADCAST_ASST_BRS_IND */
    ServiceHandle               groupId;          /*!< CAP Handle */
    uint32                      cid;              /*!< BAP Handle */
    uint8                       sourceId;         /*! Source_id of the Broadcast Receive State characteristic */
    BD_ADDR_T                   sourceAddress;
    uint8                       advertiseAddType;
    uint8                       advSid;
    uint8                       paSyncState;
    uint8                       bigEncryption;
    uint32                      broadcastId;      /*!< Broadcast ID */
    uint8                       *badCode;
    uint8                       numSubGroups;
    GmapClientSubgroupInfo      *subGroupInfo;
}GmapClientBroadcastAsstBrsInd;

typedef struct
{
    GmapClientMessageId   type;          /*!< GMAP_CLIENT_BROADCAST_ASST_START_SYNC_TO_SRC_CFM */
    ServiceHandle         groupId;       /*!< CAP Handle */
    CapClientResult       result;        /*!< Result code. */
    uint16                syncHandle;    /*!< Sync handle of the PA */
    uint8                 advSid;
    TYPED_BD_ADDR_T       addrt;
    uint8                 advPhy;
    uint16                periodicAdvInterval;
    uint8                 advClockAccuracy;
}GmapClientBroadcastAsstStartSyncToSrcCfm;

typedef struct
{
    GmapClientMessageId   type;       /*!< GMAP_CLIENT_BROADCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM */
    ServiceHandle         groupId;    /*!< CAP Handle */
    CapClientResult       result;     /*!< Result code. */
    uint16                syncHandle;
}GmapClientBroadcastAsstTerminateSyncToSrcCfm;

typedef struct
{
    GmapClientMessageId   type;       /*!< GMAP_CLIENT_BROADCAST_ASST_CANCEL_SYNC_TO_SRC_CFM */
    ServiceHandle         groupId;    /*!< CAP Handle */
    CapClientResult       result;     /*!< Result code. */
}GmapClientBroadcastAsstCancelSyncToSrcCfm;

typedef struct
{
    GmapClientMessageId     type;            /* GMAP_CLIENT_BROADCAST_ASST_ADD_SRC_CFM  */           
    ServiceHandle           groupId;         /*!< CAP Handle */
    CapClientResult         result;          /*!< CAP Status */
    uint8                   statusLen;
    CapClientDeviceStatus*  status;
}GmapClientBroadcastAsstAddSrcCfm;

typedef struct
{
    GmapClientMessageId      type;            /* GMAP_CLIENT_BROADCAST_ASST_MODIFY_SRC_CFM  */           
    ServiceHandle            groupId;         /*!< CAP Handle */
    CapClientResult          result;          /*!< CAP Status */
    uint8                    statusLen;
    CapClientDeviceStatus*   status;
}GmapClientBroadcastAsstModifySrcCfm;

typedef struct
{
    GmapClientMessageId      type;            /* GMAP_CLIENT_BROADCAST_ASST_REMOVE_SRC_CFM  */           
    ServiceHandle            groupId;         /*!< CAP Handle */
    CapClientResult          result;          /*!< CAP Status */
    uint8                    statusLen;
    CapClientDeviceStatus*   status;
}GmapClientBroadcastAsstRemoveSrcCfm;

typedef struct
{
    GmapClientMessageId         type;      /*!< GMAP_CLIENT_BROADCAST_ASST_SET_CODE_IND */
    ServiceHandle               groupId;   /*!< CAP Handle */
    uint32                      cid;       /*!< BAP Handle */
    uint8                       sourceId;  /*! Source_id of the Broadcast Receiver State characteristic */
    uint8                       flags;  
}GmapClientBroadcastAsstSetCodeInd;

#endif /* GMAP_CLIENT_PRIM_H */

