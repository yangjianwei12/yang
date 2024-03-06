/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Single Linked List API
*/

#include "list_interface.h"
#include "list_debug.h"

#include <stdlib.h>
#include <panic.h>
#include <logging.h>

/**< A linked list node containing a pointer to some 
value/structure and the prev/next node */
typedef struct list_linked_single_node_t
{
    struct list_linked_single_node_t* next;
    size_t element_size;
    unsigned char element[];
} list_linked_single_node_t;

typedef struct list_linked_single_t
{
    // Must be the first element in the struct, since the protected_data code can only access this part
    list_protected_data_t protected_data;
    // The rest of the elements are implementation specific
    list_linked_single_node_t* head;
    list_linked_single_node_t* tail;
    list_linked_single_node_t* curr_node;
    size_t length;
} list_linked_single_t;

static list_linked_single_node_t* createNode(const void *element, size_t size)
{
    list_linked_single_node_t* node = PanicNull(calloc(1, sizeof(list_linked_single_node_t) + size));
    node->next = NULL;
    node->element_size = size;
    memcpy(node->element, element, size);
    return node;
}

static list_linked_single_node_t* getPreviousNode(list_linked_single_t *list, list_linked_single_node_t *node)
{
    list_linked_single_node_t* current = list->head;

    // If it's the head node then there is no previous node
    if (current == node)
    {
        return NULL;
    }

    while(current != NULL)
    {
        if (current->next == node)
        {
            return current;
        }
        current = current->next;
    }

    DEBUG_LOG_PANIC("%s: Node is not in list", __func__);
    return NULL;
}

static void addAsHead(list_linked_single_t *list, list_linked_single_node_t *new_node)
{
    new_node->next = list->head;
    list->head = new_node;

    if (!list->tail)
    {
        list->tail = new_node;
    }

    list->length++;
}

static void addAsTail(list_linked_single_t *list, list_linked_single_node_t *new_node)
{
    if (list->tail)
    {
        list->tail->next = new_node;
    }
    else
    {
        list->head = new_node;
    }

    list->tail = new_node;

    list->length++;
}

static void insertAfterNode(list_linked_single_t *list, list_linked_single_node_t *node, list_linked_single_node_t *new_node)
{
    new_node->next = node->next;
    node->next = new_node;

    if (list->tail == node)
    {
        list->tail = new_node;
    }

    list->length++;
}

static list_t ListLinkedSingle_Create(void)
{
    list_linked_single_t* list = PanicNull(calloc(1, sizeof(list_linked_single_t)));
    return (list_t)list;
}

static void ListLinkedSingle_Destroy(list_t *_list)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;
    list_linked_single_node_t* current;
    for (current = list->head; current != NULL;)
    {
        list_linked_single_node_t* next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

static void ListLinkedSingle_GotoHead(list_t *_list)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;
    list->curr_node = list->head;
}

static void ListLinkedSingle_GotoTail(list_t *_list)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;
    list->curr_node = list->tail;
}

static void ListLinkedSingle_GotoEndOfList(list_t *_list)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;
    list->curr_node = NULL;
}

static void ListLinkedSingle_GotoNext(list_t *_list)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;
    if (!list->curr_node)
    {
        DEBUG_LOG_PANIC("%s: Current element cannot be end-of-list element", __func__);
    }
    list->curr_node = list->curr_node->next;
}

static void ListLinkedSingle_GotoPrevious(list_t *_list)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;
    if (!list->curr_node)
    {
        DEBUG_LOG_PANIC("%s: Current element cannot be end-of-list element", __func__);
    }
    list->curr_node = getPreviousNode(list, list->curr_node);
}

static void ListLinkedSingle_Append(list_t *_list, const void *element, size_t size)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;
    list_linked_single_node_t* new_node = createNode(element, size);
    addAsTail(list, new_node);
}

static void ListLinkedSingle_Prepend(list_t *_list, const void *element, size_t size)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;
    list_linked_single_node_t* new_node = createNode(element, size);
    addAsHead(list, new_node);
}

static void ListLinkedSingle_InsertAfterCurrent(list_t *_list, const void *element, size_t size)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;
    list_linked_single_node_t* curr_node = list->curr_node;
    list_linked_single_node_t* new_node = createNode(element, size);

    if (curr_node)
    {
        insertAfterNode(list, curr_node, new_node);
    }
    else
    {
        addAsTail(list, new_node);
    }
}

static void ListLinkedSingle_InsertBeforeCurrent(list_t *_list, const void *element, size_t size)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;
    list_linked_single_node_t* curr_node = list->curr_node;
    list_linked_single_node_t* new_node = createNode(element, size);

    if (curr_node)
    {
        // Prepending is effectively appending to the node before the current one
        curr_node = getPreviousNode(list, curr_node);
    }

    if (curr_node)
    {
        insertAfterNode(list, curr_node, new_node);
    }
    else
    {
        addAsHead(list, new_node);
    }
}

static void ListLinkedSingle_RemoveCurrentElement(list_t *_list)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;
    list_linked_single_node_t* node = list->curr_node;
    list_linked_single_node_t** previous;
    list_linked_single_node_t** current;

    if (!node)
    {
        DEBUG_LOG_PANIC("%s: Current element cannot be end-of-list element", __func__);
    }

    for (current = &list->head, previous = &list->head; *current != NULL; previous = current, current = &(*current)->next)
    {
        if (node == *current)
        {
            *current = (*current)->next;

            if (node == list->tail)
            {
                list->tail = *previous;
                list->curr_node = NULL;
            }
            else
            {
                list->curr_node = *current;
            }

            free(node);
            list->length--;
            break;
        }
    }
}

static data_blob_t ListLinkedSingle_GetCurrentElement(list_t *_list)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;

    if (!list->curr_node)
    {
        DEBUG_LOG_PANIC("%s: Current element cannot be end-of-list element", __func__);
    }

    data_blob_t blob = {list->curr_node->element_size, list->curr_node->element};
    return blob;
}

static size_t ListLinkedSingle_GetLength(list_t *_list)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;
    return list->length;
}

static bool ListLinkedSingle_IsAtEndOfList(list_t *_list)
{
    list_linked_single_t* list = (list_linked_single_t*)*_list;
    return (list->curr_node == NULL);
}

const list_interface_t list_linked_single_if = {
    .ListCreate = ListLinkedSingle_Create,
    .ListDestroy = ListLinkedSingle_Destroy,
    .ListGotoHead = ListLinkedSingle_GotoHead,
    .ListGotoTail = ListLinkedSingle_GotoTail,
    .ListGotoEndOfList = ListLinkedSingle_GotoEndOfList,
    .ListGotoNext = ListLinkedSingle_GotoNext,
    .ListGotoPrevious = ListLinkedSingle_GotoPrevious,
    .ListAppend = ListLinkedSingle_Append,
    .ListPrepend = ListLinkedSingle_Prepend,
    .ListInsertAfterCurrent = ListLinkedSingle_InsertAfterCurrent,
    .ListInsertBeforeCurrent = ListLinkedSingle_InsertBeforeCurrent,
    .ListRemoveCurrentElement = ListLinkedSingle_RemoveCurrentElement,
    .ListGetCurrentElement = ListLinkedSingle_GetCurrentElement,
    .ListGetLength = ListLinkedSingle_GetLength,
    .ListIsAtEndOfList = ListLinkedSingle_IsAtEndOfList,
};
