/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partition_offset_calculator_config.c

DESCRIPTION
    upgrade_partition_offset_calculator config file for CDA2 variant.
*/

#define DEBUG_LOG_MODULE_NAME upgrade
#include <logging.h>

#include <stdlib.h>
#include <system_clock.h>
#include <byte_utils.h>
#include "upgrade_partition_offset_calculator.h"
#include "upgrade_partition_offset_calculator_private.h"
#include "upgrade_psstore.h"

#define UPGRADE_HASH_SIZE 48
#define UPGRADE_SIGNATURE_LENGTH 96

bool upgradePartitionOffsetCalculatorHandleHeader(upgrade_partition_offset_calculator_context_t* context)
{
    uint8* buff;
    uint8* buff_header;
    upgrade_version newVersion;
    uint16 newPSVersion;
    uint16 endCursor = 0;

    uint16 available_header = checkHeaderPskey(context, context->partition_len + HEADER_FIRST_PART_SIZE);

    if(available_header < context->partition_len)
    {
        /* We haven't received full header yet so, request pending */
        context->part_offset = available_header;
        return FALSE;
    }

    buff =  malloc(context->partition_len + HEADER_FIRST_PART_SIZE);
    buff_header = buff + HEADER_FIRST_PART_SIZE;
    if (!buff)
    {
        /* Aborting the DFU as no memory available */
        UpgradeFatalError(UPGRADE_HOST_ERROR_NO_MEMORY);
        return FALSE;
    }

    if(!readHeaderPskey(context, buff_header, context->partition_len))
    {
        free(buff);
        return FALSE;
    }

    if(available_header == context->partition_len)
    {
        /* We have received the header but no further data after that so,  
           signature might not have been verified. */
        uint32 signatureOffset = context->partition_len + HEADER_FIRST_PART_SIZE - UPGRADE_SIGNATURE_LENGTH;

        /* Add the generic first part also in the buffer for signature verification. */
        memcpy(buff, UpgradePartitionDataGetHeaderID(), ID_FIELD_SIZE);
        ByteUtilsSet4Bytes(&buff[ID_FIELD_SIZE],0, context->partition_len);
        if(!UpgradeFWIFSignatureVerify(UPGRADE_FW_IF_SHA384, buff, context->partition_len - 
                UPGRADE_SIGNATURE_LENGTH + HEADER_FIRST_PART_SIZE, buff + signatureOffset, 
                        UPGRADE_SIGNATURE_LENGTH))
        {
            DEBUG_LOG_ERROR("UpgradePartitionDataHandleHeaderState, signature failed ");
            free(buff);
            UpgradeFatalError(UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_UPGRADE_HEADER);
            return FALSE;
        }
    }

    context->total_file_offset += context->partition_len;
    context->is_upgrade_hdr_available = TRUE;
    context->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;

    UpgradePartitionDataParseCommonHeader(buff + HEADER_FIRST_PART_SIZE,
                                          context->partition_len,
                                          &newVersion,
                                          &newPSVersion,
                                          &endCursor,
                                          TRUE);

    context->total_partition = context->pending_partition = 
    (context->partition_len - UPGRADE_SIGNATURE_LENGTH - endCursor) / UPGRADE_HASH_SIZE;
    context->hash_table_offset = endCursor + HEADER_FIRST_PART_SIZE;
    free(buff);
    DEBUG_LOG("upgradePartitionOffsetCalculatorHandleHeader next state enum:UpgradePartitionDataState:%d offset %lu",context->state, context->total_file_offset);
    return TRUE;
}

bool upgradePartitionOffsetCalculatorHandlePostGeneric1stPart(upgrade_partition_offset_calculator_context_t* context, uint8* buff)
{
    UNUSED(context);
    UNUSED(buff);
    return FALSE;
}

void upgradePartitionOffsetCalculatorParsePartitionHeader(upgrade_partition_offset_calculator_context_t* context, uint8* buff)
{
    context->part_num = UpgradeFWIFBytesToUpgradePartition(buff);
}

bool upgradePartitionOffsetCalculatorHandleClosedPartition(upgrade_partition_offset_calculator_context_t* context, 
                                                                               UpgradePartition part_num)
{
    UNUSED(part_num);
    context->pending_partition--;
    DEBUG_LOG("upgradePartitionOffsetCalculatorHandleClosedPartition pending_partition %d",context->pending_partition);
    if(context->pending_partition == 0)
    {
        context->state = UPGRADE_PARTITION_DATA_STATE_HASH_CHECK_DATA;
        return FALSE;
    }
    context->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;
    return TRUE;
}

void upgradePartitionOffsetCalculatorReinitPostPartitionCtx(upgrade_partition_offset_calculator_context_t* context)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    if(context->is_upgrade_hdr_available)
    {
        UpgradeSaveHashTableOffset(context->hash_table_offset);
        for(uint8 closed_partitions = context->total_partition - context->pending_partition; closed_partitions; closed_partitions--)
        {
            UpgradeIncreaseDfuHashTableCursor(UPGRADE_HASH_SIZE);
        }
        ctx->pendingPartition = context->pending_partition;
        ctx->totalPartitions = context->total_partition;
    }

    switch (context->state)
    {
        case UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART:
            if(context->total_partition == context->pending_partition)
            {
                ctx->totalReqSize = HEADER_FIRST_PART_SIZE;
                ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;
                DEBUG_LOG("upgradePartitionOffsetCalculatorReinitPartitionCtxState requesting gen1st part");
                break;
            }
            context->state = UPGRADE_PARTITION_DATA_STATE_HASH_CHECK_DATA;
        case UPGRADE_PARTITION_DATA_STATE_HASH_CHECK_DATA:
        {
            bool isResultReady;
            ctx->state = UPGRADE_PARTITION_DATA_STATE_HASH_CHECK_DATA;
            ctx->openNextPartition = TRUE;
            DEBUG_LOG("upgradePartitionOffsetCalculatorReinitPartitionCtxState starting hash check");
            if(!UpgradeFWIFHashSection(UPGRADE_FW_IF_SHA384, ctx->partitionNum, NULL, 0, &isResultReady))
            {
                UpgradeFatalError(UPGRADE_HOST_ERROR_INTERNAL_ERROR_1);
            }
        }
        break;

        case UPGRADE_PARTITION_DATA_STATE_HEADER:
        {
            ctx->totalReqSize = context->partition_len - context->part_offset;
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
    DEBUG_LOG("upgradePartitionOffsetCalculatorProcessNextStep enum:UpgradePartitionDataState:%d", context->state);
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

        default:
            //TO_DO: Handle rest of the newly added states
            return FALSE;
    }
}
