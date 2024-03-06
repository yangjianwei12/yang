/****************************************************************************
Copyright (c) 2004 - 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partition_data.h

DESCRIPTION
    Upgrade file processing module.
    It parses headers, write partition data to the SQIF.
    As well as verifies OEM signature.
*/
#ifndef UPGRADE_PARTITION_DATA_H_
#define UPGRADE_PARTITION_DATA_H_

#include <csrtypes.h>

#include <upgrade_protocol.h>
#include "upgrade_fw_if.h"
#include "upgrade.h"

#define HEADER_FIRST_PART_SIZE 12
#define ID_FIELD_SIZE 8
#define UPGRADE_HEADER_MIN_SECOND_PART_SIZE 14
#define UPGRADE_VERSION_SIZE 4
#define UPGRADE_VERSION_PART_SIZE 2
#define UPGRADE_NO_OF_COMPATIBLE_UPGRADES_SIZE 2
#define UPGRADE_PS_VERSION_SIZE 2
#define UPGRADE_NO_OF_COMPATIBLE_PS_VERSION_SIZE 2
#define UPGRADE_HEADER_EXTRA_INFO 2

#define UPGRADE_HEADER_VARIANT_SIZE 8
#define PARTITION_LEN_SIZE 4
#define PARTITION_TYPE_SIZE 2
#define PARTITION_NUM_SIZE 2
#define PARTITION_FIRST_WORD_SIZE 4

/* Identifier for the header of an upgrade file containing the firmware image of non-QC charger-case. */
#define EXT_CASE_DFU_FILE_HEADER "EXTUHDR1"

extern uint8 first_word_size;

/*!
    @brief State of data transfer after last segment received
*/
typedef enum {
    UPGRADE_PARTITION_DATA_XFER_ERROR,
    UPGRADE_PARTITION_DATA_XFER_COMPLETE,
    UPGRADE_PARTITION_DATA_XFER_IN_PROGRESS
    } UpgradePartitionDataPartialTransferState;

/*!
    @brief Internal messages used in upgrade_partition_data FSM
*/
typedef enum {
    UPGRADE_PARTITION_DATA_INTERNAL_START_VALIDATION,
 } upgrade_partition_data_internal_message_t;

/*!
    @brief Check if the first header PSKEY contains the header of case DFU file.
*/
bool UpgradeIsCaseDfuHeaderInPSKeys(void);

/*!
    @brief Initialisation of PartitionData module.
    @return TRUE if ok, FALSE if out of memory.
*/
bool UpgradePartitionDataInit(void);

/*!
    @brief Free memory allocated for partition data.
*/
void UpgradePartitionDataDestroy(void);

/*!
    @brief UpgradePartitionDataGetNextReqSize

    @return Size of data request for a host.
            It will never be bigger than blockSize.
*/
uint32 UpgradePartitionDataGetNextReqSize(void);

/*!
    @brief UpgradePartitionDataGetNextOffset

    @return Offset, from current position in a file, from which data should be retrieved.
*/
uint32 UpgradePartitionDataGetNextOffset(void);

/*!
    @brief UpgradePartitionDataParseIncomplete

    @param data Part of file conforming to upgrade file format.
    @param len  Size of data, less than requested due to transport packet size
                limits.

    @return The upgrade library error code
*/
UpgradeHostErrorCode UpgradePartitionDataParseIncomplete(const uint8 *data, uint16 data_len);

/*!
    @brief UpgradePartitionDataParse

    @param data Part of file conforming to upgrade file format.
    @param len  Size of data. It may be less than requested but
                it can't be more.

    @return The upgrade library error code
*/
UpgradeHostErrorCode UpgradePartitionDataParse(const uint8 *data, uint16 len);

/*!
    @brief Notify the data parser to stop future data handling.

    Causes to stop handling incoming 
    data and report errors. The error condition should be reset on a new 
    transfer/data request initiated by the library.
*/
void UpgradePartitionDataStopData(void);

/*!
    @brief UpgradePartitionDataIsDfuUpdate

    @return TRUE if one of a partitions contains a DFU file.
*/
bool UpgradePartitionDataIsDfuUpdate(void);


/*!
    @brief Message handler for upgrade_partition_data SM.

    @param id message id.
    @param message message data.

    @return is the message handled or not.
*/
bool UpgradePartitionDataHandleMessage(MessageId id, Message message);

/*!
    @brief Check if data hash checking is going on.

    @return TRUE if upgrade_partition_data FSM is in HASH_CHECK state.
*/
bool UpgradePartitionDataStateIsHashCheck(void);

/*! @brief Get the state of Upgrade Partition Data
    @param None
    @return State
*/
UpgradePartitionDataState UpgradePartitionDataGetState(void);

/*!
    @brief Get the identifier for the header of an upgrade file.

    @return String containing the header ID.
*/
const char *UpgradePartitionDataGetHeaderID(void);

/*!
    @brief Get the identifier for a partition header within an upgrade file.

    @return String containing the partition header ID.
*/
const char *UpgradePartitionDataGetPartitionID(void);

/*!
    @brief Get the identifier for the footer of an upgrade file.

    @return String containing the footer ID.
*/
const char *UpgradePartitionDataGetFooterID(void);

#endif /* UPGRADE_PARTITION_DATA_H_ */
