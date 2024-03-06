/*!
\copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       earbud_pmalloc_pools.c
\brief      Definition of the pmalloc pools used by the earbud application.

The pools defined here will be merged at run time with the base definitions
from OS - see 'pmalloc_config_P1.h'.
*/

#include "le_unicast_pools.h"
#include <pmalloc.h>

_Pragma ("unitsuppress Unused")

_Pragma ("datasection apppool")

#ifdef USE_SYNERGY

/* Additional pools required when TDL is extended to 12 devices */
#define EXTENDED_TDL_BLOCK_MIN_SIZE_8_ADDON_COUNT     12
#define EXTENDED_TDL_BLOCK_MIN_SIZE_16_ADDON_COUNT    46
#define EXTENDED_TDL_BLOCK_MIN_SIZE_20_ADDON_COUNT     6
#define EXTENDED_TDL_BLOCK_MIN_SIZE_24_ADDON_COUNT     6
#define EXTENDED_TDL_BLOCK_MIN_SIZE_28_ADDON_COUNT    40
#define EXTENDED_TDL_BLOCK_MIN_SIZE_36_ADDON_COUNT     6

#if defined(INCLUDE_EXTENDED_TDL) || defined(INCLUDE_EXTENDED_TDL_DB_SERIALISER)
#define EXTENDED_TDL_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(size) EXTENDED_TDL_BLOCK_MIN_SIZE_##size##_ADDON_COUNT
#else
#define EXTENDED_TDL_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(size) 0
#endif

/* Additional pools required for Accessory tracking support */
#define ACCESSORY_TRACKING_BLOCK_MIN_SIZE_128_ADDON_COUNT     40
#define ACCESSORY_TRACKING_BLOCK_MIN_SIZE_1400_ADDON_COUNT     2

#ifdef INCLUDE_ACCESSORY_TRACKING
#define ACCESSORY_TRACKING_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(size) ACCESSORY_TRACKING_BLOCK_MIN_SIZE_##size##_ADDON_COUNT
#else
#define ACCESSORY_TRACKING_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(size) 0
#endif

/* Additional pools required when Google Finder is enabled.
   Those pools are required because in some circumstances all the messages sent by the device_sync
   may be present in the message queue at the same time.
   Pool required by message queue to send any message is 28 bytes.
   Pool needed for Google Finder device property is 36 bytes..
 */
#define GOOGLE_FINDER_BLOCK_MIN_SIZE_28_ADDON_COUNT    1
#define GOOGLE_FINDER_BLOCK_MIN_SIZE_36_ADDON_COUNT    1

#define GOOGLE_FINDER_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(size) 0

/* Additional pools required for Audio Reach support */
#define AUDIOREACH_BLOCK_MIN_SIZE_5000_ADDON_COUNT     3
#define AUDIOREACH_BLOCK_MIN_SIZE_10000_ADDON_COUNT    2

#ifdef INCLUDE_AUDIOREACH
#define AUDIOREACH_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(size) AUDIOREACH_BLOCK_MIN_SIZE_##size##_ADDON_COUNT
#else
#define AUDIOREACH_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(size) 0
#endif

/* Additional pools required for AMA hairpin support */
#define AMA_HAIRPIN_BLOCK_MIN_SIZE_512_ADDON_COUNT    1

#ifdef INCLUDE_AMA_APP_TO_APP
#define AMA_HAIRPIN_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(size) AMA_HAIRPIN_BLOCK_MIN_SIZE_##size##_ADDON_COUNT
#else
#define AMA_HAIRPIN_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(size) 0
#endif

#define APP_POOL_BLOCK_SIZE_4_BASE_COUNT        15
#define APP_POOL_BLOCK_SIZE_8_BASE_COUNT        25
#define APP_POOL_BLOCK_SIZE_12_BASE_COUNT       32
#define APP_POOL_BLOCK_SIZE_16_BASE_COUNT       58
#define APP_POOL_BLOCK_SIZE_20_BASE_COUNT       42
#define APP_POOL_BLOCK_SIZE_24_BASE_COUNT       20
#define APP_POOL_BLOCK_SIZE_28_BASE_COUNT       79
#define APP_POOL_BLOCK_SIZE_32_BASE_COUNT       21
#define APP_POOL_BLOCK_SIZE_36_BASE_COUNT       19
#define APP_POOL_BLOCK_SIZE_44_BASE_COUNT       24
#define APP_POOL_BLOCK_SIZE_56_BASE_COUNT       14
#define APP_POOL_BLOCK_SIZE_64_BASE_COUNT       10
#define APP_POOL_BLOCK_SIZE_80_BASE_COUNT       16
#define APP_POOL_BLOCK_SIZE_128_BASE_COUNT      19
#define APP_POOL_BLOCK_SIZE_160_BASE_COUNT       8
#define APP_POOL_BLOCK_SIZE_180_BASE_COUNT       2
#define APP_POOL_BLOCK_SIZE_220_BASE_COUNT       2
#define APP_POOL_BLOCK_SIZE_512_BASE_COUNT       7
#define APP_POOL_BLOCK_SIZE_692_BASE_COUNT       2

