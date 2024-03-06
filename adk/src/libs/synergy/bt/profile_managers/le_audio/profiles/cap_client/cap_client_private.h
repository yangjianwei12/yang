/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef CAP_CLIENT_PRIVATE_H
#define CAP_CLIENT_PRIVATE_H

#include "cap_client_prim.h"
#include "cap_client_lib.h"

#include "csr_bt_gatt_client_util_lib.h"
#include "csr_bt_tasks.h"
#include "csr_bt_common.h"
#include "csr_list.h"
#include "csr_macro.h"

#include "csip.h"

#if !defined(INSTALL_LEA_UNICAST_CLIENT) && defined(INSTALL_LEA_BROADCAST_ASSISTANT)
#error "INSTALL_LEA_BROADCAST_ASSISTANT defined without INSTALL_LEA_UNICAST_CLIENT defined"
#endif

#define CAP_CLIENT_PRIM_DOWN       (0x0040)

#define CAP_CLIENT_INVALID_CID      0xFFFFFFFFu

#define CAP_CLIENT_INVALID_CIG_ID   0xFFu
#define CAP_CLIENT_INVALID_CIS_ID   0x00u
#define CAP_CLIENT_INVALID_BIG_ID   CAP_CLIENT_INVALID_CIG_ID

#define CAP_CLIENT_PACKING_SEQUENTIAL             (0x00)
#define CAP_CLIENT_PACKING_INTERLEAVED            (0x01)

/*CAP Internal Downstream Prim*/
#define CAP_CLIENT_INTERNAL_INIT_REQ                             ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0000u))
#define CAP_CLIENT_INTERNAL_ADD_NEW_DEV_REQ                      ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0001u))
#define CAP_CLIENT_INTERNAL_REMOVE_DEV_REQ                       ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0002u))
#define CAP_CLIENT_INTERNAL_INIT_STREAM_CONTROL_REQ              ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0003u))
#define CAP_CLIENT_INTERNAL_DISCOVER_STREAM_CAP_REQ              ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0004u))
#define CAP_CLIENT_INTERNAL_DISCOVER_AVAIL_AUDIO_CONTEXT_REQ     ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0005u))
#define CAP_CLIENT_INTERNAL_UNICAST_CONNECT_REQ                  ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0006u))
#define CAP_CLIENT_INTERNAL_UNICAST_START_STREAM_REQ             ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0007u))
#define CAP_CLIENT_INTERNAL_UNICAST_UPDATE_AUDIO_REQ             ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0008u))
#define CAP_CLIENT_INTERNAL_UNICAST_STOP_STREAM_REQ              ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0009u))
#define CAP_CLIENT_INTERNAL_MUTE_REQ                             ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x000Au))
#define CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ                    ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x000Bu))
#define CAP_CLIENT_INTERNAL_REGISTER_TASK_REQ                    ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x000Cu))
#define CAP_CLIENT_INTERNAL_BCAST_SRC_INIT_REQ                   ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x000Du))
#define CAP_CLIENT_INTERNAL_BCAST_SRC_CONFIG_REQ                 ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x000Eu))
#define CAP_CLIENT_INTERNAL_BCAST_SRC_START_STREAM_REQ           ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x000Fu))
#define CAP_CLIENT_INTERNAL_BCAST_SRC_STOP_STREAM_REQ            ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0010u))
#define CAP_CLIENT_INTERNAL_BCAST_SRC_REMOVE_STREAM_REQ          ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0011u))
#define CAP_CLIENT_INTERNAL_BCAST_SRC_UPDATE_STREAM_REQ          ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0012u))
#define CAP_CLIENT_INTERNAL_BCAST_ASST_START_SRC_SCAN_REQ        ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0013u))
#define CAP_CLIENT_INTERNAL_BCAST_ASST_START_SYNC_TO_SRC_REQ     ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0014u))
#define CAP_CLIENT_INTERNAL_BCAST_ASST_CANCEL_SYNC_TO_SRC_REQ    ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0015u))
#define CAP_CLIENT_INTERNAL_BCAST_ASST_TERMINATE_SYNC_TO_SRC_REQ ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0016u))
#define CAP_CLIENT_INTERNAL_BCAST_ASST_ADD_SRC_REQ               ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0017u))
#define CAP_CLIENT_INTERNAL_BCAST_ASST_MODIFY_SRC_REQ            ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0018u))
#define CAP_CLIENT_INTERNAL_BCAST_ASST_REMOVE_SRC_REQ            ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0019u))
#define CAP_CLIENT_INTERNAL_BCAST_ASST_REG_FOR_NOTIFICATION_REQ  ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x001Au))
#define CAP_CLIENT_INTERNAL_BCAST_ASST_READ_RECEIVE_STATE_REQ    ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x001Bu))
#define CAP_CLIENT_INTERNAL_BCAST_ASST_STOP_SRC_SCAN_REQ         ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x001Cu))
#define CAP_CLIENT_INTERNAL_UNLOCK_COORDINATED_SET_REQ           ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x001Du))
#define CAP_CLIENT_INTERNAL_BCAST_ASST_SET_CODE_RSP              ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x001Eu))
#define CAP_CLIENT_INTERNAL_DEREGISTER_TASK_REQ                  ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x001Fu))
#define CAP_CLIENT_INTERNAL_CSIP_READ_REQ                        ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0020u))
#define CAP_CLIENT_INTERNAL_READ_VOLUME_STATE_REQ                ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0021u))
#define CAP_CLIENT_INTERNAL_UNICAST_CIG_TEST_CONFIG_REQ          ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0022u))
#define CAP_CLIENT_INTERNAL_UNICAST_DISCONNECT_REQ               ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0023u))
#define CAP_CLIENT_INTERNAL_PENDING_OP_REQ                       ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0024u))
#define CAP_CLIENT_INTERNAL_BCAST_SRC_DEINIT_REQ                 ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0025u))
#define CAP_CLIENT_INTERNAL_SET_MICP_PROFILE_ATTRIB_HANDLES_REQ  ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0026u))
#define CAP_CLIENT_INTERNAL_INIT_OPTIONAL_SERVICES_REQ           ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0027u))
#define CAP_CLIENT_INTERNAL_SET_MIC_STATE_REQ                    ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0028u))
#define CAP_CLIENT_INTERNAL_READ_MIC_STATE_REQ                   ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0029u))
#define CAP_CLIENT_INTERNAL_UNICAST_VS_SET_CONFIG_DATA_REQ       ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0030u))
#define CAP_CLIENT_INTERNAL_SET_PARAM_REQ                        ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0x0031u))
#define CAP_CLIENT_INTERNAL_INVALID_OPERATION                    ((CapClientPrim)(CAP_CLIENT_PRIM_DOWN + 0xFFFFu))



#define CAP_CLIENT_INTERNAL_BASE                                 CAP_CLIENT_INTERNAL_INIT_REQ
/* Note: Any new internal prim added has to update CAP_CLIENT_INTERNAL_TOP value */
#define CAP_CLIENT_INTERNAL_TOP                                  CAP_CLIENT_INTERNAL_SET_PARAM_REQ

#define CAP_CLIENT_BAP       (CapClientProfile)0x01
#define CAP_CLIENT_VCP       (CapClientProfile)0x02
#define CAP_CLIENT_CSIP      (CapClientProfile)0x04
#define CAP_CLIENT_GMCS      (CapClientProfile)0x08
#define CAP_CLIENT_GTBS      (CapClientProfile)0x10
#define CAP_CLIENT_SRVC_DSC  (CapClientProfile)0x20
#define CAP_CLIENT_MICP      (CapClientProfile)0x40

typedef uint8 CapClientBool;

