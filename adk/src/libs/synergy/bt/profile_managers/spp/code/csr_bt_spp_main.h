#ifndef CSR_BT_SPP_MAIN_H__
#define CSR_BT_SPP_MAIN_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_SPP_MODULE

#include "csr_types.h"
#include "csr_bt_profiles.h"
#include "rfcomm_prim.h"
#include "bluetooth.h"
#include "hci_prim.h"
#include "csr_sched.h"
#include "csr_message_queue.h"
#include "csr_bt_spp_prim.h"
#include "csr_bt_cmn_sdc_rfc_util.h"
#include "csr_bt_cmn_sdr_tagbased_lib.h"
#include "csr_bt_cmn_sdp_lib.h"
#include "csr_log_text_2.h"

#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
#include "csr_am_lib.h"
#endif
#endif /* !EXCLUDE_CSR_AM_MODULE */

#ifdef __cplusplus
extern "C" {
#endif

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtSppLto);

#define SPP_CLASS_OF_DEVICE             (0x000000)                        /* The COD for the SPP - definition in BT assigned numbers */
#define ACTIVE_POWER_MODE               (CONNECTION_MODE)                 /* SPP always want to be in active mode once connected */
#define SPP_PAGE_SCAN_INTERVAL          (0x800)                           /* interval 0x12 - 0x1000, default 0x800 */
#define SPP_PAGE_SCAN_WINDOW            (0x100)                           /* window   0x12 - 0x1000, default 0x12 */
#define SPP_PORT_SPEED                  (PORT_SPEED_UNUSED)               /* the port speed used during port neg */
#define MAX_NUMBER_OF_CONNECTIONS       (1)                               /* SPP can only handle one connection at any time (phase I) */
#define SPP_NO_CONNID                   (0xFFFFFFFFu)
#define SPP_SDC_NUM_OF_SEARCH_RECS      (5)
#define SPP_SDC_NUM_OF_BYTES_TO_RETURN  (50)                              /* Maximum number of bytes the sdp attribute search will return */

/* Number of SPP instances that is allocated at once. */
#define SPP_INSTANCES_POOL_SIZE         (10)

/********************** Internal defines and types ***************************/
/* Spp states */
#define Init_s                0x00
#define Idle_s                0x01
#define Activated_s           0x02
#define Connected_s           0x03
#define Deactivate_s          0x04
#define SppNumberOfStates     0x05

typedef CsrUint8               SppStates_t;

#define SPP_SUB_IDLE_STATE              (0)
#define SPP_SUB_CROSSING_CONNECT_STATE  (1)
/*#define SPP_SUB_RFC_SEARCH_STATE        (2)*/
/*#define SPP_SUB_UUID_128_SEARCH_STATE   (3)*/
#define SPP_SUB_CONNECTING_STATE        (2) /*(4)*/

/* Spp Audio states */
#define SPP_AUDIO_SUB_STATE_BASE (0x00)
typedef enum SppAudioStates_t
{
    audioOff_s = SPP_AUDIO_SUB_STATE_BASE,
    audioReq_s,
    audioAcceptReq_s,
    audioAcceptInProgress_s,
    audioOn_s,
    audioRelease_s
} SppAudioStates_t;

/***  AM related definitions ***/ 
#define CSR_BT_FIRST_USE_AM_BUILD_ID         0xFFFF /* 6817  */

typedef struct SppInstancesPool_t
{
    struct SppInstancesPool_t  *next;
#ifdef CSR_BT_GLOBAL_INSTANCE
    void                       *connInstPtrs[SPP_INSTANCES_POOL_SIZE];
#endif
    CsrSchedQid                 phandles[SPP_INSTANCES_POOL_SIZE];
    CsrUint8                    numberInPool: 4;
} SppInstancesPool_t;

typedef struct
{
    CsrUint16     selectServiceIdx;
    CsrBtUuid32     serviceHandle;
} sppServiceHandle_t;

/* Settings to determine whether to use the audio manager or not */
typedef enum
{
    UndefinedUseAm  = 0,
    UseAm,
    DoNotUseAm
}SppUseAm_t;

#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
typedef struct SppExtActivationData_t
{
    CsrUint32                cod;
    CsrUint8                *serviceRecord;           /* pointer to the alternative service record */
    CsrUint16                serviceRecordSize;       /* length of the alternative service record */
    CsrUint16                serviceRecordSrvChIndex; /* index to the server channel in alternate service record */
} SppExtActivationData_t;
#endif /* !EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */   

struct SppInstanceData_tTag;
typedef void (SppCallBackFunction)(struct SppInstanceData_tTag* inst);

