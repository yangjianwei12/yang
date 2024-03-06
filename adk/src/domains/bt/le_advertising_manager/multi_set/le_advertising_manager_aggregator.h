/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_advertising_manager_multi_set
    \brief      Header file for LE advertising manager aggregator, which is responsible for collecting individual advertising items and creating advertising sets
    @{
*/

#ifndef LE_ADVERTISING_MANAGER_AGGREGATOR_H_
#define LE_ADVERTISING_MANAGER_AGGREGATOR_H_

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

#include "le_advertising_manager.h"
#include "le_advertising_manager_aggregator_types.h"


/*! \brief API to initialise advertising item aggregator
    \note This is the API function to be called before calling any other API functions this module owns
*/
void LeAdvertisingManager_InitAggregator(void);

/*! \brief API to update advertising following a change in advertising state
    \param control Pointer to le_adv_refresh_control_t object to provide input for refresh operation
*/
void LeAdvertisingManager_QueueAdvertisingStateUpdate(le_adv_refresh_control_t * control);

/*! \brief API to update advertising following a change in client data 
    \param item_handle handle to the client data flagging an update
*/
void LeAdvertisingManager_QueueClientDataUpdate(le_adv_item_handle item_handle);

/*! \brief Store the GAP flags advertising item.

    The GAP flags shall be included in any connectable advert set so they are
    stored globally in the aggregator instead of in a single group.

    \param item_handle to the GAP flags advertising item.
*/
void LeAdvertisingManager_AggregatorSetFlagsItem(le_adv_item_handle item_handle);

/*! \brief Queue an internal message to change the default parameters after a time delay.

    \param interval Interval to use after the time delay.
    \param delay Delay in seconds.
*/
void LeAdvertisingManager_QueueDefaultParametersFallbackTimeout(le_adv_preset_advertising_interval_t interval, uint32 delay);

/*! \brief Remove an advertising item from the aggregator.

    The item will be removed from any group(s) and set(s) it belongs to.

    Any group or set the item currently belongs to shall be refreshed after the
    item is removed. If the group or set is empty then it may be destroyed.

    \param item_handle Handle of the item to remove.
*/
void LeAdvertisingManager_QueueClientDataRemove(le_adv_item_handle item_handle);

#endif

#endif /* LE_ADVERTISING_MANAGER_AGGREGATOR_H_ */
/*! @} */