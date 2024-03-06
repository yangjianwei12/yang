/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #60 $
******************************************************************************

FILE NAME
    tmap_client_prim.h
    
DESCRIPTION
    Header file for the telephony and media audio profile (TMAP) primitives.
*/

#ifndef TMAP_CLIENT_PRIM_H
#define TMAP_CLIENT_PRIM_H

#include "csr_bt_gatt_prim.h"
#include "service_handle.h"
#include "gatt_tmas_client.h"

#include "cap_client_prim.h"
#include "cap_client_lib.h"

#define TMAP_CLIENT_PROFILE_PRIM             (SYNERGY_EVENT_BASE + TMAP_CLIENT_PRIM)


/*!
    \brief Profile handle type.
*/
typedef ServiceHandle TmapClientProfileHandle;

/*!
    @brief TMAP Client handles.

*/
typedef struct
{
    GattTmasClientDeviceData *tmasClientHandle;
}TmapClientHandles;

/*!
    @brief Initialisation parameters.
*/
typedef struct
{
    TmapClientConnectionId cid;
}TmapClientInitData;

/*!
   @TMAP Client role values
*/
#define TMAP_ROLE_CALL_GATEWAY                0x0001
#define TMAP_ROLE_CALL_TERMINAL               0x0002
#define TMAP_ROLE_UNICAST_MEDIA_SENDER        0x0004
#define TMAP_ROLE_UNICAST_MEDIA_RECEIVER      0x0008
#define TMAP_ROLE_BROADCAST_MEDIA_SENDER      0x0010
#define TMAP_ROLE_BROADCAST_MEDIA_RECEIVER    0x0020

/*!
    \brief TMAP Client status code type.
*/
typedef uint16                           TmapClientStatus;

#define TMAP_CLIENT_STATUS_SUCCESS                      ((TmapClientStatus) 0x0000)  /*!> Request was a success*/
#define TMAP_CLIENT_STATUS_NOT_SUPPORTED                ((TmapClientStatus) 0x0001) /*!>  Not supported by remote device*/
#define TMAP_CLIENT_STATUS_COMMAND_INCOMPLETE           ((TmapClientStatus) 0x0002)  /*!> Command requested could not be completed*/
#define TMAP_CLIENT_STATUS_DISCOVERY_ERR                ((TmapClientStatus) 0x0003)  /*!> Error in discovery of one of the services*/
#define TMAP_CLIENT_STATUS_FAILED                       ((TmapClientStatus) 0x0004)  /*!> Request has failed*/
#define TMAP_CLIENT_STATUS_IN_PROGRESS                  ((TmapClientStatus) 0x0005)  /*!> Request in progress*/
#define TMAP_CLIENT_STATUS_INVALID_PARAMETER            ((TmapClientStatus) 0x0006)  /*!> Invalid parameter was supplied */
#define TMAP_CLIENT_STATUS_ACTIVE_CONN_PRESENT          ((TmapClientStatus) 0x0007)  /*!> Active connections are still present */
#define TMAP_CLIENT_STATUS_SUCCESS_TMAS_SRVC_NOT_FOUND  ((TmapClientStatus) 0x0008)  /*!> Request was success but TMAS Srvc was not found */


/*! @brief Messages a tmap client task may receive from the profile library.
 */

typedef uint16                                   TmapClientMessageId;

#define TMAP_CLIENT_INIT_CFM                                ((TmapClientMessageId) 0x0001)
#define TMAP_CLIENT_DESTROY_CFM                             ((TmapClientMessageId) 0x0002)
#define TMAP_CLIENT_TMAS_TERMINATE_CFM                      ((TmapClientMessageId) 0x0003)
#define TMAP_CLIENT_ROLE_CFM                                ((TmapClientMessageId) 0x0004)

