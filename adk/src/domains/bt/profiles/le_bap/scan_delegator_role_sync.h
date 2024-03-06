/*!
   \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
               All Rights Reserved.
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \file
   \addtogroup le_bap
   \brief Synchronising scan delegator state and events to the secondary earbud.
   @{
*/

#ifndef SCAN_DELEGATOR_ROLE_SYNC_H_
#define SCAN_DELEGATOR_ROLE_SYNC_H_

#if defined(INCLUDE_MIRRORING) && defined(INCLUDE_LE_AUDIO_BROADCAST)


#include <scan_delegator_role.h>

/*! Initialise the scan delegator sync component */
void LeBapScanDelegatorSync_Init(void);

#else

#define LeBapScanDelegatorSync_Init()

#endif

#endif /* SCAN_DELEGATOR_ROLE_SYNC_H_ */
/*! @} */