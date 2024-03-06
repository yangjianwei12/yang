#ifndef CSR_BT_AV_MAIN_H__
#define CSR_BT_AV_MAIN_H__
/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_bt_result.h"
#include "bluetooth.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_message_queue.h"
#include "csr_bt_av_prim.h"
#include "csr_bt_usr_config.h"
#include "csr_bt_av_fragmentation.h"
#include "csr_bt_cmn_sdc_rfc_util.h"
#include "csr_bt_cmn_sdr_tagbased_lib.h"
#include "csr_bt_cmn_sdp_lib.h"
#include "csr_log_text_2.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtAvLto);

#define AV_QOS_MAX_INDICATION            (10)        /* This will show the status of the buffer 0 - AV_QOS_MAX_INDICATION */
#define AV_SDP_REG_RETRIES               (10)        /* Number of SDP register tries before failure */

#define AVDTP_1_2_VERSION                ((CsrUint16) 0x102)
#define AVDTP_1_3_VERSION                ((CsrUint16) 0x103)
#define DEFAULT_AVDTP_VERSION            AVDTP_1_2_VERSION
#define LOCAL_AVDTP_VERSION              AVDTP_1_3_VERSION

#define CSR_BT_AV_MEDIA_TYPE_INDEX            (2)
#define CSR_BT_AV_CODEC_TYPE_INDEX            (3)
/* SBC */
#define CSR_BT_AV_SFREQ_AND_CMODE_INDEX       (4)
#define CSR_BT_AV_BLOCKS_SUBBAND_METHOD_INDEX (5)
#define CSR_BT_AV_MIN_BITPOOL_INDEX           (6)
#define CSR_BT_AV_MAX_BITPOOL_INDEX           (7)
/* MPEG 2,4 AAC*/
#define CSR_BT_AV_BIT_RATE_HIGH_IDX          (7)
#define CSR_BT_AV_BIT_RATE_MED_IDX           (8)
#define CSR_BT_AV_BIT_RATE_LOW_IDX           (9)
/* MPEG-D USAC*/
#define AV_MPEG_D_USAC_BIT_RATE_HIGH_IDX     (8)
#define AV_MPEG_D_USAC_BIT_RATE_MED_IDX      (9)
#define AV_MPEG_D_USAC_BIT_RATE_LOW_IDX      (10)

/* Apt-X codec definitions */
#define CSR_BT_AV_APTX_VENDOR_ID1                   (0x0000004F)
#define CSR_BT_AV_APTX_VENDOR_ID2                   (0x0000000A)
#define CSR_BT_AV_APTX_CODEC_ID1                    (0x0001)
#define CSR_BT_AV_APTX_CODEC_ID2                    (0x0002)
#define CSR_BT_AV_APTX_VENDOR_ID_LSB_INDEX          (4)
#define CSR_BT_AV_APTX_CODEC_ID_LSB_INDEX           (8)
#define CSR_BT_AV_APTX_SFREQ_AND_CMODE_INDEX        (0x0A)

#define CSR_BT_AV_HIGH_NIBBLE_MASK                  (0xF0)
#define CSR_BT_AV_LOW_NIBBLE_MASK                   (0x0F)
#define CSR_BT_AV_SUBBANDS_MASK                     (0x0C)
#define CSR_BT_AV_ALLOC_METHOD_MASK                 (0x03)

/* Media type defines - Assigned Numbers */
#define CSR_BT_AV_AUDIO_MEDIA_TYPE       (0x00)
#define CSR_BT_AV_VIDEO_MEDIA_TYPE       (0x01)
#define CSR_BT_AV_MULTI_MEDIA_TYPE       (0x02)

/* Audio codec ID defines - Assigned Numbers */
#define CSR_BT_AV_AUDIO_SBC_CODEC               (0x00)
#define CSR_BT_AV_AUDIO_MPEG_1_2_CODEC          (0x01)
#define CSR_BT_AV_AUDIO_MPEG_2_4_AAC_CODEC      (0x02)
#define AV_AUDIO_MPEG_D_USAC_CODEC              (0x03)
#define CSR_BT_AV_AUDIO_ATRAC_CODEC             (0x04)
#define CSR_BT_AV_AUDIO_NON_A2DP_CODEC          (0x0FF)

