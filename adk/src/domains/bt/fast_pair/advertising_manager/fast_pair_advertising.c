/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file        fast_pair_advertising.c
\brief      Handles Fast Pair Advertising Data
*/

/*! Firmware and Library Headers */
#include <util.h>
#include <stdlib.h>
#include <message.h>
#include <logging.h>
#include <panic.h>
#include <stdio.h>
#include <connection_manager.h>
#include <connection.h>
#include <string.h>
#include <cryptovm.h>

/*! Application Headers */
#include "le_advertising_manager.h"
#include "fast_pair_advertising.h"
#include "fast_pair_bloom_filter.h"
#include "fast_pair.h"
#include "fast_pair_config.h"
#include "fast_pair_session_data.h"
#include "tx_power.h"
#include "fast_pair_battery_notifications.h"
#include "user_accounts.h"
#include "fast_pair_adv_sass.h"

#define DEBUG_LOG_FP_ADVERTISING        DEBUG_LOG
#define DEBUG_LOG_FP_ADVERTISING_DATA   DEBUG_LOG_DATA

/*! \brief Global data structure for fastpair adverts */
typedef struct
{
    /*! The fast pair advertising module task */
    TaskData task;
    le_adv_item_handle adv_register_handle;
    bdaddr  adv_bdaddr;
    uint8   *account_key_filter_adv_data;
    bool    identifiable;
    bool    in_use_account_key_active;
}fastpair_advert_data_t;

/*! Global Instance of fastpair advertising data */
fastpair_advert_data_t fastpair_advert;

Task fastPair_AdvGetAdvTask(void)
{
    return (&fastpair_advert.task);
}


typedef enum
{
    FAST_PAIR_ADVERTISING_INTERNAL_RETAIN_RPA,
} fast_pair_advertising_internal_msgs_t;

#define FAST_PAIR_ADVERTISING_RETAIN_RPA_TIMEOUT_MS    (D_SEC(120))

/*! Module Constants */
#define FAST_PAIR_GFPS_IDENTIFIER 0xFE2C

#define FAST_PAIR_ADV_ITEM_MODEL_ID 0 /*During BR/EDR Connectable and Discoverable*/
#define FAST_PAIR_ADV_ITEM_BLOOM_FILTER_ID 0 /*During BR/EDR Connectable and Non-Discoverable*/

/*GFPS Identifier, Model ID in Identifiable mode and GFPS Identifier, Hashed Account key (Bloom Filter) including Salt in Unidentifiable mode*/
/*Note transmit power needed for fastpair adverts will go as part of Tx_power module*/
#define FAST_PAIR_AD_ITEMS_IDENTIFIABLE 1
#define FAST_PAIR_AD_ITEMS_UNIDENTIFIABLE_WITH_ACCOUNT_KEYS 1

/*All Adv Interval values are in ms (units of 0.625)*/
/* Adv interval when BR/EDR is discoverable should be <=100ms*/
#define FP_ADV_INTERVAL_IDENTIFIABLE_MIN     128
#define FP_ADV_INTERVAL_IDENTIFIABLE_MAX     160

/*FP when BR/EDR non-discoverable/Silent Pairing*/
#define FP_ADV_INTERVAL_UNIDENTIFIABLE_MIN   320
#define FP_ADV_INTERVAL_UNIDENTIFIABLE_MAX   400

/*! Const fastpair advertising data in Length, Tag, Value format*/
#define FP_SIZE_AD_TYPE_FIELD 1 
#define FP_SIZE_LENGTH_FIELD 1

/* Flags in Advertising Payload when in non-discoverable mode(All bits are reserved for future use)*/
#define FP_ADV_PAYLOAD_FLAGS 0x00
/* Account Key data when in non-discoverable mode and no acocunt keys are present*/
#define FP_ADV_PAYLOAD_ACCOUNT_KEY_DATA 0x00

#define SIZE_GFPS_ID 2
/* Length of account key data when non-discoverable and no account keys are present*/
#define ACCOUNT_DATA_LEN 2
#define SIZE_GFPS_ID_ADV (FP_SIZE_LENGTH_FIELD+FP_SIZE_AD_TYPE_FIELD+SIZE_GFPS_ID)
/* Size of adverising data when non-discoverable and no account keys are present*/
#define SIZE_ACCOUNT_DATA_ADV (SIZE_GFPS_ID_ADV+ACCOUNT_DATA_LEN)

