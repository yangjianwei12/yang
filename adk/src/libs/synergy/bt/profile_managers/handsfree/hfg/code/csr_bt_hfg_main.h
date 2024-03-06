#ifndef CSR_BT_HFG_MAIN_H__
#define CSR_BT_HFG_MAIN_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "csr_synergy.h"

#include "csr_bt_cm_private_lib.h"
#include "csr_message_queue.h"
#include "csr_bt_hfg_prim.h"
#include "csr_bt_hfg_consts.h"

#include "csr_bt_cmn_sdc_rfc_util.h"
#include "csr_bt_cmn_sdr_tagbased_lib.h"
#include "csr_bt_cmn_sdp_lib.h"

#include "csr_log_text_2.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef CSR_BT_AT_COMMAND_MAX_LENGTH
#define CSR_BT_AT_COMMAND_MAX_LENGTH        180
#endif
/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtHfgLto);

#define CSR_BT_HFG_NUMBER_AT_CMD      6
#define CSR_BT_HFG_NUMBER_SCO_TRIALS  5  /* Number of failed incoming SCO connections before negotiation start against "old" HF */

#define HFG_REMOTE_HF_INDICATOR_ADD_LAST(listPtr)                                                                                 \
  ((CsrBtHfgRemoteHFIndicator *) CsrCmnListElementAddLast(listPtr, sizeof(CsrBtHfgRemoteHFIndicator)))
#define HFG_REMOTE_HF_INDICATOR_GET_FIRST(listPtr)                                                                                \
  ((CsrBtHfgRemoteHFIndicator *) CsrCmnListGetFirst(listPtr))
#define HFG_REMOTE_HF_INDICATOR_GET_FROM_IND_ID(listPtr, indId)                                                                   \
  ((CsrBtHfgRemoteHFIndicator *) CsrCmnListSearchOffsetUint16(listPtr, offsetof(CsrBtHfgRemoteHFIndicator, hfHfIndicator), indId))

    /* Main profile instance state */
typedef enum
{
    MainNull_s = 0,                                                 /* Register server channels */
    MainIdle_s,                                                     /* Not activated */
    MainActive_s,                                                   /* Activated, use 2nd-level jumptable */
    MainDeactivate_s,                                               /* Closing down */
    MainNum_s
} HfgMainState_t;

    /* Connection instance states */
typedef enum
{
    Activate_s = 0,                                                 /* Accepting new connection (record+accept) */
    Connect_s,                                                      /* Outgoing connection setup in progress (record) */
    ServiceSearch_s,                                                /* SDC in progress (record) */
    Connected_s,                                                    /* SLC established (-) */
    ConnectionNum_s,                                                /* Dummy (-) */
    Unused_s                                                        /* Not in use, outside jump-tables (-) */
} HfgConnectionState_t;

/* Substates for AT sequence */
typedef enum
{
    At0Idle_s = 0,                                                  /* AT sequence not yet started */
    At1Brsf_s,                                                      /* AT+BRSF   */
    At2Bac_s,                                                       /* AT+BAC=   */
    At3CindQuestion_s,                                              /* AT+CIND=? */
    At4CindStatus_s,                                                /* AT+CIND?  */
    At5Cmer_s,                                                      /* AT+CMER=  */
    At6ChldQuery_s,                                                 /* AT+CHLD=? */
    At7BindSupport_s,                                               /* AT+BIND=  */
    At8BindQuery_s,                                                 /* AT+BIND=? */
    At9BindStatus_s,                                                /* AT+BIND?  */
    At10End_s                                                       /* SLC Completed */
} HfgAtState_t;


/* SCO audio settings parameters */
typedef struct
{
    CsrUint32                    txBandwidth;
    CsrUint32                    rxBandwidth;
    CsrUint16                    maxLatency;
    CsrUint16                    voiceSettings;
    hci_pkt_type_t               audioQuality;
    CsrUint8                     reTxEffort;
    CsrUint8                     scoParmType;                        /* Internal "name" for this set of parameters, e.g SCO_DEFAULT_1P1 */
} HfgAudioParams_t;

/* Service record info placeholder */
typedef struct
{
    CsrUint8                     chan;                               /* Server channel */
    CsrBtHfgConnection           type;                               /* If accepting, this is the connection type */
    CsrBool                      registered;                         /* SDS has been registered */
    CsrBool                      accepting;                          /* Accepting incoming connections */
    CsrBtUuid32                  recHandle;                        /* Service record handle */
} HfgService_t;

/* Service search placeholder */
typedef struct
{
    CsrBtUuid32                  recordId;                           /* Service Record Id */
    CsrUint16                    num;                                /* Number of elements */
} HfgSearch_t;

/* Indicator and setting placeholder */
typedef struct
{
    CsrUint8                     ciev[CSR_BT_CIEV_NUMBER_OF_INDICATORS];    /* CIEV/CIND status indicators */
    CsrUint8                     bia[CSR_BT_CIEV_NUMBER_OF_INDICATORS];     /* BIA activation of ciev/cind update sending */
    CsrUint8                     other[CSR_BT_HFG_NUM_OF_SETTINGS];         /* special non-indicator persistent settings */
} HfgIndicators_t;