/* Video codec ID defines - Assigned Numbers */
#define CSR_BT_AV_VIDEO_H263_BASELINE_CODEC     (0x01)
#define CSR_BT_AV_VIDEO_MPEG4_VSP_CODEC         (0x02)
#define CSR_BT_AV_VIDEO_H263_P3_CODEC           (0x03)
#define CSR_BT_AV_VIDEO_H263_P8_CODEC           (0x04)

/* SBC-related definitions */
/* channel mode */
#define CSR_BT_AV_DUAL_CMODE                    (0x01)
#define CSR_BT_AV_STEREO_CMODE                  (0x02)
#define CSR_BT_AV_JOINT_STEREO_CMODE            (0x04)
#define CSR_BT_AV_MONO_CMODE                    (0x08)

/* SUBBANDS */
#define CSR_BT_AV_4_SUBBANDS                    (0x08)
#define CSR_BT_AV_8_SUBBANDS                    (0x04)

typedef enum
{
    NULL_S,
    READY_S,
    CLEANUP_S
} av_state_t;

typedef enum
{
    DISCONNECTED_S,
    CONNECTING_S,
#ifdef INSTALL_AV_CANCEL_CONNECT
    CANCEL_CONNECTING_S,
#endif
    CONNECTED_S,
    DISCONNECTING_S
} av_con_state_t;

typedef enum
{
    IDLE_S,
    CONFIGURING_S,
    CONFIGURED_S,
    OPENING_S,
    PEER_OPENING_S,
    PEER_OPENING_TO_STREAM_S,
    OPENED_S,
    STARTING_S,
    STREAMING_S,
    CLOSING_S,
    PEER_CLOSING_S,
    ABORT_REQUESTED_S, /* ABORT request sent when media is connected, Waiting for remote's response */
    ABORTING_S,
    PEER_ABORTING_S,
    TERMINATE_PEER_OPENING_S,
    TERMINATE_OPENING_S
} av_stream_state_t;

/* data queue element structure */
typedef struct q_element_tag_t
{
    CsrUint16                    length;
    void                        *data;
    struct q_element_tag_t      *next;
} q_element_t;

typedef struct
{
    CsrUint32                bitRate;                           /* bit rate of the A2DP stream; 0 if unknown */
    CsrBtConnId              mediaCid;                          /* L2CAP channel ID for media transport */
    CsrSchedTid              timerId;                           /* Holds id of running timer (outstanding signalling command)*/
#ifdef CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT
    CsrSchedTid              mediaChannelInfoTimerId;           /* Holds id of 'get l2cap media channel information' timer */
    CsrSchedTid              mediaChannelInformationTimerId;    /* Holds id of 'get l2cap media channel information' timer */
#endif /* CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT */
    l2ca_mtu_t               mediaMtu;                          /* The MTU size of the media channel */
    CsrUint16                seqNo;                             /* Holds the sequence number for stream data */
    CsrUint16                qos_replyCounter;                  /* Count when to send QoS info */
    CsrUint16                delayValue;                        /* Used to compensate delay between audio and video if needed */
    CsrUint16                remoteCid;                         /* remote l2cap media CID */
    CsrUint8                 remoteSeid;                        /* Stream end-point of remote device */
    CsrUint8                 localSeid;                         /* Stream end-point of local device */
#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
    CsrUint16                codecBR;                           /* codec bit rate in kbps */
    CsrUint8                 codecType;                         /* type of codec used for stream*/
    CsrUint8                 codecLocation;                     /* location of the codec - on/off chip */
    CsrUint8                 period;                            /* Identifies the period in ms of codec data being available for transmission. */
    CsrUint8                 samplingFreq;                      /* identifies the sampling frequency of the stream*/
    CsrBool                  startSent;                         /* flag to track if a stream start has been indicated and if an update is required */
    CsrBool                  stopSent;                          /* flag to track if a stream stop has been indicated and if an update is required */
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

#ifndef CSR_STREAMS_ENABLE
    CsrBtCmPrim*             fifoQ[CSR_BT_AV_MEDIA_BUFFER_SIZE];/* Media data queue - array of pointers to CSR_BT_CM_L2CA_DATA_REQ's */
    CsrUint8                 fifoQIn;                           /* Holds the index to end of queue */
    CsrUint8                 fifoQOut;                          /* Holds the index to start of queue */
    CsrUint8                 conId;                             /* Holds the index to the connection information */
    CsrBool                  sending:1;                           /* Flag indicating if L2CAP is busy with transmit (outstanding data req) */
    CsrBool                  sentBufFullInd:1;                    /* Flag indicating if an CSR_BT_AV_QOS_IND(full) has been sent to app. */
    av_stream_state_t        streamState:5;                       /* Stream state */
    CsrUint8                 tLabel;                            /* Holds the transaction label for outstanding signal command */
#else
    CsrUint8                 conId;                             /* Holds the index to the connection information */
    av_stream_state_t        streamState:5;                       /* Stream state */
    CsrUint8                 tLabel;                            /* Holds the transaction label for outstanding signal command */
#endif
} av_stream_info_t;

