/****************************************************************************
Copyright (c) 2018 - 2022 Qualcomm Technologies International, Ltd.


FILE NAME
    dm_crypto_api.c        

DESCRIPTION
    This file contains functions for sending message requests regarding cryptography functions on P1 for fastpair.
    
NOTES

*/
#include "connection_no_ble.h"
#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>

#include "connection.h"
//#include "common.h"

#include "cryptoalgo.h"

/* Macros for creating crypto messages */
#define MAKE_CL_CRYPTO_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T)

#ifndef UNUSED
#define UNUSED(x)       ((void)x)
#endif


/*******************************************************************************
 *                                  FUNCTIONS                                  *
 *******************************************************************************/
void ConnectionGeneratePublicPrivateKey(Task theAppTask, cl_ecc_type key_type)
{
    UNUSED(theAppTask);
    UNUSED(key_type);
}

void ConnectionGenerateSharedSecretKey(Task theAppTask, cl_ecc_type key_type, uint16 private_key[CL_CRYPTO_LOCAL_PVT_KEY_LEN], uint16 public_key[CL_CRYPTO_REMOTE_PUB_KEY_LEN])
{
    uint8 *secret = PanicUnlessMalloc(CL_CRYPTO_SECRET_KEY_LEN * sizeof(uint16));
    uint8 *key_public = PanicUnlessMalloc(CL_CRYPTO_REMOTE_PUB_KEY_LEN * sizeof(uint16));
    uint8 *key_private = PanicUnlessMalloc(CL_CRYPTO_LOCAL_PVT_KEY_LEN * sizeof(uint16));
    bool  valid_params =  TRUE;


    MAKE_CL_CRYPTO_MESSAGE(CL_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM);

    message->key_type = key_type;

    if(public_key)
    {
        memmove(key_public,  public_key, CL_CRYPTO_REMOTE_PUB_KEY_LEN * sizeof(uint16));
    }
    else
    {
        message->status = cl_ecdh_empty_public_key;    
        message->error = cl_crypto_error;  
        valid_params = FALSE;
    }
    
    if(private_key)
    {
        memmove(key_private, private_key, CL_CRYPTO_LOCAL_PVT_KEY_LEN * sizeof(uint16));
    }
    else
    {
        message->status = cl_ecdh_empty_private_key;   
        message->error = cl_crypto_error;          
        valid_params = FALSE;
    }

    if(valid_params)
    {
        if(!secp256r1_shared_secret(key_public, key_private, secret))
        {
            message->status = cl_crypto_error;    
        }
        else
        {
            message->status = cl_crypto_success;
            message->error = cl_no_error;  
            memmove(message->shared_secret_key, secret, CL_CRYPTO_SECRET_KEY_LEN * sizeof(uint16));
        }
    }

    free(secret);
    free(key_public);
    free(key_private);
   
    MessageSend(theAppTask, CL_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM, message);
}

void ConnectionEncryptBlockAes(Task theAppTask, uint16 data_array[CL_CRYPTO_AES_DATA_LEN], uint16 key_array[CL_CRYPTO_AES_KEY_LEN])
{
    uint8 *array_data = PanicUnlessMalloc(CL_CRYPTO_AES_DATA_LEN * sizeof(uint16));
    uint8 *array_key = PanicUnlessMalloc(CL_CRYPTO_AES_KEY_LEN * sizeof(uint16));
    uint8 *data_encrypted = PanicUnlessMalloc(CL_CRYPTO_AES_DATA_LEN * sizeof(uint16)); 
    bool  valid_params =  TRUE;

    MAKE_CL_CRYPTO_MESSAGE(CL_CRYPTO_ENCRYPT_CFM);

    if(data_array)
    {
        memmove(array_data, data_array, CL_CRYPTO_AES_DATA_LEN * sizeof(uint16));
    }
    else
    {
        message->status = cl_crypto_error;
        message->error = cl_aes_empty_data_array;
        valid_params =  FALSE;
    }    
    if(key_array)
    {
        memmove(array_key, key_array, CL_CRYPTO_AES_KEY_LEN * sizeof(uint16));
    }
    else
    {
        message->status = cl_crypto_error;
        message->error = cl_aes_empty_key_array;
        valid_params =  FALSE;
    }
    
    if(valid_params)
    {
        aes128_encrypt(array_data, data_encrypted, array_key);
        memmove(message->encrypted_data, data_encrypted, CL_CRYPTO_AES_DATA_LEN * sizeof(uint16));
        message->status = cl_crypto_success; /* current encrypt API doesn't have a status check */
        message->error = cl_no_error;
        message->flags = 0;
    }
    
    free(array_data);
    free(array_key);
    free(data_encrypted);

    MessageSend(theAppTask, CL_CRYPTO_ENCRYPT_CFM, message);
}


