/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_advertising_manager_multi_set
    \brief      Implementation file for LE advertising manager aggregator, which is responsible for collecting individual advertising items and creating advertising sets
*/

#include "le_advertising_manager_aggregator.h"

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

#include "le_advertising_manager.h"
#include "le_advertising_manager_advertising_item_database.h"
#include "le_advertising_manager_set_sm.h"
#include "le_advertising_manager_multi_set_private.h"
#include "le_advertising_manager_aggregator_group.h"
#include "le_advertising_manager_aggregator_set.h"
#include "le_advertising_manager_aggregator_types.h"
#include "le_advertising_manager_data_packet.h"
#include "le_advertising_manager_default_parameters.h"

#include <panic.h>
#include <logging.h>
#include <app/bluestack/dm_prim.h>

LOGGING_PRESERVE_MESSAGE_TYPE(le_advertising_manager_internal_aggregator_msg_t)
ASSERT_INTERNAL_MESSAGES_NOT_OVERFLOWED(LE_ADVERTISING_MANAGER_INTERNAL_MESSAGE_END)

#define MAX_LEGACY_DATA_SET_SIZE_IN_OCTETS      31
#define MAX_EXTENDED_DATA_SET_SIZE_IN_OCTETS    255

/*! \brief Internal task data for the aggregator. */
typedef struct {
    /*! Task to send and process internal messages. */
    TaskData task_data;

    /*! Lock object to serialise internal messages. */
    uint16 lock;

    /*! Callback to use after completing refresh operation. */
    void (*refresh_callback)(void);

    /*! Special GAP Flags advertising item */
    le_adv_item_handle gap_flags_item;

} le_advertising_manager_aggregator_t;

static le_advertising_manager_aggregator_t aggregator_task_data;

#define LeAdvertisingManager_AggregatorGetTask() (&(aggregator_task_data).task_data)
#define LeAdvertisingManager_AggregatorGetTaskData() (&aggregator_task_data)
#define LeAdvertisingManager_AggregatorGetLock() (&aggregator_task_data.lock)

static void leAdvertisingManager_RefreshAdvertising(void);

#ifdef DEBUG_GROUPS_ENABLED
static void leAdvertisingManager_LoggingGroupsSetsItems(void)
{
    DEBUG_LOG_VERBOSE("LEAM print state:");
    DEBUG_LOG_VERBOSE("Groups: %d", LeAdvertisingManager_GetNumberOfGroupsInUse());

    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            continue;
        }

        LEAM_DEBUG_LOG("   Group[%p]:", group);
        LEAM_DEBUG_LOG("      num sets [%d] set_list [%p]", group->number_of_sets, group->set_list);

        uint32 set_number = 0;
        for(le_adv_set_list_t * set = group->set_list; set != NULL; set = set->next)
        {
            LEAM_DEBUG_LOG("         Set[%d]: [%p] active[%d] needs_params_update[%d] needs_data_update[%d] needs_enabling[%d] needs_disabling[%d] needs_destroying[%d] num items[%d] item_handles[%p]", set_number, set->set_handle,
                                                set->set_handle->active, set->set_handle->needs_params_update, set->set_handle->needs_data_update, set->set_handle->needs_enabling, set->set_handle->needs_disabling,  set->set_handle->needs_destroying, set->set_handle->number_of_items, set->set_handle->item_list);
            set_number++;
        }
        if (group->number_of_sets != set_number)
        {
            DEBUG_LOG_ERROR("LEAM ERROR number_of_sets [%u] != set_number [%u]",
                                    group->number_of_sets, set_number);
        }

        LEAM_DEBUG_LOG("      num items [%d] item_handles [%p]", group->number_of_items, group->item_handles);
        uint32 item_number = 0;
        for(le_adv_item_list_t * item_in_group = group->item_handles; item_in_group != NULL; item_in_group = item_in_group->next)
        {
            unsigned found_in_set = 0;
            for(le_adv_set_list_t * set = group->set_list; set != NULL; set = set->next)
            {
                for(le_adv_item_list_t * item_in_set = set->set_handle->item_list; item_in_set != NULL; item_in_set = item_in_set->next)
                {
                    if(item_in_set->handle == item_in_group->handle)
                    {
                        LEAM_DEBUG_LOG("         Item[%d]: [%p] size [%d] present in set [%p]", item_number, item_in_set->handle, item_in_set->size, set->set_handle );
                        found_in_set++;
                        break;
                    }
                }
            }

            if (group->scan_resp_set)
            {
                le_adv_set_t * set = group->scan_resp_set;

                for(le_adv_item_list_t * item_in_set = set->item_list; item_in_set != NULL; item_in_set = item_in_set->next)
                {
                    if(item_in_set->handle == item_in_group->handle)
                    {
                        LEAM_DEBUG_LOG("         Item[%d]: [%p] size [%d] present in scan resp set [%p]", item_number, item_in_set->handle, item_in_set->size, set);
                        found_in_set++;
                        break;
                    }
                }
            }

            if(found_in_set == 0)
            {
                LEAM_DEBUG_LOG("         Item [%d]: [%p] has no set", item_number, item_in_group->handle );

            }
            else if(found_in_set > 1)
            {
                DEBUG_LOG_ERROR("LEAM ERROR item [%d] [%p] present in more than one set",
                                        item_number, item_in_group->handle);
            }
            item_number++;
        }

        if(group->number_of_items != item_number)
        {
            DEBUG_LOG_ERROR("LEAM ERROR number_of_items [%u] != item_number [%u]",
                    group->number_of_items, item_number);
        }
    }
}
#else
#define leAdvertisingManager_LoggingGroupsSetsItems()
#endif

#ifdef DEBUG_GROUPS_ENABLED
void leAdvertisingManager_LoggingItemData(le_adv_item_handle item)
{
    LEAM_DEBUG_LOG("LEAM Log Item [0x%x]", item);

    le_adv_item_data_t data = {.data=NULL, .size=0};

    if(item && item->callback)
    {
        if(item->callback->GetItemData)
        {
            item->callback->GetItemData(&data);

            LEAM_DEBUG_LOG("LEAM Log Item Data [0x%x] Size [%d]", data.data, data.size);
        }

        if(item->callback->ReleaseItemData)
        {
            item->callback->ReleaseItemData();
        }
    }
    else
    {
        LEAM_DEBUG_LOG("LEAM Log Item Data invalid");
    }
}
#endif

static void leAdvertisingManager_GetItemParams(le_adv_item_handle handle, le_adv_item_params_t * item_params)
{
    LeAdvertisingManager_PopulateDefaultAdvertisingParams(item_params);
    if(handle && handle->callback && handle->callback->GetItemParameters)
    {
        PanicFalse(handle->callback->GetItemParameters(item_params));
    }
}

static void leAdvertisingManager_GetItemInfo(le_adv_item_handle handle, le_adv_item_info_t * item_info)
{
    if(handle && handle->callback && handle->callback->GetItemInfo)
    {
        PanicFalse(handle->callback->GetItemInfo(item_info));
    }
}

bool leAdvertisingManager_IsAdvertisingTypeExtended(le_adv_data_type_t type)
{
    return !(type & ADV_EVENT_BITS_USE_LEGACY_PDU);
}

bool leAdvertisingManager_IsAdvertisingTypeConnectable(le_adv_data_type_t type)
{
    return (type & ADV_EVENT_BITS_CONNECTABLE);
}

bool leAdvertisingManager_IsAdvertisingTypeLegacyDirected(le_adv_data_type_t type)
{
    return ((type & ADV_EVENT_BITS_DIRECTED) && (type & ADV_EVENT_BITS_USE_LEGACY_PDU));
}

bool leAdvertisingManager_IsDirectedAdvertisingToBeEnabledForGroup(le_adv_item_group_t * group)
{
    bool result =  FALSE;

    le_adv_item_handle item = group->item_handles->handle;

    if(item && item->callback && item->callback->GetItemData)
    {
        le_adv_item_data_t item_data;

        if(item->callback->GetItemData(&item_data))
        {
            result = TRUE;
        }
    }

    return result;
}

