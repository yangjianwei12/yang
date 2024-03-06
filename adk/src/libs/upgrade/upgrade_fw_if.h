/****************************************************************************
Copyright (c) 2014 - 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_fw_if.h

DESCRIPTION
    Interface to functions which (largely) interact with the firmware.

*/
#ifndef UPGRADE_FW_IF_H_
#define UPGRADE_FW_IF_H_

#include <csrtypes.h>
#include <sink.h>
#include "imageupgrade.h"
#include <upgrade_protocol.h>
#include "upgrade_private.h"
#include "upgrade.h"

/*!
    @brief Enumeration of the types of partition data which can be handled.
*/
typedef enum
{
    /*! VM Executable Partition */
    UPGRADE_FW_IF_PARTITION_TYPE_EXE = 0x0,
    /*! DFU File Partition */
    UPGRADE_FW_IF_PARTITION_TYPE_DFU = 0x1,
    /*! PSFS Configuration Data */
    UPGRADE_FW_IF_PARTITION_TYPE_CONFIG = 0x2,
    /*! Standard Data on a read only (RO) partition */
    UPGRADE_FW_IF_PARTITION_TYPE_DATA = 0x3,
    /*! Standard Data on a raw serial (RS) partition */
    UPGRADE_FW_IF_PARTITION_TYPE_DATA_RAW_SERIAL = 0x4,

    UPGRADE_FW_IF_PARTITION_TYPE_NUM
} UpgradeFWIFPartitionType;

/*!
    @brief Enumeration of the status which can be returned from
           UpgradeFWIFValidateApplication.
*/
typedef enum
{
    UPGRADE_FW_IF_APPLICATION_VALIDATION_RUNNING,   /*!< Application partition validation in progress */
    UPGRADE_FW_IF_APPLICATION_VALIDATION_PASS,      /*!< Application partition validation passed */
    UPGRADE_FW_IF_APPLICATION_VALIDATION_SKIP       /*!< Application partition validation not required */
} UpgradeFWIFApplicationValidationStatus;

/*! 
    @brief Supported algorithms to calculate hash.
*/
typedef enum
{
    /*!< SHA256 algorithm. */
    UPGRADE_FW_IF_SHA256,
    UPGRADE_FW_IF_SHA384,
}upgrade_fw_if_hash_algo_t;

typedef enum
{
    /*!< ECDSA P384 algorithm. */
    UPGRADE_FW_IF_ECC384,
}upgrade_fw_if_signature_algo_t;

typedef enum
{
    /*!< Defines the image-header on alternate bank. */
    UPGRADE_FW_IF_FILTER_ALTERNATE_IMAGE_HEADER,
    /*!< Defines all sections other than image-header on alternate bank. */
    UPGRADE_FW_IF_FILTER_ALTERNATE_BANK_SECTIONS,
    /*!< Defines all sections on alternate bank. */
    UPGRADE_FW_IF_FILTER_ALTERNATE_ALL_SECTIONS,
}upgrade_fw_if_section_filter_t;
    
/*! @brief Return type for ImageUpgradeSwapCommit trap */
typedef enum
{
    /*!< Status types. */
    IMAGE_UPGRADE_COMMIT_FAILED                              = 0,
    IMAGE_UPGRADE_COMMIT_SUCCESS_SECURITY_UPDATE_FAILED      = 1,
    IMAGE_UPGRADE_COMMIT_AND_SECURITY_UPDATE_SUCCESS         = 2
}upgrade_fw_if_commit_status; 

/*!
    @brief An opaque handle to an writeable external flash partition.
*/
typedef Sink UpgradeFWIFPartitionHdl;

/*!
    @brief Initialise the context for the Upgrade FW IF.
*/
void UpgradeFWIFInit(void);

/*!
    @brief Get the identifier for the current device variant.

    @return String containing the device variant.
*/
const char *UpgradeFWIFGetDeviceVariant(void);

/*!
    @brief Get the current (running) app version.

    @todo This shouldn't be implemented in upgrade library, and is now little used.

    @return The running app version.
*/
uint16 UpgradeFWIFGetAppVersion(void);

/*!
    @brief Find the physical partition corresponding to a logical partition in a bank.

    @param logicPartition Logical partition number, from the upgrade file partition section header.

    @return uint16 Identifier for the physical partition corresponding to the logical 
                partition. Note that the ID is the full ID from FSTAB, including the 
                flash device ID. Use UPGRADE_PARTITION_PHYSICAL_PARTITION() to get the
                specific ID.
*/
UpgradePartition UpgradeFWIFGetPhysPartition(UpgradePartition logicPartition);

