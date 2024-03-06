/*******************************************************************************

Copyright (C) 2018-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef BAP_CLIENT_LIST_H_
#define BAP_CLIENT_LIST_H_

#include "qbl_types.h"
#include "bap_client_lifo_list.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Generic singly linked 'fifo' list.
 * Much of the implementation is shared with a 'lifo' list
 * with the main (only?) difference being that list elements
 * are pushed onto the tail of the list
 * It does nothing other than implement a list. To use it
 * or do anything 'special' like count the number of items
 * or add thread safety then please derive from it.
 */
typedef struct
{
    BapClientLifoList       bapClientLifoList;
    BapClientListElement*   tail;
} BapClientList;

/*
 * Public API (
 */
void bapClientListInitialise(BapClientList * const bapClientList);
Bool bapClientListPush(BapClientList * const bapClientList, BapClientListElement * const bapClientListElement);
Bool bapClientListRemove(BapClientList * const bapClientList, BapClientListElement * const bapClientListElement);

#define bapClientListPop(bapClientList)                     (bapClientLifoListPop(&(bapClientList)->bapClientLifoList))
#define bapClientListPeekFront(bapClientList)               (bapClientLifoListPeekFront(&(bapClientList)->bapClientLifoList))
#define bapClientListIsEmpty(bapClientList)                 (bapClientLifoListIsEmpty(&(bapClientList)->bapClientLifoList))
#define bapClientListGetSize(bapClientList)                 (bapClientLifoListGetSize(&(bapClientList)->bapClientLifoList))

#define bapClientListFindIf(l, fieldName, type, localVariable, predicate)  \
    bapClientLifoListFindIf(&(l)->bapClientLifoList, fieldName, type, localVariable, predicate)

#define bapClientListRemoveIf(l, fieldName, type, localVariable, predicate, actionOnRemove) \
    bapClientLifoListRemoveIf(&(l)->bapClientLifoList, fieldName, type, localVariable, predicate, actionOnRemove)


#define bapClientListForeach(l, fieldName, type, localVariable, action) \
        bapClientLifoListForeach(&(l)->bapClientLifoList, fieldName, type, localVariable, action)

#define bapClientListEnd (NULL)

#ifdef __cplusplus
}
#endif

#endif /* BAP_CLIENT_LIST_H_ */
