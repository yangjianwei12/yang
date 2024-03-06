/****************************************************************************
Copyright (c) 2014 - 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partition_data.c

DESCRIPTION
    Upgrade file processing state machine.
    It is parsing and validating headers.
    All received data are passed to MD5 validation.
    Partition data are written to SQIF.

NOTES

*/

#define DEBUG_LOG_MODULE_NAME upgrade
#include <logging.h>

#include <string.h>
#include <stdlib.h>
#include <byte_utils.h>
#include <panic.h>
#include <print.h>

#include "upgrade_partition_data.h"
#include "upgrade_partition_data_priv.h"
#include "upgrade_ctx.h"
#include "upgrade_fw_if.h"
#include "upgrade_psstore.h"
#include "upgrade_partitions.h"
#include "upgrade_msg_internal.h"

/*
*  For the time being, UPGRADE_HEADER_DATA_LOG is being defined to 36 [fixed] and can be updated/changed later
*  when when exact size be found out what makes DFU FIRST HEADER as 48 (12 from HEADER_FIRST_PART_SIZE)  while
* assuming remaining 36 comes from UPGRADE_HEADER_DATA_LOG
*
*  It should be ensured that UPGRADE_HEADER_DATA_LOG is NOT USED in the code.
*/
#define UPGRADE_HEADER_DATA_LOG 36

/* This compile time assert deals with total number of sectoins and NOT the actual
*  numbe of upgradable sections as:-
*  1. Exact number of upgradable sections are not available in the public domain.
*  2. Upgradable section and Non-Upgradable sections does not occupy seperate spaces and
*     are intermingled. If U is Upgradable section and N is Non-upgradable, sections may
*     be arranged like UNNUUUUUNNUNUNUN and not UUUUUUUUU followed by NNNNNNN.
*  Hence this assert is an early WARNING as it takes into account 'MAX' sections available.
*  Actual/Real warning is only possible iff exact number of Upgradable Sections are known.
*  
*  In calculation below-
*  Upgrade Header Size: 12 + 36 = 48 bytes which is-
*		HEADER_FIRST_PART_SIZE+ UPGRADE_HEADER_DATA_LOG = 12 + 36 =48
*
*  Partition Header (plus first word) Size: IMAGE_SECTION_ID_MAX * (HEADER_FIRST_PART_SIZE+FIRST_WORD_SIZE*PARTITION_SECOND_HEADER_SIZE); 
*		where IMAGE_SECTION_ID_MAX is 0xe i.e, 15 partitions (num partition), so 15 * (12+4+4) = 15*20= 300 bytes
*
*  Footer Size: 12 + 256(OEM Signature length) = 268 = HEADER_FIRST_PART_SIZE+EXPECTED_SIGNATURE_SIZE
*
*  Total 616 bytes. So using 10 PSKEYs (10 * 64 = 640 bytes) where
*	PSKEY_MAX_STORAGE_LENGTH = 32 words of uint16 size, but sizeof is not allowed in a preprocessor hence replaced with 2.
*	DFU_HEADER_PSKEY_LEN = 10 for 10 PSKEYs
*
*
*   Negation of condition is used, i.e, while LHS is < RHS, overflow will not happen and hence no warning.
*/
#if (((HEADER_FIRST_PART_SIZE+UPGRADE_HEADER_DATA_LOG)+\
	(IMAGE_SECTION_ID_MAX*(HEADER_FIRST_PART_SIZE+PARTITION_SECOND_HEADER_SIZE+FIRST_WORD_SIZE)+\
        (HEADER_FIRST_PART_SIZE+EXPECTED_SIGNATURE_SIZE)))>= (2*PSKEY_MAX_STORAGE_LENGTH*DFU_HEADER_PSKEY_LEN))
        #error *** Early WARNING! May exhaust PSKEYS ***
#endif


#ifndef MIN
#define MIN(a,b)    (((a)<(b))?(a):(b))
#endif

static UpgradeHostErrorCode upgradeParseCompleteData(const uint8 *data, uint16 len, bool reqComplete);

void UpgradePartitionDataDestroy(void)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    if (ctx)
    {
        if(ctx->signature)
        {
            free(ctx->signature);
            ctx->signature = NULL;
        }
        free(ctx);

        UpgradeCtxSetPartitionData(NULL);
    }
}


