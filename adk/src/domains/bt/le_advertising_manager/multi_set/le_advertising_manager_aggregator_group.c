/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_advertising_manager_multi_set
    \brief
*/

#include "le_advertising_manager_aggregator_group.h"

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

#include "le_advertising_manager_aggregator.h"
#include "le_advertising_manager_aggregator_set.h"
#include "le_advertising_manager_multi_set_private.h"

#include <bdaddr.h>
#include <logging.h>
#include <app/bluestack/dm_prim.h>


le_adv_item_group_t groups[MAX_ADVERTISING_GROUPS];
static uint32 number_of_groups = 0;


static bool leAdvertisingManager_HasSidBeenSet(uint16 sid)
{
    return (sid && sid != DM_ULP_EXT_ADV_SID_INVALID);
}

static unsigned leAdvertisingManager_GetNewSid(le_adv_data_type_t type)
{
    uint16 sid = DM_ULP_EXT_ADV_SID_INVALID;
    if(leAdvertisingManager_IsAdvertisingTypeExtended(type))
    {
        sid = DM_ULP_EXT_ADV_SID_ASSIGNED_BY_STACK;
    }
    return sid;
}

#define is_random_address_to_be_specified_by_user(type) (type == ble_local_addr_write_static || type == ble_local_addr_write_non_resolvable || type == ble_local_addr_write_resolvable)

static inline bool leAdvertisingManager_IsAddressConfigTheSame(le_adv_item_params_t * params1, le_adv_item_params_t * params2)
{
    bool same_address = FALSE;
    if(params1->own_addr_type == params2->own_addr_type) 
    {
        if(params1->own_addr_type == OWN_ADDRESS_PUBLIC)
        {
            same_address = TRUE;
        }
        else if(params1->own_addr_type == OWN_ADDRESS_RANDOM &&
                params1->random_addr_type == params2->random_addr_type &&
                params1->random_addr_generate_rotation_timeout_minimum_in_minutes == params2->random_addr_generate_rotation_timeout_minimum_in_minutes &&
                params1->random_addr_generate_rotation_timeout_maximum_in_minutes == params2->random_addr_generate_rotation_timeout_maximum_in_minutes )
        {
            if(is_random_address_to_be_specified_by_user(params1->random_addr_type))
            {
                same_address = BdaddrTypedIsSame(&params1->random_addr,  &params2->random_addr);
            }
            else
            {
                same_address = TRUE;
            }
        }
    }
    return same_address;
}

static bool leAdvertisingManager_DoParamsMatch(le_adv_item_params_t * params1, le_adv_item_params_t * params2)
{
    bool result = FALSE;

    DEBUG_LOG_VERBOSE("LEAM Min Interval Params1 0x%x, Params2 0x%x]",  params1->primary_adv_interval_min, params2->primary_adv_interval_min);
    DEBUG_LOG_VERBOSE("LEAM Max Interval Params1 0x%x, Params2 0x%x]",  params1->primary_adv_interval_max, params2->primary_adv_interval_max);
    DEBUG_LOG_VERBOSE("LEAM Adv Filter Policy Params1 0x%x, Params2 0x%x]",  params1->adv_filter_policy, params2->adv_filter_policy);

    if((params1->primary_adv_interval_min == params2->primary_adv_interval_min)&&
       (params1->primary_adv_interval_max == params2->primary_adv_interval_max)&&
       (params1->adv_filter_policy == params2->adv_filter_policy) &&
       leAdvertisingManager_IsAddressConfigTheSame(params1, params2))
    {
        DEBUG_LOG_VERBOSE("LEAM Params Match!");
        result = TRUE;
    }

    return result;
}