static bool IsHandsetConnAllowed = FALSE;

static const uint8 fp_account_data_adv[SIZE_ACCOUNT_DATA_ADV] = 
{ 
    SIZE_ACCOUNT_DATA_ADV - 1,
    ble_ad_type_service_data, 
    FAST_PAIR_GFPS_IDENTIFIER & 0xFF, 
    (FAST_PAIR_GFPS_IDENTIFIER >> 8) & 0xFF,
    FP_ADV_PAYLOAD_FLAGS,
    FP_ADV_PAYLOAD_ACCOUNT_KEY_DATA
};

static const le_adv_item_data_t fp_account_data_data_item =
{
    SIZE_ACCOUNT_DATA_ADV,
    fp_account_data_adv
};

/*! Const fastpair advertising data in Length, Tag, Value format*/
#define SIZE_MODEL_ID 3
#define SIZE_MODEL_ID_ADV (FP_SIZE_LENGTH_FIELD+FP_SIZE_AD_TYPE_FIELD+SIZE_MODEL_ID + SIZE_GFPS_ID)

static le_adv_item_data_t fp_model_id_data_item;
static uint8 fp_model_id_adv[SIZE_MODEL_ID_ADV];

#ifdef USE_SYNERGY
static void appHandleCmPrim(Message message)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    switch (*prim)
    {

        case CSR_BT_CM_CRYPTO_HASH_CFM:
            DEBUG_LOG("FastPair_AdvHandleMessage appHandleCmPrim - CSR_BT_CM_CRYPTO_HASH_CFM");
            fastPair_AdvHandleHashCfm((CsrBtCmCryptoHashCfm *)message);
        break;

        default:
            DEBUG_LOG("FastPair_AdvHandleMessage appHandleCmPrim, unexpected CM prim 0x%04x", *prim);
            break;
    }

    CmFreeUpstreamMessageContents(message);
}
#endif /* USE_SYNERGY */

/*! \brief Message Handler

    This function is the main message handler for the fast pair advertising module.
*/
static void FastPair_AdvHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
#ifdef USE_SYNERGY
        case CM_PRIM:
            appHandleCmPrim(message);
            break;
#endif /* USE_SYNERGY */

#ifndef USE_SYNERGY
        case CL_CRYPTO_HASH_CFM:
            fastPair_AdvHandleHashCfm((CL_CRYPTO_HASH_CFM_T *)message);
        break;
#endif /* !USE_SYNERGY */

        case FAST_PAIR_ADVERTISING_INTERNAL_RETAIN_RPA:
        {
            DEBUG_LOG_INFO("FastPair_AdvHandleMessage, FAST_PAIR_ADVERTISING_INTERNAL_RETAIN_RPA timeout, updating RPA");
            fastPair_AdvNotifyDataChange();
        }
        break;


        default:
            DEBUG_LOG("Unhandled MessageID = MSG:fast_pair_state_event_id:0x%d", id);
        break;
    }
}

/*! \brief Provide fastpair advert data when Identifiable (i.e. BR/EDR discoverable)

    Each data item in GetItems will be invoked separately by Adv Mgr, more precisely, one item per AD type.
*/
static inline le_adv_item_data_t fastPair_GetDataIdentifiable(void)
{
    DEBUG_LOG_FP_ADVERTISING("FP ADV: fastPair_GetDataIdentifiable: Model Id data item");
    return fp_model_id_data_item;
}

