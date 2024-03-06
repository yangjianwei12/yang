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
#include <imageupgrade.h>
#include <ps.h>
#include <system_clock.h>
#include "upgrade_partition_data.h"
#include "upgrade_partition_data_priv.h"
#include "upgrade_msg_internal.h"
#include "upgrade_ctx.h"
#include "upgrade_fw_if.h"
#include "upgrade_psstore.h"
#include "upgrade_partitions.h"
#include "rsa_decrypt.h"
#include <app/message/system_message.h>


#if defined (UPGRADE_RSA_2048)
/*
 * EXPECTED_SIGNATURE_SIZE is the size of an RSA-2048 signature.
 * 2048 bits / 8 bits per byte is 256 bytes.
 */
#define EXPECTED_SIGNATURE_SIZE 256
#elif defined (UPGRADE_RSA_1024)
/*
 * EXPECTED_SIGNATURE_SIZE is the size of an RSA-1024 signature.
 * 1024 bits / 8 bits per byte is 128 bytes.
 */
#define EXPECTED_SIGNATURE_SIZE 128
#else
#error "Neither UPGRADE_RSA_2048 nor UPGRADE_RSA_1024 defined."
#endif

#define FIRST_WORD_SIZE 4
#define PARTITION_SECOND_HEADER_SIZE 4

#define BYTES_TO_WORDS(_b_)       ((_b_+1) >> 1)
#define UPGRADE_PARTITION_ZERO 0

/*
 * A module variable set by the UpgradePartitionDataHandleHeaderState function
 * to the value of the last byte in the data array. Retrieved by the
 * UpgradeFWIFValidateFinalize function using the
 * UpgradePartitionDataGetSigningMode function.
 */
static uint8 SigningMode = 1;
uint8 first_word_size = FIRST_WORD_SIZE;

/******************************************************************************
NAME
    UpgradePartitionDataGetHeaderID

DESCRIPTION
    Get the identifier for the header of an upgrade file.

RETURNS
    const char * Pointer to the header string.
*/
const char *UpgradePartitionDataGetHeaderID(void)
{
    return "APPUHDR5";
}

/******************************************************************************
NAME
    UpgradePartitionDataGetPartitionID

DESCRIPTION
    Get the identifier for a partition header within an upgrade file.

RETURNS
    const char * Pointer to the partition header string.
*/
const char *UpgradePartitionDataGetPartitionID(void)
{
    return "PARTDATA";
}

/******************************************************************************
NAME
    UpgradePartitionDataGetFooterID

DESCRIPTION
    Get the identifier for the footer of an upgrade file.

RETURNS
    const char * Pointer to the footer string.
*/
const char *UpgradePartitionDataGetFooterID(void)
{
    return "APPUPFTR";
}

/****************************************************************************
NAME
    IsValidPartitionNum

DESCRIPTION
    Validates the partition number

RETURNS
    bool TRUE if the partition number is valid, else FALSE
*/
static bool IsValidPartitionNum(uint16 partNum)
{
    switch (partNum)
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
    default:
        return FALSE;
    }
}

/****************************************************************************
NAME
    UpgradePartitionDataPartitionSecondHeaderSize

DESCRIPTION
    Returns the size of second header 

RETURNS
    Returns the size of second header 
*/
uint32 UpgradePartitionDataPartitionSecondHeaderSize(void)
{
    return PARTITION_SECOND_HEADER_SIZE;
}

/****************************************************************************
NAME
    handleHashCheckResult

DESCRIPTION
    Returns the size of second header 
PARAM
    hashCheckDone Check for hash check done 
    hashCheckedOk Check for hash is ok 
RETURNS
     void 
*/
static void handleHashCheckResult(bool hashCheckDone, bool hashCheckedOk)
{
    if(hashCheckDone)
     {
         /* Once hash check is done, free up the signature and reset the hash ctx
            irrepsctive of the fact if hash check was successful or failure. */
         free(UpgradeCtxGet()->partitionData->signature);
         UpgradeCtxGet()->partitionData->signature = 0;
         UpgradeCtxGet()->vctx = NULL;
     }
     
     if(hashCheckedOk)
     {
         DEBUG_LOG_DEBUG("handleHashCheckResult: OK");

        /* Start the image copy process */
         ImageUpgradeCopy();
         UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_COPY);
     }
}

