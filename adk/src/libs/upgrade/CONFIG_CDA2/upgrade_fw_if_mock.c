/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_fw_if_mock.c

DESCRIPTION
    Mock interface acting as CDA2 firmware.

NOTES

*/

#define DEBUG_LOG_MODULE_NAME upgrade
#include <imageupgrade.h>
#include <logging.h>
#include <stdlib.h>
#include <string.h>
#include "upgrade_ctx.h"
#include "upgrade_fw_if.h"
#include "upgrade.h"
#include <byte_utils.h>
#include "upgrade_partitions.h"

typedef struct
{

    /*message */
    bool hash_data_result;

    /*signature */
    bool hash_signature_result;

    /*section */
    int failed_section;

    /* kind of hash to be stored */
    uint8 hash_kind;

    /* commit status */
    upgrade_fw_if_commit_status commit_status;

} upgrade_fw_if_mock_task_data_t;


upgrade_fw_if_mock_task_data_t upgrade_fw_if_mock = {.hash_data_result = TRUE, .hash_signature_result = TRUE, .failed_section = 
-1, .hash_kind = 1, .commit_status = 2};


#define upgradeFWIF_MockGetContextData() (&upgrade_fw_if_mock)

UpgradeFWIFPartitionHdl UpgradeFWIFPartitionOpen(UpgradePartition physPartition, uint32 firstWord)
{
    Sink sink;
    /** TODO
     * When audio is supported, we can determine the QSPI to use from the partition.
     * Until then only QSPI zero is used.
     */
    uint16 QSPINum = 0;

    DEBUG_LOG_DEBUG("UpgradeFWIFPartitionOpen: opening partition %u and instance %u for \
        resume", physPartition.partitionInstance.partitionID, physPartition.partitionInstance.instanceID);
    sink = ImageUpgradeStreamGetSink(QSPINum, physPartition.partitionInstance.partitionID, firstWord);
    DEBUG_LOG_DEBUG("UpgradeFWIFPartitionOpen: ImageUpgradeStreamGetSink(%d, %d, 0x%08lx) \
            returns %p\n", QSPINum, physPartition.partitionInstance.partitionID, firstWord, sink);
    if (!sink)
    {
        DEBUG_LOG_ERROR("UpgradeFWIFPartitionOpen: failed to open raw partition %u for resume", physPartition.partitionInstance.partitionID);
        return (UpgradeFWIFPartitionHdl)NULL;
    }
    SinkConfigure(sink, VM_SINK_MESSAGES, VM_MESSAGES_NONE);

    UpgradeCtxGetFW()->partitionNum = physPartition;

    return (UpgradeFWIFPartitionHdl)sink;
}

UpgradeFWIFApplicationValidationStatus UpgradeFWIFValidateApplication(void)
{
    return UPGRADE_FW_IF_APPLICATION_VALIDATION_SKIP;
}

bool UpgradeFWIFHashSection(upgrade_fw_if_hash_algo_t algo,
                            UpgradePartition section,
                            uint8 *hash,
                            uint16 hash_len_bytes,
                            bool *is_result_ready)
{
    UNUSED(algo);
    UNUSED(hash);
    UNUSED(hash_len_bytes);
    *is_result_ready= FALSE;
    upgrade_fw_if_mock_task_data_t * the_upgrade_fw_if_mock = upgradeFWIF_MockGetContextData();

    MessageSendLater(UpgradeGetAppTask(), MESSAGE_IMAGE_UPGRADE_HASH_ALL_SECTIONS_UPDATE_STATUS, NULL, 3000);
    
    if(section.partitionInstance.partitionID == the_upgrade_fw_if_mock->failed_section)
        return FALSE;
    return TRUE;
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
    upgrade_fw_if_mock_task_data_t * the_upgrade_fw_if_mock = upgradeFWIF_MockGetContextData();
    for (int i = 0; i < hash_len_bytes ; i++)
    {   
       *hash = 0xFF;
        hash ++;
    }
    *is_result_ready= TRUE;
    return the_upgrade_fw_if_mock->hash_data_result;
}

