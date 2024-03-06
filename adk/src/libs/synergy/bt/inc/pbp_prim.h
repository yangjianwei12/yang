/*******************************************************************************

Copyright (C) 2022-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef PBP_PRIM_H
#define PBP_PRIM_H

#include "csr_list.h"
#include "csr_types.h"
#include "qbl_types.h"
#include "bluetooth.h"
#include "service_handle.h"
#include "audio_announcement_parser_lib.h"

/* PBP Primitives */

typedef uint16 PbpPrim;
#define PBP_PRIM_BASE             (SYNERGY_EVENT_BASE + PBP_PRIM)

/*PBP upstream Prims*/
#define PBP_INIT_CFM                                               ((PbpPrim)0x0000u)
#define PBP_DESTROY_CFM                                            ((PbpPrim)0x0001u)
#define PBP_BROADCAST_SRC_INIT_CFM                                 ((PbpPrim)0x0002u)
#define PBP_BROADCAST_SRC_CONFIG_CFM                               ((PbpPrim)0x0003u)
#define PBP_BROADCAST_SRC_START_STREAM_CFM                         ((PbpPrim)0x0004u)
#define PBP_BROADCAST_SRC_UPDATE_AUDIO_CFM                         ((PbpPrim)0x0005u)
#define PBP_BROADCAST_SRC_STOP_STREAM_CFM                          ((PbpPrim)0x0006u)
#define PBP_BROADCAST_SRC_REMOVE_STREAM_CFM                        ((PbpPrim)0x0007u)
#define PBP_BROADCAST_SRC_DEINIT_CFM                               ((PbpPrim)0x0009u)
#define PBP_BROADCAST_ASSISTANT_SCAN_SRC_START_CFM                 ((PbpPrim)0x000Au)
#define PBP_BROADCAST_ASSISTANT_SCAN_SRC_STOP_CFM                  ((PbpPrim)0x000Bu)
#define PBP_BROADCAST_ASSISTANT_START_SYNC_TO_SRC_CFM              ((PbpPrim)0x000Cu)
#define PBP_BROADCAST_ASSISTANT_TERMINATE_SYNC_TO_SRC_CFM          ((PbpPrim)0x000Du)
#define PBP_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_CFM             ((PbpPrim)0x000Eu) 
#define PBP_BROADCAST_ASSISTANT_ADD_SRC_CFM                        ((PbpPrim)0x000Fu)
#define PBP_BROADCAST_ASSISTANT_MODIFY_SRC_CFM                     ((PbpPrim)0x0010u)
#define PBP_BROADCAST_ASSISTANT_REMOVE_SRC_CFM                     ((PbpPrim)0x0011u)
#define PBP_BROADCAST_ASSISTANT_REG_FOR_NOTIFICATION_CFM           ((PbpPrim)0x0012u)
#define PBP_BROADCAST_ASSISTANT_READ_RECEIVE_STATE_CFM             ((PbpPrim)0x0013u)
#define PBP_BROADCAST_ASSISTANT_SRC_REPORT_IND                     ((PbpPrim)0x0014u)
#define PBP_BROADCAST_ASSISTANT_BRS_IND                            ((PbpPrim)0x0015u)
#define PBP_BROADCAST_ASSISTANT_SYNC_LOSS_IND                      ((PbpPrim)0x0016u)
#define PBP_BROADCAST_ASSISTANT_SET_CODE_IND                       ((PbpPrim)0x0017u)
#define PBP_BROADCAST_ASSISTANT_READ_RECEIVE_STATE_IND             ((PbpPrim)0x0018u)
#define PBP_REGISTER_CAP_CFM                                       ((PbpPrim)0x0019u)
#define PBP_DEREGISTER_TASK_CFM                                    ((PbpPrim)0x001Au)
#define PBP_BROADCAST_SRC_SET_PARAM_CFM                            ((PbpPrim)0x001Bu)

/*
    PBP status code type.
*/
typedef uint16                           PbpStatus;

