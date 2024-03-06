/****************************************************************************
Copyright (c) 2018 - 2022 Qualcomm Technologies International, Ltd.


FILE NAME
    cryptoalgo.h

DESCRIPTION
    A cryptographic API library on P1 for applications.Note: This is to be changed later for P0 support.

*/

#ifndef _CRYPTOALGO_H__
#define _CRYPTOALGO_H__

#define AES_BLOCK_SIZE 16
#define SHA256_DIGEST_SIZE 32
#define AES_CTR_NONCE_SIZE 8

#define SECP256R1_PUBLIC_KEY_SIZE       64
#define SECP256R1_PRIVATE_KEY_SIZE      32
#define SECP256R1_SHARED_SECRET_SIZE    32

typedef struct sha256_internal_data_t * sha256_context;

/*Function to do AES128 encrypt*/
extern void aes128_encrypt(uint8 in[AES_BLOCK_SIZE], uint8 out[AES_BLOCK_SIZE], uint8 key[AES_BLOCK_SIZE]);

/*Function to do AES128 decrypt*/
extern void aes128_decrypt(uint8 in[AES_BLOCK_SIZE], uint8 out[AES_BLOCK_SIZE], uint8 key[AES_BLOCK_SIZE]);

/*! \brief Initalises SHA256
 *  \param context A pointer to a NULL SHA256 context to be populated */
extern void Sha256_Init(sha256_context * context);

/*! \brief Updates the SHA256 hash
 *  \param context A pointer to the SHA256 context
 *  \param data Pointer to the data to hash
 *  \param length The number of bytes to hash from data
 *  \note Data must be passed in multiples of 64 bytes, except for the last call which can pass any number of bytes */
extern void Sha256_Update(sha256_context * context, const uint8 * data, uint16 length);

/*! \brief Finialise the SHA256 hash
 *  \param A pointer to the SHA256 context
 *  \param digest A pointer to array which will be populated with the completed hash */
extern void Sha256_Final(sha256_context * context, uint8 digest[SHA256_DIGEST_SIZE]);

/*Function to do sha256 hash*/
extern void sha256(const uint8 *data, uint16 len, uint8 digest[SHA256_DIGEST_SIZE]);

/*Function to do generate a shared secret key with ECDH*/
extern bool secp256r1_shared_secret(const uint8 public_key[SECP256R1_PUBLIC_KEY_SIZE],
                             const uint8 private_key[SECP256R1_PRIVATE_KEY_SIZE],
                             uint8 shared_secret[SECP256R1_SHARED_SECRET_SIZE]);

/**
  * Encrypt the data using AES-CTR.
  * Please note that no memory is allocated. All the arrays should be pre-allocated by the caller.
  * Please note that the encrypted output array(encr_out) has the same size as the input array(data)
  * and should be pre-allocated by the caller.
  *
  * Parameters:
  * [in] data      Input data array
  * [in] data_sz   data size in bytes
  * [out] encr_out  Encrypted data. The data has the same size as input data (data_sz).
  * [in] key   Key array
  * [in] nonce nonce array
  *
  * Return: TRUE if the function is succesful
  *         FALSE if the function fails due to wrong input
  */
bool aesCtr_encrypt(uint8 *data, uint16 data_sz, uint8 *encr_out, uint8 key[AES_BLOCK_SIZE], uint8 nonce[AES_CTR_NONCE_SIZE]);

/**
  * Decrypt the encrypted Data using AES-CTR.
  *
  * Please note that no memory is allocated. All the arrays should be pre-allocated by the caller.
  * Please note that the decrypted output array(decr_out) has the same size as the input array(encr_in)
  * and should be pre-allocated by the caller.
  *
  * Parameters:
  * [in] encr_in      Input encryped data array
  * [in] encr_in_sz   data size in bytes
  * [out] decr_out    Decrypted data. The data has the same size as input data (encr_in_sz).
  * [in] key   Key array
  * [in] nonce nonce array.
  *
  * Return: TRUE if the function is succesful
  *         FALSE if the function fails due to wrong input
  */
bool aesCtr_decrypt(uint8 *encr_in, uint16 encr_in_sz, uint8 *decr_out, uint8 key[AES_BLOCK_SIZE], uint8 nonce[AES_CTR_NONCE_SIZE]);

/** Perform HMAC-SHA256 for Fast Pair functionality.
  * Please note that the caller should allocate memory for the arrays used.
  *
  * Parameters:
  * [in] data      Pointer to the data to be hashed
  * [in] data_sz   data size in bytes
  * [out] hmac_sha256_out  HMAC-SHA256 output array
  * [in] key   Key array
  * [in] nonce Pointer to the nonce used for hashing
  * [in] nonce_sz  nonce size in bytes
  *
  * Return: TRUE if the function is succesful
  *         FALSE if the function fails. Wrong input or unavailability of memory are some of the reasons
  */
