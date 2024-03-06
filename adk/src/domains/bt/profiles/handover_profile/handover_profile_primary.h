/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup handover_profile
    \brief      The functionality to perform handover as a primary.
    @{
*/
#ifndef HANDOVER_PROTOCOL_PRIMARY_H_
#define HANDOVER_PROTOCOL_PRIMARY_H_

#ifdef INCLUDE_MIRRORING

#include "handover_profile.h"

/*! \brief Perform handover as the primary device.
    \return The status of the handover procedure, HANDOVER_PROFILE_STATUS_SUCCESS
    if the earbud is now secondary otherwise on of the error codes will be returned
    and the earbud will still be in the primary role.
*/
handover_profile_status_t handoverProfile_HandoverAsPrimary(void);

#endif
#endif
/*! @} */