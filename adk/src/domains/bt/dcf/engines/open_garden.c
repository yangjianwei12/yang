/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      A demo DCF implementation demonstarating how the DCF framework can be used to advertise
            context data in a multidevice ecosystem
*/

#include "dcf_config.h"

#ifdef INCLUDE_DCF_OPEN_GARDEN

#include "dcf_engine.h"
#include "dcf_core.h"
#include "bt_device.h"
#include "context_framework.h"
#include "le_advertising_manager.h"
#include "device_properties.h"
#include "local_name.h"
#include "macros.h"
#include "multidevice.h"

#include <logging.h>
#include <string.h>

#define DEBUG_LOG_DCF_OPEN_GARDEN       DEBUG_LOG
#define DEBUG_LOG_DATA_DCF_OPEN_GARDEN  DEBUG_LOG_DATA

typedef enum
{
    og_data_element_device_id = 0x40,
    og_data_element_device_info = 0x201,
    og_data_element_device_address = 0x202,
    og_data_element_device_battery = 0x203,
    og_data_element_active_audio_source = 0x206,
    og_data_element_connection_info = 0x207,
} open_garden_data_element_type_t;

typedef enum
{
    og_address_type_public,
    og_address_type_private,
} open_garden_address_type_t;

typedef enum
{
    og_device_type_unknown,
    og_device_type_phone,
    og_device_type_tablet,
    og_device_type_display,
    og_device_type_laptop,
    og_device_type_tv,
    og_device_type_watch,
    og_device_type_earbuds,
    og_device_type_headset,
} open_garden_device_type_t;

typedef enum
{
    og_physical_status_in_case,
    og_physical_status_out_of_case,
    og_physical_status_in_ear,
    og_physical_status_on_head,
} open_garden_device_physical_status_t;

typedef enum
{
    og_active_source_type_none,
    og_active_source_type_phone_call,
    og_active_source_type_voice,
    og_active_source_type_media,
} open_garden_active_source_type_t;

#define OPEN_GARDEN_HEADER_VERSION  DCF_V1_HEADER_VERSION
#define OPEN_GARDEN_UUID    0x1DD2

#define OPEN_GARDEN_DATA_ELEMENT_SET_SIZE_MAX   127
#define DCF_ADVERTISING_HEADER_SIZE             4
#define OPEN_GARDEN_ADVERTISING_SIZE_MAX        (DCF_ADVERTISING_HEADER_SIZE + OPEN_GARDEN_DATA_ELEMENT_SET_SIZE_MAX)

static const int8 advertising_header[DCF_ADVERTISING_HEADER_SIZE] =
{
    0, // to be populated based on data elements being advertised
    ble_ad_type_service_data,
    OPEN_GARDEN_UUID & 0xFF,
    (OPEN_GARDEN_UUID >> 8) & 0xFF,
};

static void dcfOpenGarden_HandleContextUpdate(Task task, MessageId id, Message message);
static TaskData contextUpdateHandler = { .handler = dcfOpenGarden_HandleContextUpdate };

static uint8 dcfOpenGarden_ConstructDeviceId(uint8 * data, uint8 max_data_size);
static uint8 dcfOpenGarden_ConstructDeviceInfo(uint8 * data, uint8 max_data_size);
static uint8 dcfOpenGarden_ConstructDeviceAddress(uint8 * data, uint8 max_data_size);
static uint8 dcfOpenGarden_ConstructActiveAudioSource(uint8 * data, uint8 max_data_size);
static uint8 dcfOpenGarden_ConstructConnectionInfo(uint8 * data, uint8 max_data_size);

static const dcf_data_element_constructor_t de_constructors[] =
{
    { .type = og_data_element_device_id, .constructor_function = dcfOpenGarden_ConstructDeviceId },
    { .type = og_data_element_device_info, .constructor_function = dcfOpenGarden_ConstructDeviceInfo },
    { .type = og_data_element_device_address, .constructor_function = dcfOpenGarden_ConstructDeviceAddress },
    { .type = og_data_element_active_audio_source, .constructor_function = dcfOpenGarden_ConstructActiveAudioSource },
    { .type = og_data_element_connection_info, .constructor_function = dcfOpenGarden_ConstructConnectionInfo },
};

static const dcf_data_element_set_t open_garden_data_element_set =
{  
    .constructors = (dcf_data_element_constructor_t *)de_constructors,
    .number_of_constructors = ARRAY_DIM(de_constructors), 
    .identity = dcf_v1_public_identity 
};

static le_adv_item_handle registered_advertising_handle = NULL;
static uint8 advertising_data[OPEN_GARDEN_ADVERTISING_SIZE_MAX] = { 0 };
static unsigned advertising_data_size = 0;

static inline uint8 dcfOpenGarden_GetDeviceType(void)
{
    return ((Multidevice_GetType() == multidevice_type_pair) ? og_device_type_earbuds : og_device_type_headset);
}