typedef uint32 CapClientPendingOp;
#define CAP_CLIENT_OP_NONE                              ((CapClientPendingOp)0x00000000)
#define CAP_CLIENT_BAP_DISCOVER_AUDIO_CAPABILITY        ((CapClientPendingOp)0x00000001)
#define CAP_CLIENT_BAP_UNICAST_CONNECT                  ((CapClientPendingOp)0x00000002)
#define CAP_CLIENT_BAP_DISCOVER_AUDIO_CONTEXT           ((CapClientPendingOp)0x00000003)
#define CAP_CLIENT_BAP_UNICAST_START_STREAM             ((CapClientPendingOp)0x00000004)
#define CAP_CLIENT_BAP_BROADCAST_SRC                    ((CapClientPendingOp)0x00000005)
#define CAP_CLIENT_BAP_BROADCAST_ASSISTANT              ((CapClientPendingOp)0x00000006)
#define CAP_CLIENT_BAP_ADD_PAC_RECORD                   ((CapClientPendingOp)0x00000007)
#define CAP_CLIENT_VCP_SET_VOL                          ((CapClientPendingOp)0x00000008)
#define CAP_CLIENT_VCP_MUTE                             ((CapClientPendingOp)0x00000009)
#define CAP_CLIENT_CSIP_CLEANUP                         ((CapClientPendingOp)0x0000000A)
#define CAP_CLIENT_BAP_UNICAST_AUDIO_UPDATE             ((CapClientPendingOp)0x0000000B)
#define CAP_CLIENT_VCP_INIT                             ((CapClientPendingOp)0x0000000C)
#define CAP_CLIENT_BAP_UNICAST_DISABLE                  ((CapClientPendingOp)0x0000000D)
#define CAP_CLIENT_BAP_UNICAST_RELEASE                  ((CapClientPendingOp)0x0000000E)
#define CAP_CLIENT_BAP_BASS_SCAN_START                  ((CapClientPendingOp)0x0000000F)
#define CAP_CLIENT_BAP_BASS_SYNC_START                  ((CapClientPendingOp)0x00000010)
#define CAP_CLIENT_BAP_BASS_SYNC_TERMINATE              ((CapClientPendingOp)0x00000011)
#define CAP_CLIENT_BAP_BASS_SYNC_CANCEL                 ((CapClientPendingOp)0x00000012)
#define CAP_CLIENT_BAP_BASS_ADD_SRC                     ((CapClientPendingOp)0x00000013)
#define CAP_CLIENT_BAP_BASS_MODIFY_SRC                  ((CapClientPendingOp)0x00080014)
#define CAP_CLIENT_BAP_BASS_REMOVE_SRC                  ((CapClientPendingOp)0x00000015)
#define CAP_CLIENT_BAP_BASS_REG_NOTIFY                  ((CapClientPendingOp)0x00000016)
#define CAP_CLIENT_BAP_BASS_READ_BRS                    ((CapClientPendingOp)0x00000017)
#define CAP_CLIENT_BAP_BASS_SCAN_STOP                   ((CapClientPendingOp)0x00000018)
#define CAP_CLIENT_BAP_BASS_SET_CODE                    ((CapClientPendingOp)0x00000019)
#define CAP_CLIENT_CSIP_READ_LOCK                       ((CapClientPendingOp)0x0000001A)
#define CAP_CLIENT_UNICAST_DISCONNECT                   ((CapClientPendingOp)0x0000001B)
#define CAP_CLIENT_DEV_INIT                             ((CapClientPendingOp)0x0000001C)
#define CAP_CLIENT_MICP_INIT                            ((CapClientPendingOp)0x0000001D)
#define CAP_CLIENT_MICP_MUTE                            ((CapClientPendingOp)0x0000001E)

typedef uint16 CapClientVcpPendingOp;
#define CAP_CLIENT_INTERNAL_VCP_OP_NONE                 ((CapClientVcpPendingOp)0x0000)
#define CAP_CLIENT_INTERNAL_VCP_READ_VOLUME_STATE       ((CapClientVcpPendingOp)0x0001) /* For CAP triggered read */
#define CAP_CLIENT_INTERNAL_VCP_CHANGE_VOLUME           ((CapClientVcpPendingOp)0x0002) /* For CAP triggered change volume */
#define CAP_CLIENT_INTERNAL_VCP_MUTE                    ((CapClientVcpPendingOp)0x0003) /* For CAP triggered mute */
#define CAP_CLIENT_VCP_GET_VOL                          ((CapClientVcpPendingOp)0x0004)

typedef uint16 CapClientVcpMsgType;
#define CAP_CLIENT_INTERNAL_VCP_CHANGE_VOL_REQ          ((CapClientVcpMsgType)0x0001) /* Change volume req */
#define CAP_CLIENT_INTERNAL_VCP_MUTE_REQ                ((CapClientVcpMsgType)0x0002) /* Mute req */

typedef uint8 CapClientVcpCccdType;
#define CAP_CLIENT_INTERNAL_VCP_STATE_CCCD              ((CapClientVcpCccdType)0x01) /* Volume state */
#define CAP_CLIENT_INTERNAL_VCP_FLAG_CCCD               ((CapClientVcpCccdType)0x02) /* Volume flag */

#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_300          300
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_205          205
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_155          155
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_130          130
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_120          120
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_117          117
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_108          108
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_100          100
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_97           97
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_90           90
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_80           80
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_75           75
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_60           60
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_50           50
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_45           45
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_40           40
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_30           30
#define CAP_CLIENT_STREAM_CAPABILITY_OCTETS_26           26

typedef uint8 CapClientState;
#define CAP_CLIENT_STATE_INVALID                   ((CapClientState)0x00)
#define CAP_CLIENT_STATE_INIT                      ((CapClientState)0x01)
#define CAP_CLIENT_STATE_INIT_STREAM_CTRL          ((CapClientState)0x02)
#define CAP_CLIENT_STATE_DISCOVER_SUPPORTED_CAP    ((CapClientState)0x03)
#define CAP_CLIENT_STATE_UNICAST_CONNECTED         ((CapClientState)0x04)
#define CAP_CLIENT_STATE_STREAM_STARTED            ((CapClientState)0x05)
#define CAP_CLIENT_STATE_STREAM_STOPPED            ((CapClientState)0x06)
#define CAP_CLIENT_STATE_AUDIO_UPDATED             ((CapClientState)0x07)

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
typedef uint8 CapClientBcastAssistState;
#define CAP_CLIENT_BCAST_ASST_STATE_IDLE             ((CapClientBcastAssistState)0x00)
#define CAP_CLIENT_BCAST_ASST_STATE_ADDING_SOURCE    ((CapClientBcastAssistState)0x01)
#define CAP_CLIENT_BCAST_ASST_STATE_MODIFYING_SOURCE ((CapClientBcastAssistState)0x02)
#define CAP_CLIENT_BCAST_ASST_STATE_REMOVING_SOURCE  ((CapClientBcastAssistState)0x03)
#define CAP_CLIENT_BCAST_ASST_STATE_SEND_CODES       ((CapClientBcastAssistState)0x04)
#define CAP_CLIENT_BCAST_ASST_STATE_START_SCAN       ((CapClientBcastAssistState)0x05)
#define CAP_CLIENT_BCAST_ASST_STATE_STOP_SCAN        ((CapClientBcastAssistState)0x06)
#define CAP_CLIENT_BCAST_ASST_STATE_SYNC_TO_SRC      ((CapClientBcastAssistState)0x07)
#endif

#if defined(INSTALL_LEA_BROADCAST_SOURCE) || defined(INSTALL_LEA_BROADCAST_ASSISTANT)
typedef struct
{
    uint8     opReqCount;
    uint8     successCount;
    uint8     errorCount;
}CapClientGenericCounter;
#endif

#ifdef INSTALL_LEA_UNICAST_CLIENT
#define CAP_CLIENT_PACKING_OVERLAPPING_TYPE1      (0x81)

#define CAP_ASE_STATE_RELEASED      0x07

#define CAP_CLIENT_MAX_SUPPORTED_DEVICES  2

#define CAP_CLIENT_MAX_SUPPORTED_CIGS     4

#define CAP_CLIENT_LOCK_STATE_DISABLED   0x01
#define CAP_CLIENT_LOCK_STATE_ENABLED    0x02

#define CAP_CLIENT_NO_CIG_ID_MASK   0x00
#define CAP_CLIENT_USE_CASE_MASK    CAP_CLIENT_INVALID_CID
#define CAP_CLIENT_INVALID_USECASE  CAP_CLIENT_INVALID_CIG_ID
#define CAP_CLIENT_MASK_BYTE0       0xFFFFFF00u
#define CAP_CLIENT_MASK_BYTE1       0xFFFF00FFu
#define CAP_CLIENT_MASK_BYTE2       0xFF00FFFFu
#define CAP_CLIENT_MASK_BYTE3       0x00FFFFFFu

#define CAP_CLIENT_CIS_HANDLE_SIZE  0x02

#define CAP_CLIENT_INVALID_SERVICE_HANDLE ((ServiceHandle)(0x0000))

/* For the standard LEA use cases, from the BAP spec 4.4 it was clear that each device
 * Can have one single cis which belongs to one device for different audio configurations
 * In future if this is no more valid then we need to change number of cis handles based on
 * the new audio configuration */
#define NO_OF_CIS_HANDLES_PER_DEVICE      0x01

#define CAP_CLIENT_SIRK_SIZE              (CSIP_SIRK_SIZE)
#define ENCRYPTED_SIRK                    0x00
#define PLAIN_SIRK                        0x01


