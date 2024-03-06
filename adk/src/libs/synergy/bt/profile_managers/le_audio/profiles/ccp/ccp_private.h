/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CCP_PRIVATE_H
#define CCP_PRIVATE_H

#include <stdlib.h>
#include "csr_bt_gatt_lib.h"
#include "ccp.h"
#include "service_handle.h"
#include "csr_list.h"

#define CCP_TBS_INVALID_CHANGE_COUNTER_ERR (0x80)

#define CcpMessageSend(TASK, MSG) CsrSchedMessagePut(TASK, CCP_PRIM, MSG);
#define CcpMessageSendConditionally(TASK, ID, MSG, CMD) CcpMessageSend(TASK, MSG)

typedef ServiceHandle ccp_profile_t;


/* Opcodes of the call control point operation */
typedef enum __ccp_tbs_control_point_opcodes
{
    ccp_accept_op,
    ccp_terminate_op,
    ccp_local_hold_op,
    ccp_local_retrieve_op,
    ccp_originiate_op,
    ccp_join_op
} ccp_tbs_control_point_opcodes_t;


/* Enum For lib internal messages */
typedef enum
{
    CCP_INTERNAL_MSG_BASE = 0,
    CCP_INTERNAL_WRITE_SIGNAL_STRENGTH_INTERVAL_CFM,
    CCP_INTERNAL_WRITE_CALL_CONTROL_POINT_CFM,
    CCP_INTERNAL_MSG_TOP
}ccp_internal_msg_t;

/* Internal Message Structure to perform the Write Call control point operation */
typedef struct
{
    ccp_internal_msg_t   id;
    CcpProfileHandle prfl_hndl;
    uint8            callId;
} CcpInternalWriteCallControlPointCfm;


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
} ccp_main_inst;


/* Element of the list of TBS service handles */
struct tbs_srvc_hndl
{
    ServiceHandle srvc_hdnl;
    struct tbs_srvc_hndl *next;
};

typedef struct tbs_srvc_hndl ccp_tbs_srvc_hndl_t;

/* Element of the list of TBS instances */
struct tbs_inst
{
    connection_id_t cid;
    uint16 start_handle;
    uint16 end_handle;
    GattTelephoneBearerClientDeviceData device_data;
    struct tbs_inst *next;
};

typedef struct tbs_inst ccp_tbs_inst_t;


/* The Call Control Profile internal structure. */
typedef struct __CCP
{
    AppTaskData lib_task;
    AppTask appTask;

    /*! ID of the connection */
    connection_id_t cid;

    /*! GTBS start handle */
    uint16 gtbs_start_handle;
    /*! GTBS end handle */
    uint16 gtbs_end_handle;

    /*! List of the TBS instances to initialise */
    ccp_tbs_inst_t *first_tbs_inst;
    ccp_tbs_inst_t *last_tbs_inst;

    /*! List of Service handle of the TBS instances discovered in the remote device */
    ccp_tbs_srvc_hndl_t * first_tbs_srvc_hndl;
    ccp_tbs_srvc_hndl_t * last_tbs_srvc_hndl;

    /*! Profile handle of the CCP instance*/
    CcpProfileHandle ccp_srvc_hdl;
    /*! Service handle of the TBS client associated to this CCP instance*/
    ServiceHandle tbs_srvc_hdl;

    /*! Peer device */
    bool is_peer_device;

    /*! true if TBS services required in addition to gTBS */
    bool tbsRequired;

    /*! Pending operation */
    uint16 pending_op;

} CCP;


CsrBool ccpSrvcHndlFindByGattId(CsrCmnListElm_t *elem, void *data);
CsrBool ccpInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);


CsrBool ccpProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data);

CsrBool ccpProfileHndlFindByTbsSrvcHndl(CsrCmnListElm_t *elem, void *data);

ccp_main_inst *ccpGetMainInstance(void);

#define CCP_ADD_SERVICE_HANDLE(_List) \
    (ProfileHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ProfileHandleListElm_t))

#define CCP_REMOVE_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        ccpInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define CCP_FIND_SERVICE_HANDLE_BY_GATTID(_List,_id) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        ccpSrvcHndlFindByGattId,(void *)(&(_id))))

#define ADD_CCP_CLIENT_INST(_List) \
                         ServiceHandleNewInstance((void**)(&(_List)),sizeof(CCP))

#define FREE_CCP_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define FIND_CCP_INST_BY_SERVICE_HANDLE(_Handle) \
                              (CCP *)ServiceHandleGetInstanceData(_Handle)

#define FIND_CCP_INST_BY_PROFILE_HANDLE(_Handle) \
                              (CCP *)ServiceHandleGetInstanceData(_Handle)

#define CCP_FIND_PROFILE_HANDLE_BY_BTCONNID(_List,_id) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        ccpProfileHndlFindByBtConnId,(void *)(&(_id))))

#define CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(_List,_ServiceHandle) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        ccpProfileHndlFindByTbsSrvcHndl,(void *)(&(_ServiceHandle))))


#define MAKE_CCP_MESSAGE(TYPE) \
                TYPE *message = (TYPE *) CsrPmemZalloc(sizeof(TYPE))

#define MAKE_CCP_INTERNAL_MESSAGE(TYPE) \
                TYPE##_T *message = (TYPE##_T *) CsrPmemZalloc(sizeof(TYPE##_T))

#define MAKE_CCP_MESSAGE_WITH_LEN(TYPE, LEN) \
                TYPE *message = (TYPE *) CsrPmemZalloc(sizeof(TYPE) + LEN)



#endif /* CCP_PRIVATE_H */