static inline uint16 dcfOpenGarden_GetDeviceStatus(void)
{
    uint16 open_garden_device_info = og_physical_status_out_of_case;
    context_physical_state_t physical_state;
    if(ContextFramework_GetContextItem(context_physical_state, (unsigned *)&physical_state, sizeof(context_physical_state_t)))
    {
        switch(physical_state)
        {
            case in_case:
                open_garden_device_info = og_physical_status_in_case;
                break;
            case out_of_case:
                open_garden_device_info = og_physical_status_out_of_case;
                break;
            case on_head:
                open_garden_device_info = og_physical_status_in_ear;
                break;
            default:
                break;
        }
    }
    return open_garden_device_info;
}

static inline int8 dcfOpenGarden_GetAddressType(void)
{
    return og_address_type_public;
}

static uint8 dcfOpenGarden_ConstructDeviceId(uint8 * data, uint8 max_data_size)
{
    PanicFalse(6 <= max_data_size);
    bdaddr bd_addr = { 0 };
    appDeviceGetMyBdAddr(&bd_addr);
    unsigned index = 0;
    data[index++] = (bd_addr.nap >> 8);
    data[index++] = bd_addr.nap;
    data[index++] = bd_addr.uap;
    data[index++] = (bd_addr.lap >> 16);
    data[index++] = (bd_addr.lap >> 8);
    data[index++] = bd_addr.lap;
    DEBUG_LOG_DCF_OPEN_GARDEN("dcfOpenGarden_ConstructDeviceId length=%d", index);
    DEBUG_LOG_DATA_DCF_OPEN_GARDEN(data, index);
    return index;
}

static uint8 dcfOpenGarden_ConstructDeviceInfo(uint8 * data, uint8 max_data_size)
{
    uint16 name_length = 0;
    const uint8 * name = LocalName_GetName(&name_length);
    uint8 total_length = (4 + name_length);
    PanicFalse(total_length <= max_data_size);
    unsigned index = 0;

    data[index++] = dcfOpenGarden_GetDeviceType();

    uint16 status = dcfOpenGarden_GetDeviceStatus();
    data[index++] = (status >> 8) & 0xFF;
    data[index++] = (status & 0xFF);

    data[index++] = name_length;
    memcpy(&data[index], name, name_length);

    DEBUG_LOG_DCF_OPEN_GARDEN("dcfOpenGarden_ConstructDeviceInfo length=%d", total_length);
    DEBUG_LOG_DATA_DCF_OPEN_GARDEN(data, total_length);
    return total_length;
}

static uint8 dcfOpenGarden_ConstructDeviceAddress(uint8 * data, uint8 max_data_size)
{
    PanicNull(data);
    PanicFalse(7 <= max_data_size);
    bdaddr bd_addr = { 0 };
    appDeviceGetMyBdAddr(&bd_addr);
    unsigned index = 0;
    data[index++] = dcfOpenGarden_GetAddressType();
    data[index++] = (bd_addr.nap >> 8);
    data[index++] = bd_addr.nap;
    data[index++] = bd_addr.uap;
    data[index++] = (bd_addr.lap >> 16);
    data[index++] = (bd_addr.lap >> 8);
    data[index++] = bd_addr.lap;
    DEBUG_LOG_DCF_OPEN_GARDEN("dcfOpenGarden_ConstructDeviceAddress length=%d", index);
    DEBUG_LOG_DATA_DCF_OPEN_GARDEN(data, index);
    return index;
}

static uint8 dcfOpenGarden_ConstructActiveAudioSource(uint8 * data, uint8 max_data_size)
{
    PanicNull(data);
    PanicFalse(1 <= max_data_size);
    context_active_source_info_t active_source_info;
    open_garden_active_source_type_t active_source_type = og_active_source_type_none;
    if(ContextFramework_GetContextItem(context_active_source_info, (unsigned *)&active_source_info, sizeof(context_active_source_info_t)))
    {
        if(active_source_info.active_source.type == source_type_voice)
        {
            active_source_type = og_active_source_type_phone_call;
        }
        else if(active_source_info.active_source.type == source_type_audio)
        {
            active_source_type = og_active_source_type_media;
        }
    }
    data[0] = active_source_type;
    DEBUG_LOG_DCF_OPEN_GARDEN("dcfOpenGarden_ConstructActiveAudioSource length=%d data=0x%x", 1, data[0]);
    return 1;
}