typedef struct
{
    CsrBtDeviceAddr          remoteDevAddr;              /* Address of remote device */
    q_element_t*             qFirst;                     /* Pointer to first entry in tx queue */
    CsrBtConnId              signalCid;                  /* Signalling L2CAP channel ID */
    CsrUint16                signalMtu;                  /* Signal channel L2CAP MTU size */
    CsrUint16                remoteAVDTPVersion;         /* Needed to decide whther to issue "getCapabilities" or "getAllCapabilities" */
    CsrSchedTid              timerId;                    /* Holds id of running timer (outstanding signalling command)*/
#ifdef CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT
    CsrSchedTid              sigChannelInfoTimerId;      /* Holds id of 'get l2cap signalling channel information' timer */
#endif /* CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT */
    CsrUint16                remoteCid;                  /* remote l2cap signalling CID */
    CsrUint8                 pendingSigProc:4;             /* Holds the pending signalling procedure (discover & get_capa procedures) */
    av_con_state_t           conState:4;                   /* Connection state */
    CsrBool                  incoming:1;                   /* Flag indicating the origin of the connection */
#ifdef CSR_TARGET_PRODUCT_VM
    CsrBool                  useLargeMtu:1;                /* Flag indicating that large mtu to be used for media channel */
#endif /* CSR_TARGET_PRODUCT_VM */
    CsrBool                  sending:1;                    /* Flag indicating if L2CAP is busy with transmit (outstanding data req) */
    CsrUint16                aclHandle;                  /* ACL connection ID */
} av_connection_info_t;

