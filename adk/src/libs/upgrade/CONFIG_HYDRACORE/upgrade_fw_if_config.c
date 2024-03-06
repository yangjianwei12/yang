/****************************************************************************
Copyright (c) 2016 - 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_fw_if_config.c

DESCRIPTION
    Configuration dependent functions which (largely) interact with the firmware.

NOTES

*/

#define DEBUG_LOG_MODULE_NAME upgrade
#include <logging.h>
#include <stdlib.h>
#include <string.h>
#include <logging.h>
#include <sink.h>
#include <stream.h>
#include <panic.h>
#include <byte_utils.h>
#include <hydra_macros.h>
#include <rsa_pss_constants.h>
#include <rsa_decrypt.h>
#include <imageupgrade.h>
#include "upgrade_ctx.h"
#include "upgrade_fw_if.h"
#include "secrsa_padding.h"
#include "upgrade_partitions.h"

uint8 UpgradePartitionDataGetSigningMode(void);

/* The number of uint8s in a SHA-256 signature */
#define SHA_256_HASH_LENGTH (256/8)

/*
 * The expected signing mode types:
 * "standard" for ADK6.1 with all partitions signed;
 * "compatibility" for ADK6.0 with just the image header signed.
 */
typedef enum
{
    IMAGE_HEADER_SIGNING_MODE       = 0,
    ALL_PARTITIONS_SIGNING_MODE     = 1
} SIGNING_MODE_t;

/******************************************************************************
NAME
    UpgradeFWIFGetPhysPartitionSize

DESCRIPTION
    Find out the size of a specified partition in bytes.
    
PARAMS
    physPartition Physical partition number in external flash.
    
RETURNS
    uint32 Size of physPartition in bytes.
*/
uint32 UpgradeFWIFGetPhysPartitionSize(UpgradePartition physPartition)
{   
    /** 
     * When audio is supported, we can determine the QSPI to use from the partition.
     * Until then only QSPI zero is used.
     */
#define QSPI_NUM 0
    uint32 size;
    if(ImageUpgradeGetInfo(QSPI_NUM, physPartition.partitionID, IMAGE_SIZE, &size))
    {
        return (2 * size);
    }
    return 0;
}

/***************************************************************************
NAME
    UpgradeFWIFPartitionOpen

DESCRIPTION
    Open a write only handle to a physical partition on the external flash.
    For initial testing, the CRC check on the partition is also disabled.

PARAMS
    physPartition Physical partition number in external flash.
    firstWord First word of partition data.

RETURNS
    UpgradeFWIFPartitionHdl A valid handle or NULL if the open failed.
*/
UpgradeFWIFPartitionHdl UpgradeFWIFPartitionOpen(UpgradePartition physPartition, uint32 firstWord)
{
    Sink sink;
    /** TODO
     * When audio is supported, we can determine the QSPI to use from the partition.
     * Until then only QSPI zero is used.
     */
    uint16 QSPINum = 0;

    DEBUG_LOG_DEBUG("UpgradeFWIFPartitionOpen: opening partition %u for resume", physPartition.partitionID);
    sink = ImageUpgradeStreamGetSink(QSPINum, physPartition.partitionID, firstWord);
    DEBUG_LOG_DEBUG("UpgradeFWIFPartitionOpen: ImageUpgradeStreamGetSink(%d, %d, 0x%08lx) returns %p\n", QSPINum, physPartition.partitionID, firstWord, sink);
    if (!sink)
    {
        DEBUG_LOG_ERROR("UpgradeFWIFPartitionOpen: failed to open raw partition %u for resume", physPartition.partitionID);
        return (UpgradeFWIFPartitionHdl)NULL;
    }
    SinkConfigure(sink, VM_SINK_MESSAGES, VM_MESSAGES_NONE);

    UpgradeCtxGetFW()->partitionNum = physPartition;

    return (UpgradeFWIFPartitionHdl)sink;
}

