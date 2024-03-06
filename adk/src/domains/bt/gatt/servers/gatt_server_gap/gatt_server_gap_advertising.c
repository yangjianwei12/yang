/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    gatt_server_gap
    \brief
*/

#include "gatt_server_gap_advertising.h"
#include "gatt_server_gap.h"

#include "le_advertising_manager.h"
#include "local_name.h"

#include <logging.h>
#include <panic.h>
#include <stdlib.h>

#define GAP_ADVERT_FLAGS                           (BLE_FLAGS_GENERAL_DISCOVERABLE_MODE | BLE_FLAGS_DUAL_CONTROLLER | BLE_FLAGS_DUAL_HOST)
#define GAP_ADVERT_FLAGS_LENGTH                     3
#define GAP_APPEARANCE_ITEM_LENGTH                  4
#define GAP_APPEARANCE_ITEM_DATA_START_OCTET        2

/* The maximum length of the name item that will fit into a LE advertising set.
   The max length here is set based on the maximum size of a legacy advert and
   knowledge of the wider system. For example what other item(s) may be
   required to be included in all adverts. */
#define GAP_ADVERT_NAME_MAX_LENGTH 26


typedef struct {
    le_adv_item_data_t item;
    unsigned legacy_in_use:1;
    unsigned extended_in_use:1;
} gap_name_item_t;

static const uint8 gap_adv_flags_data[GAP_ADVERT_FLAGS_LENGTH] =
{
    GAP_ADVERT_FLAGS_LENGTH - 1,
    ble_ad_type_flags,
    GAP_ADVERT_FLAGS
};

static gap_name_item_t gap_name_item = {0};
static const le_adv_item_data_t gap_flags_item = {.data = gap_adv_flags_data, .size = sizeof(gap_adv_flags_data)};
#ifdef INCLUDE_LE_AUDIO_UNICAST
static le_adv_item_data_t gap_appearance_item = {0};
#endif

/* Get the local device name and if needed truncate the size so it will fit into an advert. */
static const uint8 *gattServerGap_GetLocalName(uint16 *name_length, bool *truncated)
{
    uint16 name_len;
    bool was_truncated = FALSE;
    const uint8* name = LocalName_GetPrefixedName(&name_len);

    if (name_len > GAP_ADVERT_NAME_MAX_LENGTH)
    {
        /* If a client has specified that only the complete name should be
           advertised we cannot truncate the name. But we can't fit the name
           into an advert either. So Panic() - the device needs to use a
           shorter name. */
        PanicFalse(!GattServerGap_IsCompleteLocalNameBeingUsed());

        name_len = GAP_ADVERT_NAME_MAX_LENGTH;
        was_truncated = TRUE;
    }

    *name_length = name_len;
    *truncated = was_truncated;
    return name;
}

static unsigned gattServerGap_GetItemDataNameSize(void)
{
    uint16 length = 0;
    bool truncated = FALSE;
    gattServerGap_GetLocalName(&length, &truncated);
    return (unsigned)(length + AD_DATA_HEADER_SIZE);
}

static bool gattServerGap_GetItemDataName(le_adv_item_data_t * item)
{
    PanicNull(item);

    DEBUG_LOG("gattServerGap_GetAdvertisingItemName gap_name_item.item.data:%p", gap_name_item.item.data);

    uint16 name_len = 0;
    bool name_truncated = FALSE;
    const uint8* name = gattServerGap_GetLocalName(&name_len, &name_truncated);

    /* Check if name has changed, if it has free the current data to force it
       to be recreated with the new name. */
    if (gap_name_item.item.data)
    {
        if (memcmp(&gap_name_item.item.data[AD_DATA_HEADER_SIZE], name, gap_name_item.item.size))
        {
            free((void *)gap_name_item.item.data);
            gap_name_item.item.data = NULL;
        }
    }

    if (gap_name_item.item.data == NULL)
    {
        PanicNull((void*)name);

        uint16 data_len = gattServerGap_GetItemDataNameSize();
        uint8* data = PanicUnlessMalloc(data_len);

        data[AD_DATA_LENGTH_OFFSET] = name_len + 1;
        data[AD_DATA_TYPE_OFFSET] = name_truncated ? ble_ad_type_shortened_local_name : ble_ad_type_complete_local_name;
        memcpy(&data[AD_DATA_HEADER_SIZE], name, name_len);

        gap_name_item.item.size = data_len;
        gap_name_item.item.data = data;
    }

    *item = gap_name_item.item;
    return TRUE;
}