typedef struct
{
    av_stream_info_t        stream[CSR_BT_AV_MAX_NUM_STREAMS]; /* Stream information */
    av_connection_info_t    con[CSR_BT_AV_MAX_NUM_CONNECTIONS];/* Connection information */
    void*                   recvMsgP;                   /* Holds the received primitive currently being handled */
    void*                   sdpAvSearchData;            /* Instance data for the common SDP search lib */
    CsrMessageQueueType*    saveQueue;                  /* Save queue, used to postpone primitive handling */
    fragmentHead_t*         fragmentHeadPtr;            /* Queue of received signal message fragments to be defragmented */
    CsrUint32               serviceRecordHandles[4];    /* Holds the handle to a registered service record for each role */
    CsrSchedQid             appHandle;                  /* Control application handle */
#ifdef INSTALL_AV_STREAM_DATA_APP_SUPPORT
    CsrSchedQid             streamAppHandle;            /* Data application handle */
#endif
    CsrUint16               localVersion;               /* Local AVDTP supported version */
#ifndef CSR_STREAMS_ENABLE
    CsrUint16               l2capCtx;                   /* L2CAP data context */
    CsrUint16               qosInterval;
#endif

#ifdef INSTALL_AV_CUSTOM_SECURITY_SETTINGS
    dm_security_level_t     secIncoming;                /* Incoming security level */
    dm_security_level_t     secOutgoing;                /* Outgoing security level */
#endif /* INSTALL_AV_CUSTOM_SECURITY_SETTINGS */

    CsrUint8                roleRegister[4];            /* Holds the number of activations for each role */
    av_state_t              state:3;                      /* AV main (instance) state */
    CsrBtAvRole             role:2;                       /* The role currently being registered/searched; used for SDP record registration & de-registration */
    CsrBool                 restoreFlag:1;                /* Flag indicating if primitives should be pop'ed off the save q */
#ifdef INSTALL_AV_DEACTIVATE
    CsrBool                 doDeactivate:1;               /* Flag indicating if deactivation is ongoing */
#endif
    CsrBool                 searchOngoing:1;             /* Marks if the SDP search is ongoing or not. */
    CsrBool                 isConnectable:1;             /* Flag indicating if there is an outstanding connect accept */
} av_instData_t;


typedef void (* avStateHandlerType)(av_instData_t * instData);

#define AV_NUMBER_OF_STATES                 (CLEANUP_S + 1 - NULL_S)
#define NUMBER_OF_AV_DOWNSTREAM_MESSAGES    (CSR_BT_AV_PRIM_DOWNSTREAM_COUNT)

#define CSR_BT_AV_STREAM_CONTEXT(_s)         (CSR_BT_CM_CONTEXT_UNUSED + 1 + (_s))

void CsrBtAvMessagePut(CsrSchedQid phandle, void *msg);

#ifdef INSTALL_AV_STREAM_DATA_APP_SUPPORT
#define PUT_PRIM_TO_APP(prim) \
    CsrBtAvMessagePut((CsrSchedQid)((instData->streamAppHandle != CSR_SCHED_QID_INVALID) ? instData->streamAppHandle : instData->appHandle), prim);
#else
#define PUT_PRIM_TO_APP(prim) \
    CsrBtAvMessagePut((CsrSchedQid)(instData->appHandle), prim);
#endif /* INSTALL_AV_STREAM_DATA_APP_SUPPORT */

#ifdef CSR_BT_GLOBAL_INSTANCE
extern av_instData_t csrBtAvInstData;
#endif

/* prototypes for csr_bt_av_main.c */
void CsrBtAvInit(void **gash);
void CsrBtAvHandler(void **gash);

/* prototypes for csr_bt_av_con.c */
void CsrBtAvConnectReqHandler( av_instData_t *instData);
#ifdef INSTALL_AV_CANCEL_CONNECT
void CsrBtAvCancelConnectReqHandler( av_instData_t *instData);
void CsrBtAvCancelConnectReqNullStateHandler( av_instData_t *instData);
#else
#define CsrBtAvCancelConnectReqHandler           NULL
#define CsrBtAvCancelConnectReqNullStateHandler  NULL
#endif /* INSTALL_AV_CANCEL_CONNECT */
void CsrBtAvDisconnectReqHandler( av_instData_t *instData);
void CsrBtAvCmL2caConnectCfmHandler( av_instData_t *instData);
void CsrBtAvCmL2caConnectAcceptCfmHandler( av_instData_t *instData);
void CsrBtAvCmL2caCancelConnectAcceptCfmHandler( av_instData_t *instData);
void CsrBtAvCmL2caDisconnectIndHandler( av_instData_t *instData);
void CsrBtAvCmL2caConnectCfmHandlerCleanup( av_instData_t *instData);
void CsrBtAvCmL2caConnectAcceptCfmHandlerCleanup( av_instData_t *instData);
void CsrBtAvCmL2caCancelConnectAcceptCfmHandlerCleanup( av_instData_t *instData);
void CsrBtAvCmL2caDisconnectIndHandlerCleanup( av_instData_t *instData);

