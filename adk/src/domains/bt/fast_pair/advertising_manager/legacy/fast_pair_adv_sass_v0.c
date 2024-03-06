/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_adv_sass.c
\brief      Handles Advertising Data for SASS feature
*/

#ifdef USE_FAST_PAIR_ACCOUNT_DATA_V0

#ifndef DISABLE_FP_SASS_SUPPORT

#include <device_list.h>
#include <logging.h>
#include <stdlib.h>
#include <bt_device.h>
#include <device_properties.h>
#include <connection_manager.h>

#include "context_framework.h"
#include "fast_pair_advertising.h"
#include "fast_pair_adv_sass.h"
#include "fast_pair_bloom_filter.h"
#include "sass.h"
#include "fast_pair.h"
#include "fast_pair_battery_notifications.h"
#include "user_accounts.h"
#include "focus_device.h"
#include "audio_router.h"

#define SASS_AD_TYPE_FIELD               (5)
#define SASS_AD_LENGTH_FIELD             (SASS_SIZE_AD_CON_STATE + SASS_SIZE_AD_CUSTOM_DATA + SASS_SIZE_AD_CON_BITMAP)
#define SASS_IN_USE_ACCOUNT_KEY_LEN      (16)
#define SASS_INITIAL_VECTOR_LEN          (16)
#define SASS_RANDOM_RESOLVABLE_DATA_TYPE (0x6)

/* SASS connection state advertisement elements */
#define SASS_SIZE_AD_LENGTH_TYPE 1
#define SASS_SIZE_AD_CON_STATE   1
#define SASS_SIZE_AD_CUSTOM_DATA 1
#define SASS_SIZE_AD_CON_BITMAP  1
#define SASS_ADV_DATA_SIZE       (SASS_SIZE_AD_LENGTH_TYPE + SASS_SIZE_AD_CON_STATE + SASS_SIZE_AD_CUSTOM_DATA + SASS_SIZE_AD_CON_BITMAP)

#define SASS_RANDOM_RESOLVABLE_DATA_SIZE (SASS_SIZE_AD_LENGTH_TYPE + SASS_ADV_DATA_SIZE)

/*! Size of empty bloom filter. */
#define NO_BLOOM_FILTER_LEN                 (0)

/*! This is advertising data really, rather than account key/bloom filter
    data. Probably not needed here, or at least */
#define FP_ACCOUNT_DATA_FLAGS_SIZE          (1)
/*! Flags all reserved, this is already defined in fast_pair_advertising.c
    where it is used for the advert version for no account keys.
    \todo combine to single definition. */
#define FP_ACCOUNT_DATA_FLAGS               (0x00)

/*! Size of the account key length and type in bytes. */
#define FP_ACCOUNT_KEY_LENGTHTYPE_SIZE      (1)
/*! Account key type definition to show UI popup. */
#define FP_ACCOUNT_KEY_TYPE_UI_SHOW         (0x0)
/*! Account key type definition to hide UI popup. */
#define FP_ACCOUNT_KEY_TYPE_UI_HIDE         (0x2)

/*! Size of a fast pair account key in bytes. */
#define ACCOUNT_KEY_LEN                     (16)

/*! Size of fast pair bloom filter salt in bytes. */
#define FP_SALT_SIZE                        (2)
/*! Type of salt */
#define FP_SALT_TYPE                        (1)
/*! Size of the Salt field length and type in bytes. */
#define FP_SALT_LENGTHTYPE_SIZE             (1)
/*! Combined salt field length and type information. */
#define FP_SALT_LENGTHTYPE                  ((FP_SALT_SIZE << 4) + FP_SALT_TYPE)
/*! Total size of the salt field TLV in the account key data. */
#define FP_SALT_FIELD_TOTAL_SIZE            (FP_SALT_LENGTHTYPE_SIZE + FP_SALT_SIZE)

#define FP_ACCOUNT_DATA_START_POS (FP_ACCOUNT_DATA_FLAGS_SIZE + FP_ACCOUNT_KEY_LENGTHTYPE_SIZE)

/*! Size of the temp array used to generate hash for the bloom filter.
    May require additional space where battery notifications are supported.
*/
#define SHA256_INPUT_ARRAY_LENGTH   (ACCOUNT_KEY_LEN + FP_SALT_SIZE + FP_BATTERY_NOTFICATION_SIZE)

typedef struct
{
    /* In single point use-case, if in-use account key handset is connected, that will automatically be the active handset
       In Multi-point use-case, this flag will help in finding the connected and active i.e. focus foreground status device */
    bool  in_use_account_key_handset_connected_and_active;
    uint8 *in_use_account_key;
    bdaddr device_addr;
}sass_account_keys_data_t;

