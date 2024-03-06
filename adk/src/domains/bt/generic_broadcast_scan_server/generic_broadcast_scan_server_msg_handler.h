/*!
   \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
               All Rights Reserved.\n
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \version    
   \addtogroup generic_broadcast_scan_server
   \brief      Header file for the Generic Broadcast Scan Server module.
   @{
*/

#ifndef GENERIC_GAIA_BROADCAST_SCAN_SERVER_MESSAGE_HANDLER_H_
#define GENERIC_GAIA_BROADCAST_SCAN_SERVER_MESSAGE_HANDLER_H_

#ifdef INCLUDE_GBSS
#include "generic_broadcast_scan_server_private.h"

void genericBroadcastScanServer_MessageHandler(Task task, MessageId id, Message message);

#endif /* INCLUDE_GBSS */
#endif /* GENERIC_GAIA_BROADCAST_SCAN_SERVER_MESSAGE_HANDLER_H_ */

/*! @} */