/******************************************************************************
 Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
 ******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

static void CsrBtCmDmCryptoGeneratePublicPrivateKeyCfmSend(CsrSchedQid appHandle,
                                                           CsrBtResultCode resultCode)
{
    CsrBtCmCryptoGeneratePublicPrivateKeyCfm * prim   = (CsrBtCmCryptoGeneratePublicPrivateKeyCfm *)CsrPmemZalloc(sizeof(CsrBtCmCryptoGeneratePublicPrivateKeyCfm));
    prim->type                                        = CSR_BT_CM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_CFM;
    prim->resultCode                                  = resultCode;
    prim->resultSupplier                              = CSR_BT_SUPPLIER_CM;
    CsrBtCmPutMessage(appHandle, prim);
}

static void CsrBtCmDmCryptoGenerateSharedSecretKeyCfmSend(CsrSchedQid appHandle,
                                                          CsrBtResultCode resultCode)
{
    CsrBtCmCryptoGenerateSharedSecretKeyCfm * prim    = (CsrBtCmCryptoGenerateSharedSecretKeyCfm *)CsrPmemZalloc(sizeof(CsrBtCmCryptoGenerateSharedSecretKeyCfm));
    prim->type                                        = CSR_BT_CM_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM;
    prim->resultCode                                  = resultCode;
    prim->resultSupplier                              = CSR_BT_SUPPLIER_CM;
    CsrBtCmPutMessage(appHandle, prim);
}

static void CsrBtCmDmCryptoEncryptCfmSend(CsrSchedQid appHandle,
                                          CsrBtResultCode resultCode)
{
    CsrBtCmCryptoEncryptCfm * prim                    = (CsrBtCmCryptoEncryptCfm *)CsrPmemZalloc(sizeof(CsrBtCmCryptoEncryptCfm));
    prim->type                                        = CSR_BT_CM_CRYPTO_ENCRYPT_CFM;
    prim->resultCode                                  = resultCode;
    prim->resultSupplier                              = CSR_BT_SUPPLIER_CM;
    CsrBtCmPutMessage(appHandle, prim);
}

static void CsrBtCmDmCryptoHashCfmSend(CsrSchedQid appHandle,
                                       CsrBtResultCode resultCode)
{
    CsrBtCmCryptoHashCfm * prim                       = (CsrBtCmCryptoHashCfm *)CsrPmemZalloc(sizeof(CsrBtCmCryptoHashCfm));
    prim->type                                        = CSR_BT_CM_CRYPTO_HASH_CFM;
    prim->resultCode                                  = resultCode;
    prim->resultSupplier                              = CSR_BT_SUPPLIER_CM;
    CsrBtCmPutMessage(appHandle, prim);
}

static void CsrBtCmDmCryptoDecryptCfmSend(CsrSchedQid appHandle,
                                          CsrBtResultCode resultCode)
{
    CsrBtCmCryptoDecryptCfm * prim                    = (CsrBtCmCryptoDecryptCfm *)CsrPmemZalloc(sizeof(CsrBtCmCryptoDecryptCfm));
    prim->type                                        = CSR_BT_CM_CRYPTO_DECRYPT_CFM;
    prim->resultCode                                  = resultCode;
    prim->resultSupplier                              = CSR_BT_SUPPLIER_CM;
    CsrBtCmPutMessage(appHandle, prim);
}

static void CsrBtCmDmCryptoAesCtrCfmSend(CsrSchedQid appHandle,
                                         CsrBtResultCode resultCode)
{
    CsrBtCmCryptoAesCtrCfm * prim                     = (CsrBtCmCryptoAesCtrCfm *)CsrPmemZalloc(sizeof(CsrBtCmCryptoAesCtrCfm));
    prim->type                                        = CSR_BT_CM_CRYPTO_AES_CTR_CFM;
    prim->resultCode                                  = resultCode;
    prim->resultSupplier                              = CSR_BT_SUPPLIER_CM;
    CsrBtCmPutMessage(appHandle, prim);
}

static void CsrBtCmDmCryptoErrorSend(CsrSchedQid appHandle, CsrBtCmPrim primType, CsrBtResultCode resultCode)
{
    switch(primType)
    {
        case CSR_BT_CM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_REQ:
            CsrBtCmDmCryptoGeneratePublicPrivateKeyCfmSend(appHandle, resultCode);
            break;
        case CSR_BT_CM_CRYPTO_GENERATE_SHARED_SECRET_KEY_REQ:
            CsrBtCmDmCryptoGenerateSharedSecretKeyCfmSend(appHandle, resultCode);
            break;
        case CSR_BT_CM_CRYPTO_ENCRYPT_REQ:
            CsrBtCmDmCryptoEncryptCfmSend(appHandle, resultCode);
            break;
        case CSR_BT_CM_CRYPTO_HASH_REQ:
            CsrBtCmDmCryptoHashCfmSend(appHandle, resultCode);
            break;
        case CSR_BT_CM_CRYPTO_DECRYPT_REQ:
            CsrBtCmDmCryptoDecryptCfmSend(appHandle, resultCode);
            break;
        case CSR_BT_CM_CRYPTO_AES_CTR_REQ:
            CsrBtCmDmCryptoAesCtrCfmSend(appHandle, resultCode);
            break;
    }
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmCryptoGeneratePublicPrivateKeyReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmCryptoGeneratePublicPrivateKeyReq *prim = cmData->recvMsgP;

    cmData->dmVar.appHandle = prim->appHandle;

    /* Ensure that an appropriate key type was provided */
    switch(prim->keyType)
    {
        case CSR_BT_CM_CRYPTO_ECC_P192:
            dm_crypto_generate_public_private_key_req(CSR_BT_CM_IFACEQUEUE,
                                                      DM_CRYPTO_ECC_P192,
                                                      0,
                                                      NULL);
            break;

        case CSR_BT_CM_CRYPTO_ECC_P256:
            dm_crypto_generate_public_private_key_req(CSR_BT_CM_IFACEQUEUE,
                                                      DM_CRYPTO_ECC_P256,
                                                      0,
                                                      NULL);
            break;

        default:
            CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                     prim->type,
                                     CSR_BT_RESULT_CODE_CM_CRYPTO_ECC_INVALID_KEY_TYPE);
            break;
    }
}