/***************************************************************************
NAME
    UpgradeFWIFValidateStart

DESCRIPTION
    Verify the accumulated data in the validation context against
    the given signature - initial request.

PARAMS
    P0 Hash context.

RETURNS
    UpgradeHostErrorCode Status code.
*/
UpgradeHostErrorCode UpgradeFWIFValidateStart(hash_context_t *vctx)
{
    uint8 SigningMode = UpgradePartitionDataGetSigningMode();
    
    UpgradeHostErrorCode errorCode;
    
    switch (SigningMode)
    {
        case IMAGE_HEADER_SIGNING_MODE:
            if (ImageUpgradeHashSectionUpdate(vctx, IMAGE_SECTION_APPS_P0_HEADER))
            {
                DEBUG_LOG_INFO("UpgradeFWIFValidateStart, ImageUpgradeHashSectionUpdate successful");
                errorCode = UPGRADE_HOST_OEM_VALIDATION_SUCCESS;
            }
            else
            {
                DEBUG_LOG_INFO("UpgradeFWIFValidateStart, ImageUpgradeHashSectionUpdate failed");
                ImageUpgradeHashFinalise(vctx, NULL, SHA_256_HASH_LENGTH);
                errorCode = UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_FOOTER;
            }
            break;

        case ALL_PARTITIONS_SIGNING_MODE:    
            /*
             * We have a valid context, so update it with the image sections that
             * are indicated by the set bits in the partitionMap.
             */
            DEBUG_LOG_INFO("UpgradeFWIFValidateStart, ImageUpgradeHashAllSectionsUpdate");
            ImageUpgradeHashAllSectionsUpdate(vctx);
            errorCode = UPGRADE_HOST_HASHING_IN_PROGRESS;
            break;

        default:
            DEBUG_LOG_INFO("UpgradeFWIFValidateStart, unknown signing mode %u", SigningMode);
            ImageUpgradeHashFinalise(vctx, NULL, SHA_256_HASH_LENGTH);
            errorCode = UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_FOOTER;
    }
    
    return errorCode;
}

/***************************************************************************
NAME
    UpgradeFWIFValidateFinish

DESCRIPTION
    Verify the accumulated data in the validation context against
    the given signature - finish request.

PARAMS
    signature Pointer to the signature to compare against.
    The signature contains the RSA-2048 encrypted, PSS padded, SHA-256 hash.
    The RSA-2048 encrypted signature is 128 bytes.
    (RSA-1024 can be used instead of RSA-2148 by defining the UPGRADE_RSA_1024
    conditional compilation flag. The RSA-1024 encrypted signature is 64 bytes.)
    After decryption the PSS padded signature is also 128 bytes.
    After decoding, the SHA-256 signature hash is 32 bytes.
    This is to be compared against the calculated SHA-256 hash that is
    calculated from the IMAGE_SECTION_APPS_P0_HEADER in the SQIF.

RETURNS
    bool TRUE if validation is successful, FALSE otherwise.
*/
bool UpgradeFWIFValidateFinish(hash_context_t *vctx, uint8 *signature)
{
    uint16 workSignature[RSA_SIGNATURE_SIZE];
    uint16 reworkSignature[RSA_SIGNATURE_SIZE];
    uint8 sectionHash[SHA_256_HASH_LENGTH];
    uint16 sign_r2n[RSA_SIGNATURE_SIZE];
    int verify_result;

    /* sanity check to make sure the use of defines for array sizes in the function 
     * matches the original use of sizeof(). sizeof() removed in case malloc used.
     */
    COMPILE_TIME_ASSERT(sizeof(workSignature) == (sizeof(uint16) * RSA_SIGNATURE_SIZE),
            compiler_assumptions_changed_u16);
    COMPILE_TIME_ASSERT(sizeof(sectionHash) == SHA_256_HASH_LENGTH,
            compiler_assumptions_changed_u8);

    /* Get the result of the ImageUpgradeHashSectionUpdate */
    if (!ImageUpgradeHashFinalise(vctx, sectionHash, SHA_256_HASH_LENGTH))
    {
        DEBUG_LOG_ERROR("UpgradeFWIFValidateFinish, ImageUpgradeHashSectionUpdate failed");
        return FALSE;
    }

    /*
     * Copy the encrypted padded signature given into the workSignature array
     * as the rsa_decrypt will modify the workSignature array to contain the
     * output decrypted PSS padded SHA-256 signature, and we don't want to
     * trample on the original input supplied.
     */
    ByteUtilsMemCpy16((uint8 *)workSignature, 0, (const uint16 *) signature, 0,
                        RSA_SIGNATURE_SIZE * sizeof(uint16));

    /*
     * Copy the constant rsa_decrypt_constant_sign_r2n array into a writable
     * array as it will get modified in the rsa_decrypt process.
     */
    memcpy(sign_r2n, rsa_decrypt_constant_sign_r2n, 
                        RSA_SIGNATURE_SIZE * sizeof(uint16));

    /*
     * Decrypt the RSA-2048 encrypted PSS padded signature.
     * The result is the PSS padded signature returned in the workSignature
     * array that is also used fopr the input.
     */
    rsa_decrypt(workSignature, &rsa_decrypt_constant_mod, sign_r2n);

    /*
     * The ce_pkcs1_pss_padding_verify was failing on looking for 0xbc in the
     * last byte of the workSignature output by rsa_decrypt, and it was in the
     * penultimate byte, so swap the uint16 endian-ness from workSignature into
     * reworkSignature and supply that as input to ce_pkcs1_pss_padding_verify
     * instead.
     */
    ByteUtilsMemCpy16((uint8 *)reworkSignature, 0, (const uint16 *) workSignature, 0,
                    RSA_SIGNATURE_SIZE * sizeof(uint16));
    /*
     * Verify the PSS padded signature in reworkSignature against the
     * SHA-256 hash in from the image section in the sectionHash.
     */
    verify_result = ce_pkcs1_pss_padding_verify(
                                (const unsigned char *) sectionHash,
                                SHA_256_HASH_LENGTH,
                                (const unsigned char *) reworkSignature,
                                RSA_SIGNATURE_SIZE * sizeof(uint16),
                                ce_pkcs1_pss_padding_verify_constant_saltlen,
                                sizeof(rsa_decrypt_constant_mod.M) * 8);

    DEBUG_LOG_ERROR("UpgradeFWIFValidateFinish, ce_pkcs1_pss_padding_verify result %u", verify_result);
    if (verify_result != CE_SUCCESS)
    {
        DEBUG_LOG_ERROR("UpgradeFWIFValidateFinish, failed");
        return FALSE;
    }

    DEBUG_LOG_INFO("UpgradeFWIFValidateFinish, passed");
    return TRUE;
}