/*! \brief Get the advert data for account key filter
*/
static le_adv_item_data_t fastPairGetAccountKeyFilterAdvData(bool compute_size_only)
{
    le_adv_item_data_t data_item;
    uint16 bloom_filter_size = fastPairGetBloomFilterLen();
    uint16 adv_size=0;

    if (fastpair_advert.account_key_filter_adv_data)
    {
        free(fastpair_advert.account_key_filter_adv_data);
        fastpair_advert.account_key_filter_adv_data=NULL;
    }

    if (bloom_filter_size)
    {
        /* calculate total size of advert data
            - service type and length
            - bloom filter
            - optionally battery notifications (may be 0)
            - mandatory random resovable data (encrypted connection status field for SASS)
         */
        adv_size = SIZE_GFPS_ID_ADV + bloom_filter_size + FP_BATTERY_NOTFICATION_SIZE + fastPair_SASSGetAdvDataSize();

        if(!compute_size_only)
        {
            uint8 index = 0;
            fastpair_advert.account_key_filter_adv_data = PanicUnlessMalloc(adv_size);
            fastpair_advert.account_key_filter_adv_data[index++] = adv_size-FP_SIZE_LENGTH_FIELD;
            fastpair_advert.account_key_filter_adv_data[index++] = ble_ad_type_service_data;
            fastpair_advert.account_key_filter_adv_data[index++] = (uint8)(FAST_PAIR_GFPS_IDENTIFIER & 0xFF);
            fastpair_advert.account_key_filter_adv_data[index++] = (uint8)((FAST_PAIR_GFPS_IDENTIFIER >> 8) & 0xFF);
            memcpy(&fastpair_advert.account_key_filter_adv_data[index], fastPairGetBloomFilterData(), bloom_filter_size);
            index += bloom_filter_size;
            /* add optional battery state data if available */
            if (FP_BATTERY_NOTFICATION_SIZE)
            {
                memcpy(&fastpair_advert.account_key_filter_adv_data[index], fastPair_BatteryGetData(), FP_BATTERY_NOTFICATION_SIZE);
                index += FP_BATTERY_NOTFICATION_SIZE;
            }
            if(fastPair_SASSGetAdvDataSize())
            {
                /* Add mandatory Random Resolvable Data for SASS feature */
                memcpy(&fastpair_advert.account_key_filter_adv_data[index], fastPair_SASSGetAdvData(), fastPair_SASSGetAdvDataSize());
            }
            DEBUG_LOG_FP_ADVERTISING("fastPairGetAccountKeyFilterAdvData advert size=%d", adv_size);
            DEBUG_LOG_FP_ADVERTISING_DATA(fastpair_advert.account_key_filter_adv_data, adv_size);
        }
        else
        {
            DEBUG_LOG_FP_ADVERTISING("FP ADV: fastPairGetAccountKeyFilterAdvData: bloom_filter_size %d adv_size %d", bloom_filter_size, adv_size);
        }
    }

    data_item.size = adv_size;
    data_item.data = fastpair_advert.account_key_filter_adv_data;

    return data_item;
}

/*! \brief Provide fastpair advert data when Unidentifiable (i.e. BR/EDR non-discoverable)

    Each data item in GetItems will be invoked separately by Adv Mgr, more precisely, one item per AD type.

    \param  compute_size_only TRUE means the caller is only interested in the size of data, so avoid any
            allocating memory for actual data
*/
static le_adv_item_data_t fastPair_GetDataUnIdentifiable(bool compute_size_only)
{
    le_adv_item_data_t data_item={0};
    if(!UserAccounts_GetNumAccountKeys())
    {
        DEBUG_LOG_FP_ADVERTISING("FP ADV: fastPair_GetDataUnIdentifiable: GFPS Id data item with empty Account Key");
        data_item = fp_account_data_data_item;
    }
    else
    {
        DEBUG_LOG_FP_ADVERTISING("FP ADV: fastPair_GetDataUnIdentifiable: GFPS Id data item with Account Key");
        data_item = fastPairGetAccountKeyFilterAdvData(compute_size_only);
        fastpair_advert.in_use_account_key_active = FastPair_SASSGetInUseAccountKeyActiveFlag();
#ifdef USE_FAST_PAIR_ACCOUNT_DATA_V0
        /*Generate new bloom filter and keep it ready for advertisements in BR/EDR Connectable and non-discoverable mode
        This will ensure next callback would have new Salt*/
        DEBUG_LOG("FP ADV: fastPair_GetDataUnIdentifiable: fastPair_GenerateBloomFilter\n");
        fastPairTaskData * theFastPair = fastPair_GetTaskData();
        /* Generating Bloom filter only In Idle state(fresh pair) and Wait Account Key state(Subsequent pair)*/
        if((fastPair_GetState(theFastPair) == FAST_PAIR_STATE_IDLE) || (fastPair_GetState(theFastPair) == FAST_PAIR_STATE_WAIT_ACCOUNT_KEY))
        {
            fastPair_GenerateBloomFilter();
        }
#endif
    }
    return data_item;
}

static inline void fastPair_ReleaseAdvertisingDataItem(void)
{
    if (fastpair_advert.account_key_filter_adv_data)
    {
        free(fastpair_advert.account_key_filter_adv_data);
        fastpair_advert.account_key_filter_adv_data=NULL;
    }
}

