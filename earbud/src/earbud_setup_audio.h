/*!
\copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    earbud
\brief      Pre and post init audio setup.
 
Functions to configure audio according to the application needs.
 
*/

#ifndef EARBUD_SETUP_AUDIO_H_
#define EARBUD_SETUP_AUDIO_H_

/*@{*/

/*! \brief Initialise/Setup audio components
    \param init_task Unused can be NULL
    \return TRUE on success, FALSE otherwise.
*/
bool AppSetupAudio_InitAudio(Task init_task);

/*@}*/

#endif /* EARBUD_SETUP_AUDIO_H_ */
