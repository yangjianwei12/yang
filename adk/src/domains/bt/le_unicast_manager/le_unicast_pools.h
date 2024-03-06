/*!
   \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
               All Rights Reserved.
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \file
   \addtogroup le_unicast_manager
   \brief      Additional application pools required for LE Audio Unicast support
   @{
*/

#ifndef LE_UNICAST_POOLS_H
#define LE_UNICAST_POOLS_H

/*
   Additional pools: 102
   Additional bytes:  3784
*/
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_4_ADDON_COUNT      20
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_8_ADDON_COUNT       3
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_12_ADDON_COUNT      7
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_16_ADDON_COUNT      4
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_20_ADDON_COUNT     19
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_24_ADDON_COUNT      4
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_28_ADDON_COUNT     12
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_36_ADDON_COUNT      1
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_44_ADDON_COUNT      2
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_56_ADDON_COUNT      3
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_64_ADDON_COUNT      4
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_80_ADDON_COUNT     16
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_100_ADDON_COUNT     2
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_128_ADDON_COUNT     3
#define LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_160_ADDON_COUNT     2

#ifdef INCLUDE_LE_AUDIO_UNICAST
#define LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(size) LE_AUDIO_UNICAST_BLOCK_MIN_SIZE_##size##_ADDON_COUNT
#else
#define LE_AUDIO_UNICAST_ADDON_COUNT_FOR_BLOCK_MIN_SIZE(size) 0
#endif

#endif /* LE_UNICAST_POOLS_H */
/*! @} */