typedef struct SppInstanceData_tTag
{
    BD_ADDR_T                bdAddr;
    CsrUint32                sppConnId;               /* connectionId */
    CsrUint32                sdsRecHandle;
    CsrCharString           *serviceName;            /* temp storage of service name */
    CsrMessageQueueType     *saveQueue;
    void                    *recvMsgP;               /* pointer to the received message. NULL if it has been passed on */
    SppInstancesPool_t      *sppInstances;
#ifndef EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ
    SppExtActivationData_t  *extendedActivationData; /* pointer to extended activation data */
#endif /* !EXCLUDE_CSR_BT_SPP_EXTENDED_ACTIVATE_REQ */   
    CsrSchedQid              ctrlHandle;              /* control application handle */
#ifndef CSR_STREAMS_ENABLE
    CsrSchedQid              dataHandle;              /* data application handle */
#endif /* !CSR_STREAMS_ENABLE */
    CsrSchedQid              myAppHandle;             /* own scheduler app handle */
    CsrBtCplTimer            activateTime;
#ifdef INSTALL_SPP_CUSTOM_SECURITY_SETTINGS    
    dm_security_level_t      secIncoming;             /* Incoming security level */
    dm_security_level_t      secOutgoing;             /* Outgoing security level */
#endif /* INSTALL_SPP_CUSTOM_SECURITY_SETTINGS  */
    CsrUint8                 serverChannel;           /* Local server channel    for server role */
    CsrUint8                 modemStatus;
    CsrUint8                 breakSignal;
#ifdef INSTALL_SPP_OUTGOING_CONNECTION
    /* Common SDP search stuff */
    void                    *sdpSppSearchConData;    /* Instance data for the common SDP search lib */
    void                    *cmSdcRfcInstData;       /* Value received in "select" callbak function, that shall be used in CSR_BT_SPP_SERVICE_NAME_RES handler */
    CsrBtSppServiceName     *sdpServiceNameList;
    sppServiceHandle_t      *serviceHandleList;
    RFC_PORTNEG_VALUES_T    *portPar;                /* pointer to port negotiation parameters */
    CsrUint16                serviceHandleListSize;
    CsrBool                  requestPortPar: 1;
    CsrBool                  validPortPar: 1;
    CsrBool                  connectReqActivated: 1; /* TRUE of a connect req have been initiated but not confirmed.
                                                      * Used to determine if incoming connections areaccepted or disconnected.*/
    CsrBool                  searchOngoing: 1;       /* Search already ongoing or not */
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */
    CsrUint8                 numberOfSppInstances: 4;
    SppStates_t              state: 3;                   /* my state */
    CsrUint8                 subState: 2;                /* SPP sub states, used for cancel connect procedure */
    CsrUint8                 numberOfConnections: 2;     /* Number of connections */
    CsrUint8                 role: 2;
    CsrUint8                 currentLinkMode: 2;
    CsrBool                  sdsUnregInProgress: 1;      /* true when unregistering an sds service (in idle and connected states) */
    CsrBool                  sdsRecordObtain: 1;         /* true if a SDS record is obtained */
    CsrBool                  restoreSppFlag: 1;
    CsrBool                  cancelReceived: 1;
#ifndef CSR_STREAMS_ENABLE
    CsrBool                  sppRegisterDataPathPending: 1;
#endif
#ifdef INSTALL_SPP_REMOTE_PORT_NEGOTIATION    
    CsrBool                  sppPortNegPending: 1;
#endif /* INSTALL_SPP_REMOTE_PORT_NEGOTIATION */
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
    CsrBtUuid128             uuid128Profile;
    uuid16_t                 extendedProfileUuid;
    CsrBool                  extendedConnect: 1;         /* one of two extended connect used */
#endif
#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
    SppCallBackFunction     *amSppCallBack;           /* Call back function for AM messages */
    CsrUint32                txBandwidth;
    CsrUint32                rxBandwidth;
    SppAudioStates_t         audioState;
    SppUseAm_t               audioUseAm;               /* Audio connections through the Audio manager module? */
    CsrUint16                amConnId;                 /* Audio manager connection ID */
    hci_pkt_type_t           audioQuality;
    CsrUint16                maxLatency;
    CsrUint16                voiceSettings;
    CsrUint8                 reTxEffort;
    CsrBool                  pendingSco: 1;
    CsrBool                  pendingScoDisconnect: 1;
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_AM_MODULE */
} SppInstanceData_t;

typedef void (* SppStateHandlerType)(SppInstanceData_t *sppInstanceData);
#ifdef CSR_BT_GLOBAL_INSTANCE
extern SppInstanceData_t sppInstanceData;
#endif

#ifdef INSTALL_SPP_OUTGOING_CONNECTION
void CsrBtSppRfcSdcConResultHandler(void *inst,
    CsrUint8               localServerCh,
    CsrUint32                    sppConnId,
    CsrBtDeviceAddr                deviceAddr,
    CsrUint16                    maxFrameSize,
    CsrBool                      validPortPar,
    RFC_PORTNEG_VALUES_T        portPar,
    CsrBtResultCode             resultCode,
    CsrBtSupplier          resultSupplier,
    CmnCsrBtLinkedListStruct     *sdpTagList);

void CsrBtSppSdcSelectServiceHandler(void                    * inst,
                                void                    * cmSdcRfcInstData,
                                CsrBtDeviceAddr            deviceAddr,
                                CsrUint8           serverChannel,
                                CsrUint16                entriesInSdpTaglist,
                                CmnCsrBtLinkedListStruct * sdpTagList);
#endif /* INSTALL_SPP_OUTGOING_CONNECTION */

#ifndef EXCLUDE_CSR_AM_MODULE
#ifdef CSR_BT_INSTALL_SPP_EXTENDED
void CsrSppAmInitCfm(SppInstanceData_t *inst);
void CsrSppAmConnectCfm(SppInstanceData_t *inst);
void CsrSppAmReleaseCfm(SppInstanceData_t *inst);
void CsrSppConnectStream(SppInstanceData_t *inst, CsrAmAudioType audioType, CsrUint8 pcmSlot, CsrUint16 escoHandle);
#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_AM_MODULE */

void CsrBtSppMessagePut(CsrSchedQid phandle, void *msg);

#ifdef __cplusplus
}
#endif

#endif  /*_SPP_HANDLER_H */
#endif  /* EXCLUDE_CSR_BT_SPP_MODULE */