/***************************************************************************
NAME
    UpgradeIsCaseDfuHeaderInPSKeys

DESCRIPTION
    Check if the first header PSKEY contains the header of case DFU file.

RETURNS
    TRUE if PSKEY contains the case DFU Header Id
*/
bool UpgradeIsCaseDfuHeaderInPSKeys(void)
{
    return UpgradeSearchIDInHeaderPSKeys(EXT_CASE_DFU_FILE_HEADER);
}

/****************************************************************************
NAME
    UpgradePartitionDataInit  -  Initialise.

DESCRIPTION
    Initialises the partition data header handling.

RETURNS
    bool TRUE if OK, FALSE if not.
*/
bool UpgradePartitionDataInit(void)
{
    UpgradePartitionDataCtx *ctx;

    ctx = UpgradeCtxGetPartitionData();
    if (!ctx)
    {
        ctx = malloc(sizeof(*ctx));
        if (!ctx)
        {
            return FALSE;
        }
        memset(ctx, 0, sizeof(*ctx));
        UpgradeCtxSetPartitionData(ctx);
    }

    /* \todo May need to take the status of peer into account, but not directly available */
    if ((UpgradeCtxGetPSKeys()->state_of_partitions == UPGRADE_PARTITIONS_UPGRADING) &&
        !UpgradeCtxGet()->force_erase)
    {
        if(!ctx->newReqSize)
        {
            /* Request the remaining partition again because the upgrade is resuming. */
            DEBUG_LOG_INFO("UpgradePartitionDataInit total requested data %u remaining data %u", ctx->totalReqSize, ctx->totalReqSize - ctx->totalReceivedSize);
            UpgradePartitionDataRequestData(ctx->totalReqSize - ctx->totalReceivedSize, UpgradeCtxGet()->dfu_file_offset);
        }
        /* A partial update has been interrupted. Don't erase. */
        DEBUG_LOG_INFO("UpgradePartitionDataInit: partial update interrupted. Not erasing, newReqSize %d offset %d dfu_file_offset %d totalReqSize %d totalReceivedSize %d",
            ctx->newReqSize, ctx->offset, UpgradeCtxGet()->dfu_file_offset, ctx->totalReqSize , ctx->totalReceivedSize);
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);
        return TRUE;
    }
    UpgradeFWIFInit();
    /*these were initialized as part of UpgradePartitionDataInitHelper called from dfu domain*/
    DEBUG_LOG_INFO("UpgradePartitionDataInit, Post UpgradePartitionDataRequestData: UpgradePartitionDataCtx->partitionLength = %d, totalReqSize = %d, totalReceivedSize = %d, newReqSize = %d, offset = %d, state enum:UpgradePartitionDataState:%d",
                    ctx->partitionLength, ctx->totalReqSize, ctx->totalReceivedSize, 
                    ctx->newReqSize, ctx->offset, ctx->state);

    /* Ensure the other bank is erased before we start. */
    if (UPGRADE_PARTITIONS_ERASING == UpgradePartitionsEraseAllManaged())
    {
        UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_ERASE);
        return TRUE;
    }

    return FALSE;
}

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_ERASE
*/
bool UpgradePartitionDataHandleErase(MessageId id, Message message)
{
    switch(id)
    {
        case MESSAGE_IMAGE_UPGRADE_ERASE_STATUS:
        {
            MessageImageUpgradeEraseStatus *msg = (MessageImageUpgradeEraseStatus *)message;
            UpdateResumePoint resumePoint = UpgradeCtxGetPSKeys()->upgrade_in_progress_key;

            DEBUG_LOG("UpgradeSMEraseStatus, erase_status %u", msg->erase_status);

            if (resumePoint == UPGRADE_RESUME_POINT_START)
            {
                if (msg->erase_status)
                {
                    DEBUG_LOG_INFO("Upgrade, SQIF erased");
                    /*
                     * Reset to dispatch the conditionally queued notification of
                     * UPGRADE_START_DATA_IND on sucessful erase completion.
                     */
                    UpgradeCtxGet()->isImgUpgradeEraseDone = 0;

                    /* The SQIF has been erased successfully. Request the first part of header. */
                    UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART);
                    UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE, 0);
                    MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);
                }
                else
                {
                    /* Tell the host that the attempt to erase the SQIF failed. */
                    UpgradeFatalError(UPGRADE_HOST_ERROR_SQIF_ERASE);
                }
            }
            else
            {
                DEBUG_LOG_ERROR("UpgradeSMEraseStatus, unexpected resume point %u", resumePoint);
            }
        }
        return TRUE;

        default:
            return FALSE;
    }
}

