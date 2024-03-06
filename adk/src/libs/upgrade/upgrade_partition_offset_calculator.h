/****************************************************************************
Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partition_offset_calculator.h

DESCRIPTION
    Header file for upgrade_partition_offset_calculator.
*/

#ifndef UPGRADE_PARTITION_OFFSET_CALCULATOR_H_
#define UPGRADE_PARTITION_OFFSET_CALCULATOR_H_

#include <upgrade.h>

void UpgradePartitionOffsetCalculatorCalculateDfuOffset(void);

bool UpgradePartitionOffsetCalculatorGetPartitionStateFromDfuFileOffset(uint32 req_offset, upgrade_partition_state_t* expected_state);

#endif /* UPGRADE_PARTITION_OFFSET_CALCULATOR_H_ */
