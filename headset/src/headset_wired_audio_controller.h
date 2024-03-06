/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       headset_wired_audio_controller.h
\brief      Header file for headset wired audio controller module.
*/

#ifndef HEADSET_WIRED_AUDIO_CONTROLLER_H
#define HEADSET_WIRED_AUDIO_CONTROLLER_H

#ifdef ALLOW_WA_BT_COEXISTENCE
#define HeadsetWiredAudioController_Init()
#define HeadsetWiredAudioController_Disable()
#define HeadsetWiredAudioController_Enable()
#else
/*! \brief Headset Wired audio controller initialisation routine.*/
void HeadsetWiredAudioController_Init(void);

/*! \brief Routine to disable Headset Wired audio controller.*/
void HeadsetWiredAudioController_Disable(void);

/*! \brief Routine to enable Headset Wired audio controller.*/
void HeadsetWiredAudioController_Enable(void);
#endif /* ALLOW_WA_BT_COEXISTENCE */

#endif /* HEADSET_WIRED_AUDIO_CONTROLLER_H */

