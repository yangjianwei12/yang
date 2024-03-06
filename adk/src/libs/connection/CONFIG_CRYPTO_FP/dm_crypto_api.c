/****************************************************************************
Copyright (c) 2008 - 2022 Qualcomm Technologies International, Ltd.


FILE NAME
    dm_crypto_api.c        

DESCRIPTION
    This file contains functions for sending message requests regarding 
    cryptography functions (ConnectionGenerateSharedSecretKey, ConnectionEncryptBlockAesEcb256) on P1 
    and other cryptography functions to the Bluestack.
    
NOTES

*/

#include <vm.h>

#include "connection.h"
#include "connection_private.h"
#include "common.h"
#include "cryptoalgo.h"

#define SHA_DATA_OCTET_LEN ((sizeof(uint16) == 1) ? CL_CRYPTO_SHA_DATA_LEN : CL_CRYPTO_SHA_DATA_LEN * 2 )

#ifndef UNUSED
#define UNUSED(x)       ((void)x)
#endif

/*******************************************************************************
 *                                  FUNCTIONS                                  *
 *******************************************************************************/
void ConnectionGeneratePublicPrivateKey(Task theAppTask, cl_ecc_type key_type)
{
    uint8_t set_key_type;
        
    /* Ensure that an appropriate key type was provided. If not, "short-circuit" the message
       process by sending the originating task an error struct with the appropriate error type */
    switch (key_type)
    {
        case cl_crypto_ecc_p192:
            set_key_type = DM_CRYPTO_ECC_P192;
            break;
            
        case cl_crypto_ecc_p256:
            set_key_type = DM_CRYPTO_ECC_P256;
            break;
        
        default:
            {
                /* Allocate error struct and no need to free the prim as we have not allocated any prim earlier. */
                MAKE_CL_MESSAGE(CL_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_CFM);
                message->status = cl_crypto_error;
                message->error = cl_ecc_invalid_key_type;
                MessageSend(theAppTask, CL_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_CFM, message);
            }
            
            return;
    }

    {
        /* Create message prim. */
        MAKE_PRIM_T(DM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_REQ);
        /* Set the prim's fields to the appropriate values and send it. */
        prim->key_type = set_key_type;
        prim->phandle = 0;
        prim->context = (context_t) theAppTask;
        VmSendDmPrim(prim);
    }
}

void ConnectionGenerateSharedSecretKey(Task theAppTask, cl_ecc_type key_type, uint16 private_key[CL_CRYPTO_LOCAL_PVT_KEY_LEN], uint16 public_key[CL_CRYPTO_REMOTE_PUB_KEY_LEN])
{
    uint8 *secret = PanicUnlessMalloc(CL_CRYPTO_SECRET_KEY_LEN * sizeof(uint16));
    uint8 *key_public = PanicUnlessMalloc(CL_CRYPTO_REMOTE_PUB_KEY_LEN * sizeof(uint16));
    uint8 *key_private = PanicUnlessMalloc(CL_CRYPTO_LOCAL_PVT_KEY_LEN * sizeof(uint16));
    bool  valid_params =  TRUE;


    MAKE_CL_MESSAGE(CL_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM);

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
    /* Create message prim. */
    /* The prim macro uses PanicUnlessMalloc, so prim will always be valid. */
    MAKE_PRIM_T(DM_CRYPTO_ENCRYPT_REQ);
    
    /* Ensure that the provided arrays are actually non-null. If not, "short-circuit" the message
       process by sending the originating task an error struct with the appropriate error type */
    if (data_array)
    {
        memmove(&prim->data, data_array, CL_CRYPTO_AES_DATA_LEN * sizeof(uint16));
    }
    else
    {
        /* Allocate error struct and free the previously allocated prim to eliminate memory leaks. */
        MAKE_CL_MESSAGE(CL_CRYPTO_ENCRYPT_CFM);
        free(prim);
        message->status = cl_crypto_error;
        message->error = cl_aes_empty_data_array;
        MessageSend(theAppTask, CL_CRYPTO_ENCRYPT_CFM, message);
        return;
    }
    
    if (key_array)
    {
        memmove(&prim->encryption_key, key_array, CL_CRYPTO_AES_KEY_LEN * sizeof(uint16));
    }
    else
    {
        /* Allocate error struct and free the previously allocated prim to eliminate memory leaks. */
        MAKE_CL_MESSAGE(CL_CRYPTO_ENCRYPT_CFM);
        free(prim);
        message->status = cl_crypto_error;
        message->error = cl_aes_empty_key_array;
        MessageSend(theAppTask, CL_CRYPTO_ENCRYPT_CFM, message);
        return;
    }
    
    /* Set the prim's fields to the appropriate values and send it. */
    prim->flags     = 0;
    prim->phandle   = 0;
    prim->context   = (context_t) theAppTask;
    
    VmSendDmPrim(prim);
}