/*! \brief Check if an item info is compatible with an existing item info.

    This is used when matching an advertising item with the group it will be put
    into.

    \note This function does not compare the placement type. This is done on
          purpose so that matching advert items and scan response items will be
          put into the same group.

    \param info1
    \param info2
*/
static bool leAdvertisingManager_DoesInfoMatch(le_adv_item_info_t * info1, le_adv_item_info_t * info2)
{
    bool result = FALSE;

    DEBUG_LOG_VERBOSE("LEAM Override Info1 0x%x, Info2 0x%x]",  info1->override_connectable_state, info2->override_connectable_state);

    if(info1->override_connectable_state == info2->override_connectable_state
            && info1->type == info2->type
            && !info1->needs_own_set && !info2->needs_own_set
            && info1->placement == info2->placement)
    {
        DEBUG_LOG_VERBOSE("LEAM Info Match!");
        result = TRUE;
    }

    return result;
}

le_adv_item_group_t * leAdvertisingManager_GetGroupForSet(le_adv_set_t * set)
{
    le_adv_item_group_t * group_for_set = NULL;

    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            continue;
        }

        if (group->number_of_sets)
        {
            le_adv_set_list_t * set_to_check = group->set_list;
            while(set_to_check != NULL && group_for_set == NULL)
            {
                if(set == set_to_check->set_handle)
                {
                    group_for_set = group;
                    break;
                }
                set_to_check = set_to_check->next;
            }
        }

        if (!group_for_set && (set == group->scan_resp_set))
        {
            group_for_set = group;
            break;
        }
    }

    DEBUG_LOG_VERBOSE("LEAM GetGroupForSet set [0x%x] group [0x%x]", set, group_for_set);
    return group_for_set;
}

le_adv_item_group_t * leAdvertisingManager_GetGroupLinkedToItem(le_adv_item_handle handle)
{
    DEBUG_LOG_VERBOSE("LEAM Is Item [0x%x] Linked to Any Group", handle);

    le_adv_item_group_t *group_found = NULL;

    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            continue;
        }

        DEBUG_LOG_VERBOSE("LEAM [Group 0x%x ]", group);

        le_adv_item_list_t * head = group->item_handles;
        while(head != NULL)
        {
            DEBUG_LOG_VERBOSE("LEAM [Item Handles 0x%x ]", head->handle );
            if(handle == head->handle)
            {
                DEBUG_LOG_VERBOSE("LEAM Item Found in Group [0x%x]" , group);
                group_found = group;
                break;
            }

            head = head->next;
        }

        if(group_found)
        {
            break;
        }
    }
    return group_found;
}

le_adv_item_group_t * leAdvertisingManager_GetGroupForParams(le_adv_item_info_t * item_info, le_adv_item_params_t * item_params)
{
    le_adv_item_group_t * group_for_params = NULL;

    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            continue;
        }

        if (   leAdvertisingManager_DoParamsMatch(item_params, &group->params)
            && leAdvertisingManager_DoesInfoMatch(item_info, &group->info))
        {
            DEBUG_LOG_VERBOSE("LEAM Group Found with Matching Params group [%p]", group);
            group_for_params = group;
            break;
        }
    }

    return group_for_params;
}

le_adv_item_group_t * leAdvertisingManager_CreateNewGroup(le_adv_item_info_t * info, le_adv_item_params_t * params)
{
    DEBUG_LOG_VERBOSE("LEAM CreateNewGroup");

    PanicNull(params);
    PanicNull(info);
    number_of_groups = number_of_groups + 1;
    PanicFalse(number_of_groups <= MAX_ADVERTISING_GROUPS);
    le_adv_item_group_t *new_group = NULL;

    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            new_group = group;
            break;
        }
    }

    if(new_group)
    {
        memset(new_group, 0, sizeof(*new_group));
        new_group->is_in_use = TRUE;

        new_group->params = *params;
        new_group->info = *info;

        if(!leAdvertisingManager_HasSidBeenSet(new_group->params.adv_sid))
        {
            new_group->params.adv_sid = leAdvertisingManager_GetNewSid(new_group->info.type);
        }
    }

    DEBUG_LOG_VERBOSE("LEAM CreateNewGroup End, Groups [0x%x] Group [0x%x] Number of Groups [%d]", &groups, new_group, number_of_groups);

    return new_group;
}

