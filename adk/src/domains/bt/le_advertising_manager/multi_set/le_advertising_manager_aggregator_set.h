/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup    le_advertising_manager_multi_set
    \brief      Header file for LE advertising manager aggregator set.

    An aggregator set is the advertising items that will be included in a single OTA advertising set.
    @{
*/

#ifndef LE_ADVERTISING_MANAGER_AGGREGATOR_SET_H
#define LE_ADVERTISING_MANAGER_AGGREGATOR_SET_H

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

#include "le_advertising_manager_aggregator_types.h"

uint32 leAdvertisingManager_GetItemSize(le_adv_item_handle item);

void leAdvertisingManager_SetAdvertisingSetBusyLock(le_adv_set_t *set);

void leAdvertisingManager_ReleaseAdvertisingSetBusyLock(le_adv_set_t *set);

le_adv_set_t * leAdvertisingManager_GetSetWithFreeSpace(le_adv_set_list_t * set_list, uint32 free_space_size);

/*! \brief Add an item to the end of the list of items in a set.

    \param set Set to add the Item to.
    \param handle Handle to the Item to add.
    \param size Size, in octets, of the Item.
*/
void leAdvertisingManager_AddItemToSet(le_adv_set_t * set, le_adv_item_handle handle, uint32 size);

/*! \brief Add an item to the beginning of the list of items in a set.

    \param set Set to add the item to.
    \param handle Handle to the item to add.
    \param size Size, in octets, of the item.
*/
void leAdvertisingManager_PrependItemToSet(le_adv_set_t *set, le_adv_item_handle handle, uint32 size);

/*! \brief Remove an advertising item from an aggregator set.

    \param set Aggregator set to remove the item from.
    \param handle Handle to the item to remove.
*/
void LeAdvertisingManager_RemoveItemfromSet(le_adv_set_t *set, le_adv_item_handle handle);

/*! \brief Check if there are no items in this set.

    \param set Set to check.

    \return TRUE if there are no items; FALSE otherwise.
*/
#define LeAdvertisingManager_SetIsEmpty(set) ((set)->number_of_items == 0)

/*! \brief Destroy an aggregator set.

    The set must be empty and if it represents a set in the BT stack it must
    be unregsitered with the BT stack.

    \param set Set to destroy.
*/
void LeAdvertisingManager_DestroyAdvertisingSet(le_adv_set_t *set);

#endif

#endif // LE_ADVERTISING_MANAGER_AGGREGATOR_SET_H
/*! @} */