/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_AICS_CLIENT_PRIVATE_H_
#define GATT_AICS_CLIENT_PRIVATE_H_

#include <message.h>
#include <panic.h>
#include <stdlib.h>

#include "gatt_aics_client.h"

/* Defines for the characteristic properties */
#define AICS_CLIENT_WRITE_CMD_PROP    (0x0004)
#define AICS_CLIENT_NOTIFY_PROP       (0x0010)

/* Enumerations for AICS client message which is pending and yet to process completely  */
typedef enum
{
    aics_client_pending_none = 0,
    aics_client_write_set_gain_setting_pending,
    aics_client_write_unmute_pending,
    aics_client_write_mute_pending,
    aics_client_write_set_manual_gain_mode_pending,
    aics_client_write_set_automatic_gain_mode_pending,
    aics_client_write_notification_pending,
    aics_client_read_pending_ccc,
    aics_client_read_pending
}aics_client_pending_operation_type_t;

/*
    @brief Client library private data.
*/
typedef struct __GAICS
{
    TaskData lib_task;
    Task app_task;

    /* Any read/write command pending */
    uint16 pending_cmd;

    /* This is used to store the handle of the characteristic when a descriptor discovery is pending */
    uint16 pending_handle;

    uint16 start_handle;
    uint16 end_handle;
    GattAicsClientDeviceData handles;

    /* Service handle of the instance */
    ServiceHandle srvc_hndl;
} GAICS;

/* Enum For LIB internal messages */
typedef enum
{
    AICS_CLIENT_INTERNAL_MSG_BASE = 0,
    AICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,          /* To enable/disable notification             */
    AICS_CLIENT_INTERNAL_MSG_READ_CCC,                  /* To read the ccc of a characteristic        */
    AICS_CLIENT_INTERNAL_MSG_READ,                      /* To read a characteristic                   */
    AICS_CLIENT_INTERNAL_MSG_WRITE,                     /* To write a characteristic                  */
    AICS_CLIENT_INTERNAL_MSG_TOP                        /* Top of message                             */
}aics_client_internal_msg_t;

/* Internal Message Structure to read the client configuration characteristic of a characteristic */
typedef struct
{
    uint16 handle;
} AICS_CLIENT_INTERNAL_MSG_READ_CCC_T;

/* Internal Message Structure to read a characteristic */
typedef AICS_CLIENT_INTERNAL_MSG_READ_CCC_T AICS_CLIENT_INTERNAL_MSG_READ_T;

/* Internal Message Structure to Initiate registering notification */
typedef struct
{
    bool enable;   /* Enable/Disable notification */
    uint16 handle; /* Handle of the CCC to set */
}AICS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T;

/* Internal Message Structure to write a characteristic */
typedef struct
{
    uint16 handle;
    uint16 size_value;
    uint8 value[1];
} AICS_CLIENT_INTERNAL_MSG_WRITE_T;

/* Opcodes of the audio input control point operation */
typedef enum _aics_client_control_point_opcodes
{
    aics_client_set_gain_setting_op = 0x01,
    aics_client_unmute_op,
    aics_client_mute_op,
    aics_client_set_manual_gain_mode_op,
    aics_client_set_automatic_gain_mode_op,
    aics_client_last_op
} aics_client_control_point_opcodes_t;

/* Macros for creating messages */
#define MAKE_AICS_CLIENT_MESSAGE(TYPE) MESSAGE_MAKE(message,TYPE);
#define MAKE_AICS_CLIENT_INTERNAL_MESSAGE(TYPE) MESSAGE_MAKE(message,TYPE##_T);
#define MAKE_AICS_CLIENT_MESSAGE_WITH_LEN(TYPE, LEN) TYPE *message = (TYPE *) PanicNull(calloc(1,sizeof(TYPE) + ((LEN) - 1) * sizeof(uint8)))
#define MAKE_AICS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *) PanicNull(calloc(1,sizeof(TYPE##_T) + ((LEN) - 1) * sizeof(uint8)))

/* Size of the Client Characteristic Configuration in number of octects */
#define AICS_CLIENT_CHARACTERISTIC_CONFIG_SIZE          (2)

#endif