void LeAdvertisingManager_DestroyGroup(le_adv_item_group_t *group)
{
    DEBUG_LOG_FN_ENTRY("LEAM DestroyGroup group [0x%x]", group);

    PanicFalse(LeAdvertisingManager_GroupIsEmpty(group));

    memset(group, 0, sizeof(*group));

    number_of_groups--;
}

void leAdvertisingManager_AddItemToGroup(le_adv_item_group_t * group, le_adv_item_handle handle)
{
    DEBUG_LOG_FN_ENTRY("LEAM AddItemToGroup group [%p] handle [%p]", group, handle);

    le_adv_item_list_t * new_item = PanicUnlessMalloc(sizeof(le_adv_item_list_t));
    memset(new_item, 0, sizeof(*new_item));

    new_item->handle = handle;
    new_item->next = NULL;

    if(group->item_handles == NULL)
    {
        group->item_handles = new_item;
    }
    else
    {
        for(le_adv_item_list_t * item = group->item_handles; item != NULL; item = item->next)
        {
            if(item->next == NULL)
            {
                item->next = new_item;
                break;
            }
        }
    }
    group->number_of_items++;
}

void leAdvertisingManager_RemoveItemFromGroup(le_adv_item_group_t * group, le_adv_item_handle handle)
{
    DEBUG_LOG_VERBOSE("LEAM Removing item %p from group %p", handle, group);
    PanicNull(handle);
    PanicFalse(group->number_of_items);

    le_adv_item_list_t * item = group->item_handles;
    le_adv_item_list_t * previous_item = group->item_handles;

    while(item != NULL && item->handle != handle)
    {
        previous_item = item;
        item = item->next;
    }

    if(item)
    {
        if (group->item_handles == item)
        {
            group->item_handles = previous_item->next;
        }
        else
        {
            previous_item->next = item->next;
        }
        free(item);
        group->number_of_items--;

        /* Remove the item from any set it is in.
           Also check the scan response set. */
        le_adv_set_list_t * set_list = group->set_list;
        while (set_list)
        {
            LeAdvertisingManager_RemoveItemfromSet(set_list->set_handle, handle);

            set_list = set_list->next;
        }

        if (group->scan_resp_set)
        {
            LeAdvertisingManager_RemoveItemfromSet(group->scan_resp_set, handle);
        }
    }
    else
    {
        DEBUG_LOG_WARN("LEAM Item %p not found in group %p", handle, group);
    }
}

void leAdvertisingManager_AddSetToGroup(le_adv_item_group_t * group, le_adv_set_t * set_to_add)
{
    DEBUG_LOG_VERBOSE("LEAM Add set %p to group %p", set_to_add, group);
    PanicNull(set_to_add);
    PanicNull(group);

    le_adv_set_list_t * new_set = PanicUnlessMalloc(sizeof(le_adv_set_list_t));
    new_set->set_handle = set_to_add;
    new_set->next = NULL;

    if(group->set_list == NULL)
    {
        group->set_list = new_set;
    }
    else
    {
        for(le_adv_set_list_t * set = group->set_list; set != NULL; set = set->next)
        {
            DEBUG_LOG_VERBOSE("    set %p", set->set_handle);
            if(set->next == NULL)
            {
                set->next = new_set;
                break;
            }
        }
    }
    group->number_of_sets++;
}

