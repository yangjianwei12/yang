/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef VCP_PRIVATE_H
#define VCP_PRIVATE_H

#include <stdlib.h>

#include "csr_bt_gatt_lib.h"

#include "gatt_vocs_client.h"
#include "gatt_aics_client.h"

#include "vcp.h"
#include "service_handle.h"

#define VCP_VCS_UUID   (0x1844)
#define VCP_VOCS_UUID  (0x1845)
#define VCP_AICS_UUID  (0x1843)


#define VCP_VCS_INVALID_CHANGE_COUNTER_ERR (0x80)
#define VCP_VOCS_INVALID_CHANGE_COUNTER_ERR (VCP_VCS_INVALID_CHANGE_COUNTER_ERR)
#define VCP_AICS_INVALID_CHANGE_COUNTER_ERR (VCP_VCS_INVALID_CHANGE_COUNTER_ERR)

#define VCP_VCS_SET_ABS_VOL_MAX_RETRY_NUM  (0x03)

#define VcpMessageSend(TASK, MSG) CsrSchedMessagePut(TASK, VCP_PRIM, MSG);
#define VcpMessageSendConditionally(TASK, ID, MSG, CMD) VcpMessageSend(TASK, MSG)

typedef ServiceHandle vcp_profile_t;

/* Element of the list of VOCS instances */
struct vocs_inst
{
    connection_id_t cid;
    uint16 start_handle;
    uint16 end_handle;
    GattVocsClientDeviceData device_data;
    struct vocs_inst *next;
};

typedef struct vocs_inst vcp_vocs_inst_t;

/* Element of the list of AICS instances */
struct aics_inst
{
    connection_id_t cid;
    uint16 start_handle;
    uint16 end_handle;
    GattAicsClientDeviceData device_data;
    struct aics_inst *next;
};

typedef struct aics_inst vcp_aics_inst_t;

/* Element of the list of VOCS service handles */
struct vocs_srvc_hndl
{
    ServiceHandle srvc_hdnl;
    struct vocs_srvc_hndl *next;
};

typedef struct vocs_srvc_hndl vcp_vocs_srvc_hndl_t;

/* Element of the list of AICS service handles */
struct aics_srvc_hndl
{
    ServiceHandle srvc_hdnl;
    struct aics_srvc_hndl *next;
};

typedef struct aics_srvc_hndl vcp_aics_srvc_hndl_t;

/* Opcodes of the volume control point operation */
typedef enum __vcp_pending_op
{
    vcp_pending_op_none,
    vcp_pending_relative_volume_down_op,
    vcp_pending_relative_volume_up_op,
    vcp_pending_unmute_relative_volume_down_op,
    vcp_pending_unmute_relative_volume_up_op,
    vcp_pending_set_absolute_volume_op,
    vcp_pending_unmute_op,
    vcp_pending_mute_op,
    vcp_pending_set_volume_offset_op,
    vcp_pending_set_gain_setting_op,
    vcp_pending_aics_unmute_op,
    vcp_pending_aics_mute_op,
    vcp_pending_set_mnl_gain_mode_op,
    vcp_pending_set_atmtc_gain_mode_op,
    vcp_pending_set_initial_vol_op
} vcp_pending_op_t;

/* Opcodes of the volume control point operation */
typedef enum __vcp_vcs_control_point_opcodes
{
    vcp_relative_volume_down_op,
    vcp_relative_volume_up_op,
    vcp_unmute_relative_volume_down_op,
    vcp_unmute_relative_volume_up_op,
    vcp_set_absolute_volume_op,
    vcp_unmute_op,
    vcp_mute_op,
    vcp_vcs_last_op
} vcp_vcs_control_point_opcodes_t;

/* Opcodes of the volume offset control point operation */
typedef enum __vcp_vocs_control_point_opcodes
{
    vcp_vocs_set_volume_offset_op = 0x01,
    vcp_vocs_last_op
} vcp_vocs_control_point_opcodes_t;

/* Opcodes of the audio input control point operation */
typedef enum __vcp_aics_control_point_opcodes
{
    vcp_aics_set_gain_setting_op = 0x01,
    vcp_aics_unmute_op,
    vcp_aics_mute_op,
    vcp_aics_set_mnl_gain_mode_op,
    vcp_aics_set_atmtc_gain_mode_op,
    vcp_aics_last_op
} vcp_aics_control_point_opcodes_t;

