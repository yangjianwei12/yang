/*!
    \copyright  Copyright (c) 2018 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_advertising_manager_multi_set
    \brief      Management of Bluetooth Low Energy advertising
*/

#include "le_advertising_manager.h"

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

#include "le_advertising_manager_advertising_item_database.h"
#include "le_advertising_manager_aggregator.h"
#include "le_advertising_manager_default_parameters.h"
#include "le_advertising_manager_set_sm.h"
#include "le_advertising_manager_multi_set_private.h"

#include <message.h>
#include <panic.h>
#include <logging.h>


static bool allowed =  FALSE;
static bool connectable =  FALSE;
static Task allow_task = NULL;
static Task enable_connectable_task = NULL;

bool LeAdvertisingManager_IsAdvertisingAllowed(void)
{
    return allowed;
}

bool LeAdvertisingManager_IsConnectableAdvertisingEnabled(void)
{
    return connectable;
}

bool LeAdvertisingManager_Init(Task init_task)
{
    DEBUG_LOG_VERBOSE("LEAM Init");

    UNUSED(init_task);

    allowed = FALSE;
    connectable = FALSE;
    allow_task = NULL;
    enable_connectable_task = NULL;

    LeAdvertisingManager_InitAdvertisingItemDatabase();
    LeAdvertisingManager_SetSmInit();
    LeAdvertisingManager_InitAggregator();
    LeAdvertisingManager_DefaultParametersInit();

    return TRUE;
}

static void leAdvertisingManager_EnableConnectableAdvertisingSendCfmMessage(bool enable)
{
    if(enable_connectable_task)
    {
        MESSAGE_MAKE(msg, LE_ADV_MGR_ENABLE_CONNECTABLE_CFM_T);
        msg->enable = enable;
        msg->status = le_adv_mgr_status_success;
        MessageSend(enable_connectable_task, LE_ADV_MGR_ENABLE_CONNECTABLE_CFM, msg);
    }
}

static void leAdvertisingManager_EnableConnectableAdvertisingCallback(void)
{
    DEBUG_LOG_VERBOSE("LEAM Enable Connectable Advertising Callback Existing State Enabled [%d]", connectable);

    leAdvertisingManager_EnableConnectableAdvertisingSendCfmMessage(connectable);
}

bool LeAdvertisingManager_EnableConnectableAdvertising(Task task, bool enable)
{
    DEBUG_LOG_VERBOSE("LEAM EnableConnectableAdvertising Task [0x%x] Enable [%d]", task, enable);

    enable_connectable_task = task;

    DEBUG_LOG_VERBOSE("LEAM Enable Connectable Advertising Existing State Enabled [%d]", connectable);

    if(connectable != enable)
    {
        connectable = enable;

        le_adv_refresh_control_t control = { .advertising_state_update_callback = leAdvertisingManager_EnableConnectableAdvertisingCallback };

        LeAdvertisingManager_QueueAdvertisingStateUpdate(&control);
    }
    else
    {
        leAdvertisingManager_EnableConnectableAdvertisingSendCfmMessage(enable);
    }

    return TRUE;
}

static void leAdvertisingManager_AllowAdvertisingSendCfmMessage(bool allow)
{
    if(allow_task)
    {
        MESSAGE_MAKE(msg, LE_ADV_MGR_ALLOW_ADVERTISING_CFM_T);
        msg->allow = allow;
        msg->status = le_adv_mgr_status_success;
        MessageSend(allow_task, LE_ADV_MGR_ALLOW_ADVERTISING_CFM, msg);
    }
}

static void leAdvertisingManager_AllowAdvertisingRefreshCallback(void)
{
    DEBUG_LOG_VERBOSE("LEAM Refresh Callback Existing State Allowed [%d]", allowed);

    leAdvertisingManager_AllowAdvertisingSendCfmMessage(allowed);
}

bool LeAdvertisingManager_AllowAdvertising(Task task, bool allow)
{
    DEBUG_LOG_VERBOSE("LEAM AllowAdvertising Task [0x%x] Allow [%d]", task, allow);

    allow_task = task;

    if (allowed != allow)
    {
        DEBUG_LOG_VERBOSE("LEAM AllowAdvertising Existing State Allowed [%d]", allowed);

        allowed = allow;

        le_adv_refresh_control_t control = { .advertising_state_update_callback = leAdvertisingManager_AllowAdvertisingRefreshCallback };
        LeAdvertisingManager_QueueAdvertisingStateUpdate(&control);
    }
    else
    {
        leAdvertisingManager_AllowAdvertisingSendCfmMessage(allow);
    }

    return TRUE;
}

le_adv_item_handle LeAdvertisingManager_RegisterAdvertisingItemCallback(Task task, const le_adv_item_callback_t * callback)
{
    le_adv_item_handle handle = LeAdvertisingManager_AdvertisingItemDatabaseAddItem(task, callback);

    if (handle)
    {
        /* Immediately schedule an internal update for this item so that it can
           be put into a group. */
        LeAdvertisingManager_QueueClientDataUpdate(handle);
    }

    return handle;
}

le_adv_item_handle LeAdvertisingManager_RegisterAdvertisingFlagsCallback(Task task, const le_adv_item_callback_t * callback)
{
    DEBUG_LOG_FN_ENTRY("LEAM RegisterAdvertisingFlagsCallback callback %p", callback);

    le_adv_item_handle handle = LeAdvertisingManager_AdvertisingItemDatabaseAddItem(task, callback);

    if (handle)
    {
        LeAdvertisingManager_AggregatorSetFlagsItem(handle);
        LeAdvertisingManager_QueueClientDataUpdate(handle);
    }

    return handle;
}

void LeAdvertisingManager_UnregisterAdvertisingItem(const le_adv_item_handle handle)
{
    DEBUG_LOG_FN_ENTRY("LEAM UnregisterAdvertisingItem handle [0x%x]", handle);

    if (handle)
    {
        LeAdvertisingManager_QueueClientDataRemove(handle);
    }
}

bool LeAdvertisingManager_UpdateAdvertisingItem(const le_adv_item_handle handle)
{
    DEBUG_LOG_FN_ENTRY("LEAM UpdateAdvertisingItem with Handle [0x%x]", handle);


    LeAdvertisingManager_QueueClientDataUpdate(handle);

    return TRUE;
}

#endif
