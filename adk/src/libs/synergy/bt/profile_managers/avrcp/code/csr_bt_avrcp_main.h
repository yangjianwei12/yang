#ifndef CSR_BT_AVRCP_MAIN_H__
#define CSR_BT_AVRCP_MAIN_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_bt_result.h"
#include "csr_bt_profiles.h"
#include "csr_sched.h"
#include "csr_bt_usr_config.h"
#include "csr_message_queue.h"
#include "csr_bt_cm_lib.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif
#include "csr_bt_sdc_support.h"
#include "csr_list.h"
#include "csr_bt_cmn_sdc_rfc_util.h"
#include "csr_bt_cmn_sdr_tagbased_lib.h"
#include "csr_bt_cmn_sdp_lib.h"
#include "csr_bt_avrcp_cmn_data.h"
#include "csr_bt_avrcp_prim.h"
#include "csr_log_text_2.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtAvrcpLto);

/* Log suborigins */
#define CSR_BT_AVRCP_LTSO_AVRCP                 (1)
#define CSR_BT_AVRCP_LTSO_STATE                 (2)
#define CSR_BT_AVRCP_LTSO_AVRCP_IMAGING         (3)

/* Context identifier for OBEX Server L2CAP registration */
#define AVRCP_TG_IMAGING_L2CA_PSM_CONTEXT_ID     (1)

/* Application states */
#define AVRCP_STATE_APP_IDLE            (0)
#define AVRCP_STATE_APP_BUSY            (1)
#define AVRCP_STATE_APP_NUM             (2)

/* Post-connection states */
#define AVRCP_STATE_PC_VALID            (0)
#define AVRCP_STATE_PC_INVALID          (1)
#define AVRCP_STATE_PC_NUM              (2)

/***** Connection specific *****/
/* Connection states */
#define AVRCP_STATE_CONN_DISCONNECTED   (0)
#define AVRCP_STATE_CONN_PENDING        (1)
#define AVRCP_STATE_CONN_CONNECTING     (2)
#define AVRCP_STATE_CONN_CONNECTED      (3)
#define AVRCP_STATE_CONN_DISCONNECTING  (4)
#define AVRCP_STATE_CONN_CANCELLING     (5)
#define AVRCP_STATE_CONN_DISC2RECONNECT (6)

/* Connection directions */
#define AVRCP_CONN_DIR_INVALID          (0)
#define AVRCP_CONN_DIR_INCOMING         (1)
#define AVRCP_CONN_DIR_OUTGOING         (2)

/* Misc connection */
#define AVRCP_MTU_INVALID               (0)
#define AVRCP_CID_INVALID               (0)
#define AVRCP_PSM_INVALID               (0xFFFF)

/* Activate states */
#define AVRCP_STATE_ACT_ACTIVATED           (0)
#define AVRCP_STATE_ACT_DEACTIVATED         (1)
#define AVRCP_STATE_ACT_ACTIVATING          (2)
#define AVRCP_STATE_ACT_DEACTIVATING        (3)
#define AVRCP_STATE_ACT_ACTIVATE_PENDING    (4)
#define AVRCP_STATE_ACT_DEACTIVATE_PENDING  (5)

/***** SDP specific *****/
/* SDP states */
#define AVRCP_STATE_SDP_IDLE            (0)
#define AVRCP_STATE_SDP_ACTIVE          (1)
#define AVRCP_STATE_SDP_DONE            (2)
#define AVRCP_STATE_SDP_PENDING         (3)

/* AVCTP version */
#define AVCTP_VERSION                   (0x0104)

/***** Pending message states *****/
#define AVRCP_STATE_PM_IDLE             (0x00)
/* Pass-through states */
#define AVRCP_STATE_PM_PT_RELEASE_PENDING   (0x01)
#define AVRCP_STATE_PM_PT_PRESS_PENDING     (0x02)

/* Misc SDP */
#define AVRCP_SDP_INVALID_SR_HANDLE     (0)
#define AVRCP_CT_SDP_FEATURES_MASK (0x03CF) /* Mask RFA bits */
#define AVRCP_TG_SDP_FEATURES_MASK (0x01FF) /* Mask RFA bits */

/* AVRCP roles */
#define AVRCP_ROLE_INVALID              (0x00)
#define AVRCP_ROLE_TARGET               (0x01)
#define AVRCP_ROLE_CONTROLLER           (0x02)
#define AVRCP_ROLE_BOTH                 (0x03)

#define AVRCP_CHANGE_STATE_INDEX(_var, _state, _index)                  \
    do                                                                  \
    {                                                                   \
        CSR_LOG_TEXT_INFO((CsrBtAvrcpLto,                               \
                           CSR_BT_AVRCP_LTSO_STATE,                     \
                           #_var" index: %d -> %d",                     \
                           (_var),                                      \
                           (_state)));                                  \
        (_var) = (_state);                                              \
    } while (0)

#define AVRCP_CHANGE_STATE(_var, _state)                                \
    do                                                                  \
    {                                                                   \
        CSR_LOG_TEXT_INFO((CsrBtAvrcpLto,                               \
                           CSR_BT_AVRCP_LTSO_STATE,                     \
                           #_var" state: %d -> %d",                     \
                           (_var),                                      \
                           (_state)));                                  \
        (_var) = (_state);                                              \
    } while (0)

#define AVRCP_MSG_ID_INVALID                                (0x00000000)
#define AVRCP_MSG_ID_ASSIGN(msgId)                          (++(msgId))

#define CSR_BT_AVRCP_INSTANCE_ID_INVALID         (0xFF) /** Indicates that a Instance ID is invalid */

/***** Macros for handling lists *****/
/* Connection list (connList) */
#define AVRCP_LIST_CONN_ADD_FIRST(listPtr)          ((AvrcpConnInstance_t *)CsrCmnListElementAddFirst((listPtr), sizeof(AvrcpConnInstance_t)))
#define AVRCP_LIST_CONN_REMOVE(listPtr, elemPtr)    (CsrCmnListElementRemove((listPtr), (CsrCmnListElm_t *)(elemPtr)))
#define AVRCP_LIST_CONN_GET_FIRST(listPtr)          ((AvrcpConnInstance_t *)CsrCmnListGetFirst((listPtr)))
#define AVRCP_LIST_CONN_GET_ADDR(listPtr, addPtr)   ((AvrcpConnInstance_t *)CsrCmnListSearchOffsetAddr((listPtr), offsetof(AvrcpConnInstance_t, address), addPtr))
#define AVRCP_LIST_CONN_GET_AID(listPtr, id)        ((AvrcpConnInstance_t *)CsrCmnListSearchOffsetUint8((listPtr), offsetof(AvrcpConnInstance_t, appConnId), id))
#define AVRCP_LIST_CONN_GET_C_ST(listPtr, cstate)   ((AvrcpConnInstance_t *)CsrCmnListSearchOffsetUint8((listPtr), offsetof(AvrcpConnInstance_t, control.state), cstate))
#define AVRCP_LIST_CONN_GET_SDP_ST(listPtr, sstate) ((AvrcpConnInstance_t *)CsrCmnListSearchOffsetUint8((listPtr), offsetof(AvrcpConnInstance_t, sdpState), sstate))
#define AVRCP_LIST_CONN_GET_C_CID(listPtr, ccid)    ((AvrcpConnInstance_t *)CsrCmnListSearchOffsetUint32((CsrCmnList_t *)(listPtr), offsetof(AvrcpConnInstance_t, control.btConnId), ccid))
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
#define AVRCP_LIST_CONN_GET_B_CID(listPtr, bcid)    ((AvrcpConnInstance_t *)CsrCmnListSearchOffsetUint32((listPtr), offsetof(AvrcpConnInstance_t, browsing.btConnId), bcid))
#endif
/* Pending TX data list (pendingTxDataList) */
#define AVRCP_LIST_TXDATA_ADD_FIRST(listPtr)        ((AvrcpPendingData_t *)CsrCmnListElementAddFirst((listPtr), sizeof(AvrcpPendingData_t)))
#define AVRCP_LIST_TXDATA_ADD_LAST(listPtr)         ((AvrcpPendingData_t *)CsrCmnListElementAddLast((listPtr), sizeof(AvrcpPendingData_t)))
#define AVRCP_LIST_TXDATA_REMOVE(listPtr, elemPtr)  (CsrCmnListElementRemove((CsrCmnList_t *)(listPtr), (CsrCmnListElm_t *)(elemPtr)))
#define AVRCP_LIST_TXDATA_GET_FIRST(listPtr)        ((AvrcpPendingData_t *)CsrCmnListGetFirst((listPtr)))

