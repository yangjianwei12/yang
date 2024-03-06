/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       headset_product_config.h
\brief      Application product configuration file
*/

#ifndef HEADSET_PRODUCT_CONFIG_H_
#define HEADSET_PRODUCT_CONFIG_H_

#ifdef ENABLE_LE_AUDIO_CSIP
  #if !defined(INCLUDE_LE_AUDIO_UNICAST) || !defined(INCLUDE_LE_AUDIO_BROADCAST)
      #error Cannot have CSIP based LR Speaker without LEA Unicast or Broadcast
  #endif
  /* currently CSIP based spk solution restricts LE only connection */
  #if !defined(ENABLE_LE_ONLY_CONNECTION)
    #error CSIP based LR Speaker restricts to LE only connection
  #endif
#endif  /* ENABLE_LE_AUDIO_CSIP */

#if defined(ENABLE_TWM_SPEAKER) && !defined(INCLUDE_MIRRORING)
    #error Require mirroring enabled for TWM based speaker
#endif

#if defined(ENABLE_TWM_SPEAKER) && !defined(DISABLE_KEY_SYNC)
    #error Key Sync is not supported in TWM Speaker as HO is not supported
#endif

#if defined (INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined (ENABLE_SIMPLE_SPEAKER)
#if defined (INCLUDE_APTX_HD) || defined (INCLUDE_APTX_ADAPTIVE) || defined (APTX_ADAPTIVE_SUPPORT_R1_ONLY) || defined (INCLUDE_APTX_ADAPTIVE_22) || defined (APTX_ADAPTIVE_SUPPORT_96K)
    #error Remove the above defined APTX DEFS as these codecs are not supported in speaker BMS
#endif /* defined(INCLUDE_APTX_HD) || defined(INCLUDE_APTX_ADAPTIVE) || defined(APTX_ADAPTIVE_SUPPORT_R1_ONLY) || defined(INCLUDE_APTX_ADAPTIVE_22) || defined(APTX_ADAPTIVE_SUPPORT_96K) */
#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) && defined(ENABLE_SIMPLE_SPEAKER) */

#endif /* HEADSET_PRODUCT_CONFIG_H_ */
