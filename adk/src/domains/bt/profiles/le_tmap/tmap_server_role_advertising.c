/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup     tmap_profile
    \brief
*/

#include "tmap_server_role_advertising.h"
#include "gatt_tmas_server_uuids.h"
#include "gatt_tmas_server.h"
#include "lea_advertising_policy.h"

#include <logging.h>
#include <panic.h>
#include <stdlib.h>

#define TMAP_SERVER_SIZE_ADVERT                  6

/*! Service Data (16-bit default). */
#define TMAP_BLE_AD_TYPE_SERVICE_DATA            0x16

#if defined(INCLUDE_LE_AUDIO_BROADCAST)
#define TMAP_BROADCAST_ROLE TMAP_ROLE_BROADCAST_MEDIA_RECEIVER
#else
#define TMAP_BROADCAST_ROLE 0
#endif

#if defined(INCLUDE_LE_AUDIO_UNICAST)

#ifndef DISABLE_LE_AUDIO_VOICE
#define TMAP_ROLE_CALL_TERMINAL_SUPPORT             TMAP_ROLE_CALL_TERMINAL
#else
#define TMAP_ROLE_CALL_TERMINAL_SUPPORT             0
#endif

#ifndef DISABLE_LE_AUDIO_MEDIA
#define TMAP_ROLE_UNICAST_MEDIA_RECEIVER_SUPPORT    TMAP_ROLE_UNICAST_MEDIA_RECEIVER
#else
#define TMAP_ROLE_UNICAST_MEDIA_RECEIVER_SUPPORT    0
#endif

#define TMAP_UNICAST_ROLE   (TMAP_ROLE_CALL_TERMINAL_SUPPORT | TMAP_ROLE_UNICAST_MEDIA_RECEIVER_SUPPORT)

#else /* !INCLUDE_LE_AUDIO_UNICAST */

#define TMAP_UNICAST_ROLE   0

#endif /* INCLUDE_LE_AUDIO_UNICAST */

/*! Tmap supporting role characteristic valus (UMR, BMR, CT))*/
#define TMAP_ROLE_VALUE     (TMAP_BROADCAST_ROLE | TMAP_UNICAST_ROLE)

#define TMAP_DEBUG_INFO     DEBUG_LOG

static const uint8 tmapServerAdvertData[TMAP_SERVER_SIZE_ADVERT] = {
    TMAP_SERVER_SIZE_ADVERT - 1,
    TMAP_BLE_AD_TYPE_SERVICE_DATA,
    /* TMAS UUID */
    UUID_TELEPHONY_MEDIA_AUDIO_SERVICE & 0xFF,
    UUID_TELEPHONY_MEDIA_AUDIO_SERVICE >> 8,
    /* TMAP ROLE*/
    (TMAP_ROLE_VALUE) & 0xFF,
    (TMAP_ROLE_VALUE) >> 8,
};

static uint8 tmapServer_GetAdvertisingDataSize(void)
{
    return TMAP_SERVER_SIZE_ADVERT;
}

static uint8 tmapServer_GetAdvertisingData(const lea_adv_policy_adv_param_t *params,
                                           uint8 *advert_buffer,
                                           uint8 advert_buffer_length)
{
    UNUSED(params);
    DEBUG_LOG("tmap_GetAdvertisingData");
    PanicFalse(advert_buffer_length >= TMAP_SERVER_SIZE_ADVERT);

    memcpy(advert_buffer, tmapServerAdvertData, TMAP_SERVER_SIZE_ADVERT);
    return TMAP_SERVER_SIZE_ADVERT;
}

static const lea_adv_policy_clients_callback_t tmap_server_callback =
{
    .GetAdvertisingDataSize = tmapServer_GetAdvertisingDataSize,
    .GetAdvertisingData = tmapServer_GetAdvertisingData
};

bool TmapServer_SetupLeAdvertisingData(void)
{
    DEBUG_LOG("TmapServer_SetupLeAdvertisingData - Registering with LE Advert Policy");
    LeaAdvertisingPolicy_RegisterClient(&tmap_server_callback);
    return TRUE;
}
