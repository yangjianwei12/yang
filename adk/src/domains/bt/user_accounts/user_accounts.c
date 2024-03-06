/*!
    \copyright  Copyright (c) 2022 - 2023  Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       user_accounts.c
    \ingroup    user_accounts
    \brief      Module to store and access user account(crypto keys) information
*/

#include "user_accounts.h"

#include <device.h>
#include <device_list.h>
#include <stdlib.h>
#include <byte_utils.h>
#include <panic.h>
#include <bt_device.h>
#include <logging.h>

#include <device_db_serialiser.h>
#include <device_properties.h>
#include <pddu_map.h>

#define MAX_NUM_ACCOUNT_KEYS                        (5)
#define MAX_USER_ACCOUNT_KEY_LEN                    (16)
#define DEVICE_SERIALIZATION_DELAY_IN_MS            (100)

#define UserAccounts_KeyTypeIsValid(key_type)       (key_type > user_account_type_invalid && key_type < user_account_type_max)
#define UserAccounts_KeyLengthIsValid(key_len)      (key_len > 0 && key_len <= MAX_USER_ACCOUNT_KEY_LEN)

typedef struct user_account_key_index
{
    uint8 num_handsets_linked;
    uint8 account_key_type;
    uint8 account_key_len;
    uint8 account_key_start_loc;
} user_account_key_index_t;

typedef struct user_account_key_info
{
    user_account_key_index_t account_key_index[MAX_NUM_ACCOUNT_KEYS];

    /* Very first account key, i.e in zeroth index considered as owner account key, 
     * order shall be maintained while adding new account key. Owner account key shall not 
     * be replaced when device runs out of space.
     */
    uint8  account_keys[MAX_USER_ACCOUNT_KEY_LEN * MAX_NUM_ACCOUNT_KEYS];
} user_account_key_info_t;

static void user_accounts_serialise_persistent_device_data(device_t device, void *buf, pdd_size_t offset)
{
    void *account_key_index_value = NULL;
    void *account_keys_value = NULL;
    size_t account_key_index_size = 0;
    size_t account_keys_size = 0;
    UNUSED(offset);

     /* store account key data to PS store*/
    if(Device_GetProperty(device, device_property_user_account_key_index, &account_key_index_value, &account_key_index_size) &&
        Device_GetProperty(device, device_property_user_account_keys, &account_keys_value, &account_keys_size))
    {
        user_account_key_info_t *buffer = PanicUnlessMalloc(sizeof(user_account_key_info_t));
        memcpy(buffer->account_key_index, account_key_index_value, sizeof(buffer->account_key_index));
        memcpy(buffer->account_keys, account_keys_value, sizeof(buffer->account_keys));
        memcpy(buf, buffer, sizeof(user_account_key_info_t));
        free(buffer);
    }
    else
    {
        DEBUG_LOG("user_accounts_serialise_persistent_device_data: Device_GetProperty(device_property_user_account_keys) fails ");
    }
}

static void user_accounts_deserialise_persistent_device_data(device_t device, void *buf, pdd_size_t data_length, pdd_size_t offset)
{
    UNUSED(offset);
    UNUSED(data_length);
    /* PS retrieve data to device database */
    user_account_key_info_t *buffer = (user_account_key_info_t *)buf;
    Device_SetProperty(device, device_property_user_account_key_index, &buffer->account_key_index, sizeof(buffer->account_key_index));
    Device_SetProperty(device, device_property_user_account_keys, &buffer->account_keys, sizeof(buffer->account_keys));

 }

static pdd_size_t user_accounts_get_device_data_len(device_t device)
{
    void *account_key_index_value = NULL;
    void *account_keys_value = NULL;
    size_t account_key_index_size = 0;
    size_t account_keys_size = 0;
    uint8 user_accounts_device_data_len = 0;

    if(DEVICE_TYPE_SELF == BtDevice_GetDeviceType(device))
    {
        if(Device_GetProperty(device, device_property_user_account_key_index, &account_key_index_value, &account_key_index_size) &&
           Device_GetProperty(device, device_property_user_account_keys, &account_keys_value, &account_keys_size))
        {
           user_accounts_device_data_len = account_key_index_size+account_keys_size;
           return user_accounts_device_data_len;
        }
    }
    return 0;
}


