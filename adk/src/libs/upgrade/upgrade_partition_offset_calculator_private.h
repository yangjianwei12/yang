/****************************************************************************
Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partition_offset_calculator.h

DESCRIPTION
    Header file for upgrade_partition_offset_calculator.
*/

#ifndef UPGRADE_PARTITION_OFFSET_CALCULATOR_PRIVATE_H_
#define UPGRADE_PARTITION_OFFSET_CALCULATOR_PRIVATE_H_

#include "upgrade_partition_data.h"
#include "upgrade_partition_data_priv.h"
#include "upgrade.h"
#include "upgrade_fw_if.h"
#include "upgrade_ctx.h"

typedef union part_data_t
{
    UpgradeFWIFPartitionHdl partition_hdl;
    uint8* signature;
} part_data_t;

typedef struct
{
    uint8 key_cache[PSKEY_MAX_STORAGE_LENGTH_IN_BYTES];
    part_data_t part_data;
    uint32 total_file_offset;
    uint32 expected_offset;
    uint32 partition_len;
    uint32 part_offset;
    bool is_upgrade_hdr_available;
    uint16 pskey;
    uint16 pskey_offset;
    uint16 pskey_len;
    UpgradePartition part_num;
    UpgradePartitionDataState state;
    uint8 pending_partition;
    uint8 total_partition;
    uint16 hash_table_offset;
} upgrade_partition_offset_calculator_context_t;

bool readHeaderPskey(upgrade_partition_offset_calculator_context_t* context, uint8* buff, uint16 size);

uint16 checkHeaderPskey(upgrade_partition_offset_calculator_context_t* context, uint16 size);

bool upgradePartitionOffsetCalculatorProcessNextStep(upgrade_partition_offset_calculator_context_t* context);

bool upgradePartitionOffsetCalculatorHandleGeneric1stPart(upgrade_partition_offset_calculator_context_t* context);

bool upgradePartitionOffsetCalculatorHandleHeader(upgrade_partition_offset_calculator_context_t* context);

bool upgradePartitionOffsetCalculatorHandlePartitionHeader(upgrade_partition_offset_calculator_context_t* context);

bool upgradePartitionOffsetCalculatorHandlePartitionData(upgrade_partition_offset_calculator_context_t* context);

bool upgradePartitionOffsetCalculatorHandleFooter(upgrade_partition_offset_calculator_context_t* context);

bool upgradePartitionOffsetCalculatorHandleClosedPartition(upgrade_partition_offset_calculator_context_t* context, UpgradePartition part_num);

void upgradePartitionOffsetCalculatorParsePartitionHeader(upgrade_partition_offset_calculator_context_t* context, uint8* buff);

/*!
    @brief Config specific post processing to initialize the upgrade lib context.

    @param context offset calculator context handle.
    @return TRUE if success.
*/
void upgradePartitionOffsetCalculatorReinitPostPartitionCtx(upgrade_partition_offset_calculator_context_t* context);

/*!
    @brief Config specific post processing of Generic 1st Part.

    @param context offset calculator context handle.
    @param buff buffer containing Generic 1st Part.
    @return TRUE if success.
*/
bool upgradePartitionOffsetCalculatorHandlePostGeneric1stPart(upgrade_partition_offset_calculator_context_t* context, uint8* buff);
#endif /* UPGRADE_PARTITION_OFFSET_CALCULATOR_PRIVATE_H_ */
