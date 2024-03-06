/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_VCS_CLIENT_PRIVATE_H_
#define GATT_VCS_CLIENT_PRIVATE_H_

#include <message.h>
#include <panic.h>
#include <stdlib.h>

#include "gatt_vcs_client.h"

/*
    @brief Client library private data.
*/
typedef struct __GVCSC
{
    TaskData lib_task;
    Task app_task;

    /* Any read/write command pending */
    uint16 pending_cmd;

    /* This is used to store the handle of the characteristic when a descriptor discovery is pending */
    uint16 pending_handle;

    GattVcsClientDeviceData handles;

    /* Service handle of the instance */
    ServiceHandle srvc_hndl;
} GVCSC;

/* Enum For LIB internal messages */
typedef enum
{
    VCS_CLIENT_INTERNAL_MSG_BASE = 0,
    VCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,          /* To enable/disable notification             */
    VCS_CLIENT_INTERNAL_MSG_READ_CCC,                  /* To read the ccc of a characteristic        */
    VCS_CLIENT_INTERNAL_MSG_READ,                      /* To read a characteristic                   */
    VCS_CLIENT_INTERNAL_MSG_WRITE,                     /* To write a characteristic                  */
    VCS_CLIENT_INTERNAL_MSG_TOP                        /* Top of message                             */
}vcs_client_internal_msg_t;

/* Internal Message Structure to read a client configuration characteristic */
typedef struct
{
    uint16 handle;
} VCS_CLIENT_INTERNAL_MSG_READ_CCC_T;

/* Internal Message Structure to read a characteristic */
typedef VCS_CLIENT_INTERNAL_MSG_READ_CCC_T VCS_CLIENT_INTERNAL_MSG_READ_T;

/* Internal Message Structure to Initiate registering notification */
typedef struct
{
    bool enable;   /* Enable/Disable notification */
    uint16 handle; /* Handle of the CCC to set */
}VCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T;

/* Internal Message Structure to write a characteristic */
typedef struct
{
    uint16 handle;
    uint16 size_value;
    uint8  value[1];
} VCS_CLIENT_INTERNAL_MSG_WRITE_T;

/* Enumerations for VCS client message which is pending and yet to process completely  */
typedef enum
{
    vcs_client_pending_none = 0,
    vcs_client_write_rel_vol_down_pending,
    vcs_client_write_rel_vol_up_pending,
    vcs_client_write_unmute_rel_vol_down_pending,
    vcs_client_write_unmute_rel_vol_up_pending,
    vcs_client_write_absolute_vol_pending,
    vcs_client_write_unmute_pending,
    vcs_client_write_mute_pending,
    vcs_client_write_notification_pending,
    vcs_client_read_pending_ccc,
    vcs_client_read_pending,
}vcs_client_pending_op_type_t;

/* Opcodes of the volume control point operation */
typedef enum _vcs_client_control_point_opcodes
{
    vcs_client_relative_volume_down_op,
    vcs_client_relative_volume_up_op,
    vcs_client_unmute_relative_volume_down_op,
    vcs_client_unmute_relative_volume_up_op,
    vcs_client_set_absolute_volume_op,
    vcs_client_unmute_op,
    vcs_client_mute_op,
    vcs_client_last_op
} vcs_client_control_point_opcodes_t;

/* Macros for creating messages */
#define MAKE_VCS_CLIENT_MESSAGE(TYPE) MESSAGE_MAKE(message,TYPE);
#define MAKE_VCS_CLIENT_INTERNAL_MESSAGE(TYPE) MESSAGE_MAKE(message,TYPE##_T);
#define MAKE_VCS_CLIENT_MESSAGE_WITH_LEN(TYPE, LEN) TYPE *message = (TYPE *) PanicNull(calloc(1,sizeof(TYPE) + ((LEN) - 1) * sizeof(uint8)))
#define MAKE_VCS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *) PanicNull(calloc(1,sizeof(TYPE##_T) + ((LEN) - 1) * sizeof(uint8)))


/* Size of the VCS characteristics in number of octects */
#define VCS_CLIENT_VOLUME_STATE_VALUE_SIZE    (3)
#define VCS_CLIENT_VOLUME_FLAG_VALUE_SIZE     (1)

/* Size of the Client Characteristic Configuration in number of octects */
#define VCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE          (2)

#endif
