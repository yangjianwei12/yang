#ifndef CSR_BT_CM_MAIN_H__
#define CSR_BT_CM_MAIN_H__
/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_pmem.h"
#include "csr_bt_tasks.h"
#include "csr_message_queue.h"
#include "l2cap_prim.h"
#include "bluestacklib.h"
#include "bkeyval.h"
#include "sdclib.h"

#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_lib.h"
#endif
#include "csr_bt_cm_prim.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_profiles.h"
#include "csr_bt_util.h"
#include "csr_bt_addr.h"
#include "csr_bt_cmn_linked_list.h"
#include "csr_list.h"
#include "csr_log_text_2.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_streams.h"
#endif

#ifdef INSTALL_CM_DEVICE_UTILITY
#include "cm_device_utility.h"
#endif /* INSTALL_CM_DEVICE_UTILITY */

#ifdef __cplusplus
extern "C" {
#endif

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtCmLto);
#define CSR_BT_CM_LTSO_GEN                  0
#define CSR_BT_CM_LTSO_STATE                1
#define CSR_BT_CM_LTSO_PLAYER               2
#define CSR_BT_CM_LTSO_DM_QUEUE             3
#define CSR_BT_CM_LTSO_SM_QUEUE             4
#define CM_LTSO_SDC_QUEUE                   5

/* L2CAP's default is 3 queues, which leaves the lowest priority at 2,
 * so cap at this value */
#define CSR_BT_CM_PRIORITY_LOW (0x02)

#define CSR_BT_CM_STATE_CHANGE(_var, _state)                            \
    do                                                                  \
    {                                                                   \
        CSR_LOG_TEXT_DEBUG((CsrBtCmLto,                                  \
                           CSR_BT_CM_LTSO_STATE,                        \
                           #_var" state: %d -> %d",                     \
                           (_var),                                      \
                           (_state)));                                  \
        (_var) = (_state);                                              \
    } while (0)

#define CSR_BT_CM_PLAYER_CHANGE(_var, _player)                          \
    do                                                                  \
    {                                                                   \
        CSR_LOG_TEXT_INFO((CsrBtCmLto,                                  \
                           CSR_BT_CM_LTSO_PLAYER,                       \
                           #_var" player: %d -> %d",                    \
                           (_var),                                      \
                           (_player)));                                 \
        (_var) = (_player);                                             \
    } while (0)

#ifdef CSR_BT_LE_ENABLE
#define CSR_BT_CM_SET_APP_HANDLE(_var, _value)                          \
    do                                                                  \
    {                                                                   \
        (_var) = (_value);                                              \
    } while (0)

#endif

#define CsrBtCmGeneralException(eventClass, primType, state, message)   \
    CsrGeneralException(CsrBtCmLto, 0 , (eventClass), (CsrUint16) (primType), (CsrUint16) (state), (message))

#define CsrBtCmGeneralExceptionOn(cond, eventClass, primType, state)                     \
    do {                                                                                 \
        if ((cond))                                                                      \
        {                                                                                \
            CsrGeneralException(CsrBtCmLto, 0, (eventClass), (primType), (state), "");   \
        }                                                                                \
    } while(0)


#if defined (CSR_TARGET_PRODUCT_VM) 

