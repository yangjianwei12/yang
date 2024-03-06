/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#ifndef MICP_PRIVATE_H
#define MICP_PRIVATE_H

#include <stdlib.h>

#include "csr_bt_gatt_lib.h"

#include "gatt_aics_client.h"

#include "micp.h"
#include "service_handle.h"

#define MICP_MICS_UUID  (0x2BC3)
#define MICP_AICS_UUID  (0x1843)


#define MICP_AICS_INVALID_CHANGE_COUNTER_ERR (0x80)

#define MicpMessageSend(TASK, MSG) CsrSchedMessagePut(TASK, MICP_PRIM, MSG);
#define MicpMessageSendConditionally(TASK, ID, MSG, CMD) MicpMessageSend(TASK, MSG)

typedef ServiceHandle micp_profile_t;

/* Element of the list of AICS instances */
typedef struct aics_inst
{
    connection_id_t cid;
    uint16 start_handle;
    uint16 end_handle;
    GattAicsClientDeviceData device_data;
    struct aics_inst *next;
} micp_aics_inst_t;

/* Element of the list of AICS service handles */
typedef struct aics_srvc_hndl
{
    ServiceHandle srvc_hdnl;
    struct aics_srvc_hndl *next;
} micp_aics_srvc_hndl_t;

/* Pending operation of the MICP profile */
typedef uint16 MicpPendingOp;

#define MICP_PENDING_OP_NONE            (MicpPendingOp)0x0000
#define MICP_PENDING_SET_MUTE_VALUE_OP  (MicpPendingOp)0x0001

/* Opcodes of the set mute operation */
typedef uint16 MicpMicsControlPointOpcodes;

#define MICP_SET_MUTE_VALUE_OP      (MicpMicsControlPointOpcodes)0x0000
#define MICP_MICS_LAST_OP           (MicpMicsControlPointOpcodes)0x0001

/* Opcodes of the audio input control point operation */
typedef uint16 MicpAicsControlPointOpcodes;

#define MICP_AICS_SET_GAIN_SETTING_OP       (MicpAicsControlPointOpcodes)0x0001
#define MICP_AICS_UNMUTE_OP                 (MicpAicsControlPointOpcodes)0x0002
#define MICP_AICS_MUTE_OP                   (MicpAicsControlPointOpcodes)0x0003
#define MICP_AICS_SET_MNL_GAIN_MODE_OP      (MicpAicsControlPointOpcodes)0x0004
#define MICP_AICS_SET_ATMTC_GAIN_OP         (MicpAicsControlPointOpcodes)0x0005
#define MICP_AICS_LAST_OP                   (MicpAicsControlPointOpcodes)0x0006

/* Lib internal messages */
typedef uint16 MicpInternalMsg;

#define MICP_INTERNAL_MSG_BASE                      (MicpInternalMsg)0x0000
#define MICP_INTERNAL_SET_MUTE_VALUE                (MicpInternalMsg)(MICP_INTERNAL_MSG_BASE + 0x0001)
#define MICP_INTERNAL_AICS_UNMUTE                   (MicpInternalMsg)(MICP_INTERNAL_MSG_BASE + 0x0002)
#define MICP_INTERNAL_AICS_MUTE                     (MicpInternalMsg)(MICP_INTERNAL_MSG_BASE + 0x0003)
#define MICP_INTERNAL_AICS_SET_MANUAL_GAIN_MODE     (MicpInternalMsg)(MICP_INTERNAL_MSG_BASE + 0x0004)
#define MICP_INTERNAL_AICS_SET_AUTOMATIC_GAIN_MODE  (MicpInternalMsg)(MICP_INTERNAL_MSG_BASE + 0x0005)
#define MICP_INTERNAL_MSG_TOP                       (MicpInternalMsg)(MICP_INTERNAL_MSG_BASE + 0x0006)

/* Internal Message Structure to perform the Set Mute Value operation */
typedef struct
{
    MicpInternalMsg   id;
    MicpProfileHandle     prfl_hndl;
    uint8                mute_value;
} MICP_INTERNAL_SET_MUTE_VALUE_T;

/* Internal Message Structure to perform the Set Gain Setting operation */
typedef struct
{
    MicpInternalMsg   id;
    MicpProfileHandle     prfl_hndl;
    ServiceHandle     srvc_hndl;
    int8                 gain_setting;
}MICP_INTERNAL_SET_GAIN_SETTING_T;

/* Internal Message Structure to perform the AICS Unmute operation */
typedef MICP_INTERNAL_SET_GAIN_SETTING_T MICP_INTERNAL_AICS_UNMUTE_T;

/* Internal Message Structure to perform the AICS Mute operation */
typedef MICP_INTERNAL_SET_GAIN_SETTING_T MICP_INTERNAL_AICS_MUTE_T;

