/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*******************************************************************************/

#ifndef GATT_MICS_CLIENT_PRIVATE_H_
#define GATT_MICS_CLIENT_PRIVATE_H_


#include "gatt_mics_client.h"
#include "csr_bt_gatt_client_util_lib.h"
/*
    @brief Client library private data.
*/
typedef struct __GMICSC
{
    AppTaskData lib_task;
    AppTask app_task;

    /* Any read/write command pending */
    uint16 pending_cmd;

    /* This is used to store the handle of the characteristic when a descriptor discovery is pending */
    uint16 pending_handle;

    GattMicsClientDeviceData handles;

    /* GattId, cid and service handle is a part of this structure */
    ServiceHandleListElm_t *srvcElem;
} GMICSC;

typedef struct gatt_mics_client
{
    CsrCmnList_t service_handle_list;
} gatt_mics_client;

/* Type LIB internal messages */
typedef int32 mics_client_internal_msg_t;

#define MICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ ((mics_client_internal_msg_t) 0x01)
#define MICS_CLIENT_INTERNAL_MSG_READ_CCC         ((mics_client_internal_msg_t) 0x02)
#define MICS_CLIENT_INTERNAL_MSG_READ             ((mics_client_internal_msg_t) 0x03)
#define MICS_CLIENT_INTERNAL_MSG_WRITE            ((mics_client_internal_msg_t) 0x04)
#define MICS_CLIENT_INTERNAL_MSG_TOP              ((mics_client_internal_msg_t) 0x05)

/* Internal Message Structure to read a client configuration characteristic */
typedef struct
{
    mics_client_internal_msg_t id;
    ServiceHandle srvc_hndl;
    uint16 handle;
} MICS_CLIENT_INTERNAL_MSG_READ_CCC_T;

/* Internal Message Structure to read a characteristic */
typedef MICS_CLIENT_INTERNAL_MSG_READ_CCC_T MICS_CLIENT_INTERNAL_MSG_READ_T;

/* Internal Message Structure to Initiate registering notification */
typedef struct
{
    mics_client_internal_msg_t id;
    ServiceHandle srvc_hndl;
    bool enable;   /* Enable/Disable notification */
    uint16 handle; /* Handle of the CCC to set */
}MICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T;

/* Internal Message Structure to write a characteristic */
typedef struct
{
    mics_client_internal_msg_t id;
    ServiceHandle srvc_hndl;
    uint16 handle;
    uint16 size_value;
    uint8*  value;
} MICS_CLIENT_INTERNAL_MSG_WRITE_T;

/* Enumerations for MICS client message which is pending and yet to process completely  */
typedef enum
{
    mics_client_pending_none = 0,
    mics_client_set_mute_value_pending,
    mics_client_write_notification_pending,
    mics_client_read_pending_ccc,
    mics_client_read_pending,
}mics_client_pending_op_type_t;

/* Opcodes of the set mute operation */
typedef enum _mics_client_control_point_opcodes
{
    mics_client_set_mute_value_op,
    mics_client_last_op
} mics_client_control_point_opcodes_t;

/* Macros for creating messages */
#define MAKE_MICS_CLIENT_MESSAGE(TYPE) TYPE *message = (TYPE *) CsrPmemZalloc(sizeof(TYPE));
#define MAKE_MICS_CLIENT_INTERNAL_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T *) CsrPmemZalloc(sizeof(TYPE##_T));
#define MAKE_MICS_CLIENT_MESSAGE_WITH_LEN(TYPE, LEN) \
                TYPE *message = (TYPE *) CsrPmemZalloc(sizeof(TYPE) + ((LEN) ? (LEN) - 1 : 0))

#define MicsMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, MICS_CLIENT_PRIM, MSG);\
    }

#define MicsMessageSendConditionally(TASK, ID, MSG, CMD) MicsMessageSend(TASK, ID, MSG)

/* Size of the MICS characteristics in number of octects */
#define MICS_CLIENT_MUTE_VALUE_SIZE         (1)

/* Size of the Client Characteristic Configuration in number of octects */
#define MICS_CLIENT_CHARACTERISTIC_CONFIG_SIZE          (2)

#endif
