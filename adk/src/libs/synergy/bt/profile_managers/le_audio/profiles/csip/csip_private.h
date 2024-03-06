/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CSIP_PRIVATE_H
#define CSIP_PRIVATE_H

#include <stdlib.h>

#include "csr_bt_gatt_lib.h"
#include "csip.h"
#include "service_handle.h"
#include"csr_bt_core_stack_pmalloc.h"

#define CSIP_CSIS_UUID  (0x1846)
#define CSIP_VOCS_UUID  (0x1845)
#define CSIP_AICS_UUID  (0x1843)

#define CSIP_CSIS_INVALID_CHANGE_COUNTER_ERR (0x80)
#define CSIP_VOCS_INVALID_CHANGE_COUNTER_ERR (CSIP_CSIS_INVALID_CHANGE_COUNTER_ERR)
#define CSIP_AICS_INVALID_CHANGE_COUNTER_ERR (CSIP_CSIS_INVALID_CHANGE_COUNTER_ERR)

#define CsipMessageSend(TASK, MSG) CsrSchedMessagePut(TASK, CSIP_PRIM, MSG);
#define CsipMessageSendConditionally(TASK, ID, MSG, CMD) CsipMessageSend(TASK, MSG)

typedef ServiceHandle CsipProfile;


/* Opcodes of the volume control point operation */
typedef uint16 CsipPendingOp ;
#define CSIP_PENDING_OP_NONE                        (CsipPendingOp)0x0000
#define CSIP_PENDING_RELATIVE_VOLUME_DOWN_OP        (CsipPendingOp)0x0001
#define CSIP_PENDING_RELATIVE_VOLUME_UP_OP          (CsipPendingOp)0x0002
#define CSIP_PENDING_UNMUTE_RELATIVE_VOLUME_DOWN_OP (CsipPendingOp)0x0003
#define CSIP_PENDING_UNMUTE_RELATIVE_VOLUME_UP_OP   (CsipPendingOp)0x0004
#define CSIP_PENDING_SET_ABSOLUTE_VOLUME_OP         (CsipPendingOp)0x0005
#define CSIP_PENDING_UNMUTE_OP                      (CsipPendingOp)0x0006
#define CSIP_PENDING_MUTE_OP                        (CsipPendingOp)0x0007
#define CSIP_PENDING_SET_VOLUME_OFFSET_OP           (CsipPendingOp)0x0008
#define CSIP_PENDING_SET_GAIN_SETTING_OP            (CsipPendingOp)0x0009
#define CSIP_PENDING_AICS_UNMUTE_OP                 (CsipPendingOp)0x000A
#define CSIP_PENDING_AICS_MUTE_OP                   (CsipPendingOp)0x000B
#define CSIP_PENDING_SET_MNL_GAIN_MODE_OP           (CsipPendingOp)0x000C
#define CSIP_PENDING_SET_ATMTC_GAIN_MODE_OP         (CsipPendingOp)0x000D
#define CSIP_PENDING_SET_INITIAL_VOL_OP             (CsipPendingOp)0x000E

/* Opcodes of the volume control point operation */
typedef uint16 CsipCsisControlPointOpcodes ;

#define CSIP_RELATIVE_VOLUME_DOWN_OP            (CsipCsisControlPointOpcodes)0x0000
#define CSIP_RELATIVE_VOLUME_UP_OP              (CsipCsisControlPointOpcodes)0x0001
#define CSIP_UNMUTE_RELATIVE_VOLUME_DOWN_OP     (CsipCsisControlPointOpcodes)0x0002
#define CSIP_UNMUTE_RELATIVE_VOLUME_UP_OP       (CsipCsisControlPointOpcodes)0x0003
#define CSIP_SET_ABSOLUTE_VOLUME_OP             (CsipCsisControlPointOpcodes)0x0004
#define CSIP_UNMUTE_OP                          (CsipCsisControlPointOpcodes)0x0005
#define CSIP_MUTE_OP                            (CsipCsisControlPointOpcodes)0x0006

/* Opcodes of the volume offset control point operation */
typedef uint16 CsipVocsControlPointOpcodes;

#define CSIP_VOCS_SET_VOLUME_OFFSET_OP (CsipVocsControlPointOpcodes)0x0001