#ifdef INSTALL_AV_CUSTOM_SECURITY_SETTINGS
void CsrBtAvSecurityInHandler(av_instData_t *instData);
void CsrBtAvSecurityOutHandler(av_instData_t *instData);
#else

#define CsrBtAvSecurityInHandler                NULL
#define CsrBtAvSecurityOutHandler               NULL

#endif

void CsrBtAvCmL2caChannelInfoCfmHandler(av_instData_t *instData);

/* prototypes for csr_bt_av_register.c */
void CsrBtAvActivateReqHandler( av_instData_t *instData);
#ifdef INSTALL_AV_DEACTIVATE
void CsrBtAvDeactivateReqHandler( av_instData_t *instData);
#else
#define CsrBtAvDeactivateReqHandler  NULL
#endif /* INSTALL_AV_DEACTIVATE */
void CsrBtAvCmSdsRegisterCfmHandler( av_instData_t *instData);
void CsrBtAvCmSdsUnregisterCfmHandler( av_instData_t *instData);
void CsrBtAvCmL2caRegisterCfmHandler(av_instData_t *instData);
void CsrBtAvCmSdsRegisterCfmHandlerCleanup( av_instData_t *instData);
void CsrBtAvCmSdsUnregisterCfmHandlerCleanup( av_instData_t *instData);

/* prototypes for csr_bt_av_signal.c */
void CsrBtAvDiscoverReqHandler(av_instData_t *instData);
void CsrBtAvGetCapabilitiesReqHandler(av_instData_t *instData);
void CsrBtAvSetConfigurationReqHandler(av_instData_t *instData);
#ifdef INSTALL_AV_GET_CONFIGURATION
void CsrBtAvGetConfigurationReqHandler(av_instData_t *instData);
#else
#define CsrBtAvGetConfigurationReqHandler    NULL
#endif /* INSTALL_AV_GET_CONFIGURATION */
void CsrBtAvReconfigureReqHandler(av_instData_t *instData);
void CsrBtAvOpenReqHandler(av_instData_t *instData);
void CsrBtAvStartReqHandler(av_instData_t *instData);
void CsrBtAvCloseReqHandler(av_instData_t *instData);
void CsrBtAvIgnoreReqHandler(av_instData_t *instData);
void CsrBtAvSuspendReqHandler(av_instData_t *instData);
void CsrBtAvAbortReqHandler(av_instData_t *instData);
#ifdef INSTALL_AV_SECURITY_CONTROL
void CsrBtAvSecurityReqHandler(av_instData_t *instData);
void CsrBtAvSecurityResHandler(av_instData_t *instData);
void CsrBtAvSecurityReqFree(av_instData_t *instData);
#else
#define CsrBtAvSecurityReqHandler    NULL
#define CsrBtAvSecurityResHandler    NULL
#define CsrBtAvSecurityReqFree       NULL
#endif /* INSTALL_AV_SECURITY_CONTROL */

#ifndef CSR_STREAMS_ENABLE
void CsrBtAvStreamDataReqHandler(av_instData_t *instData);
void CsrBtAvStreamDataReqFree(av_instData_t *instData);
#endif
void CsrBtAvDiscoverResHandler(av_instData_t *instData);
void CsrBtAvGetCapabilitiesResHandler(av_instData_t *instData);
void CsrBtAvGetCapabilitiesResFree(av_instData_t *instData);
void CsrBtAvSetConfigurationResHandler(av_instData_t *instData);
void CsrBtAvGetConfigurationResHandler(av_instData_t *instData);
void CsrBtAvGetConfigurationResFree(av_instData_t *instData);
void CsrBtAvReconfigureResHandler(av_instData_t *instData);
void CsrBtAvOpenResHandler(av_instData_t *instData);
void CsrBtAvStartResHandler(av_instData_t *instData);
void CsrBtAvCloseResHandler(av_instData_t *instData);
void CsrBtAvSuspendResHandler(av_instData_t *instData);
void CsrBtAvAbortResHandler(av_instData_t *instData);
void CsrBtAvDelayReportResHandler(av_instData_t *instData);
void CsrBtAvSetStreamInfoReqHandler(av_instData_t *instData);
void CsrBtAvSecurityResFree(av_instData_t *instData);
void CsrBtAvCmL2caDataIndHandler(av_instData_t *instData);
void CsrBtAvCmL2caDataIndHandlerCleanup(av_instData_t *instData);
void CsrBtAvCmL2caDataCfmHandler(av_instData_t *instData);

