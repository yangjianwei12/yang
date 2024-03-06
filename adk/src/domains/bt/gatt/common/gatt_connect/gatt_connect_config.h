/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup gatt_connect
    \brief      GATT Connect configuration
    @{
*/

#ifndef GATT_CONNECT_CONFIG_H_
#define GATT_CONNECT_CONFIG_H_

enum
{
    max_gatt_connect_observers_base = 10,
#ifdef  INCLUDE_LE_AUDIO_BROADCAST
    max_gatt_connect_observers_with_broadcast,
#endif
#ifdef INCLUDE_LE_AUDIO_UNICAST
    max_gatt_connect_observers_with_unicast,
    max_gatt_connect_observers_with_le_gatt_service_discovery,
    max_gatt_connect_observers_with_le_media_control_profile,
    max_gatt_connect_observers_with_le_call_control_profile,
    max_gatt_connect_observers_with_le_audio_micp,
#endif
#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
    max_gatt_connect_observers_with_le_audio_volume,
    max_gatt_connect_observers_with_le_audio_pacs,
    max_gatt_connect_observers_with_le_audio_tmap,
#endif
    max_gatt_connect_observers_with_tws_topology,
    max_gatt_connect_observers
};

#define MAX_NUMBER_GATT_CONNECT_OBSERVERS   (max_gatt_connect_observers - 1)

#endif
/*! @} */