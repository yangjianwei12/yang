/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup    le_advertising_manager_multi_set

    \brief      Header file for LE advertising manager aggregator group.

    An aggregator group is a collection of afvertising items that have compatible advertising parameters.
    @{
*/

#ifndef LE_ADVERTISING_MANAGER_AGGERGATOR_GROUP_H
#define LE_ADVERTISING_MANAGER_AGGERGATOR_GROUP_H

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

#include "le_advertising_manager_aggregator_types.h"


extern le_adv_item_group_t groups[];

/*! Initialise the aggregator groups */
void LeAdvertisingManager_AggregatorGroupInit(void);

le_adv_item_group_t * leAdvertisingManager_GetGroupForSet(le_adv_set_t * set);

le_adv_item_group_t * leAdvertisingManager_GetGroupLinkedToItem(le_adv_item_handle handle);

le_adv_item_group_t * leAdvertisingManager_GetGroupForParams(le_adv_item_info_t * item_info, le_adv_item_params_t * item_params);

le_adv_item_group_t * leAdvertisingManager_CreateNewGroup(le_adv_item_info_t * info, le_adv_item_params_t * params);

/*! \brief Destroy an aggregator group.

    The group must be empty before this is called.

    \param group Group to destroy.
*/
void LeAdvertisingManager_DestroyGroup(le_adv_item_group_t *group);

void leAdvertisingManager_AddItemToGroup(le_adv_item_group_t * group, le_adv_item_handle handle);

void leAdvertisingManager_RemoveItemFromGroup(le_adv_item_group_t * group, le_adv_item_handle handle);

void leAdvertisingManager_AddSetToGroup(le_adv_item_group_t * group, le_adv_set_t * set_to_add);

void leAdvertisingManager_RemoveSetFromGroup(le_adv_item_group_t * group, le_adv_set_t * set_to_remove);

bool leAdvertisingManager_DoesGroupPassAdvertisingCriteria(le_adv_item_group_t * group);

bool leAdvertisingManager_IsGroupToBeRefreshed(le_adv_item_group_t * group);

void leAdvertisingManager_MarkGroupToBeRefreshed(le_adv_item_group_t * group, bool refresh);

/*! \brief Check if an aggregator group is empty.

    A group is empty if it contains no items and no sets. No sets means
    no advertising data sets and no scan reponse set.

    \param group Aggregator group to check.

    \return TRUE if the group is empty; FALSE otherwise.
*/
bool LeAdvertisingManager_GroupIsEmpty(const le_adv_item_group_t *group);

/*! \brief Get the number of currently in use aggregator groups.

    \return Number of in use groups.
*/
uint32 LeAdvertisingManager_GetNumberOfGroupsInUse(void);

/*! \brief Helper macro to loop over all the groups; both in use and not in use.

    Note: This will loop over all possible groups. To check if a group is
          in use use #LeAdvertisingManager_GroupIsInUse.
*/
#define FOR_EACH_AGGREGATOR_GROUP(_group) for (le_adv_item_group_t *_group = groups;\
                                     _group < &groups[MAX_ADVERTISING_GROUPS];\
                                     _group++)

/*! \brief Check if a group is currently in use by the aggregator. */
#define LeAdvertisingManager_GroupIsInUse(group) ((group)->is_in_use)

#endif

#endif // LE_ADVERTISING_MANAGER_AGGERGATOR_GROUP_H
/*! @} */