#ifdef INSTALL_AV_STREAM_DATA_APP_SUPPORT
void CsrBtAvRegisterStreamHandleReqHandler( av_instData_t *instData);
void CsrBtAvRegisterStreamHandleCfmSend(CsrSchedQid handle);
#else
#define CsrBtAvRegisterStreamHandleReqHandler    NULL
#endif /* INSTALL_AV_STREAM_DATA_APP_SUPPORT */

#ifdef CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT
void CsrBtAvGetChannelInfoReqHandler(av_instData_t *instData);
void CsrBtAvGetStreamChannelInfoReqHandler(av_instData_t *instData);
void CsrBtAvGetMediaChannelInfoReqHandler(av_instData_t *instData);
#else
#define CsrBtAvGetChannelInfoReqHandler       NULL
#define CsrBtAvGetStreamChannelInfoReqHandler NULL
#define CsrBtAvGetMediaChannelInfoReqHandler  NULL
#endif /* CSR_BT_INSTALL_AV_CHANNEL_INFO_SUPPORT */

/* prototypes for csr_bt_av_util.c */
void CsrBtAvClearConnection(av_instData_t *instData, CsrUint8 connectionId);
#ifndef CSR_STREAMS_ENABLE
void CsrBtAvClearStreamQ(av_stream_info_t *stream);
#endif
CsrBool CsrBtAvStopStreamTimer(av_instData_t *instData, CsrUint8 s);
void CsrBtAvClearStream(av_instData_t *instData, CsrUint8 s);
void CsrBtAvIsCleanUpFinished(av_instData_t *instData);
void CsrBtAvSendHouseCleaning(av_instData_t *instData);
void CsrBtAvSaveMessage(av_instData_t *instData);
void AvSendPendingUpstreamMessages(av_instData_t *instData, CsrBtAvPrim primType);

CsrUint8 getNumActivations(CsrUint8 *roleRegister);
CsrUint8 getNumIncomingCon(av_instData_t *instData);
void CsrBtAvMakeConnectable(av_instData_t *instData);
void CsrBtAvActivateCfmSend(CsrSchedQid handle,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);
#ifdef INSTALL_AV_DEACTIVATE
void CsrBtAvDeactivateCfmSend(CsrSchedQid handle,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);
#endif
void CsrBtAvStatusIndSend(av_instData_t *instData, CsrBtAvPrim signalId, CsrBtAvRole role, CsrUint8 conId);

void CsrBtAvConnectCfmSend(CsrSchedQid handle,
                           CsrUint8 connectionId,
                           CsrBtDeviceAddr devAddr,
                           CsrBtResultCode resultCode,
                           CsrBtSupplier resultSupplier,
                           CsrBtConnId btConnId);

void CsrBtAvConnectIndSend(CsrSchedQid handle,
                           CsrUint8 connectionId,
                           CsrBtDeviceAddr devAddr,
                           CsrBtConnId btConnId);

void CsrBtAvDisconnectIndSend(CsrSchedQid handle, CsrUint8 connectionId,
                              CsrBool localTerminated,
                              CsrBtReasonCode reasonCode,
                              CsrBtSupplier reasonSupplier);

void CsrBtAvStreamMtuSizeIndSend(av_instData_t *instData, CsrUint8 shandle, CsrBtConnId btConnId);