#define CAP_CLIENT_CLEAR_PENDING_OP(VAL)  (VAL = 0)

#define CAP_CLIENT_VCP_VOL_FLAG_SET               0x01

#define CAP_CLIENT_VCP_VOL_STATE_PERSISTED(_VCP)  !!(_VCP->flags && CAP_CLIENT_VCP_VOL_FLAG_SET)

#define VALIDATE_INPUT_CONFIG(CONFIG)    (((CONFIG & (CONFIG - 1)) == 0) && \
                                            (CONFIG != 0))

#define CSR_BT_CAP_CLIENT_VCP_INDICATION_TIMER    (10 * CSR_SCHED_SECOND) /* 10 seconds timer for volume state indications */

#define CAP_CLIENT_PACKING_SEQUENTIAL             (0x00)
#define CAP_CLIENT_PACKING_INTERLEAVED            (0x01)

#define CAP_CLIENT_PACKING_OVERLAPPING_TYPE1      (0x81)

#define CAP_CLIENT_GET_MAX(_AB, _AC)             ((_AB > _AC) ? _AB: _AC)

typedef uint8 CapClientCisDirection;
#define CAP_CLIENT_CIS_DIR_INVALID           ((CapClientCisDirection)(0x00))
#define CAP_CLIENT_CIS_SINK_UNIDIRECTIONAL   ((CapClientCisDirection)(0x01))
#define CAP_CLIENT_CIS_SRC_UNIDIRECTIONAL    ((CapClientCisDirection)(0x02))
#define CAP_CLIENT_CIS_BIDIRECTIONAL         ((CapClientCisDirection)(0x03))


#define CAP_CLIENT_UNIDIRECTIONAL_SINK_USECASE   (CAP_CLIENT_CONTEXT_TYPE_ALERTS \
                                                  | CAP_CLIENT_CONTEXT_TYPE_EMERGENCY_ALARM \
                                                  | CAP_CLIENT_CONTEXT_TYPE_MEDIA \
                                                  | CAP_CLIENT_CONTEXT_TYPE_INSTRUCTIONAL\
                                                  | CAP_CLIENT_CONTEXT_TYPE_RINGTONE \
                                                  | CAP_CLIENT_CONTEXT_TYPE_SOUND_EFFECTS \
                                                  | CAP_CLIENT_CONTEXT_TYPE_GAME \
                                                  )

#define CAP_CLIENT_UNIDIRECTIONAL_SRC_USECASE   (CAP_CLIENT_CONTEXT_TYPE_LIVE )

#define CAP_CLIENT_BIDIRECTIONAL_USECASE        (CAP_CLIENT_CONTEXT_TYPE_CONVERSATIONAL \
                                                   | CAP_CLIENT_CONTEXT_TYPE_VOICE_ASSISTANT \
                                                     | CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC)

#define IS_SINK_USECASE_UNIDIRECTIONAL(USECASE)   ((USECASE & CAP_CLIENT_UNIDIRECTIONAL_SINK_USECASE) == USECASE)
#define IS_SRC_USECASE_UNIDIRECTIONAL(USECASE)    ((USECASE & CAP_CLIENT_UNIDIRECTIONAL_SRC_USECASE) == USECASE)
#define IS_USECASE_BIDIRECTIONAL(USECASE)         ((USECASE & CAP_CLIENT_BIDIRECTIONAL_USECASE) == USECASE)

#define UNIDIRECTIONAL_SINK_ASE_CIS(USECASE, ASE_USECASE)  (IS_SINK_USECASE_UNIDIRECTIONAL(USECASE) && \
                                                               IS_SINK_USECASE_UNIDIRECTIONAL(ASE_USECASE))

#define UNIDIRECTIONAL_SRC_ASE_CIS(USECASE, ASE_USECASE)  (IS_SRC_USECASE_UNIDIRECTIONAL(USECASE) && \
                                                               IS_SRC_USECASE_UNIDIRECTIONAL(ASE_USECASE))

#define BIDIRECTIONAL_ASE_CIS(USECASE, ASE_USECASE)  (IS_USECASE_BIDIRECTIONAL(USECASE) && \
                                                               IS_USECASE_BIDIRECTIONAL(ASE_USECASE))

#define ASE_RECONFIG(USECASE, ASE_USECASE)            UNIDIRECTIONAL_SRC_ASE_CIS(USECASE, ASE_USECASE) || \
                                                         UNIDIRECTIONAL_SINK_ASE_CIS(USECASE, ASE_USECASE) || \
                                                            BIDIRECTIONAL_ASE_CIS(USECASE, ASE_USECASE)

#define VALIDATE_USECASE(USECASE)                     (!IS_SINK_USECASE_UNIDIRECTIONAL(USECASE) && \
                                                         !IS_SRC_USECASE_UNIDIRECTIONAL(USECASE) && \
                                                             !IS_USECASE_BIDIRECTIONAL(USECASE)) 


#define CAP_CLIENT_ASES_NOT_CONFIGURED(USECASE, SRCSTREAMCAP, SINKSTREAMCAP)   (SRCSTREAMCAP == CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN) && \
                                                                              (SINKSTREAMCAP == CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN) && \
                                                                              (USECASE == CAP_CLIENT_CONTEXT_TYPE_PROHIBITED)

#define CAP_CLIENT_CIS_IS_BIDIRECTIONAL(_CISDIRECTION)           ((_CISDIRECTION == CAP_CLIENT_CIS_BIDIRECTIONAL) ? TRUE : FALSE)
#define CAP_CLIENT_CIS_IS_UNI_SINK(_CISDIRECTION)             ((_CISDIRECTION == CAP_CLIENT_CIS_SINK_UNIDIRECTIONAL) ? TRUE : FALSE)
#define CAP_CLIENT_CIS_IS_UNI_SRC(_CISDIRECTION)              ((_CISDIRECTION == CAP_CLIENT_CIS_SRC_UNIDIRECTIONAL) ? TRUE : FALSE)

#define CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(BAP, USECASE)  (BAP->bapCurrentState >> ((8*USECASE)) & 0xFF)

/* CAP Volume related procedures utility */
#define CAP_CLIENT_VOLUME_REQ_BUFFER_SIZE            0x02
#define CAP_CLIENT_VCP_BUFFER_INDEX_ZERO             0x0
#define CAP_CLIENT_VCP_BUFFER_INDEX_FIRST            0x1

#define CAP_CLIENT_VOLUME_REQ_MESSAGE_IDLE           0x00
#define CAP_CLIENT_VOLUME_REQ_MESSAGE_PROGRESS       0x01
#define CAP_CLIENT_VOLUME_REQ_MESSAGE_QUEUED         0x02
#define CAP_CLIENT_VOLUME_REQ_MESSAGE_COMPLETED      0x03

#define CAP_CLIENT_VCP_DEFAULT_VOLUME_STATE          0x00
#define CAP_CLIENT_VCP_DEFAULT_MUTE_STATE            0xFF

typedef uint32 CapClientBapState;
#define CAP_CLIENT_BAP_STATE_IDLE                  ((CapClientBapState)0x00)
#define CAP_CLIENT_BAP_STATE_DISCOVER_COMPLETE     ((CapClientBapState)0x01)
#define CAP_CLIENT_BAP_STATE_AVLBLE_AUDIO_CONTEXT  ((CapClientBapState)0x02)
#define CAP_CLIENT_BAP_STATE_CODEC_CONFIGURED      ((CapClientBapState)0x03)
#define CAP_CLIENT_BAP_STATE_QOS_CONFIGURED        ((CapClientBapState)0x04)
#define CAP_CLIENT_BAP_STATE_ENABLED               ((CapClientBapState)0x05)
#define CAP_CLIENT_BAP_STATE_STREAMING             ((CapClientBapState)0x06)
#define CAP_CLIENT_BAP_STATE_DISABLED              ((CapClientBapState)0x07)
#define CAP_CLIENT_BAP_STATE_RELEASED              ((CapClientBapState)0x08)
#define CAP_CLIENT_BAP_STATE_DUMMY_DATAPTH         ((CapClientBapState)0x09)
#define CAP_CLIENT_BAP_STATE_INVALID               ((CapClientBapState)0xFF)

extern uint8 cigID[CAP_CLIENT_MAX_SUPPORTED_CIGS];

/* Allow the application to set the data path ID.
 * If this is not set in config, then default value used is HCI */
#ifndef DATAPATH_ID
#define DATAPATH_ID  DATAPATH_ID_HCI
#endif

