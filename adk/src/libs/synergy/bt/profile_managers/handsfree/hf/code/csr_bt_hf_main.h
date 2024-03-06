#ifndef CSR_BT_HF_MAIN_H__
#define CSR_BT_HF_MAIN_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "csr_synergy.h"

#include "csr_message_queue.h"
#include "csr_bt_hf_prim.h"
#include "csr_bt_hf_consts.h"
#include "csr_bt_cmn_sdc_rfc_util.h"
#include "csr_bt_cmn_sdr_tagbased_lib.h"
#include "csr_bt_cmn_sdp_lib.h"
#include "csr_log_text_2.h"

#ifdef __cplusplus
extern "C" {
#endif

CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtHfLto);

/* HF / HS Main states */
#define Null_s                                  0x00
#define Activated_s                             0x01
#define Deactivate_s                            0x02
#define HfMainNumberOfStates                    0x03
#define HfMainNumberOfUpstreamStates            0x03

/* HF / HS sub states */
#define Activate_s                              0x00
#define Connect_s                               0x01
#define Connected_s                             0x02
#define ServiceSearch_s                         0x03
#define HfNumberOfStates                        0x04
#define HsNumberOfStates                        (HfNumberOfStates - 1)

#define REMOTE_HF_INDICATOR_ADD_LAST(listPtr)                    ((CsrBtHfRemoteHfIndicator *)CsrCmnListElementAddLast(listPtr, sizeof(CsrBtHfRemoteHfIndicator)))
#define REMOTE_HF_INDICATOR_GET_FIRST(listPtr)                   ((CsrBtHfRemoteHfIndicator *)CsrCmnListGetFirst(listPtr))
#define REMOTE_HF_INDICATOR_GET_FROM_IND_ID(listPtr, indId)      ((CsrBtHfRemoteHfIndicator *)CsrCmnListSearchOffsetUint8(listPtr, offsetof(CsrBtHfRemoteHfIndicator, agHfIndicator), indId))

/* AT + CIND declarations */
#define CIND_INDEX_CALL         1
#define CIND_INDEX_CALLSETUP    2
#define CIND_INDEX_SERVICE      3
#define CIND_INDEX_SIGNAL       4
#define CIND_INDEX_ROAM         5
#define CIND_INDEX_BATTCHG      6
#define CIND_INDEX_CALLHELD     7
#define CIND_INDEX_INVALID      CIND_INDEX_CALLHELD + 1

#define CIND_ENCODED_DATA_SIZE  7

/* CIND strings */
#define CIND_CALL       "(\"call\""
#define CIND_CALLSETUP  "(\"callsetup\""
#define CIND_SERVICE    "(\"service\""
#define CIND_SIGNAL     "(\"signal\""
#define CIND_ROAM       "(\"roam\""
#define CIND_BATTCHG    "(\"battchg\""
#define CIND_CALLHELD   "(\"callheld\""

/* Fixed length contains length of ,(0-1)), */
#define CIND_FIXED_LEN  8

/* Max length string i.e. max length from the above mentioned strings,
 * at the time of writing this is callsetup, hence this is length of CIND_CALLSETUP.
 */
#define CIND_MAX_STRING_MAX_LEN     12

/* Qualcomm Codec Extension is not supported */
#define CSR_BT_HF_QCE_UNSUPPORTED       0xFFFF
#define CSR_BT_HF_QCE_UNSUPPORTED_LEN   6

/* Maximum size of CHLD response string(+CHLD:<Values>) */
/* To store CHLD values: ['(' '0' ',' '1' ',' '1' 'x' ',' '2' ',' '2' 'x' ',' '3' ',' '4' ')'] */
#define HF_CHLD_MAX_STR_SIZE          18

typedef struct
{
    CsrBtHfPrim             type;
    CsrBtHfConnectionId     connectionId;
    CsrBtCmeeResultCode     cmeeResultCode;
} HF_GENERAL_CFM_T;

typedef enum
{
    supportFeatures,
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
    qceSupport,
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
    codecSupport,
    cindSupport,
    cindStatus,
    eventReport,
    callHold,
    hfIndSupport,
    hfIndQuery,
    enableHfIndQuery,
    serviceLevel,
    copsQuery,
    rest
} HfAtStates_t;

typedef enum
{
    sdcState_s,
    btConnect_s,
    serviceConnect_s
} HfServiceState_t;