/* TG specific Media Player list */
#define AVRCP_LIST_TG_MP_ADD_FIRST(listPtr)         ((CsrBtAvrcpTgMp *)CsrCmnListElementAddFirst((listPtr), sizeof(CsrBtAvrcpTgMp)))
#define AVRCP_LIST_TG_MP_ADD_LAST(listPtr)          ((CsrBtAvrcpTgMp *)CsrCmnListElementAddLast((listPtr), sizeof(CsrBtAvrcpTgMp)))
#define AVRCP_LIST_TG_MP_REMOVE(listPtr, elemPtr)   (CsrCmnListElementRemove((listPtr), (CsrCmnListElm_t *)(elemPtr)))
#define AVRCP_LIST_TG_MP_GET_FIRST(listPtr)         ((CsrBtAvrcpTgMp *)CsrCmnListGetFirst((listPtr)))
#define AVRCP_LIST_TG_MP_GET_COUNT(listPtr)         (CsrCmnListGetCount((listPtr)))
#define AVRCP_LIST_TG_MP_GET_ID(listPtr, id)        ((CsrBtAvrcpTgMp *)CsrCmnListSearchOffsetUint32((listPtr), offsetof(CsrBtAvrcpTgMp, mpId), id))

/* Target specific pending message list */
#define AVRCP_LIST_TG_PMSG_ADD_FIRST(listPtr)       ((AvrcpTgPendingMsgInfo_t *)CsrCmnListElementAddFirst((listPtr), sizeof(AvrcpTgPendingMsgInfo_t)))
#define AVRCP_LIST_TG_PMSG_REMOVE(listPtr, elemPtr) (CsrCmnListElementRemove((listPtr), (CsrCmnListElm_t *)(elemPtr)))
#define AVRCP_LIST_TG_PMSG_GET_FIRST(listPtr)       ((AvrcpTgPendingMsgInfo_t *)CsrCmnListGetFirst((listPtr)))
#define AVRCP_LIST_TG_PMSG_GET_MSGID(listPtr, msgIdVal) ((AvrcpTgPendingMsgInfo_t *)CsrCmnListSearchOffsetUint32((listPtr), offsetof(AvrcpTgPendingMsgInfo_t, msgId), msgIdVal))

/* Controller specific pending message list */
#define AVRCP_LIST_CT_PMSG_ADD_FIRST(listPtr)       ((AvrcpCtPendingMsgInfo_t *)CsrCmnListElementAddFirst((listPtr), sizeof(AvrcpCtPendingMsgInfo_t)))
#define AVRCP_LIST_CT_PMSG_REMOVE(listPtr, elemPtr) (CsrCmnListElementRemove((listPtr), (CsrCmnListElm_t *)(elemPtr)))
#define AVRCP_LIST_CT_PMSG_GET_FIRST(listPtr)       ((AvrcpCtPendingMsgInfo_t *)CsrCmnListGetFirst((listPtr)))
#define AVRCP_LIST_CT_PMSG_GET_TLABEL(listPtr, tl)  ((AvrcpCtPendingMsgInfo_t *)CsrCmnListSearchOffsetUint8((listPtr), offsetof(AvrcpCtPendingMsgInfo_t, tLabel), tl))
#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
#define AVRCP_LIST_CT_PMSG_GET_APPCONNID(listPtr, id) ((AvrcpCtPendingMsgInfo_t *)CsrCmnListSearchOffsetUint8((listPtr), offsetof(AvrcpCtPendingMsgInfo_t, appConnId), id))
#endif

/***** Type definitions *****/
/* Forward declarations */
struct AvrcpInstanceDataTag;
struct AvrcpConnInstanceTag;

typedef struct
{
    CsrUint8 tLabel;
    CsrUint16 psm;
}CsrBtAvrcpChannelTLabel;

/* Media player instances */
typedef struct AvrcpTgMpInstTag
{
#ifdef CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER
    struct AvrcpTgMpInstTag     *next;              /* Do not move - required by linked-list library */
    struct AvrcpTgMpInstTag     *prev;              /* Do not move - required by linked-list library */
#endif
    CsrSchedQid                  mpHandle;
    CsrUint32                    mpId;
#ifdef CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER
    CsrUint32                    notificationMask;   /* Notifications supported by the player */
    CsrUint32                    configMask;
    CsrUint8                     playStatus;         /**< Holds the play status of the MP when a control gets the MP list - temporary */

    /* PAS specific */
    CsrUint16                    pasLen;
    CsrUint8                     *pas;
#endif
    /* Player details */
    CsrUint8                     majorType;          /**< The major type of the player (\ref avrcp_tg_mp_major_types) */
    CsrUint32                    subType;            /**< The sub type of the player (\ref avrcp_tg_mp_major_types) */
    CsrBtAvrcpMpFeatureMask      featureMask;        /**< The features supported ny the media player (\ref avrcp_tg_mp_features) */
    CsrUtf8String                *playerName;        /**< Pointer to the name of the media player (NUL-terminated)  */
} CsrBtAvrcpTgMp;

typedef struct AvrcpPendingDataTag
{
    struct AvrcpPendingDataTag  *next;              /* Do not move - required by linked-list library */
    struct AvrcpPendingDataTag  *prev;              /* Do not move - required by linked-list library */
    CsrUint16                    dataLen;
    CsrUint8                     *data;
    /* FIXME: Link to the corresponding pending msg */
} AvrcpPendingData_t; /* Used for pending outgoing and ingoing data */

typedef struct
{
    CsrBtConnId                 btConnId;                /* L2CAP connection ID of the specific connection */
    l2ca_mtu_t                  mtu;                /* Max frame size reported by the remote device */
    CsrUint8                     state;
    CsrUint8                     ctTLabel;

    /* Data specific */
    CsrBool                      dataSendAllowed:1;
    CsrCmnListSimple_t           pendingTxDataList;  /* [AvrcpPendingData_t]  */
} CsrBtAvrcpConnDetails; /* Information about a single L2CAP connection */


typedef struct AvrcpCtPendingMsgInfoTag
{
    struct AvrcpCtPendingMsgInfoTag *next;              /* Do not move - required by linked-list library */
    struct AvrcpCtPendingMsgInfoTag *prev;              /* Do not move - required by linked-list library */
    CsrUint8                     tLabel;                 /* The TLabel of the latest transmitted command */
    CsrSchedTid                         timerId;
    void                        *cfmMsg;
    CsrUint8                     tmpState;                  /* Used when the CT is expecting multiple responses before sending a CFM to app */
    CsrUint16                    psm;                   /* Control or browsing channel */
    CsrSchedQid                   phandle;                /**< Where to send the confirmation/indication */
    struct AvrcpConnInstanceTag *connInst;              /* Link to associated connection instance */
#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
    CsrUint8                     appConnId;
    void                        *reqMsg;
    CsrUint16                    uidCounter;
#endif
} AvrcpCtPendingMsgInfo_t; /* List of pending controller specific messages */

typedef struct AvrcpTgPendingMsgInfoTag
{
    struct AvrcpTgPendingMsgInfoTag *next;              /* Do not move - required by linked-list library */
    struct AvrcpTgPendingMsgInfoTag *prev;              /* Do not move - required by linked-list library */
    CsrBtAvrcpPrim                 msgType;
    CsrSchedTid                         timerId;
    CsrUint32                    msgId;
    CsrUint8                     *rxData;
    CsrUint16                    psm;                   /* Control or browsing channel */
    struct AvrcpConnInstanceTag *connInst;              /* Link to associated connection instance */
} AvrcpTgPendingMsgInfo_t; /* List of pending target specific messages */


#define AVRCP_TG_OBEX_SERVER_IDLE           0
#define AVRCP_TG_OBEX_SERVER_ACTIVATED      1
#define AVRCP_TG_OBEX_SERVER_CONNECTED      2
#define AVRCP_TG_OBEX_SERVER_DEACTIVATING   3