static uint8 dcfOpenGarden_ConstructConnectionInfo(uint8 * data, uint8 max_data_size)
{
    uint8 index = 0;
    context_connected_handsets_info_t connected_handsets_info = { .number_of_connected_handsets = 0, .connected_handsets = { 0 } };
    if(ContextFramework_GetContextItem(context_connected_handsets_info, (unsigned *)&connected_handsets_info, sizeof(context_connected_handsets_info_t)))
    {
        PanicNull(data);
        PanicFalse((1 + (connected_handsets_info.number_of_connected_handsets * sizeof(bdaddr))) <= max_data_size);
        data[index++] = connected_handsets_info.number_of_connected_handsets;
        for(int i = 0; i < connected_handsets_info.number_of_connected_handsets; i++)
        {
            tp_bdaddr connected_address = { 0 };
            if(BtDevice_GetTpBdaddrForDevice(connected_handsets_info.connected_handsets[i].handset, &connected_address))
            {
                data[index++] = (connected_address.taddr.addr.nap >> 8);
                data[index++] = connected_address.taddr.addr.nap;
                data[index++] = connected_address.taddr.addr.uap;
                data[index++] = (connected_address.taddr.addr.lap >> 16);
                data[index++] = (connected_address.taddr.addr.lap >> 8);
                data[index++] = connected_address.taddr.addr.lap;
            }
        }
        DEBUG_LOG_DCF_OPEN_GARDEN("dcfOpenGarden_ConstructConnectionInfo length=%d", index);
        DEBUG_LOG_DATA_DCF_OPEN_GARDEN(data, index);
    }
    return index;
}

static uint8 dcfOpenGarden_ConstructAdvertisingHeader(uint8 * data, uint8 max_data_size)
{
    PanicNull(data);
    PanicFalse(DCF_ADVERTISING_HEADER_SIZE <= max_data_size);
    memcpy(data, advertising_header, DCF_ADVERTISING_HEADER_SIZE);
    DEBUG_LOG_DCF_OPEN_GARDEN("dcfOpenGarden_ConstructAdvertisingHeader length=%d", DCF_ADVERTISING_HEADER_SIZE);
    DEBUG_LOG_DATA_DCF_OPEN_GARDEN(data, DCF_ADVERTISING_HEADER_SIZE);
    return DCF_ADVERTISING_HEADER_SIZE;
}

static void dcfOpenGarden_PopulateDataSizeInHeader(uint8 * data, uint8 advertising_length)
{
    DEBUG_LOG_DCF_OPEN_GARDEN("dcfOpenGarden_PopulateDataSizeInHeader length=%d", (advertising_length-1));
    data[0] = (advertising_length - 1);
}

static void dcfOopenGarden_ConstructAdvertisingData(void)
{
    memset(advertising_data, 0, OPEN_GARDEN_ADVERTISING_SIZE_MAX);
    unsigned index = 0;
    index += dcfOpenGarden_ConstructAdvertisingHeader(&advertising_data[index], OPEN_GARDEN_ADVERTISING_SIZE_MAX);
    
    index += Dcfv1format_ConstructDataElementSet(&advertising_data[index], (OPEN_GARDEN_ADVERTISING_SIZE_MAX-index), &open_garden_data_element_set);
    advertising_data_size = index;

    dcfOpenGarden_PopulateDataSizeInHeader(advertising_data, advertising_data_size);

    DEBUG_LOG_DCF_OPEN_GARDEN("dcfOopenGarden_ConstructAdvertisingData length=%d", advertising_data_size);
    DEBUG_LOG_DATA_DCF_OPEN_GARDEN(advertising_data, advertising_data_size);

    if(registered_advertising_handle)
    {          
        LeAdvertisingManager_UpdateAdvertisingItem(registered_advertising_handle);
    }
}

static void dcfOpenGarden_HandleContextUpdate(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch(id)
    {
        case context_physical_state:
        case context_active_source_info:
        case context_connected_handsets_info:
            dcfOopenGarden_ConstructAdvertisingData();
            break;
        default:
            Panic();
            break;
    }
}

static uint8 dcfOpenGarden_GetItemDataSize(void)
{
    if(advertising_data_size == 0)
    {
        dcfOopenGarden_ConstructAdvertisingData();
    }
    return advertising_data_size;
}

static bool dcfOpenGarden_GetItemData(le_adv_item_data_t * data)
{
    PanicNull(data);
    data->size = dcfOpenGarden_GetItemDataSize();
    data->data = advertising_data;
    return TRUE;
}

static bool dcfOpenGarden_GetItemInfo(le_adv_item_info_t * info)
{
    PanicNull(info);
    *info = (le_adv_item_info_t){ .placement = le_adv_item_data_placement_advert,
                                        .type = le_adv_type_extended_connectable,
                                        .data_size = dcfOpenGarden_GetItemDataSize() };
    return TRUE;
}

static const le_adv_item_callback_t dcfOpenGarden_advertising_callbacks = 
{
    .GetItemData = &dcfOpenGarden_GetItemData,
    .GetItemInfo = &dcfOpenGarden_GetItemInfo,
};

void DcfOpenGarden_Init(void)
{
    ContextFramework_RegisterContextConsumer(context_physical_state, &contextUpdateHandler);
    ContextFramework_RegisterContextConsumer(context_active_source_info, &contextUpdateHandler);
    ContextFramework_RegisterContextConsumer(context_connected_handsets_info, &contextUpdateHandler);

    registered_advertising_handle = LeAdvertisingManager_RegisterAdvertisingItemCallback(NULL, &dcfOpenGarden_advertising_callbacks);
}

#endif