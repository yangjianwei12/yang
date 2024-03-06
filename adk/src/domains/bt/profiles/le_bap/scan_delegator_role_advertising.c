/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_bap
    \brief
*/

#include "scan_delegator_role_advertising.h"
#include "pacs_utilities.h"
#include "lea_advertising_policy.h"

#include <logging.h>
#include <panic.h>
#include <stdlib.h>
#ifdef USE_SYNERGY
#include "bap_server_lib.h"
#else
#include "bap_server.h"
#endif

static uint8 scanDelegator_GetAdvertisingDataSize(void)
{
    return BapServerGetAdvertisingDataSize(BAP_SERVER_BROADCAST_ROLE);
}

static uint8 scanDelegator_GetAdvertisingData(const lea_adv_policy_adv_param_t *params,
                                              uint8 *advert_buffer,
                                              uint8 advert_buffer_length)
{
    UNUSED(params);
    leAdvDataItem *adv_item = NULL;
    uint8 size_written;

    DEBUG_LOG("scanDelegator_GetAdvertisingData");
    adv_item =  BapServerGetAdvertisingData(BAP_SERVER_BROADCAST_ROLE);
    PanicFalse(advert_buffer_length >= adv_item->size);

    memcpy(advert_buffer, adv_item->data, adv_item->size);
    size_written = adv_item->size;

    BapServerReleaseAdvertisingItems(BAP_SERVER_BROADCAST_ROLE);
    return size_written;
}

static const lea_adv_policy_clients_callback_t scanDelegator_server_callback =
{
    .GetAdvertisingDataSize = scanDelegator_GetAdvertisingDataSize,
    .GetAdvertisingData = scanDelegator_GetAdvertisingData
};

bool LeBapScanDelegator_SetupLeAdvertisingData(void)
{
    DEBUG_LOG("LeBapUnicastServer_SetupLeAdvertisingData - Registering with LE Advert Policy");
    LeaAdvertisingPolicy_RegisterClient(&scanDelegator_server_callback);
    return TRUE;
}