void ConnectionEncryptBlockSha256(Task theAppTask, uint16 *data_array, uint16 array_size)
{   
    /* Ensure that the provided array is actually non-null. If not, "short-circuit" the message
       process by sending the originating task an error struct with the appropriate error type */
    if (!data_array)
    {
        /* Allocate error struct and free the previously allocated prim to eliminate memory leaks. */
        MAKE_CL_MESSAGE(CL_CRYPTO_HASH_CFM);
        message->status = cl_crypto_error;
        message->error = cl_sha_empty_data_array;
        MessageSend(theAppTask, CL_CRYPTO_HASH_CFM, message);
    }
    else
    {
        uint16 move_array_size;

        /* Create message prim. */
        /* The prim macro uses PanicUnlessMalloc, so prim will always be valid. */
        MAKE_PRIM_T(DM_CRYPTO_HASH_REQ);
    
        prim->flags     = 0;
        prim->phandle   = 0;
        prim->context   = (context_t) theAppTask;
        memset(&prim->data, 0, SHA_DATA_OCTET_LEN);

        /* Handling packing octets into words if the chip used is Bluecore.
           array_size is in octets. */
        if (sizeof(uint16) == 1)
        {
            move_array_size = (array_size + 1) / 2;
        }
        else
        {
            move_array_size = array_size;
        }

        /*  If the valid characters are less than 32 bytes, the hash operation can
            be completed in one pass. */
        if (move_array_size <= SHA_DATA_OCTET_LEN)
        {
            prim->data_size = array_size;
            memmove(&prim->data, data_array, move_array_size);
            prim->operation = DM_CRYPTO_SINGLE_BLOCK;
        }
        else
        {
            /* Since the data is larger than a single block of 32 bytes, the first 32 bytes are sent for hashing.
               The end of non-zero characters within the array is stored in sha_buff_offset, to be used for calculating when
               hashing is complete. ConnectionEncryptBlockSha256Continue will be called after the current hashing of 32 bytes is completed. */
            prim->data_size = SHA_DATA_OCTET_LEN;
            memmove(&prim->data, data_array, SHA_DATA_OCTET_LEN);
            prim->operation = DM_CRYPTO_DATA_START;
        }

        VmSendDmPrim(prim);
    }
}

void ConnectionEncryptBlockSha256Continue(Task theAppTask, uint16 *data_array, uint16 array_size, uint16 current_index)
{   
    /* Ensure that the provided array is actually non-null. If not, "short-circuit" the message
       process by sending the originating task an error struct with the appropriate error type */
    if (!data_array)
    {
        /* Allocate error struct and free the previously allocated prim to eliminate memory leaks. */
        MAKE_CL_MESSAGE(CL_CRYPTO_HASH_CFM);
        message->status = cl_crypto_error;
        message->error = cl_sha_cont_empty_data_array;
        MessageSend(theAppTask, CL_CRYPTO_HASH_CFM, message);
    }
    else
    {
        uint16 size;
        uint16 move_size;

        /* Create message prim. */
        /* The prim macro uses PanicUnlessMalloc, so prim will always be valid. */
        MAKE_PRIM_T(DM_CRYPTO_HASH_REQ);

        prim->flags     = 0;
        prim->phandle   = 0;
        prim->context   = 0;
        memset(&prim->data, 0, SHA_DATA_OCTET_LEN);

        /* Calculate the amount of remaining data to be hashed. */
        size = array_size - current_index;

        /* Handling packing octets into words if the chip used is Bluecore. */
        if (sizeof(uint16) == 1)
        {
            move_size = (size + 1) / 2;
        }
        else
        {
            move_size = size;
        }

        /* Remaining data can be hashed as one block, and therefore this will be the last block sent. */
        if (move_size <= SHA_DATA_OCTET_LEN)
        {
            prim->operation = DM_CRYPTO_DATA_END;
            prim->data_size = size;
            memmove(&prim->data, &data_array[current_index / 2], move_size);
        }
        else
        {
            /* Remaining data is still more than 32 bytes, so send a block of 32 bytes. */
            prim->operation = DM_CRYPTO_DATA_CONTI;
            prim->data_size = SHA_DATA_OCTET_LEN;
            memmove(&prim->data, &data_array[current_index / 2], SHA_DATA_OCTET_LEN);
        }

        VmSendDmPrim(prim);
    }
}

static void sendDecryptAesBlockError(Task theAppTask, cl_crypto_error_status error)
{
    MAKE_CL_MESSAGE(CL_CRYPTO_DECRYPT_CFM);
    memset(message, 0, sizeof(CL_CRYPTO_DECRYPT_CFM_T));
    message->status =  cl_crypto_error;
    message->error = error;
    MessageSend(theAppTask, CL_CRYPTO_DECRYPT_CFM, message);
}