typedef struct capClientCigElement
{
    struct capClientCigElement*  next;
    struct capClientCigElement*  prev;
    uint8                        cigId;
    uint8                        latency;
    uint8                        cisHandleCount;
    uint8                        configuredSrcAses;
    uint8                        configureSinkAses;
    uint8                        sinkLocCount;
    uint8                        srcLocCount;
    uint8                        state;
    CapClientCisDirection        cigDirection;
    CapClientContext             context;
    CapClientAudioLocation       sinkLoc;
    CapClientSreamCapability     sinkConfig;
    uint32                       sinkMinPdelay;
    uint32                       sinkMaxPdelay;
    CapClientSreamCapability     srcConfig;
    CapClientAudioLocation       sourceLoc;
    uint32                       srcMinPdelay;
    uint32                       srcMaxPdelay;
    CapClientUnicastConnectParamV1 unicastParam;
    uint8                        dataPath;
}CapClientCigElem;


typedef struct capClientProfileTaskListElem
{
    struct                           capClientProfileTaskListElem* next;
    struct                           capClientProfileTaskListElem* prev;
    AppTask                          profileTask;
    CapClientSreamCapability         sinkConfig;
    CapClientSreamCapability         srcConfig;
    uint8                            numOfSrcAses;
    uint8                            vendorConfigDataLen;
    uint8                            *vendorConfigData;
    CapClientCigElem                 *activeCig;
    CapClientUnicastConnectParamV1  *unicastParam;
}CapClientProfileTaskListElem;

typedef struct capBapAseElem
{
    struct capBapAseElem           *next;
    struct capBapAseElem           *prev;
    uint8                          aseId;
    uint8                          cisId;
    uint8                          state;
    bool                           inUse;
    bool                           removeDatapath;
    uint16                         cisHandle;
    CapClientContext               useCase;
    CapClientCisHandleDirection    datapathDirection;
    CapClientAudioLocation         audioLocation;
    CapClientCigElem*              cig;
}BapAseElement;

typedef struct
{
    uint16                       scanHandle;
    uint8                        sourceId;
    uint8                        advSid;
    TYPED_BD_ADDR_T              addrt;
    AppTask                      reportToTask;
}BroadcastAssistantInst;

typedef struct cap_bap_inst
{
    struct cap_bap_inst          *next;
    struct cap_bap_inst          *prev;
    CapClientResult               recentStatus;
    ServiceHandle                 groupId;
    CsrCmnList_t                  sinkAseList;
    CsrCmnList_t                  sourceAseList;
    uint8                         sinkAseCount;
    uint8                         sourceAseCount;
    uint8                         asesInUse;
    uint8                         sinkAsesToBeConfigured;
    uint8                         srcAsesToBeConfigured;
    uint8                         releasedAses;
    uint8                         rtn;  /*  retransmission effort */
    uint8                         cigId;
    uint8                         cisCount;
    uint8                         datapathReqCount;
    uint8                         serverSinkSourceStreamCount;
    uint16                        *cisHandles; /* Used for dummy instances */
    uint16                        transportLatency;
    CapClientVsMetadata           *vsMetadata;
    uint32                        cid; /* cid */
    uint32                        bapHandle; /* cid */
    CapClientAvailableContext     availableAudioContext;
#if defined(INSTALL_LEA_BROADCAST_SOURCE) || defined(INSTALL_LEA_BROADCAST_ASSISTANT)
    CapClientGenericCounter       operationCounter;
#endif
    BapHandles                    *bapData;
    BroadcastAssistantInst        *bass;
#ifdef CAP_CLIENT_NTF_TIMER
    CsrSchedTid                    ntfTimer;
#endif
    CapClientBapState              bapCurrentState;
} BapInstElement;

/* Find bap_inst by CID*/

typedef struct csip_inst
{
  struct csip_inst                *next;
  struct csip_inst                *prev;
  CapClientResult                 recentStatus;
  CapClientPrim                   currentOperation;
  uint32                          cid;
  bool                            discoveryComplete;
  ServiceHandle                   csipHandle;
  TP_BD_ADDR_T                    addr;
  ServiceHandle                   groupId;
  uint8                           lock;
  uint8                           pendingLock;
  uint8                           rank;
  GattCsisClientDeviceData        *csipData;
} CsipInstElement;

/* Find csip_inst by CID*/

typedef struct vcp_inst
{
  struct vcp_inst           *next;
  struct vcp_inst           *prev;
  CapClientResult            recentStatus;
  uint32                     cid;
  ServiceHandle              groupId;
  ServiceHandle              vcpHandle;
  uint8                      muteState;
  uint8                      expMuteState; /* Expected mute state in volume state ind */
  uint8                      volumeState;
  uint8                      expVolumeState; /* Expected volume level in volume state ind */
  uint8                      flags;
  uint8                      changeCounter;
  uint8                      expChangeCounter; /* Expected change counter in volume state ind */
  GattVcsClientDeviceData    *vcsData;
} VcpInstElement;

typedef struct vcp_command_buffer
{
    void*                    capVcpMsg;
    uint8                    capVcpMsgState;
    CapClientVcpMsgType      capVcpMsgType;
}VcpCmdBuffer;

typedef struct micp_inst
{
  struct micp_inst           *next;
  struct micp_inst           *prev;
  CapClientResult            recentStatus;
  uint32                     cid;
  ServiceHandle              groupId;
  ServiceHandle              micpHandle;
  CapClientMicState          micValue;
  GattMicsClientDeviceData   *micsData;
} MicpInstElement;


typedef struct
{
    /* Lib task is added as the first entry, reason being for report parsing lib task must be the first entry in the strcuture similar to other profiles,
     * Although this will increase cap size by 4 bytes due to extra alignment */
    AppTask                       libTask;
    AppTask                       appTask;
    CsrCmnList_t                  bapList;
    CsrCmnList_t                  csipList;
    CsrCmnList_t                  vcpList;
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    CsrCmnList_t                  *micpList;
#endif
    CsrCmnList_t                  profileTaskList;
    CsrCmnList_t                  capClientMsgQueue;
    CsrCmnList_t                  cigList;
    ServiceHandle                 groupId;
    CapClientPendingOp            pendingOp;
    CapClientVcpPendingOp         vcpPendingOp;
    uint8                         role;
    bool                          forceRemoveCig;
    uint8                         totalCisCount;
    uint8                         setSize;
    uint8                         currentDeviceCount;
    uint8                         sirkType;
    uint8                         volumeSetting;
    uint8                         requiredSinks;
    uint8                         requiredSrcs;
    uint8                         numOfSourceAses;
    uint8                         *sirk;
    CapClientState                capState;
    uint32                        requestCid;
    CapClientPublishedCapability  pendingCap;
    CapClientContext              useCase;
    uint8*                        metadata;
    uint8                         metadataLen;
    bool                          doRelease;
    bool                          stopComplete;
    bool                          profileListSort;
    uint8*                        broadcastCode;
    uint8                         prevCisId;
    uint8                         codecVersionNum;
    CapClientCisDirection         cigDirection;
    CapClientCigConfigMode        cigConfigMode;
    CapClientQhsConfig            cigConfig;
    CapClientCigElem*             activeCig;
    VcpCmdBuffer                  capVcpCmdQueue[CAP_CLIENT_VOLUME_REQ_BUFFER_SIZE];
    uint8                         capVcpCmdCount;
    CapClientBcastAssistState     bcastAsstState;
    CsrSchedTid                   timerId; /* Timer Identifier for VCP Indications */
    uint8                         groupVolume;
    uint8                         groupMute;
#ifdef CAP_CLIENT_NTF_TIMER
    bool                          capNtfTimeOut;
#endif
    CsrCmnList_t                  *cisHandlesList;
    CsrCmnList_t                  *sourceRecordList;
    CsrCmnList_t                  *sinkRecordList;
    CapClientResult               csipStatus;
}CapClientGroupInstance;

typedef struct CapClientHandleElemTag
{
    struct CapClientHandleElemTag     *next;
    struct CapClientHandleElemTag     *prev;
    ServiceHandle                     profileHandle;
}CapClientHandleElem;
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_SOURCE
typedef struct
{
    uint8                      bisIndex;
    uint8                      targetLatency;
    uint16                     bisHandle;               /* Has to be assigned post Enable operation */
    CapClientSreamCapability   config;                  /*<! Setting which needs to be configured per BIS*/
    uint32                     audioLocation;
}CapClientLocalBisInfo;