/* Opcodes of the audio input control point operation */
typedef uint16 CsipAicsControlPointOpcodes ;

#define CSIP_AICS_SET_GAIN_SETTING_OP       (CsipAicsControlPointOpcodes)0x0001
#define CSIP_AICS_UNMUTE_OP                 (CsipAicsControlPointOpcodes)0x0002
#define CSIP_AICS_MUTE_OP                   (CsipAicsControlPointOpcodes)0x0003
#define CSIP_AICS_SET_MNL_GAIN_MODE_OP      (CsipAicsControlPointOpcodes)0x0004
#define CSIP_AICS_SET_ATMTC_GAIN_MODE_OP    (CsipAicsControlPointOpcodes)0x0005

/* Enum For LIB internal messages */
typedef uint16 CsipInternalMsg ;

#define CSIP_INTERNAL_MSG_BASE                      (CsipInternalMsg)0x0000
#define CSIP_INTERNAL_REL_VOL_DOWN                  (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x0001)
#define CSIP_INTERNAL_REL_VOL_UP                    (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x0002)
#define CSIP_INTERNAL_UNMUTE_REL_VOL_DOWN           (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x0003)
#define CSIP_INTERNAL_UNMUTE_REL_VOL_UP             (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x0004)
#define CSIP_INTERNAL_ABS_VOL                       (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x0005)
#define CSIP_INTERNAL_MUTE                          (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x0006)
#define CSIP_INTERNAL_UNMUTE                        (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x0007)
#define CSIP_INTERNAL_SET_VOL_OFFSET                (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x0008)
#define CSIP_INTERNAL_SET_GAIN_SETTING              (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x0009)
#define CSIP_INTERNAL_AICS_UNMUTE                   (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x000A)
#define CSIP_INTERNAL_AICS_MUTE                     (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x000B)
#define CSIP_INTERNAL_AICS_SET_MANUAL_GAIN_MODE     (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x000C)
#define CSIP_INTERNAL_AICS_SET_AUTOMATIC_GAIN_MODE  (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x000D)
#define CSIP_INTERNAL_SET_INITIAL_VOL_OP            (CsipInternalMsg)(CSIP_INTERNAL_MSG_BASE + 0x000E)

/* Internal Message Structure to perform the Relative Volume Down operation */
typedef struct
{
    CsipInternalMsg   id;
    CsipProfileHandle prflHndl;
    uint8             volumeSetting;
} CsipInternalRelVolDown;

/* Internal Message Structure to perform the Relative Volume Up operation */
typedef CsipInternalRelVolDown CsipInternalRelVolUp;

/* Internal Message Structure to perform the Unmute Relative Volume Downn operation */
typedef CsipInternalRelVolDown CsipInternalUnmuteRelVolDown;

/* Internal Message Structure to perform the Unmute Relative Volume Up operation */
typedef CsipInternalRelVolDown CsipInternalUnmuteRelVolUp;

/* Internal Message Structure to perform the Absolte Volume operation */
typedef CsipInternalRelVolDown CsipInternalAbsVol;

/* Internal Message Structure to perform the Unmute operation */
typedef CsipInternalRelVolDown CsipInternalUnmute;

/* Internal Message Structure to perform the Mute operation */
typedef CsipInternalRelVolDown CsipInternalMute;

/* Internal Message Structure to perform the Set Volume Offset operation */
typedef struct
{
    CsipInternalMsg                id;
    CsipProfileHandle              prflHndl;
    ServiceHandle                  vocsSrvcHndl;
    CsipVocsControlPointOpcodes    opcode;
    int16                          volumeOffset;
}CsipInternalSetVolOffset;

/* Internal Message Structure to perform the Set Gain Setting operation */
typedef struct
{
    CsipInternalMsg   id;
    CsipProfileHandle prflHndl;
    ServiceHandle     srvcHndl;
    int8              gainSetting;
}CsipInternalSetGainSetting;

/* Internal Message Structure to perform the AICS Unmute operation */
typedef CsipInternalSetGainSetting CsipInternalAicsUnmute;

/* Internal Message Structure to perform the AICS Mute operation */
typedef CsipInternalSetGainSetting CsipInternalAicsMute;