#define PBP_STATUS_SUCCESS              ((PbpStatus) 0x00)  /*!> Request was a success*/
#define PBP_STATUS_FAILED               ((PbpStatus) 0x01)  /*!> Request has failed*/

#define PBP_INVALID_CID  (0xFFFFFFFFu)

/* Mode type */
typedef CapClientBigConfigMode PbpBigConfigMode;

#define PBP_BIG_CONFIG_MODE_DEFAULT      ((PbpBigConfigMode)CAP_CLIENT_BIG_CONFIG_MODE_DEFAULT)
#define PBP_BIG_CONFIG_MODE_QHS          ((PbpBigConfigMode)CAP_CLIENT_BIG_CONFIG_MODE_QHS) 
#define PBP_BIG_CONFIG_MODE_JOINT_STEREO ((PbpBigConfigMode)CAP_CLIENT_MODE_JOINT_STEREO)


typedef uint16 PbpContext;

#define PBP_CONTEXT_TYPE_UNSPECIFIED          ((PbpContext)CAP_CLIENT_CONTEXT_TYPE_UNSPECIFIED)      /* Unspecified. Matches any audio content. */
#define PBP_CONTEXT_TYPE_CONVERSATIONAL       ((PbpContext)CAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL)   /* Phone Call, Conversation between humans */
#define PBP_CONTEXT_TYPE_MEDIA                ((PbpContext)CAP_CLIENT_CONTEXT_TYPE_MEDIA)            /* Music, Radio, Podcast, Video Soundtrack or TV audio */
#define PBP_CONTEXT_TYPE_GAME                 ((PbpContext)CAP_CLIENT_CONTEXT_TYPE_GAME)             /* Audio associated with gaming */
#define PBP_CONTEXT_TYPE_INSTRUCTIONAL        ((PbpContext)CAP_CLIENT_CONTEXT_TYPE_INSTRUCTIONAL)    /* Satnav, User Guidance, Traffic Announcement */
#define PBP_CONTEXT_TYPE_VOICE_ASSISTANT      ((PbpContext)CAP_CLIENT_CONTEXT_TYPE_VOICE_ASSISTANT)  /* Virtual Assistant, Voice Recognition */
#define PBP_CONTEXT_TYPE_LIVE                 ((PbpContext)CAP_CLIENT_CONTEXT_TYPE_LIVE)             /* Live Audio */
#define PBP_CONTEXT_TYPE_SOUND_EFFECTS        ((PbpContext)CAP_CLIENT_CONTEXT_TYPE_SOUND_EFFECTS)    /*Sound effects including keyboard and touch feedback;
                                                                                                     menu and user interface sounds; and other system sounds */
#define PBP_CONTEXT_TYPE_NOTIFICATIONS        ((PbpContext)CAP_CLIENT_CONTEXT_TYPE_NOTIFICATIONS)    /* Incoming Message Alert, Keyboard Click */
#define PBP_CONTEXT_TYPE_RINGTONE             ((PbpContext)CAP_CLIENT_CONTEXT_TYPE_RINGTONE)         /* Incoming Call */
#define PBP_CONTEXT_TYPE_ALERTS               ((PbpContext)CAP_CLIENT_CONTEXT_TYPE_ALERTS)           /* Low Battery Warning, Alarm Clock, Timer Expired */
#define PBP_CONTEXT_TYPE_EMERGENCY_ALARM      ((PbpContext)CAP_CLIENT_CONTEXT_TYPE_EMERGENCY_ALARM)

#define PBP_INVALID_CID  (0xFFFFFFFFu)
#define PBP_BROADCAST_ID_DONT_CARE              ((uint32(0xFFFFFFFFu)


typedef uint16 PbpBcastSrcLocation;
#define PBP_BCAST_SRC_COLLOCATED       ((PbpBcastSrcLocation)CAP_CLIENT_BCAST_SRC_COLLOCATED)
#define PBP_BCAST_SRC_NON_COLLOCATED   ((PbpBcastSrcLocation)CAP_CLIENT_BCAST_SRC_NON_COLLOCATED)
#define PBP_BCAST_SRC_ALL              ((PbpBcastSrcLocation)CAP_CLIENT_BCAST_SRC_ALL)

