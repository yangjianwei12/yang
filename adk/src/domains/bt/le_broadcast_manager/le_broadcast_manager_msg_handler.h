/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup leabm
    \brief      Message Handler for LE Broadcast Manager.
    @{
*/

#ifndef LE_BROADCAST_MANAGER_MSG_HANDLER_H_
#define LE_BROADCAST_MANAGER_MSG_HANDLER_H_

#include "scan_delegator_role.h"

/*! \brief Message handler for LE Broadcast Manager.

 */
void LeBroadcastManager_MsgHandlerLeabmMessages(Task task, MessageId id, Message message);


/*! \brief Handles av and call related messages.

    Handles messages from the hfp_profile, av and mirror_profile modules so
    that le broadcast manager can know when a HFP call or A2DP media starts
    and ends.

    Eventually this should be replaced by the audio router pausing,
    interrupting and then resuming the audio_source_le_audio_broadcast source
    when another higher priority source is routed while it is active.

    \param id The message id.
    \param message The message payload.
*/
void LeBroadcastManager_MsgHandlerAvMessages(MessageId id, Message message);

/*! \brief Handles BAP CL and Connection Manager messages.

    \param id The message id.
    \param message The message payload.
*/

void LeBroadcastManager_MsgHandlerBapCmClPrim(MessageId id, void* message);

#endif /* LE_BROADCAST_MANAGER_MSG_HANDLER_H_ */
/*! @} */