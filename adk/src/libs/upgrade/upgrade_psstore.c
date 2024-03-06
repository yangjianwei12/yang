/****************************************************************************
Copyright (c) 2014 - 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_psstore.c

DESCRIPTION

    Implementation of an interface to Persistent Storage to get
    details of the file system and anything else related to the
    possibilities of upgrade.

NOTES
    Errors. Cause panics. End of. This behaviour in itself is problematic
            but if we are trying to switch applications then an error
            should indicate a reason to restart. We can't really
            delegate this to the VM app. We can't wind back to a previous
            application.
    Caching. Persistent store keys are not cached. There isn't a mechanism
            to subscribe to PSKEY changes. Since we don't actually expect
            to be called that frequently it makes sense to access the keys
            we need when we need them.
*/

#define DEBUG_LOG_MODULE_NAME upgrade
#include <logging.h>

#include <stdlib.h>
#include <string.h>
#include <csrtypes.h>
#include <panic.h>
#include <ps.h>
#include <print.h>
#include <upgrade.h>

#include "upgrade_ctx.h"
#include "upgrade_fw_if.h"
#include "upgrade_psstore.h"
#include "upgrade_psstore_priv.h"
#include "upgrade_partitions.h"

#define BYTES_TO_WORDS(_b_)       ((_b_+1) >> 1)
#define ID_FIELD_SIZE 8

static void loadUpgradeKey(UPGRADE_LIB_PSKEY *key_data, uint16 key, uint16 key_offset);

/****************************************************************************
NAME
    UpgradeSavePSKeys  -  Save our PSKEYS

DESCRIPTION
    Save our PSKEYS into Persistent Storage.

    The existing contents of the key are read first. If they chance not to 
    exist the the value we do not control are set to 0x0000 (deemed safer 
    than panicking or using a marker such as 0xFACE)

    Note that the upgrade library initialisation has guaranteed that the 
    the pskeys fit within the 64 words allowed.
    
    Although not technically part of our API, safest if we allow for the 
    PSKEY to be longer than we use.
*/
void UpgradeSavePSKeys(void)
{
    uint16 keyCache[PSKEY_MAX_STORAGE_LENGTH];
    uint16 min_key_length = UpgradeCtxGet()->upgrade_library_pskeyoffset
                                    +UPGRADE_PRIVATE_PSKEY_USAGE_LENGTH_WORDS;

    /* Clear the keyCache memory before it is used for reading and writing to
     * UPGRADE LIB PSKEY
     */
    memset(keyCache,0x0000,sizeof(keyCache));

    /* Find out how long the PSKEY is */
    uint16 actualLength = PsRetrieve(UpgradeCtxGet()->upgrade_library_pskey,NULL,0);
    if (actualLength)
    {
        PsRetrieve(UpgradeCtxGet()->upgrade_library_pskey,keyCache,actualLength);
    }
    else
    {
        if (UpgradeCtxGet()->upgrade_library_pskeyoffset)
        {
            /* Initialise the portion of key before us */
            memset(keyCache,0x0000,sizeof(keyCache));
        }
        actualLength = min_key_length;
    }

    /* Correct for too short a key */
    if (actualLength < min_key_length)
    {
        actualLength = min_key_length;
    }

    memcpy(&keyCache[UpgradeCtxGet()->upgrade_library_pskeyoffset],UpgradeCtxGetPSKeys(),
                UPGRADE_PRIVATE_PSKEY_USAGE_LENGTH_WORDS*sizeof(uint16));
    PsStore(UpgradeCtxGet()->upgrade_library_pskey,keyCache,actualLength);
}

/****************************************************************************
NAME
    UpgradePSSpaceForCriticalOperations

DESCRIPTION

    Checks whether there appears to be sufficient free space in the PSSTORE
    to allow upgrade PSKEY operations to complete.

RETURNS
    FALSE if insufficient space by some metric, TRUE otherwise.
*/
bool UpgradePSSpaceForCriticalOperations(void)
{
    uint16 keySize = UpgradeCtxGet()->upgrade_library_pskeyoffset 
                     + UPGRADE_PRIVATE_PSKEY_USAGE_LENGTH_WORDS;
    return (PsFreeCount(keySize) >= UPGRADE_PS_WRITES_FOR_CRITICAL_OPERATIONS);
}

