/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup    le_advertising_manager_multi_set
    \brief      Internal defines used by the advertising manager
    @{
*/

#ifndef LE_ADVERTISING_MANAGER_MULTI_SET_PRIVATE_H_
#define LE_ADVERTISING_MANAGER_MULTI_SET_PRIVATE_H_

#include "bdaddr.h"

/*! \brief Check if advertising is allowed.

    \return TRUE if advertising is allowed, FALSE otherwise.
*/
bool LeAdvertisingManager_IsAdvertisingAllowed(void);

/*! \brief Check if connectable advertising is enabled.

    \return TRUE if connectable advertising is enabled, FALSE otherwise.
*/
bool LeAdvertisingManager_IsConnectableAdvertisingEnabled(void);

/*! \brief Get number of supported advertising sets.

    \return Number of supported advertising sets.
*/
uint8 LeAdvertisingManager_AggregatorGetNumberOfSupportedSets(void);

/*! \brief Check if an advertising set with a given ID is active.

    \param set_id Numeric value from 1 to N to identify an advertising set, N matching the maximum number of supported sets.
    \return TRUE if set is active, FALSE otherwise.
*/
bool LeAdvertisingManager_AggregatorIsAdvertisingSetActive(uint8 set_id);

/*! \brief Disable advertising for an advertising set with a given ID.

    \param set_id Numeric value from 1 to N to identify an advertising set, N matching the maximum number of supported sets.
    \return TRUE if set is disabled with success, FALSE otherwise.
*/
bool LeAdvertisingManager_AggregatorDisableAdvertisingSet(uint8 set_id);

/*! \brief Get space in use by an advertising set with a given ID.

    \param set_id Numeric value from 1 to N to identify an advertising set, N matching the maximum number of supported sets.
    \return Space used by advertising set in octets.
*/
uint32 LeAdvertisingManager_AggregatorGetSpaceInUseForAdvertisingSet(uint8 set_id);

/*! \brief Get event type for an advertising set with a given ID.

    \param set_id Numeric value from 1 to N to identify an advertising set, N matching the maximum number of supported sets.
    \return Event type associated with the advertising set.
*/
uint16 LeAdvertisingManager_AggregatorGetEventTypeForAdvertisingSet(uint8 set_id);

/*! \brief Get minimum interval for an advertising set with a given ID.

    \param set_id Numeric value from 1 to N to identify an advertising set, N matching the maximum number of supported sets.
    \return Minimum interval value associated with the advertising set.
*/
uint32 LeAdvertisingManager_AggregatorGetMinIntervalForAdvertisingSet(uint8 set_id);

/*! \brief Get maximum interval for an advertising set with a given ID.

    \param set_id Numeric value from 1 to N to identify an advertising set, N matching the maximum number of supported sets.
    \return Maximum interval value associated with the advertising set.
*/
uint32 LeAdvertisingManager_AggregatorGetMaxIntervalForAdvertisingSet(uint8 set_id);

/*! \brief Get channels for an advertising set with a given ID.

    \param set_id Numeric value from 1 to N to identify an advertising set, N matching the maximum number of supported sets.
    \return Channel bitmap associated with the advertising set.
*/
uint8 LeAdvertisingManager_AggregatorGetChannelsForAdvertisingSet(uint8 set_id);

/*! \brief Get typed BT address for an advertising set with a given ID.

    \param set_id Numeric value from 1 to N to identify an advertising set, N matching the maximum number of supported sets.
    \return Typed BT address.
*/
typed_bdaddr * LeAdvertisingManager_AggregatorGetTpBdaddrForAdvertisingSet(uint8 set_id);

#endif
/*! @} */