uint32 UpgradePartitionDataGetNextReqSize(void)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    uint32 size = ctx->totalReqSize - ctx->totalReceivedSize;

    if (UpgradeCtxGet()->max_request_size)
    {
        /* Transport has a max request size, so check against it. */
        if (size > UpgradeCtxGet()->max_request_size)
        {
            /* Remaining data larger than transport's max request size.
               Request the biggest chunk we can instead. */
            size = UpgradeCtxGet()->max_request_size;
        }

        /* Wait for each chunk to finish before requesting the next. */
        if (ctx->chunkReceivedSize > 0)
        {
            /* Still receiving current chunk, check if it's finished. */
            if (ctx->chunkReceivedSize < UpgradeCtxGet()->max_request_size)
            {
                /* Chunk not finished, don't request more data until it has. */
                size = 0;
            }
            else
            {
                /* Chunk finished, reset chunk data count. */
                ctx->chunkReceivedSize = 0;
            }
        }
    }
    else
    {
        /* Transport does not have a max request size. Request the entire
           partition first time around (unless resuming a previous upgrade - the
           value stored in newReqSize takes account of this), then always return
           0 for subsequent calls whilst the transfer is in progress. */
        size = ctx->newReqSize;
        ctx->newReqSize = 0;
    }
    return size;
}


uint32 UpgradePartitionDataGetNextOffset(void)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    uint32 offset = ctx->offset;
    ctx->offset = 0;
    return  offset;
}


UpgradeHostErrorCode UpgradePartitionDataParseIncomplete(const uint8 *data, uint16 data_len)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    UpgradeHostErrorCode status = UPGRADE_HOST_SUCCESS;

    DEBUG_LOG_VERBOSE("UpgradePartitionDataParseIncomplete, data_len %d", data_len);

    /* TODO: Allocate dynamic buffer, instead of static buffer */

    /* Calculate space left in buffer */
    uint8 buf_space = sizeof(ctx->incompleteData.data) - ctx->incompleteData.size;

    if (data_len > buf_space)
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataParseIncomplete, header too big for buffer");
        return UPGRADE_HOST_ERROR_BAD_LENGTH_UPGRADE_HEADER;
    }

    /* Copy into buffer */
    memmove(&ctx->incompleteData.data[ctx->incompleteData.size], data, data_len);
    ctx->incompleteData.size += data_len;

    /* Parse data if request complete or buffer is full */
    const bool req_complete = (ctx->totalReceivedSize == ctx->totalReqSize);
    DEBUG_LOG_VERBOSE("UpgradePartitionDataParseIncomplete, received %u, requested %u, complete %u",
                   ctx->totalReceivedSize, ctx->totalReqSize, req_complete);

    /* If request is now complete attempt to parse again */
    if (req_complete)
    {
        DEBUG_LOG_DATA_V_VERBOSE(ctx->incompleteData.data, ctx->incompleteData.size);
        status = upgradeParseCompleteData(ctx->incompleteData.data, ctx->incompleteData.size,
                                          req_complete);
        ctx->incompleteData.size = 0;
    }

    DEBUG_LOG_VERBOSE("UpgradePartitionDataParseIncomplete, status %u", status);
    return status;
}


/****************************************************************************
NAME
    UpgradePartitionDataParse

DESCRIPTION
    If the received upgrade data is larger than the expected size in the next 
    data request 'nextReqSize' then this loops through until the received data 
    is either completely written into the SQIF or copied into the upgrade 
    buffer,'incompleteData'

RETURNS
    Upgrade library error code.
*/

UpgradeHostErrorCode UpgradePartitionDataParse(const uint8 *data, uint16 data_len)
{
    uint32 pc;
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    UpgradeHostErrorCode status = UPGRADE_HOST_SUCCESS;

    /* Update total received size */
    ctx->totalReceivedSize += data_len;
    ctx->chunkReceivedSize += data_len;
    UpgradeCtxGet()->dfu_file_offset += data_len;

    if(UpgradeCtxGet()->max_request_size &&
       ctx->chunkReceivedSize >= UpgradeCtxGet()->max_request_size &&
       ctx->totalReceivedSize < ctx->totalReqSize )
    {
        /* If the host has provided max_request_size then we are requesting in chunks
         * of max_request_size so, once we have received a complete chunk we need to request next. */
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);
    }

    pc = 100 * ctx->totalReceivedSize / ctx->totalReqSize;
    DEBUG_LOG_INFO("UpgradePartitionDataParse, data_len %u, total_size %u, total_received %u (%u%%) dfu_file_offset %lu",
                   data_len, ctx->totalReqSize, ctx->totalReceivedSize, pc, UpgradeCtxGet()->dfu_file_offset);

    /* Error if we've received more than we requested */
    if (ctx->totalReceivedSize > ctx->totalReqSize)
        return UPGRADE_HOST_ERROR_BAD_LENGTH_PARTITION_PARSE;

    /* Handle case of incomplete data */
    if (ctx->incompleteData.size)
        status = UpgradePartitionDataParseIncomplete(data, data_len);
    else
    {
        /* Parse block */
        const bool req_complete = ctx->totalReceivedSize == ctx->totalReqSize;
        status = upgradeParseCompleteData(data, data_len, req_complete);
    }

    DEBUG_LOG_VERBOSE("UpgradePartitionDataParse, status %u", status);
    return status;
}