/****************************************************************************
NAME
    UpgradeGetVersion

DESCRIPTION
    Get the version information from the PS keys.

*/
bool UpgradeGetVersion(uint16 *major, uint16 *minor, uint16 *config)
{
    bool result = FALSE;
    if (UpgradeIsInitialised())
    {
        *major  = UpgradeCtxGetPSKeys()->version.major;
        *minor  = UpgradeCtxGetPSKeys()->version.minor;
        *config = UpgradeCtxGetPSKeys()->config_version;
        result = TRUE;
    }

    return result;
}

/****************************************************************************
NAME
    UpgradeGetInProgressVersion

DESCRIPTION
    Get the "in progress" version information from the PS keys.

*/
bool UpgradeGetInProgressVersion(uint16 *major, uint16 *minor, uint16 *config)
{
    bool result = FALSE;
    if (UpgradeIsInitialised())
    {
        *major  = UpgradeCtxGetPSKeys()->version_in_progress.major;
        *minor  = UpgradeCtxGetPSKeys()->version_in_progress.minor;
        *config = UpgradeCtxGetPSKeys()->config_version_in_progress;
        result = TRUE;
    }

    return result;
}

/*! Hidden function used to reset state

    VM libraries do not allow for _protected header files. This function
    is purposefully not included in a header file.
 */
bool UpgradePSClearStore(void);
bool UpgradePSClearStore(void)
{
    /* This key is defined in header file. Clear regardless. */
    PsStore(UPGRADE_CONTEXT_KEY, NULL, 0);

    if (UpgradeIsInitialised())
    {
        /*Clearing all the PSKEY's except version and config_version.Since
         *UpgradeCtxClearPSKeys is taking care of UpgradeSavePSKeys, it is not required here.
         */
        memset(UpgradeCtxGetPSKeys()->logical_partition_state.status,0,sizeof(UpgradeCtxGetPSKeys()->logical_partition_state.status));
        memset(UpgradeCtxGetPSKeys()->future_partition_state.status,0,sizeof(UpgradeCtxGetPSKeys()->future_partition_state.status));
        UpgradeCtxClearPSKeys();

        return TRUE;
    }
    return FALSE;
}

/***************************************************************************
NAME
    UpgradeClearHeaderPSKeys

DESCRIPTION
    Clear all the PSKEYs which are used to store the DFU headers
*/
void UpgradeClearHeaderPSKeys(void)
{
    uint16 upgrade_pskey;

    DEBUG_LOG_INFO("UpgradeClearHeaderPSKeys: Clear Header PSKeys");
    
    for(upgrade_pskey = DFU_HEADER_PSKEY_START;
        upgrade_pskey <= DFU_HEADER_PSKEY_END; upgrade_pskey++)
    {
        PsStore(upgrade_pskey, NULL, 0);
    }
}