/****************************************************************************
NAME
    UpgradePartitionDataHandleHeaderState  -  Parser for the main header.

DESCRIPTION
    Validates content of the main header.

RETURNS
    Upgrade library error code.

NOTES
    Currently when main header size will grow beyond block size it won't work.
*/
UpgradeHostErrorCode UpgradePartitionDataHandleHeaderState(const uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    UpgradeHostErrorCode rc = UPGRADE_HOST_SUCCESS;
    upgrade_version newVersion;
    uint16 newPSVersion;
    uint16 endCursor = 0; /* Pointer into variable length portion of header */
    UpgradeHostErrorCode headerParseResult;
    if (!reqComplete)
    {
        /* Handle the case where packet size is smaller than header size */
        DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleHeaderState, header not complete");
        return UpgradePartitionDataParseIncomplete(data, len);
    }

    /* Length must contain at least ID FIELD, major, minor and compatibleVersions */
    if (len < (ID_FIELD_SIZE + UPGRADE_VERSION_SIZE + UPGRADE_NO_OF_COMPATIBLE_UPGRADES_SIZE))
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleHeaderState, packet size incorrect");
        return UPGRADE_HOST_ERROR_BAD_LENGTH_UPGRADE_HEADER;
    }

    /* TODO: Check length */
    headerParseResult = UpgradePartitionDataParseCommonHeader(data,
                                                              len,
                                                              &newVersion,
                                                              &newPSVersion,
                                                              &endCursor,
                                                              FALSE);

    if(headerParseResult != UPGRADE_HOST_SUCCESS)
    {
        return headerParseResult;
    }

    /* Store the in-progress upgrade version */
    UpgradeCtxGetPSKeys()->version_in_progress.major = newVersion.major;
    UpgradeCtxGetPSKeys()->version_in_progress.minor = newVersion.minor;
    UpgradeCtxGetPSKeys()->config_version_in_progress = newPSVersion;

    DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleHeaderState, saving versions %u.%u, PS version %u",
                       newVersion.major, newVersion.minor, newPSVersion);

    /* At this point, partitions aren't actually dirty - but want to minimise PSKEYS
     * @todo: Need to check this variable before starting an upgrade
     */
    UpgradeCtxGetPSKeys()->state_of_partitions = UPGRADE_PARTITIONS_UPGRADING;

    /*!
        @todo Need to minimise the number of times that we write to the PS
              so this may not be the optimal place. It will do for now.
    */
    UpgradeSavePSKeys();

    rc = UpgradeSaveHeaderInPSKeys(data, len);
    if (UPGRADE_HOST_SUCCESS != rc)
        return rc;

    UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE, 0);
    ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;
    MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);

    /* Set the signing mode to the value of the last byte in the data array */
    SigningMode = data[len - 1];
    DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleHeaderState, signing mode %u", SigningMode);

    return rc;
}