typedef enum HfLastAtCmdSent
{
  idleCmd = 0, /* ready to send a new command; no confirm message pending */
  vgm,
  vgs,
  brsf,
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
  qac,
  qcs,
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
  cindSupportCmd,
  cindStatusCmd,
  cmer,
  chldSupport,
  chld,
  ccwa,
  clip,
  btrh,
  clcc,
  cnum,
  copsQueryCmd,
  copsSet,
  cmee,
  bvra,
  binp,
  answer,
  callEnd,
  ckpd,
  dialNumber,
  dialMem,
  redial,
  nrec,
  vts,
  bia,
  bcs,
  bac,
  bcc,
  bindSet,
  bindTest,
  bindRead,
  bievSet,
  other  /* direct AT command from the application (CSR_BT_HF_AT_CMD_REQ); send the answer directly to the app (CSR_BT_HF_AT_CMD_IND) */
} HfLastAtCmdSent_t;

typedef CsrUint8 HfStates_t;

typedef struct
{
    CsrMessageQueueType                  *cmDataReqQueue;         /* queue for data messages to cm */
    CsrUint8                             *recvAtCmd;              /* collecting a complete AT-command line */
    CsrSchedMsgId                        atResponseTimerId;
    CsrUint16                            recvAtCmdSize;           /* number of char's currently received when */
    CsrUint16                            maxRfcFrameSize;
    CsrBool                              allowed2SendCmData;      /* send only one packet at a time and wait for cfm */
    CsrBool                              dataReceivedInConnected;
} HfHsData_t;

typedef struct
{
    CsrUint32                            theTxBandwidth;
    CsrUint32                            theRxBandwidth;
    hci_pkt_type_t                       theAudioQuality;
    CsrUint16                            theMaxLatency;
    CsrUint16                            theVoiceSettings;
    CsrUint8                             theReTxEffort;
} audioSetupParams_t;

/* Service record info placeholder */
typedef struct
{
    CsrUint8                     chan;                               /* Server channel */
    CsrBtHfConnectionType        type;                               /* If accepting, this is the connection type */
    CsrBool                      registered;                         /* SDS has been registered */
    CsrBool                      accepting;                          /* Accepting incoming connections */
    CsrBtUuid32                  recHandle;                          /* Service record handle */
} HfService_t;

struct HfMainInstanceData_tTag;
typedef void (HfCallBackFunction)(struct HfMainInstanceData_tTag* inst);

typedef struct CsrBtHfRemoteHfIndicatorSt
{
    struct CsrBtHfRemoteHfIndicatorSt  *next;
    struct CsrBtHfRemoteHfIndicatorSt  *prev;
    CsrUint16                          indvalue;
    CsrBtHfpHfIndicatorId              agHfIndicator;
    CsrBtHfpHfIndicatorStatus          indStatus:1;
    CsrBool                            validVal:1;
}CsrBtHfRemoteHfIndicator;

typedef struct
{
    CsrUint8 index;
    CsrUint8 range_start: 4;
    CsrUint8 range_end: 4;
} hfAgCindSupportIndInstance_t;

typedef struct
{
    CsrUint8 instCount;
    hfAgCindSupportIndInstance_t cindEncodedData[CIND_ENCODED_DATA_SIZE];
} hfAgCindSupportInd_t;

typedef struct
{
    hfAgCindSupportInd_t  cindData;
    CsrUint8              cindStartValueString[16];
    CsrUint8              chldStringStored[HF_CHLD_MAX_STR_SIZE]; /* To store CHLD values: ['(' '0' ',' '1' ',' '1' 'x' ',' '2' ',' '2' 'x' ',' '3' ',' '4' ')'] */
} hfAgIndicators_t;

