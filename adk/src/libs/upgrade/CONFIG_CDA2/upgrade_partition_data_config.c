/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partition_data.c

DESCRIPTION
    Upgrade file processing state machine.
    It is parsing and validating headers.
    Partition data are written to SQIF.

NOTES

*/

#define DEBUG_LOG_MODULE_NAME upgrade
#include <logging.h>
#include <string.h>
#include <byte_utils.h>
#include <panic.h>
#include <ps.h>
#include <system_clock.h>

#include <imageupgrade.h>
#include <stdlib.h>
#include "upgrade_partition_data.h"
#include "upgrade_msg_internal.h"
#include "upgrade_ctx.h"
#include "upgrade_partitions.h"
#include "upgrade_private.h"
#include "upgrade_psstore.h"
#include "upgrade_fw_if.h"

#define UPGRADE_PARTITION_ZERO 0
#define FIRST_WORD_SIZE 4
#define UPGRADE_HASH_SIZE (48)
#define UPGRADE_SIGNATURE_LENGTH (96)
#define PARTITION_SECOND_HEADER_SIZE (50)

/* Indentify all the partition is written */
#define IsLastPartition() (UpgradeCtxIsPartitionDataCtxValid() && \
    UpgradeCtxGetPartitionData()->pendingPartition == 0)


uint32 UpgradePartitionDataPartitionSecondHeaderSize(void)
{
    return PARTITION_SECOND_HEADER_SIZE;
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
/* CDA2_DFU_TODO: Need to implement in next ticket */
    UNUSED(partNum);

    return TRUE;
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

     /* if partition state is upgrading or upgrading header then resume */ 
    if (((UpgradeCtxGetPSKeys()->state_of_partitions == UPGRADE_PARTITIONS_UPGRADING) ||
          (UpgradeCtxGetPSKeys()->state_of_partitions == UPGRADE_PARTITIONS_UPGRADING_HEADER)) &&
          !UpgradeCtxGet()->force_erase)
    {
        if(ctx->state == UPGRADE_PARTITION_DATA_STATE_HASH_CHECK_DATA)
        {
            ctx->offset = UpgradeCtxGet()->dfu_file_offset;
            return TRUE;
        }
        if(!ctx->newReqSize)
        {
            /* Request the remaining partition again because the upgrade is resuming. */
            DEBUG_LOG_INFO("UpgradePartitionDataInit total requested data %u remaining data %u", 
                ctx->totalReqSize, ctx->totalReqSize - ctx->totalReceivedSize);
            UpgradePartitionDataRequestData(ctx->totalReqSize - ctx->totalReceivedSize, UpgradeCtxGet()->dfu_file_offset);
        }
        else
        {
            ctx->offset = UpgradeCtxGet()->dfu_file_offset;
        }
        /* A partial update has been interrupted. Don't erase. */
        DEBUG_LOG_INFO("UpgradePartitionDataInit: partial update interrupted. Not erasing, \
            newReqSize %d offset %d dfu_file_offset %d totalReqSize %d totalReceivedSize %d",
            ctx->newReqSize, ctx->offset, UpgradeCtxGet()->dfu_file_offset, ctx->totalReqSize , ctx->totalReceivedSize);
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);
        return TRUE;
    }
    /* if partition state is DIRTY then start Erase_image_header */
    else if((UpgradeCtxGetPSKeys()->state_of_partitions == UPGRADE_PARTITIONS_DIRTY) &&
        UpgradeFWIFEraseSection(UPGRADE_FW_IF_FILTER_ALTERNATE_IMAGE_HEADER) == UPGRADE_PARTITIONS_ERASING_HEADER)
    {
        UpgradeFWIFInit();
        /*these were initialized as part of UpgradePartitionDataInitHelper called from dfu domain*/
        DEBUG_LOG_INFO("UpgradePartitionDataInit, Post UpgradePartitionDataRequestData: \
                        UpgradePartitionDataCtx->partitionLength = %d, totalReqSize = %d, totalReceivedSize = %d, \
                        newReqSize = %d, offset = %d, state enum:UpgradePartitionDataState:%d",
                        ctx->partitionLength, ctx->totalReqSize, ctx->totalReceivedSize, 
                        ctx->newReqSize, ctx->offset, ctx->state);

        UpgradeCtxGetPSKeys()->state_of_partitions = UPGRADE_PARTITIONS_ERASING_HEADER;
        UpgradeSavePSKeys();
        UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_ERASE_IMAGE_HEADER);
        
        return TRUE;
    }
    return FALSE;
}