void ConnectionEncryptBlockSha256(Task theAppTask, uint16 *data_array, uint16 array_size)
{
    uint8 *array_data = PanicUnlessMalloc(array_size);
    uint8 *hash = PanicUnlessMalloc(CL_CRYPTO_SHA_HASH_LEN * sizeof(uint16));
    bool  valid_params =  TRUE;

    MAKE_CL_CRYPTO_MESSAGE(CL_CRYPTO_HASH_CFM);

    if(data_array)
    {
        memmove(array_data, data_array, array_size);
    }
    else
    {
        message->status = cl_crypto_error;
        message->error = cl_sha_empty_data_array;
        valid_params =  FALSE;
    }

    if(valid_params)
    {
        sha256(array_data, (uint8)array_size, hash);
        memmove(message->hash, hash, CL_CRYPTO_SHA_HASH_LEN * sizeof(uint16));
        message->status = cl_crypto_success; /* current  API doesnt have a status check */
        message->error = cl_no_error;
    }

    free(array_data);
    free(hash);

    MessageSend(theAppTask, CL_CRYPTO_HASH_CFM, message);
}

void ConnectionEncryptBlockSha256Continue(Task theAppTask, uint16 *data_array, uint16 array_size, uint16 current_index)
{
    UNUSED(theAppTask);
    UNUSED(data_array);
    UNUSED(array_size);
    UNUSED(current_index);
}
void ConnectionDecryptBlockAes(Task theAppTask, uint16 data_array[CL_CRYPTO_AES_DATA_LEN], uint16 key_array[CL_CRYPTO_AES_KEY_LEN])
{
    uint8 *array_data = PanicUnlessMalloc(CL_CRYPTO_AES_DATA_LEN * sizeof(uint16));
    uint8 *array_key = PanicUnlessMalloc(CL_CRYPTO_AES_KEY_LEN * sizeof(uint16));
    uint8 *data_decrypted = PanicUnlessMalloc(CL_CRYPTO_AES_DATA_LEN * sizeof(uint16)); 
    bool  valid_params =  TRUE;

    MAKE_CL_CRYPTO_MESSAGE(CL_CRYPTO_DECRYPT_CFM);
    
    if(data_array)
    {
        memmove(array_data, data_array, CL_CRYPTO_AES_DATA_LEN * sizeof(uint16));
    }
    else
    {
        message->status = cl_crypto_error;
        message->error = cl_aes_empty_data_array;
        valid_params =  FALSE;   
    }
    if(key_array)
    {
        memmove(array_key, key_array, CL_CRYPTO_AES_KEY_LEN * sizeof(uint16));
    }
    else
    {
        message->status = cl_crypto_error;
        message->error = cl_aes_empty_key_array;
        valid_params =  FALSE;        
    }
    
    if(valid_params)
    {
        aes128_decrypt(array_data, data_decrypted, array_key);
        memmove(message->decrypted_data, data_decrypted, CL_CRYPTO_AES_DATA_LEN * sizeof(uint16));

        message->status = cl_crypto_success;/* current decrypt API doesnt have a status check */
        message->error = cl_no_error;
    }

    free(array_data);
    free(array_key);
    free(data_decrypted);
    
    MessageSend(theAppTask, CL_CRYPTO_DECRYPT_CFM, message);
}

