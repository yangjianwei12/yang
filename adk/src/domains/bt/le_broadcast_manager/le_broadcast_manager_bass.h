/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup leabm
    \brief      Header file for Broadcast Audio BASS Protected APIs.
    @{
*/

#ifndef LE_BROADCAST_MANAGER_BASS_H_
#define LE_BROADCAST_MANAGER_BASS_H_

#if defined(INCLUDE_LE_AUDIO_BROADCAST)

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)
/*! \brief Initialise Broadcast Manager BASS module.
*/
void LeBroadcastManager_BassInit(void);

/*! \brief Send source state changed notification to Broadcast Manager BASS cients.

    \param source_id Identifier for the source id to send the notification for
*/
void LeBroadcastManager_BassSendSourceStateChangedNotificationToClients(uint8 source_id);

#else /* INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN */

#define LeBroadcastManager_BassInit()

#define LeBroadcastManager_BassSendSourceStateChangedNotificationToClients(source_id) UNUSED(source_id)

#endif /* INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN */

#endif /* INCLUDE_LE_AUDIO_BROADCAST */
#endif /* LE_BROADCAST_MANAGER_BASS_H_ */

/*! @} */