typedef struct
{
    hfAgIndicators_t                     agIndicators;
    CsrBtDeviceAddr                      currentDeviceAddress;
    CsrCmnListSimple_t                   remoteHfIndicatorList;   /* List to contain connection instance specific
                                                                     HF Indicator Information (CsrBtHfRemoteHfIndicator) */

#ifndef CSR_TARGET_PRODUCT_VM
    void                                 *mainInstance;          /* pointer to the main instance */
#endif

    HfHsData_t                           *data;
    CsrUtf8String                        *serviceName;

#ifdef CSR_BT_INSTALL_HF_CONFIG_AUDIO
    audioSetupParams_t                   *audioSetupParams;
#endif

    void                                 *sdpSearchData;
    CsrUint32                            hfConnId;
    CsrUint32                            supportedFeatures;
    CsrUint16                            remoteVersion;
    CsrUint16                            scoHandle;              /* Sco handle */
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
    CsrUint16                            hfQceCodecId;           /* Qualcomm Codec Extension Id (if selected). */

#if 0 /* hfAgQceCodecMask is not being used currently */
    CsrUint16                            hfAgQceCodecMask;      /* Bit mask representing QCE codecs supported by AG */
#endif
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
    CsrUint8                             obtainedServerCh;       /* SDC RFC lib obtained server channel for outgoing connection.
                                                                  * For incoming connection it will be CSR_BT_NO_SERVER. */
    CsrUint8                             pcmSlot;
    CsrUint8                             instId;                 /* identifier for this instance */
    CsrUint8                             codecToUse;              /* Codec requested from the AG, which should be used for the next eSCO connection */
    CsrUint8                             lastAtCmdSent:7;        /* HfLastAtCmdSent_t */
    CsrUint8                             network:1;
    HfAtStates_t                         atSequenceState:5;
    HfServiceState_t                     serviceState:3;
    CsrUint8                             nrOfIndicators:5;
    CsrUint8                             linkState:4;              /* state of the ACL link (connected, sniff, disconnected) */
    HfStates_t                           state:3;                  /* my state */
    CsrBool                              audioPending:1;
    HfStates_t                           oldState:3;
    CsrBtHfConnectionType                linkType:2;               /* Type of peer connection */
    CsrBool                              lastAudioReq:1;
    CsrBool                              pcmReassign:1;
    CsrBool                              pcmMappingReceived:1;
    CsrBool                              audioAcceptPending:1;
    CsrBool                              disconnectReqReceived:1;  /* hf_cancel_req received from the app. */
    CsrBool                              disconnectPeerReceived:1; /* disconnect received from the peer */
    CsrBool                              searchAndCon:1;
    CsrBool                              searchOngoing:1;
    CsrBool                              pendingCancel:1;
    CsrBool                              scoConnectAcceptPending:1;
    CsrBool                              incomingSlc:1;             /* Connection started from remote device */
    CsrBool                              pendingSlcXtraCmd:1;       /* Need to enable CLIP, CCWA or CMEE automatically at SLC establishment? */
    CsrBool                              accepting:1;               /* CSR_BT_CM_CONNECT_ACCEPT_REQ sent; ready to accept connection */
    CsrBool                              userAction:1;              /* Whether connection request was result of user action or reconnection attempt */
    CsrBool                              instReused:1;              /* Whether instance is being re-used for incoming connection in favor of dropping outgoing connection for crossover scenarios */
} HfInstanceData_t;