/* Bit mask to determine which BroadcastType to configure */
typedef uint32 PbpBcastType;
#define PBP_BCAST_SRC_SQ_PUBLIC_BROADCAST         ((CapClientBcastType)SQ_PUBLIC_BROADCAST)
#define PBP_BCAST_SRC_HQ_PUBLIC_BROADCAST         ((CapClientBcastType)HQ_PUBLIC_BROADCAST)

typedef uint8 PbpPaSyncState;
#define PBP_PA_SYNC_NOT_SYNCHRONIZE         ((PbpPaSyncState)0x00u)  /*!> Not synchronize to PA*/
#define PBP_PA_SYNC_SYNCHRONIZE_PAST        ((PbpPaSyncState)0x01u)  /*!> Synchronize to PA - PAST available */
#define PBP_PA_SYNC_SYNCHRONIZE_NO_PAST     ((PbpPaSyncState)0x02u)  /*!> Synchronize to PA - PAST no available*/
#define PBP_PA_SYNC_LOST                    ((PbpPaSyncState)0x03u)

/*
    Sub group related information type
*/
typedef CapClientBigSubGroup PbpBigSubGroups;

/*
    Profile handle type.
*/
typedef ServiceHandle PbpProfileHandle;

/* Connection ID type */
typedef uint32       PbpConnectionId;

/*
    Type of the configuraration of the BIG parameters in QHS mode.
*/
typedef CapClientQhsBigConfig PbpQhsBigConfig;

/*
    Public Broadcast information type
*/
typedef CapClientBcastInfo PbpBroadcastInfo;

/*
    Periodic Advertisement parameters to set type
*/
typedef CapClientBcastSrcAdvParams PbpBcastSrcAdvParams;

/*
    Subgroup info type
*/
typedef CapClientSubgroupInfo  PbpSubgroupInfo;

/*
    Type of the structure containing Connected Broadcast Delegators info
*/
typedef CapClientDelegatorInfo PbpBroadcastDelegatorInfo;

typedef CapClientBcastConfigParam PbpBcastConfigParam;

/* Upstream Primitives */

typedef struct
{
    PbpPrim            type;      /*!< PBP_INIT_CFM */
    PbpStatus          status;    /*! Status of the initialisation attempt*/
    PbpProfileHandle   prflHndl;  /*! PBP profile handle */
}PbpInitCfm;

typedef struct
{
    PbpPrim            type;      /*!< PBP_DESTROY_CFM */
    PbpStatus          status;    /*! Status of the destroy attempt*/
    PbpProfileHandle   prflHndl;  /*! PBP profile handle */
}PbpDestroyCfm;

typedef struct
{
    PbpPrim           type;                   /*!< PBP_BROADCAST_SRC_CONFIG_CFM */
    CapClientResult   result;
    PbpProfileHandle  bcastSrcProfileHandle;  /*!< Broadcast Source Handle */
} PbpBroadcastSrcConfigCfm;

typedef PbpBroadcastSrcConfigCfm PbpBroadcastSrcSetParamCfm;

typedef struct
{
    PbpPrim                     type;                   /*!< PBP_BROADCAST_SRC_START_STREAM_CFM */
    CapClientResult             result;
    PbpProfileHandle            bcastSrcProfileHandle;  /*!< Broadcast Source Handle */
    uint8                       bigId;                  /*!< BIG identifier */
    uint32                      bigSyncDelay;           /*!< BIG Sync delay */
    CapClientBigParam*          bigParameters;          /*!< BIG parameters */
    uint8                       numSubGroup;            /*!< Number of subgroups for a given broadcast Source Handle */
    CapClientBcastSubGroupInfo* subGroupInfo;           /*!< Subgroup info */
}PbpBroadcastSrcStartStreamCfm;

typedef struct
{
    PbpPrim           type;
    CapClientResult   result;                 /*!< CAP Status */
    PbpProfileHandle  bcastSrcProfileHandle;  /*!< Broadcast Source Handle */
} PbpBroadcastSrcCommonCfm;

