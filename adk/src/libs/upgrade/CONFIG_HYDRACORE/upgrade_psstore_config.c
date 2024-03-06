/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.


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
#include "upgrade_psstore.h"
#include "upgrade_ctx.h"

/****************************************************************************
NAME
    UpgradePartialUpdateInterrupted

DESCRIPTION
    Get the indication of whether a partial upgrade has been interrupted from the PS keys.

*/
bool UpgradePartialUpdateInterrupted(void)
{
    bool interrupted = FALSE;
    if ((UpgradeCtxGetPSKeys()->upgrade_in_progress_key == UPGRADE_RESUME_POINT_START) &&
        (UpgradeCtxGetPSKeys()->state_of_partitions == UPGRADE_PARTITIONS_UPGRADING) &&
        (UpgradeCtxGetPSKeys()->last_closed_partition.partitionID > 0))
    {
        /* A partial update has been interrupted. Don't erase. */
        interrupted = TRUE;
    }
    DEBUG_LOG_INFO("UpgradePartialUpdateInterrupted :  %d", interrupted);
    return interrupted;
}

