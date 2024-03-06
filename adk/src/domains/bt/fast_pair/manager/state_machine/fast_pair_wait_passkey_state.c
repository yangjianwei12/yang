/*!
\copyright  Copyright (c) 2008 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_wait_passkey_state.c
\brief      Fast Pair Wait for Passkey State Event handling
*/


#include "fast_pair_wait_passkey_state.h"
#include "fast_pair_gfps.h"
#include "fast_pair_pairing_if.h"
#include "fast_pair_session_data.h"
#include "fast_pair_events.h"
#include "user_accounts.h"
#include "user_accounts_sync.h"
#include "fast_pair_bloom_filter.h"

static bool pairing_successful = FALSE;

static bool fastPair_ValidatePasskey(fast_pair_state_event_crypto_decrypt_args_t* args)
{
    bool status = FALSE;
    uint32 seeker_passkey = 0;
    fastPairTaskData *theFastPair;
    uint8* decrypted_data_be;
    uint8 *big_endian_decr_data = PanicUnlessMalloc(FAST_PAIR_ENCRYPTED_PASSKEY_BLOCK_LEN);

#ifdef USE_SYNERGY
    decrypted_data_be = (uint8 *)args->crypto_decrypt_cfm->decryptedData;
#else
    decrypted_data_be = (uint8 *)args->crypto_decrypt_cfm->decrypted_data;
#endif

    theFastPair = fastPair_GetTaskData();

    /* Decrypted data is in little endian format, Convert it to big endian format before validating the passkey */
    fastPair_ConvertEndiannessFormat(decrypted_data_be, FAST_PAIR_ENCRYPTED_PASSKEY_BLOCK_LEN, big_endian_decr_data);

    if (big_endian_decr_data[0] != fast_pair_passkey_seeker)
    {
        /* We failed to decrypt passkey */
        DEBUG_LOG("Failed to decrypt passkey!\n");

        /* AES Key Not Valid!. Free it and set fast Pair state to state to Idle */
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
    }
    else
    {
        seeker_passkey = ((uint32)big_endian_decr_data[1] << 16 ) | 
                        ((uint32)big_endian_decr_data[2] << 8 ) |
                            (uint32)big_endian_decr_data[3];

        status = TRUE;
    }

    free(big_endian_decr_data);;
    fastPair_PairingPasskeyReceived(seeker_passkey);
    return status;

}

static bool fastPair_SendPasskeyResponse(fast_pair_state_event_crypto_encrypt_args_t* args)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();
#ifdef USE_SYNERGY
    if(args->crypto_encrypt_cfm->resultCode == success)
#else
    if(args->crypto_encrypt_cfm->status == success)
#endif /*! USE_SYNERGY */
    {
        uint8* encrypted_data;
        uint8 *big_endian_enc_data = PanicUnlessMalloc(FAST_PAIR_ENCRYPTED_RESPONSE_LEN);

#ifdef USE_SYNERGY
        encrypted_data = (uint8 *)args->crypto_encrypt_cfm->encryptedData;
#else
        encrypted_data = (uint8 *)args->crypto_encrypt_cfm->encrypted_data;
#endif /*! USE_SYNERGY */

        /* Convert the little endian encrypted response to big endian so that Seeker can understand it as
           FP Seeker expects the data in big endian format. */
        fastPair_ConvertEndiannessFormat(encrypted_data, FAST_PAIR_ENCRYPTED_RESPONSE_LEN, big_endian_enc_data);

        fastPair_SendFPNotification(FAST_PAIR_PASSKEY, big_endian_enc_data);
        free(big_endian_enc_data);

        fastPair_PairingReset();
        status = TRUE;
    }

    /*! Move fp state to wait for account key state in case of model ID/initial pairing
        No need of moving to account key state if pairing was done through subsequent pairing */
    if(pairing_successful && theFastPair->session_data.public_key != NULL)
    {
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_WAIT_ACCOUNT_KEY);
    }
    else
    {
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
    }
    return status;
}

