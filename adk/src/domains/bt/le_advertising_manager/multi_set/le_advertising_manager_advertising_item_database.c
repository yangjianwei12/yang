/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_advertising_manager_multi_set
    \brief      Implementation for LE advertising manager functionality associated with advertising item database
*/

#include "le_advertising_manager.h"
#include "le_advertising_manager_advertising_item_database.h"

#include "logging.h"

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

#define MAX_NUMBER_OF_ADVERTISING_ITEM_ENTRIES 20

static struct _le_adv_item database[MAX_NUMBER_OF_ADVERTISING_ITEM_ENTRIES];


void LeAdvertisingManager_InitAdvertisingItemDatabase(void)
{
    struct _le_adv_item *item = NULL;

    ARRAY_FOREACH(item, database)
	{
        item->task = NULL;
        item->callback = NULL;
    }
}

le_adv_item_handle LeAdvertisingManager_AdvertisingItemDatabaseAddItem(Task task, const le_adv_item_callback_t * callback)
{
    DEBUG_LOG_FN_ENTRY("LEAM AdvertisingItemDatabaseAddItem with callback [0x%x]", callback);

	le_adv_item_handle handle = NULL;
    struct _le_adv_item *item = NULL;
	
    if(callback)
    {
        ARRAY_FOREACH(item, database)
		{
            if(item->callback == NULL)
			{
                item->task = task;
                item->callback = callback;
                handle = item;
				break;
			}
        }
	}
	
	return handle;
}

bool LeAdvertisingManager_AdvertisingItemDatabaseRemoveItem(le_adv_item_handle item)
{
    bool removed = FALSE;
    struct _le_adv_item *database_item = NULL;

    ARRAY_FOREACH(database_item, database)
    {
        if (database_item == item)
        {
            memset(database_item, 0, sizeof(*database_item));
            removed = TRUE;
            break;
        }
    }

    return removed;
}

uint32 LeAdvertisingManager_GetAdvertisingItemDatabaseMaxSize(void)
{
    return MAX_NUMBER_OF_ADVERTISING_ITEM_ENTRIES;
}

uint32 LeAdvertisingManager_GetAdvertisingItemDatabaseCurrentSize(void)
{
    uint32 count = 0;
    struct _le_adv_item *item = NULL;

    ARRAY_FOREACH(item, database)
    {
        if (item->callback != NULL)
        {
            count++;
        }
    }

    return count;
}

#endif