/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_ERASE_IMAGE_HEADER
*/
bool UpgradePartitionDataHandleEraseImageHeader(MessageId id, Message message)
{
    switch(id)
       {
           case MESSAGE_IMAGE_UPGRADE_ERASE_STATUS:
           {
               MessageImageUpgradeEraseStatus *msg = (MessageImageUpgradeEraseStatus *)message;
    
               DEBUG_LOG("UpgradePartitionDataHandleEraseImageHeader, erase_status %u", msg->erase_status);

               if (msg->erase_status)
               {
                   DEBUG_LOG_INFO("UpgradePartitionDataHandleEraseImageHeader,SQIF erased");

                   UpgradeCtxGetPSKeys()->state_of_partitions = UPGRADE_PARTITIONS_UPGRADING_HEADER;
                   UpgradeSavePSKeys();

                   /* The SQIF has been erased successfully. Request the first part of header. */
                   UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART);
                   UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE, 0);
                   MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);
                   return TRUE;
               }
               else
               {
                   /* Tell the host that the attempt to erase the SQIF failed. */
                   UpgradeFatalError(UPGRADE_HOST_ERROR_SQIF_ERASE);
               }
           }
           return FALSE;
    
           default:
               return FALSE;
       }
}

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_ERASE_BANK
*/
bool UpgradePartitionDataHandleEraseBank(MessageId id, Message message)
{
    switch(id)
       {
           case MESSAGE_IMAGE_UPGRADE_ERASE_STATUS:
           {
               MessageImageUpgradeEraseStatus *msg = (MessageImageUpgradeEraseStatus *)message;
               DEBUG_LOG("UpgradePartitionDataHandleEraseBank, erase_status %u", msg->erase_status);
               if (msg != NULL && msg->erase_status)
               {
                   DEBUG_LOG_INFO("UpgradePartitionDataHandleEraseBank,SQIF bank erased");
                   if(IsLastPartition())
                   {
                       /* Inform the application that we have received all data. */
                       UpgradeSendUpgradeStatusInd(UpgradeGetAppTask(), upgrade_state_download_completed, 0);
                       /* Move to hash checking as we have received all data. */
                       UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_WAIT_FOR_VALIDATION);
                    }
                   else
                    {
                       /* set partition state to UPGRADING */
                       UpgradeCtxGetPSKeys()->state_of_partitions = UPGRADE_PARTITIONS_UPGRADING;
                       UpgradeSavePSKeys();
                       /* The SQIF has been erased successfully. Request the first part of header. */
                       UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART);
                       UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE, 0);
                       MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);
                    }
                   return TRUE;
               }
               else
               {
                   /* Tell the host that the attempt to erase the SQIF failed. */
                   UpgradeFatalError(UPGRADE_HOST_ERROR_SQIF_ERASE);
               }
           }
           return FALSE;
    
           default:
               return FALSE;
       }
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

    /* APPUHDR6 */
    if(0 == strncmp((char *)data, UpgradePartitionDataGetHeaderID(), ID_FIELD_SIZE))
    {
        if (length < UPGRADE_HEADER_MIN_SECOND_PART_SIZE)
            return UPGRADE_HOST_ERROR_BAD_LENGTH_UPGRADE_HEADER;
        /* TODO: Need to handle in future */
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
        UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_HEADER);
        ctx->isUpgradeHdrAvailable = TRUE;
        ctx->partitionLength = length;
        DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleGeneric1stPartState,ctx->partitionLength %d", ctx->partitionLength);
    }
    /* PARTDATA */
    else if (0 == strncmp((char *)data, UpgradePartitionDataGetPartitionID(), ID_FIELD_SIZE))
    {
        if((UpgradeCtxGetFW()->partitionNum.partitionInstance.partitionID == UPGRADE_PARTITION_ZERO) && (!ctx->isUpgradeHdrAvailable))
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
        UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_DATA_HEADER);
        ctx->partitionLength = length - PARTITION_SECOND_HEADER_SIZE;
        DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleGeneric1stPartState,ctx->partitionLength %d", ctx->partitionLength);
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

    uint32 signatureOffset = 0;
    uint16 endCursor = 0; /* Pointer into variable length portion of header */
    uint8 *ptrHeader = NULL;
    UpgradeHostErrorCode headerParseResult;

    /* First save upgrade header in PsKey */
    rc = UpgradeSaveHeaderInPSKeys(data, len);
    if (UPGRADE_HOST_SUCCESS != rc)
        return rc;
    
    if (!reqComplete)
    {
        /* Handle the case where complete header is not received */
        DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleHeaderState, header not complete");
        return UPGRADE_HOST_SUCCESS;
    }

    /* Calculate signature offset */
    signatureOffset = ctx->partitionLength + HEADER_FIRST_PART_SIZE - UPGRADE_SIGNATURE_LENGTH;
    
    /* allocate memory for hash calculation of upgrade header 
       Gen 1st part(12) + upgrade header length
    */
    ptrHeader = malloc(ctx->partitionLength + HEADER_FIRST_PART_SIZE);
    if(!ptrHeader)
    {
        return UPGRADE_HOST_ERROR_NO_MEMORY;
    }
    DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleHeaderState, allocate memory of size %d",
                        ctx->partitionLength + HEADER_FIRST_PART_SIZE);

    /* Read complete upgrade header from PsKey along with signature */
    UpgradeReadPsKeyData(DFU_HEADER_PSKEY_START, 0, ctx->partitionLength + HEADER_FIRST_PART_SIZE, ptrHeader);
    
    /* Verify upgrade signature */
    if(!UpgradeFWIFSignatureVerify(UPGRADE_FW_IF_SHA384, ptrHeader, ctx->partitionLength - 
            UPGRADE_SIGNATURE_LENGTH + HEADER_FIRST_PART_SIZE, ptrHeader + signatureOffset, 
                    UPGRADE_SIGNATURE_LENGTH))
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleHeaderState, signature failed ");
        free(ptrHeader);
        return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_UPGRADE_HEADER; 
    }

    headerParseResult = UpgradePartitionDataParseCommonHeader(ptrHeader+HEADER_FIRST_PART_SIZE,
                                                              ctx->partitionLength,
                                                              &newVersion,
                                                              &newPSVersion,
                                                              &endCursor,
                                                              FALSE);

    if(headerParseResult != UPGRADE_HOST_SUCCESS)
    {
        free(ptrHeader);
        return headerParseResult;
    }

    /* Save the psKey offset for hash table */
    ctx->totalPartitions = ctx->pendingPartition = (ctx->partitionLength - UPGRADE_SIGNATURE_LENGTH - endCursor) / UPGRADE_HASH_SIZE;
    UpgradeSaveHashTableOffset(endCursor + HEADER_FIRST_PART_SIZE);
    DEBUG_LOG_INFO("UpgradePartitionDataHandleHeaderState, dfuHashTablePskey %d, \
                dfuHashTablePskeyOffset %d pending partitionCount %d endcursor %d" ,ctx->dfuHashTablePskey, 
                ctx->dfuHashTablePskeyOffset, ctx->pendingPartition, endCursor);
    
    /* Store the in-progress upgrade version */
    UpgradeCtxGetPSKeys()->version_in_progress.major = newVersion.major;
    UpgradeCtxGetPSKeys()->version_in_progress.minor = newVersion.minor;
    UpgradeCtxGetPSKeys()->config_version_in_progress = newPSVersion;

    DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleHeaderState, saving versions %u.%u, PS version %u",
                       newVersion.major, newVersion.minor, newPSVersion);

    UpgradeSavePSKeys();

    UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE, 0);
    UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART);
    MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);

    /* free allocated memory for hashing */
    free(ptrHeader);
    ptrHeader = NULL;

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
    uint32 firstWord = 0;
    uint8 retrievedHash[UPGRADE_HASH_SIZE];
    uint8 *partHeader = NULL;
    uint8 calculatedHash[UPGRADE_HASH_SIZE];
    bool isResultReady = FALSE;
    
    if (!reqComplete)
    {
        /* Handle the case where packet size is smaller than header size */
        DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleDataHeaderState, header not complete");
        return UpgradePartitionDataParseIncomplete(data, len);
    }

    partHeader = malloc(len + HEADER_FIRST_PART_SIZE);
    if(!partHeader)
    {
        return UPGRADE_HOST_ERROR_NO_MEMORY;
    }

    /* get last 12 bytes written for hash calculation from PsKey */
    UpgradeRequestLastPsKeyBytes(partHeader, HEADER_FIRST_PART_SIZE);

    memcpy(partHeader + HEADER_FIRST_PART_SIZE, data, len);

    /* Calculate the Hash for partition header */
    if(!UpgradeFWIFHashData(UPGRADE_FW_IF_SHA384, partHeader, len + HEADER_FIRST_PART_SIZE, 
        calculatedHash, UPGRADE_HASH_SIZE, &isResultReady))
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataHeaderState, hash failed");
        free(partHeader);
        partHeader = NULL;
        return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_PARTITION_HEADER1; 
    }
    
    /* free allocated memory for hashing */
    free(partHeader);
    partHeader = NULL;

    /* Get the partition header hash from PsKey */
    UpgradeReadPsKeyData(ctx->dfuHashTablePskey, ctx->dfuHashTablePskeyOffset,
                           UPGRADE_HASH_SIZE, retrievedHash);
    UpgradeIncreaseDfuHashTableCursor(UPGRADE_HASH_SIZE);
    if(0 != strncmp((char *)calculatedHash, (char *)retrievedHash, UPGRADE_HASH_SIZE))
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataHeaderState, hash comaparison failed");
        return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_PARTITION_HEADER1;
    }

    if (len < PARTITION_SECOND_HEADER_SIZE + FIRST_WORD_SIZE)
        return UPGRADE_HOST_ERROR_BAD_LENGTH_DATAHDR_RESUME;

    partNum = UpgradeFWIFBytesToUpgradePartition(data);
    
    DEBUG_LOG("UpgradePartitionDataHandleDataHeaderState PART_DATA: \
        partition number %u Instance ID %u", partNum.partitionInstance.partitionID, partNum.partitionInstance.instanceID);

    /* Save partition number and instance ID for further use */
    ctx->partitionNum = partNum;

   if (!IsValidPartitionNum(partNum.partitionInstance.partitionID))
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataHeaderState, partition %u, is not valid", partNum.partitionInstance.partitionID);
        return UPGRADE_HOST_ERROR_WRONG_PARTITION_NUMBER;
    }

    UpgradeHostErrorCode rc = UpgradeSaveHeaderInPSKeys(data, PARTITION_SECOND_HEADER_SIZE);
    if (UPGRADE_HOST_SUCCESS != rc)
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataHeaderState, \
            failed to store PSKEYSs, error %u", rc);
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

    if (UpgradeCtxGetPSKeys()->last_closed_partition.partitionInstance.partitionID > partNum.partitionInstance.partitionID)
    {
        DEBUG_LOG_INFO("UpgradePartitionDataHandleDataHeaderState, already \
            handled partition %u, skipping it", partNum.partitionInstance.partitionID);
        DEBUG_LOG_INFO("UpgradePartitionDataHandleDataHeaderState,ctx->partitionLength %d", ctx->partitionLength);
        UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE, ctx->partitionLength - FIRST_WORD_SIZE);
        UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART);
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);
        return UPGRADE_HOST_SUCCESS;
    }

    /* Verify physical partition size */
    if (ctx->partitionLength > UpgradeFWIFGetPhysPartitionSize(partNum))
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataHeaderState, partition size \
            mismatch, upgrade %lu, actual %lu", ctx->partitionLength, 
            UpgradeFWIFGetPhysPartitionSize(partNum));
        return UPGRADE_HOST_ERROR_PARTITION_SIZE_MISMATCH;
    }
    
    /* Partition could have been already open if we are resuming (without rebooting the device) 
       the upgrade so, check and use the partitionHdl if its non-zero. */
    if(!ctx->partitionHdl)
    {
        DEBUG_LOG_INFO("UpgradePartitionDataHandleDataHeaderState, open partition %d to write", partNum.partitionInstance.partitionID);
        ctx->partitionHdl = UpgradeFWIFPartitionOpen(partNum, firstWord);
    }
    if (!ctx->partitionHdl)
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataHeaderState, \
            failed to open partition %u", partNum.partitionInstance.partitionID);
        return UPGRADE_HOST_ERROR_PARTITION_OPEN_FAILED;
    }

    DEBUG_LOG_INFO("UpgradePartitionDataHandleDataHeaderState, partition length %lu", 
        ctx->partitionLength);
    if (FIRST_WORD_SIZE < ctx->partitionLength)
    {
       ctx->time_start = SystemClockGetTimerTime(); 

        /* Get partition data from the start, but skipping the first word. */
        UpgradePartitionDataRequestData(ctx->partitionLength - FIRST_WORD_SIZE, 0);
        UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_DATA);
    }
    else if (FIRST_WORD_SIZE == ctx->partitionLength)
    {
        /* A case when all data are in but partition is not yet closed */
        UpgradeHostErrorCode closeStatus = UpgradeFWIFPartitionClose(ctx->partitionHdl);
        ctx->partitionHdl = NULL;
        if (UPGRADE_HOST_SUCCESS != closeStatus)
        {
            DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataHeaderState, failed to close \
                partition %u, status %u", partNum.partitionInstance.partitionID, closeStatus);
            return closeStatus;
        }

        ctx->openNextPartition = TRUE;

        UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE, 0);
        UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART);
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
    UpgradePartitionDataHandleDataState  -  Partition data handling.