typedef struct
{
    uint8 *initial_vector_ready;
    uint8 account_key_filter_length;
    uint8 *random_resolvable_data;
    uint8 random_res_data_size;
}sass_random_resolvable_data_t;

typedef struct
{
    TaskData task;
    bool updateFPAdverts;
    sass_account_keys_data_t key_data;
    sass_random_resolvable_data_t enc_data;
}fast_pair_adv_sass_t;

/*! Global Instance of SASS advertising data */
fast_pair_adv_sass_t fast_pair_adv_sass;

static void fastPair_InitializeSASSData(void)
{
    memset(&fast_pair_adv_sass, 0, sizeof(fast_pair_adv_sass_t));

    fast_pair_adv_sass.enc_data.initial_vector_ready = NULL;
    fast_pair_adv_sass.enc_data.random_resolvable_data = NULL;
    fast_pair_adv_sass.updateFPAdverts = FALSE;
    BdaddrSetZero(&fast_pair_adv_sass.key_data.device_addr);
}

static fast_pair_adv_sass_t *fastPair_SASSGetAdvTaskData(void)
{
    return (&fast_pair_adv_sass);
}

static void fastPair_SASSReleaseResolvableData(void)
{
    if (fast_pair_adv_sass.enc_data.random_resolvable_data)
    {
        free(fast_pair_adv_sass.enc_data.random_resolvable_data);
        fast_pair_adv_sass.enc_data.random_resolvable_data = NULL;
        fast_pair_adv_sass.enc_data.random_res_data_size = 0;
    }
}

static void fastPair_SASSReleaseInUseAccountKey(void)
{
    fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active = FALSE;
    if(fast_pair_adv_sass.key_data.in_use_account_key)
    {
        free(fast_pair_adv_sass.key_data.in_use_account_key);
        fast_pair_adv_sass.key_data.in_use_account_key = NULL;
    }
}

static void fastPair_SASSReleaseInitialVector(void)
{
    if(fast_pair_adv_sass.enc_data.initial_vector_ready)
    {
        free(fast_pair_adv_sass.enc_data.initial_vector_ready);
        fast_pair_adv_sass.enc_data.initial_vector_ready = NULL;
        fast_pair_adv_sass.enc_data.account_key_filter_length = 0;
    }
}

#ifdef USE_SYNERGY
static void fastPair_SASSAesCtrCfm(CsrBtCmCryptoAesCtrCfm *cfm)
#else
static void fastPair_SASSAesCtrCfm(CL_CRYPTO_ENCRYPT_DECRYPT_AES_CTR_CFM_T *cfm)
#endif /* !USE_SYNERGY */
{
    DEBUG_LOG("fastPair_SASSAesCtrCfm");

#ifdef USE_SYNERGY
    if(cfm->resultCode == success)
#else
    if(cfm->status == success)
#endif /*! USE_SYNERGY */
    {
        uint8 enc_data_sz_bytes;
        uint8 *big_endian_data;
        
#ifdef USE_SYNERGY
        enc_data_sz_bytes = cfm->dataLen * 2;
#else
        enc_data_sz_bytes = cfm->data_len * 2;
#endif

        fastPair_SASSReleaseResolvableData();

        big_endian_data = PanicUnlessMalloc(enc_data_sz_bytes);
        fastPair_ConvertEndiannessFormat((uint8 *)cfm->data, enc_data_sz_bytes, big_endian_data);
        
        fast_pair_adv_sass.enc_data.random_resolvable_data = PanicUnlessMalloc(SASS_RANDOM_RESOLVABLE_DATA_SIZE);
        *(fast_pair_adv_sass.enc_data.random_resolvable_data) = (uint8)(((enc_data_sz_bytes << 4) & 0xFF) | (SASS_RANDOM_RESOLVABLE_DATA_TYPE & 0xFF));
        memcpy((fast_pair_adv_sass.enc_data.random_resolvable_data+1), big_endian_data, enc_data_sz_bytes);
        fast_pair_adv_sass.enc_data.random_res_data_size = enc_data_sz_bytes + 1;

        DEBUG_LOG_DEBUG("fastPair_SASSAesCtrCfm: Generated Random Resolvable Data:");
        DEBUG_LOG_DATA_DEBUG(fast_pair_adv_sass.enc_data.random_resolvable_data, enc_data_sz_bytes);

        free(big_endian_data);

        if(fastPair_AdvInUseAccountKeyActiveState() != FastPair_SASSGetInUseAccountKeyActiveFlag())
        {
            fast_pair_adv_sass.updateFPAdverts = TRUE;
        }

        /* Trigger advertisement data change only if advertised LE account key filter and 
           SASS random resolvable data does not match current status. */
        if(fast_pair_adv_sass.updateFPAdverts == TRUE)
        {
            /* Notify LE Advertising Manager on random resolvable data change */
            fastPair_AdvNotifyDataChange();
            fast_pair_adv_sass.updateFPAdverts = FALSE;
        }
    }
}

