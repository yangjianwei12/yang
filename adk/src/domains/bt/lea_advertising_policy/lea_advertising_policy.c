/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       lea_advertising_policy.c
\brief      LEA Advertising policy interface implementations
*/

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

#include "lea_advertising_policy.h"
#include "lea_advertising_policy_private.h"
#include "lea_advertising_policy_config.h"

#include "device_properties.h"
#include "logging.h"
#include "cm_lib.h"

#define LEAP_LOG                         DEBUG_LOG

/*! LEA Advertising Policy taskdata */
lea_adv_policy_task_data_t lea_advert_policy_taskdata;

/* Static definition for Call backs for Undirected */
static bool leaAdvertisingPolicy_GetItemDataForUndirected(le_adv_item_data_t *data);
static bool leaAdvertisingPolicy_GetItemParamsForUndirected(le_adv_item_params_t *params);
static bool leaAdvertisingPolicy_GetItemInfoForUndirected(le_adv_item_info_t *info);
static void leaAdvertisingPolicy_ReleaseItemDataForUndirected(void);

/* Static definition for Call backs for Directed */
static bool leaAdvertisingPolicy_GetItemDataForDirected(le_adv_item_data_t *data);
static bool leaAdvertisingPolicy_GetItemParamsForDirected(le_adv_item_params_t *params);
static bool leaAdvertisingPolicy_GetItemInfoForDirected(le_adv_item_info_t *info);
static void leaAdvertisingPolicy_ReleaseItemDataForDirected(void);

/* LEA Advertising manager callbacks for undirected advertisements */
static const le_adv_item_callback_t leaAdvertisingPolicy_AdvertisingManagerCallbackForUndirected =
{
    .GetItemData  = leaAdvertisingPolicy_GetItemDataForUndirected,
    .ReleaseItemData = leaAdvertisingPolicy_ReleaseItemDataForUndirected,
    .GetItemInfo = leaAdvertisingPolicy_GetItemInfoForUndirected,
    .GetItemParameters = leaAdvertisingPolicy_GetItemParamsForUndirected,
};

/* LE Advertising manager callbacks for directed advertisements */
static const le_adv_item_callback_t leaAdvertisingPolicy_AdvertisingManagerCallbackForDirected =
{
    .GetItemData  = leaAdvertisingPolicy_GetItemDataForDirected,
    .ReleaseItemData = leaAdvertisingPolicy_ReleaseItemDataForDirected,
    .GetItemInfo = leaAdvertisingPolicy_GetItemInfoForDirected,
    .GetItemParameters = leaAdvertisingPolicy_GetItemParamsForDirected,
};

/* This function queries all the registered clients for their advert data size and 
 * computes the total advert size required.
 */
static uint8 leaAdvertisingPolicy_GetTotalAdvertDataSize(void)
{
    unsigned index = 0;
    unsigned total_advert_data_size = 0;

    while (index < lea_advert_policy_taskdata.lea_client_registry.registered_client_count)
    {
        const lea_adv_policy_clients_callback_t *callback = lea_advert_policy_taskdata.lea_client_registry.callbacks[index];
        total_advert_data_size += callback->GetAdvertisingDataSize();
        index++;
    }

    return total_advert_data_size;
}

/* Common function for directed and undirected to iterate through all the registered clients and collate the advert payload data adverts */
static bool leaAdvertisingPolicy_GetItemData(le_adv_item_data_t *item,
                                             le_adv_item_data_t *advert_data,
                                             const lea_adv_policy_adv_param_t *params)
{
    uint8 size_written;
    uint8 *write_ptr;
    unsigned index = 0;
    unsigned consumed = 0;
    uint16 total_advert_data_size = leaAdvertisingPolicy_GetTotalAdvertDataSize();

    PanicFalse(advert_data->data == NULL);
    advert_data->data = write_ptr = PanicUnlessMalloc(total_advert_data_size);

    LEAP_LOG("leaAdvertisingPolicy_GetItemData total payload %d", total_advert_data_size);

    while (index < lea_advert_policy_taskdata.lea_client_registry.registered_client_count)
    {
        PanicFalse(consumed <= total_advert_data_size);
        const lea_adv_policy_clients_callback_t *callback = lea_advert_policy_taskdata.lea_client_registry.callbacks[index];
        size_written = 0;

        size_written = callback->GetAdvertisingData(params, write_ptr, total_advert_data_size - consumed);
        write_ptr += size_written;
        consumed += size_written;
        index++;

        LEAP_LOG("leaAdvertisingPolicy_GetItemData Client %d", consumed);
    }

    advert_data->size = total_advert_data_size;
    *item = *advert_data;

    return TRUE;
}