void ConnectionDecryptBlockAes(Task theAppTask, uint16 data_array[CL_CRYPTO_AES_DATA_LEN], uint16 key_array[CL_CRYPTO_AES_KEY_LEN])
{
    if (!data_array)
    {
        sendDecryptAesBlockError(theAppTask, cl_aes_empty_data_array);
    }
    else if (!key_array)
    {
        sendDecryptAesBlockError(theAppTask, cl_aes_empty_key_array);
    }
    else
    {
        MAKE_PRIM_T(DM_CRYPTO_DECRYPT_REQ);
        prim->phandle = 0;
        prim->flags = 0;
        prim->context = (context_t) theAppTask;
        memmove(&prim->cipher_text, data_array, CL_CRYPTO_AES_DATA_LEN * sizeof(uint16));
        memmove(&prim->decryption_key, key_array, CL_CRYPTO_AES_KEY_LEN * sizeof(uint16));
        VmSendDmPrim(prim);
    }
}

static void sendEncryptDecryptAesCtrError(Task theAppTask, cl_crypto_error_status error)
{
    MAKE_CL_MESSAGE(CL_CRYPTO_ENCRYPT_DECRYPT_AES_CTR_CFM);
    memset(message, 0, sizeof(CL_CRYPTO_ENCRYPT_DECRYPT_AES_CTR_CFM_T));
    message->status =  cl_crypto_error;
    message->error = error;
    MessageSend(theAppTask, CL_CRYPTO_ENCRYPT_DECRYPT_AES_CTR_CFM, message);
}

void ConnectionEncryptDecryptAesCtrReq(
        Task                theAppTask,
        uint32              counter,
        cl_aes_ctr_flags_t  flags,
        uint16              secret_key[CL_CRYPTO_AES_KEY_LEN],
        uint16              nonce[CL_CRYPTO_AES_NONCE_LEN],
        uint16              data_len,
        uint16              *data
        )
{
    if (flags > cl_aes_ctr_big_endian)
    {
        sendEncryptDecryptAesCtrError(theAppTask, cl_aes_invalid_flag);
    }
    else if (!secret_key)
    {
        sendEncryptDecryptAesCtrError(theAppTask, cl_aes_empty_key_array);
    }
    else if (!nonce)
    {
        sendEncryptDecryptAesCtrError(theAppTask, cl_aes_empty_nonce_array);
    }
    else if (!data)
    {
        sendEncryptDecryptAesCtrError(theAppTask, cl_aes_empty_data_array);
    }
    else if (data_len == 0 || data_len > CL_CRYPTO_AES_CTR_MAX_DATA_LEN)
    {
        sendEncryptDecryptAesCtrError(theAppTask, cl_aes_invalid_data_len);
    }
    else
    {
        MAKE_PRIM_T(DM_CRYPTO_AES_CTR_REQ);
        prim->phandle = 0;
        prim->flags = (uint16)flags;
        prim->context = (context_t) theAppTask;
        prim->counter = counter;
        memmove(&prim->secret_key, secret_key, CL_CRYPTO_AES_KEY_LEN * sizeof(uint16));
        memmove(&prim->nonce, nonce, CL_CRYPTO_AES_NONCE_LEN * sizeof(uint16));
        prim->data_len = data_len;
        prim->data = (uint16 *)VmGetHandleFromPointer(data);
        VmSendDmPrim(prim);
    }
}

void ConnectionEncryptBlockAesEcb256(Task theAppTask, uint8 *input_data, uint8 data_size, uint8 *key)
{
    bool valid_params = TRUE;
    uint8 *encr_data = PanicUnlessMalloc(data_size * sizeof(uint8));

    MAKE_CL_MESSAGE(CL_CRYPTO_ENCRYPT_AES_ECB_256_CFM);

    if(key == NULL)
    {
        message->status = cl_aes_empty_key_array;
        message->error = cl_crypto_error;
        valid_params = FALSE;
    }

    if(input_data == NULL || data_size %16 != 0)
    {
        message->status = cl_aes_invalid_data_len;
        message->error = cl_crypto_error;
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

    MAKE_CL_MESSAGE(CL_CRYPTO_GENERATE_ECC_P160_PRIVATE_KEY_CFM);

    if(random == NULL || random_data_size != 32)
    {
        message->status = cl_ecc_invalid_data_len;
        message->error = cl_crypto_error;
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

    MAKE_CL_MESSAGE(CL_CRYPTO_GENERATE_ECC_P160_PUBLIC_KEY_CFM);

    if(private_key == NULL || !compressed_format)
    {
        message->status = cl_ecc_invalid_data_len;
        message->error = cl_crypto_error;
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