typedef PbpBroadcastSrcCommonCfm PbpBroadcastSrcInitCfm;
typedef PbpBroadcastSrcCommonCfm PbpBroadcastSrcDeinitCfm;
typedef PbpBroadcastSrcCommonCfm PbpBroadcastSrcUpdateAudioCfm;
typedef PbpBroadcastSrcCommonCfm PbpBroadcastSrcStopStreamCfm;
typedef PbpBroadcastSrcCommonCfm PbpBroadcastSrcRemoveStreamCfm;

typedef struct
{
    PbpPrim                type;        /*!< PBP_BROADCAST_ASSISTANT_SCAN_SRC_START_CFM */
    ServiceHandle          groupId;     /*!< CAP Handle */
    CapClientResult        result;      /*!< Result code. */
    uint16                 scanHandle;  /*!< scanHandle */
    uint8                  statusLen;
    CapClientDeviceStatus* status;
} PbpBroadcastAssistantScanSrcStartCfm;

typedef struct
{
    PbpPrim                type;
    ServiceHandle          groupId;
    CapClientResult        result;  /*!< Result code. */
    uint8                  statusLen;
    CapClientDeviceStatus* status;
} PbpBroadcastAssistantCommonMsg;

typedef PbpBroadcastAssistantCommonMsg PbpBroadcastAssistantScanSrcStopCfm;
typedef PbpBroadcastAssistantCommonMsg PbpBroadcastAssistantAddSrcCfm;
typedef PbpBroadcastAssistantCommonMsg PbpBroadcastAssistantModifySrcCfm;
typedef PbpBroadcastAssistantCommonMsg PbpBroadcastAssistantRemoveSrcCfm;
typedef PbpBroadcastAssistantCommonMsg PbpBroadcastAssistantRegForNotificationCfm;

typedef struct
{
    PbpPrim         type;     /*!< PBP_BROADCAST_ASSISTANT_READ_RECEIVE_STATE_CFM */
    ServiceHandle   groupId;  /*!< CAP Handle */
    CapClientResult result;   /*!< Result code. */
    uint32          cid;
} PbpBroadcastAssistantReadReceiveStateCfm;

typedef struct
{
    PbpPrim          type;        /*!< PBP_BROADCAST_ASSISTANT_START_SYNC_TO_SRC_CFM */
    ServiceHandle    groupId;     /*!< CAP Handle */
    CapClientResult  result;      /*!< Result code. */
    uint16           syncHandle;  /*!< Sync handle of the PA */
    uint8            advSid;
    TYPED_BD_ADDR_T  addrt;
    uint8            advPhy;
    uint16           periodicAdvInterval;
    uint8            advClockAccuracy;
} PbpBroadcastAssistantStartSyncToSrcCfm;

typedef struct
{
    PbpPrim          type;     /*!< PBP_BROADCAST_ASSISTANT_TERMINATE_SYNC_TO_SRC_CFM */
    ServiceHandle    groupId;  /*!< CAP Handle */
    CapClientResult  result;   /*!< Result code. */
    uint16           syncHandle;
} PbpBroadcastAssistantTerminateSyncToSrcCfm;

typedef struct
{
    PbpPrim          type;     /*!< PBP_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_CFM */
    ServiceHandle    groupId;  /*!< CAP Handle */
    CapClientResult  result;   /*!< Result code. */
} PbpBroadcastAssistantSyncToSrcCancelCfm;

