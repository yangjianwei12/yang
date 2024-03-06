/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #2 $
******************************************************************************/

#ifndef GMAP_CLIENT_PRIVATE_H
#define GMAP_CLIENT_PRIVATE_H

#include <stdlib.h>

#include "csr_bt_gatt_lib.h"
#include "gmap_client_lib.h"
#include "service_handle.h"


#define GmapClientMessageSend(TASK, MSG) CsrSchedMessagePut(TASK, GMAP_CLIENT_PRIM, MSG)

typedef struct ProfileHandleListElement
{
    struct ProfileHandleListElement   *next;
    struct ProfileHandleListElement   *prev;
    ServiceHandle                     profileHandle;
} GmapClientProfileHandleListElm;

typedef struct 
{
    struct ProfileHandleListElement   *next;
    struct ProfileHandleListElement   *prev;
    GmapClientConnectionId             cid;
} GmapClientCidListElm;

typedef struct
{
    CsrCmnList_t cidList;
} GmapClientCidList;

/* The Telephony Audio and Media Profile internal structure. */
typedef struct
{
    AppTaskData libTask;
    AppTask appTask;

    GmapClientCidList gmapClientCidList;                 /*! cid list */
    GmapClientProfileHandle gmapSrvcHndl;                /*! Profile handle of the GMAP Client instance */
    GmapClientProfileHandle bcastSrcHandle;              /*! Broadcast source handle */
    ServiceHandle gmasSrvcHndl;                          /*! Service handle of the GMAS client associated to this GMAP instance */
    GmapClientServices supportedGmasSrvcs;               /*! Different types of Gaming services supported by remote */
} GMAP;

typedef struct
{
    CsrCmnList_t profileHandleList;
} GmapClientMainInst;


CsrBool gmapClientFindGmapInst(CsrCmnListElm_t *elem, void *data);

CsrBool gmapClientInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data);

CsrBool gmapClientProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data);

CsrBool gmapClientProfileHndlFindByGmasSrvcHndl(CsrCmnListElm_t *elem, void *data);

CsrBool gmapClientElemFindByCid(CsrCmnListElm_t *elem, void *data);

CsrBool gmapClientFindGmapInstFromBcastSrcHandle(CsrCmnListElm_t *elem, void *data);

GmapClientMainInst *gmapClientGetMainInstance(void);

#define ADD_GMAP_CLIENT_INST(_List) \
                         ServiceHandleNewInstance((void**)(&(_List)),sizeof(GMAP))

#define FREE_GMAP_CLIENT_INST(_Handle) \
                           ServiceHandleFreeInstanceData(_Handle)

#define ADD_GMAP_CLIENT_SERVICE_HANDLE(_List) \
    (GmapClientProfileHandleListElm *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(GmapClientProfileHandleListElm))

#define REMOVE_GMAP_CLIENT_SERVICE_HANDLE(_List,_ServiceHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        gmapClientInstFindBySrvcHndl,(void *)(&(_ServiceHandle)))

#define FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(_Handle) \
                              (GMAP *)ServiceHandleGetInstanceData(_Handle)

#define FIND_GMAP_CLIENT_PROFILE_HANDLE_FROM_INST(_List,_id) \
                              ((GmapClientProfileHandleListElm *)CsrCmnListSearch(&(_List), \
                                        gmapClientFindGmapInst,(void *)(&(_id))))

#define FIND_GMAP_CLIENT_PROFILE_HANDLE_BY_BTCONNID(_List,_id) \
                              ((GmapClientProfileHandleListElm *)CsrCmnListSearch(&(_List), \
                                        gmapClientProfileHndlFindByBtConnId,(void *)(&(_id))))

#define FIND_GMAP_CLIENT_PROFILE_HANDLE_BY_GMAS_SERVICE_HANDLE(_List,_ServiceHandle) \
                              ((GmapClientProfileHandleListElm *)CsrCmnListSearch(&(_List), \
                                        gmapClientProfileHndlFindByGmasSrvcHndl,(void *)(&(_ServiceHandle))))

#define ADD_GMAP_CLIENT_CID_ELEM(_List) \
    (GmapClientCidListElm *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(GmapClientCidListElm))

#define REMOVE_GMAP_CLIENT_CID_ELEM(_List,_cid) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        gmapClientElemFindByCid,(void *)(&(_cid)))

#define FIND_GMAP_CLIENT_CID_ELEM(_List,_cid) \
                              ((GmapClientCidListElm *)CsrCmnListSearch(&(_List), \
                                        gmapClientElemFindByCid,(void *)(&(_cid))))

#define FIND_GMAP_CLIENT_PROFILE_ELEM_FROM_BCAST_SRC_HANDLE(_List,_bcastSrcHandle) \
                              ((GmapClientProfileHandleListElm *)CsrCmnListSearch(&(_List), \
                                        gmapClientFindGmapInstFromBcastSrcHandle,(void *)(&(_bcastSrcHandle))))
#endif /* GMAP_CLIENT_PRIVATE_H */
