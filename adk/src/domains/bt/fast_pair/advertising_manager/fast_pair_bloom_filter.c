/*!
\copyright  Copyright (c) 2008 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Handles Fast Pair Bloom Filter generation
*/

#ifndef USE_FAST_PAIR_ACCOUNT_DATA_V0

/*! Firmware and Library Headers */
#include <util.h>
#include <stdlib.h>
#include <message.h>
#include <logging.h>
#include <panic.h>
#include <stdio.h>
#include <connection_manager.h>
#include <string.h>
#include <cryptovm.h>
#include <connection.h>
#include <connection_no_ble.h>
#include "domain_message.h"

/*! Application Headers */
#include "fast_pair_advertising.h"
#include "fast_pair_bloom_filter.h"
#include "fast_pair_session_data.h"
#include "fast_pair.h"
#include "fast_pair_battery_notifications.h"
#include "fast_pair_adv_sass.h"
#include "user_accounts.h"

#define DEBUG_LOG_FP_BLOOM  DEBUG_LOG
#define DEBUG_LOG_FP_BLOOM_DATA DEBUG_LOG_DATA

#define NO_BLOOM_FILTER_LEN                 (0)

#define FP_ACCOUNT_DATA_VERSION_AND_FLAGS_SIZE          1
#define FP_ACCOUNT_DATA_VERSION                         0x01
#define FP_ACCOUNT_DATA_FLAGS                           0x00
#define FP_ACCOUNT_DATA_VERSION_AND_FLAGS               (((FP_ACCOUNT_DATA_VERSION << 4) & 0xF0) | (FP_ACCOUNT_DATA_FLAGS & 0x0F)) 

#define FP_ACCOUNT_KEY_LENGTHTYPE_SIZE      (1)

#define FP_ACCOUNT_KEY_TYPE_UI_SHOW         (0x0)
#define FP_ACCOUNT_KEY_TYPE_UI_HIDE         (0x2)
#define ACCOUNT_KEY_LEN                     (16)

#define FP_SALT_LENGTHTYPE_SIZE             (1)
#define FP_SALT_TYPE                        (1)
#define FP_SALT_SIZE                        (2)
#define FP_SALT_LENGTHTYPE                  ((FP_SALT_SIZE << 4) + FP_SALT_TYPE)

#define FP_RANDOM_RESOLVABLE_DATA_LENGTHTYPE_SIZE   1
#define FP_RANDOM_RESOLVABLE_DATA_TYPE              6  
#define FP_RANDOM_RESOLVABLE_DATA_SIZE              4

#define FP_ACCOUNT_DATA_START_POS (FP_ACCOUNT_DATA_VERSION_AND_FLAGS_SIZE + FP_ACCOUNT_KEY_LENGTHTYPE_SIZE)

#define SHA256_INPUT_ARRAY_LENGTH_MAX   (ACCOUNT_KEY_LEN + FP_SALT_SIZE + FP_BATTERY_NOTFICATION_SIZE + FP_RANDOM_RESOLVABLE_DATA_SIZE)

/*! \brief Global data structure for fastpair bloom filter generation */
typedef struct
{
    uint8 bloom_filter_len;
    uint8 bloom_filter_ready_len;
    bool data_generation_in_progress;
    uint8 *bloom_filter_generating;/*Buffer used to hold bloom filter while it is getting generated */
    uint8* bloom_filter_ready;/*Buffer used to hold ready bloom filter data*/
    uint16 num_account_keys;
    uint8 *account_keys;/*allocated everytime fastPair_GetAccountKeys() is called*/
    uint8 filter_size;/*Value S*/
    uint8 number_of_keys_processed;
    uint8 temp[SHA256_INPUT_ARRAY_LENGTH_MAX];
    uint16 salt;
    bool needs_regenerating;
}fastpair_bloom_filter_data_t;

/*! Global Instance of fastpair bloom filter data */
fastpair_bloom_filter_data_t fastpair_bloom_filter;

#define fastPair_GetAccountKeys(keys,account_type)      UserAccounts_GetAccountKeys(keys, account_type)
#define fastPair_GetRandomResolvableData()              fastPair_SASSGetAdvData()
#define fastPair_GetRandomResolvableDataSize()          fastPair_SASSGetAdvDataSize()
#define fastPair_GenerateSalt()                         (UtilRandom() & 0xFFFF)
#define fastPair_GetBatteryData()                       fastPair_BatteryGetData()
#define fastPair_GetBatteryDataSize()                   FP_BATTERY_NOTFICATION_SIZE