static void fastPair_SASSGenerateRandomResolvableData(void)
{
    if(!fast_pair_adv_sass.enc_data.initial_vector_ready)
    {
        fast_pair_adv_sass.enc_data.initial_vector_ready = PanicUnlessMalloc(SASS_INITIAL_VECTOR_LEN * sizeof(uint8));
        if(fastPairGetBloomFilterLen())
        {
            fast_pair_adv_sass.enc_data.account_key_filter_length = fastPairGetBloomFilterLen() - FP_ACCOUNT_DATA_START_POS - FP_SALT_FIELD_TOTAL_SIZE;

            if(fast_pair_adv_sass.enc_data.account_key_filter_length < SASS_INITIAL_VECTOR_LEN)
            {
                uint8 *connection_status_raw_data = NULL;
                uint8 data_sz_words;
                fast_pair_adv_sass_t *sass_adv_task = fastPair_SASSGetAdvTaskData();
                uint8 *little_endian_key = PanicUnlessMalloc(SASS_IN_USE_ACCOUNT_KEY_LEN);
                uint8 *little_endian_iv = PanicUnlessMalloc(SASS_IN_USE_ACCOUNT_KEY_LEN);
                uint8 *little_endian_con = PanicUnlessMalloc(SASS_ADV_DATA_SIZE);
                connection_status_raw_data = PanicUnlessMalloc(SASS_ADV_DATA_SIZE);

                connection_status_raw_data[0] = ((SASS_AD_LENGTH_FIELD << 4) & 0xF0) | (SASS_AD_TYPE_FIELD);
                connection_status_raw_data[1] = Sass_GetConnectionState();
                connection_status_raw_data[2] = Sass_GetCustomData();
                connection_status_raw_data[3] = Sass_GetConnectedDeviceBitMap();

                DEBUG_LOG_DEBUG("fastPair_SASSGenerateRandomResolvableData: Connection Status Raw Data");
                DEBUG_LOG_DATA_DEBUG(connection_status_raw_data, SASS_ADV_DATA_SIZE);

                data_sz_words = SASS_ADV_DATA_SIZE/2;

                /* Generate IV by using account key filter */
                memcpy(fast_pair_adv_sass.enc_data.initial_vector_ready, fastPairGetBloomFilterData() + FP_ACCOUNT_DATA_START_POS, fast_pair_adv_sass.enc_data.account_key_filter_length);

                /* Append the Initial Vector with Zero Padding */
                memset(&fast_pair_adv_sass.enc_data.initial_vector_ready[fast_pair_adv_sass.enc_data.account_key_filter_length], 0x00, (SASS_INITIAL_VECTOR_LEN-fast_pair_adv_sass.enc_data.account_key_filter_length));

                /* Convert the big endian data to little endian before processing it for AES-CTR */
                if(fast_pair_adv_sass.key_data.in_use_account_key != NULL)
                {
                    fastPair_ConvertEndiannessFormat(fast_pair_adv_sass.key_data.in_use_account_key, SASS_IN_USE_ACCOUNT_KEY_LEN, little_endian_key);
                }
                fastPair_ConvertEndiannessFormat(fast_pair_adv_sass.enc_data.initial_vector_ready, SASS_INITIAL_VECTOR_LEN, little_endian_iv);
                fastPair_ConvertEndiannessFormat(connection_status_raw_data, SASS_ADV_DATA_SIZE, little_endian_con);

                DEBUG_LOG("fastPair_SASSGenerateRandomResolvableData: AES-CTR Encrypt Req Send");

#ifdef USE_SYNERGY
                CmCryptoAesCtrReqSend(&(sass_adv_task->task), 0, CSR_BT_CM_AES_CTR_BIG_ENDIAN, 
                (uint16 *)little_endian_key, (uint16 *)little_endian_iv, data_sz_words, (uint16 *)little_endian_con);
                free(little_endian_con);
#else
                ConnectionEncryptDecryptAesCtrReq(&(sass_adv_task->task), 0, cl_aes_ctr_big_endian, 
                (uint16 *)little_endian_key, (uint16 *)little_endian_iv, data_sz_words, (uint16 *)little_endian_con);
#endif /*! USE_SYNERGY */

                free(connection_status_raw_data);
                free(little_endian_iv);
                free(little_endian_key);

            }
        }
        fastPair_SASSReleaseInitialVector();
    }
}