DESCRIPTION
    Writes data to a SQIF and sends it the hash verification. 

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode UpgradePartitionDataHandleDataState(const uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    bool isResultReady;
    UpgradeHostErrorCode closeStatus;

    if (len != UpgradeFWIFPartitionWrite(ctx->partitionHdl, data, len))
    {
        DEBUG_LOG_ERROR("UpgradePartitionDataHandleDataState, partition write failed, length %u",
                        len);
        return UPGRADE_HOST_ERROR_PARTITION_WRITE_FAILED_DATA;
    }

    if (reqComplete)
    {
        ctx->pendingPartition = ctx->pendingPartition - 1;
        DEBUG_LOG_INFO("UpgradePartitionDataHandleDataState, partition write complete,\
            Pending partitions:%d",ctx->pendingPartition);

        rtime_t duration_ms = rtime_sub(SystemClockGetTimerTime(), ctx->time_start) / 1000;
        uint32 bytes_per_sec = (ctx->totalReqSize * 1000) / duration_ms;
        DEBUG_LOG_INFO("UpgradePartitionDataHandleDataState, took %lu ms, %lu bytes/s", duration_ms, bytes_per_sec);

        /* close the writing completed partition */
        closeStatus = UpgradeFWIFPartitionClose(ctx->partitionHdl);
        DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleHashCheckData, partition close status is \
            enum:UpgradeHostErrorCode:%d", closeStatus);
        ctx->partitionHdl = NULL;
        ctx->openNextPartition = TRUE;
        if(closeStatus != UPGRADE_HOST_SUCCESS)
            return closeStatus;

        if(!UpgradeFWIFHashSection(UPGRADE_FW_IF_SHA384, ctx->partitionNum, NULL, 0, &isResultReady))
        {
            return UPGRADE_HOST_ERROR_INTERNAL_ERROR_1;
        }
        else
        {
            UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_HASH_CHECK_DATA);
        }
    }
    else
        DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleDataState, waiting for more data");

    if(IsLastPartition())
        return UPGRADE_HOST_DATA_TRANSFER_COMPLETE;
    return UPGRADE_HOST_SUCCESS;

}

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_HASH_CHECK_DATA
*/
bool UpgradePartitionDataHandleHashCheckData(MessageId id, Message message)
{
    UNUSED(message);
    uint8 retrievedHash[UPGRADE_HASH_SIZE];
    uint8 calculatedHash[UPGRADE_HASH_SIZE];
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();

    switch(id)
    {
    case MESSAGE_IMAGE_UPGRADE_HASH_ALL_SECTIONS_UPDATE_STATUS:

        /* Get the calculated hash from platform */
        UpgradeFWIFHashGetPendingResult(calculatedHash, UPGRADE_HASH_SIZE);

        /* Get stored hash from PsKey */
        UpgradeRequestLastPsKeyBytes(retrievedHash, UPGRADE_HASH_SIZE);
        if(0 != strncmp((char *)calculatedHash, (char *)retrievedHash, UPGRADE_HASH_SIZE))
        {
            DEBUG_LOG_ERROR("UpgradePartitionDataHandleHashCheckData, hash comaparison failed");
            UpgradeFatalError(UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_PARTITION_HEADER1);
        }

        if(UpgradeFWIFIsImageHeader(ctx->partitionNum))
        {
            /* Erase bank */
            if(UpgradeFWIFEraseSection(UPGRADE_FW_IF_FILTER_ALTERNATE_BANK_SECTIONS) == UPGRADE_PARTITIONS_ERASING)
            {
                UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_ERASE_BANK);
                /* set partition state to ERASING */
                UpgradeCtxGetPSKeys()->state_of_partitions = UPGRADE_PARTITIONS_ERASING;
                UpgradeSavePSKeys();
            }
            else
                return FALSE;
          
        }
        else if(IsLastPartition())
        {
            /* Inform the application that we have received all data. */
            UpgradeSendUpgradeStatusInd(UpgradeGetAppTask(), upgrade_state_download_completed, 0);
            /* Move to hash checking as we have received all data. */
            UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_WAIT_FOR_VALIDATION);
        }
        else
        {
            UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE, 0);
            UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART);
            MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REQUEST_DATA, NULL);
        }
        return TRUE;
          
    default :
        return FALSE;
    }
}