#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

static inline bool fastPair_ShouldAdvertiseIdentifiable(void)
{
    return fastpair_advert.identifiable;
}

static unsigned fastPair_GetItemDataSize(void)
{
    return fastPair_ShouldAdvertiseIdentifiable()
            ? fastPair_GetDataIdentifiable().size
                : fastPair_GetDataUnIdentifiable(TRUE).size;
}

static bool fastPair_GetItemData(le_adv_item_data_t * data)
{
    PanicNull(data);
    data->data = NULL;
    data->size = 0;

    *data = fastPair_ShouldAdvertiseIdentifiable()
            ? fastPair_GetDataIdentifiable()
                : fastPair_GetDataUnIdentifiable(FALSE);
    return TRUE;
}

static void fastPair_ReleaseItemData(void)
{
    fastPair_ReleaseAdvertisingDataItem();
}

static bool fastPair_GetItemInfo(le_adv_item_info_t * info)
{
    PanicNull(info);
    *info = (le_adv_item_info_t){ .placement = le_adv_item_data_placement_advert,
                                        .type = le_adv_type_legacy_connectable_scannable,
                                        .data_size = fastPair_GetItemDataSize()};
    return TRUE;
}

static bool fastPair_GetItemParams(le_adv_item_params_t * params)
{
    PanicNull(params);
    bdaddr mru_adv_addr = fastPair_AdvGetBdaddr();

    BdaddrTypedSetEmpty(&params->random_addr);

    if (MessagePendingFirst(fastPair_AdvGetAdvTask(), FAST_PAIR_ADVERTISING_INTERNAL_RETAIN_RPA, NULL) &&
        !BdaddrIsZero(&mru_adv_addr))
    {
        params->random_addr_type = ble_local_addr_write_resolvable;
        params->random_addr.type = TYPED_BDADDR_RANDOM;
        params->random_addr.addr = mru_adv_addr;
    }
    else
    {
        params->random_addr_type = ble_local_addr_generate_resolvable;
    }
    
    DEBUG_LOG_INFO("fastPair_GetItemParams, enum:ble_local_addr_type:%u, lap=%06lx", params->random_addr_type, params->random_addr.addr.lap);

    params->random_addr_generate_rotation_timeout_minimum_in_minutes = 14;
    params->random_addr_generate_rotation_timeout_maximum_in_minutes = 15;

    if(fastPair_ShouldAdvertiseIdentifiable())
    {
        params->primary_adv_interval_max = MSEC_TO_LE_TIMESLOT(100);
        params->primary_adv_interval_min = MSEC_TO_LE_TIMESLOT(90);
    }
    else
    {
        params->primary_adv_interval_max = MSEC_TO_LE_TIMESLOT(250);
        params->primary_adv_interval_min = MSEC_TO_LE_TIMESLOT(225);
    }

    return TRUE;
}

static void fastPair_HandleAdvertisingEventNotified(le_adv_event_t event, const void * event_data)
{
    DEBUG_LOG_FP_ADVERTISING("FP ADV: fastPair_HandleAdvertisingEvent event ID enum:le_adv_event_t:%d", event);

    if(event == LEAM_EVENT_ADVERTISING_SET_SUSPENDED)
    {
#ifdef USE_FAST_PAIR_ACCOUNT_DATA_V0
        fastPair_AdvNotifyDataChange();
#else
        // RPA will change when the set is next enabled so recalculate the bloom filter in 
        // anticipation of this
        fastPair_GenerateBloomFilter();
#endif
    }
    else if (event == LEAM_EVENT_ADVERTISING_SET_RANDOM_ADDRESS_CHANGED)
    {
        const LEAM_EVENT_ADVERTISING_SET_RANDOM_ADDRESS_CHANGED_T * data = (const LEAM_EVENT_ADVERTISING_SET_RANDOM_ADDRESS_CHANGED_T *)event_data;

        DEBUG_LOG_FP_ADVERTISING("fastPair_HandleAdvertisingEventNotified: new addr LAP: 0x%06lx", data->new_bdaddr.lap);
        fastpair_advert.adv_bdaddr = data->new_bdaddr;
    }
}

static const le_adv_item_callback_t fastPair_advertising_callback = {
    .GetItemData = &fastPair_GetItemData,
    .ReleaseItemData = &fastPair_ReleaseItemData,
    .GetItemParameters = &fastPair_GetItemParams,
    .NotifyAdvertisingEvent = &fastPair_HandleAdvertisingEventNotified,
    .GetItemInfo = &fastPair_GetItemInfo

};

