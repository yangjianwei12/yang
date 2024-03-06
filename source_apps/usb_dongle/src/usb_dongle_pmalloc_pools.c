/*!
\copyright  Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Definition of the pmalloc pools used by the USB dongle application.

The pools defined here will be merged at run time with the base definitions
from Hydra OS - see 'pmalloc_config_P1.h'.
*/

#include <pmalloc.h>

_Pragma ("unitsuppress Unused")

_Pragma ("datasection apppool")

#ifdef USE_SYNERGY

#define APP_POOL_BLOCK_SIZE_4_ADDON_COUNT        0
#define APP_POOL_BLOCK_SIZE_8_ADDON_COUNT        0
#define APP_POOL_BLOCK_SIZE_12_ADDON_COUNT       0
#define APP_POOL_BLOCK_SIZE_16_ADDON_COUNT       0
#define APP_POOL_BLOCK_SIZE_20_ADDON_COUNT       0
#define APP_POOL_BLOCK_SIZE_24_ADDON_COUNT       0
#define APP_POOL_BLOCK_SIZE_28_ADDON_COUNT       0
#define APP_POOL_BLOCK_SIZE_32_ADDON_COUNT       0
#define APP_POOL_BLOCK_SIZE_36_ADDON_COUNT       0
#define APP_POOL_BLOCK_SIZE_40_ADDON_COUNT       0
#define APP_POOL_BLOCK_SIZE_56_ADDON_COUNT       0
#define APP_POOL_BLOCK_SIZE_64_ADDON_COUNT       0
#define APP_POOL_BLOCK_SIZE_80_ADDON_COUNT       0
#define APP_POOL_BLOCK_SIZE_84_ADDON_COUNT       0
#define APP_POOL_BLOCK_SIZE_100_ADDON_COUNT      0
#define APP_POOL_BLOCK_SIZE_124_ADDON_COUNT      0
#define APP_POOL_BLOCK_SIZE_140_ADDON_COUNT      0
#define APP_POOL_BLOCK_SIZE_180_ADDON_COUNT      0
#define APP_POOL_BLOCK_SIZE_220_ADDON_COUNT      0
#define APP_POOL_BLOCK_SIZE_288_ADDON_COUNT      0
#define APP_POOL_BLOCK_SIZE_512_ADDON_COUNT      0
#define APP_POOL_BLOCK_SIZE_692_ADDON_COUNT      0

#define APP_POOL_BLOCK_SIZE_4_BASE_COUNT        15
#define APP_POOL_BLOCK_SIZE_8_BASE_COUNT        25
#define APP_POOL_BLOCK_SIZE_12_BASE_COUNT       32
#define APP_POOL_BLOCK_SIZE_16_BASE_COUNT       24
#define APP_POOL_BLOCK_SIZE_20_BASE_COUNT       38
#define APP_POOL_BLOCK_SIZE_24_BASE_COUNT       50
#define APP_POOL_BLOCK_SIZE_28_BASE_COUNT       64
#define APP_POOL_BLOCK_SIZE_32_BASE_COUNT       21
#define APP_POOL_BLOCK_SIZE_36_BASE_COUNT       24
#define APP_POOL_BLOCK_SIZE_40_BASE_COUNT       10
#define APP_POOL_BLOCK_SIZE_56_BASE_COUNT       12
#define APP_POOL_BLOCK_SIZE_64_BASE_COUNT        7
#define APP_POOL_BLOCK_SIZE_80_BASE_COUNT        9
#define APP_POOL_BLOCK_SIZE_84_BASE_COUNT        2
#define APP_POOL_BLOCK_SIZE_100_BASE_COUNT       8
#define APP_POOL_BLOCK_SIZE_124_BASE_COUNT      16
#define APP_POOL_BLOCK_SIZE_140_BASE_COUNT       4
#define APP_POOL_BLOCK_SIZE_180_BASE_COUNT       8
#define APP_POOL_BLOCK_SIZE_220_BASE_COUNT       4
#define APP_POOL_BLOCK_SIZE_288_BASE_COUNT       1
#define APP_POOL_BLOCK_SIZE_512_BASE_COUNT       1
#define APP_POOL_BLOCK_SIZE_692_BASE_COUNT       3

#define APP_POOL_BLOCK_ENTRY(size) \
    {size, APP_POOL_BLOCK_SIZE_##size##_BASE_COUNT + APP_POOL_BLOCK_SIZE_##size##_ADDON_COUNT}
	
static const pmalloc_pool_config app_pools[] =
{
    APP_POOL_BLOCK_ENTRY(4),
    APP_POOL_BLOCK_ENTRY(8),
    APP_POOL_BLOCK_ENTRY(12),
    APP_POOL_BLOCK_ENTRY(16),
    APP_POOL_BLOCK_ENTRY(20),
    APP_POOL_BLOCK_ENTRY(24),
    APP_POOL_BLOCK_ENTRY(28),
    APP_POOL_BLOCK_ENTRY(32),
    APP_POOL_BLOCK_ENTRY(36),
    APP_POOL_BLOCK_ENTRY(40),
    APP_POOL_BLOCK_ENTRY(56),
    APP_POOL_BLOCK_ENTRY(64),
    APP_POOL_BLOCK_ENTRY(80),
    APP_POOL_BLOCK_ENTRY(84),
    APP_POOL_BLOCK_ENTRY(100),
    APP_POOL_BLOCK_ENTRY(124),
    APP_POOL_BLOCK_ENTRY(140),
    APP_POOL_BLOCK_ENTRY(180),
    APP_POOL_BLOCK_ENTRY(220),
    APP_POOL_BLOCK_ENTRY(288), /* CAP global instance */
    APP_POOL_BLOCK_ENTRY(512),
    APP_POOL_BLOCK_ENTRY(692),

    /* Including the pools in pmalloc_config_P1.h:
       Total slots: 411
       Total bytes: 18192
    */
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
    { 120, 16 },
    { 140,  4 },
    { 180,  4 },
    { 220,  2 },
    { 288,  1 }, /* for theGatt = gattState */
    { 512,  1 },
    { 692,  3 }

    /* Including the pools in pmalloc_config_P1.h:
       Total slots: 325
       Total bytes: 13956
    */
};

#endif /* USE_SYNERGY */