/*!
    @brief Handler for the UPGRADE_PARTITION_DATA_STATE_WAIT_FOR_VALIDATION
*/
bool UpgradePartitionDataHandleWaitForValidation(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
        case UPGRADE_PARTITION_DATA_INTERNAL_START_VALIDATION:
            DEBUG_LOG_VERBOSE("UpgradePartitionDataHandleWaitForValidation, start copying");
            ImageUpgradeCopy();
            UpgradePartitionDataSetState(UPGRADE_PARTITION_DATA_STATE_COPY);
        break;

        default : 
            UpgradeFatalError(UPGRADE_HOST_ERROR_UPDATE_FAILED);
    }
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

            if (msg->copy_status)
            {          
                /* The SQIF has been copied successfully.*/
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

bool UpgradePartitionDataHandleErase(MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(message);
    return FALSE;
}

bool UpgradePartitionDataHandleHashCheck(MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(message);
    return FALSE;

}

UpgradeHostErrorCode upgradeHandleFooterState(const uint8 *data, uint16 len, bool reqComplete)
{
    UNUSED(data);
    UNUSED(len);
    UNUSED(reqComplete);
    return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_FOOTER;
}

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
    return "APPUHDR6";
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

const char *UpgradePartitionDataGetFooterID(void)
{
    return FALSE;
}

bool UpgradeIsCaseDfuHeaderInPSKeys(void)
{
    return FALSE;
}