uint32 UpgradeFWIFGetPhysPartitionSize(UpgradePartition physPartition)
{   
    /** 
     * When audio is supported, we can determine the QSPI to use from the partition.
     * Until then only QSPI zero is used.
     */
#define QSPI_NUM 0
    uint32 size;
    if(ImageUpgradeGetInfo(QSPI_NUM, physPartition.partitionInstance.partitionID, IMAGE_SIZE, &size))
    {
        return (2 * size);
    }
    return 0;
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
    upgrade_fw_if_mock_task_data_t * the_upgrade_fw_if_mock = upgradeFWIF_MockGetContextData();
    uint8 hash_val;
    if(the_upgrade_fw_if_mock->hash_kind == 1)
        hash_val = 0xFF;
    else
        hash_val = 0xF0;

    for (uint16 i = 0; i < hash_len_bytes ; i++)
    {   
       *hash = hash_val;
        hash ++;
    }
    
    return TRUE;
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
    upgrade_fw_if_mock_task_data_t * the_upgrade_fw_if_mock = upgradeFWIF_MockGetContextData();
    return the_upgrade_fw_if_mock->hash_signature_result;
}

UpgradePartitionsState UpgradeFWIFEraseSection(upgrade_fw_if_section_filter_t filter)
{
    MESSAGE_MAKE(msg, MessageImageUpgradeEraseStatus);

    if (filter == UPGRADE_FW_IF_FILTER_ALTERNATE_IMAGE_HEADER)
    {
        ImageUpgradeErase();
        return UPGRADE_PARTITIONS_ERASING_HEADER;
    }
    else if (filter ==  UPGRADE_FW_IF_FILTER_ALTERNATE_BANK_SECTIONS)
    {
        msg->erase_status = TRUE;
        MessageSendLater(UpgradeGetAppTask(), MESSAGE_IMAGE_UPGRADE_ERASE_STATUS, (void*)msg, 10);
        return UPGRADE_PARTITIONS_ERASING;
    }
    return UPGRADE_PARTITIONS_DIRTY;
}

bool UpgradeFWIFIsImageHeader(UpgradePartition section)
{
    if (section.partitionInstance.partitionID == 1 && section.partitionInstance.instanceID == 0)
        return TRUE;
    return FALSE;
}

void UpgradeFWIFCommitUpgrades(void)
{
    upgrade_fw_if_mock_task_data_t * the_upgrade_fw_if_mock = upgradeFWIF_MockGetContextData();
    bool result = ImageUpgradeSwapCommit();
    UpgradePartitionsCommitUpgrade();
    UpgradeCtxGetFW()->commit_status = the_upgrade_fw_if_mock->commit_status;
    DEBUG_LOG_DEBUG("UpgradeFWIFCommitUpgrades: ImageUpgradeSwapCommit = %u", result);
}

UpgradePartition UpgradeFWIFIncreaseUpgradePartition(UpgradePartition partition)
{
    partition.partitionInstance.partitionID += 1;
    return partition;
}

UpgradePartition UpgradeFWIFBytesToUpgradePartition(const uint8* data)
{
    UpgradePartition partition;
    partition.partitionInstance.partitionID = data[0];
    partition.partitionInstance.instanceID = data[1];

    return partition;
}

short UpgradeFWIFCmpUpgradePartition(UpgradePartition partition1, UpgradePartition partition2)
{
    return (partition1.partitionInstance.partitionID - partition2.partitionInstance.partitionID);
}

uint16 UpgradeFWIFSerializePartitionID(UpgradePartition partition)
{
    return partition.partitionInstance.partitionID;
}

bool UpgradeFWIIFIsCommitStatusAvailable(void)
{
    return TRUE;
}