#define TMAP_CLIENT_REGISTER_CAP_CFM                            ((TmapClientMessageId) 0x0005)
#define TMAP_CLIENT_UNICAST_CONNECT_CFM                         ((TmapClientMessageId) 0x0006)
#define TMAP_CLIENT_UNICAST_START_STREAM_CFM                    ((TmapClientMessageId) 0x0007)
#define TMAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM                    ((TmapClientMessageId) 0x0008)
#define TMAP_CLIENT_UNICAST_STOP_STREAM_CFM                     ((TmapClientMessageId) 0x0009)
#define TMAP_CLIENT_ABS_VOLUME_CFM                              ((TmapClientMessageId) 0x000A)
#define TMAP_CLIENT_MUTE_CFM                                    ((TmapClientMessageId) 0x000B)
#define TMAP_CLIENT_BROADCAST_SRC_INIT_CFM                      ((TmapClientMessageId) 0x000C)
#define TMAP_CLIENT_BROADCAST_SRC_CONFIG_CFM                    ((TmapClientMessageId) 0x000D)
#define TMAP_CLIENT_BROADCAST_SRC_START_STREAM_CFM              ((TmapClientMessageId) 0x000E)
#define TMAP_CLIENT_BROADCAST_SRC_UPDATE_STREAM_CFM             ((TmapClientMessageId) 0x000F)
#define TMAP_CLIENT_BROADCAST_SRC_STOP_STREAM_CFM               ((TmapClientMessageId) 0x0010)
#define TMAP_CLIENT_BROADCAST_SRC_REMOVE_STREAM_CFM             ((TmapClientMessageId) 0x0011)
#define TMAP_CLIENT_UNICAST_START_STREAM_IND                    ((TmapClientMessageId) 0x0012) 
#define TMAP_CLIENT_BROADCAST_ASST_START_SRC_SCAN_CFM           ((TmapClientMessageId) 0x0013)
#define TMAP_CLIENT_BROADCAST_ASST_SRC_REPORT_IND               ((TmapClientMessageId) 0x0014)
#define TMAP_CLIENT_BROADCAST_ASST_STOP_SRC_SCAN_CFM            ((TmapClientMessageId) 0x0015)
#define TMAP_CLIENT_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM    ((TmapClientMessageId) 0x0016)
#define TMAP_CLIENT_BROADCAST_ASST_READ_BRS_CFM                 ((TmapClientMessageId) 0x0017)
#define TMAP_CLIENT_BROADCAST_ASST_BRS_IND                      ((TmapClientMessageId) 0x0018)
#define TMAP_CLIENT_BROADCAST_ASST_START_SYNC_TO_SRC_CFM        ((TmapClientMessageId) 0x0019)
#define TMAP_CLIENT_BROADCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM    ((TmapClientMessageId) 0x001A)
#define TMAP_CLIENT_BROADCAST_ASST_CANCEL_SYNC_TO_SRC_CFM       ((TmapClientMessageId) 0x001B)
#define TMAP_CLIENT_BROADCAST_ASST_ADD_SRC_CFM                  ((TmapClientMessageId) 0x001C)
#define TMAP_CLIENT_BROADCAST_ASST_MODIFY_SRC_CFM               ((TmapClientMessageId) 0x001D)
#define TMAP_CLIENT_BROADCAST_ASST_REMOVE_SRC_CFM               ((TmapClientMessageId) 0x001E)
#define TMAP_CLIENT_BROADCAST_ASST_SET_CODE_IND                 ((TmapClientMessageId) 0x001F)
#define TMAP_CLIENT_DEREGISTER_CAP_CFM                          ((TmapClientMessageId) 0x0020)
#define TMAP_CLIENT_UNICAST_DISCONNECT_CFM                      ((TmapClientMessageId) 0x0021)
#define TMAP_CLIENT_BROADCAST_SRC_DEINIT_CFM                    ((TmapClientMessageId) 0x0022)
#define TMAP_CLIENT_BROADCAST_SRC_BIG_TEST_CONFIG_CFM           ((TmapClientMessageId) 0x0023)
#define TMAP_CLIENT_BROADCAST_ASST_READ_BRS_IND                 ((TmapClientMessageId) 0x0024)
#define TMAP_CLIENT_VOLUME_STATE_IND                            ((TmapClientMessageId) 0x0025)
#define TMAP_CLIENT_UNICAST_UPDATE_METADATA_IND                 ((TmapClientMessageId) 0x0026)
#define TMAP_CLIENT_SET_PARAMS_CFM                              ((TmapClientMessageId) 0x0027)
#define TMAP_CLIENT_UNICAST_STOP_STREAM_IND                     ((TmapClientMessageId) 0x0028) 

#define TMAP_CLIENT_BROADCAST_ID_DONT_CARE              ((uint32(0xFFFFFFFFu)