static void user_accounts_print_account_keys(uint16 num_keys, uint8* account_keys)
{
    DEBUG_LOG("user accounts : Number of account keys %d", num_keys);
    DEBUG_LOG("user accounts : Account keys : ");
    for(uint16 i=0; i<num_keys; i++)
    {
        DEBUG_LOG("%d) : ",i+1);
        DEBUG_LOG_DATA_DEBUG(&account_keys[i*MAX_USER_ACCOUNT_KEY_LEN], MAX_USER_ACCOUNT_KEY_LEN);
    }
}

/*! \brief Register User Accounts PDDU
 */
void UserAccounts_RegisterPersistentDeviceDataUser(void)
{
    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        PDDU_ID_USER_ACCOUNTS,
        user_accounts_get_device_data_len,
        user_accounts_serialise_persistent_device_data,
        user_accounts_deserialise_persistent_device_data);
}



/*! \brief  Get account keys by type
 */
uint16 UserAccounts_GetAccountKeys(uint8** account_keys, uint16 account_key_type)
{
    uint16 num_keys = 0;
    device_t my_device = BtDevice_GetSelfDevice();

    if(my_device)
    {
        void *account_key_index_value = NULL;
        void *account_keys_value = NULL;
        size_t account_key_index_size = 0;
        size_t account_keys_size = 0;
        if(Device_GetProperty(my_device, device_property_user_account_key_index, &account_key_index_value, &account_key_index_size) &&
            Device_GetProperty(my_device, device_property_user_account_keys, &account_keys_value, &account_keys_size))
        {
            user_account_key_info_t *buffer = PanicUnlessMalloc(sizeof(user_account_key_info_t));
            *account_keys = PanicUnlessMalloc(MAX_USER_ACCOUNT_KEY_LEN * MAX_NUM_ACCOUNT_KEYS);
            memcpy(buffer->account_key_index, account_key_index_value, sizeof(buffer->account_key_index));
            memcpy(buffer->account_keys, account_keys_value, sizeof(buffer->account_keys));


            /* Validate account keys stored in Account key Index */
            for(uint16 count=0;count<MAX_NUM_ACCOUNT_KEYS;count++)
            {
                if((buffer->account_key_index[count].num_handsets_linked > 0) && (buffer->account_key_index[count].account_key_type == account_key_type))
                {
                    memcpy(*account_keys+(num_keys*MAX_USER_ACCOUNT_KEY_LEN), &buffer->account_keys[count*MAX_USER_ACCOUNT_KEY_LEN], MAX_USER_ACCOUNT_KEY_LEN);
                    num_keys++;
                }
            }
            user_accounts_print_account_keys(num_keys, *account_keys);
            free(buffer);
        }
        else
        {
            /* No account keys were found return from here */
            DEBUG_LOG("User Accounts : Number of account keys %d", num_keys);
        }
    }
    else
    {
        DEBUG_LOG("User Accounts : Unexpected Error. Shouldn't have reached here");
    }
    return num_keys;
}

/*! \brief Get the user account keys
 */
uint16 UserAccounts_GetNumAccountKeys(void)
{
    uint16 num_keys = 0;
    device_t my_device = BtDevice_GetSelfDevice();

    if(my_device)
    {
        void *account_key_index_value = NULL;
        size_t account_key_index_size = 0;

        if(Device_GetProperty(my_device, device_property_user_account_key_index, &account_key_index_value, &account_key_index_size) &&
            account_key_index_size)
        {
            user_account_key_index_t account_key_index[MAX_NUM_ACCOUNT_KEYS];
            
            memcpy(&account_key_index[0], account_key_index_value, account_key_index_size);

            for(uint16 count=0;count<MAX_NUM_ACCOUNT_KEYS;count++)
            {
                if((account_key_index[count].num_handsets_linked > 0) && (account_key_index[count].account_key_type > user_account_type_invalid) &&
                    (account_key_index[count].account_key_type < user_account_type_max))
                {
                    num_keys++;
                }
            }

        }
    }
    DEBUG_LOG("User Accounts : Number of account keys %d", num_keys);
    return num_keys;
}

uint16 UserAccounts_GetMaxNumAccountKeys(void)
{
    return MAX_NUM_ACCOUNT_KEYS;
}

/*! \brief Get the user account key associated with the handset
 */