typedef struct CsrBtHfgRemoteHFIndicatorSt
{
    struct CsrBtHfgRemoteHFIndicatorSt  *next;
    struct CsrBtHfgRemoteHFIndicatorSt  *prev;
    CsrBtHfpHfIndicatorId               hfHfIndicator;
    CsrUint16                           indvalue;
    CsrBtHfpHfIndicatorStatus           indStatus;
    CsrUint16                           valueMax;
    CsrUint16                           valueMin;
}CsrBtHfgRemoteHFIndicator;

/* Connection instance */
typedef struct HfgInstance_tTag
{
    void                         *main;                              /* Main instance data pointer */
    void                         *msg;                               /* Current message */
    CsrUint16                    msgClass;                           /* Current message class */
    CsrUint8                     index;                              /* My local index in main->linkData */
    CsrUint32                    state;                              /* Connection state */
    CsrUint32                    oldState;                           /* Old connection state */
    CsrUint32                    atState;                            /* Sub-state for AT startup sequence (service search) */
    CsrBtHfgConnection           linkType;                           /* Type of peer connection */
    CsrBtHfgConnectionId         hfgConnId;                          /* Connection ID */
    CsrUint8                     serverChannel;                      /* Server channel */
    CsrUint16                    maxFrameSize;                       /* RFCOMM CSRMAX frame size */
    CsrBtDeviceAddr              address;                            /* Peer address */
    CsrUint32                    remSupFeatures;                     /* Remote peer supported features bitmask */
    CsrBtDeviceName              serviceName;                        /* Peer service name (not friendly name) */
    HfgIndicators_t              ind;                                /* Indicators etc. */
    CsrUint16                    scoHandle;                          /* SCO connection handle */
    CsrBool                      scoWantedState;                     /* Target link state (connected/disconnected) */
    HfgAudioParams_t             scoParams;                          /* User-defined (e)SCO parameters */
    CsrUint8                     scoPcmSlot;                         /* PCM slot for outgoing SCO connection */
    CsrBool                      scoPcmRealloc;                      /* PCM slot reallocation for outgoing SCO connection */

    CsrBool                      pendingCodecNegotiation;            /* Has the HFg started codec negotiation */
    CsrBtResultCode              searchConnectResult;                /* Connect confirm result for SDC resources */
    HfgSearch_t                  searchHfg;                          /* Service search for HFG */
    HfgSearch_t                  searchAg;                           /* Service search for HFG */
    CsrUint16                    remoteVersion;                      /* Only relevant for HFG */
    CsrBool                      restoreFlag;                        /* Save queue holds items */
    CsrMessageQueueType          *saveQueue;                         /* Local save queue */
    CsrBool                      cmTxReady;                          /* CM ready for next AT command */
    CsrMessageQueueType          *cmTxQueue;                         /* Outgoing AT command queue */
    CsrUint8                     *atCmd;                             /* Partial incoming AT command */
    CsrUint16                    atLen;                              /* Length of partial AT */
    CsrSchedTid                  atSlcTimer;                        /* Timer for missing CHLD or BIND event */

    CsrBool                      pendingSearch;                      /* SDS search in progress */
    CsrBool                      pendingDisconnect;                  /* Disconnect req from app received */
    CsrBool                      pendingPeerDisconnect;              /* Disconnect req from peer received */
    CsrBool                      pendingCancel;                      /* Cancel accept connection received */
    CsrBool                      pendingCancelOutgoingConn;          /* Cancel pending outgoing connection (if SDP is pending it will be cancelled too) */    
    CsrBool                      pendingSco;                         /* SCO connection is being set up */
    CsrBool                      pendingScoConfirm;                  /* SCO connection is being set up because it is requested by the app */
    CsrBool                      pendingScoDisconnect;               /* App has request SCO to be disconnected */
    CsrBool                      searchAndCon;                       /* SDS search and connection request in progress */
    CsrSchedTid                  ringTimer;                          /* Ring timer */
    CsrBtHfgRingReq              *ringMsg;                           /* Ring message data */
    CsrSchedTid                  deRegisterTimer;                    /* Service record de-register timer */
    CsrSchedTid                  conGuardTimer;                      /* Conection guard timer */
    CsrSchedTid                  atResponseTimer;                    /* AT response timer */    
    CsrUint8                     waitForDataCfm;                     /* counter to keep track of the data messages needed to ensure the data has been sent */
    CsrUint32                    remSupportedCodecs;                 /* Mask of codecs supported by the remote device */
    CsrUint8                     lastCodecUsed;                      /* Id of the last codec used during an audio connection to the remote device */ 
    CsrCmnList_t                 remoteHfIndicatorList;              /* List to contain connection instance specific HF Indicator Information*/
    CsrUint16                    selectedQceCodecId;
} HfgInstance_t;