void leAdvertisingManager_RemoveSetFromGroup(le_adv_item_group_t * group, le_adv_set_t * set_to_remove)
{
    DEBUG_LOG_VERBOSE("LEAM Remove set %p from group %p", set_to_remove, group);
    PanicNull(set_to_remove);
    PanicNull(group);
    PanicFalse(group->number_of_sets || group->scan_resp_set);

    if (group->set_list->set_handle == set_to_remove)
    {
        le_adv_set_list_t *set_list_to_remove = group->set_list;

        DEBUG_LOG_VERBOSE("  removing set_handle %p", set_list_to_remove->set_handle);

        group->set_list = set_list_to_remove->next;
        free(set_list_to_remove);
        group->number_of_sets--;
    }
    else
    {
        le_adv_set_list_t * previous_set = group->set_list;
        for (le_adv_set_list_t * set = group->set_list; set != NULL; set = set->next)
        {
            if(set->set_handle == set_to_remove)
            {
                DEBUG_LOG_VERBOSE("  removing set_handle %p", set->set_handle);
                previous_set->next = set->next;
                free(set);
                group->number_of_sets--;
                break;
            }
            previous_set = set;
        }
    }

    if (group->scan_resp_set == set_to_remove)
    {
        DEBUG_LOG("  Removing scan response set [0x%x]", group->scan_resp_set);
        group->scan_resp_set = NULL;
    }
}

bool leAdvertisingManager_DoesGroupPassAdvertisingCriteria(le_adv_item_group_t * group)
{
    DEBUG_LOG_VERBOSE("LEAM Does Group [0x%x] Pass Advertising Criteria", group);

    bool result = FALSE;

    if (group->number_of_items)
    {
        if(LeAdvertisingManager_IsAdvertisingAllowed())
        {
            DEBUG_LOG_VERBOSE("LEAM Advertising Allowed");

            if(group->info.override_connectable_state)
            {
                DEBUG_LOG_VERBOSE("LEAM Advertising Group Override Logic");

                if(leAdvertisingManager_IsAdvertisingTypeLegacyDirected(group->info.type))
                {
                    result = leAdvertisingManager_IsDirectedAdvertisingToBeEnabledForGroup(group);

                    DEBUG_LOG_VERBOSE("LEAM Group Directed Advertising To Be Enabled [%d]", result);
                }
                else
                {
                    result = TRUE;
                }
            }
            else if(leAdvertisingManager_IsAdvertisingTypeConnectable(group->info.type))
            {
                if(LeAdvertisingManager_IsConnectableAdvertisingEnabled())
                {
                    DEBUG_LOG_VERBOSE("LEAM Group Connectable Advertising");

                    if(leAdvertisingManager_IsAdvertisingTypeLegacyDirected(group->info.type))
                    {
                        result = leAdvertisingManager_IsDirectedAdvertisingToBeEnabledForGroup(group);

                        DEBUG_LOG_VERBOSE("LEAM Group Directed Advertising To Be Enabled [%d]", result);
                    }
                    else
                    {
                        result = TRUE;
                    }
                }
            }
            else
            {
                DEBUG_LOG_VERBOSE("LEAM Group non-connectable Advertising");

                result = TRUE;
            }
        }
    }
    else
    {
        DEBUG_LOG_VERBOSE("LEAM Group contains no items; nothing to advertise.");

        result = FALSE;
    }

    return result;
}

bool leAdvertisingManager_IsGroupToBeRefreshed(le_adv_item_group_t * group)
{
    return group->needs_refresh;
}

void leAdvertisingManager_MarkGroupToBeRefreshed(le_adv_item_group_t * group, bool refresh)
{
    group->needs_refresh = refresh;
}

bool LeAdvertisingManager_GroupIsEmpty(const le_adv_item_group_t *group)
{
    bool is_empty = FALSE;

    if (   group
        && (group->number_of_items == 0)
        && (group->number_of_sets == 0)
        && (!group->scan_resp_set))
    {
        PanicNotNull(group->item_handles);
        PanicNotNull(group->set_list);

        is_empty = TRUE;
    }

    return is_empty;
}

void LeAdvertisingManager_AggregatorGroupInit(void)
{
    memset(groups, 0, sizeof(groups));

    number_of_groups = 0;
}

uint32 LeAdvertisingManager_GetNumberOfGroupsInUse(void)
{
    return number_of_groups;
}

#endif