/* Common function for directed and undirected to release item data */
static void leaAdvertisingPolicy_ReleaseItemData(le_adv_item_data_t *item)
{
    if (item->data != NULL)
    {
        free((void *)item->data);
        item->data = NULL;
    }
}

/* Common function for directed and undirected to Get Item info */
static bool leaAdvertisingPolicy_GetItemInfo(le_adv_item_info_t *info, le_adv_data_type_t adv_type, unsigned size)
{
    PanicNull(info);

    *info = (le_adv_item_info_t){ .placement = le_adv_item_data_placement_advert,
                                  .type = adv_type,
                                  .data_size = size};
    return TRUE;
}

/* Fill in the desired LE Advertising Parameters for Undirected adverts */
static bool leaAdvertisingPolicy_GetItemParamsForUndirected(le_adv_item_params_t *params)
{
    params->primary_adv_interval_min = leaAdvertisingPolicy_GetMinAdvIntervalForUndirected();
    params->primary_adv_interval_max = leaAdvertisingPolicy_GetMaxAdvIntervalForUndirected();

    return TRUE;
}

static bool leaAdvertisingPolicy_GetItemInfoForUndirected(le_adv_item_info_t *info)
{
    return leaAdvertisingPolicy_GetItemInfo(info,
                                            le_adv_type_extended_connectable,
                                            leaAdvertisingPolicy_GetTotalAdvertDataSize());
}

/* Release the allocated memory for undirected advert payload */
static void leaAdvertisingPolicy_ReleaseItemDataForUndirected(void)
{
    LEAP_LOG("leaAdvertisingPolicy_ReleaseItemDataForUndirected");
    leaAdvertisingPolicy_ReleaseItemData(&lea_advert_policy_taskdata.undirected_advert_config.adv_data);
}

/* Iterate through all the registered clients and collate the advert payload data for undirected adverts */
static bool leaAdvertisingPolicy_GetItemDataForUndirected(le_adv_item_data_t *item)
{
    LEAP_LOG("leaAdvertisingPolicy_GetItemDataForUndirected");
    lea_adv_policy_adv_param_t params = {0};
    params.audio_context = leaAdvertisingPolicy_GetUndirectedAudioContext();
    params.type = leaAdvertisingPolicy_GetUndirectedAnnouncementType();

    return leaAdvertisingPolicy_GetItemData(item,
                                            &lea_advert_policy_taskdata.undirected_advert_config.adv_data,
                                            &params);
}

/* Fill in the desired LE Advertising Parameters for Directed adverts */
static bool leaAdvertisingPolicy_GetItemParamsForDirected(le_adv_item_params_t *params)
{
    params->primary_adv_interval_min = leaAdvertisingPolicy_GetMinAdvIntervalForDirected();
    params->primary_adv_interval_max = leaAdvertisingPolicy_GetMaxAdvIntervalForDirected();
    memcpy(&params->peer_tpaddr, &lea_advert_policy_taskdata.peer_bd_addr, sizeof(typed_bdaddr));

    LEAP_LOG("leaAdvPolicy_GetItemParamsForDirected peer addr:[%u %04x:%02x:%06x]",
              params->peer_tpaddr.type,
              params->peer_tpaddr.addr.nap,
              params->peer_tpaddr.addr.uap,
              params->peer_tpaddr.addr.lap);

    return TRUE;
}

static void leaAdvertisingPolicy_ReleaseItemDataForDirected(void)
{
    LEAP_LOG("leaAdvertisingPolicy_ReleaseItemDataForDirected");
    leaAdvertisingPolicy_ReleaseItemData(&lea_advert_policy_taskdata.directed_advert_config.adv_data);
}

/* Iterate through all the registered clients and collate the advert payload data for directed adverts */
static bool leaAdvertisingPolicy_GetItemDataForDirected(le_adv_item_data_t *item)
{
    LEAP_LOG("leaAdvertisingPolicy_GetItemDataForDirected");
    lea_adv_policy_adv_param_t params = {0};
    params.audio_context = leaAdvertisingPolicy_GetDirectedAudioContext();
    params.type = leaAdvertisingPolicy_GetDirectedAnnouncementType();

    return leaAdvertisingPolicy_GetItemData(item,
                                            &lea_advert_policy_taskdata.directed_advert_config.adv_data,
                                            &params);
}

static bool leaAdvertisingPolicy_GetItemInfoForDirected(le_adv_item_info_t *info)
{
    return leaAdvertisingPolicy_GetItemInfo(info,
                                            le_adv_type_extended_directed,
                                            leaAdvertisingPolicy_GetTotalAdvertDataSize());
}