#ifdef FP_USE_LOCAL_DATA_FOR_DEBUG
#include "fast_pair_bloom_filter_test.h"
#endif

/*! \brief Function to send SHA 256 hash data request to help with generation of bloom filter
*/
static void fastPair_HashData(uint8 * input_array, uint8 input_array_length)
{

    DEBUG_LOG_FP_BLOOM("fastPair_HashData");

    uint8 *little_endian_input_array = PanicUnlessMalloc(input_array_length);

    /* Convert the big endian input data to little endian before processing it for Hashing */
    fastPair_ConvertEndiannessFormat(input_array, input_array_length, little_endian_input_array);

#ifdef USE_SYNERGY
    CmCryptoHashReqSend(fastPair_AdvGetAdvTask(), (uint16 *)little_endian_input_array, input_array_length);
#else
    ConnectionEncryptBlockSha256(fastPair_AdvGetAdvTask(), (uint16 *)little_endian_input_array, input_array_length);
#endif /*! USE_SYNERGY */

    free(little_endian_input_array);
}

static inline uint8 fastPair_GetHashInputSize(void)
{
    return (ACCOUNT_KEY_LEN + FP_SALT_SIZE + fastPair_GetBatteryDataSize() + fastPair_GetRandomResolvableDataSize());
}

static void fastPair_HashAccountKey(uint8 key_index)
{
    PanicFalse(key_index < fastpair_bloom_filter.num_account_keys);
    DEBUG_LOG_FP_BLOOM("fastPair_HashAccountKey key_index=%d hash_size=%d", key_index, fastPair_GetHashInputSize());
    memcpy(fastpair_bloom_filter.temp, &fastpair_bloom_filter.account_keys[key_index * ACCOUNT_KEY_LEN], ACCOUNT_KEY_LEN);
    DEBUG_LOG_FP_BLOOM_DATA(fastpair_bloom_filter.temp, fastPair_GetHashInputSize());
    fastPair_HashData(fastpair_bloom_filter.temp, fastPair_GetHashInputSize());
}

static inline void fastPair_HashNextAccountKey(void)
{
    fastPair_HashAccountKey(fastpair_bloom_filter.number_of_keys_processed++);
}

static inline void fastPair_StartAccountKeyHashing(uint16 salt)
{
    /* build input data to hash first account key */
    // this is 'V'
    memset(fastpair_bloom_filter.temp, 0, SHA256_INPUT_ARRAY_LENGTH_MAX);

    uint8 index = ACCOUNT_KEY_LEN;
    fastpair_bloom_filter.temp[index++] = (salt >> 8) & 0xFF;
    fastpair_bloom_filter.temp[index++] = (salt) & 0xFF;
    DEBUG_LOG_FP_BLOOM("fastPair_StartAccountKeyHashing salt=0x%x", salt);
    uint8 battery_data_size = fastPair_GetBatteryDataSize();
    if (battery_data_size)
    {
        memcpy(&fastpair_bloom_filter.temp[index], fastPair_GetBatteryData(), battery_data_size);
        DEBUG_LOG_FP_BLOOM("fastPair_StartAccountKeyHashing battery_data_size=%d", battery_data_size);
        DEBUG_LOG_FP_BLOOM_DATA(&fastpair_bloom_filter.temp[index], battery_data_size);
        index += battery_data_size;
    }
    uint8 random_resolvable_data_size = fastPair_GetRandomResolvableDataSize();
    if(random_resolvable_data_size)
    {
        memcpy(&fastpair_bloom_filter.temp[index], fastPair_GetRandomResolvableData(), random_resolvable_data_size);
        DEBUG_LOG_FP_BLOOM("fastPair_StartAccountKeyHashing randon_resolvable_data_size=%d", random_resolvable_data_size);
        DEBUG_LOG_FP_BLOOM_DATA(&fastpair_bloom_filter.temp[index], random_resolvable_data_size);
    }

    fastpair_bloom_filter.number_of_keys_processed = 0;
    fastPair_HashAccountKey(fastpair_bloom_filter.number_of_keys_processed);
}

static inline uint8 fastPair_CalculateBloomFilterSize(uint8 number_of_account_keys)
{
    return (number_of_account_keys ? (((number_of_account_keys * 6) / 5) + 3) : 0);
}