uint8* UserAccounts_GetAccountKeyWithHandset(uint16 account_key_len, const bdaddr *bd_addr)
{
    uint8 acc_key_index = INVALID_USER_ACCOUNT_KEY_INDEX;
    device_t handset_device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if(handset_device)
    {
        Device_GetPropertyU8(handset_device, device_property_handset_account_key_index, &acc_key_index);

        device_t my_device = BtDevice_GetSelfDevice();
        if(my_device && (acc_key_index >= 0 && acc_key_index < MAX_NUM_ACCOUNT_KEYS))
        {
            void *account_key_index_value = NULL;
            void *account_keys_value = NULL;
            size_t account_key_index_size = 0;
            size_t account_keys_size = 0;

            if(Device_GetProperty(my_device, device_property_user_account_key_index, &account_key_index_value, &account_key_index_size) &&
                Device_GetProperty(my_device, device_property_user_account_keys, &account_keys_value, &account_keys_size))
            {
                uint8* account_key = NULL;
                user_account_key_info_t *buffer = PanicUnlessMalloc(sizeof(user_account_key_info_t));

                memset(buffer, 0x00, sizeof(user_account_key_info_t));
                memcpy(buffer->account_key_index, account_key_index_value, sizeof(buffer->account_key_index));
                memcpy(buffer->account_keys, account_keys_value, sizeof(buffer->account_keys));

                if(buffer->account_key_index[acc_key_index].num_handsets_linked > 0)
                {
                    /* Memory allocation for the account key to be returned, this should be freed by the caller of this function after its usage */
                    account_key = (uint8*)PanicUnlessMalloc(account_key_len);
                    /* Copy the account key associated with the given account key index */
                    memcpy(account_key, &buffer->account_keys[acc_key_index * MAX_USER_ACCOUNT_KEY_LEN], account_key_len);
                }
                else
                {
                    DEBUG_LOG("User Accounts : No Account Key Linked with the given handset.");
                }
                free(buffer);
                return account_key;
            }
            else
            {
                DEBUG_LOG("User Accounts : Unexpected Error. Device_GetProperty failed to fetch the data.");
            }
        }
        else
        {
            DEBUG_LOG("User Accounts : Unexpected Error. No SELF device or No valid account key linked with the given handset");
        }
    }
    else
    {
        DEBUG_LOG("User Accounts : Unexpected Error. No handset device found for the given BD Addr");
    }
    return NULL;
}

/*! \brief Dissociate the user account key from a handset
 */
static bool userAccounts_DissociateAccountKeyFromHandset(device_t device, uint8 key_index, user_account_key_info_t *key_info)
{
    uint8 hs_key_index = UserAccounts_GetAccountKeyIndexWithHandset(device);

    if (hs_key_index == key_index)
    {
        DEBUG_LOG_INFO("userAccounts_DissociateAccountKeyFromHandset, device %p, index %u", device, key_index);

        /* Invalidate the property in the device and serialize to PS */
        Device_SetPropertyU8(device, device_property_handset_account_key_index, INVALID_USER_ACCOUNT_KEY_INDEX);

        PanicFalse(key_info->account_key_index[key_index].num_handsets_linked > 0);
        key_info->account_key_index[key_index].num_handsets_linked--;

        DeviceDbSerialiser_SerialiseDeviceLater(device, DEVICE_SERIALIZATION_DELAY_IN_MS);
        return TRUE;
    }

    return FALSE;
}

/*! \brief Dissociate the user account key from all handsets
 */
static void userAccounts_DissociateAccountKeyFromAllHandsets(uint8 key_index, user_account_key_info_t *key_info)
{
    DEBUG_LOG_INFO("userAccounts_DissociateAccountKeyFromAllHandsets, index %u", key_index);

    for (unsigned index = 0; index < DeviceList_GetMaxTrustedDevices(); index++)
    {
        device_t device = DeviceList_GetDeviceAtIndex(index);

        if (device && BtDevice_GetDeviceType(device) == DEVICE_TYPE_HANDSET)
        {
            userAccounts_DissociateAccountKeyFromHandset(device, key_index, key_info);
        }
    }
}

/*! \brief Get the number of handsets associated with this account key (index).
 */
static uint8 userAccounts_CountHandsetsAssociatedWithAccountKey(uint8 key_index)
{
    uint8 count = 0;

    for (unsigned index = 0; index < DeviceList_GetMaxTrustedDevices(); index++)
    {
        device_t device = DeviceList_GetDeviceAtIndex(index);

        if (UserAccounts_GetAccountKeyIndexWithHandset(device) == key_index)
        {
            count++;
        }
    }

    return count;
}