UpgradeFWIFApplicationValidationStatus UpgradeFWIFValidateApplication(void)
{
    return UPGRADE_FW_IF_APPLICATION_VALIDATION_SKIP;
}

/****************************************************************************
NAME
    UpgradeFWIFCommitUpgrades

DESCRIPTION
    Writes the implementation store entry of the FSTAB so that the correct
    partitions are used after a power cycle.

RETURNS
    n/a
*/
void UpgradeFWIFCommitUpgrades(void)
{
    bool result = ImageUpgradeSwapCommit();
    DEBUG_LOG_DEBUG("UpgradeFWIFCommitUpgrades: ImageUpgradeSwapCommit() = %u", result);
    UpgradePartitionsCommitUpgrade();
}

bool UpgradeFWIFHashSection(upgrade_fw_if_hash_algo_t algo,
                            UpgradePartition section,
                            uint8 *hash,
                            uint16 hash_len_bytes,
                            bool *is_result_ready)
{
    UNUSED(algo);
    UNUSED(section);
    UNUSED(hash);
    UNUSED(hash_len_bytes);
    UNUSED(is_result_ready);
    return FALSE;
}

bool UpgradeFWIFHashData(upgrade_fw_if_hash_algo_t algo,
                        const uint8 *msg,
                        uint16 msg_len_bytes,
                        uint8 *hash,
                        uint16 hash_len_bytes,
                        bool *is_result_ready)
{
    UNUSED(algo);
    UNUSED(msg);
    UNUSED(msg_len_bytes);
    UNUSED(hash);
    UNUSED(hash_len_bytes);
    UNUSED(is_result_ready);
    return FALSE;
}

bool UpgradeFWIFHashAllSections(upgrade_fw_if_hash_algo_t algo,
                                uint8 *hash,
                                uint16 hash_len_bytes,
                                bool *is_result_ready)
{
    UNUSED(algo);
    UNUSED(hash);
    UNUSED(hash_len_bytes);
    UNUSED(is_result_ready);
    return FALSE;
}

bool UpgradeFWIFHashGetPendingResult(uint8 *hash,
                                     uint16 hash_len_bytes)
{
    UNUSED(hash);
    UNUSED(hash_len_bytes);
    return FALSE;
}

bool UpgradeFWIFSignatureVerify(upgrade_fw_if_signature_algo_t algo,
                                const uint8 *hash, uint32 hash_len, 
                                const uint8 *signature, uint32 signature_len)
{
    UNUSED(algo);
    UNUSED(hash);
    UNUSED(hash_len);
    UNUSED(signature);
    UNUSED(signature_len);
    return FALSE;
}

UpgradePartition UpgradeFWIFIncreaseUpgradePartition(UpgradePartition partition)
{
    partition.partitionID += 1;
    return partition;
}

UpgradePartition UpgradeFWIFBytesToUpgradePartition(const uint8* data)
{
    UpgradePartition partition;
    partition.partitionID = ByteUtilsGet2BytesFromStream(data);
    return partition;
}

short UpgradeFWIFCmpUpgradePartition(UpgradePartition partition1, UpgradePartition partition2)
{
    return (partition1.partitionID - partition2.partitionID);
}

uint16 UpgradeFWIFSerializePartitionID(UpgradePartition partition)
{
    return partition.partitionID;
}

bool UpgradeFWIIFIsCommitStatusAvailable(void)
{
    return FALSE;
}