static unsigned leAdvertisingManager_GetDataPacketSizeForAdvertisingType(le_adv_data_type_t type)
{
    return (leAdvertisingManager_IsAdvertisingTypeExtended(type) ? MAX_EXTENDED_DATA_SET_SIZE_IN_OCTETS : MAX_LEGACY_DATA_SET_SIZE_IN_OCTETS);
}

static le_adv_set_t * leAdvertisingManager_GetAdvertisingSetForSm(le_advertising_manager_set_state_machine_t * sm)
{
    bool found = FALSE;
    le_adv_set_t * set = NULL;

    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            continue;
        }

        le_adv_set_list_t * head_set = group->set_list;

        while(head_set != NULL)
        {
            if(sm == head_set->set_handle->sm)
            {
                found = TRUE;
                set = head_set->set_handle;
                break;
            }

            head_set = head_set->next;

        }

        if(found)
        {
            break;
        }
    }

    return set;
}

static void leAdvertisingManager_UpdateParamsForAdvertisingSet(le_adv_set_t * set)
{
    LEAM_DEBUG_LOG("LEAM Update Params for Set [0x%x]", set);

    le_adv_item_group_t * group = PanicNull(leAdvertisingManager_GetGroupForSet(set));

    le_advertising_manager_set_params_t params =
    {
        .adv_event_properties = group->info.type,
        .primary_adv_interval_min = group->params.primary_adv_interval_min,
        .primary_adv_interval_max = group->params.primary_adv_interval_max,
        .primary_adv_channel_map = group->params.primary_adv_channel_map,
        .adv_filter_policy = group->params.adv_filter_policy,
        .primary_adv_phy = group->params.primary_adv_phy,
        .secondary_adv_max_skip = group->params.secondary_adv_max_skip,
        .secondary_adv_phy = group->params.secondary_adv_phy,
        .adv_sid = group->params.adv_sid,
        .own_addr_type = group->params.own_addr_type,
        .random_addr_type = group->params.random_addr_type,
        .random_addr = group->params.random_addr,
        .random_addr_generate_rotation_timeout_minimum_in_minutes = group->params.random_addr_generate_rotation_timeout_minimum_in_minutes,
        .random_addr_generate_rotation_timeout_maximum_in_minutes = group->params.random_addr_generate_rotation_timeout_maximum_in_minutes,
        .peer_tpaddr = group->params.peer_tpaddr,
    };

    LeAdvertisingManager_SetSmUpdateParams(set->sm, &params);
}

static void leAdvertisingManager_AggregatorRegisterCfm(le_advertising_manager_set_state_machine_t *sm, bool success)
{
    LEAM_DEBUG_LOG("LEAM AggregatorRegisterCfm adv_handle %u success %u",
                      LeAdvertisingManager_SetSmGetAdvHandle(sm), success);

    PanicNull(sm);
    PanicFalse(success);

    le_adv_set_t * set = PanicNull(leAdvertisingManager_GetAdvertisingSetForSm(sm));

    set->needs_registering = FALSE;

    if(set->needs_params_update)
    {
        leAdvertisingManager_UpdateParamsForAdvertisingSet(set);
    }
    else
    {
        leAdvertisingManager_ReleaseAdvertisingSetBusyLock(set);
    }
}

static void leAdvertisingManager_AggregatorUnregisterCfm(le_advertising_manager_set_state_machine_t *sm, bool success)
{
    LEAM_DEBUG_LOG("LEAM AggregatorUnregisterCfm adv_handle %u success %u",
                      LeAdvertisingManager_SetSmGetAdvHandle(sm), success);

    PanicNull(sm);
    PanicFalse(success);

    le_adv_set_t * set = PanicNull(leAdvertisingManager_GetAdvertisingSetForSm(sm));

    if (set->needs_destroying)
    {
        le_adv_item_group_t *group = PanicNull(leAdvertisingManager_GetGroupForSet(set));
        leAdvertisingManager_RemoveSetFromGroup(group, set);
        LeAdvertisingManager_DestroyAdvertisingSet(set);

        if (LeAdvertisingManager_GroupIsEmpty(group))
        {
            LeAdvertisingManager_DestroyGroup(group);
        }
    }
    else
    {
        leAdvertisingManager_ReleaseAdvertisingSetBusyLock(set);
    }
}

static le_advertising_manager_data_packet_t *leAdvertisingManager_CreateDataPacketForSet(le_adv_set_t *set)
{
    le_advertising_manager_data_packet_t* data_packet = NULL;
    le_adv_item_group_t * group = PanicNull(leAdvertisingManager_GetGroupForSet(set));
    le_adv_item_list_t * head = set->item_list;

    while(head != NULL)
    {
        leAdvertisingManager_LoggingItemData(head->handle);

        le_adv_item_data_t data;

        if (   head->handle->callback->GetItemData(&data)
            && (data.size != 0))
        {
            DEBUG_LOG_VERBOSE("LEAM Update Data for Item [0x%x] with Size [%d]", head->handle, data.size);

            if(!data_packet)
            {
                data_packet = LeAdvertisingManager_DataPacketCreateDataPacket(leAdvertisingManager_GetDataPacketSizeForAdvertisingType(group->info.type));
            }

            if(!LeAdvertisingManager_DataPacketAddDataItem(data_packet, &data))
            {
                if(data.size != head->size)
                {
                    // If client data size has changed since allocating the item to a set, assume the client
                    // update is currently queued so will be processed shortly 
                    DEBUG_LOG_VERBOSE("LEAM Item Data size %d does not match size in set %d", data.size, head->size);
                }
                else
                {
                    Panic();
                }
            }

            if(head->handle->callback->ReleaseItemData)
            {
                head->handle->callback->ReleaseItemData();
            }
        }
        else
        {
            DEBUG_LOG_VERBOSE("LEAM Update Data for Item [0x%x] with Size 0", head->handle);
        }

        head = head->next;
    }

    return data_packet;
}

/*! \brief Update the advert data in the controller for the given set.

    This function will update both the advert and scan response data for the
    given set.

    \note The scan response data for each set in a group is the same.

    \param set Advertising set to update the data for.
*/
static void leAdvertisingManager_UpdateDataForAdvertisingSet(le_adv_set_t * set)
{
    le_advertising_manager_data_packet_t* adv_packet = NULL;
    le_advertising_manager_data_packet_t* scan_resp_packet = NULL;
    le_adv_item_group_t * group = PanicNull(leAdvertisingManager_GetGroupForSet(set));

    PanicNull(set);

    LEAM_DEBUG_LOG("LEAM Update Set [0x%x] Space Left [%d]", set, set->space);

    adv_packet = leAdvertisingManager_CreateDataPacketForSet(set);

    if (group->scan_resp_set)
    {
        scan_resp_packet = leAdvertisingManager_CreateDataPacketForSet(group->scan_resp_set);
    }

    DEBUG_LOG_VERBOSE("LEAM Update Data with Adv Packet %p Scan Resp Packet %p", adv_packet, scan_resp_packet);

    le_advertising_manager_set_adv_data_t adv_data = {0};

    if(adv_packet)
    {
        adv_data.adv_data = *adv_packet;
    }

    if (scan_resp_packet)
    {
        adv_data.scan_resp_data = *scan_resp_packet;
    }

    LeAdvertisingManager_SetSmUpdateData(set->sm, &adv_data);

    /* The firmware has taken ownership of the buffer(s) in the data packets so
       reset the buffers in the packets before destroying the packets */
    if (adv_packet)
    {
        LeAdvertisingManager_DataPacketReset(adv_packet);
        LeAdvertisingManager_DataPacketDestroy(adv_packet);
        adv_packet = NULL;
    }

    if (scan_resp_packet)
    {
        LeAdvertisingManager_DataPacketReset(scan_resp_packet);
        LeAdvertisingManager_DataPacketDestroy(scan_resp_packet);
        scan_resp_packet = NULL;
    }

    /* The scan response set is destroyed here, after the data update, to
       make sure that the BT stack was updated with zero length scan
       response data before the set is destroyed. */
    if (group->scan_resp_set && group->scan_resp_set->needs_destroying)
    {
        le_adv_set_t *scan_resp_set = group->scan_resp_set;

        LEAM_DEBUG_LOG("  Destroying scan_resp_set [0x%x]", scan_resp_set);

        leAdvertisingManager_RemoveSetFromGroup(group, scan_resp_set);
        LeAdvertisingManager_DestroyAdvertisingSet(scan_resp_set);
    }
}

