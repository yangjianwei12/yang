/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_advertising_manager_multi_set
    \brief
*/

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

#include "le_advertising_manager.h"
#include "le_advertising_manager_advertising_item_database.h"
#include "le_advertising_manager_aggregator.h"
#include "le_advertising_manager_aggregator_group.h"
#include "le_advertising_manager_default_parameters.h"

#include "local_addr.h"

#include <logging.h>


typedef struct
{
    /*! Registered advertising parameter sets */
    const le_adv_parameters_set_t * params_set;

    /*! Registered advertising parameter config table */
    const le_adv_parameters_config_table_t * config_table;

    /*! Selected advertising parameter set */
    le_adv_preset_advertising_interval_t active_params_set;

    /*! Selected config table entry */
    le_adv_advertising_config_set_t active_config_table_entry;

} le_adv_default_params_t;


#define DEFAULT_ADV_EVENT_PROPERTIES 0x13
#define DEFAULT_ADV_INTERVAL_MIN MSEC_TO_LE_TIMESLOT(225)
#define DEFAULT_ADV_INTERVAL_MAX MSEC_TO_LE_TIMESLOT(250)
#define DEFAULT_PRIMARY_ADV_CHANNEL_MAP 0x7
#define DEFAULT_OWN_ADDR_TYPE OWN_ADDRESS_RANDOM
#define DEFAULT_PEER_TPADDR {0xFF, { 0x00000000, 0x00, 0x0000 } }
#define DEFAULT_ADV_FILTER_POLICY 0x0
#define DEFAULT_PRIMARY_ADV_PHY 0x1
#define DEFAULT_SECONDARY_ADV_MAX_SKIP 0x0
#define DEFAULT_SECONDARY_ADV_PHY 0x1
#define DEFAULT_SID 0x100


/*! Default params config set by a client of the le advertising manager. */
static le_adv_default_params_t default_client_adv_parameters = {0};

/*! The default params that the le advertising manager will use unless:

    - An advertising item overrides one or more of the params in its
      GetItemParameters implementation.

    - A client has registered a default params config with
      LeAdvertisingManager_ParametersRegister and selected one of the configs
      with LeAdvertisingManager_ParametersSelect.
*/
const static le_adv_item_params_t default_adv_parameters = {
    .primary_adv_interval_min = DEFAULT_ADV_INTERVAL_MIN,
    .primary_adv_interval_max = DEFAULT_ADV_INTERVAL_MAX,
    .primary_adv_channel_map = DEFAULT_PRIMARY_ADV_CHANNEL_MAP,
    .own_addr_type = DEFAULT_OWN_ADDR_TYPE,
    .peer_tpaddr = {
        .type = TYPED_BDADDR_INVALID,
        .addr = { .lap = 0x00000000, .uap = 0x00, .nap = 0x0000 }
    },
    .adv_filter_policy = DEFAULT_ADV_FILTER_POLICY,
    .primary_adv_phy = DEFAULT_PRIMARY_ADV_PHY,
    .secondary_adv_max_skip = DEFAULT_SECONDARY_ADV_MAX_SKIP,
    .secondary_adv_phy = DEFAULT_SECONDARY_ADV_PHY,
    .adv_sid = DEFAULT_SID,
    .random_addr_type = ble_local_addr_use_global,
    .random_addr = {
        .type = TYPED_BDADDR_INVALID,
        .addr = { .lap = 0x00000000, .uap = 0x00, .nap = 0x0000 }
    },
};

#define leAdvertisingManager_GetDefaultAdvParameters() (&default_client_adv_parameters)


/*! \brief Update the advertising groups with the new default parameters

    Only groups that have items that all use the default parameters
    shall be updated.

    For all groups:
        If group is using default params:
            Update the cached params
            Flag all sets in the group for a params refresh.
        Else
            Don't update groups that are using custom params.

    If any group was flagged for refresh:
        Send internal LE_ADVERTISING_MANAGER_ADVERTISING_STATE_UPDATE
*/
static void leAdvertisingManager_DefaultParametersUpdateGroups(void)
{
    bool any_group_params_updated = FALSE;
    le_adv_item_params_t params = {0};

    LeAdvertisingManager_PopulateDefaultAdvertisingParams(&params);

    FOR_EACH_AGGREGATOR_GROUP(group)
    {
        if (!LeAdvertisingManager_GroupIsInUse(group))
        {
            continue;
        }

        bool group_uses_default_params = FALSE;

        /* If all items in the group implement GetItemParameters
           then assume this group uses custom advertising parameters. */
        le_adv_item_list_t *item_handle = group->item_handles;

        while (item_handle)
        {
            bool found_item_with_default_params = FALSE;

            if (item_handle->handle->callback->GetItemParameters)
            {
                DEBUG_LOG("LEAM DefaultParametersUpdateGroups, item [%x] in group[%x] with custom params callback [%p]", item_handle, group, item_handle->handle->callback->GetItemParameters );
            }
            else
            {
                found_item_with_default_params = TRUE;
            }

            if(found_item_with_default_params)
            {
                group_uses_default_params = TRUE;
                break;
            }

            item_handle = item_handle->next;
        }

        /* Only update the params in groups using default parameters. */
        if (group_uses_default_params)
        {
            if (   (group->params.primary_adv_interval_min != params.primary_adv_interval_min)
                || (group->params.primary_adv_interval_max != params.primary_adv_interval_max))
            {
                le_adv_set_list_t * set = group->set_list;

                /* Override the min / max interval only */
                group->params.primary_adv_interval_min = params.primary_adv_interval_min;
                group->params.primary_adv_interval_max = params.primary_adv_interval_max;

                while (set != NULL)
                {
                    set->set_handle->needs_params_update = TRUE;

                    set = set->next;
                }

                any_group_params_updated = TRUE;
            }
        }
    }

    if (any_group_params_updated)
    {
        le_adv_refresh_control_t control = { .advertising_state_update_callback = NULL };
        LeAdvertisingManager_QueueAdvertisingStateUpdate(&control);
    }
}

