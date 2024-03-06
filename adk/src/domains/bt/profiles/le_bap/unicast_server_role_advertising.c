/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_bap
    \brief
*/

#include "unicast_server_role_advertising.h"

#include "pacs_utilities.h"
#include <logging.h>
#include "device.h"
#include "bt_device.h"
#include "device_properties.h"
#include <panic.h>
#include <stdlib.h>
#include "lea_advertising_policy.h"

#ifdef USE_SYNERGY
#include "bap_server_lib.h"
#else
#include "bap_server.h"
#endif

#define BAP_UNICAST_SERVER_SINK_AVAIL_START_OCTECT      5
#define BAP_UNICAST_SERVER_SOURCE_AVAIL_START_OCTET     7

static uint8 unicastServer_GetAdvertisingDataSize(void)
{
    return BapServerGetAdvertisingDataSize(BAP_SERVER_UNICAST_ROLE);
}

static uint8 unicastServer_GetAdvertisingData(const lea_adv_policy_adv_param_t *params,
                                              uint8 *advert_buffer,
                                              uint8 advert_buffer_length)
{
    leAdvDataItem *adv_item = NULL;
    uint8 size_written;
    uint16 context_mask;

    if (params->type == lea_adv_policy_announcement_type_targeted)
    {
        DEBUG_LOG("unicastServer_GetAdvertisingData - targeted");
        adv_item = BapServerGetTargetedAdvertisingData();
    }
    else
    {
        DEBUG_LOG("unicastServer_GetAdvertisingData - general");
        adv_item =  BapServerGetAdvertisingData(BAP_SERVER_UNICAST_ROLE);
    }
    
    PanicFalse(advert_buffer_length >= adv_item->size);
    memcpy(advert_buffer, adv_item->data, adv_item->size);

    /* Context mask for sink direction */
    context_mask = params->audio_context & 0x0000FFFF;
    if (context_mask != AUDIO_CONTEXT_TYPE_UNKNOWN)
    {
        advert_buffer[BAP_UNICAST_SERVER_SINK_AVAIL_START_OCTECT] = context_mask & 0xFF;
        advert_buffer[BAP_UNICAST_SERVER_SINK_AVAIL_START_OCTECT+1] = context_mask >> 8;
    }

    /* Context mask for Source direction */
    context_mask = ((params->audio_context >> 16) & 0x0000FFFF);
    if (context_mask != AUDIO_CONTEXT_TYPE_UNKNOWN)
    {
        advert_buffer[BAP_UNICAST_SERVER_SOURCE_AVAIL_START_OCTET] = context_mask & 0xFF;
        advert_buffer[BAP_UNICAST_SERVER_SOURCE_AVAIL_START_OCTET+1] = context_mask >> 8;
    }

    size_written = adv_item->size;

    BapServerReleaseAdvertisingItems(BAP_SERVER_UNICAST_ROLE);
    return size_written;
}

static const lea_adv_policy_clients_callback_t unicast_server_callback =
{
    .GetAdvertisingDataSize = unicastServer_GetAdvertisingDataSize,
    .GetAdvertisingData = unicastServer_GetAdvertisingData
};

bool LeBapUnicastServer_SetupLeAdvertisingData(void)
{
    DEBUG_LOG("LeBapUnicastServer_SetupLeAdvertisingData - Registering with LE Advert Policy");
    LeaAdvertisingPolicy_RegisterClient(&unicast_server_callback);
    return TRUE;
}