static bool fastpair_PasskeyWriteEventHandler(fast_pair_state_event_passkey_write_args_t* args)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();
    
    if(args->enc_data != NULL)
    {
        uint8 *encrypted_data = (uint8 *)args->enc_data;
        uint8 *little_endian_enc_data = PanicUnlessMalloc(FAST_PAIR_ENCRYPTED_PASSKEY_BLOCK_LEN);

        /* Encrypted data received in big endian format, convert it to little endian format before processing it for decryption */
        fastPair_ConvertEndiannessFormat(encrypted_data, FAST_PAIR_ENCRYPTED_PASSKEY_BLOCK_LEN, little_endian_enc_data);

        /* Decrypt passkey Block */
#ifdef USE_SYNERGY
        CmCryptoDecryptReqSend(&theFastPair->task, (uint16 *)little_endian_enc_data, theFastPair->session_data.aes_key);
#else
        ConnectionDecryptBlockAes(&theFastPair->task, (uint16 *)little_endian_enc_data, theFastPair->session_data.aes_key);
#endif /* USE_SYNERGY */

        status = TRUE;
        free(little_endian_enc_data);
    }
    return status;
}

static uint8* fastPair_GeneratePasskeyResponse(uint32 provider_passkey)
{
    uint8 *response = PanicUnlessMalloc(FAST_PAIR_ENCRYPTED_PASSKEY_BLOCK_LEN);
    uint16 i;


    response[0] = fast_pair_passkey_provider;
    response[1] = (provider_passkey >> 16) & 0xFF;
    response[2] = (provider_passkey >> 8) & 0xFF;
    response[3] = provider_passkey & 0xFF;
    for (i = 4; i < 16; i++)
    {
        response[i] = UtilRandom() & 0xFF;
    }

    return response;
}

static bool fastpair_ProviderPasskeyEventHandler(fast_pair_state_event_provider_passkey_write_args_t* args)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();

    uint8* passkey_response =  fastPair_GeneratePasskeyResponse(args->provider_passkey);
    uint8 *little_endian_passkey_response = PanicUnlessMalloc(FAST_PAIR_ENCRYPTED_RESPONSE_LEN);

     if(args->isPasskeySame)
     {
         /* Pairing was successful. If Pairing was done using account key pairing, update the account key table */
         if(theFastPair->session_data.public_key == NULL)
         {
            uint16 added_acc_key_index;
            uint8 *aes_key = PanicUnlessMalloc(FAST_PAIR_AES_KEY_LEN);

            /* Account key will be stored in Big endian format in User Accounts module */
            fastPair_ConvertEndiannessFormat((uint8 *)theFastPair->session_data.aes_key, FAST_PAIR_AES_KEY_LEN, aes_key);

            /* Add the account key to our list, if it doesn't exist already. */
            added_acc_key_index = UserAccounts_AddAccountKeyForHandset(&args->handset_tpaddr.taddr.addr, aes_key,
                                                                       FAST_PAIR_ACCOUNT_KEY_LEN, user_account_type_fast_pair);

            if (added_acc_key_index != INVALID_USER_ACCOUNT_KEY_INDEX)
            {
                DEBUG_LOG_INFO("fastpair_ProviderPasskeyEventHandler, account key added and associated, idx=%u, lap=%06lx",
                                                                            added_acc_key_index, args->handset_tpaddr.taddr.addr.lap);
            }
            else
            {
                DEBUG_LOG_ERROR("fastpair_ProviderPasskeyEventHandler, account key addition failed, lap=%06lx", args->handset_tpaddr.taddr.addr.lap);
            }

            /*! Account Key Sharing*/
            UserAccountsSync_Sync();
            free(aes_key);
         }
         pairing_successful = TRUE;
         status = TRUE;
     }
     else
     {
         /* We failed to match passkey */
         DEBUG_LOG("Failed to match passkey!\n");
         
         pairing_successful = FALSE;
     }

    /* Convert the input data to little endian format before processing it for encryption */
    fastPair_ConvertEndiannessFormat(passkey_response, FAST_PAIR_ENCRYPTED_RESPONSE_LEN, little_endian_passkey_response);

    /* Encrypt Raw Passkey Response with AES Key */
#ifdef USE_SYNERGY
    CmCryptoEncryptReqSend(&theFastPair->task, (uint16 *)little_endian_passkey_response, theFastPair->session_data.aes_key);
#else
    ConnectionEncryptBlockAes(&theFastPair->task, (uint16 *)little_endian_passkey_response, theFastPair->session_data.aes_key);
#endif /* USE_SYNERGY */

    free(passkey_response);
    free(little_endian_passkey_response);

    return status;
}

