/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of LE advertising related testing functions.
*/

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

#include "adk_test_le_advertising.h"
#include "le_advertising_manager.h"
#include "le_advertising_manager_multi_set_private.h"
#include "panic.h"

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#define ADV_SIZE 28
static const uint8 payload[ADV_SIZE] =
{
    ADV_SIZE - 1,
    ble_ad_type_manufacturer_specific_data,
    0xe,0xf,0x1,0x2,0xa,0xb,0x3,0x4,0xc,0xd,
    0x9,0x8,0x5,0x6,0xd,0xc,0x1,0x2,0xa,0xb,
    0x5,0x6,0xf,0xe,0x1,0x2
};

static const le_adv_item_data_t advert_payload =
{
    ADV_SIZE,
    payload
};

static bool GetTestItemData(le_adv_item_data_t * item)
{
    PanicNull(item);

    item->data = NULL;
    item->size = 0;

    *item = advert_payload;

    return TRUE;
}

static bool GetTestItemParams(le_adv_item_params_t * params)
{
    PanicNull(params);

    LeAdvertisingManager_PopulateDefaultAdvertisingParams(params);

    return TRUE;
}

static bool GetTestItemInfoLegacy(le_adv_item_info_t *info)
{
    PanicNull(info);

    *info = (le_adv_item_info_t){ .placement = le_adv_item_data_placement_advert,
                                        .type = le_adv_type_legacy_connectable_scannable,
                                        .data_size = advert_payload.size,
                                        .needs_own_set = TRUE};
    return TRUE;

}

static bool GetTestItemInfoExtended(le_adv_item_info_t *info)
{
    PanicNull(info);

    *info = (le_adv_item_info_t){ .placement = le_adv_item_data_placement_advert,
                                        .type = le_adv_type_extended_connectable,
                                        .data_size = advert_payload.size,
                                        .needs_own_set = TRUE};
    return TRUE;

}

uint8 appTestLeAdvertising_GetNumberOfSupportedAdvertisingSets(void)
{
    return LeAdvertisingManager_AggregatorGetNumberOfSupportedSets();
}

bool appTestLeAdvertising_IsAdvertisingSetEnabled(uint8 set_id)
{
    return LeAdvertisingManager_AggregatorIsAdvertisingSetActive(set_id);
}


bool appTestLeAdvertising_DisableAdvertisingSet(uint8 set_id)
{
    return LeAdvertisingManager_AggregatorDisableAdvertisingSet(set_id);
}

le_adv_item_callback_t callback_legacy = {.GetItemData = &GetTestItemData,
                                   .GetItemParameters = &GetTestItemParams,
                                   .GetItemInfo = &GetTestItemInfoLegacy};

le_adv_item_callback_t callback_extended = {.GetItemData = &GetTestItemData,
                                   .GetItemParameters = &GetTestItemParams,
                                   .GetItemInfo = &GetTestItemInfoExtended};

bool appTestLeAdvertising_AddNewLegacyAdvertisingSet(void)
{
    bool result = FALSE;

    if(LeAdvertisingManager_RegisterAdvertisingItemCallback(NULL, &callback_legacy))
    {
        result = TRUE;
    }

    return result;

}

bool appTestLeAdvertising_AddNewExtendedAdvertisingSet(void)
{
    bool result = FALSE;

    if(LeAdvertisingManager_RegisterAdvertisingItemCallback(NULL, &callback_extended))
    {
        result = TRUE;
    }

    return result;
}

uint32 appTestLeAdvertising_GetSpaceInUseForAdvertisingSet(uint8 set_id)
{
    return LeAdvertisingManager_AggregatorGetSpaceInUseForAdvertisingSet(set_id);
}

uint16 appTestLeAdvertising_GetEventTypeForAdvertisingSet(uint8 set_id)
{
    return LeAdvertisingManager_AggregatorGetEventTypeForAdvertisingSet(set_id);
}

uint32 appTestLeAdvertising_GetMinimumIntervalForAdvertisingSet(uint8 set_id)
{
    return LeAdvertisingManager_AggregatorGetMinIntervalForAdvertisingSet(set_id);
}

uint32 appTestLeAdvertising_GetMaximumIntervalForAdvertisingSet(uint8 set_id)
{
    return LeAdvertisingManager_AggregatorGetMaxIntervalForAdvertisingSet(set_id);
}

uint8 appTestLeAdvertising_GetChannelsInUseForAdvertisingSet(uint8 set_id)
{
    return LeAdvertisingManager_AggregatorGetChannelsForAdvertisingSet(set_id);
}

typed_bdaddr * appTestLeAdvertising_GetTpBdaddrForAdvertisingSet(uint8 set_id)
{
    typed_bdaddr * result = NULL;

    result = LeAdvertisingManager_AggregatorGetTpBdaddrForAdvertisingSet(set_id);

    return result;

}

#endif