/***************************************************************************
NAME
    UpgradeSaveHeaderInPSKeys

DESCRIPTION
    Store the DFU headers in PSKEYs

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode UpgradeSaveHeaderInPSKeys(const uint8 *data, uint16 data_size)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    uint8 keyCache[PSKEY_MAX_STORAGE_LENGTH_IN_BYTES];
    
    do
    {
        uint16 data_write_size;

        /* Find out how many words are written into PSKEY already and read contents
         * into local cache */
        uint16 pskey_length = PsRetrieve(ctx->dfuHeaderPskey, NULL, 0);
        DEBUG_LOG_VERBOSE("UpgradeSaveHeaderInPSKeys, Current Header PS KEY = %d, len = %d words, offset = %d", ctx->dfuHeaderPskey, pskey_length, ctx->dfuHeaderPskeyOffset);

        if (pskey_length)
            PsRetrieve(ctx->dfuHeaderPskey, keyCache, pskey_length);

        /* Check and limit how much data can be written into this PSKEY */
        if (ctx->dfuHeaderPskeyOffset + data_size > PSKEY_MAX_STORAGE_LENGTH_IN_BYTES)
            data_write_size = PSKEY_MAX_STORAGE_LENGTH_IN_BYTES - ctx->dfuHeaderPskeyOffset;
        else
            data_write_size = data_size;

        /* To prevent re-writing of Header PS Key,
         * if header data is already stored in the PSKey, do not re-write it.
         * This situation can occur during DFU resume when some of the header
         * data was already stored before interruption.
         * NOTE : pskey_length is in words so converted it to bytes for the conditional check*/
        if(ctx->dfuHeaderPskeyOffset + data_write_size > (pskey_length<<1))
        {
            DEBUG_LOG_VERBOSE("UpgradeSaveHeaderInPSKeys, Writing header PS Key : %d from offset : %d of len : %d",ctx->dfuHeaderPskey, ctx->dfuHeaderPskeyOffset, data_write_size);
            /* Copy data into cache */
            memcpy(&keyCache[ctx->dfuHeaderPskeyOffset], data, data_write_size);

            /* Update PSKEY,  */
            pskey_length = BYTES_TO_WORDS(ctx->dfuHeaderPskeyOffset + data_write_size);
            if (pskey_length != PsStore(ctx->dfuHeaderPskey, keyCache, pskey_length))
            {
                DEBUG_LOG_ERROR("UpgradeSaveHeaderInPSKeys, PsStore failed, key_num %u, offset %u, length %u",
                                ctx->dfuHeaderPskey, ctx->dfuHeaderPskeyOffset, pskey_length);
                return UPGRADE_HOST_ERROR_PARTITION_WRITE_FAILED_DATA;
            }
        }
        ctx->dfuHeaderPskeyOffset += data_write_size;
        data += data_write_size;
        data_size -= data_write_size;

        /* If this PSKEY is now full, reset offset and advance PSKEY number
         * so that remainder is written into the next PSKEY */
        if (ctx->dfuHeaderPskeyOffset == PSKEY_MAX_STORAGE_LENGTH_IN_BYTES)
        {
            /* Move to next PSKEY */
            ctx->dfuHeaderPskey += 1;

            /* Reset offset */
            ctx->dfuHeaderPskeyOffset = 0;

            /* Return error if we've run out of PSKEYs */
            if (ctx->dfuHeaderPskey > DFU_HEADER_PSKEY_END)
            {
                DEBUG_LOG_ERROR("UpgradeSaveHeaderInPSKeys, no more PSKEYs available");
                return UPGRADE_HOST_ERROR_NO_MEMORY;
            }
        }
    } while (data_size != 0);

    DEBUG_LOG_INFO("UpgradeSaveHeaderInPSKeys, ctx->dfuHeaderPskey %d", ctx->dfuHeaderPskey);
    DEBUG_LOG_INFO("UpgradeSaveHeaderInPSKeys, ctx->dfuHeaderPskeyOffset %d", ctx->dfuHeaderPskeyOffset);
    
    return UPGRADE_HOST_SUCCESS;
}

bool UpgradeSearchIDInHeaderPSKeys(char *identifier)
{

    uint8 keyCache[ID_FIELD_SIZE];
    
    /* Find out how many words are written into PSKEY. */
    uint16 pskey_length = PsRetrieve(DFU_HEADER_PSKEY_START, NULL, 0);
    /* Convert the length from words to bytes. */
    pskey_length<<=1;
    if (pskey_length >= ID_FIELD_SIZE)
    {
        PsRetrieve(DFU_HEADER_PSKEY_START, keyCache, ID_FIELD_SIZE);
        
        if(0 == strncmp((char *)keyCache, (char *)identifier, ID_FIELD_SIZE))
        {
        
            return TRUE;
        }
    }
    return FALSE;
}

/****************************************************************************
NAME
    UpgradeSaveHashTableOffset  - Function to initialize PsKey and Offset for hash table  .

DESCRIPTION
    Initialize the PsKey and Offset for the hash table in PsKey

*/
void UpgradeSaveHashTableOffset(uint16 length)
{
    uint16 offset = 0;
    uint16 psKey = 0;
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();

    /* Calculate the total PsKey and Offset from the length */
    if(length > PSKEY_MAX_STORAGE_LENGTH_IN_BYTES)
        psKey = length / PSKEY_MAX_STORAGE_LENGTH_IN_BYTES;
    
    offset = length % PSKEY_MAX_STORAGE_LENGTH_IN_BYTES;

    ctx->dfuHashTablePskey = DFU_HEADER_PSKEY_START + psKey;
    ctx->dfuHashTablePskeyOffset = offset;
    DEBUG_LOG_INFO("UpgradeSaveHashTableOffset,dfuHashTablePskey %d, dfuHashTablePskeyOffset %d", 
        ctx->dfuHashTablePskey, ctx->dfuHashTablePskeyOffset);
}