void LeAdvertisingManager_DefaultParametersInit(void)
{
    memset(&default_client_adv_parameters, 0, sizeof(default_client_adv_parameters));

    default_client_adv_parameters.active_params_set = le_adv_preset_advertising_interval_invalid;
    default_client_adv_parameters.active_config_table_entry = le_adv_advertising_config_set_invalid;
}

bool LeAdvertisingManager_ParametersRegister(const le_adv_parameters_t *params)
{
    DEBUG_LOG("LEAM ParametersRegister");

    if (!params)
    {
        return FALSE;
    }

    le_adv_default_params_t *default_params = leAdvertisingManager_GetDefaultAdvParameters();

    memset(default_params, 0, sizeof(*default_params));
    default_params->params_set = params->sets;
    default_params->config_table = params->table;

    default_params->active_config_table_entry = le_adv_advertising_config_set_1;
    default_params->active_params_set = default_params->config_table->row[default_params->active_config_table_entry].set_default;

    return TRUE;
}

bool LeAdvertisingManager_ParametersSelect(uint8 index)
{
    le_adv_default_params_t *default_params = leAdvertisingManager_GetDefaultAdvParameters();

    DEBUG_LOG("LEAM ParametersSelect, current_index %u new_index %u",
              default_params->active_config_table_entry, index);

    if (index > le_adv_advertising_config_set_max)
    {
        DEBUG_LOG("LEAM ParametersSelect, Invalid Table Index");
        return FALSE;
    }

    if (   (!default_params->params_set)
        || (!default_params->config_table))
    {
        DEBUG_LOG("LEAM ParametersSelect, Invalid Config Table");
        return FALSE;
    }

    if (default_params->active_config_table_entry != index)
    {
        /* Update the selected parameters config entry. */
        default_params->active_config_table_entry = index;
        const le_adv_parameters_config_entry_t *row = &default_params->config_table->row[default_params->active_config_table_entry];

        default_params->active_params_set = row->set_default;
        if (row->timeout_fallback_in_seconds)
        {
            LeAdvertisingManager_QueueDefaultParametersFallbackTimeout(
                        le_adv_preset_advertising_interval_slow, row->timeout_fallback_in_seconds);
        }

        leAdvertisingManager_DefaultParametersUpdateGroups();
    }

    return TRUE;
}

bool LeAdvertisingManager_PopulateDefaultAdvertisingParams(le_adv_item_params_t * params_to_populate)
{
    le_adv_default_params_t *default_params = leAdvertisingManager_GetDefaultAdvParameters();

    PanicNull(params_to_populate);

    *params_to_populate = default_adv_parameters;

    params_to_populate->own_addr_type = LocalAddr_GetBleType();

    /* If client has set a default advert params config, use it now to
       override the default parameters. */
    if (default_params->active_config_table_entry != le_adv_advertising_config_set_invalid)
    {
        const le_adv_parameters_interval_t *params = &default_params->params_set->set_type[default_params->active_params_set];

        params_to_populate->primary_adv_interval_min = params->le_adv_interval_min;
        params_to_populate->primary_adv_interval_max = params->le_adv_interval_max;
    }

    return TRUE;
}

void LeAdvertisingManager_HandleInternalDefaultParametersFallbackTimeout(
        const LE_ADVERTISING_MANAGER_INTERNAL_DEFAULT_PARAMETERS_FALLBACK_TIMEOUT_T *msg)
{
    le_adv_default_params_t *default_params = leAdvertisingManager_GetDefaultAdvParameters();

    default_params->active_params_set = msg->interval;
    leAdvertisingManager_DefaultParametersUpdateGroups();
}

#endif /* !INCLUDE_LEGACY_LE_ADVERTISING_MANAGER */