void ConnectionEncryptBlockAesEcb256(Task theAppTask, uint8 *input_data, uint8 data_size, uint8 *key)
{
    bool valid_params = TRUE;
    uint8 *encr_data = PanicUnlessMalloc(data_size * sizeof(uint8));

    MAKE_CL_CRYPTO_MESSAGE(CL_CRYPTO_ENCRYPT_AES_ECB_256_CFM);

    if(key == NULL)
    {
        message->status = cl_crypto_error;
        message->error = cl_aes_empty_key_array;
        valid_params = FALSE;
    }

    if(input_data == NULL || data_size %16 != 0)
    {
        message->status = cl_crypto_error;
        message->error = cl_aes_invalid_data_len;
        valid_params = FALSE;
    }

    if(valid_params)
    {
        if(!aes_ecb_256_encrypt(input_data, data_size, encr_data, key))
        {
            message->status = cl_crypto_error;
        }
        else
        {
            message->status = cl_crypto_success;
            message->error = cl_no_error;

            /* This memory to be freed upon receiving CL_CRYPTO_ENCRYPT_AES_ECB_256_CFM message */
            message->encr_data = PanicUnlessMalloc(data_size * sizeof(uint8));
            memcpy(message->encr_data, encr_data, data_size);
        }
    }

    free(encr_data);
    free(input_data);

    MessageSend(theAppTask, CL_CRYPTO_ENCRYPT_AES_ECB_256_CFM, message);
}

void ConnectionGenerateEccP160PrivateKey(Task theAppTask, uint8* random, uint8 random_data_size)
{
    bool valid_params = TRUE;
    uint8 *private_key;

    MAKE_CL_CRYPTO_MESSAGE(CL_CRYPTO_GENERATE_ECC_P160_PRIVATE_KEY_CFM);

    if(random == NULL || random_data_size != 32)
    {
        message->status = cl_crypto_error;
        message->error = cl_ecc_invalid_data_len;
        valid_params = FALSE;
    }

    if(valid_params)
    {
        /* This memory to be freed upon receiving CL_CRYPTO_GENERATE_ECC_P160_PRIVATE_KEY_CFM message */
        private_key = PanicUnlessMalloc(CL_CRYPTO_ECC_P160_PRIVATE_KEY_LEN * sizeof(uint8));

        if(!ecdh_p160_generate_private_key(private_key, random, random_data_size))
        {
            free(private_key);
            message->status = cl_crypto_error;
            message->private_key = NULL;
        }
        else
        {
            message->status = cl_crypto_success;
            message->error = cl_no_error;

            message->private_key = private_key;
        }
    }

    free(random);

    MessageSend(theAppTask, CL_CRYPTO_GENERATE_ECC_P160_PRIVATE_KEY_CFM, message);
}

void ConnectionGenerateEccP160PublicKey(Task theAppTask, uint8* private_key, bool compressed_format)
{
    bool valid_params = TRUE;
    uint8 *public_key;

    MAKE_CL_CRYPTO_MESSAGE(CL_CRYPTO_GENERATE_ECC_P160_PUBLIC_KEY_CFM);

    if(private_key == NULL || !compressed_format)
    {
        message->status = cl_crypto_error;
        message->error = cl_ecc_invalid_data_len;
        valid_params = FALSE;
    }

    if(valid_params)
    {
        /* This memory to be freed upon receiving CL_CRYPTO_GENERATE_ECC_P160_PUBLIC_KEY_CFM message */
        public_key = PanicUnlessMalloc(CL_CRYPTO_ECC_P160_COMPRESSED_PUBLIC_KEY_LEN * sizeof(uint8));

        if(!ecdh_p160_generate_public_key(public_key, private_key))
        {
            free(public_key);
            message->status = cl_crypto_error;
            message->public_key = NULL;
        }
        else
        {
            message->status = cl_crypto_success;
            message->error = cl_no_error;

            message->public_key = public_key;
            message->compressed_format = TRUE;
        }
    }

    free(private_key);

    MessageSend(theAppTask, CL_CRYPTO_GENERATE_ECC_P160_PUBLIC_KEY_CFM, message);
}
