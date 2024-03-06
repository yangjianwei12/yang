/****************************************************************************
Copyright (c) 2015 - 2021 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partition_data_priv.h
    
DESCRIPTION
    Definition of partition data processing state datatypes.
*/

#ifndef UPGRADE_PARTITION_DATA_PRIV_H_
#define UPGRADE_PARTITION_DATA_PRIV_H_

#include <ps.h>
#include "upgrade_fw_if.h"
#include "upgrade.h"
#include <rtime.h>

#define UPGRADE_PARTITION_DATA_BLOCK_SIZE(CTX) (((CTX)->partitionDataBlockSize) ? ((CTX)->partitionDataBlockSize) : UPGRADE_MAX_PARTITION_DATA_BLOCK_SIZE )

#define PREFETCH_UPGRADE_BLOCKS 3

typedef struct {
    uint8 size;
    uint8 data[UPGRADE_MAX_PARTITION_DATA_BLOCK_SIZE];
} UpgradePartitionDataIncompleteData;

typedef struct {
    rtime_t time_start;
    uint32 newReqSize;          /* size of new request */
    uint32 totalReqSize;        /* total size of all requests */
    uint32 totalReceivedSize;   /* total size of data received */
    uint32 chunkReceivedSize;   /* size of data received for current chunk */
    uint32 offset;              /* current offset */
    UpgradePartitionDataIncompleteData incompleteData;
    UpgradePartitionDataState state;
    UpgradeFWIFPartitionHdl partitionHdl;
    uint8 pendingPartition;
    uint8 totalPartitions;
    uint32 partitionLength;
    UpgradePartition partitionNum;
    uint8 *signature;
    uint16 signatureReceived;
    bool openNextPartition:1;
    bool isUpgradeHdrAvailable:1;
    /* PSKEY information to store the DFU headers */
    uint16 dfuHeaderPskey;
    uint16 dfuHeaderPskeyOffset;
    uint16 dfuHashTablePskey;
    uint16 dfuHashTablePskeyOffset;
} UpgradePartitionDataCtx;

/*!
    @brief set the state in upgrade_partition_data SM
*/
void UpgradePartitionDataSetState(UpgradePartitionDataState state);

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_ERASE
*/
bool UpgradePartitionDataHandleErase(MessageId id, Message message);

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_HASH_CHECK
*/
bool UpgradePartitionDataHandleHashCheck(MessageId id, Message message);

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_COPY
*/
bool UpgradePartitionDataHandleCopy(MessageId id, Message message);

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_WAIT_FOR_VALIDATION
*/
bool UpgradePartitionDataHandleWaitForValidation(MessageId id, Message message);

/*!
    @brief UpgradePartitionDataHandleGeneric1stPartState
    @param data Part of file conforming to upgrade file format.
    @param len  Size of data. It may be less than requested but
                it can't be more.
    @param reqComplete  Indication of whether the request is complete.

    @return The upgrade library error code
*/
UpgradeHostErrorCode UpgradePartitionDataHandleGeneric1stPartState(const uint8 *data, uint16 len, bool reqComplete);

/*!
    @brief UpgradePartitionDataHandleHeaderState
    @param data Part of file conforming to upgrade file format.
    @param len  Size of data. It may be less than requested but
                it can't be more.
    @param reqComplete  Indication of whether the request is complete.

    @return The upgrade library error code
*/
UpgradeHostErrorCode UpgradePartitionDataHandleHeaderState(const uint8 *data, uint16 len, bool reqComplete);

/*!
    @brief UpgradePartitionDataHandleDataHeaderState
    @param data Part of file conforming to upgrade file format.
    @param len  Size of data. It may be less than requested but
                it can't be more.
    @param reqComplete  Indication of whether the request is complete.

    @return The upgrade library error code
*/
UpgradeHostErrorCode UpgradePartitionDataHandleDataHeaderState(const uint8 *data, uint16 len, bool reqComplete);

/*!
    @brief UpgradePartitionDataHandleDataState
    @param data Part of file conforming to upgrade file format.
    @param len  Size of data. It may be less than requested but
                it can't be more.
    @param reqComplete  Indication of whether the request is complete.

    @return The upgrade library error code
*/
UpgradeHostErrorCode UpgradePartitionDataHandleDataState(const uint8 *data, uint16 len, bool reqComplete);

/*!
    @brief upgradeHandleFooterState
    @param data Part of file conforming to upgrade file format.
    @param len  Size of data. It may be less than requested but
                it can't be more.
    @param reqComplete  Indication of whether the request is complete.

    @return The upgrade library error code
*/
UpgradeHostErrorCode upgradeHandleFooterState(const uint8 *data, uint16 len, bool reqComplete);

/*!
    @brief UpgradePartitionDataCopyFromStream
    @param signature The signature array being copied to.
    @param offset The offset into the signature being copied to.
    @param data Part of file conforming to upgrade file format.
    @param len  Size of data. It may be less than requested but
                it can't be more.

    @return The upgrade library error code
*/
void UpgradePartitionDataCopyFromStream(uint8 *signature, uint16 offset, const uint8 *data, uint16 len);

/*!
    @brief UpgradePartitionDataGetDfuPartition

    @return Partition number containing DFU file.
*/
uint16 UpgradePartitionDataGetDfuPartition(void);

/*!
    @brief UpgradePartitionDataRequestData
    @param size Possible size of the next data request
*/
void UpgradePartitionDataRequestData(uint32 size, uint32 offset);

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_ERASE_BANK
*/
bool UpgradePartitionDataHandleEraseBank(MessageId id, Message message);

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_ERASE_IMAGE_HEADER
*/
bool UpgradePartitionDataHandleEraseImageHeader(MessageId id, Message message);

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_HASH_CHECK_DATA
*/
bool UpgradePartitionDataHandleHashCheckData(MessageId id, Message message);

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_HASH_CHECK_HEADER
*/
bool UpgradePartitionDataHandleHashCheckHeader(MessageId id, Message message);

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_SIGNATURE_CHECK
*/
UpgradeHostErrorCode UpgradePartitionDataHandleSignatureCheck(MessageId id, Message message);

uint8 UpgradePartitionDataGetSigningMode(void);

/*!
    @brief Process the initial part of DFU file header which is common across multiple formats.
    @param buff Buffer containing the header
    @param length length of buffer 
    @param newVersion Pointer to return new version from header
    @param newPSVersion Pointer to return new PS version from header
    @param endCursor Pointer to return parsed size
    @param skipCheck skip compatibility check

    @return UpgradeHostErrorCode.
*/
UpgradeHostErrorCode UpgradePartitionDataParseCommonHeader(const uint8* buff, uint16 length,
                                                           upgrade_version* newVersion, 
                                                           uint16* newPSVersion, 
                                                           uint16* endCursor, 
                                                           bool skipCheck);

#endif /* UPGRADE_PARTITION_DATA_PRIV_H_ */
