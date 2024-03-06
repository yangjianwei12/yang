/****************************************************************************
Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partition_offset_calculator.c

DESCRIPTION
    Provides functionality to calculate Upgrade partition state variable from give offset.
*/

#define DEBUG_LOG_MODULE_NAME upgrade
#include <logging.h>

#include <string.h>
#include <stdlib.h>
#include <message.h>
#include <byte_utils.h>
#include <panic.h>
#include <system_clock.h>
#include "upgrade_partition_offset_calculator.h"
#include "upgrade_partition_offset_calculator_private.h"
#include "upgrade_fw_if.h"

/* An arbitrary high number which sets the upper limit for DFU file components */
#define MAX_DFU_FILE_COMPONENTS 1000

static bool loadNextPskey(upgrade_partition_offset_calculator_context_t* context, bool check_only)
{
    if(!context->pskey)
    {
        /* load first pskey */
        context->pskey = DFU_HEADER_PSKEY_START;
    }
    else if(context->pskey_len < PSKEY_MAX_STORAGE_LENGTH_IN_BYTES /* current pskey is not fully written so, it would be the last written key */
            || context->pskey > DFU_HEADER_PSKEY_END /* pskey overflow. An unlikely scenario. */)
    {
        return FALSE;
    }
    context->pskey_len = PsRetrieve(context->pskey, NULL, 0);
    if (context->pskey_len)
    {
        if(!check_only)
        {
            PsRetrieve(context->pskey, context->key_cache, context->pskey_len);
        }
        /* pskey var maintains the value of (current open pskey+1). */
        context->pskey++;
        context->pskey_len <<= 1;
        context->pskey_offset=0;
        return TRUE;
    }
    else
    {
        /* next pskey is empty */
        return FALSE;
    }
}

static uint16 iterateHeaderPskey(upgrade_partition_offset_calculator_context_t* context, uint8* buff, uint16 size, bool check_only)
{
    uint16 buff_offset = 0;
    uint16 read_size = size;

    /* local copies to restore if we are not able to read full data or check only. */
    uint16 header_pskey = context->pskey;
    uint16 pskey_offset = context->pskey_offset;
    uint16 pskey_len = context->pskey_len;

    if(!context->pskey && !loadNextPskey(context, check_only))
    {
        DEBUG_LOG_ERROR("readHeaderPskey error loading first key");
        context->pskey = header_pskey, context->pskey_offset = pskey_offset, context->pskey_len = pskey_len;
        return 0;
    }
    while(size)
    {
        uint16 diff;
        if(context->pskey_offset==context->pskey_len && !loadNextPskey(context, check_only))
        {
            /* requested component is not fully written so, restore the pskey details and we will rewrite
             * this component again. */
            context->pskey = header_pskey, context->pskey_offset = pskey_offset, context->pskey_len = pskey_len;

            DEBUG_LOG_ERROR("readHeaderPskey error loading key %d for size %d", context->pskey, size);

            /* Returns the actual bytes read. */
            return (read_size - size);
        }
        diff = MIN(size, (context->pskey_len - context->pskey_offset));
        if(!check_only)
        {
            memcpy(&buff[buff_offset], &context->key_cache[context->pskey_offset], diff);
            buff_offset+=diff;
        }
        context->pskey_offset+=diff;
        size-=diff;
    }

    if(check_only)
    {
        context->pskey = header_pskey, context->pskey_offset = pskey_offset, context->pskey_len = pskey_len;
    }
    return read_size;
}

bool readHeaderPskey(upgrade_partition_offset_calculator_context_t* context, uint8* buff, uint16 size)
{
    /* returns TRUE if we are able to read requested size. */
    return (iterateHeaderPskey(context, buff, size, FALSE) == size);
}

uint16 checkHeaderPskey(upgrade_partition_offset_calculator_context_t* context, uint16 size)
{
    /* returns actually read size. */
    return iterateHeaderPskey(context, NULL, size, TRUE);
}

bool upgradePartitionOffsetCalculatorHandleGeneric1stPart(upgrade_partition_offset_calculator_context_t* context)
{
    uint8 buff[HEADER_FIRST_PART_SIZE];
    if(!readHeaderPskey(context, buff, HEADER_FIRST_PART_SIZE))
    {
        return FALSE;
    }
    context->total_file_offset += HEADER_FIRST_PART_SIZE;
    context->partition_len = ByteUtilsGet4BytesFromStream(&buff[ID_FIELD_SIZE]);

    /* APPUHDRX */
    if(0 == strncmp((char *)buff, UpgradePartitionDataGetHeaderID(), ID_FIELD_SIZE))
    {
        context->state = UPGRADE_PARTITION_DATA_STATE_HEADER;
        DEBUG_LOG("upgradePartitionOffsetCalculatorHandleGeneric1stPart next state %d offset %lu",context->state, context->total_file_offset);
        return TRUE;
    }
    /* PARTDATA */
    if (0 == strncmp((char *)buff, UpgradePartitionDataGetPartitionID(), ID_FIELD_SIZE))
    {
        context->state = UPGRADE_PARTITION_DATA_STATE_DATA_HEADER;
        DEBUG_LOG("upgradePartitionOffsetCalculatorHandleGeneric1stPart next state %d offset %lu",context->state, context->total_file_offset);
        return TRUE;
    }
    if(upgradePartitionOffsetCalculatorHandlePostGeneric1stPart(context, buff))
    {
        return TRUE;
    }

    UpgradeFatalError(UPGRADE_HOST_ERROR_INTERNAL_ERROR_1);
    return FALSE;
}