/****************************************************************************
NAME
    UpgradePartitionDataStopData  -  Stop processing incoming data

DESCRIPTION
    Function to stop the processing of incoming data.
*/
void UpgradePartitionDataStopData(void)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    ctx->totalReqSize = ctx->newReqSize = 0;
    ctx->totalReceivedSize = 0;
    ctx->chunkReceivedSize = 0;
}


/****************************************************************************
NAME
    UpgradePartitionDataRequestData

DESCRIPTION
    Determine size of the next data request
*/
void UpgradePartitionDataRequestData(uint32 size, uint32 offset)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    DEBUG_LOG_DEBUG("UpgradePartitionDataRequestData: size=%u offset=%u", size, offset);
    ctx->totalReqSize = ctx->newReqSize = size;
    ctx->totalReceivedSize = 0;
    ctx->chunkReceivedSize = 0;
    ctx->offset += offset;
}


/****************************************************************************
NAME
    upgradeParseCompleteData  -  Parser state machine

DESCRIPTION
    Calls state handlers depending of current state.
    All state handlers are setting size of next data request.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode upgradeParseCompleteData(const uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    UpgradeHostErrorCode rc = UPGRADE_HOST_ERROR_INTERNAL_ERROR_1;

    DEBUG_LOG_VERBOSE("upgradeParseCompleteData, state %u, length %d, complete %u, offset %u",
                      ctx->state, len, reqComplete, ctx->offset);

    UpgradePartitionDataState state = ctx->state;
    switch (ctx->state)
    {
    case UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART:
        DEBUG_LOG_VERBOSE("upgradeParseCompleteData, UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART");
        rc = UpgradePartitionDataHandleGeneric1stPartState(data, len, reqComplete);
        break;

    case UPGRADE_PARTITION_DATA_STATE_HEADER:
        DEBUG_LOG_VERBOSE("upgradeParseCompleteData, UPGRADE_PARTITION_DATA_STATE_HEADER");
        rc = UpgradePartitionDataHandleHeaderState(data, len, reqComplete);
        break;

    case UPGRADE_PARTITION_DATA_STATE_DATA_HEADER:
        DEBUG_LOG_VERBOSE("upgradeParseCompleteData, UPGRADE_PARTITION_DATA_STATE_DATA_HEADER");
        rc = UpgradePartitionDataHandleDataHeaderState(data, len, reqComplete);
        break;

    case UPGRADE_PARTITION_DATA_STATE_DATA:
        DEBUG_LOG_VERBOSE("upgradeParseCompleteData, UPGRADE_PARTITION_DATA_STATE_DATA");
        rc = UpgradePartitionDataHandleDataState(data, len, reqComplete);
        break;

    case UPGRADE_PARTITION_DATA_STATE_FOOTER:
        DEBUG_LOG_VERBOSE("upgradeParseCompleteData, UPGRADE_PARTITION_DATA_STATE_FOOTER");
        rc = upgradeHandleFooterState(data, len, reqComplete);
        break;

    default:
        break;
    }

    if (state != ctx->state)
        DEBUG_LOG_INFO("upgradeParseCompleteData, new state %u", ctx->state);
    if (rc != UPGRADE_HOST_SUCCESS)
        DEBUG_LOG_INFO("upgradeParseCompleteData, status %u", rc);

    return rc;
}

bool UpgradePartitionDataHandleMessage(MessageId id, Message message)
{
    if(!UpgradeCtxGetPartitionData())
    {
        return FALSE;
    }
    DEBUG_LOG("UpgradePartitionDataHandleMessage, state %u, message_id 0x%04x", UpgradeCtxGetPartitionData()->state, id);

    switch (UpgradeCtxGetPartitionData()->state)
    {
        case UPGRADE_PARTITION_DATA_STATE_ERASE_IMAGE_HEADER:
            DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleMessage, UPGRADE_PARTITION_DATA_STATE_ERASE_IMAGE_HEADER");
            return UpgradePartitionDataHandleEraseImageHeader(id, message);

        case UPGRADE_PARTITION_DATA_STATE_ERASE_BANK:
            DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleMessage, UPGRADE_PARTITION_DATA_STATE_ERASE_BANK");
            return UpgradePartitionDataHandleEraseBank(id, message);

        case UPGRADE_PARTITION_DATA_STATE_ERASE:
            DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleMessage, UPGRADE_PARTITION_DATA_STATE_ERASE");
            return UpgradePartitionDataHandleErase(id, message);

        case UPGRADE_PARTITION_DATA_STATE_HASH_CHECK:
            DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleMessage, UPGRADE_PARTITION_DATA_STATE_HASH_CHECK");
            return UpgradePartitionDataHandleHashCheck(id, message);

        case UPGRADE_PARTITION_DATA_STATE_HASH_CHECK_DATA:
            DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleMessage, UPGRADE_PARTITION_DATA_STATE_HASH_CHECK_DATA");
            return UpgradePartitionDataHandleHashCheckData(id, message);

        case UPGRADE_PARTITION_DATA_STATE_WAIT_FOR_VALIDATION:
            DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleMessage, UPGRADE_PARTITION_DATA_STATE_WAIT_FOR_VALIDATION");
            return UpgradePartitionDataHandleWaitForValidation(id, message);

        case UPGRADE_PARTITION_DATA_STATE_COPY:
            DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleMessage, UPGRADE_PARTITION_DATA_STATE_COPY");
            return UpgradePartitionDataHandleCopy(id, message);

        default:
            return FALSE;
    }
}

UpgradeHostErrorCode UpgradePartitionDataParseCommonHeader(const uint8* buff, uint16 length,
                                                           upgrade_version* newVersion, 
                                                           uint16* newPSVersion, 
                                                           uint16* endCursor, 
                                                           bool skipCheck)
{
    uint16 cursor = 0;
    uint16 compatibleVersions;
    upgrade_version currVersion;
    uint16 psconfigVersions;
    uint16 version_itr;
    uint16 currPSVersion;

    /* Compare variant */
    if((strlen(UpgradeFWIFGetDeviceVariant()) > 0) &&
       (strncmp((char *)(buff + cursor), UpgradeFWIFGetDeviceVariant(), UPGRADE_HEADER_VARIANT_SIZE)))
    {
        DEBUG_LOG_VERBOSE("UpgradePartitionDataParseCommonHeader, wrong variant");
        return UPGRADE_HOST_ERROR_WRONG_VARIANT;
    }
    cursor += UPGRADE_HEADER_VARIANT_SIZE;

    /* Extract versions */
    newVersion->major = ByteUtilsGet2BytesFromStream(&buff[cursor]);
    cursor += UPGRADE_VERSION_PART_SIZE;
    newVersion->minor = ByteUtilsGet2BytesFromStream(&buff[cursor]);
    cursor += UPGRADE_VERSION_PART_SIZE;
    compatibleVersions = ByteUtilsGet2BytesFromStream(&buff[cursor]);
    cursor += UPGRADE_NO_OF_COMPATIBLE_UPGRADES_SIZE;
    currVersion.major = UpgradeCtxGetPSKeys()->version.major;
    currVersion.minor = UpgradeCtxGetPSKeys()->version.minor;

    DEBUG_LOG_VERBOSE("UpgradePartitionDataParseCommonHeader, compatible version %u.%u, %u",
                               newVersion->major, newVersion->minor, compatibleVersions);

    if (length < (cursor + (UPGRADE_VERSION_SIZE * compatibleVersions) + UPGRADE_PS_VERSION_SIZE + UPGRADE_NO_OF_COMPATIBLE_PS_VERSION_SIZE))
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataParseCommonHeader, Malformed packet. Invalid length: %u", length);
        return UPGRADE_HOST_ERROR_BAD_LENGTH_UPGRADE_HEADER;
    }

    version_itr=0;
    if(!skipCheck)
    {
        /* Check version compatibility */
        for (version_itr = 1; version_itr <= compatibleVersions; version_itr++)
        {
            upgrade_version version;
            version.major = ByteUtilsGet2BytesFromStream(&buff[cursor]);
            cursor += UPGRADE_VERSION_PART_SIZE;
            version.minor = ByteUtilsGet2BytesFromStream(&buff[cursor]);
            cursor += UPGRADE_VERSION_PART_SIZE;
            DEBUG_LOG_VERBOSE("UpgradePartitionDataParseCommonHeader, compatible version %u.%u",
                               version.major, version.minor);

            if ((version.major == currVersion.major)
                && ((version.minor == currVersion.minor) || (version.minor == 0xFFFFu)))
            {
                break;
            }
        }
        /* We failed to find a compatibility match */
        if (version_itr > compatibleVersions)
        {
            DEBUG_LOG_WARN("UpgradePartitionDataParseCommonHeader, no compatible versions");
            return UPGRADE_HOST_WARN_APP_CONFIG_VERSION_INCOMPATIBLE;
        }
    }
    cursor += (compatibleVersions - version_itr) * UPGRADE_VERSION_SIZE;

    DEBUG_LOG_VERBOSE("UpgradePartitionDataParseCommonHeader, current version %u.%u, new version %u.%u, compatible versions %u",
                      currVersion.major, currVersion.minor, newVersion->major, newVersion->minor, compatibleVersions);

    /* Extract PS versions */
    *newPSVersion = ByteUtilsGet2BytesFromStream(&buff[cursor]);
    cursor += UPGRADE_PS_VERSION_SIZE;

    psconfigVersions = ByteUtilsGet2BytesFromStream(&buff[cursor]);
    cursor += UPGRADE_NO_OF_COMPATIBLE_PS_VERSION_SIZE;

    if (length < (cursor + (UPGRADE_PS_VERSION_SIZE * psconfigVersions)))
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataParseCommonHeader, packet size incorrect %u", length);
        return UPGRADE_HOST_ERROR_BAD_LENGTH_UPGRADE_HEADER;
    }

    currPSVersion = UpgradeCtxGetPSKeys()->config_version;
    DEBUG_LOG_VERBOSE("UpgradePartitionDataParseCommonHeader, current PS version %u, new PS version %u",
                      currPSVersion, *newPSVersion);

    version_itr = 0;
    if (currPSVersion != *newPSVersion && !skipCheck)
    {
        DEBUG_LOG_VERBOSE("UpgradePartitionDataParseCommonHeader, number of compatible PS versions %u",
                          psconfigVersions);
        /* Check PS version compatibility */
        for (version_itr = 1; version_itr <= psconfigVersions; version_itr++)
        {
            uint16 version;
            version = ByteUtilsGet2BytesFromStream(&buff[cursor]);
            cursor += UPGRADE_PS_VERSION_SIZE;
            DEBUG_LOG_VERBOSE("UpgradePartitionDataParseCommonHeader, compatible PS version %u",
                              version);
            /* Break out of loop if compatible */
            if (version == currPSVersion)
                break;
        }
        if (version_itr > psconfigVersions)
        {
            DEBUG_LOG_WARN("UpgradePartitionDataParseCommonHeader, no compatible PS versions");
            return UPGRADE_HOST_WARN_APP_CONFIG_VERSION_INCOMPATIBLE;
        }
    }
    cursor += (psconfigVersions - version_itr) * UPGRADE_PS_VERSION_SIZE;
    *endCursor = cursor;
    return UPGRADE_HOST_SUCCESS;
}