/****************************************************************************
NAME
    UpgradePartitionDataHandleDataHeaderState  -  Parser for the partition data header.

DESCRIPTION
    Validates content of the partition data header.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode UpgradePartitionDataHandleDataHeaderState(const uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    UpgradePartition partNum;
    uint16 sqifNum;
    uint32 firstWord;

    if (!reqComplete)
    {
        /* Handle the case where packet size is smaller than header size */
        DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleDataHeaderState, header not complete");
        return UpgradePartitionDataParseIncomplete(data, len);
    }

    if (len < PARTITION_SECOND_HEADER_SIZE + FIRST_WORD_SIZE)
        return UPGRADE_HOST_ERROR_BAD_LENGTH_DATAHDR_RESUME;

    sqifNum = ByteUtilsGet2BytesFromStream(data);
    DEBUG_LOG("UpgradePartitionDataHandleDataHeaderState PART_DATA: SQIF number %u", sqifNum);

    partNum.partitionID = ByteUtilsGet2BytesFromStream(&data[2]);
    DEBUG_LOG("UpgradePartitionDataHandleDataHeaderState PART_DATA: partition number %u", partNum.partitionID);

    if (!IsValidPartitionNum(partNum.partitionID))
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataHeaderState, partition %u, is not valid", partNum.partitionID);
        return UPGRADE_HOST_ERROR_WRONG_PARTITION_NUMBER;
    }

    UpgradeHostErrorCode rc = UpgradeSaveHeaderInPSKeys(data, PARTITION_SECOND_HEADER_SIZE);
    if (UPGRADE_HOST_SUCCESS != rc)
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataHeaderState, failed to store PSKEYSs, error %u", rc);
        return rc;
    }

    /* reset the firstword before using */
    firstWord  = (uint32)data[PARTITION_SECOND_HEADER_SIZE + 3] << 24;
    firstWord |= (uint32)data[PARTITION_SECOND_HEADER_SIZE + 2] << 16;
    firstWord |= (uint32)data[PARTITION_SECOND_HEADER_SIZE + 1] << 8;
    firstWord |= (uint32)data[PARTITION_SECOND_HEADER_SIZE];
    DEBUG_LOG_INFO("UpgradePartitionDataHandleDataHeaderState, first word is 0x%08lx", firstWord);

    UpgradeCtxGetPSKeys()->first_word = firstWord;
    UpgradeSavePSKeys();

    if (UpgradeCtxGetPSKeys()->last_closed_partition.partitionID > partNum.partitionID)
    {
        DEBUG_LOG_INFO("UpgradePartitionDataHandleDataHeaderState, already handled partition %u, skipping it", partNum.partitionID);
        UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE, ctx->partitionLength - FIRST_WORD_SIZE);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);
        UpgradeCtxGetFW()->partitionNum = partNum;
        return UPGRADE_HOST_SUCCESS;
    }

    if (ctx->partitionLength > UpgradeFWIFGetPhysPartitionSize(partNum))
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataHeaderState, partition size mismatch, \
            upgrade %lu, actual %lu", ctx->partitionLength, UpgradeFWIFGetPhysPartitionSize(partNum));
        return UPGRADE_HOST_ERROR_PARTITION_SIZE_MISMATCH;
    }

    /* Partition could have been already open if we are resuming (without rebooting the device) 
       the upgrade so, check and use the partitionHdl if its non-zero. */
    if(!ctx->partitionHdl)
    {
        DEBUG_LOG_INFO("UpgradePartitionDataHandleDataHeaderState, open partion %d to write", partNum.partitionID);
        ctx->partitionHdl = UpgradeFWIFPartitionOpen(partNum, firstWord);
    }
    else
    {
        /* fwCtx gets reinitialized on resume so, set the partitionNum even if we are using old partitionHdl. */
        UpgradeCtxGetFW()->partitionNum = partNum;
    }

    if (!ctx->partitionHdl)
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataHeaderState, failed to open partition %u", partNum.partitionID);
        return UPGRADE_HOST_ERROR_PARTITION_OPEN_FAILED;
    }

    DEBUG_LOG_INFO("UpgradePartitionDataHandleDataHeaderState, partition length %lu", ctx->partitionLength);
    if (FIRST_WORD_SIZE < ctx->partitionLength)
    {
        ctx->time_start = SystemClockGetTimerTime();

        /* Get partition data from the start, but skipping the first word. */
        UpgradePartitionDataRequestData(ctx->partitionLength - FIRST_WORD_SIZE, 0);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_DATA;
    }
    else if (FIRST_WORD_SIZE == ctx->partitionLength)
    {
        /* A case when all data are in but partition is not yet closed */
        UpgradeHostErrorCode closeStatus = UpgradeFWIFPartitionClose(ctx->partitionHdl);
        ctx->partitionHdl = NULL;
        if (UPGRADE_HOST_SUCCESS != closeStatus)
        {
            DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataHeaderState, failed to close partition %u, status %u", partNum.partitionID, closeStatus);
            return closeStatus;
        }

        ctx->openNextPartition = TRUE;

        UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE, 0);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;
    }
    else
    {
        /* It is considered bad when partition length is less than FIRST_WORD_SIZE */
        return UPGRADE_HOST_ERROR_INTERNAL_ERROR_3;
    }
    MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);

    return UPGRADE_HOST_SUCCESS;
}

