/*******************************************************************************

Copyright (C) 2018-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef BAP_CLIENT_LIFO_LIST_H_
#define BAP_CLIENT_LIFO_LIST_H_

#include "csr_types.h"
#include "qbl_types.h"
#include "bap_client_list_element.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Generic singly linked 'lifo' list that is more memory
 * efficient than the fifo list (because it doesn't have a
 * tail pointer).
 * It does nothing other than implement a list. To use it
 * or do anything 'special' like count the number of items
 * or add thread safety then please derive from it.
 */
typedef struct BapClientLifoList
{
    BapClientListElement *head;
} BapClientLifoList;

/*
 * Public API (
 */
#define bapClientLifoListInitialise(bapClientLifoList)             ((bapClientLifoList)->head = NULL)
#define bapClientLifoListShallowCopy(bapClientLifoList, other)    ((bapClientLifoList)->head = (other)->head)

BapClientListElement* bapClientLifoListPop(BapClientLifoList * const bapClientLifoList);
bool bapClientLifoListPush(BapClientLifoList * const bapClientLifoList, BapClientListElement * const bapClientListElement);
bool bapClientLifoListRemove(BapClientLifoList * const bapClientLifoList, BapClientListElement * const bapClientListElement);
size_t bapClientLifoListGetSize(BapClientLifoList * const bapClientLifoList);

/*
 * A 'remove_if' implementation for the lifo_list. The rational for providing this is
 * similar to the rational for providing the C++ 'remove_if' template for std::list.
 * The implementation is based on the list 'best practice' examples on the Green Wiki.
 * The protection code (asserts etc.) only appear in code compiled with the DEBUG flag.
 *
 * A usage example can be found in the list unit tests.
 */
#define bapClientLifoListRemoveIf(bapClientLifoList,                                                    \
                                  listElementFieldName,                                          \
                                  containerType,                                                   \
                                  containerLocalVariableName,                                    \
                                  removeIfCondition,                                              \
                                  actionOnRemovedItems)                                          \
{                                                                                                 \
    BapClientLifoList *tmpLifoList = bapClientLifoList;                                                 \
    BapClientListElement* bapClientListElement;                                                           \
    while ((bapClientListElement = tmpLifoList->head) != NULL)                                      \
    {                                                                                             \
        containerType* containerLocalVariableName = CONTAINER_PTR(bapClientListElement,           \
                                                                  containerType,             \
                                                                  listElementFieldName);   \
        BAP_ASSERT(&containerLocalVariableName->listElementFieldName == bapClientListElement);      \
        (void)containerLocalVariableName;                                                      \
        if (removeIfCondition)                                                                  \
        {                                                                                         \
            /* Note: if this is the first element then we are setting:                            \
             * 'qbl_lifo_list->head = qbl_list_element->next'                                     \
             */                                                                                   \
            tmpLifoList->head = bapClientListElement->next;                                         \
            actionOnRemovedItems;                                                              \
        }                                                                                         \
        else                                                                                      \
        {                                                                                         \
            /* Create a tmp_lifo_list that consists of all the remaining elements                 \
             * in the list that we *haven't* yet looked at and make                               \
             * tmp_lifo_list->head *be* qbl_list_element->next,                                   \
             * i.e. &tmp_lifo_list->head = &qbl_list_element->next                                \
             * The following resolves to: tmp_lifo_list = (BapClientLifoList*)&qbl_list_element->next \
             */                                                                                   \
            tmpLifoList = CONTAINER_PTR(&bapClientListElement->next, BapClientLifoList, head);          \
            BAP_ASSERT(&tmpLifoList->head == &bapClientListElement->next);                              \
        }                                                                                         \
    }                                                                                             \
}


/*
 * The rational is similar to that of std::find_if
 */
#define bapClientLifoListFindIf(bapClientLifoList,                                       \
                                listElementFieldName,                             \
                                containerType,                                      \
                                containerLocalVariableName,                       \
                                findIfCondition)                                   \
{                                                                                  \
    BapClientListElement *listElement;                                                \
    containerLocalVariableName = NULL; /* return NULL if list is empty */       \
    for (listElement = bapClientLifoListPeekFront(bapClientLifoList);                   \
         listElement != NULL;                                                     \
         listElement = bapClientListElementGetNext(listElement))                   \
    {                                                                              \
        containerLocalVariableName = CONTAINER_PTR(listElement, containerType, listElementFieldName);\
                                                                                   \
        if (findIfCondition)                                                     \
        {                                                                          \
            break;                                                                 \
        }                                                                          \
        /* return NULL if 'find_if_condition' never returns true */                \
        containerLocalVariableName = NULL;                                      \
    }                                                                              \
}


/*
 * The rational is similar to that of std::foreach
 */
#define bapClientLifoListForeach(bapClientLifoList,                                       \
                                 listElementFieldName,                             \
                                 containerType,                                      \
                                 containerLocalVariableName,                       \
                                 action)                                              \
{                                                                                  \
    BapClientListElement *listElement = bapClientLifoListPeekFront(bapClientLifoList);      \
    while (listElement != NULL)                                                   \
    {                                                                              \
        containerType* containerLocalVariableName = CONTAINER_PTR(listElement, containerType, listElementFieldName);\
        BapClientListElement * nextElement = bapClientListElementGetNext(listElement); /* Do this before (potentially) modifying the contents (below) */ \
                                                                                   \
        action;                                                                    \
        listElement = nextElement;                                               \
        CSR_UNUSED(containerLocalVariableName);                                 \
    }                                                                              \
}

#define bapClientLifoListPeekFront(bapClientLifoList) ((bapClientLifoList)->head)
#define bapClientLifoListIsEmpty(bapClientLifoList)   ((bapClientLifoList)->head == NULL)

#define bapClientLifoListEnd (NULL)

#ifdef __cplusplus
}
#endif

#endif /* BAP_CLIENT_LIFO_LIST_H_ */
