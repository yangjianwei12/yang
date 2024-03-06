/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_VOCS_CLIENT_PRIVATE_H_
#define GATT_VOCS_CLIENT_PRIVATE_H_

#include <stdlib.h>

#include "gatt_vocs_client.h"
#include "csr_bt_gatt_client_util_lib.h"

/* Defines for the characteristic properties */
#define VOCS_CLIENT_WRITE_CMD_PROP    (0x0004)
#define VOCS_CLIENT_NOTIFY_PROP       (0x0010)

/*
    @brief Client library private data.
*/
typedef struct __GVOCSC
{
    AppTaskData lib_task;
    AppTask app_task;

    /* Any read/write command pending */
    uint16 pending_cmd;

    /* This is used to store the handle of the characteristic when a descriptor discovery is pending */
    uint16 pending_handle;

    uint16 start_handle;
    uint16 end_handle;

    GattVocsClientDeviceData handles;

    /* GattId, cid and service handle is a part of this structure */
    ServiceHandleListElm_t *srvcElem;
} GVOCS;

typedef struct gatt_vocs_client
{
    CsrCmnList_t service_handle_list;
} gatt_vocs_client;

/* Enum For LIB internal messages */
typedef enum
{
    VOCS_CLIENT_INTERNAL_MSG_BASE = 0,
    VOCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,          /* To enable/disable notification             */
    VOCS_CLIENT_INTERNAL_MSG_READ_CCC,                  /* To read the ccc of a characteristic        */
    VOCS_CLIENT_INTERNAL_MSG_READ,                      /* To read a characteristic                   */
    VOCS_CLIENT_INTERNAL_MSG_WRITE,                     /* To write a characteristic                  */
    VOCS_CLIENT_INTERNAL_MSG_AUDIO_LOC_WRITE,           /* To write the Audio Location characteristic */
    VOCS_CLIENT_INTERNAL_MSG_TOP                        /* Top of message                             */
}vocs_client_internal_msg_t;

/* Internal Message Structure to read the client configuration characteristic of a characteristic */
typedef struct
{
    vocs_client_internal_msg_t id;
    ServiceHandle srvc_hndl;
    uint16 handle;
} VOCS_CLIENT_INTERNAL_MSG_READ_CCC_T;

/* Internal Message Structure to read a characteristic */
typedef VOCS_CLIENT_INTERNAL_MSG_READ_CCC_T VOCS_CLIENT_INTERNAL_MSG_READ_T;

/* Internal Message Structure to write a characteristic */
typedef struct
{
    vocs_client_internal_msg_t id;
    ServiceHandle srvc_hndl;
    uint16 handle;
    uint16 size_value;
    uint8 *value;
} VOCS_CLIENT_INTERNAL_MSG_WRITE_T;

/* Internal Message Structure to Initiate registering notification */
typedef struct
{
    vocs_client_internal_msg_t id;
    ServiceHandle srvc_hndl;
    bool   enable;   /* Enable/Disable notification */
    uint16 handle;   /* Handle of the CCC to set */
}VOCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ_T;

/* Internal Message Structure to write the Audio Location characteristic */
typedef struct
{
    vocs_client_internal_msg_t id;
    ServiceHandle           srvc_hndl;
    uint16                     handle;
    GattVocsClientAudioLoc     value;
}VOCS_CLIENT_INTERNAL_MSG_AUDIO_LOC_WRITE_T;

/* Enumerations for AICS client message which is pending and yet to process completely  */
typedef enum
{
    vocs_client_pending_none = 0,
    vocs_client_write_set_vol_offset_pending,
    vocs_client_set_audio_desc_pending,
    vocs_client_write_notification_pending,
    vocs_client_read_pending_ccc,
    vocs_client_read_pending,
    vocs_client_set_audio_loc
}vocs_client_pending_operation_type_t;

/* Opcodes of the volume offset control point operation */
typedef enum _vocs_client_control_point_opcodes
{
    vocs_client_relative_set_vol_offset_op = 0x01,
    vocs_client_last_op
} vocs_client_control_point_opcodes_t;

/* Macros for creating messages */
#define MAKE_VOCS_CLIENT_MESSAGE(TYPE) TYPE *message = (TYPE *) CsrPmemZalloc(sizeof(TYPE));
#define MAKE_VOCS_CLIENT_INTERNAL_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T *) CsrPmemZalloc(sizeof(TYPE##_T));
#define MAKE_VOCS_CLIENT_MESSAGE_WITH_LEN(TYPE, LEN) MAKE_VOCS_CLIENT_MESSAGE(TYPE)
#define MAKE_VOCS_CLIENT_INTERNAL_MESSAGE_WITH_LEN(TYPE, LEN) MAKE_VOCS_CLIENT_INTERNAL_MESSAGE(TYPE)

#define VocsMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, VOCS_CLIENT_PRIM, MSG);\
    }

#define VocsMessageSendConditionally(TASK, ID, MSG, CMD) VocsMessageSend(TASK, ID, MSG)

/* Size of the VOCS characteristics in number of octects */
#define VOCS_CLIENT_OFFSET_STATE_VALUE_SIZE    (3)
#define VOCS_CLIENT_AUDIO_LOCATION_VALUE_SIZE  (4)

/* Size of the Client Characteristic Configuration in number of octects */
#define VOCS_CLIENT_CHARACTERISTIC_CONFIG_SIZE          (2)

#endif