/****************************************************************************
NAME
    UpgradePartitionDataHandleGeneric1stPartState  -  Parser for ID & length part of a header.

DESCRIPTION
    Parses common beginning of any header and determines which header it is.
    All headers have the same first two fields which are 'header id' and
    length.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode UpgradePartitionDataHandleGeneric1stPartState(const uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    uint32 length;
    UpgradeHostErrorCode rc = UPGRADE_HOST_SUCCESS;

    if (!reqComplete)
    {
        /* Handle the case where packet size is smaller than header size */
        DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleGeneric1stPartState, header not complete");
        return UpgradePartitionDataParseIncomplete(data, len);
    }

    if (len < HEADER_FIRST_PART_SIZE)
        return UPGRADE_HOST_ERROR_BAD_LENGTH_TOO_SHORT;

    length = ByteUtilsGet4BytesFromStream(&data[ID_FIELD_SIZE]);

    DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleGeneric1stPartState, id '%c%c%c%c%c%c%c%c', length 0x%lx",
                      data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], length);

    /* APPUHDR5 */
    if(0 == strncmp((char *)data, UpgradePartitionDataGetHeaderID(), ID_FIELD_SIZE))
    {
        if (length < UPGRADE_HEADER_MIN_SECOND_PART_SIZE)
            return UPGRADE_HOST_ERROR_BAD_LENGTH_UPGRADE_HEADER;

        if (UPGRADE_IS_CASE_SUPPORTED)
        {
            /* Inform the application so that it can abort the case DFU if it was started using DFU mode */
            MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_CASE_DFU_EARBUD_IMAGE_IND, NULL);
        }

        /* Store the header data of upgrade header on PSKEY */
        ctx->dfuHeaderPskey = DFU_HEADER_PSKEY_START;
        ctx->dfuHeaderPskeyOffset = 0;
        rc = UpgradeSaveHeaderInPSKeys(data, len);
        if (UPGRADE_HOST_SUCCESS != rc)
            return rc;

        /* Clear the dfu_partition_num pskey value before the file transfer,
         *.so that no junk value is stored. This pskey value is used during
         * application reconnect after defined reboot.
         */
        UpgradeCtxGetPSKeys()->dfu_partition_num = 0;
        UpgradeSavePSKeys();

        UpgradePartitionDataRequestData(length, 0);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_HEADER;
        ctx->isUpgradeHdrAvailable = TRUE;
    }
    /* EXTUHDR1 */
    else if(UPGRADE_IS_CASE_SUPPORTED && (0 == strncmp((char *)data, EXT_CASE_DFU_FILE_HEADER, ID_FIELD_SIZE)))
    {
        UpgradeCtxGet()->caseDfuHeaderLength = length;
        UpgradeCtxGet()->caseDfuStatus = UPGRADE_CASE_DFU_IN_PROGRESS;
        /* Inform the application that the incoming file is for charger case. */
        MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_CASE_DFU_CASE_IMAGE_IND, NULL);

        /* Store the header data of upgrade header on PSKEY */
        ctx->dfuHeaderPskey = DFU_HEADER_PSKEY_START;
        ctx->dfuHeaderPskeyOffset = 0;
        rc = UpgradeSaveHeaderInPSKeys(data, len);
        if (UPGRADE_HOST_SUCCESS != rc)
            return rc;

        UpgradePartitionDataRequestData(0, 0);
        /* Suspend the data transfer. Upgrade lib doesn't handle the Case DFU file. */
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_SUSPEND_DATA_TRANSFER, NULL);
        return UPGRADE_HOST_SUCCESS;
    }
    else if (0 == strncmp((char *)data, UpgradePartitionDataGetPartitionID(), ID_FIELD_SIZE))
    {
        if((UpgradeCtxGetFW()->partitionNum.partitionID == UPGRADE_PARTITION_ZERO) && (!ctx->isUpgradeHdrAvailable))
        {
            DEBUG_LOG_INFO("UpgradePartitionDataHandleGeneric1stPartState,Error: Upgrade Header is not available");
            return UPGRADE_HOST_ERROR_BAD_LENGTH_UPGRADE_HEADER;
        }
        if(length < PARTITION_SECOND_HEADER_SIZE + FIRST_WORD_SIZE)
        {
            return UPGRADE_HOST_ERROR_BAD_LENGTH_PARTITION_HEADER;
        }

        rc = UpgradeSaveHeaderInPSKeys(data, len);
        if(UPGRADE_HOST_SUCCESS != rc)
        {
            return rc;
        }

        UpgradePartitionDataRequestData(PARTITION_SECOND_HEADER_SIZE + FIRST_WORD_SIZE, 0);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_DATA_HEADER;
        ctx->partitionLength = length - PARTITION_SECOND_HEADER_SIZE;
    }
    else if (0 == strncmp((char *)data, UpgradePartitionDataGetFooterID(), ID_FIELD_SIZE))
    {
        if (length != EXPECTED_SIGNATURE_SIZE)
        {
            /* The length of signature must match expected length.
             * Otherwise OEM signature checking could be omitted by just
             * setting length to 0 and not sending signature.
             */
            return UPGRADE_HOST_ERROR_BAD_LENGTH_SIGNATURE;
        }

        rc = UpgradeSaveHeaderInPSKeys(data, len);
        if(UPGRADE_HOST_SUCCESS != rc)
        {
            return rc;
        }

        UpgradePartitionDataRequestData(length, 0);
        ctx->partitionLength = length;

        if(!ctx->signature)
        {
            /* if earlier malloc'd signature could not be free'd, don't malloc again */
            ctx->signature = malloc(length);
        }
        if (!ctx->signature)
        {
            return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_MEMORY;
        }

        ctx->state = UPGRADE_PARTITION_DATA_STATE_FOOTER;
    }
    else
    {
        return UPGRADE_HOST_ERROR_UNKNOWN_ID;
    }
    MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);

    return UPGRADE_HOST_SUCCESS;
}

