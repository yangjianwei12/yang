/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partition_offset_calculator_config.c

DESCRIPTION
    upgrade_partition_offset_calculator config file for HYDRACORE variant.
*/

#define DEBUG_LOG_MODULE_NAME upgrade
#include <logging.h>
#include <stdlib.h>
#include <system_clock.h>
#include "upgrade_partition_offset_calculator.h"
#include "upgrade_partition_offset_calculator_private.h"

bool upgradePartitionOffsetCalculatorHandleHeader(upgrade_partition_offset_calculator_context_t* context)
{
    uint8* buff =  malloc(context->partition_len);
    if (!buff)
    {
        /* Aborting the DFU as no memory available */
        UpgradeFatalError(UPGRADE_HOST_ERROR_NO_MEMORY);
        return FALSE;
    }

    if(!readHeaderPskey(context, buff, context->partition_len))
    {
        free(buff);
        return FALSE;
    }
    context->total_file_offset += context->partition_len;
    context->is_upgrade_hdr_available = TRUE;
    context->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;

    free(buff);
    DEBUG_LOG("upgradePartitionOffsetCalculatorHandleHeader next state %d offset %lu",context->state, context->total_file_offset);
    return TRUE;
}

void upgradePartitionOffsetCalculatorParsePartitionHeader(upgrade_partition_offset_calculator_context_t* context, uint8* buff)
{
    context->part_num = UpgradeFWIFBytesToUpgradePartition(&buff[PARTITION_TYPE_SIZE]);
}

bool upgradePartitionOffsetCalculatorHandlePostGeneric1stPart(upgrade_partition_offset_calculator_context_t* context, uint8* buff)
{
    if (0 == strncmp((char *)buff, UpgradePartitionDataGetFooterID(), ID_FIELD_SIZE))
    {
       DEBUG_LOG("upgradePartitionOffsetCalculatorHandlePostGeneric1stPart reached footer");
       context->state = UPGRADE_PARTITION_DATA_STATE_FOOTER;
       return TRUE;
    }
    return FALSE;
}

bool upgradePartitionOffsetCalculatorHandleClosedPartition(upgrade_partition_offset_calculator_context_t* context, UpgradePartition part_num)
{
    UNUSED(part_num);
    context->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;
    return TRUE;
}

bool upgradePartitionOffsetCalculatorHandleFooter(upgrade_partition_offset_calculator_context_t* context)
{
    context->part_data.signature =  malloc(context->partition_len);
    if (!context->part_data.signature)
    {
        DEBUG_LOG_ERROR("upgradePartitionOffsetCalculatorHandleFooter Can not allocate signature memory");
        UpgradeFatalError(UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_MEMORY);
        return FALSE;
    }
    if(!readHeaderPskey(context, context->part_data.signature, context->partition_len))
    {
        DEBUG_LOG_ERROR("upgradePartitionOffsetCalculatorHandleFooter Footer not available");
        free(context->part_data.signature);
        return FALSE;
    }
    context->total_file_offset += context->partition_len;
    context->state = UPGRADE_PARTITION_DATA_STATE_WAIT_FOR_VALIDATION;
    DEBUG_LOG("upgradePartitionOffsetCalculatorHandleFooter offset %lu", context->total_file_offset);
    /* return FALSE as the footer is the last component in file. */
    return FALSE;
}

void upgradePartitionOffsetCalculatorReinitPostPartitionCtx(upgrade_partition_offset_calculator_context_t* context)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    switch (context->state)
    {
        case UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART:
        {
            ctx->totalReqSize = HEADER_FIRST_PART_SIZE;
            ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;
        }
        break;

        case UPGRADE_PARTITION_DATA_STATE_HEADER:
        {
            ctx->totalReqSize = context->partition_len;
            ctx->state = UPGRADE_PARTITION_DATA_STATE_HEADER;
        }
        break;

        case UPGRADE_PARTITION_DATA_STATE_DATA_HEADER:
        {
                ctx->totalReqSize = UpgradePartitionDataPartitionSecondHeaderSize() + PARTITION_FIRST_WORD_SIZE;
                ctx->state = UPGRADE_PARTITION_DATA_STATE_DATA_HEADER;
        }
        break;

        case UPGRADE_PARTITION_DATA_STATE_DATA:
        {
            ctx->time_start = SystemClockGetTimerTime();
            ctx->totalReqSize = context->partition_len - context->part_offset;
            ctx->state = UPGRADE_PARTITION_DATA_STATE_DATA;
            ctx->partitionHdl = context->part_data.partition_hdl;
            ctx->partitionLength = context->partition_len+PARTITION_FIRST_WORD_SIZE;
        }
        break;

        case UPGRADE_PARTITION_DATA_STATE_FOOTER:
        {
            ctx->totalReqSize = context->partition_len;
            ctx->partitionLength = context->partition_len;
            ctx->state = UPGRADE_PARTITION_DATA_STATE_FOOTER;
            ctx->signature = malloc(context->partition_len);
            if (!ctx->signature)
            {
                /* Aborting the DFU as no memory available */
                UpgradeFatalError(UPGRADE_HOST_ERROR_NO_MEMORY);
                return;
            }
            ctx->signatureReceived = 0;
        }
        break;

        case UPGRADE_PARTITION_DATA_STATE_WAIT_FOR_VALIDATION:
        {
            ctx->state = UPGRADE_PARTITION_DATA_STATE_WAIT_FOR_VALIDATION;
            ctx->signature = context->part_data.signature;
            ctx->signatureReceived = 0;
        }
        break;

        case UPGRADE_PARTITION_DATA_STATE_VALIDATION_COMPLETE:
        {
            ctx->state = UPGRADE_PARTITION_DATA_STATE_VALIDATION_COMPLETE;
        }
        break;

        default:
            break;
    }

}

bool upgradePartitionOffsetCalculatorProcessNextStep(upgrade_partition_offset_calculator_context_t* context)
{
    DEBUG_LOG("upgradePartitionOffsetCalculatorProcessNextStep %d", context->state);
    switch (context->state)
    {
        case UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART:
            return upgradePartitionOffsetCalculatorHandleGeneric1stPart(context);

        case UPGRADE_PARTITION_DATA_STATE_HEADER:
            return upgradePartitionOffsetCalculatorHandleHeader(context);

        case UPGRADE_PARTITION_DATA_STATE_DATA_HEADER:
            return upgradePartitionOffsetCalculatorHandlePartitionHeader(context);

        case UPGRADE_PARTITION_DATA_STATE_DATA:
            return upgradePartitionOffsetCalculatorHandlePartitionData(context);

        case UPGRADE_PARTITION_DATA_STATE_FOOTER:
            return upgradePartitionOffsetCalculatorHandleFooter(context);

        default:
            //TO_DO: Handle rest of the newly added states
            return FALSE;
    }
}