typedef struct
{
#ifdef CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER
    /* MP specific */
    CsrBtAvrcpTgMp               *mpBrowsed;             /* Pointer to the currently browsed MP */
#endif
    CsrBtAvrcpTgMp               *mpAddressed;           /* Pointer to the currently addressed MP */

    /* SDP specific */
    CsrUint16                    ctSdpAvrcpVersion;
    CsrUint16                    ctSdpSupportedFeatures; /* Direct copy of remote SDP record */

#ifdef CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER
    /* Notification specific */
    CsrUint32                    notificationsActive;
    CsrUint32                    notificationsPBInterval;
    CsrUint8                     notiList[CSR_BT_AVRCP_NOTI_ID_COUNT];     /* List of TLabels for registered notifications */
    /* FIXME: Add timer for playback position changed */

    /* For use with getting PlayStatus for media players */
    CsrBtAvrcpTgMp               *currentPlayer;
    CsrUint32                    itemCnt;
#endif
    /* More stuff */
    CsrCmnListSimple_t           pendingMsgList;      /* [AvrcpTgPendingMsgInfo_t] */

    /* Continuing response */
    CsrUint8                     pduId;              /**< AVRCP_DATA_PDU_ID_GET_PAS_ATTRIBUTE_TEXT / AVRCP_DATA_PDU_ID_GET_PAS_VALUE_TEXT / AVRCP_DATA_PDU_ID_GET_ELEMENT_ATTRIBUTES*/
    CsrUint8                     packetType;         /**< AVRCP_DATA_MD_PACKET_TYPE_START|CONTINUE|END */

#ifdef CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER
    /* PAS - handled entirely internally */
    CsrUint8                     attIdCount;         /* GetAttributeText */
    CsrBtAvrcpPasAttId           *attId;
    CsrUint8                     valIdFromAttId;     /* GetValueText */
    CsrUint8                     valIdCount;
    CsrBtAvrcpPasValId           *valId;

    /* GetElementAttribute - information from application */
    CsrBtAvrcpTgGetAttributesRes *getAttribResPrim;
    CsrUint16                     getAttribIndex;
#endif
#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
    CsrUint8                     obexState:2;
#endif
} CsrBtAvrcpTgConnInfo; /* Information specific to a single connection and the local target */

#define AVRCP_CT_OBEX_DISCONNECTED           0
#define AVRCP_CT_OBEX_CONNECTING_OUT         1 /* State during outgoing connection */
#define AVRCP_CT_OBEX_CONNECTING_IN          2 /* State during incoming connection */
#define AVRCP_CT_OBEX_CONNECTING_GET         3 /* State during GetItemAttr/GetElemAttr/GetFolderItems/Reconnection for BIP channel*/
#define AVRCP_CT_OBEX_TRANSPORT_CONNECTED    4 /* There is no OBEX service connection but underlying L2CAP is still active */
#define AVRCP_CT_OBEX_SERVICE_CONNECTED      5
#define AVRCP_CT_OBEX_SERVICE_DISCONNECTING	 6
typedef struct
{
    CsrBtAvrcpUid               trackUid;
    CsrUint32                   playbackPos;
    CsrBtAvrcpPasAttValPair     *attValPair;
    CsrUint16                   addressedPlayerUidCounter;
    CsrUint16                   uidCounter;
    CsrUint16                   playerId;
    CsrUint8                    volume;
    CsrBtAvrcpBatteryStatus     batteryStatus;
    CsrBtAvrcpSystemStatus      systemStatus;
    CsrBtAvrcpPlaybackStatus    playbackStatus;
    CsrUint8                    attValPairCount;
} CsrBtAvrcpCtNotiParams;

typedef struct
{
    /* Notification specific */
    CsrUint32                    activeNotifications;
    CsrUint32                    tgSupportedNotifications;
    CsrUint32                    ctRequestedNotifications;
    CsrUint32                    playbackInterval;
    CsrUint32                    notiConfig;
    /* Misc */
    AvrcpCtPendingMsgInfo_t     *pendingMsgInfo;        /* For use when the CT app should reply to an indication */

    /* SDP info */
    CsrUint16                    tgSdpAvrcpVersion;
    CsrUint16                    tgSdpSupportedFeatures; /* Direct copy of remote SDP record */

    CsrUint8                     notiList[CSR_BT_AVRCP_NOTI_ID_COUNT];/* List of TLabels for registered notifications */
#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
    CsrUint16                    obexPsm; /* Obex PSM of remote cover art service */
    CsrUint8                     ctObexState;
#endif
    /* Cached notification response parameters */
    CsrBtAvrcpCtNotiParams       notiParams;

    CsrCmnListSimple_t           pendingMsgList;         /* [AvrcpCtPendingMsgInfo_t] */
} CsrBtAvrcpCtConnInfo; /* Information specific to a single connection and the local controller */

typedef struct AvrcpConnInstanceTag
{
    struct AvrcpConnInstanceTag *next;              /* Do not move - required by linked-list library */
    struct AvrcpConnInstanceTag *prev;              /* Do not move - required by linked-list library */

    /* Connection details */
    CsrBtDeviceAddr                address;            /* Address of the remote device */
    CsrUint8                     appConnId;          /* Connection ID to send to app - will start from 0 */
    CsrSchedTid                         reconnectTid;

    CsrBtAvrcpConnDetails          control;
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    CsrBtAvrcpConnDetails          browsing;
#endif
/*
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
    CsrBtAvrcpConnDetails          coverArt;
#endif
*/

    /* State details */
    CsrUint8                     sdpState;           /* Specifies whether SDP search is done, active or pending */
    CsrUint8                     connDirection;      /* Specifies the direction of the connection */

#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
    CsrBtAvrcpTgConnInfo           *tgLocal;            /* Info about local target/remote controller */
#endif

#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
    CsrBtAvrcpCtConnInfo           *ctLocal;            /* Info about local controller/remote controller */
#endif

    /* AVCTP fragmentation - only relevant for control channel */
    CsrUint16                    pendingRxDataBufferLen; /* Must not be larger than 512+AVCTP header (3) == 515 */
    CsrUint8                     *pendingRxDataBuffer;

    /* Misc */
    CsrBtAvrcpConfigSrFeatureMask remoteFeatures;
    struct AvrcpInstanceDataTag *instData;          /* Link to main instance data to avoid passing this between all functions */
    AvrcpCtPendingMsgInfo_t     *pendingNotifReg;
    CsrBool                      resetAppState:1;                    
} AvrcpConnInstance_t; /* Information related to a single remote device */

typedef struct
{
    /* Role configuration */
    CsrUint32                      configuration;

    /* SDP */
    CsrBtAvrcpConfigSrVersion      srAvrcpVersion;
    CsrBtAvrcpConfigSrFeatureMask  srFeatures;
    CsrUint32                      srHandle;
    CsrUint32                      tgMsgId;

    /* MP specific */
    CsrCmnListSimple_t             mpList;                     /* [CsrBtAvrcpTgMp] */
    CsrBtAvrcpTgMp                 *mpDefault;

    /* UID counter */
    CsrUint16                      uidCount;
#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
    /* AVRCP Cover Art OBEX Server psm */
    psm_t                      obexPsm; /* local target side psm */
#endif
} AvrcpTgMainInfo_t; /* Global (non connection specific) settings for the local target */

typedef struct
{
    /* Role configuration */
    CsrUint32                    configuration;

    /* SDP */
    CsrBtAvrcpConfigSrVersion      srAvrcpVersion;
    CsrBtAvrcpConfigSrFeatureMask  srFeatures;
    CsrUint32                    srHandle;
} AvrcpCtMainInfo_t; /* Global (non connection specific) settings for the local controller */

typedef struct AvrcpInstanceDataTag
{
    /* Control/state specific */
    CsrSchedQid                  ctrlHandle;                   /* Control application handle */
    CsrMessageQueueType         *saveQueue;                 /* Stores pending messages */
    void                        *recvMsgP;                  /* Pointer to the received message. NULL if it has been passed on */

    /* Misc */
    CsrUint32                   globalConfig;
    CsrUint16                   srAvrcpVersionHighest;     /* Highest of the local TG/CT AVRCP versions */
    CsrCmnListSimple_t          connList;                   /* [AvrcpConnInstance_t] List of active connection instances */

#ifdef INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS
    dm_security_level_t         secIncomingCont;            /* Incoming security level - control channel */
    dm_security_level_t         secOutgoingCont;            /* Outgoing security level - control channel */
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    dm_security_level_t         secIncomingBrow;            /* Incoming security level - browsing channel */
    dm_security_level_t         secOutgoingBrow;            /* Outgoing security level - browsing channel */
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
#endif /* INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS */

    /* Role specific settings */
#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
    AvrcpTgMainInfo_t           tgLocal;
#endif
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
    AvrcpCtMainInfo_t           ctLocal;
#endif

    /* Connection specific */
    CsrUint16                    mtu;
    CsrBtAvrcpPrim               pendingCtrlPrim;            /* Holds the type of a single pending control prim (connect cfm, activate cfm etc...) */

    /* SDP search specific */
    void                        *sdpSearchData;              /* Instance data for the common SDP search lib */
    CsrUint8                     sdpState;                 /* Specifies whether a SDP search is in progress or not */

    CsrUint8                     activateStateCont;          /* Specifies whether incoming control connections are being accepted */
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    CsrUint8                     activateStateBrow;          /* Specifies whether incoming browsing connections are being accepted */
#endif
#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
    CsrUint8                     activateStateCoverArt;      /* Specifies the activation status, and what should be done on completion of on going activation / deactivation, for cover art */
#endif

    /* Connection specific */
#ifdef CSR_BT_RESTRICT_MAX_PROFILE_CONNECTIONS
    CsrUint8                    numActiveAvrcpConns;        /* Number of Active AVRCP Connections */
#endif
    CsrUint8                     incomingCurrent;
    CsrUint8                     incomingMaximum:4;
    CsrUint8                     appState:4;                 /* State of the application - normally IDLE/BUSY */

    /* SR (un)registration specific */
    CsrBtAvrcpConfigReq          *srPending;
    CsrUint8                     srActiveRole:2;
    CsrBtAvrcpRoleDetails        *tgDetails;
    CsrBtAvrcpRoleDetails        *ctDetails;
    CsrBool                      restoreFlag:1;             /* Specifies whether stored messages should be restored */
    CsrBool                      searchOngoing:1;           /* Marks if the SDP search is ongoing or not. */
} AvrcpInstanceData_t;