typedef void (HfgCallBackFunction)(struct HfgInstance_tTag *inst);

/* Main (common) instance */
typedef struct HfgMainInstance_tTag
{
    HfgInstance_t               linkData[CSR_BT_HFG_NUM_SERVERS];    /* Connection instances */
    CsrSchedQid                 appHandle;                           /* App handle */
    CsrUint32                   state;                               /* Current global state */
    CsrUint32                   oldState;                            /* Old global state */
    void                        *msg;                                /* Current message */
    CsrUint16                   msgClass;                           /* Current message class */
    CsrUint32                   locSupFeatures;                     /* Specification local supported features bitmask */
    CsrUint32                   callConfig;                         /* CHLD/BTRH setup bitmask */
    CsrUint32                   hfgConfig;                          /* Profile setup bitmask */
    CsrBtDeviceName             localName;                          /* Local service name for HFG SDS */
    CsrBtHfgAtParserMode        atParser;                           /* AT parser mode (full, semi, none) */
    CsrUint8                    maxConnections;                     /* Max. number of concurrent connections */
    CsrUint8                    supportedCodecs;                    /* Supported CODECs bitmask */

    HfgIndicators_t             ind;                                /* Global default indicators etc. */
    CsrBool                     restoreFlag;                        /* Pop from global save queue */
    CsrMessageQueueType         *saveQueue;                         /* Global save queue */

    HfgService_t                 service[CSR_BT_HFG_NUM_CHANNELS];          /* Service records and server channels */
    CsrUint8                     activeService;                      /* Service index currently being manipulated */
    CsrUint8                     activeConnection;                   /* Connection currently being manipulated */
#ifdef INSTALL_HFG_CUSTOM_SECURITY_SETTINGS
    dm_security_level_t          secIncoming;                        /* Incoming security level */
    dm_security_level_t          secOutgoing;                        /* Outgoing security level */
#endif /* INSTALL_HFG_CUSTOM_SECURITY_SETTINGS*/
    /* Common SDP search stuff */
    void                        *sdpHfgSearchConData;               /* Instance data for the common SDP search lib */
    void                        *sdpHfgSearchData;                  /* Instance data for the common SDP search lib */

    CsrUint8                     HfgSendAtToApp[CSR_BT_HFG_NUMBER_AT_CMD];  /* Defined to 6 even though 5 bytes are enough so far... */
    CsrUint8                     deRegisterTime;                     /* Service record de-register time interval */
    CsrBool                      initSequenceDone;
    CsrUint8                     modemStatus;
    CsrBtHfgHfIndicator          *localHfIndicatorList;              /* Array of local supported HF Indicator */
    CsrUint16                    indCount;                           /* Number of HF indicators in localHfIndicatorList */
} HfgMainInstance_t;



#ifdef CSR_BT_GLOBAL_INSTANCE
extern HfgMainInstance_t csrBtHfgInstance;
#endif

/* Prototypes for top, connection and at handlers */
typedef void (*HfgMainHandler_t)(HfgMainInstance_t *inst);
typedef void (*HfgConnHandler_t)(HfgInstance_t *inst);
typedef void (*HfgAtHandler_t)(HfgInstance_t *inst, CsrUint16 *index, CsrBool seq);

void CsrBtHfgSdcResultHandler(void               * inst,
                        CmnCsrBtLinkedListStruct * sdpTagList,
                        CsrBtDeviceAddr          deviceAddr,
                        CsrBtResultCode          resultCode,
                        CsrBtSupplier      resultSupplier);

void CsrBtHfgRfcSdcConResultHandler(void                  *inst,
                              CsrUint8               localServerCh,
                              CsrUint32                    hfgConnId,
                              CsrBtDeviceAddr                deviceAddr,
                              CsrUint16                    maxFrameSize,
                              CsrBool                      validPortPar,
                              RFC_PORTNEG_VALUES_T        portPar,
                              CsrBtResultCode             resultCode,
                              CsrBtSupplier         resultSupplier,
                              CmnCsrBtLinkedListStruct    *sdpTagList);

void CsrBtHfgSdcSelectServiceHandler(void                    * instData,
                               void                    * cmSdcRfcInstData,
                               CsrBtDeviceAddr            deviceAddr,
                               CsrUint8           serverChannel,
                               CsrUint16                entriesInSdpTaglist,
                               CmnCsrBtLinkedListStruct * sdpTagList);


#if defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_STATE_TRANSITION)
void HfgChangeState(const CsrCharString *file, CsrUint32 lineno, 
                    CsrUint32 *state, const CsrCharString *stateName, 
                    CsrUint32 newState, const CsrCharString *newStateName);
#define HFG_CHANGE_STATE(state, newState) HfgChangeState(__FILE__, __LINE__, &(state), #state, (newState), #newState)
#else
#define HFG_CHANGE_STATE(state, newState) (state = newState)
#endif

#ifdef __cplusplus
}
#endif

#endif