/* Internal Message Structure to perform the AICS Set Manual Gain mode operation */
typedef CsipInternalSetGainSetting CsipInternalAicsSetManualGainMode;

/* Internal Message Structure to perform the AICS Set Automatic Gain mode operation */
typedef CsipInternalSetGainSetting CsipInternalAicsSetAutomaticGainMode;

/* Internal Message Structure to perform the Set Initial Volume operation */
typedef struct
{
    CsipInternalMsg   id;
    CsipProfileHandle prflHndl;
    uint8             initialVol;
}CsipInternalAicsSetInitialVolOp;

typedef struct ProfileHandleListElement
{
    struct ProfileHandleListElement    *next;
    struct ProfileHandleListElement    *prev;
    ServiceHandle                   profileHandle;
} ProfileHandleListElm_t;

typedef struct
{
    CsrBtGattId gattId;
    CsrCmnList_t profileHandleList;
} CsipMainInst;

/* The Volume Control Profile internal structure. */
typedef struct
{
    AppTaskData lib_task;
    AppTask app_task;

    /*! ID of the connection */
    connection_id_t cid;

    /*! CSIS start handle */
    uint16 startHandle;
    /*! CSIS end handle */
    uint16 endHandle;

    /*! Profile handle of the CSIP instance*/
    CsipProfileHandle csipSrvcHdl;
    /*! Service handle of the CSIS client associated to this CSIP instance*/
    ServiceHandle csisSrvcHdl;

    /*! Request for the secondary service */
    bool secondaryServiceReq;
    /*! Peer device */
    bool isPeerDevice;

} Csip;

CsrBool csipSrvcHndlFindByGattId(CsrCmnListElm_t *elem, void *data);
CsrBool csipInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

#define CSIP_ADD_SERVICE_HANDLE(_List) \
    (ProfileHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ProfileHandleListElm_t))

#define CSIP_REMOVE_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        csipInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define CSIP_FIND_SERVICE_HANDLE_BY_GATTID(_List,_id) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        csipSrvcHndlFindByGattId,(void *)(&(_id))))

#define ADD_CSIP_CLIENT_INST(_List) \
                         ServiceHandleNewInstance((void**)(&(_List)),sizeof(Csip))

#define FREE_CSIP_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define FIND_CSIP_INST_BY_SERVICE_HANDLE(_Handle) \
                              (Csip *)ServiceHandleGetInstanceData(_Handle)

#define MAKE_CSIP_MESSAGE(TYPE) \
                TYPE##_T *message = (TYPE##_T *) CsrPmemZalloc(sizeof(TYPE##_T))

#define MAKE_CSIP_MESSAGE_WITH_LEN(TYPE, LEN) MAKE_CSIP_MESSAGE(TYPE)
/* Assumes message struct with
 *    uint16 size_value;
 *    uint8 value[1];
 */
#define MAKE_CSIP_MESSAGE_WITH_LEN_U8(TYPE, LEN) MAKE_CSIP_MESSAGE_WITH_LEN(TYPE, LEN)

#define MAKE_CSIP_MESSAGE_WITH_VALUE(TYPE, SIZE, VALUE) \
        MAKE_CSIP_MESSAGE_WITH_LEN_U8(TYPE, SIZE);          \
        memmove(message->value, (VALUE), (SIZE));           \
        message->size_value = (SIZE)

CsrBool csipProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data);

CsrBool csipProfileHndlFindByCsisSrvcHndl(CsrCmnListElm_t *elem, void *data);


#define FIND_CSIP_INST_BY_PROFILE_HANDLE(_Handle) \
                              (Csip *)ServiceHandleGetInstanceData(_Handle)

#define CSIP_FIND_PROFILE_HANDLE_BY_BTCONNID(_List,_id) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        csipProfileHndlFindByBtConnId,(void *)(&(_id))))

#define CSIP_FIND_PROFILE_HANDLE_BY_CSIS_SERVICE_HANDLE(_List,_ServiceHandle) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        csipProfileHndlFindByCsisSrvcHndl,(void *)(&(_ServiceHandle))))


#endif /* CSIP_PRIVATE_H */