void CsrBtCmDmCryptoGenerateSharedSecretKeyReqHandler(cmInstanceData_t *cmData)
{
    CsrUint16 zeroPubKeyArray[CSR_BT_CM_CRYPTO_PUBLIC_KEY_LEN] = {0};
    CsrUint16 zeroPvtKeyArray[CSR_BT_CM_CRYPTO_PRIVATE_KEY_LEN] = {0};
    CsrBtCmCryptoGenerateSharedSecretKeyReq *prim = cmData->recvMsgP;

    cmData->dmVar.appHandle = prim->appHandle;

    /* Ensure that the provided arrays are non-zero */
    if (CsrMemCmp(prim->publicKey, zeroPubKeyArray, sizeof(prim->publicKey)) == 0)
    {
        CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                 prim->type,
                                 CSR_BT_RESULT_CODE_CM_CRYPTO_ECDH_EMPTY_PUBLIC_KEY);
    }
    else if (CsrMemCmp(prim->privateKey, zeroPvtKeyArray, sizeof(prim->privateKey)) == 0)
    {
        CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                 prim->type,
                                 CSR_BT_RESULT_CODE_CM_CRYPTO_ECDH_EMPTY_PRIVATE_KEY);
    }
    else
    {
        /* Ensure that an appropriate key type was provided */
        switch(prim->keyType)
        {
            case CSR_BT_CM_CRYPTO_ECC_P192:
                dm_crypto_generate_shared_secret_key_req(CSR_BT_CM_IFACEQUEUE,
                                                         DM_CRYPTO_ECC_P192, 
                                                         0,
                                                         prim->privateKey,
                                                         prim->publicKey,
                                                         NULL);
                break;

            case CSR_BT_CM_CRYPTO_ECC_P256:
                dm_crypto_generate_shared_secret_key_req(CSR_BT_CM_IFACEQUEUE,
                                                         DM_CRYPTO_ECC_P256,
                                                         0,
                                                         prim->privateKey,
                                                         prim->publicKey,
                                                         NULL);
                break;

            default:
                CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                         prim->type,
                                         CSR_BT_RESULT_CODE_CM_CRYPTO_ECC_INVALID_KEY_TYPE);
                break;
        }
    }
}

