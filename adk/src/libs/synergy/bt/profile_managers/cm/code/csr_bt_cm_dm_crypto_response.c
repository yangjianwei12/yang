/******************************************************************************
 Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
 ******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

static CsrBtCmCryptoEccType CsrBtCmCryptoConvertEccType(uint8_t keyType)
{
    CsrBtCmCryptoEccType cryptoKeyType = CSR_BT_CM_CRYPTO_ECC_UNKNOWN_TYPE;
    switch(keyType)
    {
        case DM_CRYPTO_ECC_P192:
            cryptoKeyType = CSR_BT_CM_CRYPTO_ECC_P192;
            break;
        case DM_CRYPTO_ECC_P256:
            cryptoKeyType = CSR_BT_CM_CRYPTO_ECC_P256;
            break;

        default:
            break;
    }
    return cryptoKeyType;
}

static CsrBtCmCryptoHashOperation CsrBtCmCryptoConvertCryptoHashOperation(uint8_t operation)
{
    CsrBtCmCryptoHashOperation cryptoHashOperation = CSR_BT_CM_CRYPTO_UNKNOWN_OPERATION;
    switch(operation)
    {
        case DM_CRYPTO_SINGLE_BLOCK:
            cryptoHashOperation = CSR_BT_CM_CRYPTO_SINGLE_BLOCK;
            break;
        case DM_CRYPTO_DATA_START:
            cryptoHashOperation = CSR_BT_CM_CRYPTO_DATA_START;
            break;
        case DM_CRYPTO_DATA_CONTI:
            cryptoHashOperation = CSR_BT_CM_CRYPTO_DATA_CONTINUE;
            break;
        case DM_CRYPTO_DATA_END:
            cryptoHashOperation = CSR_BT_CM_CRYPTO_DATA_END;
            break;

        default:
            break;
    }
    return cryptoHashOperation;
}

void CsrBtCmDmCryptoGeneratePublicPrivateKeyCfmHandler(cmInstanceData_t *cmData)
{
    DM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_CFM_T *cfm = (DM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_CFM_T *) cmData->recvMsgP;
    CsrBtCmCryptoGeneratePublicPrivateKeyCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_BT_CM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_CFM;

    if (cfm->status == DM_CRYPTO_SUCCESS)
    {
        /* Key pair generation successful. */
        SynMemMoveS(prim->publicKey,
                    CSR_BT_CM_CRYPTO_PUBLIC_KEY_LEN * sizeof(CsrUint16),
                    cfm->public_key,
                    CSR_BT_CM_CRYPTO_PUBLIC_KEY_LEN * sizeof(CsrUint16));
        SynMemMoveS(prim->privateKey,
                    CSR_BT_CM_CRYPTO_PRIVATE_KEY_LEN * sizeof(CsrUint16),
                    cfm->private_key,
                    CSR_BT_CM_CRYPTO_PRIVATE_KEY_LEN * sizeof(CsrUint16));
        prim->keyType = CsrBtCmCryptoConvertEccType(cfm->key_type);
        prim->resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
        prim->resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        prim->resultCode = (CsrBtResultCode)cfm->status;
        prim->resultSupplier = CSR_BT_SUPPLIER_DM;
    }

    CsrBtCmPutMessage(cmData->dmVar.appHandle, prim);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmCryptoGenerateSharedSecretKeyCfmHandler(cmInstanceData_t *cmData)
{
    DM_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM_T *cfm = (DM_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM_T *) cmData->recvMsgP;
    CsrBtCmCryptoGenerateSharedSecretKeyCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_BT_CM_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM;

    if (cfm->status == DM_CRYPTO_SUCCESS)
    {
        /* Shared secret key generation successful. */
        SynMemMoveS(prim->sharedSecretKey,
                    CSR_BT_CM_CRYPTO_SECRET_KEY_LEN * sizeof(CsrUint16),
                    cfm->shared_secret_key,
                    CSR_BT_CM_CRYPTO_SECRET_KEY_LEN * sizeof(CsrUint16));
        prim->keyType = CsrBtCmCryptoConvertEccType(cfm->key_type);
        prim->resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
        prim->resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        prim->resultCode = (CsrBtResultCode)cfm->status;
        prim->resultSupplier = CSR_BT_SUPPLIER_DM;
    }

    CsrBtCmPutMessage(cmData->dmVar.appHandle, prim);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmCryptoEncryptCfmHandler(cmInstanceData_t *cmData)
{
    DM_CRYPTO_ENCRYPT_CFM_T *cfm = (DM_CRYPTO_ENCRYPT_CFM_T *) cmData->recvMsgP;
    CsrBtCmCryptoEncryptCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_BT_CM_CRYPTO_ENCRYPT_CFM;
    prim->flags = cfm->flags;

    if (cfm->status == DM_CRYPTO_SUCCESS)
    {
        /* AES Encryption successful. */
        SynMemMoveS(prim->encryptedData,
                    CSR_BT_CM_CRYPTO_AES_DATA_LEN * sizeof(CsrUint16),
                    cfm->encrypted_data,
                    CSR_BT_CM_CRYPTO_AES_DATA_LEN * sizeof(CsrUint16));
        prim->resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
        prim->resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        prim->resultCode = (CsrBtResultCode)cfm->status;
        prim->resultSupplier = CSR_BT_SUPPLIER_DM;
    }

    CsrBtCmPutMessage(cmData->dmVar.appHandle, prim);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmCryptoHashCfmHandler(cmInstanceData_t *cmData)
{
    DM_CRYPTO_HASH_CFM_T *cfm = (DM_CRYPTO_HASH_CFM_T *) cmData->recvMsgP;
    CsrBtCmCryptoHashCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_BT_CM_CRYPTO_HASH_CFM;
    prim->operation = CsrBtCmCryptoConvertCryptoHashOperation(cfm->operation);
    prim->flags = cfm->flags;

    if (cfm->status == DM_CRYPTO_SUCCESS)
    {
        if ((prim->operation == CSR_BT_CM_CRYPTO_SINGLE_BLOCK) ||
            (prim->operation == CSR_BT_CM_CRYPTO_DATA_END))
        {
            SynMemMoveS(prim->hash,
                        CSR_BT_CM_CRYPTO_SHA_HASH_LEN * sizeof(CsrUint16),
                        cfm->hash,
                        CSR_BT_CM_CRYPTO_SHA_HASH_LEN * sizeof(CsrUint16));
            prim->resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
            prim->resultSupplier = CSR_BT_SUPPLIER_CM;
        }
    }
    else
    {
        prim->resultCode = (CsrBtResultCode)cfm->status;
        prim->resultSupplier = CSR_BT_SUPPLIER_DM;
    }

    CsrBtCmPutMessage(cmData->dmVar.appHandle, prim);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmCryptoDecryptCfmHandler(cmInstanceData_t *cmData)
{
    DM_CRYPTO_DECRYPT_CFM_T *cfm = (DM_CRYPTO_DECRYPT_CFM_T *) cmData->recvMsgP;
    CsrBtCmCryptoDecryptCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_BT_CM_CRYPTO_DECRYPT_CFM;
    prim->flags = cfm->flags;

    if (cfm->status == DM_CRYPTO_SUCCESS)
    {
        /* AES Decryption successful. */
        SynMemMoveS(prim->decryptedData,
                    CSR_BT_CM_CRYPTO_AES_DATA_LEN * sizeof(CsrUint16),
                    cfm->decrypted_data,
                    CSR_BT_CM_CRYPTO_AES_DATA_LEN * sizeof(CsrUint16));
        prim->resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
        prim->resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        prim->resultCode = (CsrBtResultCode)cfm->status;
        prim->resultSupplier = CSR_BT_SUPPLIER_DM;
    }

    CsrBtCmPutMessage(cmData->dmVar.appHandle, prim);
    CsrBtCmDmLocalQueueHandler();
}

void CsrBtCmDmCryptoAesCtrCfmHandler(cmInstanceData_t *cmData)
{
    DM_CRYPTO_AES_CTR_CFM_T *cfm = (DM_CRYPTO_AES_CTR_CFM_T *) cmData->recvMsgP;
    CsrUint16 dataSize = sizeof(CsrUint16) * cfm->data_len;
    CsrBtCmCryptoAesCtrCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_BT_CM_CRYPTO_AES_CTR_CFM;
    prim->flags = cfm->flags;
    prim->dataLen = cfm->data_len;

    if (cfm->status == DM_CRYPTO_SUCCESS)
    {
        prim->data = CsrPmemAlloc(dataSize);
        SynMemMoveS(prim->data, dataSize, cfm->data, dataSize);
        CsrPmemFree(cfm->data);
        prim->resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
        prim->resultSupplier = CSR_BT_SUPPLIER_CM;
    }
    else
    {
        prim->data = NULL;
        prim->resultCode = (CsrBtResultCode)cfm->status;
        prim->resultSupplier = CSR_BT_SUPPLIER_DM;
    }

    CsrBtCmPutMessage(cmData->dmVar.appHandle, prim);
    CsrBtCmDmLocalQueueHandler();
}