/* Internal Message Structure to perform the AICS Set Manual Gain mode operation */
typedef MICP_INTERNAL_SET_GAIN_SETTING_T MICP_INTERNAL_AICS_SET_MANUAL_GAIN_MODE_T;

/* Internal Message Structure to perform the AICS Set Automatic Gain mode operation */
typedef MICP_INTERNAL_SET_GAIN_SETTING_T MICP_INTERNAL_AICS_SET_AUTOMATIC_GAIN_MODE_T;

typedef struct ProfileHandleListElement
{
    struct ProfileHandleListElement    *next;
    struct ProfileHandleListElement    *prev;
    ServiceHandle                   profile_handle;
} ProfileHandleListElm_t;

typedef struct
{
    CsrBtGattId gattId;
    CsrCmnList_t profile_handle_list;
} micp_main_inst;

/* The Microphone Control Profile internal structure. */
typedef struct
{
    AppTaskData lib_task;
    AppTask app_task;

    /*! ID of the connection */
    connection_id_t cid;

    /*! MICS start handle */
    uint16 start_handle;
    /*! MICS end handle */
    uint16 end_handle;

    /*! Profile handle of the MICP instance*/
    MicpProfileHandle micp_srvc_hdl;
    /*! Service handle of the MICS client associated to this MICP instance*/
    ServiceHandle mics_srvc_hdl;

    /*! List of the AICS instances to initialise */
    micp_aics_inst_t * first_aics_inst;
    micp_aics_inst_t * last_aics_inst;

    /*! List of Service handle of the AICS instances discovered in the remote device */
    micp_aics_srvc_hndl_t * first_aics_srvc_hndl;
    micp_aics_srvc_hndl_t * last_aics_srvc_hndl;

    /*! AICS instance counter */
    uint16 aics_counter;
    /*! AICS instance number */
    uint16 aics_num;

    /*! Request for the secondary service */
    bool secondary_service_req;
    /*! Peer device */
    bool is_peer_device;

    /*! Pending operation */
    uint16 pending_op;

    /*! Pending control point operands */
    uint8 mute_setting_pending;
} MICP;

CsrBool micpSrvcHndlFindByGattId(CsrCmnListElm_t *elem, void *data);
CsrBool micpInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

#define MICP_ADD_SERVICE_HANDLE(_List) \
    (ProfileHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ProfileHandleListElm_t))

#define MICP_REMOVE_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        micpInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define MICP_FIND_SERVICE_HANDLE_BY_GATTID(_List,_id) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        micpSrvcHndlFindByGattId,(void *)(&(_id))))

#define ADD_MICP_CLIENT_INST(_List) \
                         ServiceHandleNewInstance((void**)(&(_List)),sizeof(MICP))

#define FREE_MICP_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define FIND_MICP_INST_BY_SERVICE_HANDLE(_Handle) \
                              (MICP *)ServiceHandleGetInstanceData(_Handle)

#define MAKE_MICP_MESSAGE(TYPE) \
                TYPE *message = (TYPE *) CsrPmemZalloc(sizeof(TYPE))

#define MAKE_MICP_INTERNAL_MESSAGE(TYPE) \
                TYPE##_T *message = (TYPE##_T *) CsrPmemZalloc(sizeof(TYPE##_T))

#define MAKE_MICP_MESSAGE_WITH_LEN(TYPE, LEN) \
                TYPE *message = (TYPE *) CsrPmemZalloc(sizeof(TYPE) + ((LEN) ? (LEN) - 1 : 0))

/* Assumes message struct with
 *    uint16 size_value;
 *    uint8 value[1];
 */
#define MAKE_MICP_MESSAGE_WITH_LEN_U8(TYPE, LEN) MAKE_MICP_MESSAGE_WITH_LEN(TYPE, LEN)

#define MAKE_MICP_MESSAGE_WITH_VALUE(TYPE, SIZE, VALUE) \
        MAKE_MICP_MESSAGE_WITH_LEN_U8(TYPE, SIZE);          \
        CsrMemMove(message->value, (VALUE), (SIZE));           \
        message->sizeValue = (SIZE)

CsrBool micpProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data);

CsrBool micpProfileHndlFindByMicsSrvcHndl(CsrCmnListElm_t *elem, void *data);


#define FIND_MICP_INST_BY_PROFILE_HANDLE(_Handle) \
                              (MICP *)ServiceHandleGetInstanceData(_Handle)

#define MICP_FIND_PROFILE_HANDLE_BY_BTCONNID(_List,_id) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        micpProfileHndlFindByBtConnId,(void *)(&(_id))))

#define MICP_FIND_PROFILE_HANDLE_BY_MICS_SERVICE_HANDLE(_List,_ServiceHandle) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        micpProfileHndlFindByMicsSrvcHndl,(void *)(&(_ServiceHandle))))


#endif /* MICP_PRIVATE_H */
