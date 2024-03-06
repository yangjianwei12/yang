/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #2 $
******************************************************************************/

#ifndef PBP_PRIVATE_H
#define PBP_PRIVATE_H

#include <stdlib.h>

#include "csr_bt_gatt_lib.h"
#include "pbp.h"
#include "service_handle.h"

typedef struct PbpProfileHandleListElement
{
    struct ProfileHandleListElement    *next;
    struct ProfileHandleListElement    *prev;
    ServiceHandle                       profileHandle;
} PbpProfileHandleListElm;

/* The PBP internal structure. */
typedef struct
{
    AppTaskData libTask;
    AppTask appTask;

    /*! ID of the connection */
    PbpConnectionId cid;

    /*! Profile handle of the PBP instance*/
    PbpProfileHandle pbpSrvcHndl;
    /*! Broadcast source handle */
    PbpProfileHandle bcastSrcHandle;

    /*! Number of subgroups of the broadcast source*/
    uint8 numSubGroup;
} PBP;

typedef struct
{
    CsrCmnList_t profileHandleList;
} PbpMainInst;


CsrBool findPbpInst(CsrCmnListElm_t *elem, void *data);

CsrBool pbpInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

CsrBool pbpFindPbpInstFromBcastSrcHandle(CsrCmnListElm_t* elem, void* data);

PbpMainInst *pbpGetMainInstance(void);

#define NO_PROFILE_HANDLE ((PbpProfileHandle) 0xFFFFu)

#define PbpMessageSend(TASK, ID, MSG) { \
                       MSG->type = ID; \
                       CsrSchedMessagePut(TASK, PBP_PRIM, MSG);\
                       }

#define ADD_PBP_INST(_List) \
                         ServiceHandleNewInstance((void**)(&(_List)),sizeof(PBP))

#define FREE_PBP_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define ADD_PBP_SERVICE_HANDLE(_List) \
    (PbpProfileHandleListElm *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(PbpProfileHandleListElm))

#define REMOVE_PBP_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        pbpInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define FIND_PBP_INST_BY_PROFILE_HANDLE(_Handle) \
                              (PBP *)ServiceHandleGetInstanceData(_Handle)

#define FIND_PBP_PROFILE_HANDLE_FROM_INST(_List,_id) \
                              ((PbpProfileHandleListElm *)CsrCmnListSearch(&(_List), \
                                        findPbpInst,(void *)(&(_id))))

#define FIND_PBP_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(_List,_bcastSrcHandle) \
                              ((PbpProfileHandleListElm *)CsrCmnListSearch(&(_List), \
                                        pbpFindPbpInstFromBcastSrcHandle,(void *)(&(_bcastSrcHandle))))

#define MAKE_PBP_MESSAGE(TYPE)   TYPE *message = (TYPE*)CsrPmemZalloc(sizeof(TYPE))

#define CHECK_STANDARD_BROADCAST_TYPE(x) (x == SQ_PUBLIC_BROADCAST)
#define CHECK_HQ_BROADCAST_TYPE(x) (x == HQ_PUBLIC_BROADCAST)
#define CHECK_ALL_PBP_BROADCAST_TYPE_SUPPORTED(x) (x == (SQ_PUBLIC_BROADCAST | HQ_PUBLIC_BROADCAST))

#endif /* PBP_PRIVATE_H */