#else

/*! \brief Query the advertisement interval and check if it in expected range*/
static void fastpair_CheckAdvIntervalInRange(le_adv_data_set_t data_set)
{
    le_adv_common_parameters_t adv_int = {0};

    /* Get the currently used advertising parameters */
    bool status = LeAdvertisingManager_GetAdvertisingInterval(&adv_int);
    DEBUG_LOG_FP_ADVERTISING("FP ADV: fastpair_CheckAdvIntervalInRange adv_int_min = %d, adv_int_max = %d", adv_int.le_adv_interval_min, adv_int.le_adv_interval_max);

    if(status)
    {
        switch(data_set)
        {
            case le_adv_data_set_handset_identifiable:
            /*  This is more stringent check as the max value returned might be above 100ms but actual adv interval
                might still be conforming to FASTPAIR discoverable requirement*/
                if(adv_int.le_adv_interval_max > FP_ADV_INTERVAL_IDENTIFIABLE_MAX)
                {
                    DEBUG_LOG_FP_ADVERTISING("FP ADV: fastpair_CheckAdvIntervalInRange: Adv interval might be non compliant with Fastpair Standard");
                }
            break;

            case le_adv_data_set_handset_unidentifiable:
            /*  This is more stringent check as the max value returned might be above 250ms but actual adv interval
                might still be conforming to FASTPAIR non-discoverable requirement*/
                if(adv_int.le_adv_interval_max > FP_ADV_INTERVAL_UNIDENTIFIABLE_MAX)
                {
                    DEBUG_LOG_FP_ADVERTISING("FP ADV: fastpair_CheckAdvIntervalInRange: Adv interval might be non compliant with Fastpair Standard");
                }
            break;

            case le_adv_data_set_peer:
                DEBUG_LOG_FP_ADVERTISING("FP ADV: fastpair_CheckAdvIntervalInRange: Non-connectable");
            break;

            default:
                DEBUG_LOG_FP_ADVERTISING("FP ADV: fastpair_CheckAdvIntervalInRange: Invalid advertisement dataset");
            break;
        }
    }
    else
    {
        DEBUG_LOG_FP_ADVERTISING("FP ADV: fastpair_CheckAdvIntervalInRange: Failed to get the LE advertising interval values");
    }
}


#define FASTPAIR_ADV_PARAMS_REQUESTED(params) ((params->completeness == le_adv_data_completeness_full) && \
            (params->placement == le_adv_data_placement_advert))

#define IS_IDENTIFIABLE(data_set) (data_set == le_adv_data_set_handset_identifiable)
#define IS_UNIDENTIFIABLE(data_set) (data_set == le_adv_data_set_handset_unidentifiable)


/*! \brief Provide the number of items expected to go in adverts for a given mode

      Advertising Manager is expected to retrive the number of items first before the fastPair_AdvGetDataItem() callback

      For fastpair there wont be any adverts in case of le_adv_data_completeness_can_be_shortened/skipped
*/
static unsigned int fastPair_AdvGetNumberOfItems(const le_adv_data_params_t * params)
{
    unsigned int number=0;

    if(params->data_set != le_adv_data_set_peer)
    {
        bool identifiable = (params->data_set == le_adv_data_set_handset_identifiable ? TRUE : FALSE);
        fastpair_advert.identifiable = identifiable;
        /* Add debug logs if existing advertising interval is not in range */
        fastpair_CheckAdvIntervalInRange(params->data_set);

        /*Check for BR/EDR connectable*/
        if (IsHandsetConnAllowed && FASTPAIR_ADV_PARAMS_REQUESTED(params))
        {
            if (IS_IDENTIFIABLE(params->data_set))
            {
                number = FAST_PAIR_AD_ITEMS_IDENTIFIABLE;
            }
            else if (IS_UNIDENTIFIABLE(params->data_set))
            {
                number = FAST_PAIR_AD_ITEMS_UNIDENTIFIABLE_WITH_ACCOUNT_KEYS;
            }
        }
        else
        {
            DEBUG_LOG_FP_ADVERTISING("FP ADV: fastPair_AdvGetNumberOfItems: Non-connectable");
        }
    }

    return number;
}

