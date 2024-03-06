/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_cap
    \brief
*/

#include "cap_server_role_advertising.h"
#include "gatt_cas_server_uuids.h"
#include "lea_advertising_policy.h"

#include <logging.h>
#include <panic.h>
#include <stdlib.h>

#define CAP_SERVER_SIZE_ADVERT                  5
#define CAP_SERVER_ANNOUNCEMENT_TYPE_OCTET      4

/*! Service Data (16-bit default). */
#define CAP_BLE_AD_TYPE_SERVICE_DATA            0x16

#define CAP_DEBUG_INFO     DEBUG_LOG

static const uint8 capServerAdvertData[CAP_SERVER_SIZE_ADVERT] = {
    CAP_SERVER_SIZE_ADVERT - 1,
    CAP_BLE_AD_TYPE_SERVICE_DATA,
    /* CAS UUID */
    UUID_COMMON_AUDIO_SERVICE  & 0xFF,
    UUID_COMMON_AUDIO_SERVICE  >> 8,
    /* 1 octet for Announcement Type */
    lea_adv_policy_announcement_type_general,
};

static uint8 capServer_GetAdvertisingDataSize(void)
{
    return CAP_SERVER_SIZE_ADVERT;
}

static uint8 capServer_GetAdvertisingData(const lea_adv_policy_adv_param_t *params,
                                         uint8 *advert_buffer,
                                         uint8 advert_buffer_length)
{
    DEBUG_LOG("capServer_GetAdvertisingData");
    PanicFalse(advert_buffer_length >= CAP_SERVER_SIZE_ADVERT);

    memcpy(advert_buffer, capServerAdvertData, CAP_SERVER_SIZE_ADVERT);
    advert_buffer[CAP_SERVER_ANNOUNCEMENT_TYPE_OCTET] = params->type;
    return CAP_SERVER_SIZE_ADVERT;
}

static const lea_adv_policy_clients_callback_t cap_server_callback =
{
    .GetAdvertisingDataSize = capServer_GetAdvertisingDataSize,
    .GetAdvertisingData = capServer_GetAdvertisingData
};

bool CapServer_SetupLeAdvertisingData(void)
{
    DEBUG_LOG("CapServer_SetupLeAdvertisingData - Registering with LE Advert Policy");
    LeaAdvertisingPolicy_RegisterClient(&cap_server_callback);
    return TRUE;
}