/****************************************************************************
NAME
    UpgradePartitionDataHandleDataState  -  Partition data handling.

DESCRIPTION
    Writes data to a SQIF and sends it the MD5 validation.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode UpgradePartitionDataHandleDataState(const uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();

    if (len != UpgradeFWIFPartitionWrite(ctx->partitionHdl, data, len))
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataState, partition write failed, length %u",
                        len);
        return UPGRADE_HOST_ERROR_PARTITION_WRITE_FAILED_DATA;
    }

    if (reqComplete)
    {
        DEBUG_LOG_INFO("UpgradePartitionDataHandleDataState, partition write complete");

        rtime_t duration_ms = rtime_sub(SystemClockGetTimerTime(), ctx->time_start) / 1000;
        uint32 bytes_per_sec = (ctx->totalReqSize * 1000) / duration_ms;
        DEBUG_LOG_INFO("UpgradePartitionDataHandleDataState, took %lu ms, %lu bytes/s", duration_ms, bytes_per_sec);

        UpgradeHostErrorCode closeStatus;
        UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE, 0);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);
        closeStatus = UpgradeFWIFPartitionClose(ctx->partitionHdl);
        ctx->partitionHdl = NULL;
        
        DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleDataState partition close status is enum:UpgradeHostErrorCode:%d", closeStatus);
        if (UPGRADE_HOST_SUCCESS != closeStatus)
        {
            DEBUG_LOG_INFO("UpgradePartitionDataHandleDataState, failed to close partition");
            return closeStatus;
        }

        ctx->openNextPartition = TRUE;
    }
    else
        DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleDataState, waiting for more data");

    return UPGRADE_HOST_SUCCESS;
}

/****************************************************************************
NAME
    HandleFooterState  -  Signature data handling.

DESCRIPTION
    Collects MD5 signature data and sends it for validation.
    Completion of this step means that data were download to a SQIF.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode upgradeHandleFooterState(const uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();

    UpgradePartitionDataCopyFromStream(ctx->signature, ctx->signatureReceived, data, len);

    ctx->signatureReceived += len;

    UpgradeHostErrorCode rc = UpgradeSaveHeaderInPSKeys(data, len);

    if(UPGRADE_HOST_SUCCESS != rc)
    {
        return rc;
    }

    if(reqComplete)
    {
        ctx->signatureReceived = 0;

        /* Inform the application that we have received all data. */
        UpgradeSendUpgradeStatusInd(UpgradeGetAppTask(), upgrade_state_download_completed, 0);

        /* Move to hash checking as we have received all data. */
        UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_WAIT_FOR_VALIDATION);
        return UPGRADE_HOST_DATA_TRANSFER_COMPLETE;
    }

    return UPGRADE_HOST_SUCCESS;
}

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_WAIT_FOR_VALIDATION
*/
bool UpgradePartitionDataHandleWaitForValidation(MessageId id, Message message)
{
    UNUSED(message);
    
    UpgradeCtx *ctx = UpgradeCtxGet();
    
    DEBUG_LOG_DEBUG("upgradeSm_HandleDataHashChecking: MESSAGE:UpgradeMsgInternal:0x%04X", id);
    
    bool hashCheckDone = FALSE;
    bool hashCheckedOk = FALSE;
    
    switch(id)
    {
        case UPGRADE_PARTITION_DATA_INTERNAL_START_VALIDATION:
        {
            ctx->vctx = ImageUpgradeHashInitialise(SHA256_ALGORITHM);

            if (ctx->vctx == NULL)
            {
                Panic();
            }
            
            switch(UpgradeFWIFValidateStart(ctx->vctx))
            {
                case UPGRADE_HOST_OEM_VALIDATION_SUCCESS:
                    hashCheckedOk = UpgradeFWIFValidateFinish(ctx->vctx, ctx->partitionData->signature);
                    if(!hashCheckedOk)
                    {
                        UpgradeFatalError(UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_FOOTER);
                    }
                    hashCheckDone = TRUE;
                    break;
                    
                case UPGRADE_HOST_HASHING_IN_PROGRESS:
                    UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_HASH_CHECK);
                    break;
                    
                case UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_FOOTER:
                default:
                    UpgradeFatalError(UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_FOOTER);
                    hashCheckDone = TRUE;
                    break;
            }
        }
        break;
    }

    handleHashCheckResult(hashCheckDone, hashCheckedOk);
    return TRUE;
}