bool hmac_sha256_fastpair(uint8 *data, uint16 data_sz, uint8 hmac_sha256_out[SHA256_DIGEST_SIZE], uint8 key[AES_BLOCK_SIZE], uint8* nonce, uint16 nonce_sz);

/** Perform HMAC-SHA256
  * Please note that the caller should allocate memory for the arrays used.
  *
  * Parameters:
  * [in] data      Pointer to the data to be hashed
  * [in] data_sz   data size in bytes
  * [out] hmac_sha256_out  Pointer to the hmac sha256 output data of 32 bytes
  * [in] key   Pointer to the key used for hashing
  * [in] key_size   key size
  *
  * Return: TRUE if the function is succesful
  *         FALSE if the function fails. Wrong input or unavailability of memory are some of the reasons
  */
bool hmac_sha256(uint8 *data, uint16 data_sz, uint8 *hmac_sha256_out, uint8 *key, uint16 key_size);

/** Encrypt the data using AES ECB 128 crypto algorithm
  * Please note that the caller should allocate memory for the arrays used.
  * Function accepts only data size in multiple of 16, any padding if necessary shall be ensured by caller.
  *
  * Parameters:
  * [in] data      Pointer to the data to be encrypted
  * [in] data_size   data size in bytes
  * [out] encr_data  encrypted output data
  * [in] key   Key used for encryption
  *
  * Return: TRUE if the function is succesful
  *         FALSE if the function fails. Wrong input or unavailability of memory are some of the reasons
  *
  */
extern bool aes_ecb_128_encrypt(uint8 *data, uint8 data_size, uint8* encr_data, uint8* key);

/** Decrypt the data using AES ECB 128 crypto algorithm
  * Please note that the caller should allocate memory for the arrays used.
  * Function accepts only data size in multiple of 16, any padding if necessary shall be ensured by caller.
  *
  * Parameters:
  * [in] encr_data      Pointer to the data to be decrypted
  * [in] encr_data_size   data size in bytes
  * [out] decr_data  decrypted output data
  * [in] key   Key used for decryption
  *
  * Return: TRUE if the function is succesful
  *         FALSE if the function fails. Wrong input or unavailability of memory are some of the reasons
  *
  */
extern bool aes_ecb_128_decrypt(uint8* encr_data, uint8 encr_data_size, uint8* decr_data, uint8* key);

/** Encrypt the data using AES ECB 256 crypto algorithm
  * Please note that the caller should allocate memory for the arrays used and free it as per its usage.
  * Function accepts only data size in multiple of 16, any padding if necessary shall be ensured by caller.
  * Function accepts the key of size 32 bytes, key size less than or greater than 32 bytes will not be entertained.
  *
  * Parameters:
  * [in] input_data      Pointer to the data to be encrypted
  * [in] data_size   data size in bytes
  * [out] encr_data  encrypted output data
  * [in] key   Key used for encryption
  *
  * Return: TRUE if the function is succesful
  *         FALSE if the function fails. Wrong input or unavailability of memory are some of the reasons
  *
  */
bool aes_ecb_256_encrypt(uint8_t *input_data, uint8_t data_size, uint8_t *encr_data, uint8_t *key);

/** Generate private key for secp160r1 curve.
  * Please note that the caller should allocate memory for the arrays used and free it as per its usage.
  * Function accepts only random data size of length 32 bytes.
  * Generated private key uint8_t array is in big endian format and is of length 21 bytes.
  * First byte in the private key array shall be ignored by the application.
  *
  * Parameters:
  * [out] private_key       Pointer to the private key uint8_t array. Should be freed by the caller
                            if this API returns FALSE.
  * [in]  random            Private key is calculated using supplied random number,
                            resultant private key will be within curve parameter.
  * [in]  random_data_size  Size of random data.
  *
  * Return: TRUE if the function is successful.
  *         FALSE if the function fails. If random or private_key is NULL or random data size is not 32 bytes.
  *
  */
bool ecdh_p160_generate_private_key(uint8_t*          private_key,
                                             uint8_t* random,
                                             uint8_t  random_data_size);

/** Generate compressed public key for secp160r1 curve.
  * Please note that the caller should allocate memory for the arrays used and free it as per its usage.
  * Generated compressed public key uint8_t array is in big endian format and is of length 21 bytes.
  * First byte in the compressed public key array shall be ignored by the application.
  *
  * Parameters:
  * [out] compressed_public_key   Pointer to the compressed public key uint8_t array. Should be freed by the caller
                                  if this API returns FALSE.
  * [in]  private_key             Pointer to the private key uint8_t array.
  *
  * Return: TRUE if the function is successful.
  *         FALSE if the function fails. If private_key or compressed_public_key is NULL.
  */
bool ecdh_p160_generate_public_key(uint8_t* compressed_public_key,
                                           uint8_t* private_key);

#endif