typedef struct
{
    uint8                         numBis;
    uint8                         targetLatency;           /*<! low latency or high reliability */
    uint8                         lc3BlocksPerSdu;         /*<! Default value = 1*/
    CapClientContext              useCase;                 /* Context type of Audio/Usecase */
    uint16                        metadataLen;             /* Total length of vendor metadata */
    uint8                         *metadata;               /* LTV format */
    CapClientSreamCapability      config;                  /*<! Setting which needs to be configured*/
    CapClientLocalBisInfo         bisInfo[BAP_MAX_NUMBER_BIS];
}CapClientLocalSubGroupInfo;

typedef struct BcastSrcInst
{
    struct BcastSrcInst *next;
    struct BcastSrcInst *prev;
    uint32                        bcastSrcProfileHandle;
    AppTask                       appTask;
    uint8                         bigId;
    uint8                         numSubGroup;
    uint8                         numBcastParam;
    CapClientBigConfigMode        mode;
    CapClientQhsBigConfig         qhsConfig;
    CapClientLocalSubGroupInfo*   subGroupInfo;             /* required to Establish datapaths*/
    uint32                        bigSyncDelay;
    CapClientBigParam             *bcastBigParam;
    CapClientBcastConfigParam     *bcastParam;
    CapClientGenericCounter       bcastSrcOpCnt;
}BroadcastSrcInst;
#endif /* #ifdef INSTALL_LEA_BROADCAST_SOURCE */

typedef struct
{
#ifdef INSTALL_LEA_UNICAST_CLIENT
    CsrCmnList_t                  capClientGroupList;
    AppTask                       vcpProfileTask;
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    AppTask                       micpProfileTask;
    uint8                         micpRequestCount;
#endif
    bool                          streaming;
    bool                          addNewDevice;
    bool                          isSink;
    bool                          discoverSource;
    uint8                         deviceCount;
    uint8                         discoveryRequestCount;
    uint8                         bapRequestCount;
    uint8                         vcpRequestCount;
    uint8                         vcpIndicationCount; /* Total expected indications after volume change triggered from CAP */
    uint8                         csipRequestCount;
    CapClientCsipType             csipReadType;
    ServiceHandle                 activeGroupId;
#endif /* INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_SOURCE
    CsrCmnList_t                  *capClientBcastSrcList;
#endif /* INSTALL_LEA_BROADCAST_SOURCE */

    AppTask                       appTask;
    AppTask                       profileTask;
    uint8                         cigId;
} CAP_INST;

#ifdef INSTALL_LEA_UNICAST_CLIENT
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
typedef void (*CapClientMicpReqSender)(MicpInstElement* micp, CAP_INST *inst, uint32 cid);
#endif
typedef void (*CapClientMessageHandler)(CAP_INST* inst, void* msg, CapClientGroupInstance* cap);

typedef void (*CapClientBcastAsstReqSender)(BapInstElement* bap, uint32 cid, AppTask appTask);

typedef struct capClientProfileMsgQueueElem
{
    struct                         capClientProfileMsgQueueElem* next;
    struct                         capClientProfileMsgQueueElem* prev;
    CapClientProfileTaskListElem   *task;
    void                           *capMsg;
    CapClientPrim                  MessageType;
    bool                           confirmationRecieved;
    uint8                          ExpectedIndCount;
    CapClientMessageHandler        handlerFunc;
}CapClientProfileMsgQueueElem;

typedef struct CapPacRecord
{
    struct CapPacRecord* next;
    struct CapPacrecord* prev;
    uint8                     codecId;
    uint8                     recordNum;
    CapClientSreamCapability  streamCapability;
    uint8                     lc3EpcVersionNum;
    CapClientAudioChannelCount channelCount;
    uint8                     supportedMaxCodecFramesPerSdu;
    CapClientFrameDuration    frameDuaration;
    uint16                    minOctetsPerCodecFrame;
    uint16                    maxOctetsPerCodecFrame;
    CapClientContext          preferredAudioContext;
    CapClientContext          streamingAudioContext;
    bool                      isLc3Epc;
    uint8                     metadataLen;
    uint8                     *metadata;
}CapClientBapPacRecord;

typedef struct cisData
{
    ServiceHandle            cisHandle[NO_OF_CIS_HANDLES_PER_DEVICE];
    uint8                    cisId;
}cisData;

typedef struct capClientCisHandlesListElem
{
    struct           capClientCisHandlesListElem* next;
    struct           capClientCisHandlesListElem* prev;
    cisData          *cisHandlesData;
}capClientCisHandlesListElem;


/* Get Service Instance data*/

#define CAP_CLIENT_GET_GROUP_INST_DATA(_PHANDLE) \
                      ServiceHandleGetInstanceData(_PHANDLE)

#define CAP_CLIENT_ADD_GROUP_SERVICE_HANDLE_INST(_INST) \
                      ServiceHandleNewInstance(((void**)&(_INST)), \
                                                           sizeof(CapClientGroupInstance))
#define CAP_CLIENT_REMOVE_GROUP_INST_DATA(_PHANDLE) \
                      ServiceHandleFreeInstanceData(_PHANDLE)

/* Cap group Instance Helper Api's */
#define CAP_CLIENT_FIND_CAP_GROUPID(_LIST, _GROUPHANDLE) \
             CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientSearchGroupId, &_GROUPHANDLE)

/* Add Element Instances to the list*/

#define CAP_CLIENT_ADD_CAP_GROUP_INST(_LIST)  \
                CsrCmnListElementAddLast(_LIST, sizeof(CapClientHandleElem))

#define CAP_CLIENT_GET_HANDLE_ELEM_FROM_GROUPID(_LIST, _GROUPID) \
                CsrCmnListSearch(_LIST, capClientGetHandleElmFromGroupId, &_GROUPID)

#define CAP_CLIENT_ADD_BAP_INST(_LIST)  \
                CsrCmnListElementAddLast((CsrCmnList_t*)&(_LIST), sizeof(BapInstElement))

#define CAP_CLIENT_BAP_ADD_ASE_ELEM(_LIST)  \
                CsrCmnListElementAddLast((CsrCmnList_t*)(_LIST), sizeof(BapAseElement))

#define CAP_CLIENT_BAP_REMOVE_ASE_ELEM(_LIST, _ELEM) \
                   CsrCmnListElementRemove((CsrCmnList_t*)_LIST, (CsrCmnListElm_t*)_ELEM)


#define CAP_CLIENT_GET_ASE_ELEM_FROM_ASEID(_LIST, ASEID) \
                CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientGetbapAseFromAseId, &ASEID)

#define CAP_CLIENT_GET_ASE_ELEM_FROM_CISHANDLE(_LIST, HANDLE) \
          CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientGetbapAseFromCisHandle, &HANDLE)


#define CAP_CLIENT_GET_ASE_ELEM_FROM_CISID(_LIST, HANDLE) \
          CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientGetbapAseFromCisId, &HANDLE)

#define CAP_CLIENT_ADD_VCP_INST(_LIST)  \
                CsrCmnListElementAddLast((CsrCmnList_t*)&(_LIST), sizeof(VcpInstElement))

#define CAP_CLIENT_ADD_CSIP_INST(_LIST)  \
                CsrCmnListElementAddLast((CsrCmnList_t*)&(_LIST), sizeof(CsipInstElement))

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
#define CAP_CLIENT_ADD_MICP_INST(_LIST)  \
                CsrCmnListElementAddLast((CsrCmnList_t*)(_LIST), sizeof(MicpInstElement))
#endif

/* CAP client Message Queue Operations*/

#define CAP_CLIENT_MSG_QUEUE_ADD(_LIST)  \
                CsrCmnListElementAddLast((CsrCmnList_t*)(_LIST), sizeof(CapClientProfileMsgQueueElem))

#define CAP_CLIENT_MSG_QUEUE_GET_FRONT(_LIST) \
                 CsrCmnListElementGetFirst((CsrCmnList_t*)(_LIST))

#define CAP_CLIENT_IS_MSG_QUEUE_EMPTY(_LIST) ((&_LIST)->count == 0)

#define CAP_CLIENT_MSG_QUEUE_REMOVE(_LIST)  capClientMessageQueueRemove(_LIST)

/* CAP Profile Task List Operations*/

#define CAP_CLIENT_ADD_TASK_TO_LIST(_LIST)  \
                CsrCmnListElementAddLast((CsrCmnList_t*)&(_LIST), sizeof(CapClientProfileTaskListElem))

#define CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(_LIST, APPHANDLE) \
           (CapClientProfileTaskListElem*)CsrCmnListSearch(_LIST, capClientGetTaskElemFromAppHandle, (void*)&APPHANDLE)