#ifdef USE_SYNERGY
static void appHandleCmPrim(Message message)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    switch (*prim)
    {
        case CSR_BT_CM_CRYPTO_AES_CTR_CFM:
            DEBUG_LOG("fastPair_SASSMessageHandler appHandleCmPrim - CSR_BT_CM_CRYPTO_AES_CTR_CFM");
            fastPair_SASSAesCtrCfm((CsrBtCmCryptoAesCtrCfm *)message);
        break;

        default:
            DEBUG_LOG("fastPair_SASSMessageHandler appHandleCmPrim, unexpected CM prim 0x%04x", *prim);
            break;
    }
    CmFreeUpstreamMessageContents(message);
}
#endif /* USE_SYNERGY */

static void fastPair_SASSMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
#ifdef USE_SYNERGY
        case CM_PRIM:
            appHandleCmPrim(message);
            break;
#endif /* USE_SYNERGY */

#ifndef USE_SYNERGY
        case CL_CRYPTO_ENCRYPT_DECRYPT_AES_CTR_CFM:
            fastPair_SASSAesCtrCfm((CL_CRYPTO_ENCRYPT_DECRYPT_AES_CTR_CFM_T *)message);
            break;
#endif /* !USE_SYNERGY */
    }
}

static void fastPair_SASSConnectionStatusUpdate(void)
{
     fast_pair_adv_sass.updateFPAdverts = TRUE;
     fastPair_GenerateBloomFilter();
}

/*! Get random resolvable data size */
uint8 fastPair_SASSGetAdvDataSize(void)
{
    return fast_pair_adv_sass.enc_data.random_res_data_size;
}

/*! Get pointer to random resolvable data */
uint8* fastPair_SASSGetAdvData(void)
{
    return fast_pair_adv_sass.enc_data.random_resolvable_data;
}

uint8* fastPair_SASSGetInUseAccountKey(bool* is_connected_and_active)
{
    device_t* devices = NULL;
    unsigned num_devices = 0;
    uint8 *value = NULL;
    size_t size = 0;
    bdaddr handset_bd_addr = {0};
    uint8 account_key_index = 0xFF;

    DEBUG_LOG("fastPair_SASSGetInUseAccountKey: In-Use account key ptr=%p", fast_pair_adv_sass.key_data.in_use_account_key);
    /* During re-boot, we won't have any in-use account key. In this case, we will select first index account key as in-use account key */
    if(fast_pair_adv_sass.key_data.in_use_account_key == NULL)
    {
        DeviceList_GetAllDevicesWithProperty(device_property_handset_account_key_index, &devices, &num_devices);
        if(num_devices != 0)
        {
            for(uint8 i = 0; i < num_devices; ++i) 
            {
                if (Device_GetProperty(devices[i], device_property_handset_account_key_index, (void**)&value, &size))
                {
                    account_key_index = *value;
                    DEBUG_LOG("fastPair_SASSGetInUseAccountKey: device=%p, account key index=%d", devices[i], account_key_index);

                    if((account_key_index >= 0) && (account_key_index < UserAccounts_GetMaxNumAccountKeys()))
                    {
                        handset_bd_addr = DeviceProperties_GetBdAddr(devices[i]);
                        fastPair_SASSReleaseInUseAccountKey();
                        fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active = FALSE;
                        fast_pair_adv_sass.key_data.in_use_account_key = UserAccounts_GetAccountKeyWithHandset(SASS_IN_USE_ACCOUNT_KEY_LEN, &handset_bd_addr);
                        fast_pair_adv_sass.key_data.device_addr = handset_bd_addr;

                        /* when in-use account key is NULL before fetching it from User Accounts, then there might be some cases where in-use account
                           key does not get updated as part of MRU device update. In these cases check if we are connected with any handset, if so
                           update in-use account key pattern flag to TRUE. */
                        if(ConManagerIsConnected(&handset_bd_addr))
                        {
                            fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active = TRUE;
                        }

                        if(fast_pair_adv_sass.key_data.in_use_account_key != NULL)
                        {
                            DEBUG_LOG("fastPair_SASSUpdateInUseAccountKey : In Use account key :");
                            DEBUG_LOG_DATA_DEBUG(fast_pair_adv_sass.key_data.in_use_account_key, SASS_IN_USE_ACCOUNT_KEY_LEN);
                        }
                        break;
                    }
                }
            }
        }
        free(devices);
    }

    if(fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active)
    {
        *is_connected_and_active = TRUE;
    }
    else
    {
        *is_connected_and_active = FALSE;
    }

    return fast_pair_adv_sass.key_data.in_use_account_key;
}