#ifdef CSR_BT_GLOBAL_INSTANCE
extern AvrcpInstanceData_t csrBtAvrcpInstance;
#endif

/* enum for direction of AVRCP connection */
typedef enum
{
    INCOMING,
    OUTGOING,
    COLLISION
} AvrcpConDir;

/***** Prototypes *****/
/* csr_bt_avrcp_main.c */
void CsrBtAvrcpInit(void **gash);
void CsrBtAvrcpHandler(void **gash);

/* csr_bt_avrcp_cmn_cm_handlers.c */
void CsrBtAvrcpCmL2caRegisterCfmHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCmSdsRegisterCfmHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCmSdsUnregisterCfmHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCmL2caConnectAcceptCfmHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCmL2caConnectCfmHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCmL2caDisconnectIndHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCmL2caDataIndHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCmL2caDataCfmHandler(AvrcpInstanceData_t *instData);

/* csr_bt_avrcp_cmn_app_handlers.c */
void CsrBtAvrcpConfiqReqIdleState(AvrcpInstanceData_t *instData);
void CsrBtAvrcpActivateReqIdleState(AvrcpInstanceData_t *instData);
void CsrBtAvrcpConnectReqIdleState(AvrcpInstanceData_t *instData);
void CsrBtAvrcpDisconnectReqIdleBusyState(AvrcpInstanceData_t *instData);

#ifdef INSTALL_AVRCP_DEACTIVATE
void CsrBtAvrcpDeactivateReqIdleState(AvrcpInstanceData_t *instData);
void CsrBtAvrcpDeactivateCfmSend(CsrSchedQid ctrlHandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtAvrcpCmL2caCancelConnectAcceptCfmHandler(AvrcpInstanceData_t *instData);
#else
#define CsrBtAvrcpDeactivateReqIdleState                 NULL
#define CsrBtAvrcpCmL2caCancelConnectAcceptCfmHandler    NULL
#endif /* INSTALL_AVRCP_DEACTIVATE */

#ifdef INSTALL_AVRCP_CANCEL_CONNECT
void CsrBtAvrcpCancelConnectReqIdleBusyState(AvrcpInstanceData_t *instData);
#else
#define CsrBtAvrcpCancelConnectReqIdleBusyState  NULL
#endif /* INSTALL_AVRCP_CANCEL_CONNECT */

#ifdef INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS
void CsrBtAvrcpSecurityInReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpSecurityOutReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpSecurityInCfmSend(CsrSchedQid ctrlHandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtAvrcpSecurityOutCfmSend(CsrSchedQid ctrlHandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
#else
#define CsrBtAvrcpSecurityInReqHandler          NULL
#define CsrBtAvrcpSecurityOutReqHandler         NULL
#endif /* INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS */

void CsrBtAvrcpRemoteFeaturesIndSend(CsrSchedQid phandle, CsrUint8 connId, CsrBtDeviceAddr addr, CsrBtAvrcpRoleDetails *tgFeatures, CsrBtAvrcpRoleDetails *ctFeatures);
void CsrBtAvrcpConfigCfmSend(CsrSchedQid ctrlHandle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);

void CsrBtAvrcpActivateCfmSend(CsrSchedQid ctrlHandle, CsrBtResultCode resultCode,
    CsrBtSupplier resultSupplier);

void CsrBtAvrcpConnectIndSend(CsrSchedQid ctrlHandle,
                              CsrBtDeviceAddr *addr,
                              CsrUint16 frameSize,
                              CsrUint8 connId,
                              CsrBtConnId btConnId);

void CsrBtAvrcpConnectCfmSend(CsrSchedQid ctrlHandle,
                              CsrBtDeviceAddr *addr,
                              CsrUint16 frameSize,
                              CsrUint8 connId,
                              CsrBtAvrcpRoleDetails *tgDetails,
                              CsrBtAvrcpRoleDetails *ctDetails,
                              CsrBtResultCode resultCode,
                              CsrBtSupplier resultSupplier,
                              CsrBtConnId btConnId);

void CsrBtAvrcpDisconnectIndSend(CsrSchedQid ctrlHandle,
                                 CsrUint8 connId,
                                 CsrBtReasonCode reasonCode,
                                 CsrBtSupplier reasonSupplier,
                                 CsrBool localTerminated);

/* csr_bt_avrcp_cmn_utils.c */
void CsrBtAvrcpMessagePut(CsrSchedQid phandle, void *msg);
void CsrBtAvrcpUtilNewConnEstablished(AvrcpInstanceData_t *instData,
                                      AvrcpConnInstance_t *connInst,
                                      AvrcpConDir conDir);
void CsrBtAvrcpUtilConnectAccept(AvrcpInstanceData_t *instData);
CsrBool CsrBtAvrcpUtilConnectAcceptCancel(AvrcpInstanceData_t *instData);
void CsrBtAvrcpUtilSaveMessage(AvrcpInstanceData_t *instData);
void CsrBtAvrcpUtilGo2Busy(AvrcpInstanceData_t *instData, CsrBtAvrcpPrim primType);
void CsrBtAvrcpUtilGo2Idle(AvrcpInstanceData_t *instData);
CsrBool CsrBtAvrcpUtilCancelSavedMessage(AvrcpInstanceData_t *instData, CsrBtAvrcpPrim primType, CsrBtDeviceAddr *addr);
AvrcpConnInstance_t *CsrBtAvrcpUtilGetConnFromL2caCid(AvrcpInstanceData_t *instData, CsrBtConnId btConnId, CsrBtAvrcpConnDetails **connDetails);
AvrcpConnInstance_t *CsrBtAvrcpUtilConnAdd(AvrcpInstanceData_t *instData, CsrBtDeviceAddr *addr);
CsrBool CsrBtAvrcpUtilConnRemove(CsrCmnListElm_t *elem, void *data);
void CsrBtAvrcpUtilFreeConfigReq(CsrBtAvrcpConfigReq **prim);
void CsrBtAvrcpUtilConnect(AvrcpConnInstance_t *connInst);
void CsrBtAvrcpUtilDisconnect(AvrcpConnInstance_t *connInst);
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
L2CA_FLOW_T *CsrBtAvrcpUtilGetFlow(l2ca_mtu_t mtu);
void CsrBtAvrcpBrowsingDataSend(AvrcpConnInstance_t *connInst, CsrUint16 dataLen, CsrUint8 *data);
void CsrBtAvrcpDataInsertBrowsingHeader(CsrUint8 *txData, CsrUint8 tLabel, CsrUint8 cr, CsrUint8 pduId, CsrUint16 paramLen);
CsrBool CsrBtAvrcpUtilDataCheckBrowsing(CsrUint16 rxDataLen, CsrUint8 *rxData);
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

CsrBool CsrBtAvrcpUtilPendingDataRemove(CsrCmnListElm_t *elem, void *data);
void CsrBtAvrcpUtilPendingDataAddLast(CsrBtAvrcpConnDetails *conn, CsrUint16 dataLen, CsrUint8 *data);
void CsrBtAvrcpUtilPendingDataSend(AvrcpConnInstance_t *connInst, CsrBtConnId connId);

/* csr_bt_avrcp_cmn_sdp.c */
CsrBool CsrBtAvrcpSdpRegisterSR(AvrcpInstanceData_t *instData);
CsrUint16 CsrBtAvrcpSdpGenerateServiceRecord(CsrBtAvrcpRoleDetails *ptr, CsrUint8 **sr, CsrUint8 role, CsrUint16 psm);
void CsrBtAvrcpSdpSearchStart(AvrcpInstanceData_t *instData, AvrcpConnInstance_t *connInst);
void CsrBtAvrcpSdpSearchCancel(AvrcpConnInstance_t *connInst);
void CsrBtAvrcpSdpResultHandler(void                     *inst,
                        CmnCsrBtLinkedListStruct  *sdpTagList,
                        CsrBtDeviceAddr         deviceAddr,
                        CsrBtResultCode          resultCode,
                        CsrBtSupplier      resultSupplier);
void CsrBtAvrcpSdpRestartSearch(AvrcpInstanceData_t *instData);

/* csr_bt_avrcp_cmn_data.c */
void AvrcpUtilMetaDataFrag(AvrcpInstanceData_t *instData, AvrcpConnInstance_t *connInst, CsrUint16 signalLen, CsrUint8 *signalData);
void AvrcpInfoSend(AvrcpInstanceData_t *instData, AvrcpConnInstance_t *connInst, CsrUint16 payloadLen, CsrUint8 *payload);
void CsrBtAvrcpDataInsertAvctpHeader(CsrUint8 *txData, CsrUint8 tLabel, CsrUint8 cr);
void CsrBtAvrcpDataInsertAvcCommonHeader(CsrUint8 *data, CsrUint8 crType);
void CsrBtAvrcpDataInsertAvcVendorHeader(CsrUint8 *data, CsrUint8 pduId, CsrUint8 packetType, CsrUint16 paramLen);
void CsrBtAvrcpDataInsertAvcPassThroughHeader(CsrUint8 *data, CsrUint8 state, CsrUint8 opId);
void CsrBtAvrcpDataInsertAvcGroupNavigationHeader(CsrUint8 *txData, CsrUint8 state, CsrUint16 operation);
void CsrBtAvrcpControlDataSend(AvrcpConnInstance_t *connInst, CsrUint16 dataLen, CsrUint8 *data);
void CsrBtAvrcpDataVendorDataInsert(CsrUint8 *txData, CsrUint8 tLabel, CsrUint8 cr, CsrUint8 crType, CsrUint8 pduId, CsrUint16 paramLen);
void CsrBtAvrcpDataInsertControlHeader(CsrUint8 *txData,
                                       CsrUint8 tLabel,
                                       CsrUint8 cr,
                                       CsrUint8 cType,
                                       CsrUint8 subunitType,
                                       CsrUint8 subunitId,
                                       CsrUint8 opcode,
                                       CsrUint8 pduId,
                                       CsrUint16 paramLen);
void CsrBtAvrcpDataSimpleVendorFrameSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel, CsrUint8 cr, CsrUint8 crType, CsrUint8 pduId);
CsrBool CsrBtAvrcpUtilDataCheckAvctp(CsrUint16 rxDataLen, CsrUint8 *rxData);
CsrBool CsrBtAvrcpUtilDataCheckAVC(CsrUint16 rxDataLen, CsrUint8 *rxData);
CsrBool CsrBtAvrcpUtilDataCheckVendor(CsrUint16 rxDataLen, CsrUint8 *rxData);
CsrBool CsrBtAvrcpUtilDataCheckPT(CsrUint16 rxDataLen, CsrUint8 *rxData);
CsrBool CsrBtAvrcpUtilDataFragRxHandle(AvrcpConnInstance_t *connInst, CsrUint16 *rxDataLen, CsrUint8 **rxData);
void CsrBtAvrcpUtilDataFragRxFree(AvrcpConnInstance_t *connInst);

#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
/* csr_bt_avrcp_tg_app_handlers.c */
void CsrBtAvrcpTgMpRegisterReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpTgNotiResHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpTgSetVolumeResHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpTgPassThroughResHandler(AvrcpInstanceData_t *instData);

void CsrBtAvrcpTgMpRegisterCfmSend(CsrUint32 playerId, CsrSchedQid phandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtAvrcpTgMpUnregisterCfmSend(CsrUint32 playerId, CsrSchedQid phandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);

void CsrBtAvrcpTgNotiIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 notiType, CsrUint32 playbackInterval);
void CsrBtAvrcpTgPlayIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrBtAvrcpScope scope, CsrUint16 uidCounter, CsrUint8 *uid);
void CsrBtAvrcpTgPassThroughIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 opId, CsrUint8 state);
void CsrBtAvrcpTgSetVolumeIndSend(AvrcpConnInstance_t *connInst, CsrUint16 rxDataLen, CsrUint8 **rxData);
void CsrBtAvrcpTgAddToNPLIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 scope, CsrUint16 uidCounter, CsrUint8 *uid);
void CsrBtAvrcpTgGetAttributesIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrBtAvrcpTgMp *mp, CsrBtAvrcpItemAttMask attribMask, CsrUint32 maxData, CsrBtAvrcpScope scope, CsrBtAvrcpUid uid, CsrUint16 uidCounter);
/* csr_bt_avrcp_tg_msg_handlers.c */
void CsrBtAvrcpTgRejectAvcSend(AvrcpConnInstance_t *connInst, CsrUint8 *rxData, CsrUint8 rType);
void CsrBtAvrcpTgRejectAvcVendorSend(AvrcpConnInstance_t *connInst, CsrUint8 *rxData, CsrUint8 rType, CsrUint8 status, CsrUint8 tLabel, CsrUint8 pduId);
void CsrBtAvrcpTgGetElementAttributesRspSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel);
void CsrBtAvrcpTgRegisterNotificationRspSend(AvrcpConnInstance_t *connInst, CsrUint8 notiId, CsrUint8 *notiData, CsrUint8 rspType, CsrBool ApChanged);

void CsrBtAvrcpTgPlayRspSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel, CsrBtAvrcpStatus status);
void CsrBtAvrcpTgChangePathRspSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel, CsrBtAvrcpStatus status, CsrUint32 itemCount);
void CsrBtAvrcpTgSetVolumeRspSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel, CsrUint8 volume);
void CsrBtAvrcpTgGetFolderItemsRspSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel, CsrUint16 txDataLen, CsrUint8 *txData, CsrUint16 itemCount, CsrUint16 uidCounter);
void CsrBtAvrcpTgGetTotalNumberOfItemsRspSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel, CsrUint32 itemsCount, CsrUint16 uidCounter);
void CsrBtAvrcpTgAddToNPLRspSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel, CsrBtAvrcpStatus status);
void CsrBtAvrcpTgRxControlHandler(AvrcpConnInstance_t *connInst, CsrUint16 rxDataLen, CsrUint8 **rxData);
/* csr_bt_avrcp_tg_util.c */
void CsrBtAvrcpTgPendingMsgTimeout(CsrUint16 dummy, void *pendingMsg);
AvrcpTgPendingMsgInfo_t *CsrBtAvrcpTgUtilMsgQueueAdd(AvrcpConnInstance_t *connInst, psm_t psm, CsrBtAvrcpPrim msgType, CsrTime delay, CsrUint16 rxDataLen, CsrUint8 **rxData);
void CsrBtAvrcpTgUtilPendingMsgUpdate(AvrcpTgPendingMsgInfo_t *pendingMsgInfo);
CsrBtAvrcpTgMp *CsrBtAvrcpTgUtilMpListAdd(AvrcpInstanceData_t *instData, CsrBtAvrcpTgMpRegisterReq *req);
void CsrBtAvrcpTgUtilAvailableMPChanged(AvrcpInstanceData_t *instData);

void CsrBtAvrcpTgUtilContinuingRspReset(AvrcpConnInstance_t *connInst, CsrBool completeReset);
CsrBool CsrBtAvrcpTgUtilMsgQueueRemove(CsrCmnListElm_t *elem, void *data);
CsrBool CsrBtAvrcpTgUtilMpListRemove(CsrCmnListElm_t *elem, void *data);
void CsrBtAvrcpTgUtilInitConnLocal(AvrcpInstanceData_t *instData, CsrBtAvrcpTgConnInfo **tgInfo);

#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
void CsrBtAvrcpTgSetAddressedPlayerCfmSend(CsrUint32 playerId, CsrSchedQid phandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtAvrcpTgUtilMpListUpdateAddressed(AvrcpInstanceData_t *instData, CsrBtAvrcpTgMp *mp, CsrUint16 uidCounter);
void CsrBtAvrcpTgSetAddressedPlayerRspSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel, CsrBtAvrcpStatus status, CsrUint8 cType);
void CsrBtAvrcpTgSetAddressedPlayerIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrBtAvrcpTgMp *mp);

void CsrBtAvrcpTgSetAddressedPlayerResHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpTgSetAddressedPlayerReqHandler(AvrcpInstanceData_t *instData);
#else
#define CsrBtAvrcpTgSetAddressedPlayerResHandler    NULL
#define CsrBtAvrcpTgSetAddressedPlayerReqHandler    NULL
#endif /* INSTALL_AVRCP_MEDIA_PLAYER_SELECTION */


#ifdef CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS
void CsrBtAvrcpTgPasCurrentIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 attIdCount, CsrBtAvrcpPasAttId *attId);
void CsrBtAvrcpTgPasSetIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 attValPairCount, CsrBtAvrcpPasAttValPair* changedPas);
void CsrBtAvrcpTgInformDispCharsetIndSend(CsrUint8 conId, CsrUint32 playerId, CsrSchedQid mpHandle, CsrUint8 charsetCount, CsrBtAvrcpCharSet *charset);
void CsrBtAvrcpTgBatteryStatusOfCtIndSend(CsrUint8 conId, CsrSchedQid mpHandle, CsrUint32 playerId, CsrUint8 battLevel);
void CsrBtAvrcpTgPasSetCfmSend(CsrUint32 playerId, CsrSchedQid phandle, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtAvrcpTgRegisterNotificationPasRspSend(AvrcpConnInstance_t *connInst, CsrUint8 rType, CsrUint8 pairsCount, CsrBtAvrcpPasAttValPair *pairs);

void CsrBtAvrcpTgPasSetReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpTgPasSetResHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpTgPasCurrentResHandler(AvrcpInstanceData_t *instData);
#else
#define CsrBtAvrcpTgPasSetReqHandler             NULL
#define CsrBtAvrcpTgPasSetResHandler             NULL
#define CsrBtAvrcpTgPasCurrentResHandler         NULL
#endif /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
void CsrBtAvrcpTgChangePathIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrBtAvrcpFolderDirection folderDir, CsrUint8 *folderUid);
void CsrBtAvrcpTgSearchIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrCharString *text);
void CsrBtAvrcpTgGetFolderItemsIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 scope, CsrUint32 startItem, CsrUint32 endItem, CsrUint32 attribMask, CsrUint32 maxData);
void CsrBtAvrcpTgSetBrowsedPlayerIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrBtAvrcpTgMp *mp);
void CsrBtAvrcpTgGetTotalNumberOfItemsIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrUint8 scope);

void CsrBtAvrcpTgGeneralRejectBrowsingSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel, CsrUint8 status);
void CsrBtAvrcpTgNormalRejectBrowsingSend(AvrcpConnInstance_t *connInst, CsrUint8 *rxData, CsrUint8 status);

void CsrBtAvrcpTgSearchRspSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel, CsrBtAvrcpStatus status, CsrUint16 uidCount, CsrUint32 itemCount);
void CsrBtAvrcpTgGetItemAttributesRspSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel, CsrUint16 txDataLen, CsrUint8 *txData, CsrUint8 attribCount, CsrUint8 status);
void CsrBtAvrcpTgSetBrowsedPlayerRspSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel, CsrBtAvrcpStatus status, CsrUint16 uidCounter, CsrUint32 itemsCount, CsrUint8 folderDepth, CsrUint16 foldersNameLen, CsrUint8 *folderNames);
void CsrBtAvrcpTgRxBrowsingHandler(AvrcpConnInstance_t *connInst, CsrUint16 rxDataLen, CsrUint8 **rxData);
void CsrBtAvrcpTgUtilMpListUpdateBrowsed(AvrcpConnInstance_t *connInst, CsrBtAvrcpTgMp *mp);
void CsrBtAvrcpTgUtilGetFolderItemsMPListBuild(AvrcpInstanceData_t *instData, CsrUint16 *txDataLen, CsrUint8 **txData, CsrUint32 startIdx, CsrUint32 endIdx);

CsrUint8 CsrBtAvrcpTgUtilSBPFolderNamesAdd(CsrUint8 **data, CsrUint16 *dataLen, CsrCharString *folderNames);

void CsrBtAvrcpTgSearchResHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpTgChangePathResHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpTgGetFolderItemsResHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpTgSetBrowsedPlayerResHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpTgGetTotalNumberOfItemsResHandler(AvrcpInstanceData_t *instData);
#else /* ! CSR_BT_INSTALL_AVRCP_BROWSING */
#define CsrBtAvrcpTgSearchResHandler                 NULL
#define CsrBtAvrcpTgChangePathResHandler             NULL
#define CsrBtAvrcpTgGetFolderItemsResHandler         NULL
#define CsrBtAvrcpTgSetBrowsedPlayerResHandler       NULL
#define CsrBtAvrcpTgGetTotalNumberOfItemsResHandler  NULL
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

#if defined (CSR_BT_INSTALL_AVRCP_BROWSING) || defined(INSTALL_AVRCP_METADATA_ATTRIBUTES)
void CsrBtAvrcpTgGetAttributesResHandler(AvrcpInstanceData_t *instData);
#else
#define CsrBtAvrcpTgGetAttributesResHandler      NULL
#endif

