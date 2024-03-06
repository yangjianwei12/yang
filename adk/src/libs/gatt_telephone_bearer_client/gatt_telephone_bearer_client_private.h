/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_TBS_CLIENT_PRIVATE_H_
#define GATT_TBS_CLIENT_PRIVATE_H_

#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>

#include <gatt_manager.h>
#include "gatt_telephone_bearer_client.h"
#include "gatt_telephone_bearer_client_debug.h"


/* Macros for creating messages */
#define MAKE_TBSC_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T*)PanicNull(calloc(1,sizeof(TYPE##_T)))
#define MAKE_TBSC_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *) PanicNull(calloc(1,sizeof(TYPE##_T) + ((LEN) - 1) * sizeof(uint8)))

/* Characteristic sizes */
#define TBS_CLIENT_SIGNAL_STRENGTH_INTERVAL_SIZE (1)
/* Size of the Client Characteristic Configuration in number of octects */
#define TBS_CLIENT_CHARACTERISTIC_CONFIG_SIZE    (2)

typedef struct
{
    bool notificationsEnable;   /* Enable/Disable notification */
    uint16 handle;               /* Handle of the CCC to set */
} TELEPHONE_BEARER_INTERNAL_MSG_SET_NOTIFICATION_T;

typedef struct
{
    uint8   interval; /* Signal Strength Reporting Interval*/
    bool    writeWithoutResponse; /* true if to use write without response */
} TELEPHONE_BEARER_INTERNAL_MSG_WRITE_SIGNAL_STRENGTH_INTERVAL_T;

typedef struct
{
    uint8  opcode;   /* Signal Strength Reporting Interval*/
    uint8  size;     /* Size of param */
    uint8  param[1]; /* Control point parameter */
} TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT_T;

typedef struct
{
    uint16 descriptor_uuid;
} TELEPHONE_BEARER_INTERNAL_MSG_READ_DESCRIPTOR_T;

/* Enum for TBS Client library internal message. */
typedef enum __tbsc_internal_msg_t
{
    TELEPHONE_BEARER_INTERNAL_MSG_READ_PROVIDER_NAME,
    TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_UCI,
    TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_TECHNOLOGY,
    TELEPHONE_BEARER_INTERNAL_MSG_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST,
    TELEPHONE_BEARER_INTERNAL_MSG_READ_SIGNAL_STRENGTH,
    TELEPHONE_BEARER_INTERNAL_MSG_READ_SIGNAL_STRENGTH_INTERVAL,
    TELEPHONE_BEARER_INTERNAL_MSG_READ_CURRENT_CALLS_LIST,
    TELEPHONE_BEARER_INTERNAL_MSG_READ_CONTENT_CONTROL_ID,
    TELEPHONE_BEARER_INTERNAL_MSG_READ_FEATURE_AND_STATUS_FLAGS,
    TELEPHONE_BEARER_INTERNAL_MSG_READ_INCOMING_CALL_TARGET_BEARER_URI,
    TELEPHONE_BEARER_INTERNAL_MSG_READ_CALL_STATE,
    TELEPHONE_BEARER_INTERNAL_MSG_READ_INCOMING_CALL,
    TELEPHONE_BEARER_INTERNAL_MSG_READ_CALL_FRIENDLY_NAME,
    TELEPHONE_BEARER_INTERNAL_MSG_READ_CCP_OPTIONAL_OPCODES,

    TELEPHONE_BEARER_INTERNAL_MSG_SET_NOTIFICATION,

    TELEPHONE_BEARER_INTERNAL_MSG_WRITE_SIGNAL_STRENGTH_INTERVAL,
    TELEPHONE_BEARER_INTERNAL_MSG_WRITE_CALL_CONTROL_POINT,
} tbsc_internal_msg_t;


/*! @brief The TBS internal structure for the client role.

    This structure is not visible to the application as it is responsible for managing resource to pass to the TBSC library.
    The elements of this structure are only modified by the TBSC library.

 */
typedef struct __GTBSC
{
    TaskData lib_task;
    Task appTask;

    /* Service handle of the instance */
    ServiceHandle srvcHandle;

    gatt_uuid_t nextDescriptorHandle;

    /* TBSC handles */
    uint16  bearer_name_handle;           /* Handle of the Bearer Provider Name characteristic value */
    uint16  bearer_name_ccc_handle;       /* Handle of the Bearer Provider Name Client characteristic config value */
    uint16  bearer_uci_handle;            /* Handle of the Bearer UCI Characteristic value */
    uint16  bearer_tech_handle;           /* Handle of the Bearer technology characteristic value */
    uint16  bearer_tech_ccc_handle;       /* Handle of the Bearer technology client characteristic config value */
    uint16  bearer_uri_prefix_list_handle;/* Handle of the Bearer URI Supported List Characteristic value */
    uint16  signal_strength_handle;       /* Handle of the Signal Strength characteristic */
    uint16  signal_strength_ccc_handle;   /* Handle of the Signal Strength Client characteristic config */
    uint16  signal_strength_interval_handle;  /* Handle of the Signal Strength Interval characteristic */
    uint8   signal_strength_interval_props;
    uint16  list_current_calls_handle;    /* Handle of the List Current Calls characteristic */
    uint16  list_current_calls_ccc_handle;/* Handle of the List Current Calls Client characteristic config */
    uint16  content_control_id_handle;    /* Handle of the Content Control ID Characteristic */
    uint16  status_flags_handle;          /* Handle of the Status Flags characteristic */
    uint16  status_flags_ccc_handle;      /* Handle of the Status Flags Client characteristic config */
    uint16  incoming_target_bearer_uri_handle; /* Handle of the Target Bearer URI Characteristic */
    uint16  incoming_target_bearer_uri_ccc_handle; /* Handle of the Target Bearer URI Client Characteristic Configuration */
    uint16  call_state_handle;            /* Handle of the Call State characteristic */
    uint16  call_state_ccc_handle;        /* Handle of the Call State Client characteristic config */
    uint16  call_control_point_handle;    /* Handle of the Call Control Point Characteristic */
    uint16  call_control_point_ccc_handle;/* Handle of the Call Control Point Client Characteristic config*/
    uint16  call_control_point_optional_opcodes_handle;/* Handle of the CCP optional opcodes */
    uint16  termination_reason_handle;    /* Handle of the Termination Reason Characteristic */
    uint16  termination_reason_ccc_handle;/* Handle of the Termination Reason client Characteristic config */
    uint16  incoming_call_handle;         /* Handle of the Incoming call characteristic */
    uint16  incoming_call_ccc_handle;     /* Handle of the Incoming call client characteristic config */
    uint16  remote_friendly_name_handle;/* Handle of the call remote friendly name characteristic */
    uint16  remote_friendly_name_ccc_handle;/* Handle of the call remote friendly name client characteristic config */

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

} GTBSC;


#endif