/****************************************************************************
NAME
    UpgradeRequestLastPsKeybytes  - this function return number bytes requested from PsKey.
    PsKey data will be read backword using current value of dfuPskey and dfuPskeyOffset 

DESCRIPTION
    Returns requested bytes data stored in PsKey backward from Pskey and Offset

*/
bool UpgradeRequestLastPsKeyBytes(uint8 *buffer, uint16 reqSize)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    uint16 psKey, psKeyOffset;

    /* Check for valid buffer pointer */
    if(!buffer)
    {
        return FALSE;
    }

    DEBUG_LOG_INFO("UpgradeRequestLastPsKeybytes,Current PsKey %d, PsKeyOffset %d requested bytes %d",
        ctx->dfuHeaderPskey, ctx->dfuHeaderPskeyOffset, reqSize);

    /* Check if requested size is in two different PsKey */
    if((ctx->dfuHeaderPskeyOffset - reqSize) < 0)
    {
        /* Retrieve the data from previous PsKey */
        psKey = ctx->dfuHeaderPskey - 1;
        psKeyOffset = PSKEY_MAX_STORAGE_LENGTH_IN_BYTES + ctx->dfuHeaderPskeyOffset - reqSize;
    }
    else
    {   
        psKey = ctx->dfuHeaderPskey;
        psKeyOffset = ctx->dfuHeaderPskeyOffset - reqSize;
    }
    
    UpgradeReadPsKeyData(psKey, psKeyOffset, reqSize, buffer);
    return TRUE;
}

/****************************************************************************
NAME
    UpgradeReadPskeyData  
    
DESCRIPTION
    Read requested bytes from PsKey ID and offset onwards.
*/
bool UpgradeReadPsKeyData(uint16 psKeyID, uint16 psKeyOffset, uint16 reqSize, uint8 *buffer)
{
    uint16 offset = 0;
    uint16 len = 0;
    uint16 lenAvailable, copySize;
    uint8 keyCache[PSKEY_MAX_STORAGE_LENGTH_IN_BYTES];

    /* Check for valid buffer pointer */
    if(!buffer)
    {
        return FALSE;
    }
    
    DEBUG_LOG_VERBOSE("UpgradeReadPsKeyData, PSKEY %d, Offset %d, requested bytes %d", psKeyID, psKeyOffset, reqSize);
        
    while (reqSize > 0)
    {
        len = PsRetrieve(psKeyID, NULL, 0);
        if(len == 0)
        {
            return FALSE;
        }

        PsRetrieve(psKeyID, keyCache, len);

        lenAvailable = (len * 2) - psKeyOffset;
        copySize = reqSize > lenAvailable ? lenAvailable:reqSize ;
        memcpy(buffer+offset, keyCache+psKeyOffset, copySize);
        reqSize -= copySize ;
        psKeyOffset = 0;
        psKeyID++;
        offset += copySize;
        DEBUG_LOG_VERBOSE("UpgradeReadPsKeyData, PSKEY %d, Offset %d, copysize %d, available len %d remaining bytes %d buffer offset %d",
        psKeyID, psKeyOffset, copySize, lenAvailable, reqSize, offset);
    }
    return TRUE;
}

/****************************************************************************
NAME
    UpgradeIncreaseDfuHashTableCursor  
    
DESCRIPTION
    Increase the hash table pointer by number of bytes as argument
*/
void UpgradeIncreaseDfuHashTableCursor(uint16 updateLen)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    DEBUG_LOG_VERBOSE("UpgradeUpdateDfuHashTablePsKey, current hashTablePsKey %d, and hashTableOffset %d", 
    ctx->dfuHashTablePskey, ctx->dfuHashTablePskeyOffset);

    if(ctx->dfuHashTablePskeyOffset + updateLen >= PSKEY_MAX_STORAGE_LENGTH_IN_BYTES)
    {
        ctx->dfuHashTablePskey++;
        ctx->dfuHashTablePskeyOffset = ctx->dfuHashTablePskeyOffset + updateLen - PSKEY_MAX_STORAGE_LENGTH_IN_BYTES;
    }
    else
    {
        ctx->dfuHashTablePskeyOffset = ctx->dfuHashTablePskeyOffset + updateLen;
    }
    DEBUG_LOG_VERBOSE("UpgradeUpdateDfuHashTablePsKey, new hashTablePsKey %d, and hashTableOffset %d", 
    ctx->dfuHashTablePskey, ctx->dfuHashTablePskeyOffset);
}

/*
 * There is no support for PSKEY_FSTAB in CSRA68100.
 */