#define TMAP_CLIENT_CCID_LTV_LEN 0x03
#define TMAP_CLIENT_CCID_LIST_LENGTH  0x02
#define TMAP_CLIENT_CCID_LIST_TYPE  0x05

typedef CapClientBigConfigParam                  TmapClientBigConfigParam;
typedef CapClientBcastInfo                       TmapClientBcastInfo;
typedef CapClientQhsConfig                       TmapClientQhsConfig;
typedef CapClientQhsBigConfig                    TmapClientQhsBigConfig;
typedef CapClientBcastSrcAdvParams               TmapClientBcastSrcAdvParams;
typedef CapClientSubgroupInfo                    TmapClientSubgroupInfo;
typedef CapClientGroupInfo                       TmapClientGroupInfo;

typedef uint16 TmapClientContext;

#define TMAP_CLIENT_CONTEXT_TYPE_UNSPECIFIED          ((TmapClientContext)CAP_CLIENT_CONTEXT_TYPE_UNSPECIFIED)     /* Unspecified. Matches any audio content. */
#define TMAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL       ((TmapClientContext)CAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL)  /* Phone Call, Conversation between humans */
#define TMAP_CLIENT_CONTEXT_TYPE_MEDIA                ((TmapClientContext)CAP_CLIENT_CONTEXT_TYPE_MEDIA)           /* Music, Radio, Podcast, Video Soundtrack or TV audio */


typedef uint32 TmapClientStreamCapability;

#define TMAP_CLIENT_STREAM_CAPABILITY_UNKNOWN             ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN)
#define TMAP_CLIENT_STREAM_CAPABILITY_8_1                 ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_8_1)
#define TMAP_CLIENT_STREAM_CAPABILITY_8_2                 ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_8_2)
#define TMAP_CLIENT_STREAM_CAPABILITY_16_1                ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_16_1)
#define TMAP_CLIENT_STREAM_CAPABILITY_16_2                ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_16_2)
#define TMAP_CLIENT_STREAM_CAPABILITY_24_1                ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_24_1)
#define TMAP_CLIENT_STREAM_CAPABILITY_24_2                ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_24_2)
#define TMAP_CLIENT_STREAM_CAPABILITY_32_1                ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_32_1)
#define TMAP_CLIENT_STREAM_CAPABILITY_32_2                ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_32_2)
#define TMAP_CLIENT_STREAM_CAPABILITY_441_1               ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_441_1)
#define TMAP_CLIENT_STREAM_CAPABILITY_441_2               ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_441_2)
#define TMAP_CLIENT_STREAM_CAPABILITY_48_1                ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_48_1)
#define TMAP_CLIENT_STREAM_CAPABILITY_48_2                ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_48_2)
#define TMAP_CLIENT_STREAM_CAPABILITY_48_3                ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_48_3)
#define TMAP_CLIENT_STREAM_CAPABILITY_48_4                ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_48_4)
#define TMAP_CLIENT_STREAM_CAPABILITY_48_5                ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_48_5)
#define TMAP_CLIENT_STREAM_CAPABILITY_48_6                ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_48_6)
#define TMAP_CLIENT_STREAM_CAPABILITY_96                  ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_96)
#define TMAP_CLIENT_STREAM_CAPABILITY_LC3_EPC             ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_LC3_EPC)
#define TMAP_CLIENT_STREAM_CAPABILITY_APTX_ADAPTIVE_48_1  ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_APTX_ADAPTIVE_48_1)
#define TMAP_CLIENT_STREAM_CAPABILITY_APTX_ADAPTIVE_96    ((TmapClientStreamCapability) CAP_CLIENT_STREAM_CAPABILITY_APTX_ADAPTIVE_96)
#define TMAP_CLIENT_CODEC_ID_MASK                         ((TmapClientStreamCapability)CAP_CLIENT_CODEC_ID_MASK)



typedef uint8 TmapClientCigConfigMode;
#define TMAP_CLIENT_CIG_CONFIG_MODE_DEFAULT           ((TmapClientCigConfigMode) CAP_CLIENT_CIG_CONFIG_MODE_DEFAULT)
#define TMAP_CLIENT_CIG_CONFIG_MODE_QHS               ((TmapClientCigConfigMode) CAP_CLIENT_CIG_CONFIG_MODE_QHS)
#define TMAP_CLIENT_MODE_JOINT_STEREO                 ((TmapClientCigConfigMode) CAP_CLIENT_MODE_JOINT_STEREO)

