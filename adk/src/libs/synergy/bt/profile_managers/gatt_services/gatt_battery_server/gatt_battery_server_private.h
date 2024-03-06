/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#ifndef GATT_BATTERY_SERVER_PRIVATE_H_
#define GATT_BATTERY_SERVER_PRIVATE_H_

#include "gatt_battery_server.h"
#include "gatt_battery_server_db.h"
#include "csr_bt_gatt_prim.h"
#include "csr_list.h"

/* Macros for creating messages */
#define MAKE_BATTERY_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T*)CsrPmemZalloc(sizeof(TYPE##_T))

#define BasMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, BAS_SERVER_PRIM, MSG);\
    }

typedef struct BasInstanceListElement
{
    struct BasInstanceListElement    *next;
    struct BasInstanceListElement    *prev;
    CsrBtGattId                      gattId;
    GBASS                            *pBass;
} BasInstanceListElm_t;

/* BAS can have more than one instance 
 * So storing all the instance in the BAS Instance list */
typedef struct __GBASS_T
{
    CsrCmnList_t instanceList;
} GBASS_T;
void initBasInstanceList(CsrCmnListElm_t *elem);
void handleBatteryServerRegisterHandleRangeCfm(GBASS *battery_server,
                        const CsrBtGattFlatDbRegisterHandleRangeCfm *cfm);

BasInstanceListElm_t *getBasServerInstanceByGattMsg(CsrCmnList_t *list, void *msg);

/* Function find the right BAS instance based on GATT ID for processing all the 
 * events received from GATT layer */
CsrBool basServerInstFindByGattId(CsrCmnListElm_t *elem, void *data);

#define BAS_SERVER_ADD_INSTANCE(_List) \
    (BasInstanceListElm_t *)CsrCmnListElementAddLast(&(_List), \
               sizeof(BasInstanceListElm_t))

#define BAS_SERVER_REMOVE_INSTANCE(_List,__gattId) \
     CsrCmnListIterateAllowRemove(&(_List), \
               basServerInstFindByGattId,(void *)(&(_gattId)))

#define BAS_SERVER_FIND_INSTANCE_BY_GATTID(_List,_gattId) \
     ((BasInstanceListElm_t *)CsrCmnListSearch(&(_List), \
               basServerInstFindByGattId,(void *)(&(_gattId))))


#endif
