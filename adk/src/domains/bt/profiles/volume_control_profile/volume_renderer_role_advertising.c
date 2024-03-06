/*!
    \copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    volume_profile
    \brief      LE advertising data required for the volume renderer role of VCS.
*/

#include "volume_renderer_role_advertising.h"
#include "lea_advertising_policy.h"

#include <logging.h>

#include <panic.h>
#include <stdlib.h>

#define VOLUME_CONTROL_SERVER_NUMBER_ADVERT_DATA_ITEMS 	1
#define VOLUME_CONTROL_SERVER_SIZE_ADVERT              	4

#define VOLUME_CONTROL_SERVICE_UUID                     0x1844

static const uint8 volume_renderer_data[VOLUME_CONTROL_SERVER_SIZE_ADVERT] = {
    VOLUME_CONTROL_SERVER_SIZE_ADVERT - 1,
    ble_ad_type_service_data,
    VOLUME_CONTROL_SERVICE_UUID & 0xFF,
    VOLUME_CONTROL_SERVICE_UUID >> 8
};

static uint8 volumeRenderer_GetAdvertisingDataSize(void)
{
    return VOLUME_CONTROL_SERVER_SIZE_ADVERT;
}

static uint8 volumeRenderer_GetAdvertisingData(const lea_adv_policy_adv_param_t *params,
                                               uint8 *advert_buffer,
                                               uint8 advert_buffer_length)
{
    UNUSED(params);

    DEBUG_LOG("volumeRenderer_GetAdvertisingData");
    PanicFalse(advert_buffer_length >= VOLUME_CONTROL_SERVER_SIZE_ADVERT);

    memcpy(advert_buffer, volume_renderer_data, VOLUME_CONTROL_SERVER_SIZE_ADVERT);
    return VOLUME_CONTROL_SERVER_SIZE_ADVERT;
}

static const lea_adv_policy_clients_callback_t volume_renderer_callback =
{
    .GetAdvertisingDataSize = volumeRenderer_GetAdvertisingDataSize,
    .GetAdvertisingData = volumeRenderer_GetAdvertisingData
};

bool VolumeRenderer_SetupLeAdvertisingData(void)
{
    DEBUG_LOG("VolumeRenderer_SetupLeAdvertisingData - Registering with LE Advert Policy");
    LeaAdvertisingPolicy_RegisterClient(&volume_renderer_callback);
    return TRUE;
}
