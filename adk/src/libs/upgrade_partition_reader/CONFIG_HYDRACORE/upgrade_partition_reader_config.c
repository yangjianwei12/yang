/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partition_reader_config.c

DESCRIPTION
    This file handles the partition read request state machine and reads data of requested
    length.

*/
#define DEBUG_LOG_MODULE_NAME upgrade_partition_reader
#include <logging.h>

#include <stream.h>
#include <source.h>
#include <imageupgrade.h>
#include "upgrade_partition_reader.h"
#include "upgrade.h"
#include "upgrade_partition_reader_private.h"

#define UPGRADE_HEADER_MIN_SECOND_PART_SIZE 22
#define PARTITION_SECOND_HEADER_SIZE (4) 
#define UPGRADE_PARTITION_INDEX     (2)

#define UPGRADE_IMAGE_SECTION_DFU_HEADERS                  (0xd)

/******************************************************************************
NAME
        upgradePartitionReader_OpenSource
DESCRIPTION
        Open a source handle to a physical partition on the external flash.

PARAMS
        physPartition Physical partition number in external flash.
RETURNS
        A valid handle or NULL if the open failed.
*/
Source upgradePartitionReader_OpenSource(UpgradePartition physPartition)
{
    uint16 QSPINum = 0;
    Source src = NULL;

    DEBUG_LOG("upgradePartitionReader_OpenSource opening partition %u", physPartition.partitionID);

    src = ImageUpgradeStreamGetSource(QSPINum, physPartition.partitionID);

    if (src == NULL)
    {
        DEBUG_LOG_WARN("upgradePartitionReader_OpenSource open failed %u", physPartition.partitionID);
        return (Source)NULL;
    }

    return src;
}

/******************************************************************************
NAME
        upgradePartitionReader_OpenSourceAtOffset
DESCRIPTION
        Open a source handle to a physical partition on the external flash from the provided offset.

PARAMS
        physPartition Physical partition number in external flash.
        offset Offset in bytes from start of partition, where the next read will happen.
RETURNS
        A valid handle or NULL if the open failed.
*/
Source upgradePartitionReader_OpenSourceAtOffset(UpgradePartition partition, uint32 offset)
{
    uint16 QSPI = 0;
    Source src = NULL;

    DEBUG_LOG("upgradePartitionReader_OpenSourceAtOffset opening partition %u at offset %lu", partition.partitionID, offset);
    src = ImageUpgradeStreamGetSourceFromOffset(QSPI, partition.partitionID, offset);

    if (src == NULL)
    {
        DEBUG_LOG_WARN("upgradePartitionReader_OpenSourceAtOffset open failed %u",partition.partitionID);
        return (Source)NULL;
    }
    return src;
}

/******************************************************************************
NAME
        upgradePartitionReader_GetPartitionIndex
DESCRIPTION
        Returns the index number for partition number .
*/
uint8 upgradePartitionReader_GetPartitionIndex(void)
{
    return UPGRADE_PARTITION_INDEX;
}

/******************************************************************************
NAME
        upgradePartitionReader_HandlePostDataState
DESCRIPTION
        This function is specific to CDA2_CONFIG 
*/
void upgradePartitionReader_HandlePostDataState(upgrade_partition_reader_data_ctx_t *handle, bool *last_packet)
{
    UNUSED(handle);
    UNUSED(last_packet);
}

/******************************************************************************
NAME
    upgradePartitionReader_IsValidPartitionNum  -  Identify the partition number 
            is valid partition
DESCRIPTION
    Identify the partition number is valid number
    Returns TRUE if partition valid else returns FALSE
*/
bool upgradePartitionReader_IsValidPartitionNum(UpgradePartition partition)
{
    switch (partition.partitionID)
    {
    case IMAGE_SECTION_NONCE:
    case IMAGE_SECTION_APPS_P0_HEADER:
    case IMAGE_SECTION_APPS_P1_HEADER:
    case IMAGE_SECTION_AUDIO_HEADER:
    case IMAGE_SECTION_CURATOR_FILESYSTEM:
    case IMAGE_SECTION_APPS_P0_IMAGE:
    case IMAGE_SECTION_APPS_RO_CONFIG_FILESYSTEM:
    case IMAGE_SECTION_APPS_RO_FILESYSTEM:
    case IMAGE_SECTION_APPS_P1_IMAGE:
    case IMAGE_SECTION_APPS_DEVICE_RO_FILESYSTEM:
    case IMAGE_SECTION_AUDIO_IMAGE:
    case IMAGE_SECTION_APPS_RW_CONFIG:
        return TRUE;

    case IMAGE_SECTION_APPS_RW_FILESYSTEM:
    case UPGRADE_IMAGE_SECTION_DFU_HEADERS:
    default:
        return FALSE;
    }
}

/******************************************************************************
NAME
        upgradePartitionReader_GetHeaderID
DESCRIPTION
        Returns header ID

*/
const char * upgradePartitionReader_GetHeaderID(void)
{
    return "APPUHDR5";
}

/******************************************************************************
NAME
        upgradePartitionRead_GetPartitionID
DESCRIPTION
        Returns partition ID

*/
const char * upgradePartitionRead_GetPartitionID(void)
{
    return "PARTDATA";
}

/******************************************************************************
NAME
        upgradePartitionReader_GetFooterID
DESCRIPTION
        Returns Footer  ID
*/
const char * upgradePartitionReader_GetFooterID(void)
{
    return "APPUPFTR";
}

/******************************************************************************
NAME
        upgradePartitionReader_GetMinSecondPartSize
DESCRIPTION
        Returns minimin required second part size
*/
uint8 upgradePartitionReader_GetMinSecondPartSize(void)
{
    return UPGRADE_HEADER_MIN_SECOND_PART_SIZE;
}


/******************************************************************************
NAME
        upgradePartitionReader_GetSecondHeaderSize
DESCRIPTION
        Returns size of the second header 
*/
uint8 upgradePartitionReader_GetSecondHeaderSize(void)
{
    return PARTITION_SECOND_HEADER_SIZE;
}