/* Enum For LIB internal messages */
typedef enum
{
    VCP_INTERNAL_MSG_BASE = 0,
    VCP_INTERNAL_REL_VOL_DOWN,
    VCP_INTERNAL_REL_VOL_UP,
    VCP_INTERNAL_UNMUTE_REL_VOL_DOWN,
    VCP_INTERNAL_UNMUTE_REL_VOL_UP,
    VCP_INTERNAL_ABS_VOL,
    VCP_INTERNAL_MUTE,
    VCP_INTERNAL_UNMUTE,
    VCP_INTERNAL_SET_VOL_OFFSET,
    VCP_INTERNAL_SET_GAIN_SETTING,
    VCP_INTERNAL_AICS_UNMUTE,
    VCP_INTERNAL_AICS_MUTE,
    VCP_INTERNAL_AICS_SET_MANUAL_GAIN_MODE,
    VCP_INTERNAL_AICS_SET_AUTOMATIC_GAIN_MODE,
    VCP_INTERNAL_SET_INITIAL_VOL_OP,
    VCP_INTERNAL_MSG_TOP
}vcp_internal_msg_t;

/* Internal Message Structure to perform the Relative Volume Down operation */
typedef struct
{
    vcp_internal_msg_t   id;
    VcpProfileHandle     prfl_hndl;
    uint8                volume_setting;
} VCP_INTERNAL_REL_VOL_DOWN_T;

/* Internal Message Structure to perform the Relative Volume Up operation */
typedef VCP_INTERNAL_REL_VOL_DOWN_T VCP_INTERNAL_REL_VOL_UP_T;

/* Internal Message Structure to perform the Unmute Relative Volume Downn operation */
typedef VCP_INTERNAL_REL_VOL_DOWN_T VCP_INTERNAL_UNMUTE_REL_VOL_DOWN_T;

/* Internal Message Structure to perform the Unmute Relative Volume Up operation */
typedef VCP_INTERNAL_REL_VOL_DOWN_T VCP_INTERNAL_UNMUTE_REL_VOL_UP_T;

/* Internal Message Structure to perform the Absolte Volume operation */
typedef VCP_INTERNAL_REL_VOL_DOWN_T VCP_INTERNAL_ABS_VOL_T;

/* Internal Message Structure to perform the Unmute operation */
typedef VCP_INTERNAL_REL_VOL_DOWN_T VCP_INTERNAL_UNMUTE_T;

/* Internal Message Structure to perform the Mute operation */
typedef VCP_INTERNAL_REL_VOL_DOWN_T VCP_INTERNAL_MUTE_T;

/* Internal Message Structure to perform the Set Volume Offset operation */
typedef struct
{
    vcp_internal_msg_t                id;
    VcpProfileHandle                  prfl_hndl;
    ServiceHandle                  vocs_srvc_hndl;
    vcp_vocs_control_point_opcodes_t  opcode;
    int16                             volume_offset;
}VCP_INTERNAL_SET_VOL_OFFSET_T;

/* Internal Message Structure to perform the Set Gain Setting operation */
typedef struct
{
    vcp_internal_msg_t   id;
    VcpProfileHandle     prfl_hndl;
    ServiceHandle     srvc_hndl;
    int8                 gain_setting;
}VCP_INTERNAL_SET_GAIN_SETTING_T;

/* Internal Message Structure to perform the AICS Unmute operation */
typedef VCP_INTERNAL_SET_GAIN_SETTING_T VCP_INTERNAL_AICS_UNMUTE_T;

/* Internal Message Structure to perform the AICS Mute operation */
typedef VCP_INTERNAL_SET_GAIN_SETTING_T VCP_INTERNAL_AICS_MUTE_T;

/* Internal Message Structure to perform the AICS Set Manual Gain mode operation */
typedef VCP_INTERNAL_SET_GAIN_SETTING_T VCP_INTERNAL_AICS_SET_MANUAL_GAIN_MODE_T;

/* Internal Message Structure to perform the AICS Set Automatic Gain mode operation */
typedef VCP_INTERNAL_SET_GAIN_SETTING_T VCP_INTERNAL_AICS_SET_AUTOMATIC_GAIN_MODE_T;

/* Internal Message Structure to perform the Set Initial Volume operation */
typedef struct
{
    vcp_internal_msg_t                   id;
    VcpProfileHandle                     prfl_hndl;
    uint8                                initial_vol;
}VCP_INTERNAL_SET_INITIAL_VOL_OP_T;

typedef struct ProfileHandleListElement
{
    struct ProfileHandleListElement    *next;
    struct ProfileHandleListElement    *prev;
    ServiceHandle                   profile_handle;
} ProfileHandleListElm_t;

typedef struct v
{
    CsrBtGattId gattId;
    CsrCmnList_t profile_handle_list;
} vcp_main_inst;