static void leAdvertisingManager_AggregatorProcessEnableDisableAdvertisingSetSm(le_advertising_manager_set_state_machine_t *sm)
{
    le_adv_set_t * set = PanicNull(leAdvertisingManager_GetAdvertisingSetForSm(sm));

    LEAM_DEBUG_LOG("LEAM AggregatorProcessEnableDisableAdvertisingSetSm adv_handle %u set_sm %p adv_set %p",
                      LeAdvertisingManager_SetSmGetAdvHandle(sm), sm, set);

    if((set->needs_enabling) && (set->needs_disabling))
    {
        Panic();
    }

    if(set->needs_enabling)
    {
        if(!set->active)
        {
            LeAdvertisingManager_SetSmEnable(sm);
        }
    }
    else if(set->needs_disabling)
    {
        if(set->active)
        {
            LeAdvertisingManager_SetSmDisable(sm);
        }
    }
    else
    {
        leAdvertisingManager_ReleaseAdvertisingSetBusyLock(set);
    }
}

static void leAdvertisingManager_AggregatorNotifyAdvertisingEventToGroupItems(le_adv_item_group_t * group, uint8 event_id, const void * event_data)
{
    LEAM_DEBUG_LOG("LEAM leAdvertisingManager_AggregatorNotifyAdvertisingEventToGroupItems for group [0x%x] event [%d]", group, event_id);

    if(group && group->item_handles)
    {
        LEAM_DEBUG_LOG("LEAM leAdvertisingManager_AggregatorNotifyAdvertisingEventToGroupItems group item list [%x]", group->item_handles);

        le_adv_item_list_t * head = group->item_handles;

        while(head)
        {
            LEAM_DEBUG_LOG("LEAM leAdvertisingManager_AggregatorNotifyAdvertisingEventToGroupItems item [%x]", head);

            le_adv_item_handle handle = head->handle;

            LEAM_DEBUG_LOG("LEAM leAdvertisingManager_AggregatorNotifyAdvertisingEventToGroupItems item handle [%x]", handle);

            if(handle && handle->callback && handle->callback->NotifyAdvertisingEvent)
            {
                handle->callback->NotifyAdvertisingEvent(event_id, event_data);
            }

            head = head->next;
        }
    }
}

static void leAdvertisingManager_AggregatorUpdateParamsCfm(le_advertising_manager_set_state_machine_t *sm, bool success)
{
    LEAM_DEBUG_LOG("LEAM AggregatorUpdateParamsCfm adv_handle %u success %u",
                      LeAdvertisingManager_SetSmGetAdvHandle(sm), success);

    PanicNull(sm);
    PanicFalse(success);

    le_adv_set_t * set = leAdvertisingManager_GetAdvertisingSetForSm(sm);

    PanicNull(set);

    set->needs_params_update = FALSE;

    if(set->needs_data_update)
    {
        leAdvertisingManager_UpdateDataForAdvertisingSet(set);
    }
    else
    {
        leAdvertisingManager_AggregatorProcessEnableDisableAdvertisingSetSm(sm);
    }
}

static void leAdvertisingManager_AggregatorUpdateDataCfm(le_advertising_manager_set_state_machine_t *sm, bool success)
{
    LEAM_DEBUG_LOG("LEAM AggregatorUpdateDataCfm adv_handle %u success %u",
                      LeAdvertisingManager_SetSmGetAdvHandle(sm), success);

    PanicNull(sm);
    PanicFalse(success);

    le_adv_set_t * set = leAdvertisingManager_GetAdvertisingSetForSm(sm);

    PanicNull(set);

    set->needs_data_update = FALSE;

    leAdvertisingManager_AggregatorProcessEnableDisableAdvertisingSetSm(sm);
}

static void leAdvertisingManager_AggregatorEnableCfm(le_advertising_manager_set_state_machine_t *sm, bool success)
{
    LEAM_DEBUG_LOG("LEAM AggregatorEnableCfm adv_handle %u success %u",
                      LeAdvertisingManager_SetSmGetAdvHandle(sm), success);

    PanicNull(sm);
    PanicFalse(success);

    le_adv_set_t * set = leAdvertisingManager_GetAdvertisingSetForSm(sm);

    le_adv_item_group_t * group = leAdvertisingManager_GetGroupForSet(set);
    leAdvertisingManager_AggregatorNotifyAdvertisingEventToGroupItems(group, LEAM_EVENT_ADVERTISING_SET_ENABLED,NULL);

    PanicNull(set);

    set->needs_enabling = FALSE;
    set->active = TRUE;
    leAdvertisingManager_ReleaseAdvertisingSetBusyLock(set);
}

static void leAdvertisingManager_AggregatorDisableCfm(le_advertising_manager_set_state_machine_t *sm, bool success)
{
    LEAM_DEBUG_LOG("LEAM AggregatorDisableCfm adv_handle %u success %u",
                      LeAdvertisingManager_SetSmGetAdvHandle(sm), success);

    PanicNull(sm);
    PanicFalse(success);

    le_adv_set_t * set = leAdvertisingManager_GetAdvertisingSetForSm(sm);

    PanicNull(set);

    set->needs_disabling = FALSE;
    set->active = FALSE;

    le_adv_item_group_t * group = leAdvertisingManager_GetGroupForSet(set);

    leAdvertisingManager_AggregatorNotifyAdvertisingEventToGroupItems(group, LEAM_EVENT_ADVERTISING_SET_SUSPENDED, NULL);

    if (set->needs_destroying)
    {
        LeAdvertisingManager_SetSmUnregister(sm);
    }
    else
    {
        leAdvertisingManager_ReleaseAdvertisingSetBusyLock(set);
    }
}

static void leAdvertisingManager_SetAggregatorRefreshLock(void)
{
    LEAM_DEBUG_LOG("LEAM Set Aggregator Refresh Lock");

    leAdvertisingManager_LoggingGroupsSetsItems();

    LeAdvertisingManager_AggregatorGetTaskData()->lock = 1;
}

static void leAdvertisingManager_AggregatorRandomAddressRotateTimeout(le_advertising_manager_set_state_machine_t *sm)
{
    LEAM_DEBUG_LOG("LEAM AggregatorRandomAddressRotateTimeout sm [0x%x]", sm);

    if(sm)
    {
        MESSAGE_MAKE(msg, LE_ADVERTISING_MANAGER_INTERNAL_ADVERTISING_SET_SUSPEND_T);
        msg->sm = sm;

        if(LeAdvertisingManager_AggregatorGetTaskData()->lock)
        {
            MessageSendConditionally(LeAdvertisingManager_AggregatorGetTask(), LE_ADVERTISING_MANAGER_INTERNAL_ADVERTISING_SET_SUSPEND, msg, &LeAdvertisingManager_AggregatorGetTaskData()->lock);
        }
        else
        {
            MessageSend(LeAdvertisingManager_AggregatorGetTask(), LE_ADVERTISING_MANAGER_INTERNAL_ADVERTISING_SET_SUSPEND, msg);

            leAdvertisingManager_SetAggregatorRefreshLock();
        }
    }
}

static void leAdvertisingManager_AggregatorRandomAddressChangedInd(le_advertising_manager_set_state_machine_t *sm, bdaddr new_bdaddr)
{
    LEAM_DEBUG_LOG("LEAM RandomAddressChangedInd sm [0x%x]", sm);

    if(sm)
    {
        le_adv_set_t * set = leAdvertisingManager_GetAdvertisingSetForSm(sm);

        PanicNull(set);

        le_adv_item_group_t * group = leAdvertisingManager_GetGroupForSet(set);

        LEAM_EVENT_ADVERTISING_SET_RANDOM_ADDRESS_CHANGED_T event_data = {.new_bdaddr = new_bdaddr};
        leAdvertisingManager_AggregatorNotifyAdvertisingEventToGroupItems(group, LEAM_EVENT_ADVERTISING_SET_RANDOM_ADDRESS_CHANGED, &event_data);
    }
}