/****************************************************************************
NAME
    UpgradeLoadPSStore  -  Load PSKEY on boot

DESCRIPTION
    Save the details of the PSKEY and offset that we were passed on 
    initialisation, and retrieve the current values of the key.

    In the unlikely event of the storage not being found, we initialise
    our storage to 0x00 rather than panicking.
*/
void UpgradeLoadPSStore(uint16 dataPskey,uint16 dataPskeyStart)
{
    union {
        uint16      keyCache[PSKEY_MAX_STORAGE_LENGTH];
        FSTAB_COPY  fstab;
        } stack_storage;
    uint16 lengthRead;

    UpgradeCtxGet()->upgrade_library_pskey = dataPskey;
    UpgradeCtxGet()->upgrade_library_pskeyoffset = dataPskeyStart;

    /* Worst case buffer is used, so confident we can read complete key 
     * if it exists. If we limited to what it should be, then a longer key
     * would not be read due to insufficient buffer
     * Need to zero buffer used as the cache is on the stack.
     */
    memset(stack_storage.keyCache,0,sizeof(stack_storage.keyCache));
    lengthRead = PsRetrieve(dataPskey,stack_storage.keyCache,PSKEY_MAX_STORAGE_LENGTH);
    if (lengthRead)
    {
        memcpy(UpgradeCtxGetPSKeys(),&stack_storage.keyCache[dataPskeyStart],
                UPGRADE_PRIVATE_PSKEY_USAGE_LENGTH_WORDS*sizeof(uint16));
    }
    else
    {
        memset(UpgradeCtxGetPSKeys(),0x0000, sizeof(UPGRADE_LIB_PSKEY));
    }
}

/****************************************************************************
NAME
    UpgradePsRunningNewApplication
    
DESCRIPTION
    See if we are part way through an upgrade. This is done by a combination of
    the ImageUpgradeSwapTryStatus trap and - if that returns TRUE - querying the
    upgrade PS keys in the same manner as the non-CONFIG_HYDRACORE variant.

    This is used by the application during early boot to check if the
    running application is the upgraded one but it hasn't been committed yet.

    Note: This should only to be called during the early init phase, before
          UpgradeInit has been called.
    
RETURNS
    TRUE if the upgraded application is running but hasn't been
    committed yet. FALSE otherwise, or in the case of an error.
*/
bool UpgradePsRunningNewApplication(uint16 dataPskey, uint16 dataPskeyStart)
{
    bool result = ImageUpgradeSwapTryStatus();
    if (result)
    {
        /* We are in the process of upgrading but have not committed, yet */
        UPGRADE_LIB_PSKEY ps_key;

        loadUpgradeKey(&ps_key, dataPskey, dataPskeyStart);

        /* 
         * Don't need to check upgrade_in_progress_key as ImageUpgradeSwapTryStatus
         * having returned TRUE does in its stead.
         * Return true if the running application is newer than the previous one.
         */
        if (ps_key.version_in_progress.major > ps_key.version.major
                || (ps_key.version_in_progress.major == ps_key.version.major
                    && ps_key.version_in_progress.minor > ps_key.version.minor))
        {
            /* The result is already TRUE */
        }
        else
        {
            result = FALSE;
        }
    }
    return result;
}

/****************************************************************************
NAME
    UpgradePsGetResumePoint
    
DESCRIPTION
    Get the resume point directly from the pskey. This function is useful before
    the PSStore gets loaded in the RAM.
    
RETURNS
    Value of the resume point.
*/
uint16 UpgradePsGetResumePoint(uint16 dataPskey, uint16 dataPskeyStart)
{
    UPGRADE_LIB_PSKEY ps_key;
    loadUpgradeKey(&ps_key, dataPskey, dataPskeyStart);
    return ps_key.upgrade_in_progress_key;
}

static void loadUpgradeKey(UPGRADE_LIB_PSKEY *key_data, uint16 key, uint16 key_offset)
{
    uint16 lengthRead;
    uint16 keyCache[PSKEY_MAX_STORAGE_LENGTH];

    /* Worst case buffer is used, so confident we can read complete key 
     * if it exists. If we limited to what it should be, then a longer key
     * would not be read due to insufficient buffer
     * Need to zero buffer used as the cache is on the stack.
     */
    memset(keyCache, 0, sizeof(keyCache));
    lengthRead = PsRetrieve(key, keyCache, PSKEY_MAX_STORAGE_LENGTH);
    if (lengthRead)
    {
        memcpy(key_data, &keyCache[key_offset], sizeof(*key_data));
    }
    else
    {
        memset(key_data, 0x0000, sizeof(*key_data));
    }
}

