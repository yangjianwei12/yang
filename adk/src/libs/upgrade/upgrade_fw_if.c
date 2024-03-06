/****************************************************************************
Copyright (c) 2014 - 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_fw_if.c

DESCRIPTION
    Implementation of functions which (largely) interact with the firmware.

NOTES

*/

#define DEBUG_LOG_MODULE_NAME upgrade
#include <logging.h>

#include <stdlib.h>
#include <string.h>
#include <byte_utils.h>
#include <csrtypes.h>
#include <panic.h>
#include <partition.h>
#include <sink.h>
#include <stream.h>
#include <imageupgrade.h>
#include "upgrade_partitions.h"
#include "upgrade_ctx.h"
#include "upgrade_psstore.h"
#include "upgrade_partitions.h"
#include "upgrade_private.h"
#include <upgrade_protocol.h>
#include "upgrade_fw_if.h"
#include "upgrade_fw_if_priv.h"

/******************************************************************************
NAME
    UpgradeFWIFInit

DESCRIPTION
    Initialise the context for the Upgrade FW IF.
*/
void UpgradeFWIFInit(void)
{
    UpgradeFWIFCtx *fwctx = UpgradeCtxGetFW();
    fwctx->vctx = NULL;
    memset(&fwctx->partitionNum, 0, sizeof(fwctx->partitionNum));
}

/******************************************************************************
NAME
    UpgradeFWIFGetDeviceVariant

DESCRIPTION
    Get the identifier for the current device variant.

RETURNS
    const char * Pointer to the device variant string.
*/
const char *UpgradeFWIFGetDeviceVariant(void)
{
    return ( const char * )UpgradeCtxGet()->dev_variant;
}

/******************************************************************************
NAME
    UpgradeFWIFGetAppVersion

DESCRIPTION
    Get the current (running) app version.

RETURNS
    uint16 The running app version.
*/
uint16 UpgradeFWIFGetAppVersion(void)
{
    return 2;
}

#ifndef UPGRADE_USE_FW_STUBS

/***************************************************************************
NAME
    UpgradeFWIFPartitionWrite

DESCRIPTION
    Write data to an open external flash partition. Each byte of the data
    is copied to the partition in a byte by byte copy operation.

PARAMS
    handle Handle to a writeable partition.
    data Pointer to the data to write.
    len Number of bytes (not words) to write.

RETURNS
    uint16 The number of bytes written, or 0 if there was an error.
*/
uint16 UpgradeFWIFPartitionWrite(UpgradeFWIFPartitionHdl handle, const uint8 *data, uint16 data_len)
{
    Sink sink = (Sink)(int)handle;
    if (!sink)
        return 0;

    uint16 data_remaining = data_len;
    while (data_remaining)
    {
        uint8 *dest = SinkMap(sink);
        if (dest)
        {
            uint16 sink_slack = SinkSlack(sink);
            const uint16 write_len = MIN(sink_slack, data_remaining);
            if (SinkClaim(sink, write_len) != 0xFFFF)
            {
                DEBUG_LOG_VERBOSE("UpgradeFWIFPartitionWrite, claimed %u bytes for writing", write_len);

                memmove(dest, data, write_len);
                if (SinkFlushBlocking(sink, write_len))
                {
                    data += write_len;
                    data_remaining -= write_len;
                }
                else
                {
                    DEBUG_LOG_ERROR("UpgradeFWIFPartitionWrite, failed to flush data to partition, length %u", write_len);
                    break;
                }
            }
            else
            {
                DEBUG_LOG_ERROR("UpgradeFWIFPartitionWrite, failed to claim %u bytes for writing", write_len);
                break;
            }

        }
        else
        {
            DEBUG_LOG_ERROR("UpgradeFWIFPartitionWrite, failed to map sink %p", (void *)sink);
            break;
        }
    }

    return data_len - data_remaining;
}


/***************************************************************************
NAME
    UpgradeFWIFPartitionClose

DESCRIPTION
    Close a handle to an external flash partition.

PARAMS
    handle Handle to a writeable partition.

RETURNS
    bool TRUE if a valid handle is given, FALSE otherwise.
*/
UpgradeHostErrorCode UpgradeFWIFPartitionClose(UpgradeFWIFPartitionHdl handle)
{

    Sink sink = (Sink)(int)handle;

    DEBUG_LOG_DEBUG("UpgradeFWIFPartitionClose");

    if (!sink)
    {
        return UPGRADE_HOST_SUCCESS;
    }

    if (!UpgradePSSpaceForCriticalOperations())
    {
        return UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED_PS_SPACE;
    }

    if (!SinkClose(sink))
    {
        DEBUG_LOG_ERROR("UpgradeFWIFPartitionClose: unable to close sink");
        return UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED;
    }
    /* last_closed_partition == partition_num + 1
     * so value 0 means no partitions were closed
     */
    UpgradeCtxGetPSKeys()->last_closed_partition = UpgradeFWIFIncreaseUpgradePartition(UpgradeCtxGetFW()->partitionNum);
    UpgradeSavePSKeys();
    DEBUG_LOG("UpgradeFWIFPartitionClose: last_closed_partition is %d", UpgradeFWIFSerializePartitionID(UpgradeCtxGetPSKeys()->last_closed_partition));

    return UPGRADE_HOST_SUCCESS;
}

#else /* UPGRADE_USE_FW_STUBS */

uint32 UpgradeFWIFGetPhysPartitionSize(UpgradePartition physPartition)
{
    UNUSED(physPartition);
    return 4000000;
}

UpgradeFWIFPartitionHdl UpgradeFWIFPartitionOpen(UpgradePartition physPartition, uint32 firstWord)
{
    UNUSED(firstWord);
    return (UpgradeFWIFPartitionHdl)(unsigned)physPartition;
}

uint16 UpgradeFWIFPartitionWrite(UpgradeFWIFPartitionHdl handle, const uint8 *data, uint16 len)
{
    UNUSED(handle);
    UpgradeCtxGetFW()->lastPartitionData = data;
    UpgradeCtxGetFW()->lastPartitionDataLen = len;
    return len;
}

UpgradeHostErrorCode UpgradeFWIFPartitionClose(UpgradeFWIFPartitionHdl handle)
{
    UNUSED(handle);
    return UPGRADE_HOST_SUCCESS;
}

#endif /* UPGRADE_USE_FW_STUBS */


/******************************************************************************
NAME
    UpgradeFWIFPartitionGetOffset
*/
uint32 UpgradeFWIFPartitionGetOffset(UpgradeFWIFPartitionHdl handle)
{
    return UpgradeFWIFGetSinkPosition((Sink)handle);
}

/******************************************************************************
NAME
    UpgradeFWIFGetSinkPosition
*/
uint32 UpgradeFWIFGetSinkPosition(Sink sink)
{
    uint32 offset = 0;
    bool result = ImageUpgradeSinkGetPosition(sink, &offset);
    DEBUG_LOG_DEBUG("ImageUpgradeSinkGetPosition(%p, @%p -> %ld) returns %d", sink, &offset, offset, result);
    return offset;
}

/****************************************************************************
NAME
    UpgradeIsRunningNewImage

DESCRIPTION
    See if we are have rebooted for an image upgrade.

RETURNS
    What the ImageUpgradeSwapTryStatus trap returns:
    TRUE if we have rebooted to try a new image and that new image is OK, else FALSE.
*/
bool UpgradeIsRunningNewImage(void)
{
    return ImageUpgradeSwapTryStatus();
}