static le_advertising_manager_set_client_interface_t aggregator_adv_set_client_interface = {
    .RegisterCfm = leAdvertisingManager_AggregatorRegisterCfm,
    .UpdateParamsCfm = leAdvertisingManager_AggregatorUpdateParamsCfm,
    .UpdateDataCfm = leAdvertisingManager_AggregatorUpdateDataCfm,
    .EnableCfm = leAdvertisingManager_AggregatorEnableCfm,
    .DisableCfm = leAdvertisingManager_AggregatorDisableCfm,
    .UnregisterCfm = leAdvertisingManager_AggregatorUnregisterCfm,
    .RandomAddressRotateTimeout = leAdvertisingManager_AggregatorRandomAddressRotateTimeout,
    .RandomAddressChangedInd = leAdvertisingManager_AggregatorRandomAddressChangedInd,
};

/* The GAP server flags should be added to any undirected, connectable, advert.
   They should be added to both legacy and extended advertising PDUs. */
static bool leAdvertisingManager_CheckIfGapFlagsShouldBeAddedToGroup(const le_adv_item_group_t *group)
{
    bool add_flags = (leAdvertisingManager_DataTypeIsConnectable(group->info.type) &&
                      !(leAdvertisingManager_IsAdvertisingTypeLegacyDirected(group->info.type)) &&
                      !group->info.dont_include_flags);

    DEBUG_LOG_VERBOSE("LEAM CheckIfGapFlagsShouldBeAddedToGroup add_flags %d",
                      add_flags);

    return add_flags;
}

static unsigned leAdvertisingManager_GetDataSetSizeForAdvertisingType(le_adv_data_type_t type)
{
    return (leAdvertisingManager_IsAdvertisingTypeExtended(type) ? MAX_EXTENDED_DATA_SET_SIZE_IN_OCTETS : MAX_LEGACY_DATA_SET_SIZE_IN_OCTETS);
}

/*! \brief Create a aggregator set to hold advert or scan response data items.

    \param type The type advert this data is for, e.g. legacy, extended, connectable.
    \param assign_adv_handle TRUE if this set represents data that should be
                             assigned to its own advertising set in the controller.
                             FALSE if this set is only used internally in the
                             aggregator to group related items together, for
                             example for scan response data.
*/
static le_adv_set_t * leAdvertisingManager_CreateNewAdvertisingSet(le_adv_data_type_t type, bool assign_adv_handle)
{
    le_adv_set_t * set = NULL;

    set = PanicUnlessMalloc((sizeof(le_adv_set_t)));
    memset(set, 0, sizeof(*set));

    LEAM_DEBUG_LOG("LEAM Create New Set type enum:le_adv_data_type_t:%u assign_adv_handle [%d]", type, assign_adv_handle);

    if (assign_adv_handle)
    {
        uint8 adv_handle = LeAdvertisingManager_SetSmGetNextUnusedAdvHandle();

        set->sm = LeAdvertisingManager_SetSmCreate(&aggregator_adv_set_client_interface, adv_handle);
        PanicNull(set->sm);
    }

    set->item_list = NULL;
    set->number_of_items = 0;
    set->needs_registering = TRUE;
    set->needs_params_update = TRUE;
    set->needs_data_update = TRUE;
    set->needs_enabling = TRUE;
    set->needs_disabling = FALSE;
    set->needs_destroying = FALSE;
    set->active = FALSE;
    set->lock = 0;
    set->space = leAdvertisingManager_GetDataSetSizeForAdvertisingType(type);

    LEAM_DEBUG_LOG("LEAM Set [%x] Created with Sm [%x]", set, set->sm );

    return set;
}

static le_adv_set_t * leAdvertisingManager_CreateNewAdvertisingSetForGroup(le_adv_item_group_t * group)
{
    le_adv_set_t * new_set = leAdvertisingManager_CreateNewAdvertisingSet(group->info.type, TRUE);
    leAdvertisingManager_AddSetToGroup(group, new_set);
    return new_set;
}

static inline void leAdvertisingManager_ClearSetOfItems(le_adv_set_t * set, uint32 set_size)
{
    le_adv_item_list_t * item = set->item_list;
    le_adv_item_list_t * next = NULL;
    while(item != NULL)
    {
        next = item->next;
        free(item);
        item = next;
    }
    set->item_list = NULL;
    set->number_of_items = 0;
    set->space = set_size;
}

static void leAdvertisingManager_AddGapFlagsToSet(le_adv_set_t *set)
{
    le_advertising_manager_aggregator_t *aggregator = LeAdvertisingManager_AggregatorGetTaskData();

    if (aggregator->gap_flags_item)
    {
        uint32 flags_size = leAdvertisingManager_GetItemSize(aggregator->gap_flags_item);

        if (flags_size)
        {
            DEBUG_LOG("LEAM adding gap flags item [%p] to set [%p]", aggregator->gap_flags_item, set);

            /* Add the GAP flags to beginning of the set items so they are the
               first item in the over the air advert data. */
            leAdvertisingManager_PrependItemToSet(set,
                                                  aggregator->gap_flags_item,
                                                  flags_size);
        }
    }
}

static void leAdvertisingManager_ClearAllSetsOfItems(le_adv_item_group_t * group)
{
    uint32 set_size = leAdvertisingManager_GetDataSetSizeForAdvertisingType(group->info.type);
    for(le_adv_set_list_t * set = group->set_list; set != NULL; set = set->next)
    {
        leAdvertisingManager_ClearSetOfItems(set->set_handle, set_size);
    }

    if (group->scan_resp_set)
    {
        leAdvertisingManager_ClearSetOfItems(group->scan_resp_set, set_size);
    }
}

static le_adv_item_group_t * leAdvertisingManager_GetMatchingScanResponseGroup(le_adv_item_group_t * group)
{
    le_adv_item_group_t * group_match = NULL;
    le_adv_item_group_t * this_group = group;

    FOR_EACH_AGGREGATOR_GROUP(group_to_check)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group_to_check))
        {
            continue;
        }

        if((le_adv_item_data_placement_scan_response == group_to_check->info.placement)
            && (this_group->info.type == group_to_check->info.type)
            &&(!this_group->info.override_connectable_state ))
        {
            group_match = group_to_check;
            break;
        }
    }

    return group_match;
}

static bool leAdvertisingManager_DoesScanResponseApplyToGroup(le_adv_item_group_t * group)
{
    return ((group->info.type & le_adv_type_extended_scannable)
            && (group->info.placement != le_adv_item_data_placement_scan_response)
            && !(group->info.needs_own_set));
}