static void gattServerGap_ReleaseItemDataName(void)
{
    if (gap_name_item.item.data && !gap_name_item.legacy_in_use && !gap_name_item.extended_in_use)
    {
        free ((void *)gap_name_item.item.data);
        gap_name_item.item.data = NULL;
        gap_name_item.item.size = 0;
    }
}

static bool gattServerGap_GetItemDataFlags(le_adv_item_data_t * data)
{
    PanicNull(data);
    *data = gap_flags_item;
    return TRUE;
}

#ifdef INCLUDE_LE_AUDIO_UNICAST
static bool gattServerGap_GetItemDataAppearance(le_adv_item_data_t * data)
{
    PanicNull(data);
    uint8 *advert_data = PanicUnlessMalloc(GAP_APPEARANCE_ITEM_LENGTH);

    advert_data[0] = GAP_APPEARANCE_ITEM_LENGTH - 1;
    advert_data[1] = ble_ad_type_appearance;
    advert_data[GAP_APPEARANCE_ITEM_DATA_START_OCTET] = GattServerGap_GetAppearanceValue() & 0xFF;
    advert_data[GAP_APPEARANCE_ITEM_DATA_START_OCTET + 1] = GattServerGap_GetAppearanceValue() >> 8;

    gap_appearance_item.data = advert_data;
    gap_appearance_item.size = GAP_APPEARANCE_ITEM_LENGTH;
    *data = gap_appearance_item;

    return TRUE;
}

static void gattServerGap_ReleaseItemDataAppearance(void)
{
    if (gap_appearance_item.data)
    {
        free((void *)gap_appearance_item.data);
        gap_appearance_item.size = 0;
    }
}
#endif

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

static le_adv_item_handle gatt_server_gap_registered_item_name_handle = NULL;
static le_adv_item_handle gatt_server_gap_registered_item_name_extended_handle = NULL;
static le_adv_item_handle gatt_server_gap_registered_item_flags_handle = NULL;
#ifdef INCLUDE_LE_AUDIO_UNICAST
static le_adv_item_handle gatt_server_gap_registered_item_appearance_handle = NULL;
#endif

static bool gattServerGap_GetItemInfoFlags(le_adv_item_info_t * info)
{
    PanicNull(info);
    *info = (le_adv_item_info_t){ .placement = le_adv_item_data_placement_advert,
                                  .type = le_adv_type_legacy_connectable_scannable,
                                  .data_size = gap_flags_item.size };
    return TRUE;
}

#ifdef INCLUDE_LE_AUDIO_UNICAST
static bool gattServerGap_GetItemInfoAppearance(le_adv_item_info_t * info)
{
    PanicNull(info);
    *info = (le_adv_item_info_t){ .placement = le_adv_item_data_placement_advert,
                                  .type =  le_adv_type_extended_connectable,
                                  .data_size = GAP_APPEARANCE_ITEM_LENGTH };
    return TRUE;
}
#endif

le_adv_item_callback_t gattServerGap_AdvertisingManagerItemFlagsCallback =
{
    .GetItemData = &gattServerGap_GetItemDataFlags,
    .GetItemInfo = &gattServerGap_GetItemInfoFlags
};

#ifdef INCLUDE_LE_AUDIO_UNICAST
le_adv_item_callback_t gattServerGap_AdvertisingManagerItemAppearanceCallback =
{
    .GetItemData = &gattServerGap_GetItemDataAppearance,
    .ReleaseItemData = &gattServerGap_ReleaseItemDataAppearance,
    .GetItemInfo = &gattServerGap_GetItemInfoAppearance
};
#endif

