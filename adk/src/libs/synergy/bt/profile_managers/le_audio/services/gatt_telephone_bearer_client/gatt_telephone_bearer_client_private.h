/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef GATT_TBS_CLIENT_PRIVATE_H_
#define GATT_TBS_CLIENT_PRIVATE_H_

#include <stdlib.h>

#include "gatt_telephone_bearer_client.h"
#include "csr_list.h"
#include "att_prim.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_gatt_client_util_prim.h"
#include "csr_pmem.h"
#include "gatt_telephone_bearer_client_debug.h"


/* Macros for creating messages */
#define MAKE_TBSC_MESSAGE(TYPE) TYPE *message = (TYPE*)CsrPmemZalloc(sizeof(TYPE))
#define MAKE_TBSC_MESSAGE_WITH_LEN(TYPE, LEN) TYPE *message = (TYPE *) CsrPmemAlloc(sizeof(TYPE) + ((LEN) ? (LEN) - 1 : 0) * sizeof(uint8))
#define MAKE_TBSC_MESSAGE_WITH_LEN_U8(TYPE, LEN)  \
    TYPE *message = (TYPE*)CsrPmemAlloc(sizeof(TYPE) + \
                                       ((LEN) ? (LEN) - 1 : 0))


/* Characteristic sizes */
#define TBS_CLIENT_SIGNAL_STRENGTH_INTERVAL_SIZE (1)
/* Size of the Client Characteristic Configuration in number of octects */
#define TBS_CLIENT_CHARACTERISTIC_CONFIG_SIZE    (2)

typedef uint16 TelephoneBearerClientInternalMessageId;

/* Messages for TBS Client library internal message. */

#define TELEPHONE_BEARER_INTERNAL_MSG_READ_PROVIDER_NAME                        (0x0000u)
#define TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_UCI                           (0x0001u)
#define TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_TECHNOLOGY                    (0x0002u)
#define TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST    (0x0003u)
#define TELEPHONE_BEARER_INTERNAL_MSG_READ_SIGNAL_STRENGTH                      (0x0004u)
#define TELEPHONE_BEARER_INTERNAL_MSG_READ_SIGNAL_STRENGTH_INTERVAL             (0x0005u)
#define TELEPHONE_BEARER_INTERNAL_MSG_READ_CURRENT_CALLS_LIST                   (0x0006u)
#define TELEPHONE_BEARER_INTERNAL_MSG_READ_CONTENT_CONTROL_ID                   (0x0007u)
#define TELEPHONE_BEARER_INTERNAL_MSG_READ_FEATURE_AND_STATUS_FLAGS             (0x0008u)
#define TELEPHONE_BEARER_INTERNAL_MSG_READ_INCOMING_CALL_TARGET_BEARER_URI      (0x0009u)
#define TELEPHONE_BEARER_INTERNAL_MSG_READ_CALL_STATE                           (0x000au)
#define TELEPHONE_BEARER_INTERNAL_MSG_READ_INCOMING_CALL                        (0x000bu)
#define TELEPHONE_BEARER_INTERNAL_MSG_READ_CALL_FRIENDLY_NAME                   (0x000cu)
#define TELEPHONE_BEARER_INTERNAL_MSG_READ_CCP_OPTIONAL_OPCODES                 (0x000du)

#define TELEPHONE_BEARER_INTERNAL_MSG_SET_NOTIFICATION                          (0x000eu)

#define TELEPHONE_BEARER_INTERNAL_MSG_WRITE_SIGNAL_STRENGTH_INTERVAL            (0x000fu)
#define TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT                  (0x0010u)


typedef struct __TelephoneBearerInternalGenericRead
{
    TelephoneBearerClientInternalMessageId id;
    ServiceHandle srvcHndl;
} TelephoneBearerInternalGenericRead;


typedef struct
{
    TelephoneBearerClientInternalMessageId id;
    ServiceHandle srvcHndl;
    uint16 handle;               /* Handle of the CCC to set */
    bool notificationsEnable;    /* Enable/Disable notification */
} TelephoneBearerInternalMsgSetNotification;

typedef struct
{
    TelephoneBearerClientInternalMessageId id;
    ServiceHandle srvcHndl;
    uint8  interval; /* Signal Strength Reporting Interval*/
    bool   writeWithoutResponse; /* true if to use write without response */
} TelephoneBearerInternalMsgWriteSignalStrengthInterval;

typedef struct
{
    TelephoneBearerClientInternalMessageId id;
    ServiceHandle srvcHndl;
    uint8  opcode;   /* Signal Strength Reporting Interval*/
    uint8  size;     /* Size of param */
    uint8  *param; /* Control point parameter */
} TelephoneBearerInternalMsgWriteCallControlPoint;

typedef struct
{
    TelephoneBearerClientInternalMessageId id;
    ServiceHandle srvcHndl;
    uint16 descriptor_uuid;
} TelephoneBearerInternalMsgReadDescriptor;