bool UpgradePartitionDataIsDfuUpdate(void)
{
    return (UpgradeCtxGetPSKeys()->dfu_partition_num != 0);
}

uint16 UpgradePartitionDataGetDfuPartition(void)
{
    return UpgradeCtxGetPSKeys()->dfu_partition_num - 1;
}

bool UpgradeIsPartitionDataState(void)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    bool rc = TRUE;

    if(ctx == NULL)
    {
        return FALSE;
    }
     if(ctx->state == UPGRADE_PARTITION_DATA_STATE_DATA)
    {
        rc = TRUE;
    }
     else
    {
        rc = FALSE;
    }

    return rc;
}

void UpgradePartitionDataSetState(UpgradePartitionDataState state)
{
    UpgradeCtxGetPartitionData()->state = state;
}

uint16 UpgradePartitionDataDfuHeaderPskeyStart(void)
{
    return DFU_HEADER_PSKEY_START;
}

uint16 UpgradePartitionDataDfuHeaderPskeyEnd(void)
{
    return DFU_HEADER_PSKEY_END;
}

uint32 UpgradePartitionDataHeaderFirstPartSize(void)
{
    return HEADER_FIRST_PART_SIZE;
}

uint8 UpgradePartitionDataIdFieldSize(void)
{
    return ID_FIELD_SIZE;
}

UpgradePartitionDataState UpgradePartitionDataGetState(void)
{
    return UpgradeCtxGetPartitionData()->state;
}

/***************************************************************************
NAME
    UpgradePartitionDataStateIsHashCheck

DESCRIPTION
    Check if the Partition hash checking is currently going on
*/
bool UpgradePartitionDataStateIsHashCheck(void)
{
    return (UpgradeCtxGetPartitionData() && UpgradeCtxGetPartitionData()->state == UPGRADE_PARTITION_DATA_STATE_HASH_CHECK);
}

