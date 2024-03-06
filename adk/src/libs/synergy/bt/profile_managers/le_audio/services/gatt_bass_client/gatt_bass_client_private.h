/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_BASS_CLIENT_PRIVATE_H_
#define GATT_BASS_CLIENT_PRIVATE_H_

#include <stdlib.h>

#include "csr_list.h"
#include "service_handle.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "gatt_bass_client.h"

/* Size of the Broadcast Code*/
#define BASS_CLIENT_BROADCAST_CODE_SIZE (16)

/* Minumum Size of the Broadcast Receive State characteristic */
#define BASS_CLIENT_BROADCAST_RECEIVE_STATE_MIN_SIZE (15)

/* Element of the list of handles */
struct bass_client_handle
{
    uint8 source_id;
    uint16 handle;
    uint16 handle_ccc;
    struct bass_client_handle *next;
};

typedef struct bass_client_handle bass_client_handle_t;

/* Handles of the BASS characteristics. */
typedef struct
{
    uint16 start_handle;
    uint16 end_handle;

    uint16 broadcast_source_num;

    uint16 broadcast_audio_scan_control_point_handle;

    bass_client_handle_t *broadcast_receive_state_handles_first;
    bass_client_handle_t *broadcast_receive_state_handles_last;
} gatt_bass_client_data_t;

/*
    @brief Client library private data.
*/
typedef struct __GBASSC
{
    AppTaskData lib_task;
    AppTask app_task;

    /* Any read/write command pending */
    uint16 pending_cmd;

    /* This is used to store the handle of the characteristic when a descriptor discovery is pending */
    uint16 pending_handle;

    /* This is used to count all the received reading confirmations during the initialization */
    uint8 counter;

    gatt_bass_client_data_t client_data;

    ServiceHandleListElm_t *srvcElem;
} GBASSC;

typedef struct gatt_bass_client
{
    CsrCmnList_t service_handle_list;
} gatt_bass_client;

typedef struct
{
    connection_id_t cid;
    uint16 start_handle;
    uint16 end_handle;
} gatt_bass_client_registration_params_t;

/* Type LIB internal messages */
typedef int32 bass_client_internal_msg_t;

#define BASS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ ((bass_client_internal_msg_t) 0x01)
#define BASS_CLIENT_INTERNAL_MSG_READ_CCC         ((bass_client_internal_msg_t) 0x02)
#define BASS_CLIENT_INTERNAL_MSG_READ             ((bass_client_internal_msg_t) 0x03)
#define BASS_CLIENT_INTERNAL_MSG_INIT_READ        ((bass_client_internal_msg_t) 0x04)
#define BASS_CLIENT_INTERNAL_MSG_WRITE            ((bass_client_internal_msg_t) 0x05)
#define BASS_CLIENT_INTERNAL_MSG_NOTIFY_READ      ((bass_client_internal_msg_t) 0x06)
#define BASS_CLIENT_INTERNAL_MSG_TOP              ((bass_client_internal_msg_t) 0x07)

/* Internal Message Structure to read the characteristics during the initialization */
typedef struct
{
    bass_client_internal_msg_t id; /*! service message id */
    ServiceHandle clnt_hndl;
    uint16 handle;
}BASS_CLIENT_INTERNAL_MSG_INIT_READ_T;

/* Internal Message Structure to read a client configuration characteristic */
typedef  BASS_CLIENT_INTERNAL_MSG_INIT_READ_T BASS_CLIENT_INTERNAL_MSG_READ_CCC_T;

/* Internal Message Structure to read a characteristic */
typedef BASS_CLIENT_INTERNAL_MSG_INIT_READ_T BASS_CLIENT_INTERNAL_MSG_READ_T;

/* Internal Message Structure to read the characteristics
 * after receiving a notifcation of a value greater than MTU-3 */
typedef  BASS_CLIENT_INTERNAL_MSG_INIT_READ_T BASS_CLIENT_INTERNAL_MSG_NOTIFY_READ_T;

/* Internal Message Structure to Initiate registering notification */
typedef struct
{
    bass_client_internal_msg_t id; /*! service message id */
    ServiceHandle clnt_hndl;
    bool enable;   /* Enable/Disable notification */
    uint16 handle; /* Handle of the CCC to set */
}BASS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T;

/* Internal Message Structure to write a characteristic */
typedef struct
{
    bass_client_internal_msg_t id; /*! service message id */
    ServiceHandle clnt_hndl;
    uint16 handle;
    bool no_response;
    bool longWrite;
    uint16 size_value;
    uint8  value[1];
} BASS_CLIENT_INTERNAL_MSG_WRITE_T;

/* Enumerations for BASS client message which is pending and yet to process completely  */
typedef enum
{
    bass_client_pending_none = 0,
    bass_client_write_remote_scan_stop_pending,
    bass_client_write_remote_scan_start_pending,
    bass_client_write_add_source_pending,
    bass_client_write_modify_source_pending,
    bass_client_write_set_broadcast_code_pending,
    bass_client_write_remove_source_pending,
    bass_client_write_notification_pending,
    bass_client_init_read_pending,
    bass_client_read_pending_ccc,
    bass_client_read_pending,
    bass_client_notify_read_pending
}bass_client_pending_op_type_t;

/* Opcodes of the Broadcast Audio Scan Control Point operations */
typedef enum _bass_client_control_point_opcodes
{
    bass_client_remote_scan_stop_op,
    bass_client_remote_scan_start_op,
    bass_client_add_source_op,
    bass_client_modify_source_op,
    bass_client_set_broadcast_code_op,
    bass_client_remove_source_op,
    bass_client_last_op
} bass_client_control_point_opcodes_t;

/* Macros for creating messages */
#define MAKE_BASS_CLIENT_MESSAGE(TYPE) TYPE *message = (TYPE *) CsrPmemZalloc(sizeof(TYPE));
#define MAKE_BASS_CLIENT_INTERNAL_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T *) CsrPmemZalloc(sizeof(TYPE##_T));
#define MAKE_BASS_CLIENT_MESSAGE_WITH_LEN(TYPE, LEN) TYPE *message = (TYPE *) calloc(1,sizeof(TYPE) + LEN)
#define MAKE_BASS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *) calloc(1,sizeof(TYPE##_T) + LEN)

/* Assumes message struct with
 *    uint16 size_value;
 *    uint16 value[1];
 */
#define MAKE_BASS_CLIENT_MESSAGE_WITH_LEN_U16(TYPE, LEN)  \
        TYPE *message = (TYPE*)calloc(1,  \
                        sizeof(TYPE) + \
                        sizeof(uint16) * ((LEN) ? (LEN - 1) : 0))

#define BassMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, BASS_CLIENT_PRIM, MSG);\
    }

#define BassMessageSendConditionally(TASK, ID, MSG, CMD) BassMessageSend(TASK, ID, MSG)

/* Size of the Broadcast Receive State characteristic in number of octects */
#define BASS_CLIENT_BROADCAST_RECEIVE_STATE_VALUE_SIZE_MIN    (16)

/* Size of the Client Characteristic Configuration in number of octects */
#define BASS_CLIENT_CHARACTERISTIC_CONFIG_SIZE          (2)

void gattBassClientPanic(void);

#endif
