/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_hkdf_test.c
\brief      Test code for the fast pair hkdf implementation.
*/

#include <logging.h>

#ifdef FP_USE_LOCAL_DATA_FOR_DEBUG

#include "fast_pair.h"
#include "fast_pair_adv_sass.h"
#include "fast_pair_hkdf.h"
#include "fast_pair_hkdf_test.h"
#include "sass.h"

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
 * garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#define FP_HKDF_TEST_DEBUG_LOG          DEBUG_LOG
#define FP_HKDF_TEST_DEBUG_LOG_DATA     DEBUG_LOG_DATA

#define SASS_INITIAL_VECTOR_LEN          (16)
#define SASS_ADV_DATA_SIZE  4
#define SASS_ACCOUNT_KEY_LEN 16

static uint8 *expected_encrypted_data = NULL;
static uint16 expected_encrypted_data_size = 0;


#ifdef USE_SYNERGY
static void fastPair_HkdfTestAesCtrCfm(const CsrBtCmCryptoAesCtrCfm *cfm)
{
    DEBUG_LOG("fastPair_HkdfTestAesCtrCfm status 0x%x", cfm->resultCode);

    if (cfm->resultCode == success)
    {
        uint16 enc_conn_status_size_bytes = cfm->dataLen * sizeof(uint16);
        uint8 *enc_conn_status = PanicUnlessMalloc(enc_conn_status_size_bytes);

        fastPair_ConvertEndiannessFormat((uint8 *)cfm->data, enc_conn_status_size_bytes, enc_conn_status);

        FP_HKDF_TEST_DEBUG_LOG("fastPair_HkdfTestAesCtrCfm data %p dataLen %u", cfm->data, cfm->dataLen);
        FP_HKDF_TEST_DEBUG_LOG_DATA(enc_conn_status, enc_conn_status_size_bytes);

        FP_HKDF_TEST_DEBUG_LOG("  expected_encrypted_data %p expected_encrypted_data_size %u", expected_encrypted_data, expected_encrypted_data_size);
        FP_HKDF_TEST_DEBUG_LOG_DATA(expected_encrypted_data, expected_encrypted_data_size);

        if (expected_encrypted_data)
        {
            if (   (expected_encrypted_data_size == enc_conn_status_size_bytes)
                && (0 == memcmp(expected_encrypted_data, enc_conn_status, expected_encrypted_data_size)))
            {
                FP_HKDF_TEST_DEBUG_LOG("fastPair_HkdfTestAesCtrCfm encrypted data matches expected data");
            }
            else
            {
                FP_HKDF_TEST_DEBUG_LOG("fastPair_HkdfTestAesCtrCfm encrypted data DOES NOT match expected data");
            }

            free(expected_encrypted_data);
            expected_encrypted_data_size = 0;
        }

        free(enc_conn_status);
    }
}

static void fastPair_HkdfTestHandleCmPrim(Message message)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    switch (*prim)
    {
        case CSR_BT_CM_CRYPTO_AES_CTR_CFM:
            FP_HKDF_TEST_DEBUG_LOG("fastPair_HkdfTestHandleCmPrim - CSR_BT_CM_CRYPTO_AES_CTR_CFM");
            fastPair_HkdfTestAesCtrCfm((const CsrBtCmCryptoAesCtrCfm *)message);
        break;

        default:
            FP_HKDF_TEST_DEBUG_LOG("fastPair_HkdfTestHandleCmPrim, unexpected CM prim 0x%04x", *prim);
            break;
    }

    CmFreeUpstreamMessageContents(message);
}

static void fastPair_HkdfTestHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);
    UNUSED(msg);

    switch (id)
    {
#ifdef USE_SYNERGY
    case CM_PRIM:
        fastPair_HkdfTestHandleCmPrim(msg);
        break;
#endif /* USE_SYNERGY */

    default:
        break;
    }
}

static TaskData hkdf_task = {
    .handler = fastPair_HkdfTestHandler
};

