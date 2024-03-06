/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef TMAP_CLIENT_PRIVATE_H
#define TMAP_CLIENT_PRIVATE_H

#include <stdlib.h>

#include "csr_bt_gatt_lib.h"
#include "tmap_client_lib.h"
#include "service_handle.h"


#define TmapClientMessageSend(TASK, MSG) CsrSchedMessagePut(TASK, TMAP_CLIENT_PRIM, MSG)

typedef struct ProfileHandleListElement
{
    struct ProfileHandleListElement   *next;
    struct ProfileHandleListElement   *prev;
    ServiceHandle                     profileHandle;
} TmapClientProfileHandleListElm;

typedef struct 
{
    struct ProfileHandleListElement   *next;
    struct ProfileHandleListElement   *prev;
    TmapClientConnectionId             cid;
} TmapClientCidListElm;

typedef struct
{
    CsrCmnList_t cidList;
} TmapClientCidList;

/* The Telephony Audio and Media Profile internal structure. */
typedef struct
{
    AppTaskData libTask;
    AppTask appTask;

    TmapClientCidList tmapClientCidList;                 /*! cid list */
    TmapClientProfileHandle tmapSrvcHndl;                /*! Profile handle of the TMAP Client instance */
    TmapClientProfileHandle bcastSrcHandle;              /*! Broadcast source handle */
    ServiceHandle tmasSrvcHndl;                          /*! Service handle of the TMAS client associated to this TMAP instance */
} TMAP;

typedef struct
{
    CsrCmnList_t profileHandleList;
} TmapClientMainInst;


CsrBool tmapClientFindTmapInst(CsrCmnListElm_t *elem, void *data);

CsrBool tmapClientInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

CsrBool tmapClientProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data);

CsrBool tmapClientProfileHndlFindByTmasSrvcHndl(CsrCmnListElm_t *elem, void *data);

CsrBool tmapClientElemFindByCid(CsrCmnListElm_t *elem, void *data);

CsrBool tmapClientFindTmapInstFromBcastSrcHandle(CsrCmnListElm_t *elem, void *data);

TmapClientMainInst *tmapClientGetMainInstance(void);

#define ADD_TMAP_CLIENT_INST(_List) \
                         ServiceHandleNewInstance((void**)(&(_List)),sizeof(TMAP))

#define FREE_TMAP_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define ADD_TMAP_CLIENT_SERVICE_HANDLE(_List) \
    (TmapClientProfileHandleListElm *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(TmapClientProfileHandleListElm))

#define REMOVE_TMAP_CLIENT_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        tmapClientInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(_Handle) \
                              (TMAP *)ServiceHandleGetInstanceData(_Handle)

#define FIND_TMAP_CLIENT_PROFILE_HANDLE_FROM_INST(_List,_id) \
                              ((TmapClientProfileHandleListElm *)CsrCmnListSearch(&(_List), \
                                        tmapClientFindTmapInst,(void *)(&(_id))))

#define FIND_TMAP_CLIENT_PROFILE_HANDLE_BY_BTCONNID(_List,_id) \
                              ((TmapClientProfileHandleListElm *)CsrCmnListSearch(&(_List), \
                                        tmapClientProfileHndlFindByBtConnId,(void *)(&(_id))))

#define FIND_TMAP_CLIENT_PROFILE_HANDLE_BY_TMAS_SERVICE_HANDLE(_List,_ServiceHandle) \
                              ((TmapClientProfileHandleListElm *)CsrCmnListSearch(&(_List), \
                                        tmapClientProfileHndlFindByTmasSrvcHndl,(void *)(&(_ServiceHandle))))

#define ADD_TMAP_CLIENT_CID_ELEM(_List) \
    (TmapClientCidListElm *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(TmapClientCidListElm))

#define REMOVE_TMAP_CLIENT_CID_ELEM(_List,_cid) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        tmapClientElemFindByCid,(void *)(&(_cid)))

#define FIND_TMAP_CLIENT_CID_ELEM(_List,_cid) \
                              ((TmapClientCidListElm *)CsrCmnListSearch(&(_List), \
                                        tmapClientElemFindByCid,(void *)(&(_cid))))

#define FIND_TMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(_List,_bcastSrcHandle) \
                              ((TmapClientProfileHandleListElm *)CsrCmnListSearch(&(_List), \
                                        tmapClientFindTmapInstFromBcastSrcHandle,(void *)(&(_bcastSrcHandle))))
#endif /* TMAP_CLIENT_PRIVATE_H */