void FastPair_SASSUpdateInUseAccountKeyForMruDevice(const bdaddr *bd_addr)
{
    device_t new_mru_device = NULL;

    if (!BdaddrIsSame(bd_addr, &fast_pair_adv_sass.key_data.device_addr) || !fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active)
    {
        unsigned num_connected_handsets;
        device_t *devices = NULL;
        bool other_active_handet_found = FALSE;

        new_mru_device = BtDevice_GetDeviceForBdAddr(bd_addr);

        if(new_mru_device == NULL || ((BtDevice_GetDeviceType(new_mru_device) != DEVICE_TYPE_HANDSET) &&
                               (BtDevice_GetDeviceType(new_mru_device) !=  DEVICE_TYPE_SINK)))
        {
            return;
        }

        DEBUG_LOG("FastPair_SASSUpdateInUseAccountKeyForMruDevice: bdaddr=[%04x,%02x,%06lx]", 
                   bd_addr->nap, bd_addr->uap, bd_addr->lap);

        num_connected_handsets = BtDevice_GetConnectedBredrHandsets(&devices);

        /* In multipoint use case, make sure that in use account key is updated only after checking for audio status of all
           connected handsets
        */
        if(num_connected_handsets > 1)
        {
            for(uint8 index = 0; index < num_connected_handsets; index++)
            {
                /* If other connected device has the routed source, then no need to update the in-use account key */
                if(devices[index] != NULL && (new_mru_device != devices[index]) && AudioRouter_IsDeviceInUse(devices[index]))
                {
                    DEBUG_LOG("FastPair_SASSUpdateInUseAccountKeyForMruDevice: No need to update in-use account key.");
                    other_active_handet_found = TRUE;
                    break;
                }
            }
        }
        free(devices);

        if(!other_active_handet_found)
        {
            uint8* in_use_account_key = NULL;

            /* Check if the new mru device is associated with any account key*/
            in_use_account_key = UserAccounts_GetAccountKeyWithHandset(SASS_IN_USE_ACCOUNT_KEY_LEN, bd_addr);

            /* If we find the sass device has an associated account key, set this account key as in-use account key */
            if(in_use_account_key != NULL)
            {
                fastPair_SASSReleaseInUseAccountKey();
                fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active = TRUE;
                fast_pair_adv_sass.key_data.in_use_account_key = in_use_account_key;
                fast_pair_adv_sass.key_data.device_addr = *bd_addr;

                DEBUG_LOG("FastPair_SASSUpdateInUseAccountKeyForMruDevice : In Use account key :");
                DEBUG_LOG_DATA(fast_pair_adv_sass.key_data.in_use_account_key, SASS_IN_USE_ACCOUNT_KEY_LEN);
            }
            /* In case the MRU device is a non-sass seeker, we need not update the in-use account key */
            else
            {
                fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active = FALSE;
                DEBUG_LOG("FastPair_SASSUpdateInUseAccountKeyForMruDevice: SASS handset not found.");
            }

            /* Clear out custom data as MRU device has now changed, latest data for the streaming type  
             * would be sent by SASS seeker  
             */
            Sass_SetCustomData(0);
        }
    }
}

bool FastPair_SASSGetInUseAccountKeyActiveFlag(void)
{
    return fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active;
}

/*! API to retrigger SASS random resolvable data */
void fastPair_SASSUpdateAdvPayload(uint16 salt, fastPair_SassUpdateAdvPayloadCompleteCallback done_callback)
{
    UNUSED(salt);
    UNUSED(done_callback);
    fastPair_SASSGenerateRandomResolvableData();
}

/*! Private API to setup SASS Advertising */
void fastPair_SetUpSASSAdvertising(void)
{
    fast_pair_adv_sass_t *sass_adv_task = fastPair_SASSGetAdvTaskData();

    /*Initialise fastpair SASS advertising globals*/
    fastPair_InitializeSASSData();

    /* Set up task handler */
    sass_adv_task->task.handler = fastPair_SASSMessageHandler;

    /* Register with SASS plugin to check if there is a change in custom data or context update from context framework */
    Sass_RegisterForConnectionStatusChange(fastPair_SASSConnectionStatusUpdate);
}

bool FastPair_SassIsDeviceTheCurrentInUseDevice(const bdaddr * bd_addr)
{
    return (BdaddrIsSame(&fast_pair_adv_sass.key_data.device_addr, bd_addr));
}

void FastPair_SassUpdateOnInUseDeviceDisconnect(void)
{
    fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active = FALSE;
}

#endif
#endif