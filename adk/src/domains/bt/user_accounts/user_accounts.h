/*!
   \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
               All Rights Reserved.\n
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \file       user_accounts.h
   \defgroup   user_accounts User Accounts
   @{
   \ingroup    bt_domain
   \brief      Module to store and access user account(crypto keys) information.

   \note       Presently, each Account Key added must be associated with at least one handset.
               For each Account Key, a count is maintained of every handset associated with it.
               This count is decremented whenever one of those handsets no longer exists in the TDL,
               or re-bonds using a different Account Key.

               Whenever the count reaches 0 for an Account Key, its slot is considered unused,
               and will be cleared at soonest opportunity, usually the next time another Account Key is added.
               This aims for efficient use of the Account Key list capacity, avoiding the retention of stale keys,
               at least for the primary user, Fast Pair.
*/

#ifndef USER_ACCOUNTS_H_
#define USER_ACCOUNTS_H_

#include "user_accounts_types.h"
#include "user_accounts_sync.h"

#include <bdaddr.h>
#include <device.h>

#define INVALID_USER_ACCOUNT_KEY_INDEX              (0xFF)

/*! \brief Initialise User Account Module
*/
void UserAccounts_Init(void);

/*! \brief Register User Accounts PDDU
 */
void UserAccounts_RegisterPersistentDeviceDataUser(void);

/*! \brief Add an account key to the list and bind it to a handset.
\param bd_addr Handset BD_ADDR
\param key account key to be added
\param key_len account key len
\param key_type account key type to be added
\return Index of the added account key, if successful. INVALID_USER_ACCOUNT_KEY_INDEX otherwise.
*/
uint16 UserAccounts_AddAccountKeyForHandset(const bdaddr *bd_addr, const uint8* key, uint16 key_len, uint16 key_type);


/*! \brief Get account keys by Type
\param account_keys account key to be obtained, valid memory shall be freed by caller
\param account_key_type account key type to be obtained
\return number of account keys.
*/
uint16 UserAccounts_GetAccountKeys(uint8** account_keys, uint16 account_key_type);

/*! \brief Get number of account keys stored
\return number of account keys.
*/
uint16 UserAccounts_GetNumAccountKeys(void);

/*! \brief Get the maximum number of account keys that can be stored
\return maximum number of account keys supported.
*/
uint16 UserAccounts_GetMaxNumAccountKeys(void);

/*! \brief Get the index for a given Account Key.
           This index can be used in other compatible API calls for referring to this account key.

    \param account_key Account Key
    \param account_key_len The Account Key length
    \return the Account Key index
 */
uint8 UserAccounts_GetAccountKeyIndex(const uint8* account_key, uint16 account_key_len);

/*! \brief Get the index associated with a handset.
           This index can be used in other compatible API calls for referring to this account key.

    \param device The handset device
    \return the handset's Account Key index
 */
uint8 UserAccounts_GetAccountKeyIndexWithHandset(device_t device);

/*! \brief Get Account Key associated to Handset
\param account_key_len account key len
\param bd_addr public address of the handset device
\return Pointer to the account key if a valid account key is found, NULL otherwise. Caller should free the memory allocation if valid.
 */
uint8* UserAccounts_GetAccountKeyWithHandset(uint16 account_key_len, const bdaddr *bd_addr);

/*! \brief Get the index for the Least Recently Used (LRU) Account Key.
    \return the LRU Account Key index
 */
uint8 UserAccounts_GetLruAccountKeyIndex(void);

/*! \brief Store the user account keys with the index values
     This inteface can be used to store the complete user account key info
    \param user_account_key_sync_req_t* reference to the user account key info
    \return bool TRUE if account keys are stored else FALSE
 */
bool UserAccounts_StoreAllAccountKeys(account_key_sync_req_t* account_key_info);

/*! \brief Delete the account key
\param account_key_index account key index to be deleted
\return TRUE to indicate successful deletion, FALSE otherwise.
*/
bool UserAccounts_DeleteAccountKey(uint16 account_key_index);


/*! \brief Delete All account keys
\return TRUE to indicate successful deletion, FALSE otherwise.
*/
bool UserAccounts_DeleteAllAccountKeys(void);

#endif /* USER_ACCOUNTS_H_ */

/*! @} */