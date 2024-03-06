/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partition_reader_priv.h
    
DESCRIPTION
    Definition of partition reader data processing state datatypes.
*/

#ifndef UPGRADE_PARTITION_READER_PRIV_H_
#define UPGRADE_PARTITION_READER_PRIV_H_
#include <string.h>
#include <stdlib.h>

typedef struct {
    Source handle;
    uint16 pskey;
    uint16 read_offset;
    uint16 app_hdr_length;
    uint32 part_data_length;
    uint16 footer_length;
    UpgradePartition part_num;
    uint16 sqif_number;
    UpgradePartitionDataState partition_reader_state;
    uint32 partition_offset;
    uint8 pending_partition;
} upgrade_partition_reader_data_ctx_t;

/******************************************************************************
NAME
    upgradePartitionReader_GetHeaderID  -  Returns the Header ID
*/
const char * upgradePartitionReader_GetHeaderID(void);

/******************************************************************************
NAME
    upgradePartitionRead_GetPartitionID  -  Returns the PARTDATA ID
*/
const char * upgradePartitionRead_GetPartitionID(void);

/******************************************************************************
NAME
    upgradePartitionReader_GetFooterID  -  Returns the Footer ID
*/
const char * upgradePartitionReader_GetFooterID(void);

/******************************************************************************
NAME
    upgradePartitionReader_IsValidPartitionNum  -  Returns the TRUE when partition is valid
    DESCRIPTION
        Handles the post header state specific to CDA2_CONFIG. It will calculate
        number of partitions available

PARAMS
        partition upgradepartition to be verify
RETURNS
        Returns TRUE if partition is valid 
*/
bool upgradePartitionReader_IsValidPartitionNum(UpgradePartition partition);

/******************************************************************************
NAME
    upgradePartitionReader_GetMinSecondPartSize  -  Returns min required second header size 
    DESCRIPTION
        Handles the post header state specific to CDA2_CONFIG. It will calculate
        number of partitions available

PARAMS
        handle Handle for upgrade_partition_reader_data_ctx_t
        endCursor offset value till hashtable.
RETURNS
        A valid handle or NULL if the open failed.
*/
uint8 upgradePartitionReader_GetMinSecondPartSize(void);

/******************************************************************************
NAME
    upgradePartitionReader_GetSecondHeaderSize  -  Returns second header size 

PARAMS
        void
RETURNS
        Returns second header size
*/
uint8 upgradePartitionReader_GetSecondHeaderSize(void);

/******************************************************************************
NAME
    upgradePartitionReader_HandlePostHeaderState  -  Handles post header state in case of CDA2_CONFIG
    DESCRIPTION
        Handles the post header state specific to CDA2_CONFIG. It will calculate
        number of partitions available

PARAMS
        handle Handle for upgrade_partition_reader_data_ctx_t
        endCursor offset value till hashtable.
RETURNS
        A valid handle or NULL if the open failed.
*/
void upgradePartitionReader_HandlePostHeaderState(upgrade_partition_reader_data_ctx_t *handle, uint16 endCursor);

/******************************************************************************
NAME
    upgradePartitionReader_HandlePostDataState  -  Handles post data state in case of CDA2_CONFIG

PARAMS
        handle Handle for upgrade_partition_reader_data_ctx_t
        last_packet bool to check data it is last packet 
*/
void upgradePartitionReader_HandlePostDataState(upgrade_partition_reader_data_ctx_t *handle, bool *last_packet);

/******************************************************************************
NAME
    upgradePartitionReader_OpenSource  -  Open the partition number and returs handle for it
    DESCRIPTION
        Handles the post header state specific to CDA2_CONFIG. It will calculate
        number of partitions available

PARAMS
        handle Handle for upgrade_partition_reader_data_ctx_t
        endCursor offset value till hashtable.
RETURNS
        A valid handle or NULL if the open failed.
*/
Source upgradePartitionReader_OpenSource(UpgradePartition physPartition);

/******************************************************************************
NAME
    upgradePartitionReader_OpenSourceAtOffset  -  
    DESCRIPTION
        Handles the post header state specific to CDA2_CONFIG. It will calculate
        number of partitions available

PARAMS
        handle Handle for upgrade_partition_reader_data_ctx_t
        endCursor offset value till hashtable.
RETURNS
        A valid handle or NULL if the open failed.
*/
Source upgradePartitionReader_OpenSourceAtOffset(UpgradePartition partition, uint32 offset);

/******************************************************************************
NAME
    upgradePartitionReader_ReadNextPartitionDataChunk  -  Read the required words from source onwards 

PARAMS
        handle Handle for upgrade_partition_reader_data_ctx_t
        data_to_populate buffer to copy 
        req_len required bytes to copy 
        length_populated Number of bytes copied successfully 
RETURNS
        return TRUE if data is available

*/
bool upgradePartitionReader_ReadNextPartitionDataChunk(Source src,               uint8 *data_to_populate, uint32 req_len, uint32 * length_populated);

/******************************************************************************
NAME
    upgradePartitionReader_ReadNextPskeyDataChunk  -  Read the required words from PSKey onwards

PARAMS
        handle Handle for upgrade_partition_reader_data_ctx_t
        data_to_populate buffer to copy 
        req_len required bytes to copy 
RETURNS
        return TRUE if data is available 
*/
bool upgradePartitionReader_ReadNextPskeyDataChunk(upgrade_partition_reader_data_ctx_t * context, uint8 *data_to_populate, uint32 req_len);

/******************************************************************************
NAME
    upgradePartitionReader_IsDataAvailableInPskeys  -  Check availability of bytes 
    DESCRIPTION
        Handles the post header state specific to CDA2_CONFIG. It will calculate
        number of partitions available

PARAMS
        handle Handle for upgrade_partition_reader_data_ctx_t
        size amount of words to check for availability. 
RETURNS
        return TRUE if data is available 
*/
bool upgradePartitionReader_IsDataAvailableInPskeys(upgrade_partition_reader_data_ctx_t * context, uint32 size);

/******************************************************************************
NAME
    UpgradeFWIFBytesToUpgradePartition  - Extract upgradepartition from the data pointer

PARAMS
        data pointer to data 
RETURNS
        returns extracted upgrade partition 
*/
UpgradePartition UpgradeFWIFBytesToUpgradePartition(uint8* data);

/******************************************************************************
NAME
    UpgradeFWIFSerializePartitionID  -  Extract partition ID and return it
PARAMS
        partition First partition to be compare
RETURNS
        returns partition ID
*/
uint16 UpgradeFWIFSerializePartitionID(UpgradePartition partition);

/******************************************************************************
NAME
    UpgradeFWIFCmpUpgradePartition  -  Compare 2 partitions and returs difference between them

PARAMS
        partition1 First partition to be compare
        partition2 second partition to be compare
RETURNS
        Difference of partition1.ID and partition2.ID .
*/
short UpgradeFWIFCmpUpgradePartition(UpgradePartition partition1, UpgradePartition partition2);

/******************************************************************************
NAME
        upgradePartitionReader_GetPartitionIndex
DESCRIPTION
        Returns the index number for partition number .

RETURNS
        Returns the index number for partition number .
*/
uint8 upgradePartitionReader_GetPartitionIndex(void);
#endif

