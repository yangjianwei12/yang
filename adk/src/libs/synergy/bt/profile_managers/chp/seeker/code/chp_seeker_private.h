/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CHP_SEEKER_PRIVATE_H
#define CHP_SEEKER_PRIVATE_H

#include <stdlib.h>

#include "csr_bt_gatt_lib.h"
#include "chp_seeker.h"
#include "csr_log_text_2.h"
#include "service_handle.h"


#define ChpSeekerMessageSend(TASK, MSG) CsrSchedMessagePut(TASK, CHP_SEEKER_PRIM, MSG)

/* Define logging related variables */
#define CHP_SEEKER_LOG_ENABLE
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtChpsLto);

/* Element of the list of TDS service handles */
struct TdsSrvcHndl
{
    ServiceHandle srvcHndl;
    struct TdsSrvcHndl *next;
};

typedef struct TdsSrvcHndl ChpSeekerTdsSrvcHndl;

typedef struct ProfileHandleListElement
{
    struct ProfileHandleListElement    *next;
    struct ProfileHandleListElement    *prev;
    ServiceHandle                   profile_handle;
} ProfileHandleListElm_t;

/* The Connection Handover Profile Seeker internal structure. */
typedef struct
{
    AppTaskData libTask;
    AppTask appTask;

    /*! ID of the connection */
    ChpSeekerConnectionId cid;

    /*! Profile handle of the CHP Seeker instance*/
    ChpSeekerProfileHandle chpSeekerSrvcHndl;
    /*! Service handle of the TDS client associated to this CHP Seeker instance*/
    ServiceHandle tdsSrvcHndl;
} CHP;

typedef struct
{
    CsrCmnList_t profileHandleList;
} ChpSeekerMainInst;

CsrBool chpSeekerInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

CsrBool chpSeekerProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data);

CsrBool chpSeekerProfileHndlFindByTdsSrvcHndl(CsrCmnListElm_t *elem, void *data);

ChpSeekerMainInst *chpSeekerGetMainInstance(void);

#define ADD_CHP_INST(_List) \
                         ServiceHandleNewInstance((void**)(&(_List)),sizeof(CHP))

#define FREE_CHP_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define ADD_CHP_SERVICE_HANDLE(_List) \
    (ProfileHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ProfileHandleListElm_t))

#define REMOVE_CHP_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        chpSeekerInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define FIND_CHP_INST_BY_PROFILE_HANDLE(_Handle) \
                              (CHP *)ServiceHandleGetInstanceData(_Handle)

#define FIND_CHP_PROFILE_HANDLE_BY_BTCONNID(_List,_id) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        chpSeekerProfileHndlFindByBtConnId,(void *)(&(_id))))

#define FIND_CHP_PROFILE_HANDLE_BY_TDS_SERVICE_HANDLE(_List,_ServiceHandle) \
                              ((ProfileHandleListElm_t *)CsrCmnListSearch(&(_List), \
                                        chpSeekerProfileHndlFindByTdsSrvcHndl,(void *)(&(_ServiceHandle))))

#endif /* CHP_SEEKER_PRIVATE_H */