static void leaAdvertisingPolicy_UpdateUndirectedAdvertParamsAndData(lea_adv_policy_announcement_type_t undir_announce_type)
{
    leaAdvertisingPolicy_SetUndirectedAnnouncementType(undir_announce_type);

    LEAP_LOG("leaAdvertisingPolicy_UpdateUndirectedAdvertParamsAndData handle: %p, undir_announce_type: enum:lea_adv_policy_announcement_type_t:%d",
              lea_advert_policy_taskdata.undirected_advert_config.registered_adv_handle, undir_announce_type);

    if (lea_advert_policy_taskdata.undirected_advert_config.registered_adv_handle == NULL)
    {
        lea_advert_policy_taskdata.undirected_advert_config.registered_adv_handle =
            LeAdvertisingManager_RegisterAdvertisingItemCallback(NULL,
                                                                &leaAdvertisingPolicy_AdvertisingManagerCallbackForUndirected);
    }
    else
    {
        LeAdvertisingManager_UpdateAdvertisingItem(lea_advert_policy_taskdata.undirected_advert_config.registered_adv_handle);
    }
}

static void leaAdvertisingPolicy_UpdateDirectedAdvertParamsAndData(lea_adv_policy_announcement_type_t dir_announce_type,
                                                                   typed_bdaddr *peer_addr)
{
    PanicFalse(peer_addr != NULL);

    LEAP_LOG("leaAdvertisingPolicy_UpdateDirectedAdvertParamsAndData handle: %p, dir_announce_type: enum:lea_adv_policy_announcement_type_t:%d",
              lea_advert_policy_taskdata.directed_advert_config.registered_adv_handle, dir_announce_type);

    leaAdvertisingPolicy_SetDirectedAnnouncementType(dir_announce_type);

    memcpy(&lea_advert_policy_taskdata.peer_bd_addr, peer_addr, sizeof(typed_bdaddr));

    if (lea_advert_policy_taskdata.directed_advert_config.registered_adv_handle == NULL)
    {
        lea_advert_policy_taskdata.directed_advert_config.registered_adv_handle =
            LeAdvertisingManager_RegisterAdvertisingItemCallback(NULL,
                                                                &leaAdvertisingPolicy_AdvertisingManagerCallbackForDirected);
    }
    else
    {
        LeAdvertisingManager_UpdateAdvertisingItem(lea_advert_policy_taskdata.directed_advert_config.registered_adv_handle);
    }
}

static void leaAdvertisingPolicy_UnregisterAdvertising(le_adv_item_handle *advert_handle)
{
    if (*advert_handle != NULL)
    {
        LEAP_LOG("leaAdvertisingPolicy_UnregisterAdvertising");
        LeAdvertisingManager_UnregisterAdvertisingItem(*advert_handle);
        *advert_handle = NULL;
    }
}

void LeaAdvertisingPolicy_RegisterClient(const lea_adv_policy_clients_callback_t *const callback)
{
    if (lea_advert_policy_taskdata.lea_client_registry.registered_client_count < MAX_NUMBER_LEA_CLIENT_OBSERVERS)
    {
        /* The register requires that all callback functions have been supplied */
        PanicNull((void*)callback);
        PanicNull((void*)callback->GetAdvertisingDataSize);
        PanicNull((void*)callback->GetAdvertisingData);

        lea_advert_policy_taskdata.lea_client_registry.\
            callbacks[lea_advert_policy_taskdata.lea_client_registry.registered_client_count] = callback;
        lea_advert_policy_taskdata.lea_client_registry.registered_client_count++;

        LEAP_LOG("LeaAdvertisingPolicy_RegisterClient - Registered Client");
    }
    else
    {
        /* No space to store a new client, as this is a fixed number at compile time.
         * cause a panic so that more space can be reserved for clients.
         */
        Panic();
    }
}

void LeaAdvertisingPolicy_SetAdvertisingInterval (lea_adv_policy_mode_t mode, uint32 min_interval, uint32 max_interval)
{
    PanicFalse(min_interval != 0 && max_interval != 0);

    if (mode & lea_adv_policy_mode_undirected)
    {
        leaAdvertisingPolicy_SetMinAdvIntervalForUndirected(min_interval);
        leaAdvertisingPolicy_SetMaxAdvIntervalForUndirected(max_interval);
    }

    if (mode & lea_adv_policy_mode_directed)
    {
        leaAdvertisingPolicy_SetMinAdvIntervalForDirected(min_interval);
        leaAdvertisingPolicy_SetMaxAdvIntervalForDirected(max_interval);
    }
}