typedef uint16 TmapClientBroadcastSrcType;
#define TMAP_CLIENT_BROADCAST_SRC_COLLOCATED       ((TmapClientBroadcastSrcType)CAP_CLIENT_BCAST_SRC_COLLOCATED)
#define TMAP_CLIENT_BROADCAST_SRC_NON_COLLOCATED   ((TmapClientBroadcastSrcType)CAP_CLIENT_BCAST_SRC_NON_COLLOCATED)
#define TMAP_CLIENT_BROADCAST_SRC_ALL              ((TmapClientBroadcastSrcType)CAP_CLIENT_BCAST_SRC_ALL)

typedef uint8 TmapClientBigConfigMode;
#define TMAP_CLIENT_BIG_CONFIG_MODE_DEFAULT           ((TmapClientBigConfigMode )CAP_CLIENT_BIG_CONFIG_MODE_DEFAULT)
#define TMAP_CLIENT_BIG_CONFIG_MODE_QHS               ((TmapClientBigConfigMode )CAP_CLIENT_BIG_CONFIG_MODE_QHS)
#define TMAP_CLIENT_BIG_CONFIG_MODE_JOINT_STEREO      ((TmapClientBigConfigMode )CAP_CLIENT_BIG_CONFIG_MODE_JOINT_STEREO)

typedef uint8  TmapClientParamsType;
#define TMAP_CLIENT_PARAMS_TYPE_NONE                 ((TmapClientParamsType)CAP_CLIENT_PARAM_TYPE_NONE)
#define TMAP_CLIENT_PARAMS_TYPE_UNICAST_CONNECT      ((TmapClientParamsType)CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT)
#define TMAP_CLIENT_PARAMS_TYPE_BROADCAST_CONFIG     ((TmapClientParamsType)CAP_CLIENT_PARAM_TYPE_BCAST_CONFIG)
#define TMAP_CLIENT_PARAMS_TYPE_UNICAST_CONNECT_V1   ((TmapClientParamsType)CAP_CLIENT_PARAM_TYPE_UNICAST_CONNECT_V1)

typedef uint8 TmapClientTargetLatency;
#define TMAP_CLIENT_TARGET_LOWER_LATENCY                     ((TmapClientTargetLatency)(CAP_CLIENT_TARGET_LOWER_LATENCY))
#define TMAP_CLIENT_TARGET_BALANCE_LATENCY_AND_RELIABILITY   ((TmapClientTargetLatency)(CAP_CLIENT_TARGET_BALANCE_LATENCY_AND_RELIABILITY))
#define TMAP_CLIENT_TARGET_HIGH_RELIABILITY                  ((TmapClientTargetLatency)(CAP_CLIENT_TARGET_HIGH_RELIABILITY))

typedef uint8 TmapClientIsoGroupType;
#define TMAP_CLIENT_CIG                              ((TmapClientIsoGroupType)(CAP_CLIENT_CIG))
#define TMAP_CLIENT_BIG                              ((TmapClientIsoGroupType)(CAP_CLIENT_BIG))


/*!
    @brief Profile library message sent as a result of calling the TmapClientInitReq API.
*/
typedef struct
{
    TmapClientMessageId       id;
    TmapClientStatus          status;          /*! Status of the initialisation attempt*/
    TmapClientProfileHandle   prflHndl;        /*! TMAP Client profile handle */
    ServiceHandle             tmasSrvcHandle;
} TmapClientInitCfm;

/*! @brief Contents of the TmapClientRoleCfm message that is sent by the library,
    as response to a read of the Role Name.
*/
typedef struct
{
    TmapClientMessageId       id;
    TmapClientProfileHandle   prflHndl;         /*! TMAP Client profile handle*/
    ServiceHandle             srvcHndl;         /*! Reference handle for the instance */
    TmapClientStatus          status;           /*! Status of the reading attempt */
    uint16                    role;             /*! Role Value */
} TmapClientRoleCfm;