#ifdef INSTALL_LEA_UNICAST_CLIENT
#define CAP_CLIENT_REMOVE_TASK_FROM_LIST(_LIST, APPHANDLE) capClientRemoveTaskFromList(&(_LIST), APPHANDLE)
#endif /* INSTALL_LEA_UNICAST_CLIENT */

/* Search element instances based on cid/profileHandle */

#define CAP_CLIENT_GET_GROUP_ELEM_FROM_CID(_LIST, CID) \
        (CapClientGroupInstance*)CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientGetCapGroupFromCid, &CID)

#define CAP_CLIENT_GET_CSIP_ELEM_FROM_PHANDLE(_LIST, _PHANDLE) \
           (CsipInstElement*)CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientGetCsipFromPhandle, &_PHANDLE)

#define CAP_CLIENT_GET_VCP_ELEM_FROM_PHANDLE(_LIST, _PHANDLE) \
               (VcpInstElement*)CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientGetVcpFromPhandle, &_PHANDLE)

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
#define CAP_CLIENT_GET_MICP_ELEM_FROM_PHANDLE(_LIST, _PHANDLE) \
               (MicpInstElement*)CsrCmnListSearch((CsrCmnList_t*)(_LIST), capClientGetMicpFromPhandle, &_PHANDLE)
#endif

#define CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(_LIST, _PHANDLE) \
              (BapInstElement*)CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientGetBapFromPhandle, &_PHANDLE)

#define CAP_CLIENT_GET_BAP_ELEM_FROM_GROUPID(_LIST, _PHANDLE) \
                         (BapInstElement*)CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientGetBapFromGroupId, &_PHANDLE)


#define CAP_CLIENT_GET_VCP_ELEM_FROM_CID(_LIST, CID) \
           (VcpInstElement*)CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientGetVcpFromCid, &CID)

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
#define CAP_CLIENT_GET_MICP_ELEM_FROM_CID(_LIST, CID) \
                           (MicpInstElement*)CsrCmnListSearch((CsrCmnList_t*)(_LIST), capClientGetMicpFromCid, &CID)
#endif

#define CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(_LIST, CID) \
                (CsipInstElement*)CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientGetCsipFromCid, &CID)

#define CAP_CLIENT_SORT_CSIP_LIST(_LIST) \
       CsrCmnListSort(&(_LIST), capClientSortCsipProfileList)

#define CAP_CLIENT_SORT_BAP_LIST(_LIST) \
       CsrCmnListSort(&(_LIST), capClientSortBapProfileList)

#define CAP_CLIENT_SORT_VCP_LIST(_LIST) \
       CsrCmnListSort(&(_LIST), capClientSortVcpProfileList)

#define CAP_CLIENT_ADD_CIS_HANDLES(_LIST) \
                CsrCmnListElementAddLast((_LIST), sizeof(capClientCisHandlesListElem))

#define CAP_CLIENT_GET_CIS_HANDLES_FROM_CID(_LIST, CID) \
                (CsipInstElement*)CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientGetCisHandlesFromCid, &CID)

/* CIG related Operations */

#define CAP_CLIENT_ADD_CIG(_LIST) \
                CsrCmnListElementAddLast((_LIST), sizeof(CapClientCigElem))

#define CAP_CLIENT_REMOVE_CIG(_LIST, _ELEM) \
                   CsrCmnListElementRemove((CsrCmnList_t*)_LIST, (CsrCmnListElm_t*)_ELEM)

#define CAP_CLIENT_GET_CIG_FROM_CONTEXT(_LIST, _CONTEXT) \
                (CapClientCigElem*)CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientGetCigFromContext, &_CONTEXT)

#define CAP_CLIENT_GET_CIG_FROM_CIGID(_LIST, _CIGID) \
                (CapClientCigElem*)CsrCmnListSearch((CsrCmnList_t*)&(_LIST), capClientGetCigFromCigId, &_CIGID)
#endif /* INSTALL_LEA_UNICAST_CLIENT */

/*Cap Broadcast List operations */
#ifdef INSTALL_LEA_BROADCAST_SOURCE
#define CAP_CLIENT_ADD_BCAST_SRC(_LIST) \
                CsrCmnListElementAddLast((_LIST), sizeof(BroadcastSrcInst))

#define CAP_CLIENT_REMOVE_BCAST_SRC(_LIST, _ELEM) \
                   CsrCmnListElementRemove((CsrCmnList_t*)_LIST, (CsrCmnListElm_t*)_ELEM)

#define CAP_CLIENT_GET_BCAST_SRC_FROM_BIGID(_LIST, _BIGID) \
                (BroadcastSrcInst*)CsrCmnListSearch((_LIST), capClientGetBcastSrcFromBigId, &_BIGID)

#define CAP_CLIENT_GET_BCAST_SRC_FROM_PHANDLE(_LIST, _PHANDLE) \
                (BroadcastSrcInst*)CsrCmnListSearch((_LIST), capClientGetBcastSrcFromPhandle, &_PHANDLE)

#define CAP_CLIENT_GET_BCAST_SRC_FROM_APPTASK(_LIST, _APPTASK) \
                (BroadcastSrcInst*)CsrCmnListSearch((_LIST), capClientGetBcastSrcFromAppTask, &_APPTASK)

#define CAP_CLIENT_GET_BCAST_SRC_PEEK_FIRST(_LIST)  (_LIST->first)
#endif /* INSTALL_LEA_BROADCAST_SOURCE */

/* Counter Operations */
#define CAP_CLIENT_REQ_COMPLETE(_COUNTER)   (_COUNTER.opReqCount == 0)


#define MAKE_CAP_CLIENT_MESSAGE(TYPE)   TYPE *message = (TYPE*)CsrPmemZalloc(sizeof(TYPE))

#define CAP_CLIENT_QHS_CONFIGURED(_MODE)  ((_MODE & CAP_CLIENT_CIG_CONFIG_MODE_QHS) == CAP_CLIENT_CIG_CONFIG_MODE_QHS)

#define CapClientMessageSend(TASK, ID, MSG) { \
                       MSG->type = ID; \
                       CsrSchedMessagePut(TASK, CAP_CLIENT_PRIM, MSG);\
                       }

/* Internal downstream Primitives */

typedef struct
 {
     CapClientPrim           type;           /*!< CAP_CLIENT_INTERNAL_INIT_REQ */
     AppTask                 appTask;        /*!< AppTask */
     CapClientRole           role;           /*!< Commander/Initiator */
     CapClientInitData       *initData;      /*!< Initialization data */
} CapClientInternalInitReq;

typedef struct
{
    CapClientPrim            type;            /*!< CAP_CLIENT_INTERNAL_ADD_NEW_DEV_REQ */
    ServiceHandle            groupId;         /*!< CAP handle */
    CapClientInitData        *initData;       /*!< Initialization data */
    bool                     discoveryComplete;   /*!< discovery complete */
    uint32                   sirk[4];         /*!<SIRK */
} CapClientInternalAddNewDevReq;

typedef struct
{
    CapClientPrim            type;            /*!< CAP_CLIENT_INTERNAL_REMOVE_DEV_REQ */
    uint32                   cid;             /*!<  Cid*/
    ServiceHandle            groupId;         /*!< CAP Handle */
} CapClientInternalRemoveDevReq;

typedef struct
{
    CapClientPrim            type;            /*!< CAP_CLIENT_INTERNAL_INIT_STREAM_CONTROL_REQ */
    ServiceHandle            groupId;         /*!< CAP group Handle */
} CapClientInternalInitStreamControlReq;

typedef struct
{
    CapClientPrim                 type;            /*!< CAP_CLIENT_INTERNAL_SET_MICP_PROFILE_ATTRIB_HANDLES_REQ */
    AppTask                       profileTask;     /*!< AppTask */
    ServiceHandle                 groupId;         /*!< CAP group Handle */
    uint32                        cid;             /*!<  Cid*/
    GattMicsClientDeviceData      *micsHandles;    /* MICS handles information */
} CapClientInternalSetMicpAttribHandlesReq;

typedef struct
{
    CapClientPrim                 type;            /*!< CAP_CLIENT_INTERNAL_INIT_OPTIONAL_SERVICES_REQ */
    ServiceHandle                 groupId;         /*!< CAP group Handle */
    CapClientOptionalServices     servicesMask;    /*!< Bitmask indicating optional services */
} CapClientInternalInitOptionalServicesReq;

typedef struct
{
    CapClientPrim                 type;        /*!< CAP_CLIENT_INTERNAL_DISCOVER_STREAM_CAP_REQ */
    ServiceHandle                 groupId;     /*!< CAP group Handle */
    CapClientPublishedCapability  attribute;   /*!< CSIS Device Propetrties */
} CapClientInternalDiscoverStreamCapReq;