uint16 UserAccounts_AddAccountKeyForHandset(const bdaddr *bd_addr, const uint8* key, uint16 key_len, uint16 key_type)
{
    /* Validate key length. */
    if (!UserAccounts_KeyLengthIsValid(key_len))
    {
        DEBUG_LOG_ERROR("UserAccounts_AddAccountKeyForHandset, invalid account key length");
        return INVALID_USER_ACCOUNT_KEY_INDEX;
    }

    /* Validate key.
     * First byte of Fast Pair account keys must be 0x04, per specification */
    if (key == NULL || (key_type == user_account_type_fast_pair && key[0] != 0x04))
    {
        DEBUG_LOG_ERROR("UserAccounts_AddAccountKeyForHandset, invalid account key");
        return INVALID_USER_ACCOUNT_KEY_INDEX;
    }

    uint8 added_key_idx = INVALID_USER_ACCOUNT_KEY_INDEX;
    device_t self_device = PanicNull(BtDevice_GetSelfDevice());
    device_t handset_device = PanicNull(BtDevice_GetDeviceForBdAddr(bd_addr));

    if (self_device == NULL || handset_device == NULL)
    {
        DEBUG_LOG_ERROR("UserAccounts_AddAccountKeyForHandset, invalid SELF or handset");
        return INVALID_USER_ACCOUNT_KEY_INDEX;
    }

    DEBUG_LOG_INFO("UserAccounts_AddAccountKeyForHandset, storing:");
    DEBUG_LOG_DATA_INFO(key, key_len);

    void *key_index_value = NULL;
    void *keys_value = NULL;
    size_t key_index_size = 0;
    size_t keys_size = 0;

    user_account_key_info_t *buffer = PanicUnlessMalloc(sizeof(user_account_key_info_t));
    memset(buffer, 0, sizeof(*buffer));

    /* Populate the buffer if any account keys have been previously stored */
    if (Device_GetProperty(self_device, device_property_user_account_key_index, &key_index_value, &key_index_size) &&
        Device_GetProperty(self_device, device_property_user_account_keys, &keys_value, &keys_size))
    {
        memcpy(buffer->account_key_index, key_index_value, sizeof(buffer->account_key_index));
        memcpy(buffer->account_keys, keys_value, sizeof(buffer->account_keys));
    }

    /* If the handset already has any account key bound to it, dissociate it from that key,
     * as we assume (and require) that a handset cannot be bound to more than one account (key). */
    uint8 hs_key_idx = UserAccounts_GetAccountKeyIndexWithHandset(handset_device);

    if (hs_key_idx != INVALID_USER_ACCOUNT_KEY_INDEX)
    {
        userAccounts_DissociateAccountKeyFromHandset(handset_device, hs_key_idx, buffer);

        /* The slot will get sanitized below, if needed. */
    }

    bool already_exists = FALSE;
    uint8 unused_idx = INVALID_USER_ACCOUNT_KEY_INDEX;

    /* - Check if the new account key already exists in our list.
     * - Take note of any unused slots */
    for (uint8 idx = 0; idx < MAX_NUM_ACCOUNT_KEYS; idx++)
    {
        user_account_key_index_t *curr_index = &buffer->account_key_index[idx];
        uint8 *curr_key = &buffer->account_keys[idx * MAX_USER_ACCOUNT_KEY_LEN];

        /* For Fast Pair, this indicates a Subsequent Pairing.
         * Corner-case: A handset which re-bonds using the same account key as before
         * is not strictly a Subsequent Pairing, but consider it so for convenience. */
        if (curr_index->account_key_len == key_len && memcmp(key, curr_key, key_len) == 0)
        {
            already_exists = TRUE;

            curr_index->num_handsets_linked++;
            added_key_idx = idx;
            break;
        }

        /* Update the linked handsets count, in case some handsets have since been deleted. */
        if (curr_index->num_handsets_linked != 0)
        {
            curr_index->num_handsets_linked = userAccounts_CountHandsetsAssociatedWithAccountKey(idx);
            DEBUG_LOG("UserAccounts_AddAccountKeyForHandset, idx=%u, linked=%u", idx, curr_index->num_handsets_linked);
        }

        if (curr_index->num_handsets_linked == 0)
        {
            /* Sanitize unused slots */
            if (curr_index->account_key_len != 0)
            {
                DEBUG_LOG_INFO("UserAccounts_AddAccountKeyForHandset, slot cleared as no linked handsets, idx=%u", idx);
                memset(curr_index, 0, sizeof(*curr_index));
                memset(curr_key, 0, MAX_USER_ACCOUNT_KEY_LEN);
            }

            /* Record only the first unused slot */
            unused_idx = (unused_idx == INVALID_USER_ACCOUNT_KEY_INDEX) ? idx : unused_idx;
        }
    }

    if (already_exists)
    {
        Device_SetProperty(self_device, device_property_user_account_key_index, &buffer->account_key_index, sizeof(buffer->account_key_index));
        Device_SetProperty(self_device, device_property_user_account_keys, &buffer->account_keys, sizeof(buffer->account_keys));
        Device_SetPropertyU8(handset_device, device_property_handset_account_key_index, added_key_idx);

        free(buffer);
        DeviceDbSerialiser_Serialise();

        DEBUG_LOG_INFO("UserAccounts_AddAccountKeyForHandset, already exists, idx=%u", added_key_idx);
        return added_key_idx;
    }

    /* If there are no free slots, delete a key always to make room.
     * Preferably, delete the LRU key, if one could be derived. Otherwise, just delete the last key in the list.
     * The user should always be able to add new keys, even at the cost of losing existing keys. */
    if (unused_idx == INVALID_USER_ACCOUNT_KEY_INDEX)
    {
        unused_idx = UserAccounts_GetLruAccountKeyIndex();
        unused_idx = unused_idx == (INVALID_USER_ACCOUNT_KEY_INDEX) ? MAX_NUM_ACCOUNT_KEYS - 1 : unused_idx;
        DEBUG_LOG_INFO("UserAccounts_AddAccountKeyForHandset, deleting key %u", unused_idx);

        userAccounts_DissociateAccountKeyFromAllHandsets(unused_idx, buffer);
        memset(&buffer->account_key_index[unused_idx], 0, sizeof(user_account_key_index_t));
        memset(&buffer->account_keys[unused_idx * MAX_USER_ACCOUNT_KEY_LEN], 0, MAX_USER_ACCOUNT_KEY_LEN);
    }

    buffer->account_key_index[unused_idx].num_handsets_linked = 1;
    buffer->account_key_index[unused_idx].account_key_type = key_type;
    buffer->account_key_index[unused_idx].account_key_len = key_len;
    buffer->account_key_index[unused_idx].account_key_start_loc = unused_idx * MAX_USER_ACCOUNT_KEY_LEN;
    memcpy(&buffer->account_keys[unused_idx * MAX_USER_ACCOUNT_KEY_LEN], key, key_len);

    added_key_idx = unused_idx;
    Device_SetProperty(self_device, device_property_user_account_key_index, &buffer->account_key_index, sizeof(buffer->account_key_index));
    Device_SetProperty(self_device, device_property_user_account_keys, &buffer->account_keys, sizeof(buffer->account_keys));
    Device_SetPropertyU8(handset_device, device_property_handset_account_key_index, added_key_idx);

    free(buffer);
    DeviceDbSerialiser_Serialise();
    DEBUG_LOG_INFO("UserAccounts_AddAccountKeyForHandset, added new key %u", added_key_idx);

    return added_key_idx;
}