/*!
    @brief Profile library message sent as a result of calling the TmapClientDestroyReq API.

    This message will send at first with the value of status of TMAP_CLIENT_STATUS_IN_PROGRESS.
    Another TMAP_CLIENT_DESTROY_CFM message will be sent with the final status (success or fail),
    after TMAP_CLIENT_TMAS_TERMINATE_CFM has been received.
*/
typedef struct
{
    TmapClientMessageId       id;
    TmapClientProfileHandle   prflHndl;     /*! TMAP Client profile handle*/
    TmapClientStatus          status;       /*! Status of the initialisation attempt*/
} TmapClientDestroyCfm;

/*!
    @brief Profile library message sent as a result of calling the TmapTerminateReq API and
           of the receiving of the GATT_TMAS_CLIENT_TERMINATE_CFM message from the gatt_tmas_client
           library.
*/
typedef struct
{
    TmapClientMessageId             id;
    TmapClientProfileHandle         prflHndl;          /*! TMAP Client profile handle*/
    TmapClientStatus                status;            /*! Status of the termination attempt*/
    GattTmasClientDeviceData        tmasClientHandle;  /*! Characteristic handles of TMAS Client */
} TmapClientTmasTerminateCfm;

typedef struct
{
    TmapClientMessageId         type;                 /*!< TMAP_CLIENT_REGISTER_CAP_CFM */
    ServiceHandle               groupId;              /*!< Group Id */
    CapClientResult             result;               /*!< CAP Status */
} TmapClientRegisterTaskCfm;

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
}TmapClientUnicastConnectParams;

/* Full set for cig/cis params */
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
    const uint8                *vsConfig;                     /* LTV pair(s) for any vendor or proprietary info  */
} TmapClientUnicastConnectParamsV1;

typedef struct
{
    uint8                      rtn;                    /* Unicast Retransmission Number*/
    uint16                     sduSize;                /* Max SDU size */
    uint8                      maxCodecFramesPerSdu;
    TmapClientTargetLatency    targetLatency;
    uint16                     maxLatency;             /* Unicast Transport Latency */
    uint16                     phy;
    uint32                     sduInterval;            /* Sdu Interval */
    TmapClientStreamCapability subGroupConfig;
} TmapClientBroadcastConfigParams;


typedef struct
{
    TmapClientMessageId         type;
    CapClientResult             result;
    uint32                      profileHandle;       /*!< Broadcast Source Handle/ GroupId */
} TmapClientSetParamsCfm;

typedef struct
{
    TmapClientMessageId         type;                  /*!< TMAP_CLIENT_UNICAST_CONNECT_CFM */
    ServiceHandle               groupId;               /*!< Group Id */
    TmapClientContext           context;               /*!< Audio Context */
    CapClientResult             result;                /*!< CAP Status */
    uint8                       numOfMicsConfigured;   /*!< Mics Configured */
    uint8                       cigId;                 /*!< cig Id */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;         /*!< Individual Device Status */
} TmapClientUnicastConnectCfm;

typedef struct
{
    TmapClientMessageId         type;             /*!< TMAP_CLIENT_UNICAST_START_STREAM_IND */
    ServiceHandle               groupId;          /*!< Group Id */
    uint32                      cid;              /*!< BtConnID */ 
    CapClientResult             result;           /*!< CAP Status */
    CapClientAudioConfig        *audioConfig;
    CapClientCisHandles         *cishandles;      /*!< Unicast ISO handles */
    uint8                       cisCount;
    uint8                       cigId;
} TmapClientUnicastStartStreamInd;

typedef struct
{
    TmapClientMessageId         type;            /*!< TMAP_CLIENT_UNICAST_START_STREAM_CFM */
    ServiceHandle               groupId;         /*!< Group Id */
    CapClientResult             result;          /*!< CAP Status */
} TmapClientUnicastStartStreamCfm;

typedef struct
{
     TmapClientMessageId        type;                   /*!< TMAP_CLIENT_UNICAST_UPDATE_METADATA_IND */
     CapClientResult            result;                 /*!< CAP Status. */
     uint16                     streamingAudioContexts; /*!< Bitmask of Audio Context type values */
     ServiceHandle              groupId;                /*!< Group Id */
     uint32                     cid;
     bool                       isSink;
     uint8                      metadataLen;
     uint8                      *metadata;
}TmapClientUnicastUpdateMetadataInd;