static bool fastpair_StateWaitPasskeyProcessACLDisconnect(fast_pair_state_event_disconnect_args_t* args)
{
    bool status = FALSE;
    uint8 index;
    fastPairTaskData *theFastPair;

    DEBUG_LOG("fastpair_StateWaitPasskeyProcessACLDisconnect");

    theFastPair = fastPair_GetTaskData();

    if(args->disconnect_ind->tpaddr.transport == TRANSPORT_BLE_ACL)
    {
        memset(&theFastPair->rpa_bd_addr, 0x0, sizeof(bdaddr));

        for(index = 0; index < MAX_BLE_CONNECTIONS; index++)
        {
            if(BdaddrIsSame(&theFastPair->peer_bd_addr[index], &args->disconnect_ind->tpaddr.taddr.addr))
            {
                DEBUG_LOG("fastpair_StateWaitPasskeyProcessACLDisconnect. Reseting peer BD address and own RPA of index %x", index);
                memset(&theFastPair->peer_bd_addr[index], 0x0, sizeof(bdaddr));
                memset(&theFastPair->own_random_address[index], 0x0, sizeof(bdaddr));

                /* If the disconnecting device is not a peer earbud i.e. FP seeker, move to idle state. */
                if(FALSE == BtDevice_LeDeviceIsPeer(&(args->disconnect_ind->tpaddr)))
                {
                    DEBUG_LOG("fastpair_StateWaitPairingRequestProcessACLDisconnect: Remote device closed the connection. Moving to FAST_PAIR_STATE_IDLE ");
                    fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
                }
            }
        }
        status = TRUE;
    }
    return status;
}

static bool fastPair_HandleAdvBloomFilterCalc(fast_pair_state_event_crypto_hash_args_t* args)
{
    DEBUG_LOG("fastPair_HandleAdvBloomFilterCalc");

#ifdef USE_SYNERGY
    if(args->crypto_hash_cfm->resultCode == success)
#else
    if(args->crypto_hash_cfm->status == success)
#endif /*! USE_SYNERGY */
    {
        fastPair_AdvHandleHashCfm(args->crypto_hash_cfm);
        return TRUE;
    }
    return FALSE;
}

bool fastPair_StateWaitPasskeyHandleEvent(fast_pair_state_event_t event)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();
    DEBUG_LOG("fastPair_StateWaitPasskeyHandleEvent event [%d]", event.id);
    /* Return if event is related to handset connection allowed/disallowed and is handled */
    if(fastPair_HandsetConnectStatusChange(event.id))
    {
        return TRUE;
    }

    switch (event.id)
    {
        case fast_pair_state_event_timer_expire:
        {
            fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
            status = TRUE;
        }
        break;
        
        case fast_pair_state_event_crypto_decrypt:
        {
            if (event.args == NULL)
            {
                return FALSE;
            }
            status = fastPair_ValidatePasskey((fast_pair_state_event_crypto_decrypt_args_t *)event.args);
        }
        break;
        
        case fast_pair_state_event_crypto_encrypt:
        {
            if (event.args == NULL)
            {
                return FALSE;
            }
            status = fastPair_SendPasskeyResponse((fast_pair_state_event_crypto_encrypt_args_t *)event.args);
        }
        break;

        case fast_pair_state_event_passkey_write:
        {
            if (event.args == NULL)
            {
                return FALSE;
            }
            status = fastpair_PasskeyWriteEventHandler((fast_pair_state_event_passkey_write_args_t *)event.args);
        }
        break;

        case fast_pair_state_event_provider_passkey:
        {
            if (event.args == NULL)
            {
                return FALSE;
            }
            status = fastpair_ProviderPasskeyEventHandler((fast_pair_state_event_provider_passkey_write_args_t *)event.args);
        }
        break;

        case fast_pair_state_event_disconnect:
        {
            if(event.args == NULL)
            {
                return FALSE;
            }
            status = fastpair_StateWaitPasskeyProcessACLDisconnect((fast_pair_state_event_disconnect_args_t*)event.args);
        }
        break;

        case fast_pair_state_event_power_off:
        {
            fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
        }
        break;

        case fast_pair_state_event_crypto_hash:
        {
            status = fastPair_HandleAdvBloomFilterCalc((fast_pair_state_event_crypto_hash_args_t *)event.args);
        }
        break;

        default:
        {
            DEBUG_LOG("Unhandled event [%d]\n", event.id);
        }
        break;
    }

    return status;
}
