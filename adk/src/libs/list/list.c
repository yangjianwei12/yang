/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      List API
*/

#include "list.h"
#include "list_interface.h"
#include "list_debug.h"

#include <stdlib.h>
#include <panic.h>
#include <logging.h>

typedef void (*AddElement)(list_t *list, const void *element, size_t size);

static const list_config_t default_config =
{
    .type = list_type_linked_single,
    .element_store =
    {
        .type = list_store_reference
    },
};

static void addElement(AddElement add, list_t *list, const void *element)
{
    if ((*list)->config->element_store.type == list_store_reference)
    {
        add(list, &element, sizeof(void*));
    }
    else if ((*list)->config->element_store.size > 0)
    {
        ASSERT_NOT_NULL(element);
        add(list, element, (*list)->config->element_store.size);
    }
    else
    {
        DEBUG_LOG_PANIC("%s: Unsupported list_store configuration enum:list_store_t:%d", __func__, (*list)->config->element_store.type);
    }
}

static void addElementWithSize(AddElement add, list_t *list, const void *element, size_t size)
{
    if ((*list)->config->element_store.type == list_store_variable_size_element)
    {
        ASSERT_NOT_NULL(element);
        add(list, element, size);
    }
    else if ((*list)->config->element_store.type == list_store_reference)
    {
        if (sizeof(void*) != size)
        {
            DEBUG_LOG_PANIC("%s: Element size %u does not match sizeof(void*)", __func__, size);
        }
        add(list, &element, size);
    }
    else if ((*list)->config->element_store.size > 0)
    {
        if ((*list)->config->element_store.size != size)
        {
            DEBUG_LOG_PANIC("%s: Element size %u does not match the fixed store size %u set", __func__, size, (*list)->config->element_store.size);
        }
        ASSERT_NOT_NULL(element);
        add(list, element, size);
    }
    else
    {
        DEBUG_LOG_PANIC("%s: Unsupported list_store configuration enum:list_store_t:%d", __func__, (*list)->config->element_store.type);
    }
}

static void updateBlobData(list_t list, data_blob_t blob, const void *new_data)
{
    if (list->config->element_store.type == list_store_reference)
    {
        memcpy(blob.data, &new_data, blob.data_length);
    }
    else
    {
        ASSERT_NOT_NULL(new_data);
        memcpy(blob.data, new_data, blob.data_length);
    }
}

const list_config_t* ListGetDefaultConfig(void)
{
    return &default_config;
}

list_t ListCreate(const list_config_t* config)
{
    if (!config)
    {
        config = ListGetDefaultConfig();
    }

    const list_interface_t* implementation_if = ListGetInterface(config);
    list_t list = implementation_if->ListCreate();
    ASSERT_NOT_NULL(list);
    list->implementation_if = implementation_if;
    list->config = config;

    return list;
}

void ListDestroy(list_t *list)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    (*list)->implementation_if->ListDestroy(list);
    *list = NULL;
}

void ListGotoHead(list_t *list)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    (*list)->implementation_if->ListGotoHead(list);
}

void ListGotoTail(list_t *list)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    (*list)->implementation_if->ListGotoTail(list);
}

void ListGotoEndOfList(list_t *list)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    (*list)->implementation_if->ListGotoEndOfList(list);
}

void ListGotoNext(list_t *list)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    (*list)->implementation_if->ListGotoNext(list);
}

void ListGotoPrevious(list_t *list)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    (*list)->implementation_if->ListGotoPrevious(list);
}

void ListAppend(list_t *list, const void *element)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    addElement((*list)->implementation_if->ListAppend, list, element);
}

void ListAppendWithSize(list_t *list, const void *element, size_t size)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    addElementWithSize((*list)->implementation_if->ListAppend, list, element, size);
}

void ListPrepend(list_t *list, const void *element)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    addElement((*list)->implementation_if->ListPrepend, list, element);
}

void ListPrependWithSize(list_t *list, const void *element, size_t size)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    addElementWithSize((*list)->implementation_if->ListPrepend, list, element, size);
}

void ListInsertAfterCurrent(list_t *list, const void *element)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    addElement((*list)->implementation_if->ListInsertAfterCurrent, list, element);
}

void ListInsertAfterCurrentWithSize(list_t *list, const void *element, size_t size)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    addElementWithSize((*list)->implementation_if->ListInsertAfterCurrent, list, element, size);
}

void ListInsertBeforeCurrent(list_t *list, const void *element)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    addElement((*list)->implementation_if->ListInsertBeforeCurrent, list, element);
}

void ListInsertBeforeCurrentWithSize(list_t *list, const void *element, size_t size)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    addElementWithSize((*list)->implementation_if->ListInsertBeforeCurrent, list, element, size);
}

void ListUpdateCurrentElement(list_t *list, const void *element)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    updateBlobData(*list, (*list)->implementation_if->ListGetCurrentElement(list), element);
}

void ListUpdateCurrentElementWithSize(list_t *list, const void *element, size_t size)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    data_blob_t blob = (*list)->implementation_if->ListGetCurrentElement(list);

    if (blob.data_length != size)
    {
        DEBUG_LOG_PANIC("%s: Elements size %u doesn't match expected size %u", __func__, blob.data_length, size);
    }

    updateBlobData(*list, blob, element);
}

void ListRemoveCurrentElement(list_t *list)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    (*list)->implementation_if->ListRemoveCurrentElement(list);
}

void ListGetCurrentElement(list_t *list, void *element)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    ASSERT_NOT_NULL(element);
    data_blob_t blob = (*list)->implementation_if->ListGetCurrentElement(list);
    memcpy(element, blob.data, blob.data_length);
}

void ListGetCurrentElementWithSize(list_t *list, void *element, size_t size)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    ASSERT_NOT_NULL(element);
    data_blob_t blob = (*list)->implementation_if->ListGetCurrentElement(list);
    if (blob.data_length != size)
    {
        DEBUG_LOG_PANIC("%s: Elements size %u doesn't match expected size %u", __func__, blob.data_length, size);
    }
    memcpy(element, blob.data, blob.data_length);
}

void* ListGetCurrentElementAddress(list_t *list)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    return (*list)->implementation_if->ListGetCurrentElement(list).data;
}

void* ListGetCurrentReference(list_t *list)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    data_blob_t blob = (*list)->implementation_if->ListGetCurrentElement(list);
    void *reference;

    if ((*list)->config->element_store.type != list_store_reference)
    {
        DEBUG_LOG_PANIC("%s: Unsupported list_store configuration enum:list_store_t:%d", __func__, (*list)->config->element_store.type);
    }

    memcpy(&reference, blob.data, blob.data_length);
    return reference;
}

size_t ListGetCurrentElementSize(list_t *list)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    data_blob_t blob = (*list)->implementation_if->ListGetCurrentElement(list);
    return blob.data_length;
}

size_t ListGetLength(list_t *list)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    return (*list)->implementation_if->ListGetLength(list);
}

bool ListIsEmpty(list_t *list)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    return ((*list)->implementation_if->ListGetLength(list) == 0);
}

bool ListIsAtEndOfList(list_t *list)
{
    ASSERT_VALID_HANDLE_REFERENCE(list);
    return (*list)->implementation_if->ListIsAtEndOfList(list);
}