/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_HASH_CHECK
*/
bool UpgradePartitionDataHandleHashCheck(MessageId id, Message message)
{
    UNUSED(message);
    
    UpgradeCtx *ctx = UpgradeCtxGet();
    
    DEBUG_LOG_DEBUG("UpgradePartitionDataHandleHashCheck 0x%04X", MESSAGE_IMAGE_UPGRADE_HASH_ALL_SECTIONS_UPDATE_STATUS);
    
    bool hashCheckDone = FALSE;
    bool hashCheckedOk = FALSE;
    
    switch(id)
    {
        case MESSAGE_IMAGE_UPGRADE_HASH_ALL_SECTIONS_UPDATE_STATUS:
        {
            MessageImageUpgradeHashAllSectionsUpdateStatus *msg = (MessageImageUpgradeHashAllSectionsUpdateStatus*)message;
            DEBUG_LOG("UpgradePartitionDataHandleHashCheck, status %u", msg->status);
            
            if(msg->status)
            {
                hashCheckedOk = UpgradeFWIFValidateFinish(ctx->vctx, ctx->partitionData->signature);
                if(!hashCheckedOk)
                {
                    UpgradeFatalError(UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_FOOTER);
                }
                hashCheckDone = TRUE;
            }
            else
            {
                UpgradeFatalError(UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_FOOTER);
                hashCheckDone = TRUE;
            }
        }
        break;

        default:
            return FALSE;
    }
    handleHashCheckResult(hashCheckDone, hashCheckedOk);
    return TRUE;
}

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_COPY
*/
bool UpgradePartitionDataHandleCopy(MessageId id, Message message)
{
    UNUSED(message);

    DEBUG_LOG_DEBUG("UpgradePartitionDataHandleCopy 0x%04X", MESSAGE_IMAGE_UPGRADE_COPY_STATUS);

    switch(id)
    {
        case MESSAGE_IMAGE_UPGRADE_COPY_STATUS:
        {
            MessageImageUpgradeCopyStatus *msg = (MessageImageUpgradeCopyStatus *)message;
            /* Let application know that copy is done */
            UpgradeSMBlockingOpIsDone();
            
            if (msg->copy_status)
            {          
                /*
                 * The SQIF has been copied successfully.
                 */
                UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_VALIDATION_COMPLETE);
                /* Inform the application that we have completed the validation */
                UpgradeSendUpgradeStatusInd(UpgradeGetAppTask(), upgrade_state_validation_completed, 0);
            }
            else
            {
                /* Tell the host that the attempt to copy the SQIF failed. */
                UpgradeFatalError(UPGRADE_HOST_ERROR_SQIF_COPY);
            }

        }
        break;

        default:
            return FALSE;
    }

    return TRUE;
}