typedef struct
{
    CapClientPrim            type;        /*!< CAP_CLIENT_INTERNAL_DISCOVER_AVAIL_AUDIO_CONTEXT_REQ */
    ServiceHandle            groupId;     /*!< CAP group Handle */
} CapClientInternalDiscoverAvailAudioContextReq;

typedef struct
{
    CapClientPrim              type;             /*!< CAP_CLIENT_INTERNAL_UNICAST_CONNECT_REQ */
    AppTask                    profileTask;      /*!< App Handle */
    ServiceHandle              groupId;          /*!< CAP group Handle */
    CapClientSreamCapability   sinkConfig;       /*!<SINK Codec Configuration */
    CapClientSreamCapability   srcConfig;        /*!<SOURCE Codec Configuration */
    CapClientTargetLatency     highReliability;  /*!< target Latency */
    CapClientContext           useCase;           /*!< Audio Context */
    uint8                      numOfMic;          /*!< No of Mics to be configured */
    CapClientAudioLocation     sinkAudioLocations;    /*!< Sink Audio Locations */
    CapClientAudioLocation     srcAudioLocations;    /*!< Source Audio Locations */
    CapClientCigConfigMode     cigConfigMode;     /*!< Default or QHS mode */
    CapClientQhsConfig         cigConfig;         /*!< Valid if cigConfigMode is set to QHS */
} CapClientInternalUnicastConnectReq;

typedef struct
{
    CapClientPrim            type;          /*!< CAP_CLIENT_INTERNAL_UNICAST_START_STREAM_REQ */
    AppTask                  profileTask;   /*!< App Handle */
    ServiceHandle            groupId;       /*!< CAP group Handle */
    CapClientContext         useCase;       /*!< Audio Context */
    uint8                    metadataLen;   /*!< Metadatalen */
    uint8                    *metadataParam; /*!< CCID LIST / Vendor Specific metadata */
} CapClientInternalUnicastCommonReq;

typedef struct
{
    CapClientPrim            type;          /*!< CAP_CLIENT_INTERNAL_UNICAST_DISCONNECT_REQ */
    AppTask                  profileTask;   /*!< App Handle */
    ServiceHandle            groupId;       /*!< CAP group Handle */
    CapClientContext         useCase;       /*!< Audio Context */
} CapClientInternalUnicastDisconnectReq;

typedef CapClientInternalUnicastCommonReq CapClientInternalUnicastStartStreamReq;
typedef CapClientInternalUnicastCommonReq CapClientInternalUnicastUpdateAudioReq;

typedef struct
{
    CapClientPrim            type;         /*!< CAP_CLIENT_INTERNAL_UNICAST_STOP_STREAM_REQ */
    AppTask                  profileTask;  /*!< App Handle */
    ServiceHandle            groupId;      /*!< CAP group Handle */
    bool                     doRelease;    /*!< Release ASEs if TRUE, disable otherwise */
} CapClientInternalUnicastStopStreamReq;

typedef struct
{
    CapClientPrim            type;         /*!< CAP_CLIENT_INTERNAL_MUTE_REQ */
    AppTask                  profileTask;  /*!< App Handle */
    ServiceHandle            groupId;      /*!< CAP group Handle */
    bool                     muteState;    /*!< device Mute State */
} CapClientInternalMuteReq;

typedef struct
{
    CapClientPrim            type;         /*!< CAP_CLIENT_INTERNAL_SET_MIC_STATE_REQ */
    AppTask                  profileTask;  /*!< App Handle */
    ServiceHandle            groupId;      /*!< CAP group Handle */
    uint32                   cid;          /*!< BtConnID */
    bool                     micState;     /*!< device Mute State */
} CapClientInternalSetMicStateReq;

typedef struct
{
    CapClientPrim            type;         /*!< CAP_CLIENT_INTERNAL_CHANGE_VOLUME_REQ */
    AppTask                  profileTask;  /*!< App Handle */
    ServiceHandle            groupId;      /*!< CAP group Handle */
    uint8                    volumeState;  /*!< device volume State */
} CapClientInternalChangeVolumeReq;

typedef struct
{
    CapClientPrim            type;         /*!< CAP_CLIENT_INTERNAL_REGISTER_TASK_REQ */
    ServiceHandle            groupId;      /*!< CAP group Handle */
    AppTask                  profileTask;  /*!< App Handle */
} CapClientInternalRegisterTaskReq;


typedef struct
{
    CapClientPrim            type;         /*!< CAP_CLIENT_INTERNAL_DEREGISTER_TASK_REQ */
    ServiceHandle            groupId;      /*!< CAP group Handle */
    AppTask                  profileTask;  /*!< App Handle */
} CapClientInternalDeRegisterTaskReq;

typedef struct
{
    CapClientPrim                       type;         /*!< CAP_CLIENT_INTERNAL_CSIP_READ_REQ */
    ServiceHandle                       groupId;      /*!< CAP group Handle */
    uint32                              cid;          /*!< BtConnID */
    CapClientCsipType                   csipCharType; /*!< CSIP READ characteristic type */
}CapClientInternalCsipReadReq;

typedef struct
{
    CapClientPrim                       type;         /*!< CAP_CLIENT_INTERNAL_READ_VOLUME_STATE_REQ */
    ServiceHandle                       groupId;      /*!< CAP group Handle */
    uint32                              cid;          /*!< BtConnID */
    AppTask                             profileTask;  /*!< App Handle */
}CapClientInternalReadVolumeStateReq;

typedef CapClientInternalReadVolumeStateReq CapClientInternalReadMicStateReq;

typedef struct
{
    CapClientPrim              type;             /*!< CAP_CLIENT_INTERNAL_UNICAST_CIG_TEST_CONFIG_REQ */
    AppTask                    profileTask;      /*!< App Handle */
    ServiceHandle              groupId;          /*!< CAP group Handle */
    CapClientContext           useCase;           /*!< Audio Context */
    CapClientCigTestConfig     *cigConfig;         /*!< CIG Test mode params */
} CapClientInternalUnicastCigTestConfigReq;

typedef CapClientInternalUnicastCommonReq CapClientInternalUnicastSetVsConfigDataReq;

typedef struct
{
    CapClientPrim            type;      /*!< CAP_CLIENT_INTERNAL_BCAST_SRC_INIT_REQ */
    AppTask                  appTask;   /*!< App Handle */
} CapClientInternalBcastSrcInitReq;

typedef struct
{
    CapClientPrim            type;                    /*!< CAP_CLIENT_INTERNAL_BCAST_SRC_DEINIT_REQ */
    uint32                   bcastSrcProfileHandle;   /*!< Broadcast Source Handle */
} CapClientInternalBcastSrcDeinitReq;

typedef struct
{
    CapClientPrim            type;                    /*!< CAP_CLIENT_INTERNAL_BCAST_CONFIG_SRC_REQ */
    uint32                   bcastSrcProfileHandle;   /*!< Broadcast Source Handle */
    uint32                   presentationDelay;       /*!< in Millisecond */
    uint8                    ownAddress;
    uint8                    numSubgroup;
    CapClientBigConfigMode   mode;
    CapClientBigSubGroup     *subgroupInfo;
    CapClientBcastInfo       *broadcastInfo;
    CapClientQhsBigConfig    *qhsConfig;              /*!< This will be NULL if mode is default */
} CapClientInternalBcastSrcConfigReq;

typedef struct
{
    CapClientPrim            type;                   /*!< CAP_CLIENT_INTERNAL_BCAST_SRC_ENABLE_REQ */
    uint32                   bcastSrcProfileHandle;  /*!< Broadcast Source Handle */
    bool                     encryption;             /*!< TRUE/FALSE  */
    uint8                    *broadcastCode;         /*!< Broadcast Code */
} CapClientInternalBcastStartStreamSrcReq;

typedef struct
{
    CapClientPrim                 type;                   /*!< CAP_CLIENT_INTERNAL_BCAST_SRC_ENABLE_REQ */
    uint32                        bcastSrcProfileHandle;  /*!< Broadcast Source Handle */
    CapClientBcastSetupDatapath   *datapathParams;
} CapClientInternalBcastSetupDatapathReq;

typedef struct
{
    CapClientPrim            type;
    uint32                   bcastSrcProfileHandle;   /*!< Broadcast Source Handle */
} CapClientInternalBcastCommonReq;