static bool gattServerGap_GetItemDataNameLegacyAdvertising(le_adv_item_data_t * item)
{
    gap_name_item.legacy_in_use = TRUE;
    return gattServerGap_GetItemDataName(item);
}

static void gattServerGap_ReleaseItemDataNameLegacyAdvertising(void)
{
    gap_name_item.legacy_in_use = FALSE;
    gattServerGap_ReleaseItemDataName();
}

static bool gattServerGap_GetItemInfoNameLegacyAdvertising(le_adv_item_info_t * info)
{
    PanicNull(info);
    *info = (le_adv_item_info_t){ .placement = le_adv_item_data_placement_scan_response,
                                  .type = le_adv_type_legacy_connectable_scannable,
                                  .data_size = gattServerGap_GetItemDataNameSize() };
    return TRUE;
}

le_adv_item_callback_t gattServerGap_AdvertisingManagerItemNameCallback =
{
    .GetItemData = &gattServerGap_GetItemDataNameLegacyAdvertising,
    .ReleaseItemData = &gattServerGap_ReleaseItemDataNameLegacyAdvertising,
    .GetItemInfo = &gattServerGap_GetItemInfoNameLegacyAdvertising
};

static bool gattServerGap_GetItemDataNameExtendedAdvertising(le_adv_item_data_t * item)
{
    gap_name_item.extended_in_use = TRUE;
    return gattServerGap_GetItemDataName(item);
}

static void gattServerGap_ReleaseItemDataNameExtendedAdvertising(void)
{
    gap_name_item.extended_in_use = FALSE;
    gattServerGap_ReleaseItemDataName();
}

static bool gattServerGap_GetItemInfoNameExtendedAdvertising(le_adv_item_info_t * info)
{
    PanicNull(info);
    *info = (le_adv_item_info_t){ .placement = le_adv_item_data_placement_advert,
                                  .type = le_adv_type_extended_connectable,
                                  .data_size = gattServerGap_GetItemDataNameSize() };
    return TRUE;
}

le_adv_item_callback_t gattServerGap_AdvertisingManagerItemNameExtendedAdvertisingCallback =
{
    .GetItemData = &gattServerGap_GetItemDataNameExtendedAdvertising,
    .ReleaseItemData = &gattServerGap_ReleaseItemDataNameExtendedAdvertising,
    .GetItemInfo = &gattServerGap_GetItemInfoNameExtendedAdvertising
};

#else

static le_adv_mgr_register_handle gatt_server_gap_registered_handle = NULL;

static bool gattServerGap_IsNameReturned(const le_adv_data_params_t * params)
{
    if((params->data_set != le_adv_data_set_handset_unidentifiable) &&
       (params->data_set != le_adv_data_set_extended_handset))
    {
        return FALSE;
    }

    if(params->placement != le_adv_data_placement_dont_care)
    {
        return FALSE;
    }

    if(GattServerGap_IsCompleteLocalNameBeingUsed())
    {
        if(params->completeness != le_adv_data_completeness_full)
        {
            return FALSE;
        }
    }
    else
    {
        if(params->completeness != le_adv_data_completeness_can_be_shortened)
        {
            return FALSE;
        }
    }

    return TRUE;
}

static bool gattServerGap_IsFlagsReturned(const le_adv_data_params_t * params)
{
    if(params->data_set == le_adv_data_set_peer)
    {
        return FALSE;
    }

    if(params->placement != le_adv_data_placement_advert)
    {
        return FALSE;
    }

    if(params->completeness != le_adv_data_completeness_full)
    {
        return FALSE;
    }

    return TRUE;
}

static unsigned gattServerGap_NumberOfAdvItems(const le_adv_data_params_t * params)
{
    unsigned count = 0;

    if(gattServerGap_IsNameReturned(params))
    {
        count++;
    }

    if(gattServerGap_IsFlagsReturned(params))
    {
        count++;
    }

    return count;
}