typedef struct
{
    PbpPrim          type;                           /*!< PBP_BROADCAST_ASSISTANT_SRC_REPORT_IND */
    TYPED_BD_ADDR_T  sourceAddrt;                    /*! BT address of the Broadcast Source */
    uint8            advSid;                         /*! Advertising SID */
    uint8            advHandle;                      /*! advHandle valid in case of collocated Broadcast Source. Else it will be 0 */
    bool             collocated;                     /*! Whether the Broad is collocated(local) or not */
    uint32           broadcastId;                    /*!< Broadcast ID */
    /* Level 1 */
    uint8            numSubgroup;                    /*!< Number of Subgroups */
    /* Level 2 Sub Group and Level 3 */
    BapBigSubgroup   *subgroupInfo;                  /*!< Subgroups Info - It's application responsability 
                                                          to free the memory associated with this pointer */
    uint8            bigNameLen;                     
    char             *bigName;                       /*!< BIG Name - It's application responsability 
                                                              to free the memory associated with this pointer */
    AudioAnnouncementParserPbpDataType pbpFeatures;  /*!< Public Broadcast Announcement features */
}PbpBroadcastAssistantSrcReportInd;

typedef struct
{
    PbpPrim          type;         /*!< PBP_BROADCAST_ASSISTANT_BRS_IND */
    ServiceHandle    groupId;      /*!< CAP Handle */
    uint32           cid;
    uint8            sourceId;     /*! Source_id of the Broadcast Receive State characteristic */
    TYPED_BD_ADDR_T  sourceAddrt;  /*! BT address of the Broadcast Source */
    uint8            advSid;
    PbpPaSyncState   paSyncState;
    uint8            bigEncryption;
    uint32           broadcastId;  /*!< Broadcast ID */
    uint8            *badCode;     /*!< Bad broadcast Code. NULL if not present.
                                        It's app/profile responsibility to free this pointer.
                                        Pointer size is 16.*/
    uint8            numSubGroups;
    PbpSubgroupInfo *subGroupInfo; /*!< Subgroups info. It's app/profile responsibility 
                                        to free this pointer. Pointer size is numSubGroups.*/
}PbpBroadcastAssistantBrsInd;

typedef struct
{
    PbpPrim       type;     /*!< PBP_BROADCAST_ASSISTANT_SYNC_LOSS_IND */
    ServiceHandle groupId;  /*!< CAP Handle */
    uint32        cid;
    uint16        syncHandle;
} PbpBroadcastAssistantSyncLossInd;

typedef struct
{
    PbpPrim        type;      /*!< PBP_BROADCAST_ASSISTANT_SET_CODE_IND */
    ServiceHandle  groupId;   /*!< CAP Handle */
    uint32         cid;
    uint8          sourceId;  /*! Source_id of the Broadcast
                                  Receive State characteristic */
    uint8          flags;
}PbpBroadcastAssistantSetCodeInd;

typedef struct
{
    PbpPrim         type;           /*!< PBP_BROADCAST_ASSISTANT_READ_RECEIVE_STATE_IND */
    ServiceHandle   groupId;        /*!< CAP Handle */
    CapClientResult result;         /*!< Result code. */
    uint32          cid; 
    uint8           sourceId;       /*! Source_id of the Broadcast Receive State characteristic */
    TYPED_BD_ADDR_T sourceAddrt;    /*! BT address of the Broadcast Source */
    uint8           advSid;
    PbpPaSyncState  paSyncState;
    uint8           bigEncryption;
    uint32          broadcastId;    /*!< Broadcast ID */
    uint8           *badCode;       /*!< Bad broadcast Code. NULL if not present.
                                         It's app/profile responsibility to free this pointer.
                                         Pointer size is 16.*/
    uint8           numSubGroups;
    PbpSubgroupInfo *subGroupInfo;  /*!< Subgroups info. It's app/profile responsibility 
                                         to free this pointer. Pointer size is numSubGroups.*/
}PbpBroadcastAssistantReadReceiveStateInd;

typedef struct
{
    PbpPrim          type;     /*!< PBP_REGISTER_CAP_CFM */
    ServiceHandle    groupId;  /*!< Group Id */
    CapClientResult  result;   /*!< CAP Status */
} PbpRegisterTaskCfm;

typedef struct
{
    PbpPrim          type;     /*!< PBP_DEREGISTER_TASK_CFM */
    ServiceHandle    groupId;  /*!< Group Id */
    CapClientResult  result;   /*!< CAP Status */
} PbpDeRegisterTaskCfm;

#endif
