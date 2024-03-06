/*!
\copyright  Copyright (c) 2008 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Bloom filter on-chip testing with Google supplied test data
*/
#ifndef FAST_PAIR_BLOOM_FILTER_TEST_H_
#define FAST_PAIR_BLOOM_FILTER_TEST_H_

#ifdef FP_USE_LOCAL_DATA_FOR_DEBUG

#undef DEBUG_LOG_FP_BLOOM
#define DEBUG_LOG_FP_BLOOM  DEBUG_LOG_ALWAYS
#undef DEBUG_LOG_FP_BLOOM_DATA
#define DEBUG_LOG_FP_BLOOM_DATA DEBUG_LOG_DATA_ALWAYS

#define LOCAL_DEBUG_NUMBER_TEST_KEYS                    2
#define LOCAL_DEBUG_RANDOM_RESOLVABLE_DATA_SIZE         5 // 0 or 5
#define LOCAL_DEBUG_BATTERY_DATA_SIZE                   0 // 0 or 4
#define LOCAL_DEBUG_SALT                                0xC7C8

static uint16 fastPair_GetTestAccountKeys(uint8 ** keys, uint16 account_key_type)
{
    UNUSED(account_key_type);
    const uint8 account_keys[ACCOUNT_KEY_LEN * 2] = 
    {       
        //0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        //0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88,
        //0x99, 0x99, 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC, 0xDD, 0xDD, 0xEE, 0xEE, 0xFF, 0xFF, 0xDE, 0xAD,  
        0x06, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        0x04, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88,
    };
    *keys = PanicUnlessMalloc(ACCOUNT_KEY_LEN * LOCAL_DEBUG_NUMBER_TEST_KEYS);
    memcpy(*keys, account_keys, ACCOUNT_KEY_LEN * LOCAL_DEBUG_NUMBER_TEST_KEYS);
    DEBUG_LOG_FP_BLOOM("fastPair_GetTestAccountKeys num_keys=%d", LOCAL_DEBUG_NUMBER_TEST_KEYS);
    DEBUG_LOG_FP_BLOOM_DATA(*keys, (ACCOUNT_KEY_LEN * LOCAL_DEBUG_NUMBER_TEST_KEYS));
    return LOCAL_DEBUG_NUMBER_TEST_KEYS;
}

static uint8 local_debug_random_resolvable_data[] = //{ 0x46, 0xF4, 0xBB, 0x40, 0x6F }; // 0x46 LT
                                                    { 0x46, 0x6E, 0xBC, 0xCB, 0x21 };
static uint8 local_debug_battery_data[] = { 0x33, 0x40, 0x40, 0x40 }; // 0x33 LT

#undef fastPair_GetAccountKeys
#define fastPair_GetAccountKeys(keys,account_type)      fastPair_GetTestAccountKeys(keys, account_type)
#undef fastPair_GetRandomResolvableData
#define fastPair_GetRandomResolvableData()              local_debug_random_resolvable_data
#undef fastPair_GetRandomResolvableDataSize
#define fastPair_GetRandomResolvableDataSize()          LOCAL_DEBUG_RANDOM_RESOLVABLE_DATA_SIZE
#undef fastPair_GenerateSalt
#define fastPair_GenerateSalt()                         LOCAL_DEBUG_SALT
#undef fastPair_GetBatteryData
#define fastPair_GetBatteryData()                       local_debug_battery_data
#undef fastPair_GetBatteryDataSize
#define fastPair_GetBatteryDataSize()                   LOCAL_DEBUG_BATTERY_DATA_SIZE

#endif
#endif