static le_adv_data_item_t gattServerGap_GetAdvData(const le_adv_data_params_t * params, unsigned index)
{
    PanicFalse(index == 0);

    DEBUG_LOG("gattServerGap_GetAdvData data_set: %d, completeness: %d,  placement:%d",
               params->data_set, params->completeness, params->placement);

    le_adv_data_item_t item = { .size = 0, .data = NULL };
    if(gattServerGap_IsNameReturned(params))
    {
        gattServerGap_GetItemDataName(&item);
        return item;
    }

    if(gattServerGap_IsFlagsReturned(params))
    {
        gattServerGap_GetItemDataFlags(&item);
        return item;
    }

    Panic();
    return item;
}

static void gattServerGap_ReleaseAdvData(const le_adv_data_params_t * params)
{
    DEBUG_LOG("gattServerGap_ReleaseAdvData data_set: %d, completeness: %d,  placement:%d",
               params->data_set, params->completeness, params->placement);

    if(gattServerGap_IsNameReturned(params))
    {
        gattServerGap_ReleaseItemDataName();
    }
}

static const le_adv_data_callback_t gattServerGap_AdvertisingManagerCallback =
{
    .GetNumberOfItems = gattServerGap_NumberOfAdvItems,
    .GetItem = gattServerGap_GetAdvData,
    .ReleaseItems = gattServerGap_ReleaseAdvData
};

#endif

bool GattServerGap_SetupLeAdvertisingData(void)
{
#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    gatt_server_gap_registered_item_name_handle = LeAdvertisingManager_RegisterAdvertisingItemCallback(NULL, &gattServerGap_AdvertisingManagerItemNameCallback);
    gatt_server_gap_registered_item_name_extended_handle = LeAdvertisingManager_RegisterAdvertisingItemCallback(NULL, &gattServerGap_AdvertisingManagerItemNameExtendedAdvertisingCallback);
    gatt_server_gap_registered_item_flags_handle = LeAdvertisingManager_RegisterAdvertisingFlagsCallback(NULL, &gattServerGap_AdvertisingManagerItemFlagsCallback);
#ifdef INCLUDE_LE_AUDIO_UNICAST
    gatt_server_gap_registered_item_appearance_handle = LeAdvertisingManager_RegisterAdvertisingItemCallback(NULL, &gattServerGap_AdvertisingManagerItemAppearanceCallback);
#endif
    return ((gatt_server_gap_registered_item_name_handle
            && gatt_server_gap_registered_item_name_extended_handle
            && gatt_server_gap_registered_item_flags_handle
#ifdef INCLUDE_LE_AUDIO_UNICAST
            && gatt_server_gap_registered_item_appearance_handle
#endif
            ) ? TRUE : FALSE);
#else
    gatt_server_gap_registered_handle = LeAdvertisingManager_Register(NULL, &gattServerGap_AdvertisingManagerCallback);
    return (gatt_server_gap_registered_handle ? TRUE : FALSE);
#endif
}

bool GattServerGap_UpdateLeAdvertisingData(void)
{
#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    return ((gatt_server_gap_registered_item_name_handle ? LeAdvertisingManager_UpdateAdvertisingItem(gatt_server_gap_registered_item_name_handle) : FALSE)
            && (gatt_server_gap_registered_item_name_extended_handle ? LeAdvertisingManager_UpdateAdvertisingItem(gatt_server_gap_registered_item_name_extended_handle) : FALSE)
            && (gatt_server_gap_registered_item_flags_handle ? LeAdvertisingManager_UpdateAdvertisingItem(gatt_server_gap_registered_item_flags_handle) : FALSE)
#ifdef INCLUDE_LE_AUDIO_UNICAST
            && (gatt_server_gap_registered_item_appearance_handle ? LeAdvertisingManager_UpdateAdvertisingItem(gatt_server_gap_registered_item_appearance_handle) : FALSE)
#endif
            );

#else
    return (gatt_server_gap_registered_handle ? LeAdvertisingManager_NotifyDataChange(NULL, gatt_server_gap_registered_handle) : FALSE);
#endif
}