static void leAdvertisingManager_RefreshSetsForGroup(le_adv_item_group_t * group)
{
    LEAM_DEBUG_LOG("LEAM refreshing group %p", group);

    leAdvertisingManager_LoggingGroupsSetsItems();

    /* In the current implementation by this point we've lost knowledge of which item the update request
       came from so remove all items from all sets and recreate */
    leAdvertisingManager_ClearAllSetsOfItems(group);

    /* Calculate the size of the space to reserve for the GAP flags if they
       need to be added to the set(s) in this group. */
    unsigned reserved_size = 0;
    if (leAdvertisingManager_CheckIfGapFlagsShouldBeAddedToGroup(group))
    {
        le_advertising_manager_aggregator_t *aggregator = LeAdvertisingManager_AggregatorGetTaskData();

        if (aggregator->gap_flags_item)
        {
            reserved_size = leAdvertisingManager_GetItemSize(aggregator->gap_flags_item);
        }
    }

    for(le_adv_item_list_t * item = group->item_handles; item != NULL; item = item->next)
    {
        le_adv_item_info_t item_info = {0};
        leAdvertisingManager_GetItemInfo(item->handle, &item_info);
        PanicFalse(item_info.data_size <= (leAdvertisingManager_GetDataSetSizeForAdvertisingType(group->info.type) - reserved_size));

        if((item_info.data_size) || (item_info.type == le_adv_type_legacy_directed))
        {
            DEBUG_LOG_VERBOSE("     item[%p] placement [enum:le_adv_item_data_placement_t:%u]", item->handle, item_info.placement);

            if (le_adv_item_data_placement_advert == item_info.placement)
            {
                le_adv_set_t * set = leAdvertisingManager_GetSetWithFreeSpace(group->set_list, (item_info.data_size + reserved_size));
                if(!set)
                {
                    set = leAdvertisingManager_CreateNewAdvertisingSetForGroup(group);
                }

                leAdvertisingManager_AddItemToSet(set, item->handle, item_info.data_size);

                set->needs_data_update = TRUE;
            }            
        }
    }

    /* If the GAP flags should be added to the set(s) in this group, add the
       GAP flags item to any set with one or more other items already in it. */
    if (leAdvertisingManager_CheckIfGapFlagsShouldBeAddedToGroup(group))
    {
        for (le_adv_set_list_t * set = group->set_list; set != NULL; set = set->next)
        {
            if (!LeAdvertisingManager_SetIsEmpty(set->set_handle))
            {
                leAdvertisingManager_AddGapFlagsToSet(set->set_handle);
            }
        }
    }

    le_adv_item_group_t * scan_response = leAdvertisingManager_GetMatchingScanResponseGroup(group);

    if(scan_response && leAdvertisingManager_DoesScanResponseApplyToGroup(group))
    {
        for(le_adv_item_list_t * scan_response_item = scan_response->item_handles; scan_response_item != NULL; scan_response_item = scan_response_item->next)
        {
            le_adv_item_info_t scan_reponse_item_info = {0};
            leAdvertisingManager_GetItemInfo(scan_response_item->handle, &scan_reponse_item_info);
            PanicFalse(scan_reponse_item_info.data_size <= (leAdvertisingManager_GetDataSetSizeForAdvertisingType(scan_response->info.type)));

            if(scan_reponse_item_info.data_size)
            {
                if (!group->scan_resp_set)
                {
                    group->scan_resp_set = leAdvertisingManager_CreateNewAdvertisingSet(group->info.type, FALSE);
                }

                leAdvertisingManager_AddItemToSet(group->scan_resp_set, scan_response_item->handle, scan_reponse_item_info.data_size);
            }
        }
    }

    leAdvertisingManager_LoggingGroupsSetsItems();
}

static void leAdvertisingManager_EnableAdvertisingForGroup(le_adv_item_group_t * group, bool enable)
{
    LEAM_DEBUG_LOG("LEAM EnableAdvertisingForGroup group %p enable %d", group, enable);

    for(le_adv_set_list_t * set = group->set_list; set != NULL; set = set->next)
    {
        if (enable && !LeAdvertisingManager_SetIsEmpty(set->set_handle))
        {
            if(!set->set_handle->active)
            {                
                /* If set's address is generated by LEAM, schedule parameter update to make sure
                 *  the address is updated while enabling the set as part of parameter update. */
                if(group->params.random_addr_type == ble_local_addr_generate_resolvable)
                {
                   set->set_handle->needs_params_update = TRUE;
                }

                set->set_handle->needs_enabling = TRUE;

            }
        }
        else
        {
            if(set->set_handle->active)
            {
                set->set_handle->needs_disabling = TRUE;
            }

            if(LeAdvertisingManager_SetIsEmpty(set->set_handle))
            {
                set->set_handle->needs_destroying = TRUE;
            }
        }

        LEAM_DEBUG_LOG("  set %p needs_enabling %d needs_disabling %d needs_destroying %d",
                       set,
                       set->set_handle->needs_enabling,
                       set->set_handle->needs_disabling,
                       set->set_handle->needs_destroying);
    }

    /* Process sets of the group with scan response data */
    if (group->scan_resp_set)
    {
        if (LeAdvertisingManager_SetIsEmpty(group->scan_resp_set))
        {
            LEAM_DEBUG_LOG("  scan_resp_set [0x%x] needs_destroying", group->scan_resp_set);
            group->scan_resp_set->needs_destroying = TRUE;
        }
    }

    /* Process the group with with scan response data */
    if(group->info.placement == le_adv_item_data_placement_scan_response)
    {
        if (LeAdvertisingManager_GroupIsEmpty(group))
        {
            LeAdvertisingManager_DestroyGroup(group);
        }
    }
}

static bool leAdvertisingManager_DoesGroupOnlyHaveFlagsToAdvertise(le_adv_item_group_t * group)
{
    bool result =  FALSE;

    if(group)
    {
        if((group->number_of_sets) && (group->number_of_sets == 1))
        {
            le_adv_set_list_t * list = group->set_list;

            if(list)
            {
                le_adv_set_t * set = list->set_handle;

                if((set->number_of_items == 1) &&
                   (set->item_list) &&
                   (set->item_list->handle == LeAdvertisingManager_AggregatorGetTaskData()->gap_flags_item))
                {
                    LEAM_DEBUG_LOG("LEAM Group [0x%x] has only 1 flags item to advertise", group);

                    result = TRUE;
                }
            }
        }
    }

    return result;

}

static void leAdvertisingManager_MarkAllSetsForDataUpdate(le_adv_item_group_t * group)
{
    for(le_adv_set_list_t * set = group->set_list; set != NULL; set = set->next)
    {
        if(set->set_handle)
        {
            set->set_handle->needs_data_update = TRUE;
        }
    }
}

static void LeAdvertisingManager_RegroupAdvertisingItems(void)
{
    LEAM_DEBUG_LOG("LEAM RegroupAdvertisingItems Number of Groups [%d]", LeAdvertisingManager_GetNumberOfGroupsInUse());

    leAdvertisingManager_LoggingGroupsSetsItems();

    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            continue;
        }

        bool enable_advertising = leAdvertisingManager_DoesGroupPassAdvertisingCriteria(group);

        if(enable_advertising && leAdvertisingManager_IsGroupToBeRefreshed(group))
        {
            leAdvertisingManager_RefreshSetsForGroup(group);

            leAdvertisingManager_MarkGroupToBeRefreshed(group, FALSE);
        }
        else if(!enable_advertising && leAdvertisingManager_IsGroupToBeRefreshed(group))
        {
            if(leAdvertisingManager_DoesGroupOnlyHaveFlagsToAdvertise(group))
            {
                /* At this stage, group has only flags to advertise, which means it needs to be destroyed.
                 * Therefore, all the sets in the group are cleared and marked for data update
                 * to make sure all the sets are destroyed gracefully before the group is destroyed */
                leAdvertisingManager_ClearAllSetsOfItems(group);

                leAdvertisingManager_MarkAllSetsForDataUpdate(group);

                leAdvertisingManager_MarkGroupToBeRefreshed(group, FALSE);

                enable_advertising = FALSE;
            }
        }

        leAdvertisingManager_EnableAdvertisingForGroup(group, enable_advertising);
    }

    leAdvertisingManager_LoggingGroupsSetsItems();
}

static bool leAdvertisingManager_IsItemScanResponseItem(le_adv_item_handle handle)
{
    bool result = FALSE;

    if(handle)
    {
        le_adv_item_group_t * group = leAdvertisingManager_GetGroupLinkedToItem(handle);
        if(group)
        {
            result = (group->info.placement == le_adv_item_data_placement_scan_response);
        }
    }

    return result;
}

static void leAdvertisingManager_MarkAllGroupsWithScannableEventsToBeRefreshed(void)
{
    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            continue;
        }

        if(group->info.type & le_adv_type_extended_scannable)
        {
            leAdvertisingManager_MarkGroupToBeRefreshed(group, TRUE);
        }
    }
}