static inline uint8 fastPair_CalculateAccountKeyDataSize(uint8 bloom_filter_size)
{
    return (bloom_filter_size ? (FP_ACCOUNT_DATA_VERSION_AND_FLAGS_SIZE
                                                + FP_ACCOUNT_KEY_LENGTHTYPE_SIZE
                                                + bloom_filter_size 
                                                + FP_SALT_LENGTHTYPE_SIZE 
                                                + FP_SALT_SIZE) : 0);
}

static inline uint8 fastPair_GetAccountKeyLengthAndType(uint8 bloom_filter_size)
{
    /* Build lengthtype field
        - always include size of the filter
        - if still in the case then hide the pairing prompt on the handset
        - if out of the case then show the pairing prompt
    */
    return ((bloom_filter_size << 4) & 0xF0) | FP_ACCOUNT_KEY_TYPE_UI_SHOW;
}

/*! \brief Helper function to generate account key data.

    \param acc_keys[in] Pointer to an array of account key data.
    \param bloom_filter[in,out] Pointer to array in which to build the bloom filter.
    \return uint8 Size of the account key data in bytes.

    \note If the bloom_filter parameter is NULL just returns the size of the
          account key data and doesn't start a new bloom filter generation process.
 */
static void fastPair_CalculateAccountKeyData(uint16 salt)
{
    DEBUG_LOG_FP_BLOOM("fastPair_CalculateAccountKeyData");

    if(fastpair_bloom_filter.num_account_keys)
    {
        fastpair_bloom_filter.filter_size = fastPair_CalculateBloomFilterSize(fastpair_bloom_filter.num_account_keys);
        fastpair_bloom_filter.bloom_filter_len = fastPair_CalculateAccountKeyDataSize(fastpair_bloom_filter.filter_size);

        DEBUG_LOG_FP_BLOOM("fastPair_CalculateAccountKeyData filter_size=%d bloom_filter_length=%d salt=0x%x", fastpair_bloom_filter.filter_size, fastpair_bloom_filter.bloom_filter_len, salt);

        if (fastpair_bloom_filter.bloom_filter_len)
        {
            fastpair_bloom_filter.bloom_filter_generating = PanicUnlessMalloc(fastpair_bloom_filter.bloom_filter_len);

            // this is account key data
            memset(fastpair_bloom_filter.bloom_filter_generating, 0, fastpair_bloom_filter.bloom_filter_len);
            uint8 index = 0;
            fastpair_bloom_filter.bloom_filter_generating[index++] = FP_ACCOUNT_DATA_VERSION_AND_FLAGS;
            fastpair_bloom_filter.bloom_filter_generating[index++] = fastPair_GetAccountKeyLengthAndType(fastpair_bloom_filter.filter_size);

            // bloom filter to be populated through hashing
            index += fastpair_bloom_filter.filter_size;

            // salt
            fastpair_bloom_filter.bloom_filter_generating[index++] = FP_SALT_LENGTHTYPE; 
            fastpair_bloom_filter.bloom_filter_generating[index++] = ((salt >> 8) & 0xFF);
            fastpair_bloom_filter.bloom_filter_generating[index++] = (salt & 0xFF);

            fastPair_StartAccountKeyHashing(salt);
        }
    }
}

static void fastPair_RandomResolvableDataEncrypted(bool success)
{
    UNUSED(success);
    fastPair_CalculateAccountKeyData(fastpair_bloom_filter.salt);
}

/*! @brief Helper function to release account key memory
 */
static void fastPairReleaseAccountKeys(void)
{
    if (fastpair_bloom_filter.account_keys)
    {
        DEBUG_LOG_FP_BLOOM("fastPairReleaseAccountKeys\n");
        free(fastpair_bloom_filter.account_keys);
        fastpair_bloom_filter.account_keys=NULL;
        fastpair_bloom_filter.num_account_keys=0;
    }
}

/*! @brief Helper function to release bloom filter memory
 */
static void fastPairReleaseBloomFilter(void)
{
    if (fastpair_bloom_filter.bloom_filter_generating)
    {
        DEBUG_LOG_FP_BLOOM("fastPairReleaseBloomFilter\n");
        free(fastpair_bloom_filter.bloom_filter_generating);
        fastpair_bloom_filter.bloom_filter_generating=NULL;
        fastpair_bloom_filter.bloom_filter_len=NO_BLOOM_FILTER_LEN;
    }
}


/*! \brief Function to initialise the fastpair bloom filter generation globals
*/
void fastPair_InitBloomFilter(void)
{
    memset(&fastpair_bloom_filter, 0, sizeof(fastpair_bloom_filter_data_t));
}