typedef struct
{
    TmapClientMessageId         type;                  /*!< TMAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM */
    ServiceHandle               groupId;               /*!< CAP Handle */
    TmapClientContext           context;               /*!< Group Id */
    CapClientResult             result;                /*!< CAP Status */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;         /*!< Individual Device Status */
} TmapClientUnicastUpdateAudioCfm;

/* When released is CAP_CLIENT_STATE_DISABLING it means Remote is in QOS config state.
 * Application can call start stream directly for the given use case o start.
 * If released state is CAP_CLIENT_STATE_RELEASING it means Remote is in Idle/Codec Config.
 * Application need to call unicast connect to start the use case again.
 * Other than these values Remote state is not known and App may release the given use case */
typedef struct
{
    TmapClientMessageId         type;            /*!< TMAP_CLIENT_UNICAST_STOP_STREAM_IND */
    ServiceHandle               groupId;         /*!< handle of CAP */
    uint32                      cid;             /*!< BtConnID */
    uint16                      *cishandles;     /*!< Unicast ISO handles */
    uint8                       cisCount;
    CapClientAseState           released;        /*!< Refer CapClientAseState for values */
    uint8                       cigId;           /*!< CIG id for the stopped use case */
} TmapClientUnicastStopStreamInd;

typedef struct
{
    TmapClientMessageId         type;            /*!< TMAP_CLIENT_UNICAST_STOP_STREAM_CFM */
    ServiceHandle               groupId;         /*!< Group Id */
    bool                        released;        /*!< If release Done */
    CapClientResult             result;          /*!< CAP Status */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;    /*!< Individual Device Status */
} TmapClientUnicastStopStreamCfm;

typedef struct
{
    TmapClientMessageId         type;                  /*!< TMAP_CLIENT_UNICAST_DISCONNECT_CFM */
    ServiceHandle               groupId;               /*!< CAP Handle */
    CapClientResult             result;                /*!< CAP Status */
} TmapClientUnicastDisconnectCfm;

typedef struct
{
    TmapClientMessageId         type;                 /*!< TMAP_CLIENT_DEREGISTER_CAP_CFM */
    ServiceHandle               groupId;              /*!< Group Id */
    CapClientResult             result;               /*!< CAP Status */
} TmapClientDeRegisterTaskCfm;

typedef struct
{
    TmapClientMessageId         type;
    ServiceHandle               groupId;         /*!< Group Id */
    CapClientResult             result;          /*!< CAP Status */
    uint8                       deviceStatusLen;
    CapClientDeviceStatus       *deviceStatus;   /*!< Individual Device Status */
} TmapClientVolumeCommonCfm;

typedef TmapClientVolumeCommonCfm                 TmapClientAbsVolumeCfm;
typedef TmapClientVolumeCommonCfm                 TmapClientMuteCfm;

typedef struct
{
    TmapClientMessageId         type;            /*!< TMAP_CLIENT_VOLUME_STATE_IND */
    ServiceHandle               groupId;         /*!< CAP handle */
    uint8                       volumeState;
    uint8                       mute;
    uint8                       changeCounter;
} TmapClientVolumeStateInd;

typedef struct
{
    TmapClientStreamCapability config;                  /*<! Setting which needs to be configured per BIS*/
    uint32                     audioLocation;
    uint8                      targetLatency;
    uint8                      lc3BlocksPerSdu;
}TmapClientBisInfo;

/*!
    This structure will be deprecated. Please use TmapClientBigMultiSubGroup
*/
typedef struct
{
    TmapClientStreamCapability config;                  /*<! Setting which needs to be configured*/
    uint8                      targetLatency;           /*<! low latency or high reliability */
    uint8                      lc3BlocksPerSdu;
    TmapClientContext          useCase;                 /* Context type of Audio/Usecase */
    uint8                      metadataLen;             /* Total length of vendor metadata */
    uint8                      *metadata;                /* LTV format */
    uint8                      numBis;
    /*!
     *  The array of TmapClientBisInfo has 'numBis' elements.
     *  There is one TmapClientBisInfo per BIS.
     *
     *  The memory allocated to store the TmapClientBisInfo structure is one contiguous
     *  block of memory that is large enough to store the appropriate number of.
     *  TmapClientBisInfo (i.e. the number specified by the numBis field).
     *  This means that, if numBis is greater than 1, then the memory allocated to store
     *  the TmapClientBisInfo is greater than sizeof(TmapClientBisInfo).
     */
    TmapClientBisInfo          bisInfo[1];
}TmapClientBigSubGroup;

