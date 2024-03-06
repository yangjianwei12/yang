/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partition_reader_config.c

DESCRIPTION
    This file handles the partition read request state machine and reads data of requested
    length specific to CDA2.

*/
#define DEBUG_LOG_MODULE_NAME upgrade_partition_reader
#include <logging.h>
#include <stream.h>
#include <source.h>
#include <imageupgrade.h>
#include "upgrade_partition_reader.h"
#include "upgrade.h"
#include "upgrade_partition_reader_private.h"

#define UPGRADE_HEADER_MIN_SECOND_PART_SIZE (166)
#define PARTITION_SECOND_HEADER_SIZE ( 50 ) 
#define UPGRADE_PARTITION_INDEX     (0)

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

    DEBUG_LOG("upgradePartitionReader_OpenSource opening partition %u", physPartition.partitionInstance.partitionID);

    src = ImageUpgradeStreamGetSource(QSPINum, physPartition.partitionInstance.partitionID);

    if (src == NULL)
    {
        DEBUG_LOG_WARN("upgradePartitionReader_OpenSource open failed %u", physPartition.partitionInstance.partitionID);
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

    DEBUG_LOG("upgradePartitionReader_OpenSourceAtOffset opening partition %u at offset %lu", partition.partitionInstance.partitionID, offset);
    src = ImageUpgradeStreamGetSourceFromOffset(QSPI, partition.partitionInstance.partitionID, offset);

    if (src == NULL)
    {
        DEBUG_LOG_WARN("upgradePartitionReader_OpenSourceAtOffset open failed %u",partition.partitionInstance.partitionID);
        return (Source)NULL;
    }
    return src;
}

/******************************************************************************
NAME
        upgradePartitionReader_GetPartitionIndex
DESCRIPTION
        Returns the index number for partition number .

RETURNS
        Returns the index number for partition number .
*/
uint8 upgradePartitionReader_GetPartitionIndex(void)
{
    return UPGRADE_PARTITION_INDEX;
}

/******************************************************************************
NAME
        upgradePartitionReader_GetFooterID
DESCRIPTION
        Returns NULL in case of CDA2_CONFIG
*/
const char * upgradePartitionReader_GetFooterID(void)
{
    return NULL;
}

/******************************************************************************
NAME
    upgradePartitionReader_GetHeaderID  -  Returns the Header ID
*/
const char * upgradePartitionReader_GetHeaderID(void)
{
    return "APPUHDR6";
}

/******************************************************************************
NAME
    upgradePartitionRead_GetPartitionID  -  Returns the PARTDATA ID
*/
const char * upgradePartitionRead_GetPartitionID(void)
{
    return "PARTDATA";
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
    upgradePartitionReader_IsValidPartitionNum  -  Identify the partition number 
            is valid partition
DESCRIPTION
    Identify the partition number is valid number
    Returns TRUE if partition valid else returns FALSE
*/
bool upgradePartitionReader_IsValidPartitionNum(UpgradePartition partition)
{
/* TODO_CDA2 : Need to implement */
    switch (partition.partitionID)
    {
    default:
        return TRUE;
    }
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

/******************************************************************************
NAME
        upgradePartitionReader_HandlePostDataState
DESCRIPTION
        Handles the post data state specific to CDA2_CONFIG. It will check remaining 
        partitions and update the last packet. 

PARAMS
        handle Handle for upgrade_partition_reader_data_ctx_t
        last_packet update on last packet. 
*/
void upgradePartitionReader_HandlePostDataState(upgrade_partition_reader_data_ctx_t *handle, bool *last_packet)
{
    PanicNull(handle);
    upgrade_partition_reader_data_ctx_t * context = handle;
    context->pending_partition -= 1;

    if(context->pending_partition == 0 && context->part_data_length == 0)
    {
        *last_packet = TRUE;
    }
}