/*! @brief Function to generate bloom filter

      Account key filter or Bloom filter is used in adverts in unidentifable or BR/EDR non-discoverable mode only.
      
      The following are the trigger to generate bloom filter
      - During fastpair advertising init, to be ready with the data
      - During addition of new account key
      - During Adv Mgr callback to get Item, this is to ensure new bloom filter is ready with a new Salt.
      - Deletion of account keys is taken care through the session data API
      - Internal timer expiry to track Salt change in Account key Filter  
      - Receipt of new battery status information from the case
      - When in-use account key potentially changes as indicated by FP seeker in messsage stream
 */
void fastPair_GenerateBloomFilter(void)
{
    DEBUG_LOG_FP_BLOOM("fastPair_GenerateBloomFilter: data_generation_in_progress=%d", fastpair_bloom_filter.data_generation_in_progress);
    if(!fastpair_bloom_filter.data_generation_in_progress)
    {
        /*Housekeeping*/
        fastPairReleaseAccountKeys();
        fastPairReleaseBloomFilter();

        /*Start with checking the num account keys*/
        fastpair_bloom_filter.num_account_keys = fastPair_GetAccountKeys(&fastpair_bloom_filter.account_keys, user_account_type_fast_pair);
        
        if (fastpair_bloom_filter.num_account_keys)
        {
            bool in_use_account_key_handset_connected_and_active = FALSE;
            uint8* in_use_account_key = fastPair_SASSGetInUseAccountKey(&in_use_account_key_handset_connected_and_active);

            if(in_use_account_key != NULL)
            {
                /* Find the in-use/MRU account key index */
                for(uint16 count = 0; count < fastpair_bloom_filter.num_account_keys; count++)
                {
                    if(memcmp(in_use_account_key, &fastpair_bloom_filter.account_keys[count * ACCOUNT_KEY_LEN], ACCOUNT_KEY_LEN) == 0)
                    {
                        /* If there is an account key in use, just modify the in-use account key first octet from 0x04 to 0x06, 
                           else modify the Most Recently Used Account Key first octet from 0x04 to 0x05 */
                        DEBUG_LOG_FP_BLOOM("fastPair_GenerateBloomFilter: In Use account key first byte modified. Is in-use account key handset connected and active=%d",
                                   in_use_account_key_handset_connected_and_active);
                        if(in_use_account_key_handset_connected_and_active)
                        {
                            fastpair_bloom_filter.account_keys[count * ACCOUNT_KEY_LEN] = 0x06;
                        }
                        else
                        {
                            fastpair_bloom_filter.account_keys[count * ACCOUNT_KEY_LEN] = 0x05;
                        }
                        break;
                    }
                }
            }
            fastpair_bloom_filter.salt = fastPair_GenerateSalt();
            fastpair_bloom_filter.data_generation_in_progress = TRUE;
            fastpair_bloom_filter.needs_regenerating = FALSE;

            fastPair_SASSUpdateAdvPayload(fastpair_bloom_filter.salt, fastPair_RandomResolvableDataEncrypted);
        }
    }
    else
    {
        fastpair_bloom_filter.needs_regenerating++;
    }
    DEBUG_LOG_FP_BLOOM("fastPair_GenerateBloomFilter: needs_regenerating=%d", fastpair_bloom_filter.needs_regenerating);
}


/*! @brief Private API to return the generated bloom filter length
 */
uint8 fastPairGetBloomFilterLen(void)
{
    DEBUG_LOG_FP_BLOOM("FP Bloom Filter Len: %d \n", fastpair_bloom_filter.bloom_filter_ready_len);
    return fastpair_bloom_filter.bloom_filter_ready_len;
}

/*! @brief Private API to return the generated bloom filter 
 */
uint8* fastPairGetBloomFilterData(void)
{
    return fastpair_bloom_filter.bloom_filter_ready;
}


/*! @brief Private API to handle CRYPTO_HASH_CFM for Bloom filter generation
 */
