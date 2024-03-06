/*!
\copyright  Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Module for setting up LE advertising & BREDR page scanning
*/

#include "charger_case_advertising.h"

/* framework includes */
#include <bredr_scan_manager.h>
#include <connection_manager.h>
#include <le_advertising_manager.h>
#include <local_addr.h>
/* system includes */
#include <connection.h>
#include <macros.h>
#include <panic.h>
#include <rtime.h>
#include <uuid.h>

#define UUID128_CHARGER_CASE_IDENTIFIER 00,00,cc,00,d1,02,11,e1,9b,23,00,02,5b,00,a5,a5
#define NUMBER_OF_ADVERT_DATA_ITEMS     1
#define SIZE_CHARGER_CASE_ADV_DATA_ITEM 18

#define LE_FAST_ADVERTISING_INTERVAL_MIN_SLOTS US_TO_BT_SLOTS(90000)
#define LE_FAST_ADVERTISING_INTERVAL_MAX_SLOTS US_TO_BT_SLOTS(100000)
#define LE_SLOW_ADVERTISING_INTERVAL_MIN_SLOTS US_TO_BT_SLOTS(225000)
#define LE_SLOW_ADVERTISING_INTERVAL_MAX_SLOTS US_TO_BT_SLOTS(250000)

#define TIMEOUT_FALLBACK_IN_SECONDS 10


/* Declarations for functions referred to by private data */
static unsigned int chargerCaseAdvertising_GetNumberOfAdvDataItems(const le_adv_data_params_t *params);
static le_adv_data_item_t chargerCaseAdvertising_GetAdvDataItem(const le_adv_data_params_t *params, unsigned int id);
static void chargerCaseAdvertising_ReleaseAdvDataItems(const le_adv_data_params_t *params);


/*! Advertising data item to identify device as a charger case */
static const uint8 charger_case_adv_data_item[SIZE_CHARGER_CASE_ADV_DATA_ITEM] = {
    SIZE_CHARGER_CASE_ADV_DATA_ITEM - 1,
    ble_ad_type_complete_uuid128,
    UUID_128_FORMAT_uint8(UUID128_CHARGER_CASE_IDENTIFIER)
};

/*! Callbacks to register advertising data item with LeAdvertisingManager */
static const le_adv_data_callback_t charger_case_adv_data_callbacks =
{
    .GetNumberOfItems = chargerCaseAdvertising_GetNumberOfAdvDataItems,
    .GetItem = chargerCaseAdvertising_GetAdvDataItem,
    .ReleaseItems = chargerCaseAdvertising_ReleaseAdvDataItems
};

/* Advertising parameters to pass to LE/BREDR scan managers */

static const bredr_scan_manager_scan_parameters_set_t inquiry_scan_params_set[] =
{
    {
        {
            [SCAN_MAN_PARAMS_TYPE_SLOW] = { .interval = US_TO_BT_SLOTS(2560000), .window = US_TO_BT_SLOTS(11250) },
            [SCAN_MAN_PARAMS_TYPE_FAST] = { .interval = US_TO_BT_SLOTS(320000),  .window = US_TO_BT_SLOTS(11250) },
        },
    },
};

static const bredr_scan_manager_scan_parameters_set_t page_scan_params_set[] =
{
    {
        {
            [SCAN_MAN_PARAMS_TYPE_SLOW] = { .interval = US_TO_BT_SLOTS(1280000), .window = US_TO_BT_SLOTS(22500) },
            [SCAN_MAN_PARAMS_TYPE_FAST] = { .interval = US_TO_BT_SLOTS(100000),  .window = US_TO_BT_SLOTS(11250) },
        },
    },
};

const bredr_scan_manager_parameters_t inquiry_scan_params =
{
    inquiry_scan_params_set, ARRAY_DIM(inquiry_scan_params_set)
};

static const bredr_scan_manager_parameters_t page_scan_params =
{
    page_scan_params_set, ARRAY_DIM(page_scan_params_set)
};