typedef struct HfMainInstanceData_tTag
{
    audioSetupParams_t                  generalAudioParams;
    CsrBtDeviceAddr                     currentDeviceAddress;

    void                               *recvMsgP;               /* Pointer to the received message. NULL if it has been passed on */
    CsrMessageQueueType                *saveQueue;
    HfInstanceData_t                   *linkData;               /* linkData */
    HfInstanceData_t                   *inactiveLinkData;       /* inactive linkData temporarily copied here to be released during sdsunregister / cancel connect accept */
    CsrBtHfpHfIndicatorId              *localHfIndicatorList;   /* Array of local supported HF Indicator */

    CsrUint32                           hfServiceRecHandle;      /* used to save the HF service record handle */
    CsrUint32                           hsServiceRecHandle;      /* used to save the HS service record handle */
    CsrUint32                           localSupportedFeatures;
    CsrUint32                           mainConfig;
    CsrUint16                           eventClass;                             /* */
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
    CsrBtHfQceCodecMask                 hfQceCodecMask;          /* Bitfield of QCE Mode IDS supported by HF */
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
    CsrSchedQid                         appHandle;               /* Application handle */
#ifdef INSTALL_HF_CUSTOM_SECURITY_SETTINGS
    dm_security_level_t                 secIncoming;           /* Incoming/outgoing security level based on the connection direction */
    dm_security_level_t                 secOutgoing;           /* Incoming/outgoing security level based on the connection direction */
#endif /* INSTALL_HF_CUSTOM_SECURITY_SETTINGS */
    CsrUint8                            hfServerChannel;         /* allocated (local) HF server channel */
    CsrUint8                            hsServerChannel;         /* allocated (local) HS server channel */
    CsrUint8                            atRespWaitTime;          /* time to wait for answer from HFG  after sending an AT cmd */
#ifdef HF_ENABLE_OPTIONAL_CODEC_SUPPORT
    HfCodecId                           *optionalCodecList;       /* Pointer to list of optional codecs supported by the device. */
    CsrUint8                            optionalCodecCount;       /* Number of optional codecs in 'optionalCodecList' list. */
#endif
    CsrUint8                            index:6;                   /* index to current connection */
    HfStates_t                          state:2;                   /* Main state */
    CsrUint8                            allocInactiveHfCons:5;     /* number of inactive HF service records allocated (registered at CM) */
    CsrUint8                            supportedCodecsMask:3;
    CsrUint8                            numberOfUnregister:2;      /* Used to control the number of retries on CmSdsUnregister */
    CsrBool                             restoreFlag:1;             /* Common saveQueue */
    CsrUint8                            allocInactiveHsCons:5;     /* number of inactive HS service records allocated (registered at CM) */
    CsrBool                             connectSdcActive:1;        /* Set when hf_main_sef runs SDC search in connect */
    CsrBool                             deactivated:1;             /* Used when profile is deactivated in the ServiceSearch_s state. Then the message */
                                                                 /* is saved and this flag is used to indicate that no messages should be send to app */
    CsrBool                             doingCleanup:1;
    CsrUint8                            indCount:5;                /* Number of HF indicators in localHfIndicatorList */
    CsrBool                             reActivating:1;            /* activation ongoing (even if already activated) */
    CsrUint8                            maxHFConnections:5;        /* maximum number of simultaneous HF connections allowed; if 0, the HFP is disabled! */
    CsrUint8                            maxHSConnections:5;        /* maximum number of simultaneous HS connections allowed; if 0, the HSP is disabled! */
    CsrUint8                            maxTotalSimultaneousConnections:5; /* maximum number of simultaneous connections allowed; overrides the two previous if needed */
    CsrUint8                            deferSelCodecInd: 1;       /* Defer sending selected codec indication to the application immediately after codec negotiation. */
} HfMainInstanceData_t;


#ifdef CSR_BT_GLOBAL_INSTANCE
extern HfMainInstanceData_t csrBtHfInstance;

#define CSR_BT_HF_MAIN_INSTANCE_GET(_linkInst)                 (CSR_UNUSED(_linkInst), &csrBtHfInstance)
#define CSR_BT_HF_MAIN_INSTANCE_SET(_linkInst, _mainInstance)

#else

#define CSR_BT_HF_MAIN_INSTANCE_GET(_linkInst)  ((HfMainInstanceData_t *) (_linkInst)->mainInstance)
#define CSR_BT_HF_MAIN_INSTANCE_SET(_linkInst, _mainInstance)  (_linkInst)->mainInstance = (_mainInstance)
#endif


typedef void (*HfStateHandlerType)(HfMainInstanceData_t * hfInstanceData);
void CsrBtHfpHandler(HfMainInstanceData_t * instData);
typedef CsrBool (*HfAtHandler_t)(HfMainInstanceData_t *instData, CsrUint8 *atTextString);

void CsrBtHfSdcResultHandler(void                     * inst,
                             CmnCsrBtLinkedListStruct * sdpTagList,
                             CsrBtDeviceAddr          deviceAddr,
                             CsrBtResultCode          resultCode,
                             CsrBtSupplier      resultSupplier);

void CsrBtHfRfcSdcConResultHandler(void                        *inst,
                                   CsrUint8               localServerCh,
                                   CsrUint32                    hfConnId,
                                   CsrBtDeviceAddr             deviceAddr,
                                   CsrUint16                    maxFrameSize,
                                   CsrBool                      validPortPar,
                                   RFC_PORTNEG_VALUES_T        *portPar,
                                   CsrBtResultCode             resultCode,
                                   CsrBtSupplier         resultSupplier,
                                   CmnCsrBtLinkedListStruct    *sdpTagList);

void CsrBtHfSdcSelectServiceHandler(void                    * instData,
                               void                    * cmSdcRfcInstData,
                               CsrBtDeviceAddr            deviceAddr,
                               CsrUint8           serverChannel,
                               CsrUint16                entriesInSdpTaglist,
                               CmnCsrBtLinkedListStruct * sdpTagList);

#ifdef __cplusplus
}
#endif

#endif          /*_HF_MAIN_H*/