typedef struct
{
    TmapClientStreamCapability config;                  /*<! Setting which needs to be configured*/
    uint8                      targetLatency;           /*<! low latency or high reliability */
    uint8                      lc3BlocksPerSdu;
    TmapClientContext          useCase;                 /* Context type of Audio/Usecase */
    uint8                      metadataLen;             /* Total length of vendor metadata */
    uint8                      *metadata;                /* LTV format */
    uint8                      numBis;
    TmapClientBisInfo*         bisInfo;          /* The array of TmapClientBisInfo has 'numBis' elements.*/
}TmapClientBigMultiSubGroup;

typedef struct
{
    TmapClientMessageId         type;                     /*!< TMAP_CLIENT_BROADCAST_SRC_INIT_CFM */
    CapClientResult             result;
    TmapClientProfileHandle     bcastSrcProfileHandle;    /*!< Broadcast Source Handle */
} TmapClientBroadcastSrcInitCfm;

typedef struct
{
    TmapClientMessageId         type;                     /*!< TMAP_CLIENT_BROADCAST_SRC_CONFIG_CFM */
    CapClientResult             result;
    TmapClientProfileHandle     bcastSrcProfileHandle;    /*!< Broadcast Source Handle */
} TmapClientBroadcastSrcConfigCfm;

typedef struct
{
    TmapClientMessageId         type;                     /*!<TMAP_CLIENT_BROADCAST_SRC_START_STREAM_CFM */
    CapClientResult             result;
    TmapClientProfileHandle     bcastSrcProfileHandle;    /* Broadcast Source Handle */
    uint8                       bigId;
    uint32                      bigSyncDelay;             /* BIG Sync delay */
    CapClientBigParam           *bigParameters;
    uint8                       numSubGroup;              /* Number of subgroups for a given broadcast Source Handle */
    CapClientBcastSubGroupInfo  *subGroupInfo;
}TmapClientBroadcastSrcStartStreamCfm;

typedef struct
{
    TmapClientMessageId         type;
    CapClientResult             result;                   /*!< CAP Status */
    TmapClientProfileHandle     bcastSrcProfileHandle;    /*!< Broadcast Source Handle */
} TmapClientBroadcastSrcCommonCfm;

typedef TmapClientBroadcastSrcCommonCfm TmapClientBroadcastSrcUpdateStreamCfm;
typedef TmapClientBroadcastSrcCommonCfm TmapClientBroadcastSrcStopStreamCfm;
typedef TmapClientBroadcastSrcCommonCfm TmapClientBroadcastSrcRemoveStreamCfm;
typedef TmapClientBroadcastSrcCommonCfm TmapClientBroadcastSrcDeinitCfm;

typedef struct
{
    uint32 cid;
    uint8  sourceId;
}TmapClientBroadcastSinkInfo;

typedef struct
{
    TmapClientMessageId    type;        /*!< TMAP_CLIENT_BROADCAST_ASST_START_SRC_SCAN_CFM */
    ServiceHandle          groupId;     /*!< CAP Handle */
    CapClientResult        result;      /*!< Result code. */
    uint16                 scanHandle;  /*!< scanHandle */
    uint8                  statusLen;
    CapClientDeviceStatus  *status;
}TmapClientBroadcastAsstStartSrcScanCfm;

typedef struct
{
    TmapClientMessageId         type;          /*!< TMAP_CLIENT_BROADCAST_ASST_SRC_REPORT_IND */
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
}TmapClientBroadcastAsstSrcReportInd;

typedef struct
{
    TmapClientMessageId    type;      /*!< TMAP_CLIENT_BROADCAST_ASST_STOP_SRC_SCAN_CFM */
    ServiceHandle          groupId;   /*!< CAP Handle */
    CapClientResult        result;    /*!< Result code. */
    uint8                  statusLen;
    CapClientDeviceStatus  *status;
}TmapClientBroadcastAsstStopSrcScanCfm;

