/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_advertising_manager_multi_set
    \brief      Source file for LE advertising manager aggregator set.
*/

#include "le_advertising_manager_aggregator_set.h"

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

#include "le_advertising_manager.h"
#include "le_advertising_manager_advertising_item_database.h"

#include <logging.h>



uint32 leAdvertisingManager_GetItemSize(le_adv_item_handle item)
{
    PanicNull(item);
    le_adv_item_info_t item_info = { .data_size = 0 };
    item->callback->GetItemInfo(&item_info);
    return item_info.data_size;
}

void leAdvertisingManager_SetAdvertisingSetBusyLock(le_adv_set_t * set)
{
    DEBUG_LOG_FN_ENTRY("LEAM Set Busy Lock for Set [0x%x]", set);

    set->lock = 1;
}

void leAdvertisingManager_ReleaseAdvertisingSetBusyLock(le_adv_set_t * set)
{
    DEBUG_LOG_FN_ENTRY("LEAM Release Busy Lock for Set [0x%x]", set);

    set->lock = 0;
}

le_adv_set_t * leAdvertisingManager_GetSetWithFreeSpace(le_adv_set_list_t * set_list, uint32 free_space_size)
{
    le_adv_set_t * set_handle = NULL;
    for(le_adv_set_list_t * set = set_list; set != NULL; set = set->next)
    {
        if(set->set_handle->space >= free_space_size)
        {
            set_handle = set->set_handle;
            break;
        }
    }
    return set_handle;
}

void leAdvertisingManager_AddItemToSet(le_adv_set_t * set, le_adv_item_handle handle, uint32 size)
{
    DEBUG_LOG_VERBOSE("LEAM Add item %p of size %d to set %p", handle, size, set);
    PanicNull(handle);
    PanicNull(set);
    PanicFalse(size <= set->space);
    PanicFalse(!set->needs_destroying);

    le_adv_item_list_t * new_item = PanicUnlessMalloc(sizeof(le_adv_item_list_t));
    new_item->handle = handle;
    new_item->next = NULL;
    new_item->size = size;

    if(set->item_list == NULL)
    {
        set->item_list = new_item;
    }
    else
    {
        for(le_adv_item_list_t * item = set->item_list; item != NULL; item = item->next)
        {
            leAdvertisingManager_LoggingItemData(item->handle);
            if(item->next == NULL)
            {
                item->next = new_item;
                break;
            }
        }
    }
    set->space -= new_item->size;
    set->number_of_items++;
}

void leAdvertisingManager_PrependItemToSet(le_adv_set_t * set, le_adv_item_handle handle, uint32 size)
{
    DEBUG_LOG_VERBOSE("LEAM Prepend item %p of size %d to head of set %p", handle, size, set);
    PanicNull(handle);
    PanicNull(set);
    PanicFalse(size);
    PanicFalse(size <= set->space);
    PanicFalse(!set->needs_destroying);

    le_adv_item_list_t * new_item = PanicUnlessMalloc(sizeof(le_adv_item_list_t));
    new_item->handle = handle;
    new_item->size = size;

    new_item->next = set->item_list;
    set->item_list = new_item;

    set->space -= new_item->size;
    set->number_of_items++;
}

void LeAdvertisingManager_RemoveItemfromSet(le_adv_set_t *set, le_adv_item_handle handle)
{
    DEBUG_LOG_VERBOSE("LEAM Remove item %p from set %p", handle, set);
    PanicNull(set);
    PanicNull(handle);

    le_adv_item_list_t *item_list = set->item_list;
    le_adv_item_list_t *previous_item_list = set->item_list;

    while (item_list)
    {
        if (item_list->handle == handle)
        {
            break;
        }

        previous_item_list = item_list;
        item_list = item_list->next;
    }

    if (item_list)
    {
        if (set->item_list == item_list)
        {
            set->item_list = item_list->next;
        }
        else
        {
            previous_item_list->next = item_list->next;
        }

        free(item_list);
        set->number_of_items--;
    }
    else
    {
        DEBUG_LOG_WARN("LEAM Item %p not found in set %p", handle, set);
    }
}

void LeAdvertisingManager_DestroyAdvertisingSet(le_adv_set_t *set)
{
    DEBUG_LOG_FN_ENTRY("LEAM Destroy set %p set->sm %p", set, set->sm);

    PanicNull(set);
    PanicNotNull(set->item_list);

    if (set->sm)
    {
        LeAdvertisingManager_SetSmDestroy(set->sm);
    }
    free(set);
}

#endif /* INCLUDE_LEGACY_LE_ADVERTISING_MANAGER*/