/* The Volume Control Profile internal structure. */
typedef struct __VCP
{
    AppTaskData lib_task;
    AppTask app_task;

    /*! ID of the connection */
    connection_id_t cid;

    /*! VCS start handle */
    uint16 start_handle;
    /*! VCS end handle */
    uint16 end_handle;

    /*! Profile handle of the VCP instance*/
    VcpProfileHandle vcp_srvc_hdl;
    /*! Service handle of the VCS client associated to this VCP instance*/
    ServiceHandle vcs_srvc_hdl;

    /*! List of the VOCS instances to initialise */
    vcp_vocs_inst_t *first_vocs_inst;
    vcp_vocs_inst_t *last_vocs_inst;

    /*! List of Service handle of the VOCS instances discovered in the remote device */
    vcp_vocs_srvc_hndl_t * first_vocs_srvc_hndl;
    vcp_vocs_srvc_hndl_t * last_vocs_srvc_hndl;

    /*! VOCS instance counter */
    uint16 vocs_counter;
    /*! VOCS instance number */
    uint16 vocs_num;


    /*! List of the AICS instances to initialise */
    vcp_aics_inst_t * first_aics_inst;
    vcp_aics_inst_t * last_aics_inst;

    /*! List of Service handle of the AICS instances discovered in the remote device */
    vcp_aics_srvc_hndl_t * first_aics_srvc_hndl;
    vcp_aics_srvc_hndl_t * last_aics_srvc_hndl;

    /*! AICS instance counter */
    uint16 aics_counter;
    /*! AICS instance number */
    uint16 aics_num;

    /*! Request for the secondary service */
    bool secondary_service_req;
    /*! Peer device */
    bool is_peer_device;

    /*! Change counters */
    uint8 vcs_change_counter;
    uint8 vocs_change_counter;
    uint8 aics_change_counter;
    uint8 retryCounter;

    /*! Pending operation */
    uint16 pending_op;

    /*! Pending control point operands */
    uint8 volume_setting_pending;
    int16 volume_offset_pending;
    int8  gain_setting_pending;
} VCP;

CsrBool vcpSrvcHndlFindByGattId(CsrCmnListElm_t *elem, void *data);
CsrBool vcpInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

#define VCP_ADD_SERVICE_HANDLE(_List) \
    (ProfileHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ProfileHandleListElm_t))

#define VCP_REMOVE_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        vcpInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define VCP_FIND_SERVICE_HANDLE_BY_GATTID(_List,_id) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        vcpSrvcHndlFindByGattId,(void *)(&(_id))))

#define ADD_VCP_CLIENT_INST(_List) \
                         ServiceHandleNewInstance((void**)(&(_List)),sizeof(VCP))

#define FREE_VCP_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define FIND_VCP_INST_BY_SERVICE_HANDLE(_Handle) \
                              (VCP *)ServiceHandleGetInstanceData(_Handle)

#define MAKE_VCP_MESSAGE(TYPE) \
                TYPE *message = (TYPE *) CsrPmemZalloc(sizeof(TYPE))

#define MAKE_VCP_INTERNAL_MESSAGE(TYPE) \
                TYPE##_T *message = (TYPE##_T *) CsrPmemZalloc(sizeof(TYPE##_T))

#define MAKE_VCP_MESSAGE_WITH_LEN(TYPE, LEN) MAKE_VCP_MESSAGE(TYPE)

/* Assumes message struct with
 *    uint16 size_value;
 *    uint8 value[1];
 */
#define MAKE_VCP_MESSAGE_WITH_LEN_U8(TYPE, LEN) MAKE_VCP_MESSAGE_WITH_LEN(TYPE, LEN)

#define MAKE_VCP_MESSAGE_WITH_VALUE(TYPE, SIZE, VALUE) \
        MAKE_VCP_MESSAGE_WITH_LEN_U8(TYPE, SIZE);          \
        memmove(message->value, (VALUE), (SIZE));           \
        message->sizeValue = (SIZE)

CsrBool vcpProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data);

CsrBool vcpProfileHndlFindByVcsSrvcHndl(CsrCmnListElm_t *elem, void *data);


#define FIND_VCP_INST_BY_PROFILE_HANDLE(_Handle) \
                              (VCP *)ServiceHandleGetInstanceData(_Handle)

#define VCP_FIND_PROFILE_HANDLE_BY_BTCONNID(_List,_id) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        vcpProfileHndlFindByBtConnId,(void *)(&(_id))))

#define VCP_FIND_PROFILE_HANDLE_BY_VCS_SERVICE_HANDLE(_List,_ServiceHandle) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        vcpProfileHndlFindByVcsSrvcHndl,(void *)(&(_ServiceHandle))))


#endif /* VCP_PRIVATE_H */