#define CsrBtCmLogTextErrorRetVal(cond, retval)                                     \
    do {                                                                            \
        if (!(cond))                                                                \
        {                                                                           \
            SYNERGY_HYDRA_LOG_STRING(cond_str, SYNERGY_FMT(#cond, bonus_arg));      \
            SYNERGY_HYDRA_LOG_STRING(the_file, SYNERGY_FMT(__FILE__, bonus_arg));   \
            CSR_LOG_TEXT_ERROR((CsrBtCmLto,                                         \
                               0,                                                   \
                               "Assertion \"%s\" failed at %s:%u",                  \
                               cond_str,                                            \
                               the_file,                                            \
                               __LINE__));                                          \
            return (retval);                                                        \
        }                                                                           \
    } while(0)

#elif defined (CSR_TARGET_PRODUCT_WEARABLE) 

#define CsrBtCmLogTextErrorRetVal(cond, retval)                          \
    do {                                                                 \
        if (!(cond))                                                     \
        {                                                                \
            CSR_LOG_TEXT_ERROR((CsrBtCmLto,                              \
                               0,                                        \
                               "Assertion Failure at %u",                \
                               __LINE__));                               \
            return (retval);                                             \
        }                                                                \
    } while(0)

#else
#define CsrBtCmLogTextErrorRetVal(cond, retval)                          \
        do {                                                                 \
            if (!(cond))                                                     \
            {                                                                \
                CSR_LOG_TEXT_ERROR((CsrBtCmLto,                              \
                                   0,                                        \
                                   "Assertion \"%s\" failed at %s:%u",       \
                               #cond,                                    \
                                   __FILE__,                                 \
                                   __LINE__));                               \
                return (retval);                                             \
            }                                                                \
        } while(0)

#endif /* CSR_TARGET_PRODUCT_VM */

#define CsrBtCmAssertRet(cond)                                           \
    CSR_LOG_TEXT_ASSERT(CsrBtCmLto, 0, cond)

#define MILLI_TO_BB_SLOTS(a) ((CsrUint16) (((a) * CSR_SCHED_MILLISECOND) / 625))

#define CM_ERROR                                      (255)

typedef CsrUint8 CsrBtCmPlayer;
#define RFC_PLAYER                                    ((CsrBtCmPlayer) 0x00)
#define L2CAP_PLAYER                                  ((CsrBtCmPlayer) 0x01)
#define BNEP_PLAYER                                   ((CsrBtCmPlayer) 0x02)
#define SDC_PLAYER                                    ((CsrBtCmPlayer) 0x03)
#define SC_PLAYER                                     ((CsrBtCmPlayer) 0x04)
#define CM_PLAYER                                     ((CsrBtCmPlayer) 0x05)
#define CM_LINK_POLICY_PLAYER                         ((CsrBtCmPlayer) 0x06)
#define CM_ROLE_SWITCH_PLAYER                         ((CsrBtCmPlayer) 0x07)
#define APP_PLAYER                                    ((CsrBtCmPlayer) 0x08)
#define RNR_PLAYER                                    ((CsrBtCmPlayer) 0x09)
#define LE_PLAYER                                     ((CsrBtCmPlayer) 0x0A)
#define UNDEFINED_PLAYER                              ((CsrBtCmPlayer) 0x0B)
#define KEEP_CURRENT_PLAYER                           ((CsrBtCmPlayer) 0x0F)

#define MAX_PCM_SLOTS                                 (4)

/* Defines Sniff values         */
#define CM_HCI_MAX_SNIFF_INTERVAL                    (0xFFFE)
#define CM_HCI_MIN_SNIFF_INTERVAL                    (0x0002)
#define CM_HCI_MAX_SNIFF_ATTEMPT                     (0x7FFF)
#define CM_HCI_MIN_SNIFF_ATTEMPT                     (0x0001)
#define CM_HCI_MAX_SNIFF_TIMEOUT                     (0x7FFF)
#define CM_HCI_SNIFF_DRAWBACK                        (0x0010)

/* Defines for DM cache handling */
#define CM_DM_CACHE_FLUSH_TIMEOUT                     (25000000)
#define CM_DM_CACHE_PARAM_POOL_SIZE                   (10)
#define CM_DM_MAX_NUM_OF_CACHE_POOLS                  (10)
#define CM_DM_MAX_SEQ_NUMBER                          (65535)

#ifndef NUM_OF_ACL_CONNECTION
#define NUM_OF_ACL_CONNECTION                         (3) /* CSR_TARGET_PRODUCT_VM requires 2 ACL (peer & 2 handsets), TODO: Add CMAKE Configuration for Max ACL. */
#endif

/* Defines for RFC CONNECT state */
typedef CsrUint8 CsrBtCmRfcState;
#define CM_RFC_IDLE                                   ((CsrBtCmRfcState) 0)
#define CM_RFC_CONNECTED                              ((CsrBtCmRfcState) 1)
#define CM_RFC_CANCELING                              ((CsrBtCmRfcState) 2)
#define CM_RFC_SSP_REPAIR                             ((CsrBtCmRfcState) 3)

/* Defines for L2CA connect State */
typedef CsrUint8 CsrBtCmL2caState;
#define CM_L2CA_IDLE                                  ((CsrBtCmL2caState) 0)
#define CM_L2CA_CONNECT                               ((CsrBtCmL2caState) 1)
#define CM_L2CA_CONNECT_PENDING                       ((CsrBtCmL2caState) 2)
#define CM_L2CA_SSP_REPAIR                            ((CsrBtCmL2caState) 3)
#define CM_L2CA_CANCELING                             ((CsrBtCmL2caState) 4)

/* Defines for L2CA connection context */
#define CM_L2CA_INCOMING_CONNECT_REJECTED_CTX         (0)
#define CM_L2CA_CONNECT_INPROGRESS_CTX                (1)
#define CM_L2CA_RECONFIG_INPROGRESS_CTX               (2)

/* Defines for BNEP connect State */
#define CM_BNEP_IDLE                                  (0)
#define CM_BNEP_CONNECT                               (1)
#define CM_BNEP_SSP_REPAIR                            (2)

#define DM_SM_ACCESS_REQ_RETRY_DELAY                  (50*CSR_SCHED_MILLISECOND)
#define DM_SM_ACCESS_MAX_RETRY                        (1)

/* Defines for SSR */
#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1
#define CM_SSR_DISABLED                               (0x0)
#define CM_SSR_MAX_LATENCY_VALUE                      (0xFFFE)
#define SSR_MAX_LATENCY_DONT_CARE                     (0xFFFF)
#define SSR_MIN_REMOTE_TIMEOUT_DONT_CARE              (0xFFFF)
#define SSR_MIN_LOCAL_TIMEOUT_DONT_CARE               (0xFFFF)
#endif /* CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 */

#define SSP_REPAIR_DELAY                              (300000)

/* Defines for low power settings */
#define LOW_POWER_DEFAULT_PRIORITY                    (255)
#define SNIFF_MAX_INTERVAL                            (0xFFFF)
#define SNIFF_MIN_INTERVAL                            (0xFFFF)

/* SCO related defines*/
#define SCO_PACKET_RESERVED_BITS                      (0xFC00)
#define HCI_ESCO_PKT_COUNT                            (0x0A)
#define CSR_BT_CM_SCO_MASK_BITS                       (0xFFF8)

/* CM inquiry states */
#define CM_INQUIRY_APP_STATE_IDLE                     (0)
#define CM_INQUIRY_APP_STATE_INQUIRING                (1)       /* An inquiry is in progress */
#define CM_INQUIRY_APP_STATE_RESTARTING               (2)       /* An inquiry is pending */

#define CM_INQUIRY_DM_STATE_IDLE                      (0)
#define CM_INQUIRY_DM_STATE_INQUIRING                 (1)       /* Performing inquiry */
#define CM_INQUIRY_DM_STATE_CANCELLING                (2)       /* Cancelling inquiry */
#define CM_INQUIRY_DM_STATE_SETTING_INQUIRY_POWER_LEVEL (3)     /* Setting Inquiry TX power level */
#define CM_INQUIRY_DM_STATE_SET_LOW_PRIORITY          (4) 
#define CM_INQUIRY_DM_STATE_SCAN_ALLOWED              (5) 

typedef CsrUint8 CsrBtCmModemStatusState;
/* CM modem status states */
#define CSR_BT_CM_MODEM_STATUS_IDLE                   ((CsrBtCmModemStatusState) 0x00)
#define CSR_BT_CM_MODEM_STATUS_PENDING                ((CsrBtCmModemStatusState) 0x01)
#define CSR_BT_CM_MODEM_STATUS_QUEUED                 ((CsrBtCmModemStatusState) 0x02)

/* Invalid L2CAP extended features cache */
#define CM_INVALID_L2CAP_EXT_FEAT                     ((CsrUint32)0xFFFFFFFFu)

/* Define for InquiryMask                               */
#define CSR_BT_CM_INQUIRY_MASK_SET_NONE             0x0000
#define CSR_BT_CM_INQUIRY_MASK_SET_TRANSMIT_POWER   0x0001
#define CSR_BT_CM_INQUIRY_MASK_SET_PRIORITY_LEVEL   0x0002

/* Define for Inquiry Priority levels                   */
#define CSR_BT_CM_DEFAULT_INQUIRY_LEVEL               0x00
#define CSR_BT_CM_LOW_PRIORITY_INQUIRY_LEVEL          0x01
#define CSR_BT_CM_UNDEFINED_PRIORITY_INQUIRY_LEVEL    0xFF

/* AMP/BR-EDR controller activity */
#define CTRL_ACTIVE_AMP_ONLY                          0
#define CTRL_ACTIVE_BREDR_ACTIVE                      1
#define CTRL_ACTIVE_INACTIVE                          2

/* AMP/BR-EDR controller processState */
#define CSR_BT_CM_AMP_PROCESS_IDLE                    0
#define CSR_BT_CM_AMP_PROCESS_LINK_LOST_IND_PENDING   1
#define CSR_BT_CM_AMP_PROCESS_LINK_LOST_REQ_PENDING   2

/* Default HCI Quality of Service parameters                    */
#define CSR_BT_CM_DEFAULT_QOS_SERVICE_TYPE        (HCI_QOS_BEST_EFFORT) /* Valid values are HCI_QOS_BEST_EFFORT, HCI_QOS_NO_TRAFFIC, HCI_QOS_GUARANTEED */
#define CSR_BT_CM_DEFAULT_QOS_TOKEN_RATE          (0x00000000)          /* Token Rate in bytes per second       */
#define CSR_BT_CM_DEFAULT_QOS_PEAK_BANDWIDTH      (0x00000000)          /* Peak Bandwidth in bytes per second   */
#define CSR_BT_CM_DEFAULT_QOS_LATENCY             (0x000061A8)          /* Latency in micro seconds             */
#define CSR_BT_CM_DEFAULT_QOS_DELAY_VARIATION     (0x000061A8)          /* Delay Variation in micro seconds     */

#define CSR_BT_CM_INVALID_LMP_VERSION               0xFF

#define CM_QCA_MAX_EVENT_FILTERS                    0x05

/* Global states */
typedef CsrUint8 CsrBtCmStateGlobal;
#define CSR_BT_CM_STATE_NOT_READY                   ((CsrBtCmStateGlobal) 0)
#define CSR_BT_CM_STATE_IDLE                        ((CsrBtCmStateGlobal) 1)

/* DM states */
typedef CsrUint8 CsrBtCmStateDm;
#define CSR_BT_CM_DM_STATE_NULL                     ((CsrBtCmStateDm) 0)
#define CSR_BT_CM_DM_STATE_CONNECT_INIT             ((CsrBtCmStateDm) 1)
#define CSR_BT_CM_DM_STATE_CONNECT                  ((CsrBtCmStateDm) 2)

/* RFCOMM connection states */
typedef CsrUint8 CsrBtCmStateRfc;
#define CSR_BT_CM_RFC_STATE_IDLE                    ((CsrBtCmStateRfc) 0)
#define CSR_BT_CM_RFC_STATE_CONNECT_INIT            ((CsrBtCmStateRfc) 1)
#define CSR_BT_CM_RFC_STATE_CONNECT                 ((CsrBtCmStateRfc) 2)
#define CSR_BT_CM_RFC_STATE_ACCESS                  ((CsrBtCmStateRfc) 3)
#define CSR_BT_CM_RFC_STATE_CONNECTABLE             ((CsrBtCmStateRfc) 4)
#define CSR_BT_CM_RFC_STATE_CONNECT_ACCEPT          ((CsrBtCmStateRfc) 5)
#define CSR_BT_CM_RFC_STATE_CONNECT_ACCEPT_FINAL    ((CsrBtCmStateRfc) 6)
#define CSR_BT_CM_RFC_STATE_CANCEL_CONNECTABLE      ((CsrBtCmStateRfc) 7)
#define CSR_BT_CM_RFC_STATE_RELEASE                 ((CsrBtCmStateRfc) 8)
#define CSR_BT_CM_RFC_STATE_CONNECTED               ((CsrBtCmStateRfc) 9)
#define CSR_BT_CM_RFC_STATE_CANCEL_TIMER            ((CsrBtCmStateRfc) 10)

/* L2CAP connection states */
typedef CsrUint8 CsrBtCmStateL2cap;
#define CSR_BT_CM_L2CAP_STATE_IDLE                  ((CsrBtCmStateL2cap) 0)
#define CSR_BT_CM_L2CAP_STATE_CONNECT               ((CsrBtCmStateL2cap) 1)
#define CSR_BT_CM_L2CAP_STATE_CONNECT_INIT          ((CsrBtCmStateL2cap) 2)
#define CSR_BT_CM_L2CAP_STATE_CONNECT_ACCEPT        ((CsrBtCmStateL2cap) 3)
#define CSR_BT_CM_L2CAP_STATE_CONNECT_ACCEPT_FINAL  ((CsrBtCmStateL2cap) 4)
#define CSR_BT_CM_L2CAP_STATE_RELEASE               ((CsrBtCmStateL2cap) 5)
#define CSR_BT_CM_L2CAP_STATE_CONNECTABLE           ((CsrBtCmStateL2cap) 6)
#define CSR_BT_CM_L2CAP_STATE_CONNECTED             ((CsrBtCmStateL2cap) 7)
#define CSR_BT_CM_L2CAP_STATE_CANCEL_CONNECTABLE    ((CsrBtCmStateL2cap) 8)
#define CSR_BT_CM_L2CAP_STATE_LEGACY_DETACH         ((CsrBtCmStateL2cap) 9)

/* BNEP connection states */
typedef CsrUint8 CsrBtCmStateBnep;
#define CSR_BT_CM_BNEP_STATE_IDLE                   ((CsrBtCmStateBnep) 0)
#define CSR_BT_CM_BNEP_STATE_CONNECT                ((CsrBtCmStateBnep) 1)
#define CSR_BT_CM_BNEP_STATE_CONNECT_INIT           ((CsrBtCmStateBnep) 2)
#define CSR_BT_CM_BNEP_STATE_CONNECT_CFM            ((CsrBtCmStateBnep) 3)
#define CSR_BT_CM_BNEP_STATE_CONNECT_ACCEPT         ((CsrBtCmStateBnep) 4)
#define CSR_BT_CM_BNEP_STATE_CANCEL_CONNECT_ACCEPT  ((CsrBtCmStateBnep) 5)
#define CSR_BT_CM_BNEP_STATE_DISCONNECT_REQ         ((CsrBtCmStateBnep) 6)
#define CSR_BT_CM_BNEP_STATE_CONNECTED              ((CsrBtCmStateBnep) 7)

/* SDC connection states */
typedef CsrUint8 CsrBtCmStateSdc;
#define CSR_BT_CM_SDC_STATE_IDLE                    ((CsrBtCmStateSdc) 0)
#define CSR_BT_CM_SDC_STATE_SEARCH                  ((CsrBtCmStateSdc) 1)
#define CSR_BT_CM_SDC_STATE_ATTR                    ((CsrBtCmStateSdc) 2)
#define CSR_BT_CM_SDC_STATE_CLOSE                   ((CsrBtCmStateSdc) 3)

#ifdef CSR_BT_INSTALL_EXTENDED_ADVERTISING
#define MAX_EXT_ADV_APP                               DM_ULP_EXT_ADV_MAX_ADV_HANDLES
#endif

#ifdef CSR_BT_INSTALL_EXTENDED_SCANNING
#define MAX_EXT_SCAN_APP                              0x05
#define CSR_BT_EXT_SCAN_HANDLE_INVALID                0xFF
#endif

#ifdef CSR_BT_INSTALL_PERIODIC_SCANNING
#define MAX_PERIODIC_SCAN_APP                         0x05
#define CSR_BT_PERIODIC_SCAN_HANDLE_INVALID           0xFF
#endif

#if defined(CSR_BT_INSTALL_EXTENDED_SCANNING) || defined(CSR_BT_INSTALL_PERIODIC_SCANNING)
#ifdef CSR_STREAMS_ENABLE
#define CSR_BT_UNIDENTIFIED_STREAM                    0x00
#define CSR_BT_EXT_SCAN_STREAM                        0x01
#define CSR_BT_PER_SCAN_STREAM                        0x02

#define CSR_BT_EXT_SCAN_STREAM_KEY                    (0xFFFF)

/* paSyncState and extScanState state bitmask values */
#define CSR_BT_EA_PA_NONE                             0x00
#define CSR_BT_EA_PA_REPORT_PROCESSING                0x01 /* EA or PA report is being processed */
#define CSR_BT_EA_PA_TERMINATING                      0x02 /* EA all scanners disabled or a PA train sync terminated */

#endif /* CSR_STREAMS_ENABLE */
#endif /* CSR_BT_INSTALL_EXTENDED_SCANNING || CSR_BT_INSTALL_PERIODIC_SCANNING */

#ifndef MAX_BUFFERED_L2CAP_REQUESTS
#define MAX_BUFFERED_L2CAP_REQUESTS                   (0x02) /* default number of pending datawrites for basic mode */
#endif

typedef struct {
    CsrUtf8String                *currentName;               /* Current local name */
    CsrUint8                     *services;                  /* Locally registered service records with service UUIDs */
    CsrUint16                     servicesOctetsUsed;
    CsrUint16                     manufacturerDataSettings;
    CsrUint8                      manufacturerDataLength;
    CsrUint8                     *manufacturerData;          /* Requested manufacturer data */
    CsrUint8                      flags;                     /* EIR flags */

    /* Temporary storage of parameters between request and a successful confirm */
    CsrUtf8String                *requestedName;             /* NULL-terminated */
    CsrUint16                     requestedServicesLength;
    CsrUint8                     *requestedServices;
    CsrBool                       requestedServicesNew;
} localEirData_t;

typedef struct
{
    CsrBool                       dataResReceived:1;
    CsrBool                       dataCfmPending:1;
    CsrBool                       pendingBufferStatus:1;
    CsrUint16                     restoreCount;
    CsrUint16                     saveCount;
    CsrUint8                      txCount;
    void                          *receivedBuffer[CSR_BT_CM_INIT_CREDIT];
} dataParameters;

typedef CsrUint32 cm_event_mask_t;

typedef struct
{
    CsrCmnListElm_t               elem;

    CsrBtCmEventMask              eventMask;
    CsrSchedQid                   appHandle;
    CsrBtCmEventMaskCond          condition;
} subscribeParms;

typedef struct
{
    CsrCmnListElm_t               elem;

    CsrSchedQid                   appHandle;
    CsrUint8                      map[10];
} afhMapParms_t;

typedef struct
{
    CsrSchedQid                   appHandle;
    CsrBtDeviceAddr               addr;
    CsrBtAddressType              addressType;
    CsrBtTransportType            transportType;
} remoteVersionReq;

typedef struct
{
    CsrSchedQid                   appHandle;
    CsrBtDeviceAddr               deviceAddr;
    CsrUint16                     timeout;
} cmLstoParms_t;

typedef struct
{
    CsrSchedQid                   appHandle;
    CsrUint16                     flags;
    CsrBtTypedAddr                addrt;
} cmAclOpenParam;

typedef struct
{
    CsrSchedQid appHandle;
    CsrUint16   localPsm;
} cmL2caUnregisterParam;

typedef struct
{
    CsrSchedQid appHandle;
} cmCommonParams;

#define CSR_BT_CM_PENDING_MSG_REMOTE_VERSION        0
#define CSR_BT_CM_PENDING_MSG_LSTO_PARAMS           1
#define CSR_BT_CM_PENDING_MSG_ACL_OPEN_PARAMS       2
#define CM_PENDING_MSG_L2CA_UNREGISTER_PARAMS       3
#define CM_PENDING_MSG_SM_CONFIG_PARAMS             4

typedef struct
{
    CsrCmnListElm_t               elem;

    CsrUint8                      type;

    union
    {
        remoteVersionReq          remoteVer;
        cmLstoParms_t             lstoParams;
        cmAclOpenParam            aclOpenParams;
        cmL2caUnregisterParam     l2caUnregisterParams;
        cmCommonParams            commonParams;
    } arg;
} cmPendingMsg_t;

#define CSR_BT_CM_PENDING_MSG_FIND_TYPE(_pendingMsgs, _type)                \
    (_pendingMsgs ?                                                         \
        CsrCmnListSearchOffsetUint8((CsrCmnList_t *) (_pendingMsgs),        \
                                    CsrOffsetOf(cmPendingMsg_t, type),      \
                                    (_type)) :                              \
        NULL)

typedef struct
{
    hci_pkt_type_t                packetTypes[HCI_ESCO_PKT_COUNT];
    CsrBtCmScoCommonParms         parms;
    CsrUint16                     count;
    CsrUint16                     index;
} cmSyncNegotiationType_t;

typedef struct
{
    cmSyncNegotiationType_t      *negotiateTypes;
    CsrUint16                     count;
    CsrUint16                     index;
} cmSyncNegotiationCntType_t;

typedef struct
{
    cmSyncNegotiationCntType_t   *negotiateCnt;
    CsrUint32                     txBdw;
    CsrUint32                     rxBdw;
    CsrUint16                     maxLatency;
    CsrUint16                     voiceSettings;
    CsrUint16                     rxPacketLength;
    CsrUint16                     txPacketLength;
    hci_connection_handle_t       handle;
    hci_pkt_type_t                packetType;
    CsrUint8                      reTxEffort;
    CsrUint8                      linkType;
    CsrUint8                      txInterval;
    CsrUint8                      weSco;
    CsrUint8                      reservedSlots;
    CsrUint8                      airMode;
    CsrUint8                      pcmSlot:4;
    CsrBool                       incoming:1;
    CsrBool                       closeSco:1;
} eScoParmVars;

typedef struct
{
    CsrBtDeviceAddr               deviceAddr;
    CsrBtConnId                   btConnId;

#ifndef CSR_STREAMS_ENABLE
    dataParameters                dataControl;
#endif

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && defined(CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS)
    CsrBtSsrSettingsDownstream    ssrSettings;
#endif /* CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS */

    void                         *controlSignalQueue;

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
    eScoParmVars                 *eScoParms;
#endif

    CsrUint24                     classOfDevice;
    CsrSchedQid                   appHandle;
    CsrUint16                     profileMaxFrameSize;
    CsrUint16                     context;
    CsrSchedTid                   timerId;

    CsrUint8                      remoteServerChan;
    CsrUint8                      serverChannel;
#ifdef CSR_TARGET_PRODUCT_VM
    CsrUint8                      dlci; /* Used to uniquely identify RFC sink on Handover */
#endif
#ifdef CSR_AMP_ENABLE
    CsrBtAmpController            controller;
    l2ca_identifier_t             moveIdentifier;
    CsrUint8                      ampProcessState;
#endif

    CsrUint8                      modemStatus;
    CsrUint8                      signalBreak;
    CsrUint8                      mscTimeout;

    CsrBtCmStateRfc               state:4;
    CsrBtCmModemStatusState       modemStatusState:2;
    CsrBool                       pending:1; /* For when outgoing RFC client connection is pending */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
    CsrBool                       logicalChannelData:1;
    CsrBool                       logicalChannelControl:1;
#endif

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && defined(CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS)
    CsrBool                       ssrAccepted:1;
#endif /* CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS */
    CsrUint8                       dmSmAccessRetry:1;
} cmRfcConnInstType;

typedef struct CmRfcConnTag
{
    struct CmRfcConnTag         *next;                      /* Do not move - used for common linked list handling */
    struct CmRfcConnTag         *prev;                      /* Do not move - used for common linked list handling */

    cmRfcConnInstType           *cmRfcConnInst;             /* RFC connection instance */
    CsrUint8                     elementId;                 /* Element ID */

#ifdef CSR_TARGET_PRODUCT_VM
    CsrBool                      unmarshalledAdded  :1;     /* Indicates if the element was added due to unmarshalling */
    CsrBool                      unmarshalledFilled :1;     /* Indicates if the element was filled due to unmarshalling */
#endif
    CsrBool                      app_controlled     :1;     /* Indicates if the element was added due to response from application */
} cmRfcConnElement;

typedef struct
{
    CsrCmnList_t                  connList;             /* List of cmRfcConnElement */
    CsrUint16                     connectAcceptTimeOut;
    CsrUint8                      activeElemId;
    CsrBool                       cancelConnect:1;
    CsrBtCmRfcState               connectState:3;
} rfcVariables;

#ifdef CSR_STREAMS_ENABLE
#define CSR_BT_CM_L2CAP_TX_MAX_COUNT_SET(_l2capInst, _val)
#else
#define CSR_BT_CM_L2CAP_TX_MAX_COUNT_SET(_l2capInst, _val)              \
    ((_l2capInst)->txMaxCount = (_val))
#endif

#ifdef INSTALL_L2CAP_LECOC_TX_SEG

/* Structure for maintaining record of LECOC data PDUs sent to Bluestack. */
typedef struct
{
    CsrUint16            pduSize;       /* Size of lecoc data pdu */ 
    CsrUint16            lengthCfd;     /* In case of segmentation, Bluestack sends multiple confirm messages for each data request.
    This variable stores the accumulated Length of the lecoc data pdu for which one or more confirms has already been received from Bluestack. */
} cmL2caLecocTxSegInfo;

#endif /* INSTALL_L2CAP_LECOC_TX_SEG */


typedef struct
{
    CsrBtDeviceAddr               deviceAddr;
    BKV_ITERATOR_T                conftabIter;
    CsrBtConnId                   btConnId;

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && defined(CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS)
    CsrBtSsrSettingsDownstream    ssrSettings;
#endif /* CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS */

    CsrUint24                     classOfDevice;

    CsrSchedQid                   appHandle;
    CsrUint16                     context;

    psm_t                         psm;
    psm_t                         remotePsm;

#ifndef EXCLUDE_CSR_BT_CM_LEGACY_PAIRING_DETACH
    dm_security_level_t           secLevel;
#endif

    l2ca_mtu_t                    outgoingMtu;
    l2ca_mtu_t                    incomingMtu;
    CsrUint16                     outgoingFlush;

    CsrUint8                      addressType;
    CsrUint8                      transportType;

#ifndef CSR_STREAMS_ENABLE
    CsrMblk                      *combine;          /* Partial L2CAP data read resegment buffer */

    /* Tx and Rx queues */
    CsrMessageQueueType          *rxQueue;          /* Receive buffer queue */
    CsrUint16                     rxQueueCount;     /* Total number of waiting/unacked messages */
    CsrUint16                     rxQueueMax;       /* Max number of elements we allow */
    CsrUint16                     rxQueueOverflow;  /* Number of overflowed indications */
    CsrUint16                     rxQueueOverpack;  /* Cummulative overflowed 'packets' counter */
    CsrUint16                     rxAppCredits;     /* Number of acks application has given us */
    CsrBool                       rxBusy;           /* Has RnR been sent? (only for ERTM) */

    CsrUint16                     txCount;          /* Current number of elements pending in L2CAP */
    CsrUint16                     txMaxCount;       /* Max number of pending elements */
    CsrUint16                     txPendingContext; /* Context for blocked element */
#ifdef INSTALL_L2CAP_LECOC_TX_SEG
    cmL2caLecocTxSegInfo          txLecocSegInfo[MAX_BUFFERED_L2CAP_REQUESTS];
    CsrUint16                     lecocSegDequeueIdx;
    CsrUint16                     lecocSegEnqueueIdx;
#endif /* INSTALL_L2CAP_LECOC_TX_SEG */

#endif /* !CSR_STREAMS_ENABLE */

#ifdef CSR_AMP_ENABLE
    l2ca_identifier_t             moveIdentifier;
    CsrBtAmpController            controller;
    CsrUint8                      ampProcessState;  
#endif

    CsrBtCmStateL2cap             state:4;
    CsrUint8                      dataPriority:2;
    CsrBool                       authorised:1;
    CsrBool                       ertm:1;
    CsrBool                       pendingBufferStatus:1;

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && defined(CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS)
    CsrBool                       ssrAccepted:1;
#endif /* CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
    CsrBool                       logicalChannelData:1;
    CsrBool                       logicalChannelControl:1;
#endif

#if defined(CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE) || defined(CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_A2DP_BIT_RATE)
    CsrBool                       logicalChannelStream:1;
#endif
} cmL2caConnInstType;

typedef struct CmL2caConnTag
{
    struct CmL2caConnTag        *next;                      /* Do not move - used for common linked list handling */
    struct CmL2caConnTag        *prev;                      /* Do not move - used for common linked list handling */

    cmL2caConnInstType          *cmL2caConnInst;            /* Connection instance */
    CsrUint8                     elementId;                 /* Element ID */

#ifdef CSR_TARGET_PRODUCT_VM
    CsrBool                      unmarshalledAdded:  1;     /* Indicates if the element was added due to unmarshalling */
    CsrBool                      unmarshalledFilled: 1;     /* Indicates if the element was filled due to unmarshalling */
#endif
    CsrBool                      app_controlled:     1;     /* Indicates if the element was added due to response from application */
    CsrBool                      useTpPrim:          1;     /* Indicates if the element uses TP Prims  */
} cmL2caConnElement;

#ifdef CSR_BT_INSTALL_L2CAP_CONNLESS_SUPPORT
typedef struct cmL2caConnlessTag
{
    CsrCmnListDataElm_t          *next;          /* Do not move - used for common linked list handling */
    CsrCmnListDataElm_t          *prev;          /* Do not move - used for common linked list handling */
    CsrBtDeviceAddr               deviceAddr;    /* Peer address */
    psm_t                         psm;           /* Connectionless PSM */
    CsrMessageQueueType          *queue;         /* Tx queue while waiting for ACL */
    CsrBtConnId                  btConnId;       /* L2CA_CID_INVALID for channels being mapped */
    CsrSchedTid                  unmapTimer;     /* Unmap delay timer */
} cmL2caConnlessElement;
#endif

typedef struct
{
    CsrCmnList_t                  connList;
#ifdef CSR_BT_INSTALL_L2CAP_CONNLESS_SUPPORT
    CsrCmnList_t                  connlessList;
#endif
    CsrUint8                      activeElemId;
    CsrBool                       cancelConnect:1;
    CsrBtCmL2caState              connectState:3;
} l2CaVariables;

typedef struct
{
    CsrUint32                     psm;
    CsrBtDeviceAddr               deviceAddr;

    CsrUint8                      deviceIndex:6;
    CsrUint8                      locked:1;         /* Used to identify internal CM add device request when handling the confirmation, internal request would not lock the DM queue. */
} scVariables;

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
typedef struct
{
    CsrBtCmStateBnep               state;
    CsrBtDeviceAddr                deviceAddr;
    CsrUint16                      id;
#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && defined(CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS)
    CsrBtSsrSettingsDownstream     ssrSettings;
    CsrBool                        ssrAccepted:1;
#endif /* CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS */
    CsrBool                        initConnect:1;
    CsrBtLogicalChannelType        logicalChannelTypeMask;
} bnepTable;
#endif /* !EXCLUDE_CSR_BT_BNEP_MODULE */

typedef struct
{
    CsrSchedQid                   appHandle;
    CsrUint8                      role;
    CsrBtCmRoleType               roleType;
}  aclRoleVars_t;

typedef struct
{
    CsrBool                       roleSwitchNeeded;
}  rnrWorkaround_t;

typedef struct
{
    CsrBtSsrSettingsUpstream      curSsrSettings;
    CsrUint8                      remoteFeatures[8];

#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(INSTALL_CM_INTERNAL_LPM)
    CsrBtSniffSettings            sniffSettings;
#endif /* INSTALL_CM_DEVICE_UTILITY && INSTALL_CM_INTERNAL_LPM */

#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(CSR_BT_INSTALL_CM_INTERNAL_ROLE_CONTROL)
    aclRoleVars_t                 roleVars;
    rnrWorkaround_t               rnrWorkaround;
#endif /* INSTALL_CM_DEVICE_UTILITY && CSR_BT_INSTALL_CM_INTERNAL_ROLE_CONTROL */

    CsrBtDeviceAddr               deviceAddr;
    link_policy_settings_t        linkPolicySettings;
    CsrSchedTid                   rfcCloseTimerId;
    CsrBtClassOfDevice            cod;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
    CsrBtLogicalChannelType       logicalChannelTypeMask;
#endif

    CsrUint32                     l2capExtendedFeatures;
    CsrUint32                     tokenRate;         /* in bytes per second */
    CsrUint32                     peakBandwidth;     /* peak bandwidth in bytes per sec */
    CsrUint32                     latency;            /* in microseconds */
    CsrUint32                     delayVariation;    /* in microseconds */
    CsrUint16                     flushTo;
    CsrUint16                     manufacturerName;
    CsrUint16                     lmpSubversion;
    CsrUint16                     lsto;
    CsrUint16                     interval;
#ifndef CSR_BT_EXCLUDE_HCI_QOS_SETUP
    CsrSchedQid                   aclQosSetupAppHandle;
#endif
    hci_qos_type_t                serviceType;
    CsrUint8                      role;
    CsrUint16                     encryptType;
    CsrBool                       roleChecked:1;
    CsrBool                       remoteFeaturesValid:1;
    CsrBool                       incoming:1;
#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(INSTALL_CM_INTERNAL_LPM)
    CsrBool                       appControlsLowPower:1;
#endif
    CsrBool                       unsolicitedQosSetup:1;
    CsrBool                       aclRequestedByApp:1;
#ifdef CSR_BT_INSTALL_CM_QHS_PHY_SUPPORT
    CsrBool                       qhsPhyConnected:1;        /* Indicates whether QHS mode is supported on this connection */
#endif
#ifdef CSR_BT_INSTALL_CM_SWB_DISABLE_STATE
    CsrBool                       swbDisabled:1;            /* Indicates if SWB codec negotiation is disabled with the device. */
#endif /* CSR_BT_INSTALL_CM_SWB_DISABLE_STATE */
#ifdef CSR_BT_LE_ENABLE
    CsrBool                       gattConnectionActive:1;
#endif
#ifdef CSR_TARGET_PRODUCT_VM
    CsrBool                       aclLockedForPeer:1;       /* ACL has been locked for Peer L2CAP Connections. */
#endif
    CsrUint8                      lmpVersion;
    CsrUint8                      mode;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
    CsrUint8                      noOfGuaranteedLogicalChannels;
#endif

#ifdef EXCLUDE_CSR_BT_SC_MODULE
    CsrBool                       bondRequired:1;           /* Indicates if application wants to bond or not */
#endif
}  aclTable;

typedef struct
{
    aclTable                      aclVar[NUM_OF_ACL_CONNECTION];
    CsrUint8                      index;
#ifdef INSTALL_CM_DEVICE_UTILITY
    CsrUint8                      requestedRole;
    CsrBool                       roleSwitchPerformed:1;
#if defined(CSR_BT_INSTALL_CM_ROLE_SWITCH_CONFIG) || defined(CSR_BT_INSTALL_CM_PRI_ALWAYS_SUPPORT_MASTER_ROLE)
    CsrBool                       alwaysSupportMasterRole:1;
#endif
#endif /* INSTALL_CM_DEVICE_UTILITY */

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
    CsrBool                       doMssBeforeScoSetup:1;
#endif
    CsrBool                       doMssBeforeRnr:1;
} roleVariables;

typedef struct dmCacheParamEntry
{
    struct dmCacheParamEntry     *next;
    struct dmCacheParamEntry     *prev;
    CsrBtDeviceAddr               deviceAddr;
    CsrUint16                     clockOffset;
    page_scan_mode_t              pageScanMode;
    page_scan_rep_mode_t          pageScanRepMode;
} dmCacheParamEntry;

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
typedef struct
{
    bnepTable                     connectVar[CSR_BT_MAX_NUM_OF_SIMULTANEOUS_BNEP_CONNECTIONS];
    bnepTable                    *indexPtr;
    CsrUint24                     classOfDevice;
    CsrUint8                      connectTypeInProgress;
    CsrSchedQid                   appHandle;
    BNEP_CONNECT_REQ_FLAGS        connectReqFlags;
#if defined(INSTALL_CM_DEVICE_UTILITY) || !defined(EXCLUDE_CSR_BT_BNEP_MODULE)
    CsrUint8                      roleSwitchReqIndex;
#endif
    CsrBool                       cancelConnect:1;
    CsrUint32                     connectState;
} bnepVariables;
#endif /* !EXCLUDE_CSR_BT_BNEP_MODULE */

typedef struct
{
    CsrBtUuid32                   serviceHandle;
    CsrUint8                      serverChannel;
} sdcResult_t;

#ifndef CSR_BT_CM_SDC_RESULT_COUNT_MAX
#define CSR_BT_CM_SDC_RESULT_COUNT_MAX      0x0C  /* The max number of UUIDs that can be requested in a
                                                     service search pattern is 12 as defined by spec */
#endif

#define CSR_BT_CM_SDC_RESULT_COUNT_BITS     (((CSR_BT_CM_SDC_RESULT_COUNT_MAX - 1) / 8) + 1)

#define CSR_BT_CM_SDC_RESULT_IS_VALID(_valid, _index)               \
    CSR_BIT_IS_SET((_valid)[(_index)/8], (_index) % 8)

#define CSR_BT_CM_SDC_RESULT_VALID_SET(_valid, _index)              \
    CSR_BIT_SET((_valid)[(_index)/8], (_index) % 8)

#define CSR_BT_CM_SDC_RESULT_VALID_UNSET(_valid, _index)            \
    CSR_BIT_UNSET((_valid)[(_index)/8], (_index) % 8)

typedef struct
{
    CsrUint8                      valid[CSR_BT_CM_SDC_RESULT_COUNT_BITS];
    CsrUint8                      resultListSize;
    sdcResult_t                   resultList[1];
} sdcList_t;

typedef struct
{
    CsrBtDeviceAddr               deviceAddr;
    CsrBtUuid32                   serviceHandle;
    CsrSchedQid                   appHandle;
    CsrUint8                      serverChannel;
} sdcResultItem;

typedef struct
{
    CsrBtDeviceAddr               deviceAddr;
    CsrSchedQid                   appHandle;
    CsrUint8                      index;
} sdcResultIndexItem;

typedef struct
{
    CsrCmnListElm_t               elem;

    CsrBtDeviceAddr               deviceAddr;

    union
    {
        CsrBtUuid32 *uuid32;
        CsrBtUuid128 *uuid128;
        CsrUint8 *uuidSet;
    } uuid;

    CsrSchedQid                   appHandle;

    CsrUint8                      uuidCount;
    CsrUint8                      assignedServiceHandle; /* Used internally when valid service handle is unavailable from remote */

    CsrUint8                      uuidType:2;
    cmSdcSearchAttrInfo          *attrInfoList;
    cmSdcSearchServiceInfo        serviceInfoList[1];
    CsrBool                       obtainServerChannels:1;
    CsrBool                       dmOpenResult:1;
    CsrBool                       readAttrDirect:1;
    CsrBool                       extendedProtocolDescriptorList:1;
    CsrBool                       extendedUuidSearch:1;

    sdcList_t                    *searchPtrArray[1];
} sdcSearchElement;

typedef struct
{
    filter_type_t                type;
    filter_condition_type_t      conditionType;
    CONDITION_T                  condition;
} eventFilterStruct;

typedef struct
{
    CsrBtDeviceAddr               keyMissingDeviceAddr;
    CsrSchedTid                   keyMissingTimerId;
    CsrUint16                     keyMissingId;
    hci_return_t                  dmSmAccessIndStatus;
} rebond_t;

#define CSR_BT_CM_DM_QUEUE_UNLOCK           ((CsrBtCmPrim) 0xFFFF)
#define CSR_BT_CM_DM_QUEUE_LOCKED(_dm)      ((_dm)->lockMsg != CSR_BT_CM_DM_QUEUE_UNLOCK)

typedef struct
{
    CsrUint16                     scanInterval; /* Scan interval for central */
    CsrUint16                     scanWindow; /* Scan window for central */
    CsrUint16                     connIntervalMin; /* Connection interval minimum */
    CsrUint16                     connIntervalMax; /* Connection interval maximum */
    CsrUint16                     connLatency; /* Default connection latency */
    CsrUint16                     supervisionTimeout; /* Default connection supervision timeout */
    CsrUint16                     connLatencyMax; /* Max acceptable connection latency */
    CsrUint16                     supervisionTimeoutMin; /* Min acceptable supervision timeout */
    CsrUint16                     supervisionTimeoutMax; /* Max acceptable supervision timeout */
} leConnParams_t;

typedef struct
{
    CsrMessageQueueType            *saveQueue;
    CsrBtCmStateDm                  state:6;
    CsrBool                         cancel:1;
    CsrBool                         disableInquiryScan:1;
    CsrBool                         disablePageScan:1;
    CsrBool                         fallbackPerformed:1;
#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(INSTALL_CM_INTERNAL_LPM)
    CsrBool                         appControlsAllLowPower:1;
#endif
    CsrBool                         forceSniffSettings:1;
    CsrBool                         scanEnabled:1;
    CsrBool                         scanIntervalChanged:1;
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE
    CsrBool                         pagingInProgress:1;
#endif

#ifdef CSR_BT_INSTALL_CM_WRITE_COD
    CsrBool                         writingCod:1;
#endif

#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(CSR_BT_INSTALL_CM_CACHE_PARAMS)
    CsrBool                         readingName:1;
#endif /* INSTALL_CM_DEVICE_UTILITY && CSR_BT_INSTALL_CM_CACHE_PARAMS */

#ifdef CSR_BT_INSTALL_CM_CACHE_PARAMS
    CsrCmnListSimple_t              dmCacheParamTable;      /* List of dmCacheParamEntry */
#endif /* CSR_BT_INSTALL_CM_CACHE_PARAMS */

    CsrBtCmPrim                     lockMsg;
    CsrSchedQid                     appHandle;

    CsrUint24                       inquiryAccessCode;
    CsrUint8                        inquiryAppState:2;
    CsrUint8                        inquiryDmState:3;
    CsrSchedQid                     inquiryAppHandle;
    CsrUint8                        maxResponses;   /* Maximum number of responses, after which inquiry process is terminated */
    CsrUint8                        inquiryTimeout; /* Maximum amount of time for the inquiry, before it is terminated*/
    CsrUint8                        rfcConnIndex;
#ifdef CSR_BT_INSTALL_CM_DUT_MODE
    CsrBool                         deviceUnderTest;
#endif
    CsrBtClassOfDevice              serviceCod;
    CsrBtClassOfDevice              majorCod;
    CsrBtClassOfDevice              minorCod;
    CsrBtClassOfDevice              pendingCod;

    CsrUint16                       hciRevision;
    CsrUint16                       manufacturerName;         /* defined in LMP */
    CsrUint16                       lmpSubversion;
    link_policy_settings_t          defaultLinkPolicySettings;
    CsrUint8                        lmpVersion;
    hci_version_t                   hciVersion;

#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(INSTALL_CM_INTERNAL_LPM)
    CsrBtSniffSettings              defaultSniffSettings;
#endif /* INSTALL_CM_DEVICE_UTILITY && INSTALL_CM_INTERNAL_LPM */

    CsrBtCmPrim                     pendingFilterRequest;
    CsrUint8                        maxEventFilters;
    eventFilterStruct               filterInProgress;
    eventFilterStruct               *eventFilters;

    page_scan_mode_t             pageScanMode;
    page_scan_rep_mode_t         pageScanRepMode;
    CsrBtDeviceAddr                 cacheTargetDev;
    CsrSchedTid                   cacheFlushTimerId;
    CsrUint8                        numberOfCachePools;
    CsrUint16                       seqNumber;
#ifndef EXCLUDE_CSR_BT_SCO_MODULE
    CsrBtDeviceAddr               scoConnectAddr;
#endif
    hci_connection_handle_t       pcmAllocationTable[MAX_PCM_SLOTS];
    CsrUint24                     codWrittenToChip;
    CsrUint8                      currentChipScanMode;
    CsrUint8                      pendingChipScanMode;
    CsrUint16                     pagescanInterval;
    CsrUint16                     pagescanWindow;
    CsrUint8                      pagescanType;
    CsrUint16                     inquiryscanInterval;
    CsrUint16                     inquiryscanWindow;
    CsrUint8                      inquiryscanType;
    CsrBtDeviceAddr               readNameDeviceAddr;
    CsrUint8                      retryCounter;
    rebond_t                     rebond;
#ifdef CSR_BT_INSTALL_CM_AUTO_EIR
    localEirData_t                  *localEirData;
#endif
    CsrUint8                        lmpSuppFeatures[8];
    CsrInt8                         inquiryTxPowerLevel;
    CsrUint8                        lowInquiryPriorityLevel;
    CsrUint16                       inquiryMask;
    CsrUint32                       scanTime;
    CsrSchedTid                     scanTimerId;
    CsrUint16                       origInqScanInterval;
    CsrUint16                       origInqScanWindow;
    CsrUint16                       origPageScanInterval;
    CsrUint16                       origPageScanWindow;
    CsrBtDeviceAddr                 detachAddr;
#ifdef CSR_BT_LE_ENABLE
    CsrBtAddressType                detachAddrType;
#endif

    CsrBtDeviceAddr                 operatingBdAddr;
    CsrUtf8String                  *localName;

#ifdef CSR_BT_LE_ENABLE
    leConnParams_t                  connParams;
#endif
    CsrBtCmPlayer                   activePlayer;
} dmVariables;


/* Low energy cache */
#ifdef CSR_BT_LE_ENABLE
#define CSR_BT_CM_LE_FEATURE_SUPPORT_PRIVACY(_leVar)                        \
    CSR_BT_LE_LOCAL_FEATURE_SUPPORTED((_leVar)->localLeFeatures,            \
                                      CSR_BT_LE_FEATURE_LL_PRIVACY)

typedef struct leConnVarTag
{
    struct leConnVarTag          *next;

    CsrBtTypedAddr                addr;          /* Peer device address */
    DM_ACL_BLE_CONN_PARAMS_T      connParams;
    CsrBool                       master;        /* Master/Slave */
    CsrBtTypedAddr                idAddr;
#ifdef EXCLUDE_CSR_BT_SC_MODULE
    CsrBool                       bondRequired:1; /* Indicates if application wants to bond or not */
#endif
} leConnVar;

typedef struct
{
    leConnVar                    *connCache;

    struct
    {
        struct
        {
            CsrUint16             interval;
            CsrUint16             window;
        } scan;

        struct
        {
            CsrUint16             intervalMin;
            CsrUint16             intervalMax;
        } adv;
    } params;

#ifdef CSR_BT_LE_RANDOM_ADDRESS_TYPE_STATIC
    CsrBtDeviceAddr               localStaticAddr; /* Local static address to be used */
    CsrBool                       staticAddrSet;
#else
    CsrUint16                     pvtAddrTimeout; /* Private address timeout */
#endif

    CsrUint8                      scanType:2;
    CsrUint8                      advType:2;
    CsrUint8                      advChannelMap:3;
    CsrUint8                      scanMode:1;
    CsrUint8                      advMode:1;
    CsrBtOwnAddressType           ownAddressType:2;
    CsrUint8                      llFeaturePrivacy:1;
} leVariables;
#endif /* CSR_BT_LE_ENABLE */

typedef struct
{
    CsrMessageQueueType          *saveQueue;
    CsrBtDeviceAddr               operatingBdAddr;
    union
    {
        struct
        {
            CsrBtResultCode code;
            CsrBtSupplier supplier;
        } result;

        struct
        {
            CsrUint16 context;
            CsrUint8 registeringSrvChannel;
        } reg;
    } arg;

    CsrSchedQid                   appHandle;
    CsrBtCmPrim                   smMsgTypeInProgress;

    CsrBool                       smInProgress:1;
    CsrBool                       popFromSaveQueue:1;
    CsrUint8                      requestedMode: 2; /* This is used to identify if the mode is changed as per the requirement or not. */
} smVariables;

#define CSR_BT_CM_SDC_SEARCH_ELEM_INVALID               0xFF
#define ASSIGN_SERVICE_HANDLE(_currentElement)          ((CsrUint32)(_currentElement->assignedServiceHandle++))
#define CM_SDC_QUEUE_UNLOCK           ((CsrBtCmPrim) 0xFFFF)
#define CM_SDC_QUEUE_LOCKED(_sdc)     ((_sdc)->lockMsg != CM_SDC_QUEUE_UNLOCK)

typedef struct
{
    CsrCmnListSimple_t            sdcSearchList;            /* List of sdcSearchElement */
    CsrMessageQueueType          *saveQueue;
    CsrBtCmPrim                   lockMsg;
    CsrUint16                     attrId;
    CsrUint8                      currentElement;
    CsrUint8                      currentIndex;
    CsrUint8                      currentServiceIndex;
    CsrUint8                      uuidIndex;
    CsrUint8                      localServer;
    CsrBool                       validSearchResult:1;
    CsrBool                       aclOpenRequested:1;
    CsrBool                       sdcMsgPending:1;
    CsrBool                       cancelPending:1;
    CsrBtCmStateSdc               state:3;
    CsrBool                       exitSniffModePending:1;
} sdcVariables;

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
typedef struct
{
    CsrUint16       aclHandle; /* Identifies the ACL Link */
    CsrUint16       l2capConnectionId; /* Identifies the local L2CAP channel ID */
    CsrUint16       bitRate; /* Identifies the bit rate of the codec in kbps */
    CsrUint16       sduSize; /* Identifies the L2CAP MTU negotiated for av */
    CsrUint8        period; /* Identifies the period in ms of codec data being available for transmission */
    CsrUint8        role; /* Identifies the local device role, source or sink */
    CsrUint8        samplingFreq; /* Identifies the sampling frequency of audio codec used */
    CsrUint8        codecType; /* Identifies the codec type e.g. SBC/aptX etc */
    CsrUint8        codecLocation; /* Identifies the location of the codec on/off-chip*/
    CsrBool         start; /* identifies start/stop of av stream */
} avStreamVariables;
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

#ifdef CSR_BT_INSTALL_EXTENDED_SCANNING
typedef struct
{
    CsrSchedQid pHandle;
    CsrUint8    scanHandle;
    CsrBool     pending;
} CsrBtExtScanRegisterHandles;
#endif

#ifdef CSR_BT_INSTALL_PERIODIC_SCANNING
typedef struct
{
    CsrSchedQid pHandle;
    CsrUint16   syncHandle;
#ifdef CSR_STREAMS_ENABLE
    Source      source;
    CsrUint8    paSyncState;
#endif /* CSR_STREAMS_ENABLE */
    CsrBool     pending;
} CsrBtPeriodicScanHandles;
#endif

#ifdef CSR_BT_INSTALL_CM_HANDLE_REGISTER
#define CSR_BT_CM_AMPM_HANDLE(_cmData)              (_cmData)->ampmHandle
#define CSR_BT_CM_LE_HANDLE(_cmData)                (_cmData)->leHandle
#define CSR_BT_CM_SC_HANDLE(_cmData)                (_cmData)->scHandle
#define CSR_BT_CM_SD_HANDLE(_cmData)                (_cmData)->sdHandle
#else
#define CSR_BT_CM_AMPM_HANDLE(_cmData)              CSR_BT_AMPM_IFACEQUEUE
#define CSR_BT_CM_GATT_HANDLE(_cmData)              CSR_BT_GATT_IFACEQUEUE
#define CSR_BT_CM_SC_HANDLE(_cmData)                CSR_BT_SC_IFACEQUEUE
#define CSR_BT_CM_SD_HANDLE(_cmData)                CSR_BT_SD_IFACEQUEUE
#endif

typedef struct
{
    void                        *recvMsgP;

    dmVariables                  dmVar;
    sdcVariables                 sdcVar;
    smVariables                  smVar;
    roleVariables                roleVar;

    CsrCmnListSimple_t           subscriptions;     /* List of subscribeParms */
    CsrCmnListSimple_t          *pendingMsgs;       /* List of cmPendingMsg_t */
#ifdef CSR_BT_INSTALL_CM_AFH
    CsrCmnListSimple_t           afhMaps;           /* List of afhMapParms_t */
#endif

#ifndef CSR_TARGET_PRODUCT_VM
    CsrBool                      rfcBuild;
#endif
    CsrBtCmStateGlobal           globalState;

#ifdef CSR_BT_INSTALL_CM_HANDLE_REGISTER
    CsrSchedQid                  sdHandle;
    CsrSchedQid                  scHandle;
#ifdef CSR_AMP_ENABLE
    CsrSchedQid                  ampmHandle;
#endif /* CSR_AMP_ENABLE */
#endif

    CsrSchedQid                 isocUnicastHandle;
    CsrSchedQid                 isocHandle;
    CsrSchedQid                 pendingIsocHandle;
    void                        *callback;

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    rfcVariables                 rfcVar;
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    l2CaVariables                l2caVar;
    scVariables                  scVar;
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
    bnepVariables                bnepVar;
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */

#ifdef CSR_BT_LE_ENABLE
    leVariables                  leVar;
#ifdef CSR_BT_INSTALL_CM_HANDLE_REGISTER
    CsrSchedQid                  leHandle;
#endif
#endif /* CSR_BT_LE_ENABLE */

#ifdef CSR_BT_INSTALL_EXTENDED_ADVERTISING
    CsrSchedQid                   extAdvAppHandle[MAX_EXT_ADV_APP]; /* To send unsolicited messages */
#endif
#ifdef CSR_BT_INSTALL_EXTENDED_SCANNING
    CsrBtExtScanRegisterHandles   extScanHandles[MAX_EXT_SCAN_APP];
#ifdef CSR_STREAMS_ENABLE
    Source                        extScanSource;
    CsrUint8                      extScanState;
#endif /* CSR_STREAMS_ENABLE */
#endif
#ifdef CSR_BT_INSTALL_PERIODIC_SCANNING
    CsrBtPeriodicScanHandles      periodicScanHandles[MAX_PERIODIC_SCAN_APP];
    CsrSchedQid                   pastAppHandle; /* To send unsolicited messages */
#endif

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
    CsrBool                     cmeServiceRunning;
    avStreamVariables           avStreamVar[CSR_BT_AV_MAX_NUM_STREAMS];
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

    CsrUint8                      elementCounter;

    CmSecurityConfigOptions       options;          /* Security options configured by higher layers. */

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    CsrBool                       sendDiscRsp:1;    /* Flag to decide if CM needs to send Disconnect response. */
#endif
    CsrUint8                      advHandle;
#ifdef INSTALL_CONTEXT_TRANSFER
    CsrSchedQid                   offloadHandle;    /* Application Handle to which offload messages will be sent */ 
#endif
    CsrSchedQid                   l2caSigAppHandle; /* Application handle to which the ping indication will be sent */
} cmInstanceData_t;


#ifdef CSR_BT_GLOBAL_INSTANCE
/* CM Instance Data */
extern cmInstanceData_t  csrBtCmData;

/* Get pointer to CM Instance data */
#define CmGetInstanceDataPtr() (&csrBtCmData)
#endif

typedef void (* SignalHandlerType)(cmInstanceData_t * taskData);

/* Prototypes from csr_bt_cm_free_down.c */
void CsrBtCmFreeDownstreamMessageContents(CsrUint16 eventClass, void * message);
void CsrBtCmFreeUpstreamMessageContents(CsrUint16 eventClass, void * message);

#ifdef EXCLUDE_CSR_BT_BNEP_MODULE
#define CsrBtCmBnepRoleSwitchAllowed(cmData) TRUE
#endif

typedef CsrUint8 CmInitSeqEvent;

#define CM_INIT_SEQ_BLUECORE_ACTIVATE_TRANSPORT_CFM             ((CmInitSeqEvent) 0x00)
#define CM_INIT_SEQ_AM_REGISTER_CFM                             ((CmInitSeqEvent) 0x01)
#define CM_INIT_SEQ_READ_LOCAL_VER_INFO_CFM                     ((CmInitSeqEvent) 0x02)
#define CM_INIT_SEQ_READ_LOCAL_SUPP_FEATURES_CFM                ((CmInitSeqEvent) 0x03)
#define CM_INIT_SEQ_WRITE_PAGE_TIMEOUT_CFM                      ((CmInitSeqEvent) 0x04)
#define CM_INIT_SEQ_LE_READ_LOCAL_SUPPORTED_FEATURES_CFM        ((CmInitSeqEvent) 0x05)
#define CM_INIT_SEQ_WRITE_PAGESCAN_ACTIVITY_CFM                 ((CmInitSeqEvent) 0x06)
#define CM_INIT_SEQ_WRITE_INQUIRY_SCAN_TYPE_CFM                 ((CmInitSeqEvent) 0x07)
#define CM_INIT_SEQ_WRITE_INQUIRY_MODE_CFM                      ((CmInitSeqEvent) 0x08)
#define CM_INIT_SEQ_SET_BT_VERSION_CFM                          ((CmInitSeqEvent) 0x09)
#define CM_INIT_SEQ_LP_WRITE_ROLESWITCH_POLICY_CFM              ((CmInitSeqEvent) 0x0A)
#define CM_INIT_SEQ_WRITE_DEFAULT_LINK_POLICY_SETTINGS_CFM      ((CmInitSeqEvent) 0x0B)
#define CM_INIT_SEQ_WRITE_PAGE_SCAN_TYPE_CFM                    ((CmInitSeqEvent) 0x0C)
#define CM_INIT_SEQ_SM_INIT_CFM                                 ((CmInitSeqEvent) 0x0D)
#define CM_INIT_SEQ_READ_LOCAL_NAME_CFM                         ((CmInitSeqEvent) 0x0E)
#define CM_INIT_SEQ_WRITE_EXTENDED_INQUIRY_RESPONSE_DATA_CFM    ((CmInitSeqEvent) 0x0F)
#define CM_INIT_SEQ_WRITE_INQUIRYSCAN_ACTIVITY_CFM              ((CmInitSeqEvent) 0x10)
#define CM_INIT_SEQ_WRITE_VOICE_SETTING_CFM                     ((CmInitSeqEvent) 0x11)
#define CM_INIT_SEQ_WRITE_COD_CFM                               ((CmInitSeqEvent) 0x12)
#define CM_INIT_SEQ_WRITE_SCAN_ENABLE_CFM                       ((CmInitSeqEvent) 0x13)
#define CM_INIT_RFC_INIT_CFM                                    ((CmInitSeqEvent) 0x14)
#define CM_INIT_SYNC_REGISTER_CFM                               ((CmInitSeqEvent) 0x15)
#define CM_INIT_SM_ADD_DEVICE_CFM                               ((CmInitSeqEvent) 0x16)
#define CM_INIT_SM_ADD_DEVICE_COMPLETE                          ((CmInitSeqEvent) 0x17)
#define CM_INIT_COMPLETED                                       ((CmInitSeqEvent) 0x18)

#ifdef INSTALL_CM_DEVICE_UTILITY
CsrBool CmDuHandleAutomaticProcedure(cmInstanceData_t *cmData,
                                     CmDuAutoEvent event,
                                     void *context,
                                     CsrBtDeviceAddr *deviceAddr);
CsrBool CmDuHandleCommandComplete(cmInstanceData_t *cmData, CmDuCmdComplete command);
void CmDuAutomaticProcedureHandler(cmInstanceData_t *cmData);
void CmDuInit(cmInstanceData_t *cmData);
#else
#define CmDuHandleAutomaticProcedure(cmData,event,context,deviceAddr)       FALSE
#define CmDuHandleCommandComplete(cmData, command)                          FALSE
#define CmDuAutomaticProcedureHandler                                       NULL
#define CmDuInit(cmData)
#endif /* INSTALL_CM_DEVICE_UTILITY */

#ifdef __cplusplus
}
#endif

#endif /* ndef _CM_MAIN_H */