static void leAdvertisingManager_UpdateAdvertisingItem(le_adv_item_handle handle)
{
    le_adv_item_params_t item_params;
    le_adv_item_info_t item_info;
    leAdvertisingManager_GetItemParams(handle, &item_params);
    leAdvertisingManager_GetItemInfo(handle, &item_info);

    DEBUG_LOG("LEAM UpdateAdvertisingItem handle %p adv_filter_policy %u", handle, item_params.adv_filter_policy);

    le_adv_item_group_t * new_group = leAdvertisingManager_GetGroupForParams(&item_info, &item_params);
    le_adv_item_group_t * current_group = leAdvertisingManager_GetGroupLinkedToItem(handle);
    LEAM_DEBUG_LOG("LEAM UpdateAdvertisingItem current %p new %p", current_group, new_group);
    if(current_group != new_group || !new_group)
    {
        if(current_group)
        {
            leAdvertisingManager_RemoveItemFromGroup(current_group, handle);
            leAdvertisingManager_MarkGroupToBeRefreshed(current_group, TRUE);
        }
        if(!new_group)
        {
            LEAM_DEBUG_LOG("LEAM No Groups with Matching Params, Create New One");
            new_group = PanicNull(leAdvertisingManager_CreateNewGroup(&item_info, &item_params));
        }
        leAdvertisingManager_AddItemToGroup(new_group, handle);
    }
    leAdvertisingManager_MarkGroupToBeRefreshed(new_group, TRUE);

    if(leAdvertisingManager_IsItemScanResponseItem(handle))
    {
        leAdvertisingManager_MarkAllGroupsWithScannableEventsToBeRefreshed();
    }
}

static void leAdvertisingManager_RemoveAdvertisingItem(le_adv_item_handle handle)
{
    le_adv_item_group_t *group = leAdvertisingManager_GetGroupLinkedToItem(handle);

    if (group)
    {
        leAdvertisingManager_RemoveItemFromGroup(group, handle);
        leAdvertisingManager_MarkGroupToBeRefreshed(group, TRUE);
    }
}

static void leAdvertisingManager_UpdateAdvertisingFlagsItem(void)
{
    /* Mark any groups that should include the GAP server flags for refresh. */
    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            continue;
        }

        if (leAdvertisingManager_CheckIfGapFlagsShouldBeAddedToGroup(group))
        {
            leAdvertisingManager_MarkGroupToBeRefreshed(group, TRUE);
        }
    }
}

static void leAdvertisingManager_ReleaseAggregatorRefreshLock(void)
{
    LEAM_DEBUG_LOG("LEAM Release Aggregator Refresh Lock");

    leAdvertisingManager_LoggingGroupsSetsItems();

    LeAdvertisingManager_AggregatorGetTaskData()->lock = 0;
}

void LeAdvertisingManager_QueueAdvertisingStateUpdate(le_adv_refresh_control_t * control)
{
    LEAM_DEBUG_LOG("LEAM Refresh Advertising Sets, Current Lock[%d]", LeAdvertisingManager_AggregatorGetTaskData()->lock );
    le_adv_refresh_control_t ctrl = { .advertising_state_update_callback = NULL };
    if(control)
    {
        ctrl.advertising_state_update_callback = control->advertising_state_update_callback;
    }
    MESSAGE_MAKE(msg, LE_ADVERTISING_MANAGER_ADVERTISING_STATE_UPDATE_T);
    msg->control = ctrl;
    
    if(LeAdvertisingManager_AggregatorGetTaskData()->lock)
    {
        MessageSendConditionally(LeAdvertisingManager_AggregatorGetTask(), LE_ADVERTISING_MANAGER_ADVERTISING_STATE_UPDATE, msg, &LeAdvertisingManager_AggregatorGetTaskData()->lock);
    }
    else
    {
        MessageSend(LeAdvertisingManager_AggregatorGetTask(), LE_ADVERTISING_MANAGER_ADVERTISING_STATE_UPDATE, msg);

        leAdvertisingManager_SetAggregatorRefreshLock();
    }
}

void LeAdvertisingManager_QueueClientDataUpdate(le_adv_item_handle item_handle)
{
    LEAM_DEBUG_LOG("LEAM QueueClientDataUpdate, handle %p Current Lock[%d]", item_handle, LeAdvertisingManager_AggregatorGetTaskData()->lock );
    MESSAGE_MAKE(msg, LE_ADVERTISING_MANAGER_CLIENT_DATA_UPDATE_T);
    msg->item_handle = item_handle;
    
    if(LeAdvertisingManager_AggregatorGetTaskData()->lock)
    {
        MessageSendConditionally(LeAdvertisingManager_AggregatorGetTask(), LE_ADVERTISING_MANAGER_CLIENT_DATA_UPDATE, msg, &LeAdvertisingManager_AggregatorGetTaskData()->lock);
    }
    else
    {
        MessageSend(LeAdvertisingManager_AggregatorGetTask(), LE_ADVERTISING_MANAGER_CLIENT_DATA_UPDATE, msg);

        leAdvertisingManager_SetAggregatorRefreshLock();
    }    
}

void LeAdvertisingManager_QueueDefaultParametersFallbackTimeout(le_adv_preset_advertising_interval_t interval, uint32 delay)
{
    LEAM_DEBUG_LOG("LEAM QueueDefaultParametersFallbackTimeout enum:le_adv_preset_advertising_interval_t:%u delay %u",
                   interval, delay);

    MESSAGE_MAKE(msg, LE_ADVERTISING_MANAGER_INTERNAL_DEFAULT_PARAMETERS_FALLBACK_TIMEOUT_T);
    msg->interval = interval;
    MessageSendLater(LeAdvertisingManager_AggregatorGetTask(),
                     LE_ADVERTISING_MANAGER_INTERNAL_DEFAULT_PARAMETERS_FALLBACK_TIMEOUT,
                     msg, D_SEC(delay));
}

void LeAdvertisingManager_QueueClientDataRemove(le_adv_item_handle item_handle)
{
    LEAM_DEBUG_LOG("LEAM QueueClientDataRemove, handle %p Current Lock[%d]", item_handle, LeAdvertisingManager_AggregatorGetTaskData()->lock );
    MESSAGE_MAKE(msg, LE_ADVERTISING_MANAGER_INTERNAL_CLIENT_DATA_REMOVE_T);
    msg->item_handle = item_handle;
    
    if(LeAdvertisingManager_AggregatorGetTaskData()->lock)
    {
        MessageSendConditionally(LeAdvertisingManager_AggregatorGetTask(), LE_ADVERTISING_MANAGER_INTERNAL_CLIENT_DATA_REMOVE, msg, &LeAdvertisingManager_AggregatorGetTaskData()->lock);
    }
    else
    {
        MessageSend(LeAdvertisingManager_AggregatorGetTask(), LE_ADVERTISING_MANAGER_INTERNAL_CLIENT_DATA_REMOVE, msg);

        leAdvertisingManager_SetAggregatorRefreshLock();
    }    
}

void LeAdvertisingManager_AggregatorSetFlagsItem(le_adv_item_handle item_handle)
{
    le_advertising_manager_aggregator_t *aggregator = LeAdvertisingManager_AggregatorGetTaskData();

    DEBUG_LOG_ALWAYS("LEAM AggregatorSetFlagsItem with Handle [0x%x]", item_handle);

    aggregator->gap_flags_item = item_handle;
}

static void leAdvertisingManager_SendInternalMessageRefreshLockCheck(void)
{
    MessageCancelFirst(LeAdvertisingManager_AggregatorGetTask(), LE_ADVERTISING_MANAGER_INTERNAL_AGGREGATOR_MSG_CHECK_REFRESH_LOCK);
    MessageSendLater(LeAdvertisingManager_AggregatorGetTask(), LE_ADVERTISING_MANAGER_INTERNAL_AGGREGATOR_MSG_CHECK_REFRESH_LOCK, NULL, 50);
}

static bool IsAnySetBusy(void)
{
    LEAM_DEBUG_LOG("LEAM Is Any Set Busy");

    bool found = FALSE;
    bool result = FALSE;

    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            continue;
        }

        le_adv_set_list_t * head_set = group->set_list;

        while(head_set != NULL)
        {
            DEBUG_LOG_VERBOSE("LEAM Is Set [0x%x] Busy[%d]", head_set->set_handle, head_set->set_handle->lock );

            if(head_set->set_handle->lock)
            {
                DEBUG_LOG_VERBOSE("LEAM Found Busy Set [0x%x]", head_set->set_handle);

                found = TRUE;
                break;
            }

            head_set = head_set->next;
        }

        if(found)
        {
            result = TRUE;
            break;
        }
    }

    return result;
}

