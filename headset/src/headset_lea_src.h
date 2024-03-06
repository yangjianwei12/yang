/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       headset_lea_src.h
\brief      Header file for Speaker LE Source audio functionality.

*/

#ifndef HEADSET_LEA_SRC_H
#define HEADSET_LEA_SRC_H

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

#include <types.h>

/*! \brief Starts Broadcasting of audio

     \param None
     \return None
*/
void HeadsetLeaSrc_AudioStart(void);

/*! \brief Stops Broadcasting of audio

     \param None
     \return None
*/
void HeadsetLeaSrc_AudioStop(void);

/*! \brief Initializes the Speaker LE Source Audio interface.

     \param init_task Task to receive responses.
     \return TRUE if able to initialize, returns FALSE otherwise.
*/
bool HeadsetLeaSrc_Init(void);

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#endif /* HEADSET_LEA_SRC_H */