#ifdef USE_SYNERGY
void fastPair_AdvHandleHashCfm(CsrBtCmCryptoHashCfm *cfm)
#else
void fastPair_AdvHandleHashCfm(CL_CRYPTO_HASH_CFM_T *cfm)
#endif /*! USE_SYNERGY */
{
    uint16 result;
#ifdef USE_SYNERGY
    result = cfm->resultCode;
#else
    result = cfm->status;
#endif /*! USE_SYNERGY */

    /* Bloom filter byte ordering is as below 
    | 0x00 |0b LLLL   TTTT   |0x--    0x--| ob LLLL   TTTT  |  0x   |
    |  FFU |   LACF   TYPEA  |   VLACF... |    LSF    TYPES.|..SALT |

    FFU --> For future use. Should be 0x00
    LACF --> Length of account key filter in bytes. Should be 0x00 if account key filter is empty
    TYPEA --> Type of account key filter. Should be 0x0
    VLACF --> Variable length account key filter
    LSF --> Length of salt filed in bytes i.e. 0x1
    TYPES --> Type of salt.Should be 0x1 if salt is used
    SALT --> Salt data */

    /* Allocated bloom filter buffer holds data from fields LACF to SALT.
    Generated bloom filter to be copied from field VLACF. Assign VLCAF index i.e. 1 to temporary place holder */

    if ((fastpair_bloom_filter.bloom_filter_generating) && result == success)
    {
        uint8* key_filter_ptr = &fastpair_bloom_filter.bloom_filter_generating[FP_ACCOUNT_DATA_START_POS];
        for(uint8 index = 0; index < ACCOUNT_KEY_LEN; index += 2)
        {
            uint32 hash_value  = ((uint32) cfm->hash[index] & 0x00FF) << 24;
            hash_value |= ((uint32) cfm->hash[index] & 0xFF00) << 8;
            hash_value |= ((uint32) cfm->hash[index + 1] & 0x00FF) << 8;
            hash_value |= ((uint32) cfm->hash[index + 1] & 0xFF00) >> 8;

            uint32 hash_account_key = hash_value % (fastpair_bloom_filter.filter_size * 8);
            key_filter_ptr[hash_account_key/ 8] |= (1 << (hash_account_key % 8));
        }

        fastpair_bloom_filter.number_of_keys_processed++;
        DEBUG_LOG_FP_BLOOM("fastPair_AdvHandleHashCfm success, keys_processed=%d", fastpair_bloom_filter.number_of_keys_processed);
        if(fastpair_bloom_filter.number_of_keys_processed < fastpair_bloom_filter.num_account_keys)
        {
            fastPair_HashAccountKey(fastpair_bloom_filter.number_of_keys_processed);
        }
        else
        {
            /*Account Key Filter generated successfully*/
            if(!fastpair_bloom_filter.bloom_filter_ready)
            {
                fastpair_bloom_filter.bloom_filter_ready = PanicUnlessMalloc(fastpair_bloom_filter.bloom_filter_len);
            }
            else
            {
                uint8* buf = fastpair_bloom_filter.bloom_filter_ready;
                fastpair_bloom_filter.bloom_filter_ready = PanicNull(realloc(buf, fastpair_bloom_filter.bloom_filter_len));
            }
            
            memcpy(fastpair_bloom_filter.bloom_filter_ready, fastpair_bloom_filter.bloom_filter_generating, fastpair_bloom_filter.bloom_filter_len);
            fastpair_bloom_filter.bloom_filter_ready_len = fastpair_bloom_filter.bloom_filter_len;
            fastPairReleaseAccountKeys();
            fastpair_bloom_filter.data_generation_in_progress = FALSE;
            DEBUG_LOG_FP_BLOOM("fastPair_AdvHandleHashCfm complete, size=%d", fastpair_bloom_filter.bloom_filter_ready_len);
            DEBUG_LOG_FP_BLOOM_DATA(fastpair_bloom_filter.bloom_filter_ready, fastpair_bloom_filter.bloom_filter_ready_len);

            if(!fastpair_bloom_filter.needs_regenerating)
            {
                fastPair_AdvNotifyDataChange();
            }
        }
    }
    else
    {
        DEBUG_LOG_FP_BLOOM("fastPair_AdvHandleHashCfm failure");
        fastPairReleaseAccountKeys();
        fastPairReleaseBloomFilter();
        fastpair_bloom_filter.data_generation_in_progress = FALSE;
    }

    if(!fastpair_bloom_filter.data_generation_in_progress && fastpair_bloom_filter.needs_regenerating)
    {
        fastPair_GenerateBloomFilter();
    }
}


/*! @brief Private API to handle new account key addition
 */
void fastPair_AccountKeysModified(void)
{
    /*Generate new bloom filter and keep it ready for advertisements in BR/EDR Connectable and non-discoverable mode*/
    DEBUG_LOG_FP_BLOOM("fastPair_AccountKeysModified: fastPair_GenerateBloomFilter\n");
    fastPair_GenerateBloomFilter();
}

#endif