#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT
void LeaAdvertisingPolicy_SetAudioContext(lea_adv_policy_mode_t mode, audio_context_t sink_audio_context, audio_context_t source_audio_context)
{
    if (mode & lea_adv_policy_mode_undirected)
    {
        leaAdvertisingPolicy_SetUndirectedAudioContext(sink_audio_context, source_audio_context);
    }

    if (mode & lea_adv_policy_mode_directed)
    {
        leaAdvertisingPolicy_SetDirectedAudioContext(sink_audio_context, source_audio_context);
    }
}
#endif

uint32 LeaAdvertisingPolicy_GetAudioContext(lea_adv_policy_mode_t mode)
{
    if (mode & lea_adv_policy_mode_undirected)
    {
        return leaAdvertisingPolicy_GetUndirectedAudioContext();
    }

    if (mode & lea_adv_policy_mode_directed)
    {
        return leaAdvertisingPolicy_GetDirectedAudioContext();
    }

    return 0;
}

bool LeaAdvertisingPolicy_SetAdvertisingMode(lea_adv_policy_mode_t adv_mode,
                                             lea_adv_policy_announcement_type_t dir_announce_type,
                                             lea_adv_policy_announcement_type_t undir_announce_type,
                                             typed_bdaddr *peer_addr)
{

    LEAP_LOG("LeaAdvertisingPolicy_SetAdvertisingMode enum:lea_adv_policy_mode_t:%d", adv_mode);

    if (adv_mode & lea_adv_policy_mode_undirected)
    {
        leaAdvertisingPolicy_UpdateUndirectedAdvertParamsAndData(undir_announce_type);
    }
    else
    {
        LEAP_LOG("Unregistering lea_adv_policy_mode_undirected");
        /* Unregister undirected advertising in case of directed only mode */
        leaAdvertisingPolicy_UnregisterAdvertising(&lea_advert_policy_taskdata.undirected_advert_config.registered_adv_handle);
    }

    if (adv_mode & lea_adv_policy_mode_directed)
    {
        leaAdvertisingPolicy_UpdateDirectedAdvertParamsAndData(dir_announce_type, peer_addr);
    }
    else
    {
        LEAP_LOG("Unregistering lea_adv_policy_mode_directed");
        /* Unregister directed advertising in case of undirected only mode */
        leaAdvertisingPolicy_UnregisterAdvertising(&lea_advert_policy_taskdata.directed_advert_config.registered_adv_handle);
    }

    return TRUE;
}

bool LeaAdvertisingPolicy_UpdateAdvertisingItems(void)
{
    bool status = FALSE;

    if (lea_advert_policy_taskdata.undirected_advert_config.registered_adv_handle != NULL)
    {
        status = LeAdvertisingManager_UpdateAdvertisingItem(lea_advert_policy_taskdata.undirected_advert_config.registered_adv_handle);
    }

    if (lea_advert_policy_taskdata.directed_advert_config.registered_adv_handle != NULL)
    {
        status = LeAdvertisingManager_UpdateAdvertisingItem(lea_advert_policy_taskdata.directed_advert_config.registered_adv_handle);
    }

    return status;
}

bool LeaAdvertisingPolicy_Init(Task init_task)
{
    UNUSED(init_task);

    LEAP_LOG("LeaAdvertisingPolicy_Init");

    memset(&lea_advert_policy_taskdata, 0, sizeof(lea_adv_policy_task_data_t));

    /* Set the default settings for the directed adverts */
    leaAdvertisingPolicy_SetMinAdvIntervalForDirected(LEA_ADVERTISING_POLICY_DIRECTED_ADVERT_INTERVAL_MIN);
    leaAdvertisingPolicy_SetMaxAdvIntervalForDirected(LEA_ADVERTISING_POLICY_DIRECTED_ADVERT_INTERVAL_MAX);
    leaAdvertisingPolicy_SetDirectedAnnouncementType(lea_adv_policy_announcement_type_targeted);

    /* Set the default settings for the undirected adverts */
    leaAdvertisingPolicy_SetMinAdvIntervalForUndirected(LEA_ADVERTISING_POLICY_UNDIRECTED_ADVERT_INTERVAL_MIN);
    leaAdvertisingPolicy_SetMaxAdvIntervalForUndirected(LEA_ADVERTISING_POLICY_UNDIRECTED_ADVERT_INTERVAL_MAX);
    leaAdvertisingPolicy_SetUndirectedAnnouncementType(lea_adv_policy_announcement_type_general);

    return TRUE;
}

#endif