/*! \brief Provide the advertisement data expected to go in adverts for a given mode

    Each data item in GetItems will be invoked separately by Adv Mgr, more precisely, one item per AD type.
*/
static le_adv_data_item_t fastPair_AdvGetDataItem(const le_adv_data_params_t * params, unsigned int id)
{
    UNUSED(id);
    le_adv_data_item_t data_item={0};

    if(params->data_set != le_adv_data_set_peer)
    {
        if (IsHandsetConnAllowed && FASTPAIR_ADV_PARAMS_REQUESTED(params))
        {
            if (IS_IDENTIFIABLE(params->data_set))
            {
                return fastPair_GetDataIdentifiable();
            }
            else if (IS_UNIDENTIFIABLE(params->data_set))
            {
                return fastPair_GetDataUnIdentifiable(FALSE);
            }
        }
    }

    return data_item;
}


/*! \brief Release any allocated fastpair data

      Advertising Manager is expected to retrive the number of items first before the fastPair_AdvGetDatatems() callback
*/
static void fastPair_ReleaseItems(const le_adv_data_params_t * params)
{
    if (FASTPAIR_ADV_PARAMS_REQUESTED(params) && IS_UNIDENTIFIABLE(params->data_set))
    {        
        fastPair_ReleaseAdvertisingDataItem();
    }
}

/*! Callback registered with LE Advertising Manager*/
static const le_adv_data_callback_t fastPair_advertising_callback = {
    .GetNumberOfItems = &fastPair_AdvGetNumberOfItems,
    .GetItem = &fastPair_AdvGetDataItem,
    .ReleaseItems = &fastPair_ReleaseItems
};

#endif

/*! \brief Function to initialise the fastpair advertising globals
*/
static void fastPair_InitialiseAdvGlobal(void)
{
    memset(&fastpair_advert, 0, sizeof(fastpair_advert_data_t));
    fastpair_advert.in_use_account_key_active = FALSE;
}

static void fastPair_ModelIdAdvData(void)
{
    uint32 fp_model_id;

    fp_model_id = fastPair_GetModelId();
    DEBUG_LOG_FP_ADVERTISING("fastPair_ModelIdAdvData %04x", fp_model_id);

    fp_model_id_adv[0] = SIZE_MODEL_ID_ADV - 1;
    fp_model_id_adv[1] = ble_ad_type_service_data;
    fp_model_id_adv[2] = FAST_PAIR_GFPS_IDENTIFIER & 0xFF;
    fp_model_id_adv[3] = (FAST_PAIR_GFPS_IDENTIFIER >> 8) & 0xFF;
    fp_model_id_adv[4] = (fp_model_id >> 16) & 0xFF;
    fp_model_id_adv[5] = (fp_model_id >> 8) & 0xFF;
    fp_model_id_adv[6] = fp_model_id & 0xFF;

    fp_model_id_data_item.size = SIZE_MODEL_ID_ADV;
    fp_model_id_data_item.data = fp_model_id_adv;
}

/*! @brief Private API to initialise fastpair
 */
bool fastPair_SetUpAdvertising(void)
{
    DEBUG_LOG_FP_ADVERTISING("FP ADV: fastPair_SetUpAdvertising");
    /*Initialise fastpair advertising globals*/
    fastPair_InitialiseAdvGlobal();

    /* Initialise Fast Pair SASS advertising module */
    fastPair_SetUpSASSAdvertising();

    /* Setup Fast Pair Model ID advertising data */
    fastPair_ModelIdAdvData();

    /*Initialise fastpair bloom filter globals*/
    fastPair_InitBloomFilter();
    
    /*PreCalculate Bloom Filter with available account filters, if any*/
    fastPair_GenerateBloomFilter();

    /*Mandate use of Transmit Power in fastpair adverts*/
    TxPower_Mandatory(TRUE, le_client_fast_pair);

    if (fastpair_advert.adv_register_handle!=NULL)
    {
        DEBUG_LOG_FP_ADVERTISING("FP ADV: fastPair_SetUpAdvertising: Adv Handle NOT NULL");
    }

    /*Register callback with Advertising Manager*/
#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    fastpair_advert.adv_register_handle = LeAdvertisingManager_RegisterAdvertisingItemCallback(NULL, &fastPair_advertising_callback);
#else
    fastPairTaskData *fast_pair_task_data = fastPair_GetTaskData();
    fastpair_advert.adv_register_handle = LeAdvertisingManager_Register(&fast_pair_task_data->task, &fastPair_advertising_callback);
#endif
    /* Set up task handler */
    fastPair_AdvGetAdvTask()->handler = FastPair_AdvHandleMessage;

    return (fastpair_advert.adv_register_handle ? TRUE : FALSE);
}