/*! @brief The TBS internal structure for the client role.

    This structure is not visible to the application as it is responsible for managing resource to pass to the TBSC library.
    The elements of this structure are only modified by the TBSC library.

 */
typedef struct __GTBSC
{
    AppTaskData lib_task;
    AppTask appTask;

    /* Service handle of the instance */
    ServiceHandle srvcHandle;
    uint32 nextDescriptorHandle;

    /* TBSC handles */
    GattTelephoneBearerClientDeviceData handles;

    /* Client Configs */
    uint16 bearer_name_client_cfg;
    uint16 bearer_tech_client_cfg;
    uint16 signal_strength_client_cfg;
    uint16 list_current_calls_ccc_client_cfg;
    uint16 supported_features_ccc_client_cfg;
    uint16 incoming_target_bearer_uri_client_cfg;
    uint16 call_state_client_cfg;
    uint16 call_control_point_client_cfg;
    uint16 termination_reason_client_cfg;
    uint16 incoming_remote_friendly_name_client_cfg;
    uint16 incoming_call_client_cfg;
    uint16 startHandle;
    uint16 endHandle;

    /* GattId ,btConnId, service_handle of the Telephone Bearer Client Instance*/
    ServiceHandleListElm_t *srvcElem;

} GTBSC;

typedef struct gatt_tbs_client
{
    CsrCmnList_t service_handle_list;
} gatt_tbs_client;

#define TbsClientMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, TBS_CLIENT_PRIM, MSG);\
    }

/*TBS service uuids*/
#if 0

/* Keep it in case if want to use old values */
#define UUID_TELEPHONE_BEARER_SERVICE                        0x8FD5
#define GATT_GTBS_UUID_TELEPHONE_BEARER_SERVICE              0x8FDF


#define GATT_TBS_UUID_BEARER_PROVIDER_NAME                   0x8FEA
#define GATT_TBS_UUID_BEARER_UCI                             0x8FEB
#define GATT_TBS_UUID_BEARER_TECHNOLOGY                      0x8FEC
#define GATT_TBS_UUID_BEARER_URI_PREFIX_LIST                 0x8FED
#define GATT_TBS_UUID_SIGNAL_STRENGTH                        0x8FEF
#define GATT_TBS_UUID_SIGNAL_STRENGTH_REPORTING_INTERVAL     0x8FF0
#define GATT_TBS_UUID_LIST_CURRENT_CALLS                     0x8FF1
#define GATT_TBS_UUID_CONTENT_CONTROL_ID                     0x8FB5 /*0x8FF2  or 0x8FB5 ???*/
#define GATT_TBS_UUID_STATUS_FLAGS                           0x8FF3
#define GATT_TBS_UUID_INCOMING_CALL_TARGET_BEARER_URI        0x8FF4
#define GATT_TBS_UUID_CALL_STATE                             0x8FF5
#define GATT_TBS_UUID_CALL_CONTROL_POINT                     0x8FF6
#define GATT_TBS_UUID_CALL_CONTROL_POINT_OPCODES             0x8FF7
#define GATT_TBS_UUID_TERMINATION_REASON                     0x8FF8
#define GATT_TBS_UUID_INCOMING_CALL                          0x8FF9
#define GATT_TBS_UUID_REMOTE_FRIENDLY_NAME                   0x8FFA

#else /* if 0 */

/* UUID Values as per latest spec */
#define UUID_TELEPHONE_BEARER_SERVICE                        0x184B
#define GATT_GTBS_UUID_TELEPHONE_BEARER_SERVICE              0x184C


#define GATT_TBS_UUID_BEARER_PROVIDER_NAME                   0x2BB3
#define GATT_TBS_UUID_BEARER_UCI                             0x2BB4
#define GATT_TBS_UUID_BEARER_TECHNOLOGY                      0x2BB5
#define GATT_TBS_UUID_BEARER_URI_PREFIX_LIST                 0x2BB6
#define GATT_TBS_UUID_SIGNAL_STRENGTH                        0x2BB7
#define GATT_TBS_UUID_SIGNAL_STRENGTH_REPORTING_INTERVAL     0x2BB8
#define GATT_TBS_UUID_LIST_CURRENT_CALLS                     0x2BB9
#define GATT_TBS_UUID_CONTENT_CONTROL_ID                     0x2BBA
#define GATT_TBS_UUID_STATUS_FLAGS                           0x2BBB
#define GATT_TBS_UUID_INCOMING_CALL_TARGET_BEARER_URI        0x2BBC
#define GATT_TBS_UUID_CALL_STATE                             0x2BBD
#define GATT_TBS_UUID_CALL_CONTROL_POINT                     0x2BBE
#define GATT_TBS_UUID_CALL_CONTROL_POINT_OPCODES             0x2BBF
#define GATT_TBS_UUID_TERMINATION_REASON                     0x2BC0
#define GATT_TBS_UUID_INCOMING_CALL                          0x2BC1
#define GATT_TBS_UUID_REMOTE_FRIENDLY_NAME                   0x2BC2

#endif /* if 0 */

#define GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID       (0x2902)

#endif