/*!
    @brief Find the number of physical partitions on the serial flash.

    @return uint16 Number of partitions.
*/
uint16 UpgradeFWIFGetPhysPartitionNum(void);

/*!
    @brief Find the size of a physical partition in bytes.

    @param physPartition Number of the physical partition for which the size will be returned.

    @return uint32 Size of partition physPartition in bytes.
*/
uint32 UpgradeFWIFGetPhysPartitionSize(UpgradePartition physPartition);

/*!
    @brief Open a physical partition in external flash for writing.

    @param physPartition Index of the physical partition to open.
    @param firstWord First word of partition data.

    @return Valid UpgradeFWIFPartitionHdl if ok, zero otherwise.
*/
UpgradeFWIFPartitionHdl UpgradeFWIFPartitionOpen(UpgradePartition physPartition, uint32 firstWord);

/*!
    @brief Write data to an open external flash partition.
           Each byte of the data is copied to the partition
           in a byte by byte copy operation.

    @param handle Handle to a writeable partition.
    @param data Pointer to the data buffer to write.
    @param len Number of bytes (not words) to write.

    @return The number of bytes written, or 0 if there was an error.
*/
uint16 UpgradeFWIFPartitionWrite(UpgradeFWIFPartitionHdl handle, const uint8 *data, uint16 len);

/*!
    @brief Close a handle to an external flash partition.

    @param handle Handle to a writeable partition.

    @return UPGRADE_HOST_SUCCESS if close successful, 
            UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED_PS_SPACE if PS space is critical
            UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED in (unlikely) error scenario
            
*/
UpgradeHostErrorCode UpgradeFWIFPartitionClose(UpgradeFWIFPartitionHdl handle);

/*!
    @brief Start verify the accumulated data in the validation context against
           the given signature. The signature is a sequence of 128 bytes
           packed into 64 16-bit words.

    @param vctx P0 Hash context.

    @return Status code.
*/
UpgradeHostErrorCode UpgradeFWIFValidateStart(hash_context_t *vctx);

/*!
    @brief Finish verify the accumulated data in the validation context against
           the given signature. The signature is a sequence of 128 bytes
           packed into 64 16-bit words.

    @param vctx P0 Hash context.
    @param signature Signature sequence.

    @return TRUE if a validation was successful, FALSE otherwise.
*/
bool UpgradeFWIFValidateFinish(hash_context_t *vctx, uint8 *signature);

/*!
*/
UpgradeFWIFApplicationValidationStatus UpgradeFWIFValidateApplication(void);


/* Need to add a function to do a warm reboot?
    It will need to be told to do a zarkov or DFU reboot */

#ifdef MAYBE_USEFUL_LATER
/*!
    @brief UpgradeFWIFIsPartitionCompleted()

    @param physPartition Physical partition number to be checked.

    @return TRUE if partition is not completed (doesn't have first word written).
*/
bool UpgradeFWIFIsPartitionCompleted(UpgradePartition physPartition);
#endif

/*!
    @brief UpgradeFWIFPartitionGetOffset()

    @param handle Handle to the partition offset to be taken from.

    @return Size in bytes of offset to skip already written data.
*/
uint32 UpgradeFWIFPartitionGetOffset(UpgradeFWIFPartitionHdl handle);

/*!
    @brief UpgradeFWIFGetSinkPosition()
Get the sink position of the partition sink stream.

    @param sink sink stream.

    @return the position of the sink stream.
*/
uint32 UpgradeFWIFGetSinkPosition(Sink sink);

#ifdef MAYBE_USEFUL_LATER
/*!
    @brief UpgradeFWIFGetPartitionIsInProgress()

    @return TRUE if there is partition which should be open for resume.
*/
bool UpgradeFWIFGetPartitionIsInProgress(void);

/*!
    @brief UpgradeFWIFClearPartitionIsInProgress()

    Makes UpgradeFWIFGetPartitionIsInProgress() return FALSE .
*/
void UpgradeFWIFClearPartitionIsInProgress(void);

/*!
    @brief UpgradeFWIFSetPartitionInProgressNum()

    @return Number of a physical partition to be resumed.
*/
uint16 UpgradeFWIFSetPartitionInProgressNum(void);

/*!
    @brief UpgradeFWIFGetPartitionInProgressNum()

    @return Number of a physical partition to be resumed.
*/
uint16 UpgradeFWIFGetPartitionInProgressNum(void);
#endif

