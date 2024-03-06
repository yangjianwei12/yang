/*******************************************************************************

Copyright (C) 2010 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Allocate and return a best effort flowspec

    We need flowspecs before we can create or move channels. It may be
    necessary to supply best-effort flowspecs in case the application
    doesn't care.
*/
#ifdef INSTALL_L2CAP_FLOWSPEC_SUPPORT
L2CA_FLOWSPEC_T *L2CA_AllocFlowspec(void)
{
    L2CA_FLOWSPEC_T *fs = pnew(L2CA_FLOWSPEC_T);
    fs->fs_identifier   = 1; /* Best effort always uses id 1 */
    fs->fs_service_type = L2CA_QOS_TYPE_BEST_EFFORT;
    fs->fs_max_sdu      = L2CA_FLOWSPEC_MAX_SDU;
    fs->fs_interarrival = L2CA_FLOWSPEC_INTERAR_DEFAULT;
    fs->fs_latency      = L2CA_FLOWSPEC_ACCESS_DEFAULT;
    fs->fs_flush_to     = L2CA_FLOWSPEC_FLUSH_TO_INFINITE;
    return fs;    
}
#endif /* FLOWSPEC_SUPPORT */
