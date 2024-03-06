/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    adk_test_le_advertising
\brief      Interface for LE advertising related testing functions.
*/

/*! @{ */

#ifndef ADK_TEST_LE_ADVERTISING_H
#define ADK_TEST_LE_ADVERTISING_H

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

/*! \brief Get number of supported advertising sets

    \return Numeric value for the number of supported advertising sets;
*/
uint8 appTestLeAdvertising_GetNumberOfSupportedAdvertisingSets(void);

/*! \brief Check if advertising set with a given id is active

    \return TRUE if the advertising set is enabled, FALSE otherwise;
*/
bool appTestLeAdvertising_IsAdvertisingSetEnabled(uint8 set_id);

/*! \brief Disable advertising for the set with the given ID

    \return TRUE if advertising set disable operation is succesful, FALSE otherwise;
*/
bool appTestLeAdvertising_DisableAdvertisingSet(uint8 set_id);

/*! \brief Obtain advertising space in use for the set with the given ID

    \return Advertising space used by the advertising set in octets;
*/
uint32 appTestLeAdvertising_GetSpaceInUseForAdvertisingSet(uint8 set_id);

/*! \brief Obtain advertising event type for the set with the given ID

    \return Advertising event type bitmask;
*/
uint16 appTestLeAdvertising_GetEventTypeForAdvertisingSet(uint8 set_id);

/*! \brief Obtain minimum advertising interval for the set with the given ID

    \return Value in BT slots for mimimum advertising interval;
*/
uint32 appTestLeAdvertising_GetMinimumIntervalForAdvertisingSet(uint8 set_id);

/*! \brief Obtain maximum advertising interval for the set with the given ID

    \return Value in BT slots for maximum advertising interval;
*/
uint32 appTestLeAdvertising_GetMaximumIntervalForAdvertisingSet(uint8 set_id);

/*! \brief Obtain advertising channels to use for the set with the given ID

    \return Advertising channel bitmask;
*/
uint8 appTestLeAdvertising_GetChannelsInUseForAdvertisingSet(uint8 set_id);

/*! \brief Add a connectable/scannable legacy advertising set with a fixed data size and default parameters

    \return TRUE if legacy advertising set was added with success, FALSE otherwise.
*/
bool appTestLeAdvertising_AddNewLegacyAdvertisingSet(void);

/*! \brief Add a connectable/scannable extended advertising set with a fixed data size and default parameters

    \return TRUE if extended advertising set was added with success, FALSE otherwise.
*/
bool appTestLeAdvertising_AddNewExtendedAdvertisingSet(void);

/*! \brief Get typed BT address for the set with the given ID

    \return Typed BT address.
*/
typed_bdaddr * appTestLeAdvertising_GetTpBdaddrForAdvertisingSet(uint8 set_id);

#endif
#endif /* ADK_TEST_LE_ADVERTISING_H */

/*! @} */