uint8 UserAccounts_GetAccountKeyIndex(const uint8* account_key, uint16 account_key_len)
{
    uint8 key_index = INVALID_USER_ACCOUNT_KEY_INDEX;
    device_t self_device = BtDevice_GetSelfDevice();

    if (self_device)
    {
        void *account_keys_value = NULL;
        size_t account_keys_size = 0;

        user_account_key_info_t *buffer = PanicUnlessMalloc(sizeof(user_account_key_info_t));
        memset(buffer, 0x00, sizeof(user_account_key_info_t));

        if (Device_GetProperty(self_device, device_property_user_account_keys, &account_keys_value, &account_keys_size))
        {
            memcpy(buffer->account_keys, account_keys_value, sizeof(buffer->account_keys));

            for (uint8 index = 0; index < MAX_NUM_ACCOUNT_KEYS; index++)
            {
                if (memcmp(account_key, &buffer->account_keys[index*MAX_USER_ACCOUNT_KEY_LEN], account_key_len) == 0)
                {
                    key_index = index;
                    break;
                }
            }
        }

        free(buffer);
    }

    return key_index;
}

uint8 UserAccounts_GetAccountKeyIndexWithHandset(device_t device)
{
    uint8 hs_key_index = INVALID_USER_ACCOUNT_KEY_INDEX;

    if (device && BtDevice_GetDeviceType(device) == DEVICE_TYPE_HANDSET)
    {
        Device_GetPropertyU8(device, device_property_handset_account_key_index, &hs_key_index);
    }

    return hs_key_index;
}