static const pmalloc_pool_config app_pools[] =
{
    {
        4, APP_POOL_BLOCK_SIZE_4_BASE_COUNT +
           LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(4)
    },
    {
        8, APP_POOL_BLOCK_SIZE_8_BASE_COUNT +
           LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(8) +
           EXTENDED_TDL_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(8)
    },
    {
        12, APP_POOL_BLOCK_SIZE_12_BASE_COUNT +
            LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(12)
    },
    {
        16, APP_POOL_BLOCK_SIZE_16_BASE_COUNT +
            LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(16) +
            EXTENDED_TDL_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(16)
    },
    {
        20, APP_POOL_BLOCK_SIZE_20_BASE_COUNT +
            LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(20) +
            EXTENDED_TDL_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(20)
    },
    {
        24, APP_POOL_BLOCK_SIZE_24_BASE_COUNT +
            LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(24) +
            EXTENDED_TDL_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(24)
    },
    {
        28, APP_POOL_BLOCK_SIZE_28_BASE_COUNT +
            LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(28) +
            EXTENDED_TDL_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(28) +
            GOOGLE_FINDER_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(28)
    },
    {
        32, APP_POOL_BLOCK_SIZE_32_BASE_COUNT
    },
    {
        36, APP_POOL_BLOCK_SIZE_36_BASE_COUNT +
            LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(36) +
            EXTENDED_TDL_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(36) +
            GOOGLE_FINDER_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(36)
    },
    {
        44, APP_POOL_BLOCK_SIZE_44_BASE_COUNT +
            LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(44)
    },
    {
        56, APP_POOL_BLOCK_SIZE_56_BASE_COUNT +
            LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(56)
    },
    {
        64, APP_POOL_BLOCK_SIZE_64_BASE_COUNT +
            LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(64)
    },
    {
        80, APP_POOL_BLOCK_SIZE_80_BASE_COUNT +
            LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(80)
    },
#ifdef INCLUDE_LE_AUDIO_UNICAST
    {
        100, LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(100)
    },
#endif
    {
        128, APP_POOL_BLOCK_SIZE_128_BASE_COUNT +
             LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(128) +
             ACCESSORY_TRACKING_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(128)
    },
    {
        160, APP_POOL_BLOCK_SIZE_160_BASE_COUNT +
             LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(160)
    },
    {
        180, APP_POOL_BLOCK_SIZE_180_BASE_COUNT
    },
    {
        220, APP_POOL_BLOCK_SIZE_220_BASE_COUNT
    },
    {
        512, APP_POOL_BLOCK_SIZE_512_BASE_COUNT +
             AMA_HAIRPIN_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(512)
    },
    {
        692, APP_POOL_BLOCK_SIZE_692_BASE_COUNT
    },
#ifdef INCLUDE_ACCESSORY_TRACKING
    {
        1400, ACCESSORY_TRACKING_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(1400)
    },
#endif
#ifdef INCLUDE_AUDIOREACH
    {
        5000, AUDIOREACH_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(5000)
    },
    {
        10000, AUDIOREACH_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(10000)
    },
#endif
};

#else /* USE_SYNERGY */

static const pmalloc_pool_config app_pools[] =
{
    {   4, 15 },
    {   8, 25 },
    {  12, 32 },
    {  16, 13 },
    {  20, 31 },
    {  24, 20 },
    {  28, 55 },
    {  32, 21 },
    {  36, 19 },
    {  40, 10 },
    {  56,  9 },
    {  64,  7 },
    {  80,  9 },
    { 124, 15 }, /* One of this blocks is added for DeviceDbSerialiser_MakeBackup() */
    { 160,  8 },
    { 180,  2 },
    { 220,  2 },
    { 288,  1 }, /* for theGatt = gattState */
    { 512,  2 }, /* One of this blocks has been added for AMA hairpin functionality. */
    { 692,  2 },

#ifdef INCLUDE_AUDIOREACH
    { 1300, 3 }
#endif
};

#endif /* USE_SYNERGY */

