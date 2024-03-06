/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup    le_advertising_manager_multi_set
    \brief      Header file for LE advertising manager functionality associated with advertising item database
    @{

*/

#ifndef LE_ADVERTISING_MANAGER_ADVERTISING_ITEM_DATABASE_H_
#define LE_ADVERTISING_MANAGER_ADVERTISING_ITEM_DATABASE_H_

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

/*! \brief Data type for the iterator object to be used as input parameter for the API functions
 to retrieve advertising items stored in the advertising item database */
typedef struct
{
    le_adv_item_handle handle;
}le_adv_item_iterator_t;

/*! \brief Data type for the advertising item object */
struct _le_adv_item
{
    Task task;
    const le_adv_item_callback_t *callback;
};

/*! \brief API to initialise advertising item database
    \note This is the API function to be called before calling any other API functions this module owns
*/
void LeAdvertisingManager_InitAdvertisingItemDatabase(void);

/*! \brief API to retrieve maximum number of advertising items to be stored in the advertising item database
    \return Maximum number of advertising items of type uint32 to be stored in the advertising item database
*/
uint32 LeAdvertisingManager_GetAdvertisingItemDatabaseMaxSize(void);

/*! \brief Get the number of items curently in the database.

    \return Number of items currently in the database.
*/
uint32 LeAdvertisingManager_GetAdvertisingItemDatabaseCurrentSize(void);

/*! \brief Add an item to the database.

    \param task Client Task for the item.
    \param callback Client callback implementation for the item.

    \return A handle to the new item or NULL if it could not be added.
*/
le_adv_item_handle LeAdvertisingManager_AdvertisingItemDatabaseAddItem(Task task, const le_adv_item_callback_t * callback);

/*! \brief Remove an item from the database.

    \param[in] item Handle to the item to remove.

    \return TRUE if item was found and removed; FALSE otherwise.
*/
bool LeAdvertisingManager_AdvertisingItemDatabaseRemoveItem(le_adv_item_handle item);

#endif

#endif /* LE_ADVERTISING_MANAGER_ADVERTISING_ITEM_DATABASE_H_ */
/*! @} */