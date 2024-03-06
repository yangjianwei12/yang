/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Array List API
*/

#include "list_interface.h"
#include "list_debug.h"

#include <stdlib.h>
#include <panic.h>
#include <logging.h>

typedef struct list_array_t
{
    // Must be the first element in the struct, since the protected_data code can only access this part
    list_protected_data_t protected_data;
    // The rest of the elements are implementation specific
    size_t length;
    // 0 if at end-of-list, 1 if at first element etc
    size_t curr_index;
    uint8 elements_data[];
} list_array_t;

static size_t getFixedSize(list_array_t *list)
{
    if (list->protected_data.config->element_store.type == list_store_reference)
        return sizeof(void*);
    else
        return list->protected_data.config->element_store.size;
}

static size_t getByteIndex(size_t element_index, size_t element_size)
{
    PanicFalse(element_index > 0);
    return (element_index - 1) * element_size;
}

static size_t getDataSize(size_t num_of_elements, size_t element_size)
{
    return getByteIndex(num_of_elements + 1, element_size);
}

static size_t getListSize(size_t num_of_elements, size_t element_size)
{
    return sizeof(list_array_t) + getDataSize(num_of_elements, element_size);
}

static void removeElement(list_array_t* list, size_t element_index)
{
    size_t size = getFixedSize(list);

    for(size_t i = element_index; i < list->length; i++)
    {
        memcpy(&list->elements_data[getByteIndex(i, size)], &list->elements_data[getByteIndex(i + 1, size)], size);
    }
}

static void insertElement(list_array_t* list, size_t element_index, const void *element_data)
{
    size_t size = getFixedSize(list);

    for(size_t i = list->length; i > element_index ; i--)
    {
        memcpy(&list->elements_data[getByteIndex(i, size)], &list->elements_data[getByteIndex(i - 1, size)], size);
    }

    memcpy(&list->elements_data[getByteIndex(element_index, size)], element_data, size);
}

static list_t ListArray_Create(void)
{
    list_array_t* list = PanicNull(calloc(1, sizeof(list_array_t)));
    return (list_t)list;
}

static void ListArray_Destroy(list_t *_list)
{
    list_array_t* list = (list_array_t*)*_list;
    free(list);
}

static void ListArray_GotoHead(list_t *_list)
{
    list_array_t* list = (list_array_t*)*_list;
    if (list->length)
        list->curr_index = 1;
}

static void ListArray_GotoTail(list_t *_list)
{
    list_array_t* list = (list_array_t*)*_list;
    list->curr_index = list->length;
}

static void ListArray_GotoEndOfList(list_t *_list)
{
    list_array_t* list = (list_array_t*)*_list;
    list->curr_index = 0;
}

static void ListArray_GotoNext(list_t *_list)
{
    list_array_t* list = (list_array_t*)*_list;
    PanicFalse(list->curr_index != 0);

    if (list->curr_index == list->length)
        list->curr_index = 0;
    else
        list->curr_index++;
}

static void ListArray_GotoPrevious(list_t *_list)
{
    list_array_t* list = (list_array_t*)*_list;
    PanicFalse(list->curr_index != 0);
    list->curr_index--;
}

static void ListArray_Append(list_t *_list, const void *element, size_t size)
{
    list_array_t** list = (list_array_t**)_list;
    PanicFalse(size == getFixedSize(*list));
    (*list)->length++;
    *list = realloc(*list, getListSize((*list)->length, size));
    insertElement(*list, (*list)->length, element);
}

static void ListArray_Prepend(list_t *_list, const void *element, size_t size)
{
    list_array_t** list = (list_array_t**)_list;
    PanicFalse(size == getFixedSize(*list));
    (*list)->length++;
    *list = realloc(*list, getListSize((*list)->length, size));
    insertElement(*list, 1, element);
}

static void ListArray_InsertAfterCurrent(list_t *_list, const void *element, size_t size)
{
    list_array_t** list = (list_array_t**)_list;
    PanicFalse(size == getFixedSize(*list));
    (*list)->length++;
    *list = realloc(*list, getListSize((*list)->length, size));

    if ((*list)->curr_index)
    {
        insertElement(*list, (*list)->curr_index + 1, element);
    }
    else
    {
        insertElement(*list, (*list)->length, element);
    }
}

static void ListArray_InsertBeforeCurrent(list_t *_list, const void *element, size_t size)
{
    list_array_t** list = (list_array_t**)_list;
    PanicFalse(size == getFixedSize(*list));
    (*list)->length++;
    *list = realloc(*list, getListSize((*list)->length, size));

    if ((*list)->curr_index)
    {
        insertElement(*list, (*list)->curr_index, element);
        (*list)->curr_index++;
    }
    else
    {
        insertElement(*list, 1, element);
    }
}

static void ListArray_RemoveCurrentElement(list_t *_list)
{
    list_array_t** list = (list_array_t**)_list;
    PanicFalse((*list)->curr_index != 0);
    PanicFalse((*list)->length > 0);
    removeElement(*list, (*list)->curr_index);
    (*list)->length--;
    *list = realloc(*list, getListSize((*list)->length, getFixedSize(*list)));

    if ((*list)->length == 0)
    {
        (*list)->curr_index = 0;
    }
}

static data_blob_t ListArray_GetCurrentElement(list_t *_list)
{
    list_array_t* list = (list_array_t*)*_list;
    size_t size = getFixedSize(list);
    PanicFalse(list->curr_index != 0);
    data_blob_t blob = {.data = &list->elements_data[getByteIndex(list->curr_index, size)], .data_length = size};
    return blob;
}

static size_t ListArray_GetLength(list_t *_list)
{
    list_array_t* list = (list_array_t*)*_list;
    return list->length;
}

static bool ListArray_IsAtEndOfList(list_t *_list)
{
    list_array_t* list = (list_array_t*)*_list;
    return list->curr_index == 0;
}

const list_interface_t list_array_if = {
    .ListCreate = ListArray_Create,
    .ListDestroy = ListArray_Destroy,
    .ListGotoHead = ListArray_GotoHead,
    .ListGotoTail = ListArray_GotoTail,
    .ListGotoEndOfList = ListArray_GotoEndOfList,
    .ListGotoNext = ListArray_GotoNext,
    .ListGotoPrevious = ListArray_GotoPrevious,
    .ListAppend = ListArray_Append,
    .ListPrepend = ListArray_Prepend,
    .ListInsertAfterCurrent = ListArray_InsertAfterCurrent,
    .ListInsertBeforeCurrent = ListArray_InsertBeforeCurrent,
    .ListRemoveCurrentElement = ListArray_RemoveCurrentElement,
    .ListGetCurrentElement = ListArray_GetCurrentElement,
    .ListGetLength = ListArray_GetLength,
    .ListIsAtEndOfList = ListArray_IsAtEndOfList,
};