typedef CapClientInternalBcastCommonReq CapClientInternalBcastStopStreamSrcReq;
typedef CapClientInternalBcastCommonReq CapClientInternalBcastRemoveStreamSrcReq;

typedef struct
{
    CapClientPrim            type;                   /*!< CAP_CLIENT_INTERNAL_BCAST_UPDATE_AUDIO_SRC_REQ */
    uint32                   bcastSrcProfileHandle;  /*!< Broadcast Source Handle */
    uint8                    numSubgroup;
    uint8                    metadataLen;            /*!< MetadataLen */
    uint8                    *metadata;              /*!< CCID List/ VS metadata */
    CapClientContext         useCase;                /*!< Audio Context */
} CapClientInternalBcastUpdateStreamSrcReq;

typedef struct
{
    CapClientPrim             type;        /*!< CAP_CLIENT_INTERNAL_BCAST_ASST_START_SRC_SCAN_REQ */
    ServiceHandle             groupId;      /*!< CAP handle */
    CapClientBcastSrcLocation bcastSrcType;
    CapClientBcastType        bcastType;
    uint32                    cid;          /*!< BtConnID */
    AppTask                   profileTask;  /*!< App Handle */
    CapClientContext          filterContext;
    uint8                     scanFlags;
    uint8                     ownAddressType;
    uint8                     scanningFilterPolicy;
} CapClientInternalBcastAsstStartSrcScanReq;

typedef struct
{
    CapClientPrim             type;        /*!< CAP_CLIENT_INTERNAL_BCAST_ASST_STOP_SRC_SCAN_REQ */
    ServiceHandle             groupId;      /*!< CAP handle */
    AppTask                   profileTask;
    uint16                    scanHandle;
    uint32                    cid;          /*!< BtConnID */
} CapClientInternalBcastAsstStopSrcScanReq;

typedef struct
{
    CapClientPrim            type;          /*!< CAP_CLIENT_BCAST_ASST_SYNC_TO_SRC_START_REQ */
    ServiceHandle            groupId;       /*!< CAP Handle*/
    AppTask                  profileTask;   /*!< App Handle */
    TYPED_BD_ADDR_T          addrt;         /*!< Broadcast Src Address */
    uint8                    advSid;        /*!< ADV handle */
} CapClientInternalBcastAsstSyncToSrcStartReq;

typedef struct
{
    CapClientPrim            type;          /*!< CAP_CLIENT_INTERNAL_BCAST_ASST_TERMINAL_SYNC_TO_SRC_REQ */
    ServiceHandle            groupId;       /*!< CAP group Handle */
    AppTask                  profileTask;   /*!< App Handle */
    uint16                   syncHandle;    /*!< SyncHandle of PA train synced already */
} CapClientInternalBcastAsstTerminateSyncToSrcReq;

typedef struct
{
    CapClientPrim            type;          /*!< CAP_CLIENT_INTERNAL_BCAST_ASST_SYNC_TO_SRC_CANCEL_REQ */
    ServiceHandle            groupId;       /*!< CAP group Handle */
    AppTask                  profileTask;   /*!< App Handle */
} CapClientInternalBcastAsstSyncCommonReq;

typedef CapClientInternalBcastAsstSyncCommonReq CapClientInternalBcastAsstSyncToSrcCancelReq;

typedef struct
{
    CapClientPrim            type;           /*!< CAP_CLIENT_INTERNAL_BCAST_ASST_ADD_SRC_REQ */
    ServiceHandle            groupId;        /*!< CAP group Handle */
    uint32                   cid;            /*!< BtConnID , if set to zero add for all devices*/
    AppTask                  profileTask;    /*!< App Handle */
    BD_ADDR_T                sourceAddrt;    /*!< Src Address */
    uint8                    advertiserAddressType; /*!< Broadcast Src Address  Type*/
    uint8                    sourceAdvSid;  /*! Advertising SID */
    CapClientPaSyncState     paSyncState;   /*! PA Synchronization state */
    bool                     srcCollocated; /*!< is source Collocated */
    uint16                   syncHandle;    /*!< SyncHandle of PA or advHandle of
                                                      collocated Broadcast src */
    uint16                   paInterval;    /*!< Periodic Advertisement Interval */
    uint32                   broadcastId;   /*!< Broadcast ID */
    uint8                    numbSubGroups; /*! Number of subgroups */
    CapClientSubgroupInfo    *subgroupInfo[BAP_MAX_SUPPORTED_NUM_SUBGROUPS];
} CapClientInternalBcastAsstAddSrcReq;

typedef struct
{
    CapClientPrim            type;           /*!< CAP_CLIENT_INTERNAL_BCAST_ASST_MODIFY_SRC_REQ */
    ServiceHandle            groupId;        /*!< CAP group Handle */
    AppTask                  profileTask;    /*!< App Handle */
    bool                     srcCollocated;  /*!<  TRUE/FALSE */
    uint16                   syncHandle;     /*!< SyncHandle of PA or advHandle of
                                                      collocated Broadcast src */
    uint8                    sourceAdvSid;   /*! Advertising SID */
    uint8                    paSyncState;    /*! PA Synchronization state */
    uint16                   paInterval;     /*!< Periodic Advertisement Interval */
    uint8                    infoCount;
    CapClientDelegatorInfo   *info;
    uint8                    numbSubGroups;  /*! Number of subgroups */
    CapClientSubgroupInfo    *subgroupInfo[BAP_MAX_SUPPORTED_NUM_SUBGROUPS];
} CapClientInternalBcastAsstModifySrcReq;

typedef struct
{
    CapClientPrim            type;           /*!< CAP_CLIENT_INTERNAL_BCAST_ASST_REMOVE_SRC_REQ */
    ServiceHandle            groupId;        /*!< CAP group Handle */
    AppTask                  profileTask;    /*!< App Handle */
    uint8                    infoCount;      
    CapClientDelegatorInfo   *info;
} CapClientInternalBcastAsstRemoveSrcReq;

typedef struct
{
    CapClientPrim            type;           /*!< CAP_CLIENT_INTERNAL_BCAST_ASST_REG_NOTIFICATIONS_REQ */
    ServiceHandle            groupId;        /*!< CAP group Handle */
    uint32                   cid;            /*!< BtConnID */
    AppTask                  profileTask;    /*!< App Handle */
    uint8                    sourceId;       /*!<  Broadcast Source ID, ignored if allSources set */
    bool                     allSources;     /*!<  subcribe  for notification on all Broadcast states */
    bool                     notificationEnable;
} CapClientInternalBcastAsstRegNotificationReq;

typedef struct
{
    CapClientPrim            type;           /*!< CAP_CLIENT_INTERNAL_BCAST_ASST_READ_RECEIVE_STATE_REQ */
    ServiceHandle            groupId;        /*!< CAP group Handle */
    uint32                   cid;            /*!< BtConnID */
    AppTask                  profileTask;    /*!< App Handle */
} CapClientInternalBcastAsstReadReceiveStateReq;


typedef struct
{
    CapClientPrim            type;          /*!< CAP_CLIENT_INTERNAL_UNLOCK_COORDINATED_SET_REQ */
    ServiceHandle            groupId;       /*!< CAP group Handle */
} CapClientInternalUnlockCoordinatedSetReq;

typedef struct
{
    CapClientPrim            type;          /*!< CAP_CLIENT_INTERNAL_BCAST_ASST_SET_CODE_RSP */
    ServiceHandle            groupId;       /*!< CAP group Handle */
    uint32                   cid;           /*!< BtConnID */
    uint8                    sourceId;      /*! Source_id of the Broadcast
                                                    Receive State characteristic */
    uint8                    *broadcastCode; /*! Value of Broadcast Code to set */
} CapClientInternalBcastAsstSetCodeRsp;

typedef struct
{
    CapClientPrim            type;          /*!< CAP_CLIENT_INTERNAL_PENDING_OP_REQ */
    CapClientPendingOp       pendingOp;
}CapClientInternalPendingOpReq;


typedef union
{
    CapClientUnicastConnectParam     unicastParam;
    CapClientUnicastConnectParamV1   unicastParamV1;
    CapClientBcastConfigParam        bcastParam;
} CapClientInternalSetParams;


typedef struct
{
    CapClientPrim            type;          /*!< CAP_CLIENT_INTERNAL_SET_PARAM_REQ */
    AppTask                  profileTask;
    uint32                   profileHandle;
    CapClientSreamCapability sinkConfig;
    CapClientSreamCapability srcConfig;
    CapClientParamType       paramType;
    uint8                    numOfParamElems;
    CapClientInternalSetParams  *paramElems;
}CapClientInternalSetParamReq;

#endif