bool upgradePartitionOffsetCalculatorHandlePartitionHeader(upgrade_partition_offset_calculator_context_t* context)
{
    uint8* buff = malloc(UpgradePartitionDataPartitionSecondHeaderSize() + PARTITION_FIRST_WORD_SIZE);
    if (!buff)
    {
        /* Aborting the DFU as no memory available */
        UpgradeFatalError(UPGRADE_HOST_ERROR_NO_MEMORY);
        return FALSE;
    }

    if(!readHeaderPskey(context, buff, UpgradePartitionDataPartitionSecondHeaderSize()))
    {
        free(buff);
        return FALSE;
    }
    context->total_file_offset += UpgradePartitionDataPartitionSecondHeaderSize() + PARTITION_FIRST_WORD_SIZE;
    context->partition_len -= UpgradePartitionDataPartitionSecondHeaderSize() + PARTITION_FIRST_WORD_SIZE;
    upgradePartitionOffsetCalculatorParsePartitionHeader(context, buff);

    context->state = UPGRADE_PARTITION_DATA_STATE_DATA;
    DEBUG_LOG("upgradePartitionOffsetCalculatorHandlePartitionHeader part_num %d offset %lu",UpgradeFWIFSerializePartitionID(context->part_num), context->total_file_offset);
    free(buff);
    return TRUE;
}

bool upgradePartitionOffsetCalculatorHandlePartitionData(upgrade_partition_offset_calculator_context_t* context)
{
    if(context->expected_offset)
    {
        if(context->expected_offset < (context->total_file_offset+context->partition_len))
        {
            context->part_offset = context->expected_offset - context->total_file_offset;
            context->total_file_offset = context->expected_offset;
            return TRUE;
        }
    }
    else if(UpgradeFWIFCmpUpgradePartition(context->part_num, UpgradeCtxGetPSKeys()->last_closed_partition) >= 0)
    {
        context->part_data.partition_hdl = UpgradeFWIFPartitionOpen(context->part_num, UpgradeCtxGetPSKeys()->first_word);
        if (!context->part_data.partition_hdl)
        {
            DEBUG_LOG_ERROR("upgradePartitionOffsetCalculatorHandlePartitionData, failed to open partition %u", UpgradeFWIFSerializePartitionID(context->part_num));
            UpgradeFatalError(UPGRADE_HOST_ERROR_PARTITION_OPEN_FAILED);
            return FALSE;
        }
        if(ImageUpgradeSinkGetPosition((Sink) context->part_data.partition_hdl, &context->part_offset)) /*Trap call to get offset from sink*/
        {
            DEBUG_LOG("upgradePartitionOffsetCalculatorHandlePartitionData Sink offset of interrupted partiton : %ld", context->part_offset);
        }
        else
        {
            DEBUG_LOG_ERROR("upgradePartitionOffsetCalculatorHandlePartitionData  Could not retrieve partition offset");
            UpgradeFatalError(UPGRADE_HOST_ERROR_PARTITION_OPEN_FAILED);
            return FALSE;
        }
        context->total_file_offset += context->part_offset;
        DEBUG_LOG("upgradePartitionOffsetCalculatorHandlePartitionData opened part_num %d offset %lu",UpgradeFWIFSerializePartitionID(context->part_num), context->total_file_offset);
        return FALSE;
    }
    context->total_file_offset += context->partition_len;

    DEBUG_LOG("upgradePartitionOffsetCalculatorHandlePartitionData skipping part_num %d offset %lu",UpgradeFWIFSerializePartitionID(context->part_num), context->total_file_offset);
    return upgradePartitionOffsetCalculatorHandleClosedPartition(context, context->part_num);
}