/*!
    @brief Calculate hash of a section.

    @param algo hash algorithm.
    @param section Id of image-section to calculate hash
    @param hash buffer to store calculated hash
    @parma hash_len Length of the hash buffer
    @param is_result_ready will be filled with TRUE if the calculation was done synchronously
           otherwise a system message will be sent when results are ready.

    @return FALSE if error occurs else TRUE.
*/
bool UpgradeFWIFHashSection(upgrade_fw_if_hash_algo_t algo,
                            UpgradePartition section,
                            uint8 *hash,
                            uint16 hash_len,
                            bool *is_result_ready);

/*!
    @brief Calculate hash of a given buffer.

    @param algo hash algorithm.
    @param data pointer to buffer on which hash will be calculated
    @parma data_len length of msg buffer
    @param hash buffer to store calculated hash
    @parma hash_len Length of the hash buffer
    @param is_result_ready will be filled with TRUE if the calculation was done synchronously
           otherwise a system message will be sent when results are ready.

    @return FALSE if error occurs else TRUE.

*/
bool UpgradeFWIFHashData(upgrade_fw_if_hash_algo_t algo,
                        const uint8 *data,
                        uint16 data_len,
                        uint8 *hash,
                        uint16 hash_len,
                        bool *is_result_ready);

/*!
    @brief Calculate hash of all available sections.

    @param algo hash algorithm.
    @param hash buffer to store calculated hash
    @parma hash_len Length of the hash buffer
    @param is_result_ready will be filled with TRUE if the calculation was done synchronously
           otherwise a system message will be sent when results are ready.

    @return FALSE if error occurs else TRUE.

*/
bool UpgradeFWIFHashAllSections(upgrade_fw_if_hash_algo_t algo,
                                uint8 *hash,
                                uint16 hash_len,
                                bool *is_result_ready);

/*!
    @brief Return the hash of last asynchronous request if ready.

    @param hash buffer to store calculated hash
    @parma hash_len Length of the hash buffer

    @return TRUE if hash was available and was filled in hash buffer.
*/
bool UpgradeFWIFHashGetPendingResult(uint8 *hash,
                                     uint16 hash_len);

/*!
    @brief This function verifies the given signature against hash

    @param algo Signature verify algorithm.
    @param hash Message pointer to the calculated hash
    @parma hash_len Length of the hash
    @param signature Message pointer to the signature
    @parma signature_len Length of the signature
    @return TRUE if the signature verification is successful, else returns FALSE
*/
bool UpgradeFWIFSignatureVerify(upgrade_fw_if_signature_algo_t algo,
                                const uint8 *hash, uint32 hash_len, 
                                const uint8 *signature, uint32 signature_len);

/*!
    @brief This function erases the selected sections on the other bank.

    @param filter to select a set of image-sections.
    @return Current state of partitions.
*/
UpgradePartitionsState UpgradeFWIFEraseSection(upgrade_fw_if_section_filter_t filter);

/*!
    @brief Check if the given section is an image-header or not.

    @param section Id of an image-section 
    @return TRUE if an image-header.
*/
bool UpgradeFWIFIsImageHeader(UpgradePartition section);

/*!
    @brief UpgradeFWIFCommitUpgrades

    Updates the Persistent Storage so that the current partitions are
    'permanent'. In the event of a storage error a panic will be raised
    leading to a reboot. 

    @note That this could become a vicious circle
    
*/
void UpgradeFWIFCommitUpgrades(void);

/*!
    @brief Increase the UpgradePartition by 1.

    @param partition partition to increase 
    @return Updated UpgradePartition.

    @note Instance ID will also be increased if it supports.
*/
UpgradePartition UpgradeFWIFIncreaseUpgradePartition(UpgradePartition partition);

/*!
    @brief Convert 2 bytes from stream to UpgradePartition type.

    @param data partition IDs in bytes.
    @return partition in UpgradePartition type.
*/
UpgradePartition UpgradeFWIFBytesToUpgradePartition(const uint8* data);

/*!
    @brief Compare the IDs of two partition.

    @param partition1 first partition to compare.
    @param partition2 second partition to compare.
    @return less than 0 if partition1 is lesser, 0 if both are same otherwise greater than 0.
*/
short UpgradeFWIFCmpUpgradePartition(UpgradePartition partition1, UpgradePartition partition2);

uint16 UpgradeFWIFSerializePartitionID(UpgradePartition partition);

bool UpgradeFWIIFIsCommitStatusAvailable(void);

#endif /* UPGRADE_FW_IF_H_ */