/*! @brief Private API to handle change in Connectable state and notify the LE Advertising Manager
 */
bool fastPair_AdvNotifyChangeInConnectableState(bool connectable)
{
    bool notify = FALSE;
    if(IsHandsetConnAllowed != connectable)
    {
        IsHandsetConnAllowed = connectable;
        DEBUG_LOG_FP_ADVERTISING("fastPair_AdvNotifyChangeInConnectableState %d", connectable);
        notify = fastPair_AdvNotifyDataChange();
    }
    return notify;
}

void fastPair_AdvNotifyPairingSuccess(void)
{
    DEBUG_LOG_INFO("fastPair_AdvNotifyPairingSuccess, retain RPA for %u ms", FAST_PAIR_ADVERTISING_RETAIN_RPA_TIMEOUT_MS);

    MessageCancelAll(fastPair_AdvGetAdvTask(), FAST_PAIR_ADVERTISING_INTERNAL_RETAIN_RPA);
    MessageSendLater(fastPair_AdvGetAdvTask(), FAST_PAIR_ADVERTISING_INTERNAL_RETAIN_RPA, NULL, FAST_PAIR_ADVERTISING_RETAIN_RPA_TIMEOUT_MS);
}


bool fastPair_AdvNotifyChangeInIdentifiable(bool identifiable)
{
#ifdef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
    fastpair_advert.identifiable = identifiable;
    DEBUG_LOG_FP_ADVERTISING("fastPair_AdvNotifyChangeInIdentifiable %d", fastpair_advert.identifiable);
    return TRUE;
#else
    bool notify = FALSE;
    if(fastpair_advert.identifiable != identifiable)
    {
        fastpair_advert.identifiable = identifiable;
        DEBUG_LOG_FP_ADVERTISING("fastPair_AdvNotifyChangeInIdentifiable %d", fastpair_advert.identifiable);
        notify = fastPair_AdvNotifyDataChange();
        MessageCancelAll(fastPair_AdvGetAdvTask(), FAST_PAIR_ADVERTISING_INTERNAL_RETAIN_RPA);
        
        /* Upon exiting identifiable/pairing mode, use the same RPA for a while afterwards */
        if (!identifiable)
        {
            DEBUG_LOG_INFO("fastPair_AdvNotifyChangeInIdentifiable, retain RPA for %u ms", FAST_PAIR_ADVERTISING_RETAIN_RPA_TIMEOUT_MS);
            MessageSendLater(fastPair_AdvGetAdvTask(), FAST_PAIR_ADVERTISING_INTERNAL_RETAIN_RPA, NULL, FAST_PAIR_ADVERTISING_RETAIN_RPA_TIMEOUT_MS);
        }

    }
    return notify;
#endif
}

/*! @brief Private API to notify the LE Advertising Manager on FP adverts data change
 */
bool fastPair_AdvNotifyDataChange(void)
{
    bool status = FALSE;
    if (fastpair_advert.adv_register_handle)
    {
#ifndef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER
        status = LeAdvertisingManager_UpdateAdvertisingItem(fastpair_advert.adv_register_handle);
#else
        fastPairTaskData *fast_pair_task_data = fastPair_GetTaskData();
        status = LeAdvertisingManager_NotifyDataChange(&fast_pair_task_data->task, fastpair_advert.adv_register_handle);
#endif
    }
    else
    {
        DEBUG_LOG_FP_ADVERTISING("FP ADV: Invalid handle in fastPair_AdvNotifyDataChange");
    }
    return status;
}

/*! @brief Private API to provide BR/EDR discoverablity information
 */
bool fastPair_AdvIsBrEdrDiscoverable(void)
{
    DEBUG_LOG_FP_ADVERTISING("FP ADV: fastpair_AdvIsBrEdrDiscoverable %d", fastpair_advert.identifiable);
    return fastpair_advert.identifiable;
}

bool fastPair_AdvInUseAccountKeyActiveState(void)
{
    return fastpair_advert.in_use_account_key_active;
}

bdaddr fastPair_AdvGetBdaddr(void)
{
    return fastpair_advert.adv_bdaddr;
}