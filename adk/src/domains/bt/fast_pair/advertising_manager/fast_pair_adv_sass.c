/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_adv_sass.c
\brief      Handles Advertising Data for SASS feature
*/

#ifndef USE_FAST_PAIR_ACCOUNT_DATA_V0

#include "fast_pair_adv_sass.h"

#ifndef DISABLE_FP_SASS_SUPPORT

#include <device_list.h>
#include <logging.h>
#include <stdlib.h>
#include <bt_device.h>
#include <device_properties.h>
#include <connection_manager.h>

#include "context_framework.h"
#include "fast_pair_advertising.h"
#include "fast_pair_bloom_filter.h"
#include "fast_pair_hkdf.h"
#include "sass.h"
#include "fast_pair.h"
#include "fast_pair_battery_notifications.h"
#include "user_accounts.h"
#include "focus_device.h"
#include "audio_router.h"

#define DEBUG_LOG_ADV_SASS          DEBUG_LOG
#define DEBUG_LOG_ADV_SASS_DATA     DEBUG_LOG_DATA

#define SASS_CONNECTION_STATUS_FIELD_LENGTH_TYPE_SIZE                   1
#define SASS_CONNECTION_STATUS_FIELD_CONNECTION_STATE_SIZE              1
#define SASS_CONNECTION_STATUS_FIELD_CUSTOM_DATA_SIZE                   1
#define SASS_CONNECTION_STATUS_FIELD_CONNECTED_DEVICES_BITMAP_SIZE      1

#define SASS_CONNECTION_STATUS_FIELD_TYPE                       0x05
#define SASS_CONNECTION_STATUS_FIELD_LENGTH                     (SASS_CONNECTION_STATUS_FIELD_CONNECTION_STATE_SIZE + SASS_CONNECTION_STATUS_FIELD_CUSTOM_DATA_SIZE + SASS_CONNECTION_STATUS_FIELD_CONNECTED_DEVICES_BITMAP_SIZE)

#define SASS_RANDOM_RESOLVABLE_DATA_LENGTH_TYPE_SIZE            1
#define SASS_RANDOM_RESOLVABLE_DATA_ENCRYPTED_DATA_SIZE         (SASS_CONNECTION_STATUS_FIELD_LENGTH_TYPE_SIZE + SASS_CONNECTION_STATUS_FIELD_LENGTH)

#define SASS_RANDOM_RESOLVABLE_DATA_TYPE                        0x06
#define SASS_RANDOM_RESOLVABLE_DATA_LENGTH                      (SASS_RANDOM_RESOLVABLE_DATA_LENGTH_TYPE_SIZE + SASS_RANDOM_RESOLVABLE_DATA_ENCRYPTED_DATA_SIZE)

#define SASS_ACCOUNT_KEY_LEN    16
#define SASS_INITIAL_VECTOR_LEN          (16)


typedef struct
{
    /* In single point use-case, if in-use account key handset is connected, that will automatically be the active handset
       In Multi-point use-case, this flag will help in finding the connected and active i.e. focus foreground status device */
    bool  in_use_account_key_handset_connected_and_active;
    uint8 *in_use_account_key;
    bdaddr device_addr;
} sass_account_keys_data_t;

typedef struct
{
    uint8 *random_resolvable_data;
    uint8 random_res_data_size;
    bool random_resolvable_data_encryption_in_progress;
    fastPair_SassUpdateAdvPayloadCompleteCallback callback_when_done;
 }sass_random_resolvable_data_t;

typedef struct
{
    TaskData task;  
    sass_account_keys_data_t key_data;
    sass_random_resolvable_data_t enc_data;
} fast_pair_adv_sass_t;

/*! Global Instance of SASS advertising data */
static fast_pair_adv_sass_t fast_pair_adv_sass;

static void fastPair_InitializeSASSData(void)
{
    memset(&fast_pair_adv_sass, 0, sizeof(fast_pair_adv_sass_t));
    fast_pair_adv_sass.enc_data.random_resolvable_data = NULL;
    BdaddrSetZero(&fast_pair_adv_sass.key_data.device_addr);
}

