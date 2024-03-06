/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_VCS_CLIENT_PRIVATE_H_
#define GATT_VCS_CLIENT_PRIVATE_H_


#include <stdlib.h>

#include "gatt_vcs_client.h"
#include "csr_bt_gatt_client_util_lib.h"
/*
    @brief Client library private data.
*/
typedef struct __GVCSC
{
    AppTaskData lib_task;
    AppTask app_task;

    /* Any read/write command pending */
    uint16 pending_cmd;

    /* This is used to store the handle of the characteristic when a descriptor discovery is pending */
    uint16 pending_handle;

    GattVcsClientDeviceData handles;

    /* GattId, cid and service handle is a part of this structure */
    ServiceHandleListElm_t *srvcElem;
} GVCSC;

typedef struct gatt_vcs_client
{
    CsrCmnList_t service_handle_list;
} gatt_vcs_client;

/* Type LIB internal messages */
typedef int32 vcs_client_internal_msg_t;

#define VCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ ((vcs_client_internal_msg_t) 0x01)
#define VCS_CLIENT_INTERNAL_MSG_READ_CCC         ((vcs_client_internal_msg_t) 0x02)
#define VCS_CLIENT_INTERNAL_MSG_READ             ((vcs_client_internal_msg_t) 0x03)
#define VCS_CLIENT_INTERNAL_MSG_WRITE            ((vcs_client_internal_msg_t) 0x04)
#define VCS_CLIENT_INTERNAL_MSG_TOP              ((vcs_client_internal_msg_t) 0x05)

/* Internal Message Structure to read a client configuration characteristic */
typedef struct
{
    vcs_client_internal_msg_t id;
    ServiceHandle srvc_hndl;
    uint16 handle;
} VCS_CLIENT_INTERNAL_MSG_READ_CCC_T;

/* Internal Message Structure to read a characteristic */
typedef VCS_CLIENT_INTERNAL_MSG_READ_CCC_T VCS_CLIENT_INTERNAL_MSG_READ_T;

/* Internal Message Structure to Initiate registering notification */
typedef struct
{
    vcs_client_internal_msg_t id;
    ServiceHandle srvc_hndl;
    bool enable;   /* Enable/Disable notification */
    uint16 handle; /* Handle of the CCC to set */
}VCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T;

/* Internal Message Structure to write a characteristic */
typedef struct
{
    vcs_client_internal_msg_t id;
    ServiceHandle srvc_hndl;
    uint16 handle;
    uint16 size_value;
    uint8*  value;
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
#define MAKE_VCS_CLIENT_MESSAGE(TYPE) TYPE *message = (TYPE *) CsrPmemZalloc(sizeof(TYPE));
#define MAKE_VCS_CLIENT_INTERNAL_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T *) CsrPmemZalloc(sizeof(TYPE##_T));
#define MAKE_VCS_CLIENT_MESSAGE_WITH_LEN(TYPE, LEN) MAKE_VCS_CLIENT_MESSAGE(TYPE)

#define VcsMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, VCS_CLIENT_PRIM, MSG);\
    }

#define VcsMessageSendConditionally(TASK, ID, MSG, CMD) VcsMessageSend(TASK, ID, MSG)

/* Size of the VCS characteristics in number of octects */
#define VCS_CLIENT_VOLUME_STATE_VALUE_SIZE    (3)
#define VCS_CLIENT_VOLUME_FLAG_VALUE_SIZE     (1)

/* Size of the Client Characteristic Configuration in number of octects */
#define VCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE          (2)

#endif