static const le_adv_parameters_set_t le_adv_params_set =
{
    {
        /* This is an ordered list, do not make any assumptions when changing the order of the list items */
        {LE_SLOW_ADVERTISING_INTERVAL_MIN_SLOTS, LE_SLOW_ADVERTISING_INTERVAL_MAX_SLOTS},
        {LE_FAST_ADVERTISING_INTERVAL_MIN_SLOTS, LE_FAST_ADVERTISING_INTERVAL_MAX_SLOTS}
    }
};

static const le_adv_parameters_config_table_t le_adv_params_config_table =
{
    {
        /* This is an ordered list, do not make any assumptions when changing the order of the list items */
        {le_adv_preset_advertising_interval_fast, 0},
        {le_adv_preset_advertising_interval_fast, TIMEOUT_FALLBACK_IN_SECONDS},
        {le_adv_preset_advertising_interval_slow, 0}
    }
};

const le_adv_parameters_t le_adv_params = {.sets = &le_adv_params_set, .table = &le_adv_params_config_table};


/* Private functions */

static bool chargerCaseAdvertising_AdvDataShouldBeIncluded(const le_adv_data_params_t *params)
{
    /* The charger_case_adv_data_item (a UUID indicating that this is a charger
       case product) should be included in full in any identifiable LE adverts.
    */
    return ((params->data_set == le_adv_data_set_handset_identifiable) &&
            (params->completeness == le_adv_data_completeness_full) &&
            (params->placement == le_adv_data_placement_advert)
           );
}

static unsigned int chargerCaseAdvertising_GetNumberOfAdvDataItems(const le_adv_data_params_t *params)
{
    if (chargerCaseAdvertising_AdvDataShouldBeIncluded(params))
    {
        return NUMBER_OF_ADVERT_DATA_ITEMS;
    }
    return 0;
}

static le_adv_data_item_t chargerCaseAdvertising_GetAdvDataItem(const le_adv_data_params_t *params, unsigned int id)
{
    UNUSED(id);
    le_adv_data_item_t adv_data_item = {.size = 0, .data = NULL};

    if (chargerCaseAdvertising_AdvDataShouldBeIncluded(params))
    {
        adv_data_item.size = SIZE_CHARGER_CASE_ADV_DATA_ITEM;
        adv_data_item.data = charger_case_adv_data_item;
    }
    return adv_data_item;
}

static void chargerCaseAdvertising_ReleaseAdvDataItems(const le_adv_data_params_t *params)
{
    UNUSED(params);
    return;
}


/* Public functions */

bool ChargerCaseAdvertising_Init(Task init_task)
{
    le_adv_item_handle adv_handle;

    /* Set scan parameters for BREDR & LE advertising */
    BredrScanManager_PageScanParametersRegister(&page_scan_params);
    BredrScanManager_InquiryScanParametersRegister(&inquiry_scan_params);
    PanicFalse(LeAdvertisingManager_ParametersRegister(&le_adv_params));
    PanicFalse(LeAdvertisingManager_ParametersSelect(0));

    /* Allow LE advertising & connections */
    ConManagerAllowHandsetConnect(TRUE);
    ConManagerAllowConnection(cm_transport_ble, TRUE);
    PanicFalse(LeAdvertisingManager_AllowAdvertising(init_task, TRUE));

    /* Set up advertising data to include 128-bit charger case UUID */
    adv_handle = LeAdvertisingManager_Register(NULL, &charger_case_adv_data_callbacks);

    return (adv_handle != NULL);
}

bool ChargerCaseAdvertising_EnableLePrivateAddresses(Task init_task)
{
    /* Configure Random Resolvable Private Address (RPA) advertising for LE */
    LocalAddr_ConfigureBleGeneration(init_task, local_addr_host_gen_resolvable,
                                                local_addr_controller_gen_none);
    return TRUE;
}