static fast_pair_adv_sass_t *fastPair_SASSGetAdvTaskData(void)
{
    return (&fast_pair_adv_sass);
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

static inline uint8 fastPair_ConstructSassRandomResolvableLengthType(uint8 length)
{
    return (((length << 4) & 0xF0) | (SASS_RANDOM_RESOLVABLE_DATA_TYPE & 0x0F));
}

#ifdef USE_SYNERGY
static void fastPair_SASSAesCtrCfm(CsrBtCmCryptoAesCtrCfm *cfm)
#else
static void fastPair_SASSAesCtrCfm(CL_CRYPTO_ENCRYPT_DECRYPT_AES_CTR_CFM_T *cfm)
#endif /* !USE_SYNERGY */
{
#ifdef USE_SYNERGY
    bool result_code = cfm->resultCode;
#else
    bool result_code = cfm->status;
#endif
    DEBUG_LOG_ADV_SASS("fastPair_SASSAesCtrCfm result_code 0x%x", result_code);

    if(result_code == success)
    {
#ifdef USE_SYNERGY
        uint16 enc_data_size_bytes = cfm->dataLen * sizeof(uint16);
#else
        uint16 enc_data_size_bytes = cfm->data_len * sizeof(uint16);
#endif
        uint8 * big_endian_enc_data = PanicUnlessMalloc(enc_data_size_bytes);
        fastPair_ConvertEndiannessFormat((uint8 *)cfm->data, enc_data_size_bytes, big_endian_enc_data);
        DEBUG_LOG_ADV_SASS("fastPair_SASSAesCtrCfm data %p bytes %u", cfm->data, enc_data_size_bytes);
        DEBUG_LOG_ADV_SASS_DATA(big_endian_enc_data, enc_data_size_bytes);

        if(fast_pair_adv_sass.enc_data.random_resolvable_data == NULL)
        {
            fast_pair_adv_sass.enc_data.random_resolvable_data = PanicUnlessMalloc(SASS_RANDOM_RESOLVABLE_DATA_LENGTH);
        }
        fast_pair_adv_sass.enc_data.random_resolvable_data[0] = fastPair_ConstructSassRandomResolvableLengthType(enc_data_size_bytes);
        memcpy(&fast_pair_adv_sass.enc_data.random_resolvable_data[1], big_endian_enc_data, enc_data_size_bytes);
        fast_pair_adv_sass.enc_data.random_res_data_size = (SASS_RANDOM_RESOLVABLE_DATA_LENGTH_TYPE_SIZE + enc_data_size_bytes);

        free(big_endian_enc_data);
    }

    DEBUG_LOG_ADV_SASS("fastPair_SASSAesCtrCfm : callback : 0x%p", fast_pair_adv_sass.enc_data.callback_when_done);
    fast_pair_adv_sass.enc_data.random_resolvable_data_encryption_in_progress = FALSE;
    fast_pair_adv_sass.enc_data.callback_when_done(result_code);
    fast_pair_adv_sass.enc_data.callback_when_done = NULL;
}

static inline void fastPair_SassGetConnectionStatusFieldRawData(uint8 * data, uint8 max_length)
{
    PanicFalse(max_length <= SASS_RANDOM_RESOLVABLE_DATA_ENCRYPTED_DATA_SIZE);
    data[0] = ((SASS_CONNECTION_STATUS_FIELD_LENGTH << 4) & 0xF0) | (SASS_CONNECTION_STATUS_FIELD_TYPE);
    data[1] = Sass_GetConnectionState();
    data[2] = Sass_GetCustomData();
    data[3] = Sass_GetConnectedDeviceBitMap();
}

static const uint8 info[] = { "SASS-RRD-KEY" };
static const uint16 info_len = (ARRAY_DIM(info) - 1); /* Don't count the nul character at the end of the string in the length. */

static void fastPair_SASSGenerateRandomResolvableData(uint16 salt)
{
    if(!fast_pair_adv_sass.enc_data.random_resolvable_data_encryption_in_progress)
    {
        uint8 * in_use_account_key = fastPair_SASSGetInUseAccountKey(NULL);
        uint8 * sass_key = PanicNull(calloc(1, SASS_ACCOUNT_KEY_LEN));

        fastpair_hkdf_t * params = PanicUnlessMalloc(sizeof(fastpair_hkdf_t));
        params->salt = NULL;
        params->salt_len = 0;
        params->ikm = in_use_account_key;
        params->ikm_len = SASS_ACCOUNT_KEY_LEN;
        params->info = info;
        params->info_len = info_len;
        params->okm = sass_key;
        params->okm_len = SASS_ACCOUNT_KEY_LEN;

        if(FastPair_HkdfSha256(params))
        {
            DEBUG_LOG_ADV_SASS("fastPair_SASSGenerateRandomResolvableData in_use_account_key:");
            DEBUG_LOG_ADV_SASS_DATA(in_use_account_key, SASS_ACCOUNT_KEY_LEN);
            
            DEBUG_LOG_ADV_SASS("fastPair_SASSGenerateRandomResolvableData sass_key:");
            DEBUG_LOG_ADV_SASS_DATA(sass_key, SASS_ACCOUNT_KEY_LEN);

            uint8 * iv = PanicNull(calloc(1, SASS_INITIAL_VECTOR_LEN));
            iv[0] = ((salt >> 8) & 0xFF);
            iv[1] = (salt & 0xFF);
            DEBUG_LOG_ADV_SASS("fastPair_SASSGenerateRandomResolvableData iv:");
            DEBUG_LOG_ADV_SASS_DATA(iv, SASS_INITIAL_VECTOR_LEN);

            uint8 connection_status_raw_data[SASS_RANDOM_RESOLVABLE_DATA_ENCRYPTED_DATA_SIZE] = { 0 };
            fastPair_SassGetConnectionStatusFieldRawData(connection_status_raw_data, SASS_RANDOM_RESOLVABLE_DATA_ENCRYPTED_DATA_SIZE);
            DEBUG_LOG_ADV_SASS("fastPair_SASSGenerateRandomResolvableData connection_status_raw_data:");
            DEBUG_LOG_ADV_SASS_DATA(connection_status_raw_data, SASS_RANDOM_RESOLVABLE_DATA_ENCRYPTED_DATA_SIZE);

            uint8 *little_endian_key = PanicUnlessMalloc(SASS_ACCOUNT_KEY_LEN);
            uint8 *little_endian_iv = PanicUnlessMalloc(SASS_ACCOUNT_KEY_LEN);
            uint8 *little_endian_con = PanicUnlessMalloc(SASS_RANDOM_RESOLVABLE_DATA_ENCRYPTED_DATA_SIZE);

            /* Convert the big endian data to little endian before processing it for AES-CTR */
            fastPair_ConvertEndiannessFormat((uint8 *)sass_key, SASS_ACCOUNT_KEY_LEN, little_endian_key);
            fastPair_ConvertEndiannessFormat((uint8 *)iv, SASS_INITIAL_VECTOR_LEN, little_endian_iv);
            fastPair_ConvertEndiannessFormat((uint8 *)connection_status_raw_data, SASS_RANDOM_RESOLVABLE_DATA_ENCRYPTED_DATA_SIZE, little_endian_con);

            free(iv);

            fast_pair_adv_sass.enc_data.random_resolvable_data_encryption_in_progress = TRUE;
            fast_pair_adv_sass.enc_data.random_res_data_size = 0;
            fast_pair_adv_sass_t *sass_adv_task = fastPair_SASSGetAdvTaskData();
#ifdef USE_SYNERGY
            CmCryptoAesCtrReqSend(&(sass_adv_task->task), 0, CSR_BT_CM_AES_CTR_BIG_ENDIAN,
                              (uint16 *)little_endian_key, (uint16 *)little_endian_iv,
                              (SASS_RANDOM_RESOLVABLE_DATA_ENCRYPTED_DATA_SIZE >> 1), (uint16 *)little_endian_con);
            free(little_endian_con);
#else
            ConnectionEncryptDecryptAesCtrReq(&(sass_adv_task->task), 0, cl_aes_ctr_big_endian, 
                    (uint16 *)little_endian_key, (uint16 *)little_endian_iv, (SASS_RANDOM_RESOLVABLE_DATA_ENCRYPTED_DATA_SIZE >> 1), (uint16 *)little_endian_con);
#endif
            free(little_endian_iv);
            free(little_endian_key);
        }
        else
        {
            DEBUG_LOG_ADV_SASS("fastPair_SASSGenerateRandomResolvableData: FAILED using HKDF to create key");
        }
        free(sass_key);
        free(params);
    }
}

#ifdef USE_SYNERGY
static void appHandleCmPrim(Message message)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    switch (*prim)
    {
        case CSR_BT_CM_CRYPTO_AES_CTR_CFM:
            DEBUG_LOG_ADV_SASS("fastPair_SASSMessageHandler appHandleCmPrim - CSR_BT_CM_CRYPTO_AES_CTR_CFM");
            fastPair_SASSAesCtrCfm((CsrBtCmCryptoAesCtrCfm *)message);
        break;

        default:
            DEBUG_LOG_ADV_SASS("fastPair_SASSMessageHandler appHandleCmPrim, unexpected CM prim 0x%04x", *prim);
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

    DEBUG_LOG_ADV_SASS("fastPair_SASSGetInUseAccountKey: In-Use account key ptr=%p", fast_pair_adv_sass.key_data.in_use_account_key);
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
                    DEBUG_LOG_ADV_SASS("fastPair_SASSGetInUseAccountKey: device=%p, account key index=%d", devices[i], account_key_index);

                    if((account_key_index >= 0) && (account_key_index < UserAccounts_GetMaxNumAccountKeys()))
                    {
                        handset_bd_addr = DeviceProperties_GetBdAddr(devices[i]);
                        fastPair_SASSReleaseInUseAccountKey();
                        fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active = FALSE;
                        fast_pair_adv_sass.key_data.in_use_account_key = UserAccounts_GetAccountKeyWithHandset(SASS_ACCOUNT_KEY_LEN, &handset_bd_addr);
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
                            DEBUG_LOG_ADV_SASS("fastPair_SASSUpdateInUseAccountKey : In Use account key :");
                            DEBUG_LOG_ADV_SASS_DATA(fast_pair_adv_sass.key_data.in_use_account_key, SASS_ACCOUNT_KEY_LEN);
                        }
                        break;
                    }
                }
            }
        }
        free(devices);
    }

    if(is_connected_and_active)
    {
        *is_connected_and_active = fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active;
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
                    DEBUG_LOG_ADV_SASS("FastPair_SASSUpdateInUseAccountKeyForMruDevice: No need to update in-use account key.");
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
            in_use_account_key = UserAccounts_GetAccountKeyWithHandset(SASS_ACCOUNT_KEY_LEN, bd_addr);

            /* If we find the sass device has an associated account key, set this account key as in-use account key */
            if(in_use_account_key != NULL)
            {
                fastPair_SASSReleaseInUseAccountKey();
                fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active = TRUE;
                fast_pair_adv_sass.key_data.in_use_account_key = in_use_account_key;
                fast_pair_adv_sass.key_data.device_addr = *bd_addr;

                DEBUG_LOG_ADV_SASS("FastPair_SASSUpdateInUseAccountKeyForMruDevice : In Use account key :");
                DEBUG_LOG_ADV_SASS_DATA(fast_pair_adv_sass.key_data.in_use_account_key, SASS_ACCOUNT_KEY_LEN);
            }
            /* In case the MRU device is a non-sass seeker, we need not update the in-use account key */
            else
            {
                fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active = FALSE;
                DEBUG_LOG_ADV_SASS("FastPair_SASSUpdateInUseAccountKeyForMruDevice: SASS handset not found.");
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

bool FastPair_SassIsDeviceTheCurrentInUseDevice(const bdaddr * bd_addr)
{
    return (BdaddrIsSame(&fast_pair_adv_sass.key_data.device_addr, bd_addr));
}

void FastPair_SassUpdateOnInUseDeviceDisconnect(void)
{
    fast_pair_adv_sass.key_data.in_use_account_key_handset_connected_and_active = FALSE;
}

/*! API to retrigger SASS random resolvable data */
void fastPair_SASSUpdateAdvPayload(uint16 salt, fastPair_SassUpdateAdvPayloadCompleteCallback done_callback)
{
    DEBUG_LOG_ADV_SASS("fastPair_SASSUpdateAdvPayload : callback : 0x%p", done_callback);
    fast_pair_adv_sass.enc_data.callback_when_done = done_callback;
    fastPair_SASSGenerateRandomResolvableData(salt);
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

#endif
#endif