void CsrBtAvDiscoverCfmSendReject(av_instData_t *instData, CsrUint8 conId, CsrUint8 label,
    CsrBtResultCode avResultCode,
    CsrBtSupplier avResultSupplier);

void CsrBtAvGetCapabilitiesCfmSendReject(av_instData_t *instData, CsrUint8 conId, CsrUint8 label,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);

void CsrBtAvSetConfigurationCfmSend(av_instData_t *instData, CsrUint8 conId, CsrUint8 label, CsrUint8 shandle, CsrUint8 servCategory,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);

#ifdef INSTALL_AV_GET_CONFIGURATION
void CsrBtAvGetConfigurationCfmSend(av_instData_t *instData, CsrUint8 label, CsrUint8 shandle, CsrUint16 servCapLen, CsrUint8 *servCapData,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);
#endif

void CsrBtAvReconfigureCfmSend(av_instData_t *instData, CsrUint8 label, CsrUint8 shandle, CsrUint8 servCategory,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);

void CsrBtAvOpenCfmSend(av_instData_t *instData, CsrUint8 shandle, CsrUint8 label,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);

void CsrBtAvStartCfmSend(av_instData_t *instData,
    CsrUint8 shandle,
    CsrUint8 label,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);

void CsrBtAvCloseCfmSend(av_instData_t *instData,
    CsrUint8 shandle,
    CsrUint8 label,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);

void AvCloseIndSend(av_instData_t *instData, CsrUint8 shandle, CsrUint8 label);

void CsrBtAvSuspendCfmSend(av_instData_t *instData, CsrUint8 shandle, CsrUint8 label,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);

void CsrBtAvAbortCfmSend(av_instData_t *instData, CsrUint8 shandle, CsrUint8 label);

#ifdef INSTALL_AV_SECURITY_CONTROL
void CsrBtAvSecurityControlCfmSend(av_instData_t *instData, CsrUint8 shandle, CsrUint8 label, CsrUint16 length, CsrUint8 *data,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);
#endif

void CsrBtAvDelayReportCfmSend(av_instData_t *instData,
                                CsrUint8 shandle,
                                CsrUint8 label,
                                CsrBtResultCode resultCode,
                                CsrBtSupplier resultSupplier);
#ifndef CSR_STREAMS_ENABLE
void CsrBtAvQosIndSend(av_instData_t *instData, CsrUint8 shandle, CsrUint16 bufferStatus);
#endif


#ifdef INSTALL_AV_CUSTOM_SECURITY_SETTINGS
void CsrBtAvSecurityInCfmSend(CsrSchedQid appHandle,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);
void CsrBtAvSecurityOutCfmSend(CsrSchedQid appHandle,
    CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);
#endif

#ifdef CSR_BT_INSTALL_AV_SET_QOS_INTERVAL
void CsrBtAvSetQosIntervalReqHandler(av_instData_t *instData);
#endif
void CsrBtAvDelayReportReqHandler(av_instData_t *instData);
void CsrBtAvSdcStartRemoteVersionSearch(av_instData_t *instData, CsrBtDeviceAddr deviceAddr);
void CsrBtAvSdcResultHandler(void                     * inst,
                             CmnCsrBtLinkedListStruct * sdpTagList,
                             CsrBtDeviceAddr          deviceAddr,
                             CsrBtResultCode          resultCode,
                             CsrBtSupplier            resultSupplier);
CsrUint32 csrBtAvCalculateStreamBitRate(av_instData_t *instData, CsrUint8 *servCap, CsrUint16 servCapLen, CsrUint8 strIdx);
/* Prototypes from av_free_down.c */
void CsrBtAvFreeDownstreamMessageContents(CsrUint16 eventClass, void * message);

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
void CsrBtAvStreamStartStopIndSend(av_instData_t* instData, CsrUint8 sHandle, CsrBool start);
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

CsrUint16 CsrBtAvGetMtuSize(CsrUint8 conId);

#ifdef __cplusplus
}
#endif

#endif