typedef struct
{
    TmapClientMessageId    type;            /* TMAP_CLIENT_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM */           
    ServiceHandle          groupId;         /*!< CAP Handle */
    CapClientResult        result;          /*!< CAP Status */
    uint8                  statusLen;
    CapClientDeviceStatus  *status;
}TmapClientBroadcastAsstRegisterNotificationCfm;

typedef struct
{
    TmapClientMessageId         type;             /*!< TMAP_CLIENT_BROADCAST_ASST_READ_BRS_IND */
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
    TmapClientSubgroupInfo      *subGroupInfo;
}TmapClientBroadcastAsstReadBrsInd;

typedef struct
{
    TmapClientMessageId         type;             /*!< TMAP_CLIENT_BROADCAST_ASST_READ_BRS_CFM */
    ServiceHandle               groupId;          /*!< CAP Handle */
    CapClientResult             result;           /*!< Result code. */
    uint32                      cid;              /*!< Connection Id*/
}TmapClientBroadcastAsstReadBrsCfm;

typedef struct
{
    TmapClientMessageId         type;             /*!< TMAP_CLIENT_BROADCAST_ASST_BRS_IND */
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
    TmapClientSubgroupInfo      *subGroupInfo;
}TmapClientBroadcastAsstBrsInd;

typedef struct
{
    TmapClientMessageId   type;          /*!< TMAP_CLIENT_BROADCAST_ASST_START_SYNC_TO_SRC_CFM */
    ServiceHandle         groupId;       /*!< CAP Handle */
    CapClientResult       result;        /*!< Result code. */
    uint16                syncHandle;    /*!< Sync handle of the PA */
    uint8                 advSid;
    TYPED_BD_ADDR_T       addrt;
    uint8                 advPhy;
    uint16                periodicAdvInterval;
    uint8                 advClockAccuracy;
}TmapClientBroadcastAsstStartSyncToSrcCfm;

typedef struct
{
    TmapClientMessageId   type;       /*!< TMAP_CLIENT_BROADCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM */
    ServiceHandle         groupId;    /*!< CAP Handle */
    CapClientResult       result;     /*!< Result code. */
    uint16                syncHandle;
}TmapClientBroadcastAsstTerminateSyncToSrcCfm;

typedef struct
{
    TmapClientMessageId   type;       /*!< TMAP_CLIENT_BROADCAST_ASST_CANCEL_SYNC_TO_SRC_CFM */
    ServiceHandle         groupId;    /*!< CAP Handle */
    CapClientResult       result;     /*!< Result code. */
}TmapClientBroadcastAsstCancelSyncToSrcCfm;

typedef struct
{
    TmapClientMessageId     type;            /* TMAP_CLIENT_BROADCAST_ASST_ADD_SRC_CFM  */           
    ServiceHandle           groupId;         /*!< CAP Handle */
    CapClientResult         result;          /*!< CAP Status */
    uint8                   statusLen;
    CapClientDeviceStatus*  status;
}TmapClientBroadcastAsstAddSrcCfm;

typedef struct
{
    TmapClientMessageId      type;            /* TMAP_CLIENT_BROADCAST_ASST_MODIFY_SRC_CFM  */           
    ServiceHandle            groupId;         /*!< CAP Handle */
    CapClientResult          result;          /*!< CAP Status */
    uint8                    statusLen;
    CapClientDeviceStatus*   status;
}TmapClientBroadcastAsstModifySrcCfm;

typedef struct
{
    TmapClientMessageId      type;            /* TMAP_CLIENT_BROADCAST_ASST_REMOVE_SRC_CFM  */           
    ServiceHandle            groupId;         /*!< CAP Handle */
    CapClientResult          result;          /*!< CAP Status */
    uint8                    statusLen;
    CapClientDeviceStatus*   status;
}TmapClientBroadcastAsstRemoveSrcCfm;

typedef struct
{
    TmapClientMessageId         type;      /*!< TMAP_CLIENT_BROADCAST_ASST_SET_CODE_IND */
    ServiceHandle               groupId;   /*!< CAP Handle */
    uint32                      cid;       /*!< BAP Handle */
    uint8                       sourceId;  /*! Source_id of the Broadcast Receiver State characteristic */
    uint8                       flags;  
}TmapClientBroadcastAsstSetCodeInd;


#endif /* TMAP_CLIENT_PRIM_H */