static bool upgradePartitionOffsetCalculatorReinitPartitionCtx(upgrade_partition_offset_calculator_context_t* context)
{
    UpgradePartitionDataCtx *ctx;
    ctx = malloc(sizeof(*ctx));
    if (!ctx)
    {
        /* Aborting the DFU as no memory available */
        UpgradeFatalError(UPGRADE_HOST_ERROR_NO_MEMORY);
        return FALSE;
    }
    memset(ctx, 0, sizeof(*ctx));
    UpgradeCtxSetPartitionData(ctx);

    /* Move to next pskey if we have exhausted current one. */
    if(context->pskey_offset == PSKEY_MAX_STORAGE_LENGTH_IN_BYTES)
    {
        context->pskey_offset = 0;
        context->pskey+=1;
    }
    /* In the context variable, we maintain {current open pskey+1} value so decrease it by 1 before using it further. */
    if(context->pskey)
    {
        context->pskey-=1;
    }
    ctx->dfuHeaderPskey = context->pskey;
    ctx->dfuHeaderPskeyOffset = context->pskey_offset;
    ctx->isUpgradeHdrAvailable = context->is_upgrade_hdr_available;
    UpgradeCtxGet()->dfu_file_offset = context->total_file_offset;
    UpgradeCtxGetFW()->partitionNum = context->part_num;

    upgradePartitionOffsetCalculatorReinitPostPartitionCtx(context);
    DEBUG_LOG_INFO("upgradePartitionOffsetCalculatorReinitPartitionCtx success pskey %d offset %d state enum:UpgradePartitionDataState:%d",ctx->dfuHeaderPskey, ctx->dfuHeaderPskeyOffset, ctx->state);
    return TRUE;
}

void UpgradePartitionOffsetCalculatorCalculateDfuOffset(void)
{
    upgrade_partition_offset_calculator_context_t context = {0};
    uint16 cnt = 0;
    context.state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;
    DEBUG_LOG("UpgradePartitionOffsetCalculatorCalculateDfuOffset");

    if(UpgradeCtxGetPSKeys()->upgrade_in_progress_key == UPGRADE_RESUME_POINT_PRE_REBOOT)
    {
        /* Both devices have completed data-xfer and validation so, no need to parse the dfu file headers. */
        context.state = UPGRADE_PARTITION_DATA_STATE_VALIDATION_COMPLETE;
    }
    else
    {
        for(cnt=MAX_DFU_FILE_COMPONENTS; upgradePartitionOffsetCalculatorProcessNextStep(&context) && cnt; cnt--);
        if(!cnt)
        {
            /* cnt is a fail-safe mechanism. It reaches 0 when upgradePartitionOffsetCalculatorProcessNextStep() is stuck in an endless loop due to some error.
                Aborting DFU with fatal error. */
            UpgradeFatalError(UPGRADE_HOST_ERROR_UPDATE_FAILED);
            return;
        }
    }
    upgradePartitionOffsetCalculatorReinitPartitionCtx(&context);
}

bool UpgradePartitionOffsetCalculatorGetPartitionStateFromDfuFileOffset(uint32 req_offset, upgrade_partition_state_t* expected_state)
{
    upgrade_partition_offset_calculator_context_t context = {0};
    context.state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;
    context.expected_offset = req_offset;
    uint16 cnt = 0;

    for(cnt=MAX_DFU_FILE_COMPONENTS; upgradePartitionOffsetCalculatorProcessNextStep(&context) && context.total_file_offset < req_offset && cnt; cnt--);
    if(!cnt)
    {
        /* cnt is a fail-safe mechanism. It reaches 0 when upgradePartitionOffsetCalculatorProcessNextStep() is stuck in an endless loop due to some error.
            Aborting DFU with fatal error. */
        UpgradeFatalError(UPGRADE_HOST_ERROR_UPDATE_FAILED);
        return FALSE;
    }

    if(context.total_file_offset < req_offset)
    {
        DEBUG_LOG_ERROR("UpgradePartitionOffsetCalculatorGetPartitionStateFromDfuFileOffset requested data %lu not available, total_file_offset %lu", req_offset, context.total_file_offset);
        return FALSE;
    }

    /* Move to next pskey if we have exhausted current one. */
    if(context.pskey_offset == PSKEY_MAX_STORAGE_LENGTH_IN_BYTES)
    {
        context.pskey_offset = 0;
        context.pskey+=1;
    }
    /* In the context variable, we maintain {current open pskey+1} value so decrease it by 1 before using it further. */
    if(context.pskey)
    {
        context.pskey-=1;
    }

    expected_state->pskey = context.pskey;
    expected_state->pskey_offset = context.pskey_offset;
    expected_state->state = context.state;
    expected_state->total_file_offset = context.total_file_offset;
    expected_state->partition_len = context.partition_len;
    expected_state->part_num = context.part_num;
    expected_state->part_offset = context.part_offset;
    expected_state->pending_partition = context.pending_partition;

    DEBUG_LOG_INFO("UpgradePartitionOffsetCalculatorGetPartitionStateFromDfuFileOffset state enum:UpgradePartitionDataState:%d pskey %d offset %d", expected_state->state,context.pskey, context.pskey_offset);
    return TRUE;
}

