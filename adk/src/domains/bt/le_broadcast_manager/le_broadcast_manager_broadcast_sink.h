/*!
   \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
               All Rights Reserved.
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \file
   \addtogroup leabm
   \brief      LE Broadcast Manager interface with the BAP Broadcast 
               Sink role.
   @{
*/

#ifndef LE_BROADCAST_MANAGER_BROADCAST_SINK_H_
#define LE_BROADCAST_MANAGER_BROADCAST_SINK_H_

#include "le_broadcast_manager_data.h"
#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
#include "le_broadcast_manager_periodic_scan.h"
#endif

/*! \brief Initialises the BAP Broadcast Sink component.

 */
void LeBroadcastManager_BroadcastSinkInit(void);

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
le_bm_periodic_scan_callback_interface_t *leBroadcastManager_GetPeriodicScanInterface(void);
#endif /* INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN */

#endif /* LE_BROADCAST_MANAGER_BROADCAST_SINK_H_ */

/*! @} */