static void leAdvertisingManager_CheckReleaseRefreshLock(void)
{
    if(!IsAnySetBusy())
    {
        leAdvertisingManager_ReleaseAggregatorRefreshLock();
        if(LeAdvertisingManager_AggregatorGetTaskData()->refresh_callback)
        {
            LEAM_DEBUG_LOG("LEAM Release Refresh Client Callback [0x%x]", LeAdvertisingManager_AggregatorGetTaskData()->refresh_callback );

            LeAdvertisingManager_AggregatorGetTaskData()->refresh_callback();
            LeAdvertisingManager_AggregatorGetTaskData()->refresh_callback = NULL;
        }

    }
    else
    {
        leAdvertisingManager_SetAggregatorRefreshLock();
        leAdvertisingManager_SendInternalMessageRefreshLockCheck();
    }
}

static void leAdvertisingManager_RefreshAdvertising(void)
{
    LEAM_DEBUG_LOG("LEAM Handle Internal Refresh Message");
    LeAdvertisingManager_RegroupAdvertisingItems();

    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            continue;
        }

        if (LeAdvertisingManager_GroupIsEmpty(group))
        {
            LEAM_DEBUG_LOG("  group [%p] is empty - destroy it", group);

            LeAdvertisingManager_DestroyGroup(group);
            continue;
        }

        le_adv_set_list_t * head_set = group->set_list;

        while(head_set != NULL)
        {
            LEAM_DEBUG_LOG("LEAM Refresh Group[0x%x] Set[0x%x]", group, head_set->set_handle);

            if(head_set->set_handle->needs_registering)
            {
                LEAM_DEBUG_LOG("LEAM Set Needs Registering [%x]", head_set->set_handle);

                leAdvertisingManager_SetAdvertisingSetBusyLock(head_set->set_handle);
                LeAdvertisingManager_SetSmRegister(head_set->set_handle->sm);
            }
            else if(head_set->set_handle->needs_params_update)
            {
                LEAM_DEBUG_LOG("LEAM Set Needs Params Update [%x]", head_set->set_handle);

                leAdvertisingManager_SetAdvertisingSetBusyLock(head_set->set_handle);
                leAdvertisingManager_UpdateParamsForAdvertisingSet(head_set->set_handle);
            }
            else if(head_set->set_handle->needs_data_update)
            {
                LEAM_DEBUG_LOG("LEAM Set Needs Data Update [%x]", head_set->set_handle);

                leAdvertisingManager_SetAdvertisingSetBusyLock(head_set->set_handle);
                leAdvertisingManager_UpdateDataForAdvertisingSet(head_set->set_handle);
            }
            else if(head_set->set_handle->needs_disabling)
            {
                LEAM_DEBUG_LOG("LEAM Set Needs Disabling [%x]", head_set->set_handle);

                if(head_set->set_handle->active)
                {
                    leAdvertisingManager_SetAdvertisingSetBusyLock(head_set->set_handle);
                    LeAdvertisingManager_SetSmDisable(head_set->set_handle->sm);
                }
                else
                {
                    head_set->set_handle->needs_disabling = FALSE;
                }
            }
            else if(head_set->set_handle->needs_enabling)
            {
                LEAM_DEBUG_LOG("LEAM Set Needs Enabling [%x]", head_set->set_handle);

                if(!head_set->set_handle->active)
                {
                    leAdvertisingManager_SetAdvertisingSetBusyLock(head_set->set_handle);
                    LeAdvertisingManager_SetSmEnable(head_set->set_handle->sm);
                }
                else
                {
                    head_set->set_handle->needs_enabling = FALSE;
                }
            }
            else if (head_set->set_handle->needs_destroying)
            {
                LEAM_DEBUG_LOG("LEAM Set Needs Destroying [%x]", head_set->set_handle);

                if (head_set->set_handle->active)
                {
                    leAdvertisingManager_SetAdvertisingSetBusyLock(head_set->set_handle);
                    LeAdvertisingManager_SetSmDisable(head_set->set_handle->sm);
                }
                else
                {
                    leAdvertisingManager_SetAdvertisingSetBusyLock(head_set->set_handle);
                    LeAdvertisingManager_SetSmUnregister(head_set->set_handle->sm);
                }
            }

            head_set = head_set->next;
        }
    }

    leAdvertisingManager_CheckReleaseRefreshLock();

}

static void leAdvertisingManager_AggregatorHandleAdvertisingStateUpdate(const LE_ADVERTISING_MANAGER_ADVERTISING_STATE_UPDATE_T * message)
{
    LeAdvertisingManager_AggregatorGetTaskData()->refresh_callback = message->control.advertising_state_update_callback;
    leAdvertisingManager_RefreshAdvertising();
}

static void leAdvertisingManager_AggregatorHandleInternalMsgCheckRefreshLock(void)
{
    leAdvertisingManager_CheckReleaseRefreshLock();
}

static void leAdvertisingManager_AggregatorHandleClientDataUpdate(const LE_ADVERTISING_MANAGER_CLIENT_DATA_UPDATE_T * message)
{
    le_advertising_manager_aggregator_t *aggregator = LeAdvertisingManager_AggregatorGetTaskData();

    if (aggregator->gap_flags_item == message->item_handle)
    {
        leAdvertisingManager_UpdateAdvertisingFlagsItem();
    }
    else
    {
        leAdvertisingManager_UpdateAdvertisingItem(message->item_handle);
    }
    leAdvertisingManager_RefreshAdvertising();
}

static void leAdvertisingManager_AggregatorHandleClientDataRemove(const LE_ADVERTISING_MANAGER_INTERNAL_CLIENT_DATA_REMOVE_T *message)
{
    le_advertising_manager_aggregator_t *aggregator = LeAdvertisingManager_AggregatorGetTaskData();

    LEAM_DEBUG_LOG("LEAM AggregatorHandleClientDataRemove item_handle %p", message->item_handle);

    if (aggregator->gap_flags_item == message->item_handle)
    {
        LeAdvertisingManager_AggregatorSetFlagsItem(NULL);
        leAdvertisingManager_UpdateAdvertisingFlagsItem();
    }
    else
    {
        if(leAdvertisingManager_IsItemScanResponseItem(message->item_handle))
        {
            leAdvertisingManager_MarkAllGroupsWithScannableEventsToBeRefreshed();
        }

        leAdvertisingManager_RemoveAdvertisingItem(message->item_handle);
    }

    leAdvertisingManager_RefreshAdvertising();

    LeAdvertisingManager_AdvertisingItemDatabaseRemoveItem(message->item_handle);
}

static le_adv_item_group_t * LeAdvertisingManager_AggregatorGetGroupForAdvertisingSet(uint8 adv_handle)
{
    le_adv_item_group_t * result = NULL;

    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            continue;
        }

        le_adv_set_list_t * curr = group->set_list;

        while(curr)
        {
            le_adv_set_t * set_handle = curr->set_handle;

            if(adv_handle == LeAdvertisingManager_SetSmGetAdvHandle(set_handle->sm))
            {
                result = group;
                break;
            }

            curr = curr->next;
        }
    }

    return result;

}

static void leAdvertisingManager_AggregatorHandleAdvertisinSetSuspend(const LE_ADVERTISING_MANAGER_INTERNAL_ADVERTISING_SET_SUSPEND_T * msg)
{
    le_advertising_manager_set_state_machine_t * sm = msg->sm;

    le_adv_set_t * set = leAdvertisingManager_GetAdvertisingSetForSm(sm);

    if(set)
    {
        set->needs_disabling = TRUE;
        leAdvertisingManager_RefreshAdvertising();
    }
}