/****************************************************************************
NAME
    UpgradePartitionDataCopyFromStream  -  Copy from stream.

DESCRIPTION
    Accounts for differences in offset value in CONFIG_HYDRACORE.

RETURNS
    Nothing.
*/
void UpgradePartitionDataCopyFromStream(uint8 *signature, uint16 offset, const uint8 *data, uint16 len)
{
    ByteUtilsMemCpyFromStream(&signature[offset], data, len);
}

/****************************************************************************
NAME
    UpgradePartitionDataGetSigningMode

DESCRIPTION
    Gets the signing mode value set by the header.

RETURNS
    uint8 signing mode.
*/
uint8 UpgradePartitionDataGetSigningMode(void)
{
    return SigningMode;
}

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_ERASE_IMAGE_HEADER

DESCRIPTION
    This is not applicable for HYDRACORE
*/
bool UpgradePartitionDataHandleEraseImageHeader(MessageId id, Message message)
{
    UNUSED(message);
    UNUSED(id);
    return FALSE;
}

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_ERASE_BANK

DESCRIPTION
    This is not applicable for HYDRACORE
*/
bool UpgradePartitionDataHandleEraseBank(MessageId id, Message message)
{
    UNUSED(message);
    UNUSED(id);
    return FALSE;
}

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_SIGNATURE_CHECK

DESCRIPTION
    This is not applicable for HYDRACORE
*/

UpgradeHostErrorCode UpgradePartitionDataHandleSignatureCheck(MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(message);
    return FALSE;
}

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_HASH_CHECK_HEADER
    
DESCRIPTION
    This is not applicable for HYDRACORE
*/
bool UpgradePartitionDataHandleHashCheckHeader(MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(message);
    return FALSE;
}

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_HASH_CHECK_DATA
    
DESCRIPTION
    This is not applicable for HYDRACORE
*/
bool UpgradePartitionDataHandleHashCheckData(MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(message);
    return FALSE;
}