static bool userAccounts_GetMruHandsetForAccountKey(uint8 key_index, uint8 *tdl_index)
{
    device_t device = NULL;

    if (key_index == INVALID_USER_ACCOUNT_KEY_INDEX)
    {
        return FALSE;
    }

    /* Traverse the TDL in MRU order */
    for (uint8 index = 0; index < DeviceList_GetMaxTrustedDevices(); index++)
    {
        if (BtDevice_GetIndexedDevice(index, &device) && BtDevice_GetDeviceType(device) == DEVICE_TYPE_HANDSET)
        {
            uint8 hs_key_index = UserAccounts_GetAccountKeyIndexWithHandset(device);

            if (hs_key_index == key_index)
            {
                *tdl_index = index;
                return TRUE;
            }
        }
    }

    return FALSE;
}

uint8 UserAccounts_GetLruAccountKeyIndex(void)
{
    uint8 lru_key_index = INVALID_USER_ACCOUNT_KEY_INDEX;
    uint8 max_tdl_index_seen = 0;

    for (uint8 key_index = 0; key_index < MAX_NUM_ACCOUNT_KEYS; key_index++)
    {
        uint8 tdl_index = 0;

        /* Find out the last time (i.e. handset) when this account key was used */
        if (userAccounts_GetMruHandsetForAccountKey(key_index, &tdl_index))
        {
            /* Keep a running maximum i.e. a running LRU */
            if (tdl_index >= max_tdl_index_seen)
            {
                max_tdl_index_seen = tdl_index;
                lru_key_index = key_index;
            }
        }
    }

    DEBUG_LOG("UserAccounts_GetLruAccountKeyIndex, lru=%u", lru_key_index);

    return lru_key_index;
}

/*! \brief Store the Fast Pair account keys with the index values
 */
bool UserAccounts_StoreAllAccountKeys(account_key_sync_req_t *account_key_info)
{
    bool result = FALSE;

    /* Find the SELF device to add account keys to */
    device_t my_device = BtDevice_GetSelfDevice();
    if(my_device)
    {
        DEBUG_LOG("User Accounts : Storing the complete account key info.");
        Device_SetProperty(my_device, device_property_user_account_key_index, &account_key_info->account_key_index, sizeof(account_key_info->account_key_index));
        Device_SetProperty(my_device, device_property_user_account_keys, &account_key_info->account_keys, sizeof(account_key_info->account_keys));

        result = TRUE;
        DeviceDbSerialiser_Serialise();
    }
    return result;
}

/*! \brief Delete all user account keys
 */
bool UserAccounts_DeleteAllAccountKeys(void)
{
    bool result = FALSE;
    device_t my_device = BtDevice_GetSelfDevice();

    DEBUG_LOG("User Accounts : Delete all account keys");
    if(my_device)
    {
        void *account_key_index_value = NULL;
        void *account_keys_value = NULL;
        size_t account_key_index_size = 0;
        size_t account_keys_size = 0;
        if(Device_GetProperty(my_device, device_property_user_account_key_index, &account_key_index_value, &account_key_index_size) &&
            Device_GetProperty(my_device, device_property_user_account_keys, &account_keys_value, &account_keys_size))
        {
            user_account_key_info_t *buffer = PanicUnlessMalloc(sizeof(user_account_key_info_t));
            memset(buffer, 0x00, sizeof(user_account_key_info_t));

            Device_SetProperty(my_device, device_property_user_account_key_index, &buffer->account_key_index, sizeof(buffer->account_key_index));
            Device_SetProperty(my_device, device_property_user_account_keys, &buffer->account_keys, sizeof(buffer->account_keys));

            free(buffer);
            DeviceDbSerialiser_Serialise();
            result = TRUE;
        }
    }
    else
    {
        DEBUG_LOG("User Accounts : Unexpected Error. Shouldn't have reached here");
    }
    return result;
}