void CsrBtAvrcpTgNotiCfmSend(CsrUint32 playerId, CsrSchedQid phandle, CsrBtAvrcpNotiId notiType, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtAvrcpTgNotiReqHandler(AvrcpInstanceData_t *instData);

void CsrBtAvrcpTgGetPlayStatusIndSend(AvrcpTgPendingMsgInfo_t *pendingMsgInfo, CsrBtAvrcpTgMp *mp);

#if defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) || defined(INSTALL_AVRCP_NOTIFICATIONS)
void CsrBtAvrcpTgGetPlayStatusRspSend(AvrcpConnInstance_t *connInst, CsrUint8 tLabel, CsrUint32 songLength, CsrBtAvrcpPlaybackPos songPosition, CsrBtAvrcpPlaybackStatus playStatus);
void CsrBtAvrcpTgGetPlayStatusResHandler(AvrcpInstanceData_t *instData);
#else
#define CsrBtAvrcpTgGetPlayStatusResHandler    NULL
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES || INSTALL_AVRCP_NOTIFICATIONS */

#ifndef CSR_TARGET_PRODUCT_VM
void CsrBtAvrcpTgMpUnregisterReqHandler(AvrcpInstanceData_t *instData);
#else
#define CsrBtAvrcpTgMpUnregisterReqHandler       NULL
#endif /* !CSR_TARGET_PRODUCT_VM */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
void CsrBtAvrcpTgPlayResHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpTgAddToNowPlayingResHandler(AvrcpInstanceData_t *instData);
#else
#define CsrBtAvrcpTgPlayResHandler               NULL
#define CsrBtAvrcpTgAddToNowPlayingResHandler    NULL
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
#endif /* !EXCLUDE_CSR_BT_AVRCP_TG_MODULE */

#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
/* csr_bt_avrcp_ct_app_handlers.c */
void CsrBtAvrcpCtNotiRegisterReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtPassThroughReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtSetVolumeReqHandler(AvrcpInstanceData_t *instData);

CsrBool CsrBtAvrcpCtPassThroughCmdSend(AvrcpConnInstance_t *connInst,
                                       AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                       CsrUint8 opId,
                                       CsrUint8 state);
CsrBool CsrBtAvrcpCtGetCapabilitiesCmdSend(AvrcpConnInstance_t *connInst,
                                           AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                           CsrUint8 capaType);
CsrBool CsrBtAvrcpCtRegisterNotificationCmdSend(AvrcpConnInstance_t *connInst,
                                                AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                                CsrUint8 notificationId,
                                                CsrUint32 interval);
CsrBool CsrBtAvrcpCtGroupNavigationCmdSend(AvrcpConnInstance_t *connInst,
                                           AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                           CsrUint16 operation,
                                           CsrUint8 state);
CsrBool CsrBtAvrcpCtSetAbsoluteVolumeCmdSend(AvrcpConnInstance_t *connInst,
                                             AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                             CsrUint8 volume);

CsrBtAvrcpCtNotiRegisterCfm *CsrBtAvrcpCtNotiRegisterCfmBuild(CsrUint8 connId, CsrUint32 playbackInterval,CsrBtAvrcpNotiMask notiMask);
CsrBtAvrcpCtPassThroughCfm *CsrBtAvrcpCtPassThroughCfmBuild(CsrUint8 appConnId, CsrUint8 opId, CsrUint8 state);
void CsrBtAvrcpCtNotiIndSend(AvrcpConnInstance_t *connInst, CsrUint16 rxDataLen, CsrUint8 *rxData, CsrBool compare);
CsrBtAvrcpCtSetVolumeCfm *CsrBtAvrcpCtSetVolumeCfmBuild(CsrUint8 connId);
#ifdef INSTALL_AVRCP_METADATA_ATTRIBUTES
void CsrBtAvrcpCtGetAttributesIndSend(CsrSchedQid phandle, CsrUint8 connId, CsrBtAvrcpScope scope, CsrBtAvrcpUid *uid, CsrUint8 attribCount, CsrUint16 attribDataLen, CsrUint8 *attribData, CsrUint16 offset);
#endif
void CsrBtAvrcpCtRxControlHandler(AvrcpConnInstance_t *connInst, CsrUint16 rxDataLen, CsrUint8 **rxData);

/* csr_bt_avrcp_ct_util.c */
void CsrBtAvrcpCtCompletePendingMsg(AvrcpCtPendingMsgInfo_t *pendingMsgInfo);
void CsrBtAvrcpCtPendingMsgTimeout(CsrUint16 dummy, void *pendingMsg);
void CsrBtAvrcpCtPendingMsgUpdateResult(AvrcpCtPendingMsgInfo_t *pendingMsg, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);
void CsrBtAvrcpCtUtilPendingMsgCompleteFromPsm(AvrcpConnInstance_t *connInst, CsrUint16 psm, CsrBtResultCode resultCode, CsrBtSupplier resultSupplier);

CsrBool CsrBtAvrcpCtUtilGetAvailableTLabel(AvrcpConnInstance_t *connInst, CsrBtAvrcpConnDetails *conn);
AvrcpCtPendingMsgInfo_t *CsrBtAvrcpCtUtilMsgQueueAdd(AvrcpConnInstance_t *connInst, void *cfmMsg, CsrSchedQid phandle, CsrUint16 psm);
AvrcpCtPendingMsgInfo_t *CsrBtAvrcpCtUtilMsgGetTLabel(CsrCmnList_t *pendingMsgList, CsrUint8 tl, CsrUint16 psm);
CsrBool CsrBtAvrcpCtUtilMsgQueueRemove(CsrCmnListElm_t *elem, void *data);

void CsrBtAvrcpCtResetTransactionTimer(AvrcpCtPendingMsgInfo_t* pendingMsgInfo,
                                       CsrTime time);
CsrBool CsrBtAvrcpCtResetPendingMsg(CsrBtAvrcpConnDetails *conn,
                                    AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                    CsrTime time);

CsrBtResultCode CsrBtAvrcpCtUtilCheckRemoteRdy(AvrcpConnInstance_t *connInst, CsrUint16 avrcpVersion, CsrBool control);

void CsrBtAvrcpCtUtilConvertAVCRspType(CsrUint8 rspType,
    CsrBtResultCode *resultCode, CsrBtSupplier *resultSupplier);

void CsrBtAvrcpCtUtilConvertOperationStatus(CsrUint8 rspType,
                                            CsrBtResultCode *resultCode,
                                            CsrBtSupplier *resultSupplier);

CsrBool CsrBtAvrcpCtMatchNotificationLabel(AvrcpConnInstance_t *connInst,
                                           CsrUint8 notiId,
                                           CsrUint8 tLabel);
void CsrBtAvrcpCtRegisterNextNotification(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo);
void CsrBtAvrcpCtUtilInitConnLocal(CsrBtAvrcpCtConnInfo **ctInfo);
CsrUint16 CsrBtAvrcpCtSBPFolderNamesGet(CsrUint8 **folderPathName, CsrUint8 *inputData, CsrUint8 depth);


#ifdef CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS
CsrBool CsrBtAvrcpCtPasListAttCmdSend(AvrcpConnInstance_t *connInst,
                                      AvrcpCtPendingMsgInfo_t *pendingMsgInfo);
CsrBool CsrBtAvrcpCtPasListValCmdSend(AvrcpConnInstance_t *connInst,
                                      AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                      CsrBtAvrcpPasAttId attId);
CsrBool CsrBtAvrcpCtPasGetAttTextCmdSend(AvrcpConnInstance_t *connInst,
                                         AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                         CsrUint8 attIdCount,
                                         CsrBtAvrcpPasAttId *attId);
CsrBool CsrBtAvrcpCtPasGetValTextCmdSend(AvrcpConnInstance_t *connInst,
                                         AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                         CsrBtAvrcpPasAttId attId,
                                         CsrUint8 valIdCount,
                                         CsrBtAvrcpPasValId *valId);
CsrBool CsrBtAvrcpCtPasGetCurrentValCmdSend(AvrcpConnInstance_t *connInst,
                                            AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                            CsrUint8 attIdCount,
                                            CsrBtAvrcpPasAttId *attId);
CsrBool CsrBtAvrcpCtPasSetValCmdSend(AvrcpConnInstance_t *connInst,
                                     AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                     CsrUint8 changedPasCount,
                                     CsrBtAvrcpPasAttValPair *changedPas);
CsrBool CsrBtAvrcpCtInformDispCharsetCmdSend(AvrcpConnInstance_t *connInst,
                                             AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                             CsrUint8 charsetCount,
                                             CsrBtAvrcpCharSet *charset);
CsrBool CsrBtAvrcpCtInformBatterySend(AvrcpConnInstance_t *connInst,
                                      AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                      CsrBtAvrcpBatteryStatus batStatus);

CsrBtAvrcpCtInformBatteryStatusCfm *CsrBtAvrcpCtInformBatteryStatusCfmBuild(CsrUint8 connId);
CsrBtAvrcpCtInformDispCharsetCfm *CsrBtAvrcpCtInformDispCharsetCfmBuild(CsrUint8 connId);
CsrBtAvrcpCtPasAttIdCfm *CsrBtAvrcpCtPasAttIdCfmBuild(CsrUint8 connId);
CsrBtAvrcpCtPasValIdCfm *CsrBtAvrcpCtPasValIdCfmBuild(CsrUint8 connId, CsrBtAvrcpPasAttId attId);
CsrBtAvrcpCtPasAttTxtCfm *CsrBtAvrcpCtPasAttTxtCfmBuild(CsrUint8 connId);
CsrBtAvrcpCtPasValTxtCfm *CsrBtAvrcpCtPasValTxtCfmBuild(CsrUint8 connId, CsrBtAvrcpPasAttId attId);
CsrBtAvrcpCtPasCurrentCfm *CsrBtAvrcpCtPasCurrentCfmBuild(CsrUint8 connId);
CsrBtAvrcpCtPasSetCfm *CsrBtAvrcpCtPasSetCfmBuild(CsrUint8 connId);
void CsrBtAvrcpCtPasValTxtIndSend(CsrUint8 connId, CsrBtAvrcpPasAttId attId, CsrUint16 pasDataLen, CsrUint8 *pasData, CsrSchedQid phandle);
void CsrBtAvrcpCtPasAttTxtIndSend(CsrUint8 connId, CsrUint16 pasDataLen, CsrUint8 *pasData, CsrSchedQid phandle);
void CsrBtAvrcpCtInformDispCharsetReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtPasAttTxtResHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtPasValTxtResHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtPasAttIdReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtPasValIdReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtPasAttTxtReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtPasValTxtReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtPasSetReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtPasCurrentReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtInformBatteryStatusReqHandler(AvrcpInstanceData_t *instData);
#else /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */
#define CsrBtAvrcpCtInformDispCharsetReqHandler   NULL
#define CsrBtAvrcpCtPasAttTxtResHandler           NULL
#define CsrBtAvrcpCtPasValTxtResHandler           NULL
#define CsrBtAvrcpCtPasAttIdReqHandler            NULL
#define CsrBtAvrcpCtPasValIdReqHandler            NULL
#define CsrBtAvrcpCtPasAttTxtReqHandler           NULL
#define CsrBtAvrcpCtPasValTxtReqHandler           NULL
#define CsrBtAvrcpCtPasSetReqHandler              NULL
#define CsrBtAvrcpCtPasCurrentReqHandler          NULL
#define CsrBtAvrcpCtInformBatteryStatusReqHandler NULL
#endif /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */

void CsrBtAvrcpCtRequestAbortContinuationCmdSend(AvrcpConnInstance_t *connInst,
                                                 AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                                 CsrBool proceed,
                                                 CsrUint8 pduId);

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
CsrBool CsrBtAvrcpCtSetBrowsedPlayerCmdSend(AvrcpConnInstance_t *connInst,
                                            AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                            CsrUint16 playerId);
CsrBool CsrBtAvrcpCtGetFolderItemsCmdSend(AvrcpConnInstance_t *connInst,
                                          AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                          CsrBtAvrcpScope scope,
                                          CsrUint32 startItem,
                                          CsrUint32 endItem,
                                          CsrBtAvrcpItemAttMask attributeMask);

CsrBool CsrBtAvrcpCtChangePathCmdSend(AvrcpConnInstance_t *connInst,
                                      AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                      CsrUint16 uidCount,
                                      CsrBtAvrcpFolderDirection direction,
                                      CsrBtAvrcpUid *folderUid);

CsrBool CsrBtAvrcpCtGetItemAttributesCmdSend(AvrcpConnInstance_t *connInst,
                                             AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                             CsrBtAvrcpScope scope,
                                             CsrUint16 uidCount,
                                             CsrBtAvrcpUid *itemUid,
                                             CsrBtAvrcpItemAttMask attributeMask);
CsrBool CsrBtAvrcpCtGetTotalNumberOfItemsCmdSend(AvrcpConnInstance_t *connInst,
                                    AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                    CsrBtAvrcpScope scope);

CsrBool CsrBtAvrcpCtSearchCmdSend(AvrcpConnInstance_t *connInst,
                                  AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                  CsrBtAvrcpCharSet charset,
                                  CsrUtf8String *text);

void CsrBtAvrcpCtRxBrowsingHandler(AvrcpConnInstance_t *connInst, CsrUint16 rxDataLen, CsrUint8 **rxData);

CsrBtAvrcpCtGetFolderItemsCfm *CsrBtAvrcpCtGetFolderItemsCfmBuild(CsrUint8 connId, CsrBtAvrcpScope scope, CsrUint32 startItem, CsrUint32 endItem);
CsrBtAvrcpCtChangePathCfm *CsrBtAvrcpCtChangePathCfmBuild(CsrUint8 connId);
CsrBtAvrcpCtSearchCfm *CsrBtAvrcpCtSearchCfmBuild(CsrUint8 connId);
CsrBtAvrcpCtGetTotalNumberOfItemsCfm *CsrBtAvrcpCtGetTotalNumberOfItemsCfmBuild(CsrUint8 connId, CsrBtAvrcpScope scope);
CsrBtAvrcpCtSetBrowsedPlayerCfm *CsrBtAvrcpCtSetBrowsedPlayerCfmBuild(CsrUint8 connId, CsrUint32 playerId);

void CsrBtAvrcpCtGetFolderItemsReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtSearchReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtChangePathReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtSetBrowsedPlayerReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtGetTotalNumberOfItemsReqHandler(AvrcpInstanceData_t *instData);
#else
#define CsrBtAvrcpCtGetFolderItemsReqHandler         NULL
#define CsrBtAvrcpCtSearchReqHandler                 NULL
#define CsrBtAvrcpCtChangePathReqHandler             NULL
#define CsrBtAvrcpCtSetBrowsedPlayerReqHandler       NULL
#define CsrBtAvrcpCtGetTotalNumberOfItemsReqHandler  NULL
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

#ifdef INSTALL_AVRCP_UNIT_COMMANDS
CsrBool CsrBtAvrcpCtUnitInfoCmdSend(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 pDataLen, CsrUint8 *pData);
CsrBool CsrBtAvrcpCtSubUnitInfoCmdSend(AvrcpConnInstance_t *connInst, AvrcpCtPendingMsgInfo_t *pendingMsgInfo, CsrUint16 pDataLen, CsrUint8 *pData);
CsrBtAvrcpCtUnitInfoCmdCfm *CsrBtAvrcpCtUnitInfoCmdCfmBuild(CsrUint8 connId);
CsrBtAvrcpCtSubUnitInfoCmdCfm *CsrBtAvrcpCtSubUnitInfoCmdCfmBuild(CsrUint8 connId);
void CsrBtAvrcpCtUnitInfoCmdReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtSubUnitInfoCmdReqHandler(AvrcpInstanceData_t *instData);
#else
#define CsrBtAvrcpCtUnitInfoCmdReqHandler        NULL
#define CsrBtAvrcpCtSubUnitInfoCmdReqHandler     NULL
#endif /* INSTALL_AVRCP_UNIT_COMMANDS */

#if defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) || defined(INSTALL_AVRCP_NOTIFICATIONS)
CsrBtAvrcpCtGetPlayStatusCfm *CsrBtAvrcpCtGetPlayStatusCfmBuild(CsrUint8 connId);
CsrBool CsrBtAvrcpCtGetPlayStatusCmdSend(AvrcpConnInstance_t *connInst,
                                         AvrcpCtPendingMsgInfo_t *pendingMsgInfo);
void CsrBtAvrcpCtGetPlayStatusReqHandler(AvrcpInstanceData_t *instData);
#else
#define CsrBtAvrcpCtGetPlayStatusReqHandler    NULL
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES || INSTALL_AVRCP_NOTIFICATIONS */

#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
CsrBtAvrcpCtSetAddressedPlayerCfm *CsrBtAvrcpCtSetAddressedPlayerCfmBuild(CsrUint8 connId, CsrUint32 playerId);
CsrBool CsrBtAvrcpCtSetAddressedPlayerCmdSend(AvrcpConnInstance_t *connInst,
                                              AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                              CsrUint16 playerId);
void CsrBtAvrcpCtSetAddressedPlayerReqHandler(AvrcpInstanceData_t *instData);
#else
#define CsrBtAvrcpCtSetAddressedPlayerReqHandler    NULL
#endif /* INSTALL_AVRCP_MEDIA_PLAYER_SELECTION */

#ifdef INSTALL_AVRCP_METADATA_ATTRIBUTES
CsrBool CsrBtAvrcpCtGetElementAttributesCmdSend(AvrcpConnInstance_t *connInst,
                                                AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                                CsrUint32 attributeMask);
#endif

#if defined (CSR_BT_INSTALL_AVRCP_BROWSING) || defined(INSTALL_AVRCP_METADATA_ATTRIBUTES)
CsrBtAvrcpCtGetAttributesCfm *CsrBtAvrcpCtGetAttributesCfmBuild(CsrUint8 connId,
                                                 CsrBtAvrcpScope scope, CsrBtAvrcpUid uid);
void CsrBtAvrcpCtGetAttributesReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtGetAttributesResHandler(AvrcpInstanceData_t *instData);
#else
#define CsrBtAvrcpCtGetAttributesReqHandler      NULL
#define CsrBtAvrcpCtGetAttributesResHandler      NULL
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
CsrBtAvrcpCtPlayCfm *CsrBtAvrcpCtPlayCfmBuild(CsrUint8 connId, CsrBtAvrcpScope scope, CsrBtAvrcpUid *uid);
CsrBtAvrcpCtAddToNowPlayingCfm *CsrBtAvrcpCtAddToNowPlayingCfmBuild(CsrUint8 connId, CsrBtAvrcpScope scope, CsrBtAvrcpUid *uid);

CsrBool CsrBtAvrcpCtPlayItemCmdSend(AvrcpConnInstance_t *connInst,
                                    AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                    CsrBtAvrcpScope scope,
                                    CsrUint16 uidCount,
                                    CsrBtAvrcpUid *itemUid);
CsrBool CsrBtAvrcpCtAddToNPLCmdSend(AvrcpConnInstance_t *connInst,
                                    AvrcpCtPendingMsgInfo_t *pendingMsgInfo,
                                    CsrBtAvrcpScope scope,
                                    CsrUint16 uidCount,
                                    CsrBtAvrcpUid *itemUid);
void CsrBtAvrcpCtPlayReqHandler(AvrcpInstanceData_t *instData);
void CsrBtAvrcpCtAddToNowPlayingReqHandler(AvrcpInstanceData_t *instData);
#else /* CSR_BT_INSTALL_AVRCP_BROWSING */
#define CsrBtAvrcpCtPlayReqHandler               NULL
#define CsrBtAvrcpCtAddToNowPlayingReqHandler    NULL
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
#endif /* ! EXCLUDE_CSR_BT_AVRCP_CT_MODULE */

/* Generic downsteam free function */
void CsrBtAvrcpFreeDownstreamMessageContents(CsrUint16 eventClass, void * message);

#if defined(CSR_TARGET_PRODUCT_VM) && defined(CSR_LOG_ENABLE)
CsrUint8 AvrcpUtilGetPduIdFromPacket(CsrUint8* data);
#endif /* CSR_TARGET_PRODUCT_VM && CSR_LOG_ENABLE */
#ifdef __cplusplus
}
#endif

#endif