/* Test generation of the encrypted connection field using the values given by Google.

    Source of test values:
    https://developers.google.com/nearby/fast-pair/early-access/specifications/appendix/testcases.
*/
bool fastPair_HkdfTestEncryptedConnectionFieldTest(void)
{
    bool ret = FALSE;

    /* Connection Status Field Raw */
    const uint8 connection_status_raw_data[SASS_ADV_DATA_SIZE] = { 0x35, 0x85, 0x38, 0x09 };
    const uint8 in_use_account_key[SASS_ACCOUNT_KEY_LEN] = { 0x04, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    const uint8 info[] = { "SASS-RRD-KEY" };
    const uint16 info_len = (ARRAY_DIM(info) - 1); /* Don't count the nul character at the end of the string in the length. */
    const uint8 encrypted_connection_status_field[] = { 0x6E, 0xBC, 0xCB, 0x21 };
    uint8 sass_key[SASS_ACCOUNT_KEY_LEN] = {0};

    FP_HKDF_TEST_DEBUG_LOG("appTestLeAdvertising_FastPairHkdf info:");
    FP_HKDF_TEST_DEBUG_LOG_DATA(info, info_len);

    FP_HKDF_TEST_DEBUG_LOG("appTestLeAdvertising_FastPairHkdf in_use_account_key:");
    FP_HKDF_TEST_DEBUG_LOG_DATA(in_use_account_key, ARRAY_DIM(in_use_account_key));

    fastpair_hkdf_t * params = PanicUnlessMalloc(sizeof(fastpair_hkdf_t));
    params->salt = NULL;
    params->salt_len = 0;
    params->ikm = in_use_account_key;
    params->ikm_len = ARRAY_DIM(in_use_account_key);
    params->info = info;
    params->info_len = info_len;
    params->okm = sass_key;
    params->okm_len = ARRAY_DIM(sass_key);

    // RRD key is derived using HKDF
    if (FastPair_HkdfSha256(params))
    {
        /* Random salt */
        const uint8 salt[] = { 0xC7, 0xC8 };
        const uint16 salt_len = sizeof(salt);

        /* Account Key Filter (this is IV)? */
        uint8 iv[SASS_INITIAL_VECTOR_LEN] = { 0 };

        /* Copy the salt into the iv. */
        memmove(iv, salt, salt_len);

        /* Pad out the iv with zeroes */
        memset(&iv[salt_len], 0x00, (SASS_INITIAL_VECTOR_LEN - salt_len));

        FP_HKDF_TEST_DEBUG_LOG("appTestLeAdvertising_FastPairHkdf sass_key:");
        FP_HKDF_TEST_DEBUG_LOG_DATA(sass_key, ARRAY_DIM(sass_key));

        FP_HKDF_TEST_DEBUG_LOG("appTestLeAdvertising_FastPairHkdf iv:");
        FP_HKDF_TEST_DEBUG_LOG_DATA(iv, ARRAY_DIM(iv));

        FP_HKDF_TEST_DEBUG_LOG("appTestLeAdvertising_FastPairHkdf: AES-CTR Encrypt Req Send");

        PanicFalse(ARRAY_DIM(sass_key) == SASS_ACCOUNT_KEY_LEN);
        PanicFalse(ARRAY_DIM(iv) == SASS_INITIAL_VECTOR_LEN);
        PanicFalse(ARRAY_DIM(connection_status_raw_data) == SASS_ADV_DATA_SIZE);

        uint8 *little_endian_key = PanicUnlessMalloc(SASS_ACCOUNT_KEY_LEN);
        uint8 *little_endian_iv = PanicUnlessMalloc(SASS_ACCOUNT_KEY_LEN);
        uint8 *little_endian_con = PanicUnlessMalloc(SASS_ADV_DATA_SIZE);

        /* Convert the big endian data to little endian before processing it for AES-CTR */
        fastPair_ConvertEndiannessFormat((uint8 *)sass_key, SASS_ACCOUNT_KEY_LEN, little_endian_key);
        fastPair_ConvertEndiannessFormat((uint8 *)iv, SASS_INITIAL_VECTOR_LEN, little_endian_iv);
        fastPair_ConvertEndiannessFormat((uint8 *)connection_status_raw_data, SASS_ADV_DATA_SIZE, little_endian_con);

        CmCryptoAesCtrReqSend(&hkdf_task, 0, CSR_BT_CM_AES_CTR_BIG_ENDIAN,
                              (uint16 *)little_endian_key, (uint16 *)little_endian_iv,
                              (SASS_ADV_DATA_SIZE >> 1), (uint16 *)little_endian_con);

        expected_encrypted_data_size = sizeof(encrypted_connection_status_field);
        expected_encrypted_data = PanicUnlessMalloc(expected_encrypted_data_size);
        memmove(expected_encrypted_data, encrypted_connection_status_field, expected_encrypted_data_size);

        free(little_endian_con);
        free(little_endian_iv);
        free(little_endian_key);

        ret = TRUE;
    }
    else
    {
        DEBUG_LOG("appTestLeAdvertising_FastPairHkdf: FAILED using HKDF to create key");
    }
    free(params);

    return ret;
}

bool fastPair_HkdfTestAesCtrTest(void)
{
    const uint8 input_data[] = {
        0x53, 0x6F, 0x6D, 0x65, 0x6F, 0x6E, 0x65, 0x27,
        0x73, 0x20, 0x47, 0x6F, 0x6F, 0x67, 0x6C, 0x65,
        0x20, 0x48, 0x65, 0x61, 0x64, 0x70, 0x68, 0x6F,
        0x6E, 0x65 };

    const uint8 secret_key[] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF
    };

    const uint8 nonce[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    const uint8 expected_encryption_result[] = {
        0xEE, 0x4A, 0x24, 0x83, 0x73, 0x80, 0x52, 0xE4,
        0x4E, 0x9B, 0x2A, 0x14, 0x5E, 0x5D, 0xDF, 0xAA,
        0x44, 0xB9, 0xE5, 0x53, 0x6A, 0xF4, 0x38, 0xE1,
        0xE5, 0xC6
    };

    uint8 *little_endian_key = PanicUnlessMalloc(sizeof(secret_key));
    uint8 *little_endian_nonce = PanicUnlessMalloc(sizeof(nonce));
    uint8 *little_endian_data = PanicUnlessMalloc(sizeof(input_data));

    /* Convert the big endian data to little endian before processing it for AES-CTR */
    fastPair_ConvertEndiannessFormat((uint8 *)secret_key, sizeof(secret_key), little_endian_key);
    fastPair_ConvertEndiannessFormat((uint8 *)nonce, sizeof(nonce), little_endian_nonce);
    fastPair_ConvertEndiannessFormat((uint8 *)input_data, sizeof(input_data), little_endian_data);

    CmCryptoAesCtrReqSend(&hkdf_task, 0, CSR_BT_CM_AES_CTR_BIG_ENDIAN,
                          (uint16 *)little_endian_key, (uint16 *)little_endian_nonce,
                          (sizeof(input_data) >> 1), (uint16 *)little_endian_data);

    expected_encrypted_data_size = sizeof(expected_encryption_result);
    expected_encrypted_data = PanicUnlessMalloc(expected_encrypted_data_size);
    memmove(expected_encrypted_data, expected_encryption_result, expected_encrypted_data_size);

    free(little_endian_data);
    free(little_endian_nonce);
    free(little_endian_key);

    return TRUE;
}

/*! \brief CTR-AES128 Encryption test.

    Source:
    https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf

    Test case: F.5.1 CTR-AES128.Encrypt
*/
bool fastPair_HkdfTestAesCtrExampleVector(void)
{
    /* Input Block: 6bc1bee22e409f96e93d7e117393172a */
    static const uint8 input_data[] = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };

    /* Key: 2b7e151628aed2a6abf7158809cf4f3c */
    static const uint8 secret_key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };

    /* IV: f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff */
    static const uint8 nonce[] = { 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };

    /* Output Block: 874d6191b620e3261bef6864990db6ce */
    const uint8 expected_encryption_result[] = { 0x87, 0x4d, 0x61, 0x91, 0xb6, 0x20, 0xe3, 0x26, 0x1b, 0xef, 0x68, 0x64, 0x99, 0x0d, 0xb6, 0xce };

    uint8 *little_endian_key = PanicUnlessMalloc(sizeof(secret_key));
    uint8 *little_endian_nonce = PanicUnlessMalloc(sizeof(nonce));
    uint8 *little_endian_data = PanicUnlessMalloc(sizeof(input_data));

    /* Convert the big endian data to little endian before processing it for AES-CTR */
    fastPair_ConvertEndiannessFormat((uint8 *)secret_key, sizeof(secret_key), little_endian_key);
    fastPair_ConvertEndiannessFormat((uint8 *)nonce, sizeof(nonce), little_endian_nonce);
    fastPair_ConvertEndiannessFormat((uint8 *)input_data, sizeof(input_data), little_endian_data);

    CmCryptoAesCtrReqSend(&hkdf_task, 0, CSR_BT_CM_AES_CTR_BIG_ENDIAN,
                          (uint16 *)little_endian_key, (uint16 *)little_endian_nonce,
                          (sizeof(input_data) >> 1), (uint16 *)little_endian_data);

    expected_encrypted_data_size = sizeof(expected_encryption_result);
    expected_encrypted_data = PanicUnlessMalloc(expected_encrypted_data_size);
    memmove(expected_encrypted_data, expected_encryption_result, expected_encrypted_data_size);

    free(little_endian_data);
    free(little_endian_nonce);
    free(little_endian_key);

    return TRUE;
}

#endif /* USE_SYNERGY */
#endif