void CsrBtCmDmCryptoEncryptReqHandler(cmInstanceData_t *cmData)
{
    CsrUint16 zeroDataArray[CSR_BT_CM_CRYPTO_AES_DATA_LEN] = {0};
    CsrUint16 zeroKeyArray[CSR_BT_CM_CRYPTO_AES_KEY_LEN] = {0};
    CsrBtCmCryptoEncryptReq *prim = cmData->recvMsgP;

    cmData->dmVar.appHandle = prim->appHandle;

    /* Ensure that the provided arrays are non-zero */
    if (CsrMemCmp(prim->dataArray, zeroDataArray, sizeof(prim->dataArray)) == 0)
    {
        CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                 prim->type, 
                                 CSR_BT_RESULT_CODE_CM_CRYPTO_AES_EMPTY_DATA_ARRAY);
    }
    else if (CsrMemCmp(prim->keyArray, zeroKeyArray, sizeof(prim->keyArray)) == 0)
    {
        CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                 prim->type,
                                 CSR_BT_RESULT_CODE_CM_CRYPTO_AES_EMPTY_KEY_ARRAY);
    }
    else
    {
        dm_crypto_encrypt_req(CSR_BT_CM_IFACEQUEUE,
                              prim->flags,
                              0,
                              prim->dataArray,
                              prim->keyArray,
                              NULL);
    }
}

static void cryptoHashReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmCryptoHashReq *prim = cmData->recvMsgP;
    CsrUint16 dataSize = CSR_BT_CM_CRYPTO_SHA_DATA_LEN * sizeof(CsrUint16);
    CsrUint16 data[CSR_BT_CM_CRYPTO_SHA_DATA_LEN] = {0};

    cmData->dmVar.appHandle = prim->appHandle;

    /* Ensure that the provided array is non-zero */
    if (!prim->dataArray)
    {
        CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                 prim->type,
                                 CSR_BT_RESULT_CODE_CM_CRYPTO_SHA_EMPTY_DATA_ARRAY);
        return;
    }

    /*  If the valid characters are less than 32 bytes, the hash operation can 
        be completed in one pass. */
    if (prim->arraySize <= dataSize)
    {
        SynMemMoveS(data, prim->arraySize, prim->dataArray, prim->arraySize);
        dm_crypto_hash_req(CSR_BT_CM_IFACEQUEUE,
                           CSR_BT_CM_CRYPTO_SINGLE_BLOCK,
                           prim->flags,
                           0,
                           prim->arraySize,
                           data,
                           NULL);
    }
    else
    {
        /* Since the data is larger than a single block of 32 bytes, 
           the first 32 bytes are sent for hashing */
        SynMemMoveS(data, dataSize, prim->dataArray, dataSize);
        dm_crypto_hash_req(CSR_BT_CM_IFACEQUEUE,
                           CSR_BT_CM_CRYPTO_DATA_START,
                           prim->flags,
                           0,
                           dataSize,
                           data,
                           NULL);
    }

    CsrPmemFree(prim->dataArray);
}

static void cryptoHashContinueReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmCryptoHashReq *prim = cmData->recvMsgP;
    CsrUint16 size;
    CsrUint16 dataSize = CSR_BT_CM_CRYPTO_SHA_DATA_LEN * sizeof(CsrUint16);
    CsrUint16 data[CSR_BT_CM_CRYPTO_SHA_DATA_LEN] = {0};

    cmData->dmVar.appHandle = prim->appHandle;

    /* Ensure that the provided array is non-zero */
    if (!prim->dataArray)
    {
        CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                 prim->type,
                                 CSR_BT_RESULT_CODE_CM_CRYPTO_SHA_EMPTY_DATA_ARRAY);
        return;
    }

    /* Calculate the amount of remaining data to be hashed. */
    size = prim->arraySize - prim->currentIndex;

    /* Remaining data can be hashed as one block, and therefore this will be the last block sent. */
    if (size <= dataSize)
    {
        SynMemMoveS(data, size, &prim->dataArray[prim->currentIndex / 2], size);
        dm_crypto_hash_req(CSR_BT_CM_IFACEQUEUE,
                           CSR_BT_CM_CRYPTO_DATA_END,
                           prim->flags,
                           0,
                           size,
                           data,
                           NULL);    
    }
    else
    {
      /* Remaining data is still more than 32 bytes, so send a block of 32 bytes. */
      SynMemMoveS(data, dataSize, &prim->dataArray[prim->currentIndex / 2], dataSize);
      dm_crypto_hash_req(CSR_BT_CM_IFACEQUEUE,
                         CSR_BT_CM_CRYPTO_DATA_CONTINUE,
                         prim->flags,
                         0,
                         dataSize,
                         data,
                         NULL);     
    }

    CsrPmemFree(prim->dataArray);
}

void CsrBtCmDmCryptoHashReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmCryptoHashReq *prim = cmData->recvMsgP;

    /* Check if continuation of previous Hash request */
    if (prim->currentIndex)
    {
        cryptoHashContinueReqHandler(cmData);
    }
    else
    {
        cryptoHashReqHandler(cmData);
    }
}

void CsrBtCmDmCryptoDecryptReqHandler(cmInstanceData_t *cmData)
{
    CsrUint16 zeroDataArray[CSR_BT_CM_CRYPTO_AES_DATA_LEN] = {0};
    CsrUint16 zeroKeyArray[CSR_BT_CM_CRYPTO_AES_KEY_LEN] = {0};
    CsrBtCmCryptoDecryptReq *prim = cmData->recvMsgP;

    cmData->dmVar.appHandle = prim->appHandle;
    /* Ensure that the provided arrays are non-zero */
    if (CsrMemCmp(prim->dataArray, zeroDataArray, sizeof(prim->dataArray)) == 0)
    {
        CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                 prim->type,
                                 CSR_BT_RESULT_CODE_CM_CRYPTO_AES_EMPTY_DATA_ARRAY);
    }
    else if (CsrMemCmp(prim->keyArray, zeroKeyArray, sizeof(prim->keyArray)) == 0)
    {
        CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                 prim->type,
                                 CSR_BT_RESULT_CODE_CM_CRYPTO_AES_EMPTY_KEY_ARRAY);
    }
    else
    {
        dm_crypto_decrypt_req(CSR_BT_CM_IFACEQUEUE,
                              prim->flags,
                              0,
                              prim->dataArray,
                              prim->keyArray,
                              NULL);
    }
}

void CsrBtCmDmCryptoAesCtrReqHandler(cmInstanceData_t *cmData)
{
    CsrUint16 zeroKeyArray[CSR_BT_CM_CRYPTO_AES_KEY_LEN] = {0};
    CsrUint16 zeroNonceArray[CSR_BT_CM_CRYPTO_AES_NONCE_LEN] = {0};
    CsrBtCmCryptoAesCtrReq *prim = cmData->recvMsgP;

    cmData->dmVar.appHandle = prim->appHandle;

    /* Ensure flag value is valid */
    if (prim->flags > CSR_BT_CM_AES_CTR_BIG_ENDIAN)
    {
        CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                 prim->type,
                                 CSR_BT_RESULT_CODE_CM_CRYPTO_AES_INVALID_FLAG);
    }
    /* Ensure that the provided arrays are non-zero */
    else if (CsrMemCmp(prim->secretKey, zeroKeyArray, sizeof(prim->secretKey)) == 0)
    {
        CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                 prim->type,
                                 CSR_BT_RESULT_CODE_CM_CRYPTO_AES_EMPTY_KEY_ARRAY);
    }
    else if (CsrMemCmp(prim->nonce, zeroNonceArray, sizeof(prim->nonce)) == 0)
    {
        CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                 prim->type,
                                 CSR_BT_RESULT_CODE_CM_CRYPTO_AES_EMPTY_NONCE_ARRAY);
    }
    else if (!prim->data)
    {
        CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                 prim->type,
                                 CSR_BT_RESULT_CODE_CM_CRYPTO_AES_EMPTY_DATA_ARRAY);
    }
    /* Ensure if data length provided is within valid range */
    else if (prim->dataLen == 0 || prim->dataLen > CSR_BT_CM_CRYPTO_AES_CTR_MAX_DATA_LEN)
    {
        CsrBtCmDmCryptoErrorSend(prim->appHandle,
                                 prim->type,
                                 CSR_BT_RESULT_CODE_CM_CRYPTO_AES_INVALID_DATA_LEN);
    }
    else
    {
        dm_crypto_aes_ctr_req(CSR_BT_CM_IFACEQUEUE,
                              0,
                              prim->counter,
                              (CsrUint16)prim->flags,
                              prim->secretKey,
                              prim->nonce,
                              prim->dataLen,
                              prim->data,
                              NULL);
    }
    CsrPmemFree(prim->data);
}