static void LeAdvertisingManager_AggregatorMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    LEAM_DEBUG_LOG("LEAM AggregatorMessageHandler id enum:le_advertising_manager_internal_aggregator_msg_t:%u", id);

    switch (id)
    {
    case LE_ADVERTISING_MANAGER_ADVERTISING_STATE_UPDATE:
        leAdvertisingManager_AggregatorHandleAdvertisingStateUpdate((const LE_ADVERTISING_MANAGER_ADVERTISING_STATE_UPDATE_T *)message);
        break;

    case LE_ADVERTISING_MANAGER_INTERNAL_AGGREGATOR_MSG_CHECK_REFRESH_LOCK:
        leAdvertisingManager_AggregatorHandleInternalMsgCheckRefreshLock();
        break;

    case LE_ADVERTISING_MANAGER_CLIENT_DATA_UPDATE:
        leAdvertisingManager_AggregatorHandleClientDataUpdate((const LE_ADVERTISING_MANAGER_CLIENT_DATA_UPDATE_T *)message);
        break;

    case LE_ADVERTISING_MANAGER_INTERNAL_DEFAULT_PARAMETERS_FALLBACK_TIMEOUT:
        LeAdvertisingManager_HandleInternalDefaultParametersFallbackTimeout((const LE_ADVERTISING_MANAGER_INTERNAL_DEFAULT_PARAMETERS_FALLBACK_TIMEOUT_T *)message);
        break;

    case LE_ADVERTISING_MANAGER_INTERNAL_CLIENT_DATA_REMOVE:
        leAdvertisingManager_AggregatorHandleClientDataRemove((const LE_ADVERTISING_MANAGER_INTERNAL_CLIENT_DATA_REMOVE_T *)message);
        break;

    case LE_ADVERTISING_MANAGER_INTERNAL_ADVERTISING_SET_SUSPEND:
        leAdvertisingManager_AggregatorHandleAdvertisinSetSuspend((const LE_ADVERTISING_MANAGER_INTERNAL_ADVERTISING_SET_SUSPEND_T *)message);
        break;

    default:
       break;
    }
}

void LeAdvertisingManager_InitAggregator(void)
{
    le_advertising_manager_aggregator_t *aggregator = LeAdvertisingManager_AggregatorGetTaskData();

    aggregator->task_data.handler = LeAdvertisingManager_AggregatorMessageHandler;
    leAdvertisingManager_ReleaseAggregatorRefreshLock();
    aggregator->refresh_callback = NULL;
    aggregator->gap_flags_item = NULL;

    LeAdvertisingManager_AggregatorGroupInit();
}

static le_adv_set_t * LeAdvertisingManager_AggregatorFindSetInGroupsForAdvertisingHandle(unsigned adv_handle)
{
    le_adv_set_t * result = NULL;

    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            continue;
        }

        le_adv_set_list_t * curr = group->set_list;

        while(curr)
        {
            le_adv_set_t * set_handle = curr->set_handle;

            if(adv_handle == LeAdvertisingManager_SetSmGetAdvHandle(set_handle->sm))
            {
                result = set_handle;
                break;
            }

            curr = curr->next;
        }
    }

    return result;
}

#define INVALID_ADVERTISING_EVENT 0xFFFF

uint16 LeAdvertisingManager_AggregatorGetEventTypeForAdvertisingSet(uint8 set_id)
{
    uint16 result = INVALID_ADVERTISING_EVENT;

    if(set_id <= LeAdvertisingManager_SetSmGetNumberOfSupportedSets())
    {
        le_adv_set_t * set_handle = LeAdvertisingManager_AggregatorFindSetInGroupsForAdvertisingHandle(set_id);

        if(set_handle)
        {
            le_adv_item_group_t * group = LeAdvertisingManager_AggregatorGetGroupForAdvertisingSet(set_id);
            if(group)
            {
                result = group->info.type;
            }
        }
    }

    return result;
}

uint32 LeAdvertisingManager_AggregatorGetSpaceInUseForAdvertisingSet(uint8 set_id)
{
    uint32 result = 0;

    if(set_id <= LeAdvertisingManager_SetSmGetNumberOfSupportedSets())
    {
        le_adv_set_t * set_handle = LeAdvertisingManager_AggregatorFindSetInGroupsForAdvertisingHandle(set_id);

        if(set_handle)
        {
            uint16 event = LeAdvertisingManager_AggregatorGetEventTypeForAdvertisingSet(set_id);
            uint32 advert_size = ( event & ADV_EVENT_BITS_USE_LEGACY_PDU)
                    ? MAX_LEGACY_DATA_SET_SIZE_IN_OCTETS
                    : MAX_EXTENDED_DATA_SET_SIZE_IN_OCTETS;

            result = advert_size - set_handle->space;
        }
    }

    return result;
}

uint32 LeAdvertisingManager_AggregatorGetMinIntervalForAdvertisingSet(uint8 set_id)
{
    uint32 result = 0;

    if(set_id <= LeAdvertisingManager_SetSmGetNumberOfSupportedSets())
    {
        le_adv_set_t * set_handle = LeAdvertisingManager_AggregatorFindSetInGroupsForAdvertisingHandle(set_id);

        if(set_handle)
        {
            le_adv_item_group_t * group = LeAdvertisingManager_AggregatorGetGroupForAdvertisingSet(set_id);

            if(group)
            {
                result = group->params.primary_adv_interval_min;
            }
        }
    }

    return result;
}

uint32 LeAdvertisingManager_AggregatorGetMaxIntervalForAdvertisingSet(uint8 set_id)
{
    uint32 result = 0;

    if(set_id <= LeAdvertisingManager_SetSmGetNumberOfSupportedSets())
    {
        le_adv_set_t * set_handle = LeAdvertisingManager_AggregatorFindSetInGroupsForAdvertisingHandle(set_id);

        if(set_handle)
        {
            le_adv_item_group_t * group = LeAdvertisingManager_AggregatorGetGroupForAdvertisingSet(set_id);

            if(group)
            {
                result = group->params.primary_adv_interval_max;
            }
        }
    }

    return result;
}

uint8 LeAdvertisingManager_AggregatorGetChannelsForAdvertisingSet(uint8 set_id)
{
    uint8 result  = 0;

    if(set_id <= LeAdvertisingManager_SetSmGetNumberOfSupportedSets())
    {
        le_adv_set_t * set_handle = LeAdvertisingManager_AggregatorFindSetInGroupsForAdvertisingHandle(set_id);

        if(set_handle)
        {
            le_adv_item_group_t * group = LeAdvertisingManager_AggregatorGetGroupForAdvertisingSet(set_id);

            if(group)
            {
                result = group->params.primary_adv_channel_map;
            }
        }
    }

    return result;
}

bool LeAdvertisingManager_AggregatorIsAdvertisingSetActive(uint8 set_id)
{
    bool result = FALSE;

    if(set_id <= LeAdvertisingManager_SetSmGetNumberOfSupportedSets())
    {
        le_adv_set_t * set_handle = LeAdvertisingManager_AggregatorFindSetInGroupsForAdvertisingHandle(set_id);

        if(set_handle)
        {
            result = set_handle->active;
        }
    }

    return result;
}

bool LeAdvertisingManager_AggregatorDisableAdvertisingSet(uint8 set_id)
{
    bool result = FALSE;

    if(set_id <= LeAdvertisingManager_SetSmGetNumberOfSupportedSets())
    {
        le_adv_set_t * set_handle = LeAdvertisingManager_AggregatorFindSetInGroupsForAdvertisingHandle(set_id);

        if( (set_handle) && !(set_handle->lock) && (set_handle->active))
        {
            set_handle->needs_disabling = TRUE;

            leAdvertisingManager_SetAdvertisingSetBusyLock(set_handle);
            LeAdvertisingManager_SetSmDisable(set_handle->sm);
            leAdvertisingManager_CheckReleaseRefreshLock();

            result = TRUE;
        }
    }

    return result;
}

uint8 LeAdvertisingManager_AggregatorGetNumberOfSupportedSets(void)
{
    return LeAdvertisingManager_SetSmGetNumberOfSupportedSets();
}

typed_bdaddr * LeAdvertisingManager_AggregatorGetTpBdaddrForAdvertisingSet(uint8 set_id)
{
    typed_bdaddr * result = NULL;

    if(set_id <= LeAdvertisingManager_SetSmGetNumberOfSupportedSets())
    {
        le_adv_set_t * set_handle = LeAdvertisingManager_AggregatorFindSetInGroupsForAdvertisingHandle(set_id);

        if(set_handle && set_handle->sm)
        {
            result = &set_handle->sm->params.random_addr;
        }
    }

    return result;

}


#endif
