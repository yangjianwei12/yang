/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup    le_advertising_manager_multi_set
    \brief      Data types used by the LE advertising aggregator.
    @{
*/

#ifndef LE_ADVERTISING_MANAGER_AGGREGATOR_TYPES_H
#define LE_ADVERTISING_MANAGER_AGGREGATOR_TYPES_H

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

#include "le_advertising_manager.h"
#include "le_advertising_manager_data_packet.h"
#include "le_advertising_manager_set_sm.h"

/*! \brief The maximum number of aggregator advertising groups that are supported. */
#define MAX_ADVERTISING_GROUPS  8

typedef struct __advertising_item
{
    le_adv_item_handle handle;
    uint32 size;
    struct __advertising_item *next;
} le_adv_item_list_t;

typedef struct
{
    bool needs_registering:1;
    bool needs_params_update:1;
    bool needs_data_update:1;
    bool needs_enabling:1;
    bool needs_disabling:1;
    bool needs_destroying:1;
    bool active:1;

    uint32 space;
    uint16 lock;

    le_advertising_manager_set_state_machine_t * sm;

    uint32 number_of_items;
    le_adv_item_list_t * item_list;

} le_adv_set_t;

typedef struct __advertising_set
{
    le_adv_set_t *set_handle;
    struct __advertising_set *next;
} le_adv_set_list_t;

typedef struct
{
    /*! Does this group need to be refreshed. */
    bool needs_refresh:1;

    /*! Is this group currently in use by the aggregator. */
    bool is_in_use:1;

    /*! Linked list of all advertising items in this group. */
    le_adv_item_list_t * item_handles;

    /*! Total number of advertising items. */
    uint32 number_of_items;

    /*! Advertising parameters for this group. */
    le_adv_item_params_t params;

    /*! Advertising info for this group. */
    le_adv_item_info_t info;

    /*! Total number of advertising sets in this group. */
    uint32 number_of_sets;

    /*! linked list of advertising sets in this group. */
    le_adv_set_list_t * set_list;

    /*! A single set for the scan response data items in this group.
        All advertising sets in the group use this when setting their
        scan response data. */
    le_adv_set_t * scan_resp_set;

} le_adv_item_group_t;

/*! \brief Definition for the object to provide input for refresh operation
 */
typedef struct
{
    void (*advertising_state_update_callback)(void);
}le_adv_refresh_control_t;

/*! \brief Internal messages for the aggregator. */
typedef enum {
    LE_ADVERTISING_MANAGER_ADVERTISING_STATE_UPDATE = INTERNAL_MESSAGE_BASE,
    LE_ADVERTISING_MANAGER_INTERNAL_AGGREGATOR_MSG_CHECK_REFRESH_LOCK,
    LE_ADVERTISING_MANAGER_CLIENT_DATA_UPDATE,
    LE_ADVERTISING_MANAGER_INTERNAL_DEFAULT_PARAMETERS_FALLBACK_TIMEOUT,
    LE_ADVERTISING_MANAGER_INTERNAL_CLIENT_DATA_REMOVE,
    LE_ADVERTISING_MANAGER_INTERNAL_ADVERTISING_SET_SUSPEND,

    /*! This must be the final message */
    LE_ADVERTISING_MANAGER_INTERNAL_MESSAGE_END
} le_advertising_manager_internal_aggregator_msg_t;

/*! \brief Internal aggregator message */
typedef struct {
    le_adv_refresh_control_t control;
} LE_ADVERTISING_MANAGER_ADVERTISING_STATE_UPDATE_T;

typedef struct
{
    le_adv_item_handle item_handle;
} LE_ADVERTISING_MANAGER_CLIENT_DATA_UPDATE_T;

typedef struct {
    le_adv_preset_advertising_interval_t interval;
} LE_ADVERTISING_MANAGER_INTERNAL_DEFAULT_PARAMETERS_FALLBACK_TIMEOUT_T;

typedef LE_ADVERTISING_MANAGER_CLIENT_DATA_UPDATE_T LE_ADVERTISING_MANAGER_INTERNAL_CLIENT_DATA_REMOVE_T;

typedef struct {
    le_advertising_manager_set_state_machine_t * sm;
}LE_ADVERTISING_MANAGER_INTERNAL_ADVERTISING_SET_SUSPEND_T;

#if (LOG_LEVEL_CURRENT_SYMBOL >= DEBUG_LOG_LEVEL_V_VERBOSE)
#define DEBUG_GROUPS_ENABLED
#endif

#define LEAM_DEBUG_LOG  DEBUG_LOG_VERBOSE

#ifdef DEBUG_GROUPS_ENABLED
void leAdvertisingManager_LoggingItemData(le_adv_item_handle item);
#else
#define leAdvertisingManager_LoggingItemData(x) UNUSED(x)
#endif

/*! \brief  */
bool leAdvertisingManager_IsAdvertisingTypeExtended(le_adv_data_type_t type);

/*! \brief  */
bool leAdvertisingManager_IsAdvertisingTypeConnectable(le_adv_data_type_t type);

bool leAdvertisingManager_IsDirectedAdvertisingToBeEnabledForGroup(le_adv_item_group_t *);

bool leAdvertisingManager_IsAdvertisingTypeLegacyDirected(le_adv_data_type_t type);

#endif

#endif // LE_ADVERTISING_MANAGER_AGGREGATOR_TYPES_H
/*! @} */